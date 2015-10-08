// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/log_writer.h"

#include <stdint.h>
#include "leveldb/env.h"
#include "util/coding.h"
#include "util/crc32c.h"

namespace leveldb {
namespace log {

Writer::Writer(WritableFile* dest)
    : dest_(dest),
      block_offset_(0) {
  for (int i = 0; i <= kMaxRecordType; i++) {
    char t = static_cast<char>(i);
    type_crc_[i] = crc32c::Value(&t, 1);
  }
}

Writer::~Writer() {
}

Status Writer::AddRecord(const Slice& slice) {
  const char* ptr = slice.data();
  size_t left = slice.size();

  // Fragment the record if necessary and emit it.  Note that if slice
  // is empty, we still want to iterate once to emit a single
  // zero-length record
  Status s;
  bool begin = true;
//do while循环,直到写入出错，或者成功写入全部数据
  do {
    const int leftover = kBlockSize - block_offset_;
    assert(leftover >= 0);
    if (leftover < kHeaderSize) {
	//本块中剩余空间不足7,即log recode header
      // Switch to a new block
      // 注意这里的更换新的块并不需要额外的代码进行控制，只要把当前块写満就可以了，写过之后，它就自动从逻辑上开始向下一个块写
      if (leftover > 0) {
        // Fill the trailer (literal below relies on kHeaderSize being 7)
        assert(kHeaderSize == 7);
        dest_->Append(Slice("\x00\x00\x00\x00\x00\x00", leftover));//"\x"表示是16进制
      }
      block_offset_ = 0;
    }

    // Invariant: we never leave < kHeaderSize bytes in a block.
    assert(kBlockSize - block_offset_ - kHeaderSize >= 0);

//计算block剩余大小,以及本次log record 可写入数据长度
    const size_t avail = kBlockSize - block_offset_ - kHeaderSize; //剩余空间中除去包头可写的包体长度
    const size_t fragment_length = (left < avail) ? left : avail;//可写入数据长度 为剩余空间和包体两者中小者


    RecordType type;
    const bool end = (left == fragment_length);//两者相等，当前块可以写完包体
    if (begin && end) { 
      type = kFullType;
    } else if (begin) {
      type = kFirstType;
    } else if (end) {
      type = kLastType;
    } else {
      type = kMiddleType;
    }
//调用EmitPhysicalRecord ,append日志，并更新指针,剩余长度和begin标记
    s = EmitPhysicalRecord(type, ptr, fragment_length);
    ptr += fragment_length;
    left -= fragment_length;
    begin = false;
  } while (s.ok() && left > 0);
  return s;
}

/*
 * 函 数:EmitPhysicalRecord
 * 功 能:实际写日志
 * 参 数:ptr为用户record数据,参数n为写入当前块的长度,不包含log header 
 */
//emit 发射，发出
Status Writer::EmitPhysicalRecord(RecordType t, const char* ptr, size_t n) {
  assert(n <= 0xffff);  // Must fit in two bytes ,因为每个块为32k，即256 * 4 * 32 大小，故n 是<=65536
  assert(block_offset_ + kHeaderSize + n <= kBlockSize);

//封装记录头
  // Format the header
  // 记录长度和类型
  char buf[kHeaderSize];
  buf[4] = static_cast<char>(n & 0xff); //buf[4],buf[5]存放数据长度
  buf[5] = static_cast<char>(n >> 8);
  buf[6] = static_cast<char>(t); //buf[6]存放record type

  // Compute the crc of the record type and the payload.
  //计算校验码
  uint32_t crc = crc32c::Extend(type_crc_[t], ptr, n);
  crc = crc32c::Mask(crc);                 // Adjust for storage
  EncodeFixed32(buf, crc); 	//buf[0], buf[1],buf[2],buf[3]存放crc校验和

  // Write the header and the payload
  // 写入头部
  Status s = dest_->Append(Slice(buf, kHeaderSize));
  if (s.ok()) {
	//写入包体
    s = dest_->Append(Slice(ptr, n));
    if (s.ok()) {
	//刷新
      s = dest_->Flush();
    }
  }
  //更新当前偏移
  block_offset_ += kHeaderSize + n;
  return s;
}

}  // namespace log
}  // namespace leveldb
