// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef VM_SCAVENGER_H_
#define VM_SCAVENGER_H_

#include "vm/globals.h"
#include "vm/object.h"
#include "vm/utils.h"
#include "vm/virtual_memory.h"

namespace dart {

// Forward declarations.
class Heap;

class Scavenger {
 public:
  Scavenger(Heap* heap, intptr_t max_capacity, uword object_alignment);
  ~Scavenger();

  // Check whether this Scavenger contains this address.
  // During scavenging both the to and from spaces contain "legal" objects.
  // During a scavenge this function only returns true for addresses that will
  // be part of the surviving objects.
  bool Contains(uword addr) const {
    // No reasonable algorithm should be checking for objects in from space. At
    // least unless it is debugging code. This might need to be relaxed later,
    // but currently it helps prevent dumb bugs.
    ASSERT(!from_->Contains(addr));
    return to_->Contains(addr);
  }

  uword TryAllocate(intptr_t size) {
    ASSERT(Utils::IsAligned(size, kObjectAlignment));
#if defined(DEBUG)
    if (FLAG_gc_at_alloc && !scavenging_) {
      Scavenge();
    }
#endif
    uword result = top_;
    intptr_t remaining = end_ - top_;
    if (remaining < size) {
      return 0;
    }
    ASSERT(to_->Contains(result));
    ASSERT((result & kObjectAlignmentMask) == object_alignment_);

    top_ += size;
    ASSERT(to_->Contains(top_) || (top_ == to_->end()));
    return result;
  }

  // Collect the garbage in this scavenger.
  void Scavenge();

  // Accessors to generate code for inlined allocation.
  uword* TopAddress() { return &top_; }
  uword* EndAddress() { return &end_; }
  static intptr_t top_offset() { return OFFSET_OF(Scavenger, top_); }
  static intptr_t end_offset() { return OFFSET_OF(Scavenger, end_); }

  intptr_t in_use() const { return (top_ - FirstObjectStart()); }

  void VisitObjectPointers(ObjectPointerVisitor* visitor) const;

 private:
  uword FirstObjectStart() const { return to_->start() | object_alignment_; }
  void Prologue();
  void IterateRoots(ObjectPointerVisitor* visitor);
  void ProcessToSpace(ObjectPointerVisitor* visitor);
  void Epilogue();

  VirtualMemory* space_;
  MemoryRegion* to_;
  MemoryRegion* from_;

  Heap* heap_;

  // Current allocation top and end. These values are being accessed directly
  // from generated code.
  uword top_;
  uword end_;

  // All object are aligned to this value.
  uword object_alignment_;

  // Scavenge cycle count.
  int count_;
  // Keep track whether a scavenge is currently running.
  bool scavenging_;

  friend class ScavengerVisitor;

  DISALLOW_COPY_AND_ASSIGN(Scavenger);
};

}  // namespace dart

#endif  // VM_SCAVENGER_H_
