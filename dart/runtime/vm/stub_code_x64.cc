// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/globals.h"
#if defined(TARGET_ARCH_X64)

#include "vm/stub_code.h"

#define __ assembler->

namespace dart {

void StubCode::GenerateDartCallToRuntimeStub(Assembler* assembler) {
  __ Unimplemented("DartCallToRuntime stub");
}


void StubCode::GenerateStubCallToRuntimeStub(Assembler* assembler) {
  __ Unimplemented("StubCallToRuntime stub");
}


void StubCode::GenerateCallNativeCFunctionStub(Assembler* assembler) {
  __ Unimplemented("CallNativeCFunction stub");
}


void StubCode::GenerateCallNoSuchMethodFunctionStub(Assembler* assembler) {
  __ Unimplemented("CallNoSuchMethodFunction stub");
}


void StubCode::GenerateInvokeDartCodeStub(Assembler* assembler) {
  __ Unimplemented("InvokeDartCode stub");
}


void StubCode::GenerateCallStaticFunctionStub(Assembler* assembler) {
  __ Unimplemented("CallStaticFunction stub");
}


void StubCode::GenerateMegamorphicLookupStub(Assembler* assembler) {
  __ Unimplemented("MegamorphicLookup stub");
}


void StubCode::GenerateCallInstanceFunctionStub(Assembler* assembler) {
  __ Unimplemented("CallInstanceFunction stub");
}


void StubCode::GenerateCallClosureFunctionStub(Assembler* assembler) {
  __ Unimplemented("CallClosureFunction stub");
}


void StubCode::GenerateAllocateContextStub(Assembler* assembler) {
  __ Unimplemented("AllocateContext stub");
}


void StubCode::GenerateAllocationStubForClass(Assembler* assembler,
                                              const Class& cls) {
  __ Unimplemented("AllocateObject stub");
}


void StubCode::GenerateAllocationStubForClosure(Assembler* assembler,
                                                const Function& func) {
  __ Unimplemented("AllocateClosure stub");
}


void StubCode::GenerateAllocationStubForStaticImplicitClosure(
    Assembler* assembler, const Function& func) {
  __ Unimplemented("AllocateStaticImplicitClosure stub");
}

}  // namespace dart

#endif  // defined TARGET_ARCH_X64
