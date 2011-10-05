// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/globals.h"
#if defined(TARGET_ARCH_X64)

#include "vm/runtime_entry.h"

namespace dart {

void RuntimeEntry::CallFromDart(Assembler* assembler) const {
  UNIMPLEMENTED();
}

void RuntimeEntry::CallFromStub(Assembler* assembler) const {
  UNIMPLEMENTED();
}

}  // namespace dart

#endif  // defined TARGET_ARCH_X64
