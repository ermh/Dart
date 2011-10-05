// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef VM_VISITOR_H_
#define VM_VISITOR_H_

#include "vm/globals.h"

namespace dart {

// Forward declarations.
class RawObject;

// An object pointer visitor interface.
class ObjectPointerVisitor {
 public:
  virtual ~ObjectPointerVisitor() {}

  // Range of pointers to visit 'first' <= pointer <= 'last'.
  virtual void VisitPointers(RawObject** first, RawObject** last) = 0;

  // len argument is the number of pointers to visit starting from 'p'.
  void VisitPointers(RawObject** p, intptr_t len) {
    VisitPointers(p, (p + len - 1));
  }

  void VisitPointer(RawObject** p) { VisitPointers(p , p); }
};

}  // namespace dart

#endif  // VM_VISITOR_H_
