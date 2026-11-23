//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_header_page.cpp
//
// Identification: src/storage/page/extendible_htable_header_page.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/extendible_htable_header_page.h"

#include "common/exception.h"

namespace bustub {

void ExtendibleHTableHeaderPage::Init(uint32_t max_depth) {
  //需要mutex么？不需要，page通过readpageguard或writepageguard传入时已经上了读写锁
  max_depth_=max_depth;
  for(int i=0;i<HTABLE_HEADER_ARRAY_SIZE;i++){
    directory_page_ids_[i]=INVALID_PAGE_ID;
  }
}

//用hashkey的高max_depth位找到它对应的directorypage
auto ExtendibleHTableHeaderPage::HashToDirectoryIndex(uint32_t hash) const -> uint32_t {
  if(max_depth_==0) return 0;//深度为0索引只有一个为0
  
  return hash>>(32-max_depth_);
}

auto ExtendibleHTableHeaderPage::GetDirectoryPageId(uint32_t directory_idx) const -> uint32_t {  
  BUSTUB_ASSERT(directory_idx<MaxSize(),"invalid directory_idx");
  
  return directory_page_ids_[directory_idx];
}

void ExtendibleHTableHeaderPage::SetDirectoryPageId(uint32_t directory_idx, page_id_t directory_page_id) {
  BUSTUB_ASSERT(directory_idx<MaxSize(),"invalid directory_idx");

  directory_page_ids_[directory_idx]=directory_page_id;
}

auto ExtendibleHTableHeaderPage::MaxSize() const -> uint32_t {
  //return 1<<max_depth_;
  return 1U<<max_depth_;
}

}  // namespace bustub
