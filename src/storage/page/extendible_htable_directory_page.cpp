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
  max_depth_=max_depth;
  global_depth_=0;
}

auto ExtendibleHTableDirectoryPage::HashToBucketIndex(uint32_t hash) const -> uint32_t {
  return hash&((1u<<global_depth_)-1);
}

auto ExtendibleHTableDirectoryPage::GetBucketPageId(uint32_t bucket_idx) const -> page_id_t {
  BUSTUB_ASSERT(bucket_idx<Size(),"bucket_index larger than Size()");
  return bucket_page_ids_[bucket_idx];
}

void ExtendibleHTableDirectoryPage::SetBucketPageId(uint32_t bucket_idx, page_id_t bucket_page_id) {
  BUSTUB_ASSERT(bucket_idx<Size(),"bucket_index larger than Size()");
  bucket_page_ids_[bucket_idx]=bucket_page_id;
}

auto ExtendibleHTableDirectoryPage::GetSplitImageIndex(uint32_t bucket_idx) const -> uint32_t{
  BUSTUB_ASSERT(bucket_idx<Size(),"bucket_index larger than Size()");
  int depth=local_depths_[bucket_idx];
  return GetSplitImageIndex(bucket_idx,depth);
}

auto ExtendibleHTableDirectoryPage::GetSplitImageIndex(uint32_t bucket_idx, uint32_t depth) const -> uint32_t {
  BUSTUB_ASSERT(bucket_idx<Size(),"bucket_index larger than Size()");
  if(depth==0)  return 0;
  auto size=1<<(depth-1);
  if(bucket_idx<size) return bucket_idx+size;
  return bucket_idx-size;
}

auto ExtendibleHTableDirectoryPage::GetGlobalDepth() const -> uint32_t {
  return global_depth_;
}

void ExtendibleHTableDirectoryPage::IncrGlobalDepth() {
  BUSTUB_ASSERT(global_depth_<max_depth_,"global_depth larger than max_depth");
  for(int i=0;i<Size();i++){
    local_depths_[i+Size()]=local_depths_[i];
    bucket_page_ids_[i+Size()]=bucket_page_ids_[i];
  }
  
  ++global_depth_;
}

void ExtendibleHTableDirectoryPage::DecrGlobalDepth() {
  BUSTUB_ASSERT(global_depth_>0,"global_depth too small");
  --global_depth_;
}

auto ExtendibleHTableDirectoryPage::CanShrink() -> bool {
  for(int i=0;i<Size();i++){
    if(local_depths_[i]>=global_depth_) return false;
  }
  return true;
}

auto ExtendibleHTableDirectoryPage::Size() const -> uint32_t {
  return 1<<global_depth_;
}

auto ExtendibleHTableDirectoryPage::GetLocalDepth(uint32_t bucket_idx) const -> uint32_t {
  BUSTUB_ASSERT(bucket_idx<Size(),"bucket_index larger than Size()");
  return local_depths_[bucket_idx];
}

void ExtendibleHTableDirectoryPage::SetLocalDepth(uint32_t bucket_idx, uint8_t local_depth) {
  BUSTUB_ASSERT(bucket_idx<Size(),"bucket_index larger than Size()");
  local_depths_[bucket_idx]=local_depth;
}

void ExtendibleHTableDirectoryPage::IncrLocalDepth(uint32_t bucket_idx) {
  BUSTUB_ASSERT(bucket_idx<Size(),"bucket_index larger than Size()");
  BUSTUB_ASSERT(local_depths_[bucket_idx]<=global_depth_,"local_depth larger than global_depth");
  ++local_depths_[bucket_idx];
}

void ExtendibleHTableDirectoryPage::DecrLocalDepth(uint32_t bucket_idx) {
  BUSTUB_ASSERT(bucket_idx<Size(),"bucket_index larger than Size()");
  BUSTUB_ASSERT(local_depths_[bucket_idx]>0,"local_depth too small");
  --local_depths_[bucket_idx];
}

auto ExtendibleHTableDirectoryPage::GetMaxDepth() const -> uint32_t {
  return max_depth_;
}

}  // namespace bustub
