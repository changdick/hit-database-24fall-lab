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
    // 初始化file_handle和rid（指向第一个存放了记录的位置）
    
    // 从file_handle获取到页
    // file_handle_->fetch_page_handle()

    // 记录从第1页开始的，1开始
    rid_.page_no = 1;
    rid_.slot_no = -1;
    next(); // 设置第一页，slotno=-1开始调用next
}

/**
 * @brief 找到文件中下一个存放了记录的位置
 */
void RmScan::next() {
    // Todo:
    // 找到文件中下一个存放了记录的非空闲位置，用rid_来指向这个位置
    
    RmPageHandle page_handle = file_handle_->fetch_page_handle(rid_.page_no);
    int next_in_this_page = Bitmap::next_bit(true, page_handle.bitmap, file_handle_->file_hdr_.num_records_per_page, rid_.slot_no);
    if (next_in_this_page == file_handle_->file_hdr_.num_records_per_page) {
        // 说明这一页里面，没有了没有记录了
        // 得通过循环，在后面页里找
        if(rid_.page_no == (file_handle_->file_hdr_.num_pages - 1)) {
            //先判断当前页是不是最后一页，如果是，说明此时就是文件末尾
            rid_.slot_no = file_handle_->file_hdr_.num_records_per_page;
        } else //其他情况，跑下面循环找
        
        for (int i = rid_.page_no + 1; i < file_handle_->file_hdr_.num_pages; i++) {
            
            RmPageHandle page_handle = file_handle_->fetch_page_handle(i);
      
            int first_after_this = Bitmap::first_bit(true, page_handle.bitmap, file_handle_->file_hdr_.num_records_per_page);
            if(first_after_this == file_handle_->file_hdr_.num_records_per_page) {
                // 循环下去
                // 如果最后一页还没找到，直接放文件末尾
                if(i == (file_handle_->file_hdr_.num_pages - 1)) {
                    rid_.slot_no = file_handle_->file_hdr_.num_records_per_page;
                    rid_.page_no = file_handle_->file_hdr_.num_pages - 1;
                    break; 
                //   next（） 如果没有下一个，就不可以改rid_！！！ 不对，可以改！！！ 是后面判断错了！
                } 
            } else {
                // 说明找到了
                rid_.page_no = i;
                rid_.slot_no = first_after_this;
                break;
            }

        }
    } else {
        rid_.slot_no = next_in_this_page;
    }

}

/**
 * @brief ​ 判断是否到达文件末尾
 */
bool RmScan::is_end() const {
    // Todo: 修改返回值
    // RmPageHandle page_handle = file_handle_->fetch_page_handle(rid_.page_no);
    // int next_in_this_page = Bitmap::next_bit(true, page_handle.bitmap, file_handle_->file_hdr_.num_records_per_page, rid_.slot_no);

    // if (next_in_this_page == file_handle_->file_hdr_.num_records_per_page) {
    //     if(rid_.page_no == (file_handle_->file_hdr_.num_pages - 1)) {
    //         return true;
    //     } else
    //     for (int i = rid_.page_no + 1; i < file_handle_->file_hdr_.num_pages; i++) {
            
    //         RmPageHandle page_handle = file_handle_->fetch_page_handle(i);
      
    //         int first_after_this = Bitmap::first_bit(true, page_handle.bitmap, file_handle_->file_hdr_.num_records_per_page);
    //         if(first_after_this == file_handle_->file_hdr_.num_records_per_page) {
    //             // 循环下去
    //             // 如果最后一页还没找到,说明到文件末尾
    //             if(i == (file_handle_->file_hdr_.num_pages - 1)) {
    //                 return true;
    //             }
    //         } else {
    //             // 说明找到了
    //             break;
    //         }

    //     }
    // }
    // return false;
    return (rid_.page_no == (file_handle_->file_hdr_.num_pages - 1)) && (rid_.slot_no == (file_handle_->file_hdr_.num_records_per_page ));
}

/**
 * @brief RmScan内部存放的rid
 */
Rid RmScan::rid() const {
    return rid_;
}