// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_INCLUDE_TABLE_H_
#define STORAGE_LEVELDB_INCLUDE_TABLE_H_

#include <stdint.h>
#include "leveldb/iterator.h"

namespace leveldb {

class Block;
class BlockHandle;
class Footer;
struct Options;
class RandomAccessFile;
struct ReadOptions;
class TableCache;

// A Table is a sorted map from strings to strings.  Tables are
// immutable and persistent.  A Table may be safely accessed from
// multiple threads without external synchronization.
class Table {
 public:
  // Attempt to open the table that is stored in bytes [0..file_size)
  // of "file", and read the metadata entries necessary to allow
  // retrieving data from the table.
  //
  // If successful, returns ok and sets "*table" to the newly opened
  // table.  The client should delete "*table" when no longer needed.
  // If there was an error while initializing the table, sets "*table"
  // to NULL and returns a non-ok status.  Does not take ownership of
  // "*source", but the client must ensure that "source" remains live
  // for the duration of the returned table's lifetime.
  //
  // *file must remain live while this Table is in use.
  //如果操作成功，指针*table指向新打开的表，否则返回错误,要打开的文件和大小分别由参数file和file_size指定；option是一些选项；
  static Status Open(const Options& options,
                     RandomAccessFile* file,
                     uint64_t file_size,
                     Table** table);

  ~Table();

  // Returns a new iterator over the table contents.
  // The result of NewIterator() is initially invalid (caller must
  // call one of the Seek methods on the iterator before using it).
  Iterator* NewIterator(const ReadOptions&) const;

  // Given a key, return an approximate byte offset in the file where
  // the data for that key begins (or would begin if the key were
  // present in the file).  The returned value is in terms of file
  // bytes, and so includes effects like compression of the underlying data.
  // E.g., the approximate offset of the last key in the table will
  // be close to the file length.
  //这里并不是精确的定位，而是在Table中找到第一个>=指定key的k/v对，然后返回其value在sstable文件中的偏移。
  uint64_t ApproximateOffsetOf(const Slice& key) const;

 private:
  struct Rep;
  Rep* rep_;

  explicit Table(Rep* rep) { rep_ = rep; }
  //它根据参数指明的blockdata，返回一个iterator对象，调用者就可以通过这个iterator对象遍历blockdata存储的k/v对，
  static Iterator* BlockReader(void*, const ReadOptions&, const Slice&);

  // Calls (*handle_result)(arg, ...) with the entry found after a call
  // to Seek(key).  May not make such a call if filter policy says
  // that key is not present.
  friend class TableCache;
  //这是为TableCache开的一个口子
  Status InternalGet(
      const ReadOptions&, const Slice& key,
      void* arg,
      void (*handle_result)(void* arg, const Slice& k, const Slice& v));


  void ReadMeta(const Footer& footer);
  void ReadFilter(const Slice& filter_handle_value);

  // No copying allowed
  Table(const Table&);
  void operator=(const Table&);
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_TABLE_H_
