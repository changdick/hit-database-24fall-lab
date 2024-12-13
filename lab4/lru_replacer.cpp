/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "lru_replacer.h"

LRUReplacer::LRUReplacer(size_t num_pages) { max_size_ = num_pages; }

LRUReplacer::~LRUReplacer() = default;  


/*
  LRU替换策略是通过两个表实现的
    std::list<frame_id_t> LRUlist_;    // unpinned pages的列表，存储的是帧id
        // std::list 是标准库的的双向链表 ,可以用迭代器迭代
        // 用这个实现记录使用的远近，可以在头部插入刚使用完（unpin的插入位置）的帧，最久没用（最远被unpin）的帧在尾部，淘汰时淘汰尾部的帧


    std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> LRUhash_; 
        // 一个从 frame_id 到 frame_id在列表中所在位置的迭代器 的哈希映射
        // 如果要查找，就用这个哈希表，实现O(1)时间的查找
*/


/**
 * @description: 使用LRU策略删除一个victim frame，并返回该frame的id（通过参数那个指针）
 * @param {frame_id_t*} frame_id 被移除的frame的id，如果没有frame被移除返回nullptr
 * @return {bool} 如果成功淘汰了一个页面则返回true，否则返回false
 */
bool LRUReplacer::victim(frame_id_t* frame_id) {
    // C++17 std::scoped_lock
    // 它能够避免死锁发生，其构造函数能够自动进行上锁操作，析构函数会对互斥量进行解锁操作，保证线程安全。
    std::scoped_lock lock{latch_};  //  如果编译报错可以替换成其他lock

    // Todo:
    //  利用lru_replacer中的LRUlist_,LRUHash_实现LRU策略
    //  选择合适的frame指定为淘汰页面,赋值给*frame_id

    if (LRUlist_.empty()) {
        return false;
    }

    *frame_id = LRUlist_.back();   // 将要淘汰的frame_id通过指针返回
    // 更新两表，移除该frame
    LRUhash_.erase(*frame_id);
    LRUlist_.pop_back();          

    return true;
    
}

/**
 * @description: 固定指定的frame，即该页面无法被淘汰
 * @param {frame_id_t} 需要固定的frame的id
 */
void LRUReplacer::pin(frame_id_t frame_id) {
    std::scoped_lock lock{latch_};
    // Todo:
    // 固定指定id的frame
    // 在数据结构中移除该frame

    // 本质上就是维护 LRUlist表，在表里面的元素是unpinned的frame

    // 哈希表用find方法，传入key，是查找key为该值的元素，会返回一个迭代器。
    // 哈希表的erase方法可以读入迭代器，删除指定位置元素；也可以读入key，删除指定key的元素。
    // 容器.end()是一个指向容器最后一个元素的下一个位置的迭代器，表示容器的结束位置。find找不到时返回end()。

    // list类型的erase只可以读入迭代器，删除指定位置元素。

    // 这里LRUhash_的value是LRUlist_的迭代器，因此LRUhash_[frame_id] 就是 frame_id 在 LRUlist_ 中的迭代器
    // 所以可以直接作为LRUlist.erase方法的参数输入。

    // 先查哈希表，看unpinned的表是否存在目标frame_id。若存在，就把他从LRUlist_中删除。同时哈希表也要删除
    if (LRUhash_.find(frame_id) != LRUhash_.end()) {
        LRUlist_.erase(LRUhash_[frame_id]);
        LRUhash_.erase(frame_id);
    }

}

/**
 * @description: 取消固定一个frame，代表该页面可以被淘汰
 * @param {frame_id_t} frame_id 取消固定的frame的id
 */
void LRUReplacer::unpin(frame_id_t frame_id) {
    // Todo:
    //  支持并发锁
    //  选择一个frame取消固定

    // unpin就是把目标frame_id加入到表中，根据约定，往头部插入

    // 对容器调用 .begin()返回头部位置的迭代器。 哈希表的value是迭代器，所以调用begin方法返回frame_id在list中的迭代器，作为哈希表的value

    std::scoped_lock lock{latch_};

    if (LRUhash_.find(frame_id) == LRUhash_.end()) {
        LRUlist_.push_front(frame_id);
        LRUhash_[frame_id] = LRUlist_.begin();
    }
}

/**
 * @description: 获取当前replacer中可以被淘汰的页面数量
 */
size_t LRUReplacer::Size() { return LRUlist_.size(); }
