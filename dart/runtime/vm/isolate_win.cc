// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/assert.h"
#include "vm/globals.h"
#include "vm/isolate.h"

namespace dart {

DWORD isolate_key = TLS_OUT_OF_INDEXES;


void Isolate::SetCurrent(Isolate* current) {
  ASSERT(isolate_key != TLS_OUT_OF_INDEXES);
  BOOL result = TlsSetValue(isolate_key, current);
  if (!result) {
    FATAL("TlsSetValue failed");
  }
}


// Empty isolate init callback which is registered before VM isolate creation.
static void* VMIsolateInitCallback(void* data) {
  return reinterpret_cast<void*>(1);
}


void Isolate::InitOnce() {
  ASSERT(isolate_key == TLS_OUT_OF_INDEXES);
  isolate_key = TlsAlloc();
  if (isolate_key == TLS_OUT_OF_INDEXES) {
    FATAL("TlsAlloc failed");
  }
  init_callback_ = VMIsolateInitCallback;
}

}  // namespace dart
