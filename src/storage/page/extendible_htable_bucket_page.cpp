//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_bucket_page.cpp
//
// Identification: src/storage/page/extendible_htable_bucket_page.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <optional>
#include <utility>

#include "common/exception.h"
#include "storage/page/extendible_htable_bucket_page.h"

namespace bustub {

template <typename K, typename V, typename KC>
void ExtendibleHTableBucketPage<K, V, KC>::Init(uint32_t max_size) {
  max_size_=max_size;
  size_=0;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Lookup(const K &key, V &value, const KC &cmp) const -> bool {
  for(uint32_t i=0;i<size_;i++){
    if(cmp(key,array_[i].first)==0){
      value=array_[i].second;
      return true;
    }
  }

  return false;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Insert(const K &key, const V &value, const KC &cmp) -> bool {
  for(uint32_t i=0;i<size_;i++){
    if(cmp(key,array_[i].first)==0){
      return false;
    }
  }

  //别忘了这个
  if(IsFull())  return false;

  array_[size_++]=std::make_pair(key,value);
  return true;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Remove(const K &key, const KC &cmp) -> bool {
  for(uint32_t i=0;i<size_;i++){
    if(cmp(key,array_[i].first)==0){
      //K 和 V 可以是用户自定义类型。它们的赋值运算符不一定能安全处理自赋值。
      //array_[i]=array_[--size_];
      
      //改成这样
      --size_;
      if(i!=size_)  array_[i]=array_[size_];

      return true;
    }
  }

  //key不存在
  return false;
}

template <typename K, typename V, typename KC>
void ExtendibleHTableBucketPage<K, V, KC>::RemoveAt(uint32_t bucket_idx) {
  BUSTUB_ASSERT(bucket_idx<size_,"invalid bucket_idx");
  //K 和 V 可以是用户自定义类型。它们的赋值运算符不一定能安全处理自赋值。
  //array_[bucket_idx]=array_[--size_];

  --size_;
  if(bucket_idx!=size_) array_[bucket_idx]=array_[size_];
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::KeyAt(uint32_t bucket_idx) const -> K {
  BUSTUB_ASSERT(bucket_idx<size_,"invalid bucket_idx");
  return array_[bucket_idx].first;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::ValueAt(uint32_t bucket_idx) const -> V {
  BUSTUB_ASSERT(bucket_idx<size_,"invalid bucket_idx");
  return array_[bucket_idx].second;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::EntryAt(uint32_t bucket_idx) const -> const std::pair<K, V> & {
  BUSTUB_ASSERT(bucket_idx<size_,"invalid bucket_idx");
  return array_[bucket_idx];
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Size() const -> uint32_t {
  return size_;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::IsFull() const -> bool {
  //return size_==max_size_;
  return size_>=max_size_;//更好的防御性
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::IsEmpty() const -> bool {
  return size_==0;
}

template class ExtendibleHTableBucketPage<int, int, IntComparator>;
template class ExtendibleHTableBucketPage<GenericKey<4>, RID, GenericComparator<4>>;
template class ExtendibleHTableBucketPage<GenericKey<8>, RID, GenericComparator<8>>;
template class ExtendibleHTableBucketPage<GenericKey<16>, RID, GenericComparator<16>>;
template class ExtendibleHTableBucketPage<GenericKey<32>, RID, GenericComparator<32>>;
template class ExtendibleHTableBucketPage<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
