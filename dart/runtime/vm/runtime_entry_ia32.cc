// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/globals.h"
#if defined(TARGET_ARCH_IA32)

#include "vm/runtime_entry.h"

#include "vm/assembler.h"
#include "vm/stub_code.h"

namespace dart {

#define __ assembler->


// Generate code to call into the stub which will call the runtime
// function. Input for the stub is as follows:
//   ESP : points to the arguments and return value array.
//   ECX : address of the runtime function to call.
//   EDX : number of arguments to the call.
void RuntimeEntry::CallFromDart(Assembler* assembler) const {
  __ movl(ECX, Immediate(GetEntryPoint()));
  __ movl(EDX, Immediate(argument_count()));
  __ call(&StubCode::DartCallToRuntimeLabel());
}


// Generate code to call into the stub which will call the runtime
// function. Input for the stub is as follows:
//   ESP : points to the arguments and return value array.
//   ECX : address of the runtime function to call.
//   EDX : number of arguments to the call.
void RuntimeEntry::CallFromStub(Assembler* assembler) const {
  __ movl(ECX, Immediate(GetEntryPoint()));
  __ movl(EDX, Immediate(argument_count()));
  __ call(&StubCode::StubCallToRuntimeLabel());
}

}  // namespace dart

#endif  // defined TARGET_ARCH_IA32
