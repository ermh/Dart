// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef VM_C99_SUPPORT_WIN_H_
#define VM_C99_SUPPORT_WIN_H_

// Visual C++ is missing a bunch of C99 math macros and
// functions. Define them here.

#include <float.h>

static const unsigned __int64 kQuietNaNMask =
    static_cast<unsigned __int64>(0xfff) << 51;

#define INFINITY HUGE_VAL
#define NAN \
    *reinterpret_cast<const double*>(&kQuietNaNMask)

static inline int isinf(double x) {
  return (_fpclass(x) & (_FPCLASS_PINF | _FPCLASS_NINF)) != 0;
}

static inline int isnan(double x) {
  return _isnan(x);
}

static inline int signbit(double x) {
  if (x == 0) {
    return _fpclass(x) & _FPCLASS_NZ;
  } else {
    return x < 0;
  }
}

static inline double trunc(double x) {
  if (x < 0) {
    return ceil(x);
  } else {
    return floor(x);
  }
}

static inline double round(double x) {
  if (x < 0) {
    return ceil(x - 0.5);
  } else {
    return floor(x + 0.5);
  }
}

#endif  // VM_C99_SUPPORT_WIN_H_
