#include "concurrency/watermark.h"
#include <exception>
#include "common/exception.h"

namespace bustub {

auto Watermark::AddTxn(timestamp_t read_ts) -> void {
  std::unique_lock<std::shared_mutex> lock(latch_);

  if (read_ts < commit_ts_) {
    throw Exception("read ts < commit ts");
  }

  // TODO(fall2023): implement me!
  if(current_reads_.empty())  watermark_=read_ts;
  current_reads_[read_ts]++;
  watermark_=std::min(watermark_,read_ts);
}

auto Watermark::RemoveTxn(timestamp_t read_ts) -> void {
  // TODO(fall2023): implement me!
  std::unique_lock<std::shared_mutex> lock(latch_);
  
  //read_ts项--,=0就删除
  auto iter=current_reads_.find(read_ts);
  if(iter==current_reads_.end()){
    throw Exception("read ts not found in Watermark::RemoveTxn");
  }
  if(--iter->second==0){
    current_reads_.erase(iter);
  }

  //更新为read时间戳中最小的
  if(!current_reads_.empty()){
    watermark_=std::min(watermark_,current_reads_.begin()->first);
  }else{
    //如果没有活跃事务，那就没有事务需要读旧快照了。数据库可以把watermark推进到当前最新提交时间戳
    watermark_=commit_ts_;
  }
}

}  // namespace bustub
