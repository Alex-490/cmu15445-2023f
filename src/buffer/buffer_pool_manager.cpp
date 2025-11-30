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
  frame_id_t replaceframe;
  if(!free_list_.empty()){
    replaceframe=free_list_.front();
    free_list_.pop_front();
  }else{
    if(!replacer_->Evict(&replaceframe)){
      return nullptr;
    }

    if(pages_[replaceframe].IsDirty()){
      auto promise1=disk_scheduler_->CreatePromise();
      auto future=promise1.get_future();
      disk_scheduler_->Schedule({true,pages_[replaceframe].GetData(),pages_[replaceframe].page_id_,std::move(promise1)});
      future.get();
    }

    page_table_.erase(pages_[replaceframe].page_id_);
  }

  auto page=AllocatePage();
  
}

auto BufferPoolManager::FetchPage(page_id_t page_id, [[maybe_unused]] AccessType access_type) -> Page * {
  return nullptr;
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty, [[maybe_unused]] AccessType access_type) -> bool {
  std::scoped_lock<std::mutex> lock(latch_);

  auto it=page_table_.find(page_id);
  if(it==page_table_.end()||pages_[it->second].pin_count_<=0) return false;

  pages_[it->second].pin_count_--;
  if(pages_[page_id].pin_count_==0){
    replacer_->SetEvictable(it->second,true);
  }

  if(is_dirty){
    pages_[it->second].is_dirty_=is_dirty;
  }
  return false;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  std::scoped_lock<std::mutex> lock(latch_);
  BUSTUB_ASSERT(page_id!=INVALID_PAGE_ID,"page_id is invalid");
  
  auto it=page_table_.find(page_id);
  if(it==page_table_.end()) return false;
  
  auto target=it->second;
  auto promise=disk_scheduler_->CreatePromise();
  auto future=promise.get_future();
  disk_scheduler_->Schedule({true,pages_[target].data_,pages_[target].page_id_,std::move(promise)});
  future.get();
  pages_[target].is_dirty_=false;
}

void BufferPoolManager::FlushAllPages() {
  std::scoped_lock<std::mutex> lock(latch_);  
  
  for(auto &it:page_table_){
    auto page_id=it.first;
    BUSTUB_ASSERT(page_id!=INVALID_PAGE_ID,"page_id is invalid");
    auto target=it.second;
    auto promise=disk_scheduler_->CreatePromise();
    auto future=promise.get_future();
    disk_scheduler_->Schedule({true,pages_[target].data_,pages_[target].page_id_,std::move(promise)});
    future.get();
    pages_[target].is_dirty_=false;
  }
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool { 
  std::scoped_lock<std::mutex> lock(latch_);
  auto it=page_table_.find(page_id);
  if(it==page_table_.end()) return false;

  auto target=it->second;
  if(pages_[target].pin_count_>0) return false;
  
  page_table_.erase(page_id);
  replacer_->Remove(target);
  free_list_.push_back(target);
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
