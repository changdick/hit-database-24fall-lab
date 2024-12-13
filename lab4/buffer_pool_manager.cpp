/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details.  哎纯逆天，早上8点半写到晚上7点半，任务1.2和1.3折腾一天，
fetchpage里面updatepage一个bug修六个多小时，服了，*/

#include "buffer_pool_manager.h"

/**
 * @description: 从free_list或replacer中得到可淘汰帧页的 *frame_id
 * 
 * @return {bool} true: 可替换帧查找成功 , false: 可替换帧查找失败
 * @param {frame_id_t*} frame_id 帧页id指针,返回成功找到的可替换帧id
 */
bool BufferPoolManager::find_victim_page(frame_id_t* frame_id) {
    // Todo:
    // 1 使用BufferPoolManager::free_list_判断缓冲池是否已满需要淘汰页面
    // 1.1 未满获得frame
    // 1.2 已满使用lru_replacer中的方法选择淘汰页面

    
    // free_list_是缓冲池中空闲帧的列表。刚创建的时候，pool_size个帧全在free_list_中。如果free_list_为空，就满了
    if(!free_list_.empty()) {
        // 未满，直接从free_list中取出一个帧，通过frame_id 返回
        *frame_id = free_list_.front(); // 返回空闲帧id列表第一个元素
        free_list_.pop_front(); // 从空闲帧id列表中删除该元素
        return true;
    } else {
        // 已满，使用lru_replacer选择淘汰页面
        if(replacer_->victim(frame_id)) {
            return true;
        }
    }

    return false;
}

/**
 * @description: 更新页面数据, 如果为脏页则需写入磁盘，再更新为新页面，更新page元数据(data, is_dirty, page_id)和page table
 * 描述：更新页面。调用update_page后，原本位置的页就被一个新页替换，iD、脏位、pin_count都是新的，数据刷成空的。
 * 在new_page中以及fetch_page当目标页不在内存中时调用，会根据脏位将原本的页写回磁盘，更新成一个新的空页。
 * 自动维护page_table_的映射关系。会自动删掉旧页的，添加新页的。
 * @param {Page*} page 写回页指针
 * @param {PageId} new_page_id 新的page_id
 * @param {frame_id_t} new_frame_id 新的帧frame_id
 */
void BufferPoolManager::update_page(Page *page, PageId new_page_id, frame_id_t new_frame_id) {
    // Todo:
    // 1 如果是脏页，写回磁盘，并且把dirty置为false
    // 2 更新page table
    // 3 重置page的data，更新page id
    
    page_table_.erase(page->get_page_id());  // 删除旧页的映射关系
    if (page->is_dirty()) {
        disk_manager_->write_page(page->get_page_id().fd, page->get_page_id().page_no, page->data_, PAGE_SIZE);
        page->is_dirty_ = false;
    }
    // 更新页面元数据
    page->id_ = new_page_id;     // 更新page id
    page->reset_memory();        // 重置data
    page_table_[new_page_id] = new_frame_id;   // 添加新页的映射关系

}

/**
 * @description: 从buffer pool获取需要的页。
 *              如果页表中存在page_id（说明该page在缓冲池中），并且pin_count++。
 *              如果页表不存在page_id（说明该page在磁盘中），则找缓冲池victim page，将其替换为磁盘中读取的page，pin_count置1。
 * @return {Page*} 若获得了需要的页则将其返回，否则返回nullptr
 * @param {PageId} page_id 需要获取的页的PageId
 */
Page* BufferPoolManager::fetch_page(PageId page_id) {
    //Todo:
    // 1.     从page_table_中搜寻目标页
    // 1.1    若目标页有被page_table_记录，则将其所在frame固定(pin)，并返回目标页。
    // 1.2    否则，尝试调用find_victim_page获得一个可用的frame，若失败则返回nullptr
    // 2.     若获得的可用frame存储的为dirty page，则须调用update_page将page写回到磁盘
    // 3.     调用disk_manager_的read_page读取目标页到frame
    // 4.     固定目标页，更新pin_count_
    // 5.     返回目标页
    std::scoped_lock lock{latch_};
    if(page_table_.count(page_id)) {
        // 1.1 page_table_中有目标页的记录
        replacer_->pin(page_table_[page_id]);   // 调用replacer中pin方法固定page所在frame
        pages_[page_table_[page_id]].pin_count_++; // pin_count自增
        return &pages_[page_table_[page_id]];

    }
    frame_id_t frame_id;
    // 尝试调用find_victim_page获得一个可用的frame，若失败则返回nullptr
    if(find_victim_page(&frame_id)) { 


        // 2. 若获得的可用frame存储的为dirty page，则须调用update_page将page写回到磁盘
        // if(pages_[frame_id].is_dirty()) {
        //     // is_dirty默认是false，一定是这个page被用过才会是true，就一定要写回
        //     update_page(&pages_[frame_id], page_id, frame_id);
            
        // } ！！！这一段逻辑是错的！！！ update_page包含了更新这一个frame里面，页的id、data、is_dirty等信息
        //   最重要的是ID， 如果检测到脏位才认为要调用update_page写回，那么页的ID没有更新！！！所以这是个假的新页
        //   本来update_page会判断脏位写回。 所以在这个位置无需判断本来的页是否有脏位。
        // 因为关键不在于本来的页要不要写回，而是这里要更新成一个新的页，元数据都应该是新的。
        // update_page的功能理解不够透彻导致的 

        // 将该frame存储的page换成一个新的页，即目标页
        update_page(&pages_[frame_id], page_id, frame_id);

        // 3. 调用disk_manager_的read_page读取目标页到frame
        disk_manager_->read_page(page_id.fd, page_id.page_no, pages_[frame_id].data_, PAGE_SIZE);
        // 4. 固定目标页，更新pin_count_
        replacer_->pin(page_table_[page_id]);
        pages_[frame_id].pin_count_ = 1;

        // ** 要建立新页的页帧id对应关系
        // page_table_[page_id] = frame_id; 在update里面自动完成

        // 5. 返回目标页
        return &pages_[frame_id];
    } 

    return nullptr;
}

/**
 * @description: 取消固定pin_count>0的在缓冲池中的page
 * @return {bool} 如果目标页的pin_count<=0则返回false，否则返回true
 * @param {PageId} page_id 目标page的page_id
 * @param {bool} is_dirty 若目标page应该被标记为dirty则为true，否则为false
 */
bool BufferPoolManager::unpin_page(PageId page_id, bool is_dirty) {
    // Todo:
    // 0. lock latch
    // 1. 尝试在page_table_中搜寻page_id对应的页P
    // 1.1 P在页表中不存在 return false
    // 1.2 P在页表中存在，获取其pin_count_
    // 2.1 若pin_count_已经等于0，则返回false
    // 2.2 若pin_count_大于0，则pin_count_自减一
    // 2.2.1 若自减后等于0，则调用replacer_的Unpin
    // 3 根据参数is_dirty，更改P的is_dirty_
    
    // page_table_[page_id] 就是缓冲池里pages_的下标

    std::scoped_lock lock{latch_};
    if(!page_table_.count(page_id)) {
        return false;
    } else {
        // 1.2 
        Page* page = pages_ + page_table_[page_id];
        if(page->pin_count_ <= 0) {
            return false;
        } else {
            // 2.2
            page->pin_count_--;
            if(!page->is_dirty())
                page->is_dirty_ = is_dirty;     // 稍微改一下，脏位只能0改1，不能1改成0
            if(page->pin_count_ == 0) {
                replacer_->unpin(page_table_[page_id]);
            }
        }
        
    }

    return true;
}

/**
 * @description: 将目标页写回磁盘，不考虑当前页面是否正在被使用
 * @return {bool} 成功则返回true，否则返回false(只有page_table_中没有目标页时)
 * @param {PageId} page_id 目标页的page_id，不能为INVALID_PAGE_ID
 */
bool BufferPoolManager::flush_page(PageId page_id) {
    // Todo:
    // 0. lock latch
    // 1. 查找页表,尝试获取目标页P
    // 1.1 目标页P没有被page_table_记录 ，返回false
    // 2. 无论P是否为脏都将其写回磁盘。
    // 3. 更新P的is_dirty_
    std::scoped_lock lock{latch_};
    if(!page_table_.count(page_id) && page_id.page_no != INVALID_PAGE_ID) {
        return false;
    } else {
        Page* page = &pages_[page_table_[page_id]];
        disk_manager_->write_page(page_id.fd, page_id.page_no, page->data_, PAGE_SIZE);
        page->is_dirty_ = false;
    }
   
    return true;
}

/**
 * @description: 创建一个新的page，即从磁盘中移动一个新建的空page到缓冲池某个位置。
 * @return {Page*} 返回新创建的page，若创建失败则返回nullptr
 * @param {PageId*} page_id 当成功创建一个新的page时存储其page_id （用来返回的）
 */
Page* BufferPoolManager::new_page(PageId* page_id) {
    // 1.   获得一个可用的frame，若无法获得则返回nullptr
    // 2.   在fd对应的文件分配一个新的page_id
    // 3.   将frame的数据写回磁盘
    // 4.   固定frame，更新pin_count_
    // 5.   返回获得的page
    std::scoped_lock lock{latch_};
    frame_id_t frame_id;
    if (find_victim_page(&frame_id)) {
        
        page_id->page_no = disk_manager_->allocate_page(page_id->fd);
        if (page_id->page_no == INVALID_PAGE_ID) {
            return nullptr; // 分配页面失败
        }
        update_page(&pages_[frame_id], *page_id, frame_id);

        // page_table_[*page_id] = frame_id; update方法里面会完成
        pages_[frame_id].pin_count_ = 1; // 设置pin_count
        return &pages_[frame_id];
        

    }

   return nullptr;
}

/**
 * @description: 从buffer_pool删除目标页
 * @return {bool} 如果目标页不存在于buffer_pool或者成功被删除则返回true，若其存在于buffer_pool但无法删除则返回false
 * @param {PageId} page_id 目标页
 */
bool BufferPoolManager::delete_page(PageId page_id) {
    // 1.   在page_table_中查找目标页，若不存在返回true
    // 2.   若目标页的pin_count不为0，则返回false
    // 3.   将目标页数据写回磁盘，从页表中删除目标页，重置其元数据，将其加入free_list_，返回true
    std::scoped_lock lock{latch_};
    if(page_table_.count(page_id)) {
        Page* page = &pages_[page_table_[page_id]];
        if(page->pin_count_ != 0) {
            return false;
        }
        if(page->is_dirty()) {
            disk_manager_->write_page(page_id.fd, page_id.page_no, page->data_, PAGE_SIZE);
            page->is_dirty_ = false;
        }
        page->reset_memory();   
        free_list_.push_back(page_table_[page_id]);
        page_table_.erase(page_id);           

        return true;
    }
    return true;
}

/**
 * @description: 将指定文件fd存在于buffer_pool中的所有页写回到磁盘
 * @param {int} fd 文件句柄
 */
void BufferPoolManager::flush_all_pages(int fd) {
    // 1.   遍历page_table_，找到所有属于fd的页
    // 2.   将这些页写回磁盘
    for(auto it = page_table_.begin(); it != page_table_.end(); it++) {
        if(it->first.fd == fd) {
            flush_page(it->first);
        }
    }
    
    
    
}