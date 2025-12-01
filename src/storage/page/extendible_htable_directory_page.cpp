//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_directory_page.cpp
//
// Identification: src/storage/page/extendible_htable_directory_page.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/extendible_htable_directory_page.h"

#include <algorithm>
#include <unordered_map>

#include "common/config.h"
#include "common/logger.h"

namespace bustub {

void ExtendibleHTableDirectoryPage::Init(uint32_t max_depth) {
  throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

auto ExtendibleHTableDirectoryPage::HashToBucketIndex(uint32_t hash) const -> uint32_t { return 0; }

auto ExtendibleHTableDirectoryPage::GetBucketPageId(uint32_t bucket_idx) const -> page_id_t {
  return bucket_page_ids_[bucket_idx];
}

void ExtendibleHTableDirectoryPage::SetBucketPageId(uint32_t bucket_idx, page_id_t bucket_page_id) {
  bucket_page_ids_[bucket_idx]=bucket_page_id;
  //throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

auto ExtendibleHTableDirectoryPage::GetSplitImageIndex(uint32_t bucket_idx) const -> uint32_t { return 0; }

auto ExtendibleHTableDirectoryPage::GetGlobalDepth() const -> uint32_t { return 0; }

void ExtendibleHTableDirectoryPage::IncrGlobalDepth() {
  throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

void ExtendibleHTableDirectoryPage::DecrGlobalDepth() {
  throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

auto ExtendibleHTableDirectoryPage::CanShrink() -> bool { return false; }

auto ExtendibleHTableDirectoryPage::Size() const -> uint32_t {
  return 1<<global_depth_;
}

auto ExtendibleHTableDirectoryPage::GetLocalDepth(uint32_t bucket_idx) const -> uint32_t {
  return local_depths_[bucket_idx];
}

void ExtendibleHTableDirectoryPage::SetLocalDepth(uint32_t bucket_idx, uint8_t local_depth) {
  BUSTUB_ASSERT(bucket_idx<Size(),"bucket id greater than Size()");
  local_depths_[bucket_idx]=local_depth;
  //throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

void ExtendibleHTableDirectoryPage::IncrLocalDepth(uint32_t bucket_idx) {
  BUSTUB_ASSERT(bucket_idx<Size(),"bucket id greater than Size()");
  local_depths_[bucket_idx]+=1;
  //throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

void ExtendibleHTableDirectoryPage::DecrLocalDepth(uint32_t bucket_idx) {
  BUSTUB_ASSERT(bucket_idx<Size(),"bucket id greater than Size()");
  BUSTUB_ASSERT(local_depths_[bucket_idx]>=1,"can not decrese depth below 1");
  local_depths_[bucket_idx]-=1;
  //throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

}  // namespace bustub
