//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_directory_page.h
//
// Identification: src/include/storage/page/extendible_htable_directory_page.h
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/**
 * Directory page format:
 *  --------------------------------------------------------------------------------------
 * | MaxDepth (4) | GlobalDepth (4) | LocalDepths (512) | BucketPageIds(2048) | Free(1528)
 *  --------------------------------------------------------------------------------------
 */

#pragma once

#include <cassert>
#include <climits>
#include <cstdlib>
#include <string>

#include "common/config.h"
#include "storage/index/generic_key.h"

namespace bustub {

static constexpr uint64_t HTABLE_DIRECTORY_PAGE_METADATA_SIZE = sizeof(uint32_t) * 2;

/**
 * HTABLE_DIRECTORY_ARRAY_SIZE is the number of page_ids that can fit in the directory page of an extendible hash index.
 * This is 512 because the directory array must grow in powers of 2, and 1024 page_ids leaves zero room for
 * storage of the other member variables.
 */
static constexpr uint64_t HTABLE_DIRECTORY_MAX_DEPTH = 9;
static constexpr uint64_t HTABLE_DIRECTORY_ARRAY_SIZE = 1 << HTABLE_DIRECTORY_MAX_DEPTH;

/**
 * Directory Page for extendible hash table.
 */
class ExtendibleHTableDirectoryPage {
 public:
  // Delete all constructor / destructor to ensure memory safety
  ExtendibleHTableDirectoryPage() = delete;
  DISALLOW_COPY_AND_MOVE(ExtendibleHTableDirectoryPage);

  /**
   * After creating a new directory page from buffer pool, must call initialize
   * method to set default values
   * @param max_depth Max depth in the directory page
   */
  void Init(uint32_t max_depth = HTABLE_DIRECTORY_MAX_DEPTH);

  /**
   * Get the bucket index that the key is hashed to
   *
   * @param hash the hash of the key
   * @return bucket index current key is hashed to
   */
  auto HashToBucketIndex(uint32_t hash) const -> uint32_t;

  /**
   * Lookup a bucket page using a directory index
   *
   * @param bucket_idx the index in the directory to lookup
   * @return bucket page_id corresponding to bucket_idx
   */
  auto GetBucketPageId(uint32_t bucket_idx) const -> page_id_t;

  /**
   * Updates the directory index using a bucket index and page_id
   *
   * @param bucket_idx directory index at which to insert page_id
   * @param bucket_page_id page_id to insert
   */
  void SetBucketPageId(uint32_t bucket_idx, page_id_t bucket_page_id);

  /**
   * Gets the split image of an index
   *
   * @param bucket_idx the directory index for which to find the split image
   * @return the directory index of the split image
   **/
  auto GetSplitImageIndex(uint32_t bucket_idx) const -> uint32_t;

  auto GetSplitImageIndex(uint32_t bucket_idx, uint32_t depth) const -> uint32_t;

  /**
   * GetGlobalDepthMask - returns a mask of global_depth 1's and the rest 0's.
   *
   * In Extendible Hashing we map a key to a directory index
   * using the following hash + mask function.
   *
   * DirectoryIndex = Hash(key) & GLOBAL_DEPTH_MASK
   *
   * where GLOBAL_DEPTH_MASK is a mask with exactly GLOBAL_DEPTH 1's from LSB
   * upwards.  For example, global depth 3 corresponds to 0x00000007 in a 32-bit
   * representation.
   *
   * @return mask of global_depth 1's and the rest 0's (with 1's from LSB upwards)
   */
  auto GetGlobalDepthMask() const -> uint32_t;

  /**
   * GetLocalDepthMask - same as global depth mask, except it
   * uses the local depth of the bucket located at bucket_idx
   *
   * @param bucket_idx the index to use for looking up local depth
   * @return mask of local 1's and the rest 0's (with 1's from LSB upwards)
   */
  auto GetLocalDepthMask(uint32_t bucket_idx) const -> uint32_t;

  /**
   * Get the global depth of the hash table directory
   *
   * @return the global depth of the directory
   */
  auto GetGlobalDepth() const -> uint32_t;

  auto GetMaxDepth() const -> uint32_t;

  /**
   * Increment the global depth of the directory
   */
  void IncrGlobalDepth();

  /**
   * Decrement the global depth of the directory
   */
  void DecrGlobalDepth();

  /**
   * @return true if the directory can be shrunk
   */
  auto CanShrink() -> bool;

  /**
   * @return the current directory size
   */
  auto Size() const -> uint32_t;

  /**
   * @return the max directory size
   */
  auto MaxSize() const -> uint32_t;

  /**
   * Gets the local depth of the bucket at bucket_idx
   *
   * @param bucket_idx the bucket index to lookup
   * @return the local depth of the bucket at bucket_idx
   */
  auto GetLocalDepth(uint32_t bucket_idx) const -> uint32_t;

  /**
   * Set the local depth of the bucket at bucket_idx to local_depth
   *
   * @param bucket_idx bucket index to update
   * @param local_depth new local depth
   */
  void SetLocalDepth(uint32_t bucket_idx, uint8_t local_depth);

  /**
   * Increment the local depth of the bucket at bucket_idx
   * @param bucket_idx bucket index to increment
   */
  void IncrLocalDepth(uint32_t bucket_idx);

  /**
   * Decrement the local depth of the bucket at bucket_idx
   * @param bucket_idx bucket index to decrement
   */
  void DecrLocalDepth(uint32_t bucket_idx);

  /**
   * VerifyIntegrity
   *
   * Verify the following invariants:
   * (1) All LD <= GD.
   * (2) Each bucket has precisely 2^(GD - LD) pointers pointing to it.
   * (3) The LD is the same at each index with the same bucket_page_id
   */
  void VerifyIntegrity() const;

  /**
   * Prints the current directory
   */
  void PrintDirectory() const;

 private:
  uint32_t max_depth_;
  /*
  具体有三个用途：
1. 分裂 Bucket：知道需要检查 Hash 的第几个低位，以及应该修改哪些目录项。
2. 合并 Bucket：判断两个 Bucket 是否可以合并，也就是它们是否具有相同的 Local Depth。
3. 缩小 Directory：判断所有 Bucket 是否都不再需要当前的最高索引位。
最后再强调一句：

Local Depth 在逻辑上属于 Bucket，但在 BusTub 的物理实现中，它存放在 Directory Page 的 local_depths_ 数组里。
  */
 /*
 key和hashkey是一一对应的，hashkey和bucket_pages_ids_的索引是一一对应的
 假设globaldepth是3，那么bpi_的长度为8，8项的内容都指向同一个bucket页。也就是说只有一个桶。
 一开始localdepth都为0。开始放入直到桶满了，于是开始分裂。以0结尾的索引和以1结尾的索引分开成2个桶，localdepth++。
 如果两个桶的某一个又满了，比如000，010，100，110，也就是0，2，4，6.这4个会分成以00和10结尾的两个桶，这时候总共是3个桶。
 这4个的localdepth又++.
 */
  uint32_t global_depth_;
  uint8_t local_depths_[HTABLE_DIRECTORY_ARRAY_SIZE];
  page_id_t bucket_page_ids_[HTABLE_DIRECTORY_ARRAY_SIZE];
};

static_assert(sizeof(page_id_t) == 4);

static_assert(sizeof(ExtendibleHTableDirectoryPage) == HTABLE_DIRECTORY_PAGE_METADATA_SIZE +
                                                           HTABLE_DIRECTORY_ARRAY_SIZE +
                                                           sizeof(page_id_t) * HTABLE_DIRECTORY_ARRAY_SIZE);

static_assert(sizeof(ExtendibleHTableDirectoryPage) <= BUSTUB_PAGE_SIZE);

}  // namespace bustub
