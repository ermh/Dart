// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef VM_BOOTSTRAP_NATIVES_H_
#define VM_BOOTSTRAP_NATIVES_H_

#include "vm/native_entry.h"

// bootstrap dart natives used in the core dart library.

namespace dart {

// List of bootstrap native entry points used in the core dart library.
#define BOOTSTRAP_NATIVE_LIST(V)                                               \
  V(Object_toString, 1)                                                        \
  V(Object_noSuchMethod, 3)                                                    \
  V(Integer_bitAndFromInteger, 2)                                              \
  V(Integer_bitOrFromInteger, 2)                                               \
  V(Integer_bitXorFromInteger, 2)                                              \
  V(Integer_addFromInteger, 2)                                                 \
  V(Integer_subFromInteger, 2)                                                 \
  V(Integer_mulFromInteger, 2)                                                 \
  V(Integer_truncDivFromInteger, 2)                                            \
  V(Integer_moduloFromInteger, 2)                                              \
  V(Integer_greaterThanFromInteger, 2)                                         \
  V(Integer_equalToInteger, 2)                                                 \
  V(IsolateNatives_start, 2)                                                   \
  V(ReceivePortImpl_factory, 1)                                                \
  V(ReceivePortImpl_closeInternal, 1)                                          \
  V(SendPortImpl_sendInternal_, 3)                                             \
  V(Smi_shlFromInt, 2)                                                         \
  V(Smi_sarFromInt, 2)                                                         \
  V(Smi_bitNegate, 1)                                                          \
  V(Mint_bitNegate, 1)                                                         \
  V(Bigint_bitNegate, 1)                                                       \
  V(Double_add, 2)                                                             \
  V(Double_sub, 2)                                                             \
  V(Double_mul, 2)                                                             \
  V(Double_div, 2)                                                             \
  V(Double_trunc_div, 2)                                                       \
  V(Double_remainder, 2)                                                       \
  V(Double_modulo, 2)                                                          \
  V(Double_greaterThanFromInteger, 2)                                          \
  V(Double_equalToInteger, 2)                                                  \
  V(Double_greaterThan, 2)                                                     \
  V(Double_equal, 2)                                                           \
  V(Double_isNegative, 1)                                                      \
  V(Double_isInfinite, 1)                                                      \
  V(Double_isNaN, 1)                                                           \
  V(Double_doubleFromInteger, 2)                                               \
  V(Double_round, 1)                                                           \
  V(Double_floor, 1)                                                           \
  V(Double_ceil, 1)                                                            \
  V(Double_truncate, 1)                                                        \
  V(Double_toInt, 1)                                                           \
  V(Double_pow, 2)                                                             \
  V(JSSyntaxRegExp_factory, 3)                                                 \
  V(JSSyntaxRegExp_getPattern, 1)                                              \
  V(JSSyntaxRegExp_getFlags, 1)                                                \
  V(JSSyntaxRegExp_getGroupCount, 1)                                           \
  V(JSSyntaxRegExp_ExecuteMatch, 3)                                            \
  V(ObjectArray_allocate, 2)                                                   \
  V(ObjectArray_getIndexed, 2)                                                 \
  V(ObjectArray_setIndexed, 3)                                                 \
  V(ObjectArray_getLength, 1)                                                  \
  V(ObjectArray_copyFromObjectArray, 5)                                        \
  V(StringBase_createFromCodePoints, 1)                                        \
  V(String_hashCode, 1)                                                        \
  V(String_getLength, 1)                                                       \
  V(String_charAt, 2)                                                          \
  V(String_charCodeAt, 2)                                                      \
  V(String_concat, 2)                                                          \
  V(Strings_concatAll, 1)                                                      \
  V(MathNatives_sqrt, 1)                                                       \
  V(MathNatives_sin, 1)                                                        \
  V(MathNatives_cos, 1)                                                        \
  V(MathNatives_tan, 1)                                                        \
  V(MathNatives_asin, 1)                                                       \
  V(MathNatives_acos, 1)                                                       \
  V(MathNatives_atan, 1)                                                       \
  V(MathNatives_2atan, 2)                                                      \
  V(MathNatives_exp, 1)                                                        \
  V(MathNatives_log, 1)                                                        \
  V(MathNatives_random, 0)                                                     \
  V(MathNatives_parseInt, 1)                                                   \
  V(MathNatives_parseDouble, 1)                                                \
  V(DateTimeNatives_brokenDownToSecondsSinceEpoch, 7)                          \
  V(DateTimeNatives_currentTimeMillis, 0)                                      \
  V(DateTimeNatives_getYear, 2)                                                \
  V(DateTimeNatives_getMonth, 2)                                               \
  V(DateTimeNatives_getDay, 2)                                                 \
  V(DateTimeNatives_getHours, 2)                                               \
  V(DateTimeNatives_getMinutes, 2)                                             \
  V(DateTimeNatives_getSeconds, 2)                                             \
  V(AssertError_throwNew, 2)                                                   \
  V(FallThroughError_throwNew, 1)                                              \
  V(Clock_now, 0)                                                              \
  V(Clock_frequency, 0)                                                        \


BOOTSTRAP_NATIVE_LIST(DECLARE_NATIVE_ENTRY)

}  // namespace dart

#endif  // VM_BOOTSTRAP_NATIVES_H_
