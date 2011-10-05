// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef VM_VERIFIER_H_
#define VM_VERIFIER_H_

#include "vm/flags.h"
#include "vm/globals.h"
#include "vm/visitor.h"

namespace dart {

DECLARE_FLAG(bool, verify_on_transition);

#define VERIFY_ON_TRANSITION                                                   \
  if (FLAG_verify_on_transition) {                                             \
    VerifyPointersVisitor::VerifyPointers();                                   \
    Isolate::Current()->heap()->Verify();                                      \
  }                                                                            \


// Forward declarations.
class RawObject;

// A sample object pointer visitor implementation which verifies that
// the pointers visited are contained in the isolate heap.
class VerifyPointersVisitor : public ObjectPointerVisitor {
 public:
  VerifyPointersVisitor() {}

  virtual void VisitPointers(RawObject** first, RawObject** last);

  static void VerifyPointers();
};

}  // namespace dart

#endif  // VM_VERIFIER_H_
