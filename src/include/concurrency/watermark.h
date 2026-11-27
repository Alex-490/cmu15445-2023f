#pragma once

#include <map>
#include <vector>
#include <queue>

#include "concurrency/transaction.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * @brief tracks all the read timestamps.
 *
 */
//数据库产生了很多旧版本，如何安全地把它们删除
class Watermark {
 public:
  explicit Watermark(timestamp_t commit_ts) : commit_ts_(commit_ts), watermark_(commit_ts) {}

  auto AddTxn(timestamp_t read_ts) -> void;

  auto RemoveTxn(timestamp_t read_ts) -> void;

  /** The caller should update commit ts before removing the txn from the watermark so that we can track watermark
   * correctly. */
  auto UpdateCommitTs(timestamp_t commit_ts) { commit_ts_ = commit_ts; }

  auto GetWatermark() -> timestamp_t {
    if (current_reads_.empty()) {
      return commit_ts_;
    }
    return watermark_;
  }

  timestamp_t commit_ts_;//所有事务最新的提交时间戳

  timestamp_t watermark_;

  std::shared_mutex latch_;
  std::map<timestamp_t, size_t> current_reads_;//维护所有事务地read时间戳，某个事务提交后从表中这个时间戳地value-1，为0就删掉这个时间戳，watermark因此上涨
};

};  // namespace bustub
