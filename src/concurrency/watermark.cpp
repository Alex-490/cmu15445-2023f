#include "concurrency/watermark.h"
#include <exception>
#include "common/exception.h"

namespace bustub {

auto Watermark::AddTxn(timestamp_t read_ts) -> void {
  if (read_ts < commit_ts_) {
    throw Exception("read ts < commit ts");
  }

  // TODO(fall2023): implement me!
  current_reads_[read_ts]++;
  pq_.push(read_ts);
}

auto Watermark::RemoveTxn(timestamp_t read_ts) -> void {
  // TODO(fall2023): implement me!
  current_reads_[read_ts]--;

  while(!pq_.empty()&&current_reads_[pq_.top()]==0){
    pq_.pop();
  }

  if(pq_.empty()){
    watermark_=commit_ts_;
  }else{
    watermark_=pq_.top();
  }
}

}  // namespace bustub
