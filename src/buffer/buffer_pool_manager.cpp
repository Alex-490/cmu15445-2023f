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

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  std::scoped_lock<std::mutex> lock(latch_);

  auto newpage=AllocatePage();
  frame_id_t replaceframe;
  if(!free_list_.empty()){
    replaceframe=free_list_.front();
    free_list_.pop_front();
  }else{
    if(!replacer_->Evict(&replaceframe)){
      return nullptr;
    }
  }

  if(pages_[replaceframe].IsDirty()){
    auto promise=disk_scheduler_->CreatePromise();
    auto future=promise.get_future();
    disk_scheduler_->Schedule({true,pages_[replaceframe].GetData(),pages_[replaceframe].page_id_,std::move(promise)});
    future.get();
  }

  page_table_.erase(pages_[replaceframe].page_id_);
  pages_[replaceframe].page_id_=newpage;
  page_table_.insert({newpage,replaceframe});
  *page_id=newpage;
  
}

auto BufferPoolManager::FetchPage(page_id_t page_id, [[maybe_unused]] AccessType access_type) -> Page * {
  std::scoped_lock<std::mutex> lock(latch_);

  frame_id_t replaceframe;
  auto it=page_table_.find(page_id);
  if(it!=page_table_.end()){
    replaceframe=it->second;
    pages_[replaceframe].pin_count_++;
    replacer_->RecordAccess(replaceframe);
    replacer_->SetEvictable(replaceframe,false);
    return &pages_[replaceframe];
  }
  
  if(!free_list_.empty()){
    replaceframe=std::move(free_list_.front());
    free_list_.pop_front();
  }else{
    if(!replacer_->Evict(&replaceframe)){
      return nullptr;
    }
  }

  if(pages_[replaceframe].IsDirty()){
    auto promise=disk_scheduler_->CreatePromise();
    auto future=promise.get_future();
    disk_scheduler_->Schedule({true,pages_[replaceframe].GetData(),pages_[replaceframe].GetPageId(),std::move(promise)});
    future.get();
  }

  pages_[replaceframe].page_id_=page_id;
  pages_[replaceframe].pin_count_++;
  replacer_->RecordAccess(replaceframe);
  replacer_->SetEvictable(replaceframe,false);
  page_table_.erase(page_id);
  page_table_.insert({page_id,replaceframe});
  return &pages_[replaceframe];
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty, [[maybe_unused]] AccessType access_type) -> bool {
  std::scoped_lock<std::mutex> lock(latch_);

  auto it=page_table_.find(page_id);
  if(it==page_table_.end()||pages_[it->second].GetPinCount()==0) return false;

  pages_[it->second].pin_count_--;
  if(pages_[page_id].GetPinCount()==0){
    replacer_->SetEvictable(it->second,true);
  }

  if(is_dirty){
    pages_[it->second].is_dirty_=is_dirty;
  }
  return false;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  std::scoped_lock<std::mutex> lock(latch_);
  
  auto it=page_table_.find(page_id);
  if(it==page_table_.end()) return false;
  
  auto promise=disk_scheduler_->CreatePromise();
  auto future=promise.get_future();
  disk_scheduler_->Schedule({true,pages_[it->second].GetData(),pages_[it->second].GetPageId(),std::move(promise)});
  future.get();

  pages_[it->second].is_dirty_=false;
  return true;
}

void BufferPoolManager::FlushAllPages() {
  std::scoped_lock<std::mutex> lock(latch_);  
  
  for(auto &it:page_table_){
    auto promise=disk_scheduler_->CreatePromise();
    auto future=promise.get_future();
    disk_scheduler_->Schedule({true,pages_[it.second].GetData(),pages_[it.second].GetPageId(),std::move(promise)});
    future.get();
    pages_[it.second].is_dirty_=false;
  }
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool { 
  std::scoped_lock<std::mutex> lock(latch_);
  auto it=page_table_.find(page_id);
  if(it==page_table_.end()) return false;

  auto target=it->second;
  if(pages_[target].GetPinCount()>0) return false;
  
  page_table_.erase(page_id);
  replacer_->Remove(target);
  free_list_.emplace_back(target);
  
  pages_[target].ResetMemory();
  pages_[target].page_id_=INVALID_PAGE_ID;
  pages_[target].pin_count_=0;
  pages_[target].is_dirty_=false;
  
  DeallocatePage(page_id);
  return true;
}

auto BufferPoolManager::AllocatePage() -> page_id_t { return next_page_id_++; }

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard { return {this, nullptr}; }

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard { return {this, nullptr}; }

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard { return {this, nullptr}; }

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard { return {this, nullptr}; }

}  // namespace bustub
