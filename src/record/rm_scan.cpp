/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "rm_scan.h"
#include "rm_file_handle.h"

/**
 * @brief 初始化file_handle和rid
 * @param file_handle
 */
RmScan::RmScan(const RmFileHandle *file_handle) : file_handle_(file_handle) {
    // Todo:
    // 初始化file_handle和rid（指向第一个存放了记录的位置
    int slotPosi;
    if(file_handle->file_hdr_.num_pages > 1){
        for(int pageno = 1;pageno < file_handle->file_hdr_.num_pages;pageno++){
            if(file_handle->fetch_page_handle(pageno).page_hdr->num_records > 0){
                slotPosi = Bitmap::first_bit(1,file_handle->fetch_page_handle(pageno).bitmap,file_handle->file_hdr_.num_records_per_page);
                this->rid_=Rid{pageno,slotPosi};
                return;
            }
        }
    }
    this->rid_ = Rid{-1,-1};
    return;
}

/**
 * @brief 找到文件中下一个存放了记录的位置
 */
void RmScan::next() {
    // Todo:
    // 找到文件中下一个存放了记录的非空闲位置，用rid_来指向这个位置
    Rid rid=this->rid_;
    if(rid.page_no==RM_NO_PAGE)
        return;
    RmPageHandle rph=this->file_handle_->fetch_page_handle(rid.page_no);
    rid.slot_no=Bitmap::next_bit(1,rph.bitmap,rph.file_hdr->num_records_per_page,rid.slot_no);
    if(rid.slot_no==rph.file_hdr->num_records_per_page){
        do{
            rid.page_no++;
            if(rid.page_no == this->file_handle_->file_hdr_.num_pages){
                rid_=Rid{-1,-1};
                return;
            }
            rph=this->file_handle_->fetch_page_handle(rid.page_no);
            if(rph.page_hdr->num_records>0)
                break;
        }while(rid.page_no < this->file_handle_->file_hdr_.num_pages);
        rid.slot_no=Bitmap::first_bit(1,rph.bitmap,rph.file_hdr->num_records_per_page);
    }
    this->rid_=rid;
    return;
}

/**
 * @brief ​ 判断是否到达文件末尾
 */
bool RmScan::is_end() const {
    // Todo: 修改返回值
    return rid_.page_no == RM_NO_PAGE;
}

/**
 * @brief RmScan内部存放的rid
 */
Rid RmScan::rid() const {
    return rid_;
}