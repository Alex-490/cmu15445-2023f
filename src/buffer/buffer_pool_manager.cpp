//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"

#include "common/exception.h"
#include "common/macros.h"
#include "storage/page/page_guard.h"

namespace bustub {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_scheduler_(std::make_unique<DiskScheduler>(disk_manager)), log_manager_(log_manager) {
  // TODO(students): remove this line after you have implemented the buffer pool manager
  throw NotImplementedException(
      "BufferPoolManager is not implemented yet. If you have finished implementing BPM, please remove the throw "
      "exception line in `buffer_pool_manager.cpp`.");

  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() { delete[] pages_; }


/*
别人说：“我要一个全新的页面。”

你需要：
1. 找一个空闲的内存位置。
2. 如果没有空闲位置，就淘汰一个可以淘汰的页面。
3. 如果被淘汰的页面是脏页，先写回磁盘。
4. 分配一个全新的 Page ID。
5. 清空内存位置中的旧数据。
6. 把新页面登记到管理结构里，返回给调用者。
注意：创建新页面不等于从磁盘读取旧页面，而是分配一个新的页面编号和内存空间。
*/
auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  std::lock_guard<std::mutex> lock(latch_);
  
  //获取帧，要么从空闲帧里找，要么使用replacer的evict淘汰一个,淘汰帧记得处理刷盘失败
  frame_id_t f=-1;
  if(!free_list_.empty()){
    f=free_list_.back();
    free_list_.pop_back();
  }else{
    if(!safeFuncOfFrameFromEvict(&f)){
      return nullptr;
    }
  }
  
  //到这里帧pages[f]准备好了
  //获取新页的id
  Page* page=&pages_[f];
  page->page_id_=AllocatePage();
  page->pin_count_=1;
  page->is_dirty_=false;
  page->ResetMemory();
  
  //页号传出
  *page_id=page->page_id_;
  //需要记录的记录
  page_table_.emplace(page->page_id_,f);
  replacer_->RecordAccess(f);

  return page;
}

/*
别人说：“我要使用磁盘上的 Page 5。”

你需要：
1. 先检查 Page 5 是否已经在内存里。
2. 如果已经在，就直接返回它，并增加它的使用计数（Pin Count）。
3. 如果不在，就找一个空闲的内存位置。
4. 如果没有空闲位置，就让 LRU-K 帮你挑一个可以淘汰的页面。
5. 如果被淘汰的页面修改过，就先写回磁盘。
6. 把 Page 5 从磁盘读进内存，返回给调用者。
如果所有页面都正在被使用，无法淘汰，就返回 nullptr。
*/
auto BufferPoolManager::FetchPage(page_id_t page_id, [[maybe_unused]] AccessType access_type) -> Page * {
  std::lock_guard<std::mutex> lock(latch_);

  //先看bufferpool里面有没有
  auto it=page_table_.find(page_id);
  
  //有就先处理再返回
  if(it!=page_table_.end()){
    Page* page=&pages_[it->second];
    //如果之前是可淘汰（驱逐）状态（evictable），那么要先设置为！evictable
    if(page->pin_count_==0){
      replacer_->SetEvictable(it->second,false);
    }
    //添加lruknode的访问记录
    replacer_->RecordAccess(it->second);//lruknode是为了方便使用lruk算法淘汰某个已经evictable帧
    //页的引用计数
    page->pin_count_++;//页的引用计数是为了确定某页的evictable状态
    return page;
  }

  //没有那就找个帧位调进去
  //先看空闲帧链表有没有，没有就调用evict
  frame_id_t f=-1;
  if(!free_list_.empty()){
    f=free_list_.back();
    free_list_.pop_back();
  }else{
    if(!safeFuncOfFrameFromEvict(&f)){
      return nullptr;
    }
  }

  pages_[f].page_id_=page_id;
  pages_[f].pin_count_=1;
  pages_[f].is_dirty_=false;
  pages_[f].ResetMemory();

  //从磁盘写入参数页
  //创建read请求
  DiskRequest r;
  std::promise<bool> p;
  auto future=p.get_future();
  r.callback_=std::move(p);
  r.data_=pages_[f].data_;//写入的位置
  r.is_write_=false;
  r.page_id_=page_id;

  //把请求放入请求队列
  disk_scheduler_->Schedule(std::move(r));

  //等待请求执行完毕
  //执行结果
  bool success=future.get();
  //失败了别忘了保存帧f
  if(!success){
    FrameReset(&pages_[f]);

    free_list_.push_front(f);
    return nullptr;
  }
  page_table_[page_id]=f;
  replacer_->RecordAccess(f);

  return &pages_[f];
}

/*
别人之前通过 FetchPage() 或 NewPage() 拿到了一个页面。

现在他说：“我用完了，你可以考虑把它换出去了。”//这里的意思是pincount--后如果为0就设置为evictable

你需要：
1. 找到这个页面。
2. 将它的 Pin Count 减一。
3. 根据 is_dirty 判断页面是否被修改过。
4. 如果 Pin Count 变成 0，就通知 LRU-K：这个页面现在可以被淘汰了。
这里最重要的是：

Pin Count 大于 0，表示还有人在使用这个页面，不能淘汰；Pin Count 等于 0，才有资格参与淘汰。
*/
auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty, [[maybe_unused]] AccessType access_type) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  
  auto it=page_table_.find(page_id);
  if(it==page_table_.end()){
    //没找到要怎么处理？
    return false;
  }
  
  //找到了
  frame_id_t f=it->second;
  Page* page=&pages_[f];

  //先判断pincount，再判断dirty
  if(page->pin_count_==0) return false;
  if(is_dirty)  page->is_dirty_=true;
  page->pin_count_--;

  //page的nuPin操作只在这一个函数，因此unPin后要加上setEvictAble的判断
  if(page->pin_count_==0){
    replacer_->SetEvictable(f,true);
  }
  return true;
}

/*
4. FlushPage：把一个页面写回磁盘
别人说：“把这个页面的数据保存到磁盘。”

你需要：
1. 找到内存里的这个页面。
2. 调用 DiskScheduler 安排一次写磁盘操作。
3. 等待写入完成。
4. 将页面的 Dirty 标记清除。
注意：即使页面还在被使用，也允许 Flush，不要求 Pin Count 为 0。
*/
auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  
  return flushPageUnsafe(page_id);
}

//不加锁的刷盘
//刷盘后只是把is_dirty的状态改成false了；
//淘汰帧前必刷盘
//刷盘和evictable无关，任何时候都可以刷盘
bool BufferPoolManager::flushPageUnsafe(page_id_t page_id){
  auto it=page_table_.find(page_id);
  if(it==page_table_.end()){
    //没找到要怎么处理？
    return false;
  }
  
  //找到了
  frame_id_t f=it->second;
  Page* page=&pages_[f];
  //如果是干净的就不需要刷盘
  if(!page->IsDirty()){
    return true;
  }

  //创建write请求
  DiskRequest r;
  std::promise<bool> p;
  std::future<bool> future=p.get_future();
  r.callback_=std::move(p);
  r.data_=page->data_;
  r.is_write_=true;
  r.page_id_=page_id;
  
  //放入请求队列
  disk_scheduler_->Schedule(std::move(r));

  //等待请求执行结果
  bool success=future.get();
  if(!success)  return false;
  page->is_dirty_=false;
  return true;
}


//遍历当前内存中的所有页面，把它们全部写回磁盘。
void BufferPoolManager::FlushAllPages() {
  std::lock_guard<std::mutex> lock(latch_);
  
  for(auto it:page_table_){
    flushPageUnsafe(it.first);
  }
}

/*
别人说：“这个页面我不再需要了，帮我删除。”

你需要：
1. 检查页面是否存在。
2. 如果 Pin Count 大于 0，说明还有人在使用，不能删除。
3. 如果可以删除，就清理它的内存数据。
4. 从页面映射表中删除记录。
5. 从 LRU-K 中移除它的访问历史。
6. 把对应内存位置放回空闲列表。
7. 调用 DeallocatePage()。
*/

//删除一个页面不一定需要写回磁盘，不需要关注is_dirty
auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool { 
  std::lock_guard<std::mutex> lock(latch_);
  
  auto it=page_table_.find(page_id);
  if(it==page_table_.end()){
    //没找到要怎么处理？
    DeallocatePage(page_id);
    return true;//不需要删除就是删除成功
  }

  //找到了
  frame_id_t f=it->second;
  Page* page=&pages_[f];

  //还有人用
  if(page->pin_count_>0){
    return false;
  }else{
    //内存数据怎么清理？
    page->ResetMemory();//其他成员变量需要处理么
    page->is_dirty_=false;
    page->page_id_=INVALID_PAGE_ID;
    page->pin_count_=0;

    page_table_.erase(it);
    free_list_.push_front(f);//从前面放从后面取
    replacer_->Remove(f);//帧f的访问历史记录的相关内存删除
  }

  DeallocatePage(page_id);
  return true;
}

auto BufferPoolManager::AllocatePage() -> page_id_t { return next_page_id_++; }

//这个有保护作用么？难道为了升级
auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard {
  Page* page=FetchPage(page_id);
  return BasicPageGuard(this,page);
}

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  Page* page=FetchPage(page_id);
  if(page!=nullptr) page->RLatch();
  return ReadPageGuard(this,page);//在ReadPageGuard的Drop()里面解锁
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  Page* page=FetchPage(page_id);
  if(page!=nullptr) page->WLatch();
  return WritePageGuard(this,page);//在WritePageGuard的Drop()里面解锁
}

//这个有保护作用么？难道为了升级
auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard {
  Page* page=NewPage(page_id);
  return BasicPageGuard(this,page);
}

}  // namespace bustub
