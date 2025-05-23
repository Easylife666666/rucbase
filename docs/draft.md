### Notice
这是草稿，每一个都会随时扩充
试验报告中的模块接口部分不充足。其实这就是补充。
### draft
```cpp
config.h\
    #define BUFFER_LENGTH 8192

    static constexpr int HEADER_PAGE_ID = 0;                                      // the header page id
    static constexpr int PAGE_SIZE = 4096;                                        // size of a data page in byte  4KB
    static constexpr int BUFFER_POOL_SIZE = 65536;                                // size of buffer pool 256MB
    static constexpr int LOG_BUFFER_SIZE = (1024 * PAGE_SIZE);                    // size of a log buffer in byte
    static constexpr int BUCKET_SIZE = 50;                                        // size of extendible hash bucke

    using frame_id_t = int32_t;  // frame id type, 帧页ID, 页在BufferPool中的存储单元称为帧,一帧对应一页
    using page_id_t = int32_t;   // page id type , 页ID
    using txn_id_t = int32_t;    // transaction id type
    using lsn_t = int32_t;       // log sequence number type
    using slot_offset_t = size_t;  // slot offset type
    using oid_t = uint16_t;
    using timestamp_t = int32_t;  // timestamp type, used for transaction concurrency
```
#### 这个感觉像各种常量的声明。
##### ？
##### ？，页面大小，缓冲区大小，？，？
##### ？，？，？，？，？，？，？

```cpp
lru_replacer.cpp\
    LRUReplacer::LRUReplacer(size_t num_pages) { max_size_ = num_pages; }
    size_t LRUReplacer::Size() { return LRUlist_.size(); }
replacer.h\
    virtual bool victim(frame_id_t *frame_id) = 0;
    virtual void pin(frame_id_t frame_id) = 0;
    virtual void unpin(frame_id_t frame_id) = 0;
lru_replacer.h\
    std::mutex latch_;                  // 互斥锁
    std::list<frame_id_t> LRUlist_;     // 按加入的时间顺序存放unpinned pages的frame id，首部表示最近被访问
    std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> LRUhash_;   // frame_id_t -> unpinned pages的frame id
    size_t max_size_;   // 最大容量（与缓冲池的容量相同）
```
#### 这个是缓冲池替换策略管理器。
##### 声明与查看：
声明一个管理器，看看列表大小
##### 函数：
选出一个要被移除的frame，星标一个frame，不星标一个frame
##### 管理器参数：
互斥锁，LRU存储表，LRU存储表映射（能够快速找到它），最大页数

```cpp
disk_manager.cpp\
    DiskManager::DiskManager() { memset(fd2pageno_, 0, MAX_FD * (sizeof(std::atomic<page_id_t>) / sizeof(char))); }
disk_manager.h\
    void write_page(int fd, page_id_t page_no, const char *offset, int num_bytes);
    void read_page(int fd, page_id_t page_no, char *offset, int num_bytes);
    page_id_t allocate_page(int fd);
    void deallocate_page(page_id_t page_id);

    /*目录操作*/
    bool is_dir(const std::string &path);
    void create_dir(const std::string &path);
    void destroy_dir(const std::string &path);

    /*文件操作*/
    bool is_file(const std::string &path);
    void create_file(const std::string &path);
    void destroy_file(const std::string &path);
    int open_file(const std::string &path);
    void close_file(int fd);
    int get_file_size(const std::string &file_name);
    std::string get_file_name(int fd);
    int get_file_fd(const std::string &file_name);

    /*日志操作*/
    int read_log(char *log_data, int size, int offset);
    void write_log(char *log_data, int size);
    void SetLogFd(int log_fd) { log_fd_ = log_fd; }
    int GetLogFd() { return log_fd_; }
    void set_fd2pageno(int fd, int start_page_no) { fd2pageno_[fd] = start_page_no; }
    page_id_t get_fd2pageno(int fd) { return fd2pageno_[fd]; }

    static constexpr int MAX_FD = 8192;

   private:
    // 文件打开列表，用于记录文件是否被打开
    std::unordered_map<std::string, int> path2fd_;  //<Page文件磁盘路径,Page fd>哈希表
    std::unordered_map<int, std::string> fd2path_;  //<Page fd,Page文件磁盘路径>哈希表
    int log_fd_ = -1;                             // WAL日志文件的文件句柄，默认为-1，代表未打开日志文件
    std::atomic<page_id_t> fd2pageno_[MAX_FD]{};  // 文件中已经分配的页面个数，初始值为0
```
#### 这个是磁盘存储管理器。
##### 声明
##### 磁盘操作：
往磁盘里写字，读磁盘的字，分配页面，（这个不实现）
##### 文件夹操作：
有没有，创建，删除
##### 文件操作：
有没有，创建，删除，打开，关闭，文件大小，文件名，文件位置
##### 日志操作：
？，？，？，？，？，？
##### 最大页码数量
##### 磁盘管理器参数：
路径对位置，位置对路径，？，已分配页面个数。

```cpp
page.h\
    struct PageId {
        int fd;  //  Page所在的磁盘文件开启后的文件描述符, 来定位打开的文件在内存中的位置
        page_id_t page_no = INVALID_PAGE_ID;
    };

    class Page {
        friend class BufferPoolManager;
    public:
        Page() { reset_memory(); }
        PageId get_page_id() const { return id_; }
        inline char *get_data() { return data_; }
        bool is_dirty() const { return is_dirty_; }

        static constexpr size_t OFFSET_PAGE_START = 0;
        static constexpr size_t OFFSET_LSN = 0;
        static constexpr size_t OFFSET_PAGE_HDR = 4;
    private:
        void reset_memory() { memset(data_, OFFSET_PAGE_START, PAGE_SIZE); }  // 将data_的PAGE_SIZE个字节填充为0
        /** page的唯一标识符 */                 PageId id_;
        /** 该页面在bufferPool中的偏移地址 */    char data_[PAGE_SIZE] = {};
        /** 脏页判断 */                        bool is_dirty_ = false;
        /** The pin count of this page. */    int pin_count_ = 0;
```
#### 页面定义？
##### 页面ID结构体：
磁盘位置，？
##### 清理内存数据，得到页面ID，得到数据指针，得到是否脏数据
##### 页面起始位置，？，？
##### 清理内存数据，页面ID，数据，是否脏数据，固定了几个

```cpp
buffer_pool_manager.h\
    private:
    size_t pool_size_;      // buffer_pool中可容纳页面的个数，即帧的个数
    Page *pages_;           // buffer_pool中的Page对象数组，在构造空间中申请内存空间，在析构函数中释放，大小为BUFFER_POOL_SIZE
    std::unordered_map<PageId, frame_id_t, PageIdHash> page_table_; // 帧号和页面号的映射哈希表，用于根据页面的PageId定位该页面的帧编号
    std::list<frame_id_t> free_list_;   // 空闲帧编号的链表
    DiskManager *disk_manager_;
    Replacer *replacer_;    // buffer_pool的置换策略，当前赛题中为LRU置换策略
    std::mutex latch_;      // 用于共享数据结构的并发控制

   public:
    BufferPoolManager(size_t pool_size, DiskManager *disk_manager)
        : pool_size_(pool_size), disk_manager_(disk_manager) {
        // 为buffer pool分配一块连续的内存空间
        pages_ = new Page[pool_size_];
        // 可以被Replacer改变
        if (REPLACER_TYPE.compare("LRU"))
            replacer_ = new LRUReplacer(pool_size_);
        else if (REPLACER_TYPE.compare("CLOCK"))
            replacer_ = new LRUReplacer(pool_size_);
        else {
            replacer_ = new LRUReplacer(pool_size_);
        }
        // 初始化时，所有的page都在free_list_中
        for (size_t i = 0; i < pool_size_; ++i) {
            free_list_.emplace_back(static_cast<frame_id_t>(i));  // static_cast转换数据类型
        }
    }
```
#### 缓冲池管理器
##### 参数：页面数，页面数组，页面在哪里，空闲页，磁盘管理器，缓冲池策略管理器，并发锁
##### 声明：初始化管理器，并全放进空闲里。