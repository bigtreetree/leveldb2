// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_UTIL_ARENA_H_
#define STORAGE_LEVELDB_UTIL_ARENA_H_

#include <vector>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

namespace leveldb {

class Arena {
 public:
  Arena();
  ~Arena();

  // Return a pointer to a newly allocated memory block of "bytes" bytes.
  char* Allocate(size_t bytes);

  // Allocate memory with the normal alignment guarantees provided by malloc
  char* AllocateAligned(size_t bytes);

  // Returns an estimate of the total memory usage of data allocated
  // by the arena (including space allocated but not yet used for user
  // allocations).
  size_t MemoryUsage() const {
    return blocks_memory_ + blocks_.capacity() * sizeof(char*);
  }

 private:
  char* AllocateFallback(size_t bytes);
  char* AllocateNewBlock(size_t block_bytes);

  // Allocation state
  char* alloc_ptr_; 			//每分配一个block,记录当前可用offset指针
  size_t alloc_bytes_remaining_; 	//每分配一个block,记录当前可用bytes大小

  // Array of new[] allocated memory blocks
  std::vector<char*> blocks_; 	//每次分配的内存都存入blocks_中

  // Bytes of memory in blocks allocated so far
  size_t blocks_memory_; 	//当前已分配的内存容量

  // No copying allowed
  Arena(const Arena&);
  void operator=(const Arena&);
};

/*
 * 函 数:Allocate
 * 功 能:分配bytes大小的内存空间，返回分配的内存的指针
 */
inline char* Arena::Allocate(size_t bytes) {
  // The semantics of what to return are a bit messy if we allow
  // 0-byte allocations, so we disallow them here (we don't need
  // them for our internal use).
  assert(bytes > 0);
  if (bytes <= alloc_bytes_remaining_) { //当前block剩余空间可用
    char* result = alloc_ptr_; 		 //返回当前可用内存offset指针
    alloc_ptr_ += bytes; 		//向后偏移
    alloc_bytes_remaining_ -= bytes;    //当前剩余bytes大小
    return result;
  }
  return AllocateFallback(bytes);
}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_UTIL_ARENA_H_
