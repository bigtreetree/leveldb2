// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_SNAPSHOT_H_
#define STORAGE_LEVELDB_DB_SNAPSHOT_H_

#include "db/dbformat.h"
#include "leveldb/db.h"

namespace leveldb {

class SnapshotList;

// Snapshots are kept in a doubly-linked list in the DB.
// Each SnapshotImpl corresponds to a particular sequence number.
class SnapshotImpl : public Snapshot {
 public:
  SequenceNumber number_;  // const after creation

 private:
  friend class SnapshotList;

  // SnapshotImpl is kept in a doubly-linked circular list
  SnapshotImpl* prev_;
  SnapshotImpl* next_;

//sanity 心智健康，通情达理
  SnapshotList* list_;                 // just for sanity checks
					//list_ 该SnapshortImpl所在的双向链表
};

/*
 * SnapshotList是SnapshotImpl的双向环链表
 */
class SnapshotList {
 public:
  SnapshotList() {
    list_.prev_ = &list_;
    list_.next_ = &list_;
  }

  bool empty() const { return list_.next_ == &list_; }
	//返回双向链表中最早插入的那个结点
  SnapshotImpl* oldest() const { assert(!empty()); return list_.next_; }
//返回双向链表中最晚插入的那个结点
  SnapshotImpl* newest() const { assert(!empty()); return list_.prev_; }

//向双向链表中插入一个新的SequenceNumber(一个SequenceNumber即一个SnapshortImpl)
  const SnapshotImpl* New(SequenceNumber seq) {
    SnapshotImpl* s = new SnapshotImpl;
    s->number_ = seq;
    s->list_ = this;
    s->next_ = &list_;
    s->prev_ = list_.prev_;
    s->prev_->next_ = s;
    s->next_->prev_ = s;
    return s;
  }

//从双向链表中删除结点s
  void Delete(const SnapshotImpl* s) {
    assert(s->list_ == this);
    s->prev_->next_ = s->next_;
    s->next_->prev_ = s->prev_;
    delete s;
  }

 private:
  // Dummy head of doubly-linked list of snapshots
  SnapshotImpl list_;//双向链表的起始结点
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_SNAPSHOT_H_
