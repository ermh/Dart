// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/globals.h"
#if defined(TARGET_ARCH_IA32)

#include "vm/instructions.h"
#include "vm/isolate.h"
#include "vm/stack_frame.h"

namespace dart {

// The constant kExitLinkOffsetInEntryFrame must be kept in sync with the
// code in the InvokeDartCode stub.
static const int kExitLinkOffsetInEntryFrame = -4 * kWordSize;
static const int kPcAddressOffsetFromSp = -1 * kWordSize;
static const int kSpOffsetFromPreviousFp = 2 * kWordSize;


static bool ExitedFromStub(uword exit_marker) {
  if (exit_marker != 0) {
    uword caller_pc = *reinterpret_cast<uword*>(exit_marker + kWordSize);
    uword call_instr_pc = caller_pc - Call::InstructionLength();
    uword call_target = Call(call_instr_pc).TargetAddress();
    return StubCode::InStubCallToRuntimeStubCode(call_target);
  }
  return false;
}


intptr_t StackFrame::PcAddressOffsetFromSp() {
  return kPcAddressOffsetFromSp;
}


uword StackFrame::GetCallerSp() const {
  return fp() + kSpOffsetFromPreviousFp;
}


uword StackFrame::GetCallerFp() const {
  return *(reinterpret_cast<uword*>(fp()));
}


intptr_t EntryFrame::ExitLinkOffset() {
  return kExitLinkOffsetInEntryFrame;
}


void StackFrameIterator::SetupLastExitFrameData() {
  Isolate* current = Isolate::Current();
  uword exit_marker = current->top_exit_frame_info();
  frames_.fp_ = exit_marker;
  frames_.from_stub_exitframe_ = ExitedFromStub(exit_marker);
}


void StackFrameIterator::SetupNextExitFrameData() {
  uword exit_address = entry_.fp() + kExitLinkOffsetInEntryFrame;
  uword exit_marker = *reinterpret_cast<uword*>(exit_address);
  frames_.fp_ = exit_marker;
  frames_.from_stub_exitframe_ = ExitedFromStub(exit_marker);
  frames_.sp_ = 0;
}

}  // namespace dart

#endif  // defined TARGET_ARCH_IA32
