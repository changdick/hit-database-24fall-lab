/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "rm_file_handle.h"

/**
 * @description: 获取当前表中记录号为rid的记录
 * @param {Rid&} rid 记录号，指定记录的位置
 * @param {Context*} context
 * @return {unique_ptr<RmRecord>} rid对应的记录对象指针
 */
std::unique_ptr<RmRecord> RmFileHandle::get_record(const Rid& rid, Context* context) const {
    // Todo:
    // 1. 获取指定记录所在的page handle
    // 2. 初始化一个指向RmRecord的指针（赋值其内部的data和size）
    // PageId pageid_ = PageId{.fd = fd_, .page_no = rid.page_no};

    if (is_record(rid)) {
        RmPageHandle page_handle = fetch_page_handle(rid.page_no);
        RmRecord * record = new RmRecord(file_hdr_.record_size, page_handle.get_slot(rid.slot_no));// RmRecord的构造方法，传入record_size和slot的地址，slot的地址可以用方法获取
        return std::unique_ptr<RmRecord>(record);
    }

    return nullptr;
}

/**
 * @description: 在当前表中插入一条记录，不指定插入位置
 * @param {char*} buf 要插入的记录的数据
 * @param {Context*} context
 * @return {Rid} 插入的记录的记录号（位置）
 */
Rid RmFileHandle::insert_record(char* buf, Context* context) {
    // Todo:
    // 1. 获取当前未满的page handle
    // 2. 在page handle中找到空闲slot位置
    // 3. 将buf复制到空闲slot位置
    // 4. 更新page_handle.page_hdr中的数据结构
    // 注意考虑插入一条记录后页面已满的情况，需要更新file_hdr_.first_free_page_no

    RmPageHandle page_handle = create_page_handle(); // 获取一个page_handle,不指定

    // 用bitmap类的静态方法找到第一个空槽
    int free_slot = Bitmap::first_bit(false, page_handle.bitmap, file_hdr_.num_records_per_page);

    //新纪录就插入free_slot对应位置
    // 通过get_slot方法来得到插入数据的首地址
    char* slot = page_handle.get_slot(free_slot);

    // 用memcpy来把数据复制上去
    memcpy(slot, buf, file_hdr_.record_size);

    Bitmap::set(page_handle.bitmap, free_slot); // 设置bitmap中的对应位为1
    // 4 更新页头
    page_handle.page_hdr->num_records++; // 更新记录数

    // 构建返回的rid
    Rid rid = {.page_no = page_handle.page->get_page_id().page_no, .slot_no = free_slot};

    // 检查是否已满
    //page_handle.page_hdr->num_records == file_hdr_.num_records_per_page  这个条件应该是等价
    if (Bitmap::first_bit(false, page_handle.bitmap, file_hdr_.num_records_per_page) == file_hdr_.num_records_per_page) {
        //如果满了，要更新 第一个空闲页
        file_hdr_.first_free_page_no = page_handle.page_hdr->next_free_page_no;
    }

    return rid;
}

/**
 * @description: 在当前表中的指定位置插入一条记录
 * @param {Rid&} rid 要插入记录的位置
 * @param {char*} buf 要插入记录的数据
 */
void RmFileHandle::insert_record(const Rid& rid, char* buf) {
    if(is_record(rid)) return; // 我们预期rid位置不应该有记录，如果有的话就不对了

    RmPageHandle page_handle = fetch_page_handle(rid.page_no);
    char* slot = page_handle.get_slot(rid.slot_no); 
    memcpy(slot, buf, file_hdr_.record_size);
    // set bitmap
    Bitmap::set(page_handle.bitmap, rid.slot_no);
    page_handle.page_hdr->num_records++; // 更新记录数

    // 也要检查是否已满
    if (page_handle.page_hdr->num_records == file_hdr_.num_records_per_page) {
        // 如果满了，要更新  前面所有页的下一个空闲
        int page_no_this = rid.page_no; 
        for (int i = page_no_this - 1; i > 0; i--) {
            // 取页检查 
            RmPageHandle former_page_handle = fetch_page_handle(i);
            if (former_page_handle.page_hdr->next_free_page_no == page_no_this) {
                former_page_handle.page_hdr->next_free_page_no = page_handle.page_hdr->next_free_page_no;
            } else {
                break;
            }
        }
        if (file_hdr_.first_free_page_no == page_no_this) {
            file_hdr_.first_free_page_no = page_handle.page_hdr->next_free_page_no;
        }
    }
}

/**
 * @description: 删除记录文件中记录号为rid的记录
 * @param {Rid&} rid 要删除的记录的记录号（位置）
 * @param {Context*} context
 */
void RmFileHandle::delete_record(const Rid& rid, Context* context) {
    // Todo:
    // 1. 获取指定记录所在的page handle
    // 2. 更新page_handle.page_hdr中的数据结构
    // 注意考虑删除一条记录后页面未满的情况，需要调用release_page_handle()

    // 因为要考虑是不是已满删后变成未满，所以要看看page是不是满的
    
    // get page handle
    RmPageHandle page_handle = fetch_page_handle(rid.page_no);

    
    // get slot
    char* slot = page_handle.get_slot(rid.slot_no);
    // reset this slot
    Bitmap::reset(page_handle.bitmap, rid.slot_no);
    // slot里面具体数据好像不用改，只要改掉bitmap等记录，就可以看作是删掉了
    // 检查删前是不是满的
    if(page_handle.page_hdr->num_records == file_hdr_.num_records_per_page) {
        release_page_handle(page_handle);
    }
    // 更新记录数
    page_handle.page_hdr->num_records--;
    
}


/**
 * @description: 更新记录文件中记录号为rid的记录
 * @param {Rid&} rid 要更新的记录的记录号（位置）
 * @param {char*} buf 新记录的数据
 * @param {Context*} context
 */
void RmFileHandle::update_record(const Rid& rid, char* buf, Context* context) {
    // Todo:
    // 1. 获取指定记录所在的page handle
    // 2. 更新记录

    RmPageHandle page_handle = fetch_page_handle(rid.page_no);

    char * slot = page_handle.get_slot(rid.slot_no);

    memcpy(slot, buf, file_hdr_.record_size); // 更新记录

}

/**
 * 以下函数为辅助函数，仅提供参考，可以选择完成如下函数，也可以删除如下函数，在单元测试中不涉及如下函数接口的直接调用
*/
/**
 * @description: 获取指定页面的页面句柄
 * @param {int} page_no 页面号
 * @return {RmPageHandle} 指定页面的句柄
 */
RmPageHandle RmFileHandle::fetch_page_handle(int page_no) const {
    // Todo:
    // 使用缓冲池获取指定页面，并生成page_handle返回给上层
    // if page_no is invalid, throw PageNotExistError exception
    
    Page* page_;
    page_ = buffer_pool_manager_->fetch_page(PageId{fd_, page_no});
    if (!page_) {
        throw PageNotExistError("1",page_no);
    }

    return RmPageHandle(&file_hdr_, page_);
}

/**
 * @description: 创建一个新的page handle
 * @return {RmPageHandle} 新的PageHandle
 */
RmPageHandle RmFileHandle::create_new_page_handle() {
    // Todo:
    // 1.使用缓冲池来创建一个新page
    // 2.更新page handle中的相关信息
    // 3.更新file_hdr_
    PageId new_page_id = {.fd = fd_, .page_no = INVALID_PAGE_ID};  // 先创建PageId，再传入new_page,new_page里面会自己分配id号
    Page * page_ = buffer_pool_manager_->new_page(&new_page_id);
    RmPageHandle new_page_handle = RmPageHandle(&file_hdr_, page_);
    
    file_hdr_.first_free_page_no = file_hdr_.num_pages;   // 如果这种情况应该新页就是文件头的第一个空闲页，它的页号应该正好是新增前的总页数
    file_hdr_.num_pages++; // 创建了新页，更新文件头信息


    // 得手动设置页头
    new_page_handle.page_hdr->next_free_page_no = RM_NO_PAGE;
    new_page_handle.page_hdr->num_records = 0;

    return new_page_handle;
}

/**
 * @brief 创建或获取一个空闲的page handle
 *
 * @return RmPageHandle 返回生成的空闲page handle
 * @note pin the page, remember to unpin it outside!
 */
RmPageHandle RmFileHandle::create_page_handle() {
    // Todo:
    // 1. 判断file_hdr_中是否还有空闲页
    //     1.1 没有空闲页：使用缓冲池来创建一个新page；可直接调用create_new_page_handle()
    //     1.2 有空闲页：直接获取第一个空闲页
    // 2. 生成page handle并返回给上层
    
    if (file_hdr_.first_free_page_no == RM_NO_PAGE) {
        return create_new_page_handle(); // 调用create_new_page_handle()创建新页
    } else {
        return fetch_page_handle(file_hdr_.first_free_page_no);
    }

}

/**
 * @description: 当一个页面从没有空闲空间的状态变为有空闲空间状态时，更新文件头和页头中空闲页面相关的元数据
 */
void RmFileHandle::release_page_handle(RmPageHandle&page_handle) {
    // Todo:
    // 当page从已满变成未满，考虑如何更新：
    // 1. page_handle.page_hdr->next_free_page_no
    // 2. file_hdr_.first_free_page_no

    //fist_free_page_no 文件中当前第一个包含空闲空间的页面号（初始化为-1）
    // file_hdr_.first_free_page_no = page_handle.page->get_page_id().page_no; // 如果这个变成有空闲，他么他就是这个文件第一个空闲的
    // page_handle.page_hdr->next_free_page_no = file_hdr_.first_free_page_no;

    int page_no_this = page_handle.page->get_page_id().page_no;
    for (int i = page_no_this - 1; i > 0; i--) {
        RmPageHandle former_page_handle = fetch_page_handle(i);
        if (former_page_handle.page_hdr->next_free_page_no > page_no_this) {
            former_page_handle.page_hdr->next_free_page_no = page_no_this;
        } else {
            break;
        }
    }
    if(file_hdr_.first_free_page_no > page_no_this) {
        file_hdr_.first_free_page_no = page_no_this;
    }
    
}