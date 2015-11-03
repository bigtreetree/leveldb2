// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Endian-neutral encoding:
// * Fixed-length numbers are encoded with least-significant byte first
// * In addition we support variable length "varint" encoding
// * Strings are encoded prefixed by their length in varint format

#ifndef STORAGE_LEVELDB_UTIL_CODING_H_
#define STORAGE_LEVELDB_UTIL_CODING_H_

#include <stdint.h>
#include <string.h>
#include <string>
#include "leveldb/slice.h"
#include "port/port.h"

namespace leveldb {

// Standard Put... routines append to a string
/*
 * 函 数:PutFixed32
 * 功 能:把uint32_t value 按小端模式转换到char buf中，然后，把char buf追加到dst后面
 */
extern void PutFixed32(std::string* dst, uint32_t value);
/*
 * 函 数:PutFixed64
 * 功 能:把uint64_t value 按小端模式转换到 char buf中，然后,把char buf追加到dst后面
 */
extern void PutFixed64(std::string* dst, uint64_t value);

/*
 * 函 数:PutVarint32
 * 功 能:按varint压缩方法把value追加到dst后面
 */
extern void PutVarint32(std::string* dst, uint32_t value);
/*
 * 函 数:PutVarint64
 * 功 能:按varint压缩方法把value追加到dst后面
 */
extern void PutVarint64(std::string* dst, uint64_t value);
/*
 * 函 数:PutLengthPrefixedSlice
 * 功 能:把Slice 类型的value加入到dst中(注意加入时先按fixed方法加入value的长度，然后再加value的内容)
 */
extern void PutLengthPrefixedSlice(std::string* dst, const Slice& value);

// Standard Get... routines parse a value from the beginning of a Slice
// and advance the slice past the parsed value.
extern bool GetVarint32(Slice* input, uint32_t* value);
extern bool GetVarint64(Slice* input, uint64_t* value);
extern bool GetLengthPrefixedSlice(Slice* input, Slice* result);

// Pointer-based variants of GetVarint...  These either store a value
// in *v and return a pointer just past the parsed value, or return
// NULL on error.  These routines only look at bytes in the range
// [p..limit-1]

/*
 * 函 数:GetVarint32Ptr
 * 功 能:从p到limit或4字节按varint方法解析出值保存到v
 * 返回值:返回解析之后,p的新下标地址，错误返回NULL
 */
extern const char* GetVarint32Ptr(const char* p,const char* limit, uint32_t* v);
extern const char* GetVarint64Ptr(const char* p,const char* limit, uint64_t* v);

// Returns the length of the varint32 or varint64 encoding of "v"
/*
 * 函 数:VarintLength
 * 功 能:返回v的字节长度
 */
extern int VarintLength(uint64_t v);

// Lower-level versions of Put... that write directly into a character buffer
// REQUIRES: dst has enough space for the value being written
/*
 * 函 数:EncodeFixed32
 * 功 能:把uint32_t value 按照小端模式存储到dst中
 */
extern void EncodeFixed32(char* dst, uint32_t value);
/*
 *函 数:EncodeFixed64
 *功 能:把uint64_t value 按照小端模式存储到dst中
 */
extern void EncodeFixed64(char* dst, uint64_t value);

// Lower-level versions of Put... that write directly into a character buffer
// and return a pointer just past the last byte written.
// REQUIRES: dst has enough space for the value being written
extern char* EncodeVarint32(char* dst, uint32_t value);
extern char* EncodeVarint64(char* dst, uint64_t value);

// Lower-level versions of Get... that read directly from a character buffer
// without any bounds checking.

/*
 * 函 数:DecodeFixed32
 * 功 能:把ptr 转换成uint32_t
 */
inline uint32_t DecodeFixed32(const char* ptr) {
  if (port::kLittleEndian) {
    // Load the raw bytes
    uint32_t result;
    memcpy(&result, ptr, sizeof(result));  // gcc optimizes this to a plain load
    return result;
  } else {
    return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[0])))
        | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8)
        | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16)
        | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24));
  }
}
/*
 * 函 数:DecodeFixed64
 * 功 能:把ptr转换成uint64_t
 */

inline uint64_t DecodeFixed64(const char* ptr) {
  if (port::kLittleEndian) {
    // Load the raw bytes
    uint64_t result;
    memcpy(&result, ptr, sizeof(result));  // gcc optimizes this to a plain load
    return result;
  } else {
    uint64_t lo = DecodeFixed32(ptr);
    uint64_t hi = DecodeFixed32(ptr + 4);
    return (hi << 32) | lo;
  }
}

// Internal routine for use by fallback path of GetVarint32Ptr
extern const char* GetVarint32PtrFallback(const char* p,
                                          const char* limit,
                                          uint32_t* value);
//返回varint32之后的起始地址, value用来存放计算出uint32值
inline const char* GetVarint32Ptr(const char* p,
                                  const char* limit,
                                  uint32_t* value) {
  if (p < limit) {
    //第一个字节的值赋值给result
    uint32_t result = *(reinterpret_cast<const unsigned char*>(p));
    //如果result的最高位为0，说明数字只占一个字节
    if ((result & 128) == 0) {
      *value = result;
      return p + 1;
    }
  }
  //varint32　占用了多个字节,(uint32最多占用５个字节)
  return GetVarint32PtrFallback(p, limit, value);
}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_UTIL_CODING_H_
