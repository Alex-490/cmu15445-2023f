//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_scheduler.cpp
//
// Identification: src/storage/disk/disk_scheduler.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/disk/disk_scheduler.h"
#include "common/exception.h"
#include "storage/disk/disk_manager.h"

namespace bustub {

DiskScheduler::DiskScheduler(DiskManager *disk_manager) : disk_manager_(disk_manager) {
  // TODO(P1): remove this line after you have implemented the disk scheduler API
  throw NotImplementedException(
      "DiskScheduler is not implemented yet. If you have finished implementing the disk scheduler, please remove the "
      "throw exception line in `disk_scheduler.cpp`.");

  // Spawn the background thread
  background_thread_.emplace([&] { StartWorkerThread(); });
}

DiskScheduler::~DiskScheduler() {
  // Put a `std::nullopt` in the queue to signal to exit the loop
  request_queue_.Put(std::nullopt);
  if (background_thread_.has_value()) {
    background_thread_->join();
  }
}

//n个线程把diskrequest放入requestqueue里面 
//这个应该是放进去，因为传入了一个参数
void DiskScheduler::Schedule(DiskRequest r) {
  //怎么给r套上std::optional?
  request_queue_.Put(std::optional<DiskRequest>(std::move(r)));
}

/*
它需要不断重复以下工作：
1. 从队列中取出一个请求。
2. 判断这是读请求还是写请求。
3. 如果是读，就调用 DiskManager 的读接口。
4. 如果是写，就调用 DiskManager 的写接口。
5. 操作完成后，通过 callback_ 通知请求发起者。
6. 继续处理下一个请求。
*/
void DiskScheduler::StartWorkerThread() {
  while(1){
    //因为get本身就设置了阻塞因此这里不需要使用阻塞
    std::optional<DiskRequest> r=request_queue_.Get();
    //终止信号
    if(!r.has_value()){
      //析构是往队列里面放nullopt，为的是等队列里面剩余任务完成
      //DiskScheduler::~DiskScheduler();
      return;//还需要return么？
    }

    //写
    if(r.value().is_write_){
      disk_manager_->WritePage(r.value().page_id_,r.value().data_);
    }else{
      disk_manager_->ReadPage(r.value().page_id_,r.value().data_);
    }

    r.value().callback_.set_value(true);//这里设置为true，触发通知，共享状态就绪，future解除阻塞
  }
}

}  // namespace bustub
