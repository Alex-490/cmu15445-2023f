//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_extendible_hash_table.cpp
//
// Identification: src/container/disk/hash/disk_extendible_hash_table.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "common/config.h"
#include "common/exception.h"
#include "common/logger.h"
#include "common/macros.h"
#include "common/rid.h"
#include "common/util/hash_util.h"
#include "container/disk/hash/disk_extendible_hash_table.h"
#include "storage/index/hash_comparator.h"
#include "storage/page/extendible_htable_bucket_page.h"
#include "storage/page/extendible_htable_directory_page.h"
#include "storage/page/extendible_htable_header_page.h"
#include "storage/page/page_guard.h"

namespace bustub {

template <typename K, typename V, typename KC>
DiskExtendibleHashTable<K, V, KC>::DiskExtendibleHashTable(const std::string &name, BufferPoolManager *bpm,
                                                           const KC &cmp, const HashFunction<K> &hash_fn,
                                                           uint32_t header_max_depth, uint32_t directory_max_depth,
                                                           uint32_t bucket_max_size)
    : bpm_(bpm),
      cmp_(cmp),
      hash_fn_(std::move(hash_fn)),
      header_max_depth_(header_max_depth),
      directory_max_depth_(directory_max_depth),
      bucket_max_size_(bucket_max_size) {}
/*
原因是：
- 已经拿到 Directory 的读锁，不再需要 Header 的读锁。
- 已经拿到 Bucket 的读锁，不再需要 Directory 的读锁。
- Lookup 期间只需要保护 Bucket。
这也更符合 T4 的 latch crabbing 思路。官方要求在确认子页面安全后尽早释放父页面锁。
*/
/*****************************************************************************
 * SEARCH
 *****************************************************************************/
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::GetValue(const K &key, std::vector<V> *result, Transaction *transaction) const-> bool {
  uint32_t hashKey=Hash(key);
  
  //header,read
  ReadPageGuard hRGuard=bpm_->FetchPageRead(header_page_id_);
  const ExtendibleHTableHeaderPage* header=hRGuard.As<ExtendibleHTableHeaderPage>();

  //directory,read
  page_id_t dPageId=header->GetDirectoryPageId(header->HashToDirectoryIndex(hashKey));
  if(dPageId==INVALID_PAGE_ID)  return false;//注意检查dPageId的有效性
  ReadPageGuard dRGuard=bpm_->FetchPageRead(dPageId);
  hRGuard.Drop();//得到了子页面后释放父页面的锁
  const ExtendibleHTableDirectoryPage *directory=dRGuard.As<ExtendibleHTableDirectoryPage>();
  
  //bucket,read
  page_id_t bPageId=directory->GetBucketPageId(directory->HashToBucketIndex(hashKey));
  if(bPageId==INVALID_PAGE_ID)  return false;//注意检查bPageId的有效性
  ReadPageGuard bRGuard=bpm_->FetchPageRead(bPageId);
  dRGuard.Drop();////得到了子页面后释放父页面的锁
  const ExtendibleHTableBucketPage<K,V,KC>* bucket=bRGuard.As<ExtendibleHTableBucketPage<K,V,KC>>();

  V value;
  if(!bucket->Lookup(key,value,cmp_)){
    return false;
  }

  //结果传出
  result->emplace_back(value);
  return true;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
//插入 
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::Insert(const K &key, const V &value, Transaction *transaction) -> bool {
  uint32_t hashKey=Hash(key);
  
  //header,read
  ReadPageGuard hRGuard=bpm_->FetchPageRead(header_page_id_);
  const ExtendibleHTableHeaderPage* header=hRGuard.As<ExtendibleHTableHeaderPage>();

  //directory,write
  page_id_t dPageId=header->GetDirectoryPageId(header->HashToDirectoryIndex(hashKey));
  WritePageGuard dWGuard=bpm_->FetchPageWrite(dPageId);
  ExtendibleHTableDirectoryPage *directory=dWGuard.AsMut<ExtendibleHTableDirectoryPage>();
  
  //bucket,write
  page_id_t bPageId=directory->GetBucketPageId(directory->HashToBucketIndex(hashKey));
  WritePageGuard bWGuard=bpm_->FetchPageWrite(bPageId);
  ExtendibleHTableBucketPage<K,V,KC>* bucket=bWGuard.AsMut<ExtendibleHTableBucketPage<K,V,KC>>();

  auto bIndex=directory->HashToBucketIndex(Hash(key));

  //插入，没有满，因为上次插入如果满了上次会跑下行的逻辑，就会处理掉
  if(!bucket->Insert(key,value,cmp_))  return false;//插入失败
  
  //这时bucket满了
  if(bucket->IsFull()){
    if(!split(directory,bucket,bIndex)){
       //return false;
       //分裂失败怎么处理？
    }
  }
  
  return true;//成功
}

//某个桶因为某次Insert()桶满了要递归分裂
template <typename K, typename V, typename KC>
bool DiskExtendibleHashTable<K,V,KC>::split(ExtendibleHTableDirectoryPage* directory,ExtendibleHTableBucketPage<K,V,KC>* bucket,uint32_t bIndex){
  uint32_t ld=directory->GetLocalDepth(bIndex);
  if(ld==directory->GetGlobalDepth()) return false;
  
  //找到bucket_page_ids_里所有内容为指向bIndex所在的桶页号的索引
  std::vector<uint32_t> vet;
  for(uint32_t i=0;i<directory->Size();i++){
    if(directory->GetBucketPageId(i)==directory->GetBucketPageId(bIndex)){
      vet.emplace_back(i);
    }
  }

  //获取一个新桶
  page_id_t newPageId;
  Page* newBucket=bpm_->NewPage(*newPageId);
  
  WritePageGuard guard=bpm_->FetchPageWrite(newPageId);
  ExtendibleHTableBucketPage<K,V,KC>* newBucket=guard.AsMut<ExtendibleHTableBucketPage<K,V,KC>>();
  
  std::vector<uint32_t> vet1;
  for(uint32_t i=0;i<bucket->Size();i++){
    const K& key=bucket->EntryAt(i).first;
    if(Hash(key)^(1U<<ld)!=0){
      vet1.emplace_back(i);
    }
  }

  std::sort(vet1.rbegin(),vet1.rend());
  for(auto& index:vet1){
    newBucket->Insert(bucket->EntryAt(index).first,bucket->EntryAt(index).second,cmp_);
    bucket->RemoveAt(index);
  }

  //从这些索引中找到local_depth+1位为1的,把它们指向的页号改为新桶页号
  //改变local_depth_
  for(auto &index:vet){
    if(index^(1U<<ld)!=0){
      directory->SetBucketPageId(index,newPageId);
    }
    directory->IncrLocalDepth(index);
  }

  return true;
}

template <typename K, typename V, typename KC>
void DiskExtendibleHashTable<K, V, KC>::UpdateDirectoryMapping(ExtendibleHTableDirectoryPage *directory,
                                                               uint32_t new_bucket_idx, page_id_t new_bucket_page_id,
                                                               uint32_t new_local_depth, uint32_t local_depth_mask) {
  throw NotImplementedException("DiskExtendibleHashTable is not implemented");
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::Remove(const K &key, Transaction *transaction) -> bool {
  uint32_t hashKey=Hash(key);
  
  //header,read
  ReadPageGuard hRGuard=bpm_->FetchPageRead(header_page_id_);
  //那这里读信息为什么不用写锁，是因为全局就不涉及对header们的修改么？
  const ExtendibleHTableHeaderPage* header=hRGuard.As<ExtendibleHTableHeaderPage>();

  //directory,write
  page_id_t dPageId=header->GetDirectoryPageId(header->HashToDirectoryIndex(hashKey));
  if(dPageId==INVALID_PAGE_ID)  return false;
  WritePageGuard dWGuard=bpm_->FetchPageWrite(dPageId);
  hRGuard.Drop();//header后面不需要了
  //asmut，用写锁写信息，准备要弄脏。as，用写锁读信息
  //const ExtendibleHTableDirectoryPage *directory=dWGuard.As<ExtendibleHTableDirectoryPage>();
  ExtendibleHTableDirectoryPage *directory=dWGuard.AsMut<ExtendibleHTableDirectoryPage>();
  
  //bucket,write
  page_id_t bPageId=directory->GetBucketPageId(directory->HashToBucketIndex(hashKey));
  if(bPageId==INVALID_PAGE_ID)  return false;

  //保留remove后是否空桶。因为要在函数内就释放掉bucket的写保护，为后面的合并做准备（bucket要被删掉，directory要被修改）
  //那能够用Drop()呢
  bool bucketEmpty;
  {
    WritePageGuard bWGuard=bpm_->FetchPageWrite(bPageId);
    //asmut，用写锁写信息，准备要弄脏。as，用写锁读信息
    //ExtendibleHTableBucketPage<K,V,KC>* bucket=bWGuard.As<ExtendibleHTableBucketPage<K,V,KC>>();
    ExtendibleHTableBucketPage<K,V,KC>* bucket=bWGuard.AsMut<ExtendibleHTableBucketPage<K,V,KC>>();

    //key可能不存在，那就不需要看空不空, 直接走，因为空的要么是刚创建要么是remove掉了最后一个，后者会走到后面几句，那里再处理
    if(!bucket->Remove(key,cmp_)) return false;

    bucketEmpty=bucket->IsEmpty();
  }

  //如果是空的就递归合并
  if(bucketEmpty){
    uint32_t bIndex=directory->HashToBucketIndex(hashKey);
    if(mergeR(directory,bIndex)){
      //合并完成后收缩
      while(directory->CanShrink()){
        directory->DecrGlobalDepth();
      }
    }
  }

  return true;
}

//某个桶因为某次Remove()桶空了要递归合并
template <typename K, typename V, typename KC>
bool DiskExtendibleHashTable<K,V,KC>::mergeR(ExtendibleHTableDirectoryPage* directory,uint32_t bIndex){
  //局部深度为0不需要合并
  uint32_t ld=directory->GetLocalDepth(bIndex);
  if(ld==0) return false;

  page_id_t bucketPageId=directory->GetBucketPageId(bIndex);
  std::vector<uint32_t> sa,sb;
  
  //收集指向桶是这个bindex指向空桶的全部下标,集合成为sa
  for(uint32_t i=0;i<directory->Size();i++){
    if(directory->GetBucketPageId(i)==bucketPageId){
      if(directory->GetLocalDepth(i)!=ld){
        return false; 
      }
      sa.emplace_back(i);
    }
  }

  //找到集合sa的image集合sb(千万不要直觉认为image是你想象的那个样子!!!!!!!)
  for(uint32_t i=0;i<sa.size();i++){
    sb.emplace_back(directory->GetSplitImageIndex(sa[i]));
  }

  page_id_t anotherBucketPageId=directory->GetBucketPageId(sb[0]);
  if(anotherBucketPageId==bucketPageId) return false;
  for(uint32_t i=0;i<sb.size();i++){
    if(directory->GetBucketPageId(sb[i])!=anotherBucketPageId||directory->GetLocalDepth(sb[i])!=ld){
      return false;
    }
  }

  //不涉及修改bucket内容只需要改变不同索引内容指向的bucket页就行
  bool emptyA,emptyB;
  {
    //获得写锁来获取信息，因为多线程下其他锁可以获得了写锁在改东西。这里的writeguard用的As就说明了获取信息用
    WritePageGuard guardA=bpm_->FetchPageWrite(bucketPageId);
    const ExtendibleHTableBucketPage<K,V,KC>* bucketA=guardA.As<ExtendibleHTableBucketPage<K,V,KC>>();
    WritePageGuard guardB=bpm_->FetchPageWrite(anotherBucketPageId);
    const ExtendibleHTableBucketPage<K,V,KC>* bucketB=guardB.As<ExtendibleHTableBucketPage<K,V,KC>>();
    emptyA=bucketA->IsEmpty();
    emptyB=bucketB->IsEmpty();
  }
  //有一个空才合并
  if(!emptyA&&!emptyB)  return false;

  //合并工作事实需要的操作
  //修改所有索引指向存活桶,修改指向存活桶的所有索引的localdepth,删除被“合并”的桶
  page_id_t deletePageId,keepPageId;
  if(emptyA){
    deletePageId=bucketPageId;
    keepPageId=anotherBucketPageId;
  }else{
    deletePageId=anotherBucketPageId;
    keepPageId=bucketPageId;
  }

  if (!bpm_->DeletePage(deletePageId)) return false;

  for(auto index:sa){
    directory->SetBucketPageId(index,keepPageId);
    directory->DecrLocalDepth(index);
  }
  for(auto index:sb){
    directory->SetBucketPageId(index,keepPageId);
    directory->DecrLocalDepth(index);
  }

  //存活桶不为空
  mergeR(directory,bIndex);
  return true;
}

template class DiskExtendibleHashTable<int, int, IntComparator>;
template class DiskExtendibleHashTable<GenericKey<4>, RID, GenericComparator<4>>;
template class DiskExtendibleHashTable<GenericKey<8>, RID, GenericComparator<8>>;
template class DiskExtendibleHashTable<GenericKey<16>, RID, GenericComparator<16>>;
template class DiskExtendibleHashTable<GenericKey<32>, RID, GenericComparator<32>>;
template class DiskExtendibleHashTable<GenericKey<64>, RID, GenericComparator<64>>;
}  // namespace bustub
