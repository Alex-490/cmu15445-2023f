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
  for(uint32_t i=0;i<HTABLE_DIRECTORY_ARRAY_SIZE;i++){
    SetBucketPageId(i,INVALID_PAGE_ID);
    SetLocalDepth(i,0);
  }
}

//用hashkey的低global_depth_位找到这个hashkey对应的bucket_page_ids_的下标
auto ExtendibleHTableDirectoryPage::HashToBucketIndex(uint32_t hash) const -> uint32_t {
  return hash&((static_cast<uint32_t>(1)<<global_depth_)-1);
}

auto ExtendibleHTableDirectoryPage::GetBucketPageId(uint32_t bucket_idx) const -> page_id_t {
    BUSTUB_ASSERT(bucket_idx<HTABLE_DIRECTORY_ARRAY_SIZE,"invalid bucket_idx");
    return bucket_page_ids_[bucket_idx];
}

void ExtendibleHTableDirectoryPage::SetBucketPageId(uint32_t bucket_idx, page_id_t bucket_page_id) {
    BUSTUB_ASSERT(bucket_idx<HTABLE_DIRECTORY_ARRAY_SIZE,"invalid bucket_idx");
    bucket_page_ids_[bucket_idx]=bucket_page_id;
}

//坑中之坑！！！！！！
/*
合并的时候，一个bucket_idx对应bucket（称为A）的remove操作导致A空桶，
这时候查找bucket_page_ids_里全部是这个bucket页号的下标，下标集合为SA
用这些下标传入这个函数找到对应的idx集合SB。SB全体的对应bucket必须指向另一个相同的桶B
且SA和SB的对应localdepth必须是bucketidx对应的localdepth，这时候才确定A与B是分裂出来的
而且再经过检验空条件和容量能不能装下，全部通过才能合并
合并的时候SA全体目录项内容改为桶B的页号，且SA和SB全体的localdepth-1
然后再递归进行直到不行为止
这时候再遍历全部桶的localdepth，globaldepth收缩得尽量小
*/
auto ExtendibleHTableDirectoryPage::GetSplitImageIndex(uint32_t bucket_idx) const -> uint32_t{
  //获取bucket_idx的对应idx
  return bucket_idx^(static_cast<uint32_t>(1)<<(local_depths_[bucket_idx]-1));
  //假设localdepth=2,100->110,010->000,111->101,
}

auto ExtendibleHTableDirectoryPage::GetSplitImageIndex(uint32_t bucket_idx, uint32_t depth) const -> uint32_t {
  
}

auto ExtendibleHTableDirectoryPage::GetGlobalDepth() const -> uint32_t {
  return global_depth_;
}

void ExtendibleHTableDirectoryPage::IncrGlobalDepth() {
  ++global_depth_;
}

void ExtendibleHTableDirectoryPage::DecrGlobalDepth() {
  --global_depth_;
}

auto ExtendibleHTableDirectoryPage::CanShrink() -> bool {
  uint16_t max=local_depths_[0];
  for(uint32_t ld:local_depths_){
    if(ld==global_depth_) return false;
    if(ld>max)  max=ld;
  }
  global_depth_=max;
  return true;
}

auto ExtendibleHTableDirectoryPage::Size() const -> uint32_t {
  return static_cast<uint32_t>(1)<<global_depth_;
}

auto ExtendibleHTableDirectoryPage::GetLocalDepth(uint32_t bucket_idx) const -> uint32_t {
  BUSTUB_ASSERT(bucket_idx<HTABLE_DIRECTORY_ARRAY_SIZE,"invalid bucket_idx");
  return local_depths_[bucket_idx];
}

void ExtendibleHTableDirectoryPage::SetLocalDepth(uint32_t bucket_idx, uint8_t local_depth) {
  BUSTUB_ASSERT(bucket_idx<HTABLE_DIRECTORY_ARRAY_SIZE,"invalid bucket_idx");
  local_depths_[bucket_idx]=local_depth;
}

void ExtendibleHTableDirectoryPage::IncrLocalDepth(uint32_t bucket_idx) {
  BUSTUB_ASSERT(bucket_idx<HTABLE_DIRECTORY_ARRAY_SIZE,"invalid bucket_idx");
  ++local_depths_[bucket_idx];
}

void ExtendibleHTableDirectoryPage::DecrLocalDepth(uint32_t bucket_idx) {
  BUSTUB_ASSERT(bucket_idx<HTABLE_DIRECTORY_ARRAY_SIZE,"invalid bucket_idx");
  --local_depths_[bucket_idx];
}

auto ExtendibleHTableDirectoryPage::GetMaxDepth() const -> uint32_t {
  return max_depth_;
}

}  // namespace bustub
