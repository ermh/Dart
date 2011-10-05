// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Class for patching compiled code.

#ifndef VM_CODE_PATCHER_H_
#define VM_CODE_PATCHER_H_

#include "vm/allocation.h"

namespace dart {

// Forward declaration.
class Code;
class ExternalLabel;
class Function;
class String;

class CodePatcher : public AllStatic {
 public:
  // Dart static calls have a distinct, machine-dependent code pattern.

  // Patch static call to the new target.
  static void PatchStaticCallAt(uword addr, uword new_target_address);

  // Overwrites code at 'at_addr' with a call to 'label'.
  static void InsertCall(uword at_addr, const ExternalLabel* label);

  // Overwrites code at 'at_addr' with a jump to 'label'.
  static void InsertJump(uword at_addr, const ExternalLabel* label);

  // Patch entry point with a jump as specified in the code's patch region.
  static void PatchEntry(const Code& code);

  // Restore entry point with original code (i.e., before patching).
  static void RestoreEntry(const Code& code);

  // Returns true if the code can be patched with a jump at beginnning (checks
  // that there are no conflicts with object pointers).
  static bool CodeIsPatchable(const Code& code);

  // Get static call information.
  static void GetStaticCallAt(uword return_address,
                              Function* function,
                              uword* target);

  // Patch instance call to the new target.
  static void PatchInstanceCallAt(uword addr, uword new_target_address);

  // Get instance call information.
  static void GetInstanceCallAt(uword return_address,
                                String* function_name,
                                int* num_arguments,
                                int* num_named_arguments,
                                uword* target);
};

}  // namespace dart

#endif  // VM_CODE_PATCHER_H_
