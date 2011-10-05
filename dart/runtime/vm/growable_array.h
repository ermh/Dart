// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Defines growable array classes, that differ where they are allocated:
// - GrowableArray: allocate on stack.
// - ZoneGrowableArray: allocated in the zone.

#ifndef VM_GROWABLE_ARRAY_H_
#define VM_GROWABLE_ARRAY_H_

#include "vm/allocation.h"
#include "vm/isolate.h"
#include "vm/utils.h"
#include "vm/zone.h"

namespace dart {

template<typename T, typename B>
class BaseGrowableArray : public B {
 public:
  BaseGrowableArray() : length_(0), capacity_(0), data_(NULL), zone_(NULL) {
    ASSERT(Isolate::Current() != NULL);
    zone_ = Isolate::Current()->current_zone();
  }

  explicit BaseGrowableArray(int initial_capacity)
      : length_(0), capacity_(0), data_(NULL), zone_(NULL) {
    ASSERT(Isolate::Current() != NULL);
    zone_ = Isolate::Current()->current_zone();
    if (initial_capacity > 0) {
      capacity_ = Utils::RoundUpToPowerOfTwo(initial_capacity);
      data_ = reinterpret_cast<T*>(zone_->Allocate(capacity_ * sizeof(T)));
    }
  }

  int length() const { return length_; }
  T* data() const { return data_; }
  bool is_empty() const { return length_ == 0; }

  void Add(const T& value) {
    Resize(length() + 1);
    Last() = value;
  }

  void RemoveLast() {
    ASSERT(length_ > 0);
    length_--;
  }

  T& operator[](int index) const {
    ASSERT(0 <= index);
    ASSERT(index < length_);
    ASSERT(length_ <= capacity_);
    return data_[index];
  }

  T& Last() const {
    ASSERT(length_ > 0);
    return operator[](length_ - 1);
  }

  void AddArray(const BaseGrowableArray<T, B>& src) {
    for (int i = 0; i < src.length(); i++) {
      Add(src[i]);
    }
  }

  void Clear() {
    length_ = 0;
  }

  // Sort the array in place.
  inline void Sort(int compare(const T*, const T*));

 private:
  int length_;
  int capacity_;
  T* data_;
  Zone* zone_;  // Zone in which we are allocating the array.

  void Resize(int new_length);

  DISALLOW_COPY_AND_ASSIGN(BaseGrowableArray);
};


template<typename T, typename B>
inline void BaseGrowableArray<T, B>::Sort(
    int compare(const T*, const T*)) {
  typedef int (*CompareFunction)(const void*, const void*);
  qsort(data_, length_, sizeof(T), reinterpret_cast<CompareFunction>(compare));
}


template<typename T, typename B>
void BaseGrowableArray<T, B>::Resize(int new_length) {
  if (new_length > capacity_) {
    ASSERT(Isolate::Current() != NULL);
    // Check that we allocating in the array's zone.
    ASSERT(zone_ == Isolate::Current()->current_zone());
    int new_capacity = Utils::RoundUpToPowerOfTwo(new_length);
    T* new_data = reinterpret_cast<T*>(
        zone_->Reallocate(reinterpret_cast<uword>(data_),
                          capacity_ * sizeof(T),
                          new_capacity * sizeof(T)));
    ASSERT(new_data != NULL);
    data_ = new_data;
    capacity_ = new_capacity;
  }
  length_ = new_length;
}


template<typename T>
class GrowableArray : public BaseGrowableArray<T, ValueObject> {
 public:
  explicit GrowableArray(int initial_capacity)
      : BaseGrowableArray<T, ValueObject>(initial_capacity) {}
  GrowableArray() : BaseGrowableArray<T, ValueObject>() {}
};


template<typename T>
class ZoneGrowableArray : public BaseGrowableArray<T, ZoneAllocated> {
 public:
  explicit ZoneGrowableArray(int initial_capacity)
      : BaseGrowableArray<T, ZoneAllocated>(initial_capacity) {}
  ZoneGrowableArray() : BaseGrowableArray<T, ZoneAllocated>() {}
};

}  // namespace dart

#endif  // VM_GROWABLE_ARRAY_H_
