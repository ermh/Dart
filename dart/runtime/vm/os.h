// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef VM_OS_H_
#define VM_OS_H_

#include "vm/globals.h"

namespace dart {

// Forward declarations.
class Isolate;

// Interface to the underlying OS platform.
class OS {
 public:
  typedef struct BrokenDownDateTime {
    int year;   // Offset by 1900. A value of 111 Represents the year 2011.
    int month;  // [0..11]
    int day;    // [1..31]
    int hours;
    int minutes;
    int seconds;
  } BrokenDownDateTime;

  // Takes the seconds since epoch (midnight, January 1, 1970 UTC) and breaks it
  // down into date and time.
  // If 'inUtc', then the broken down date and time are in the UTC timezone,
  // otherwise the local timezone is used.
  // The returned year is offset by 1900. The returned month is 0-based.
  // Returns true if the conversion succeeds, false otherwise.
  static bool BreakDownSecondsSinceEpoch(time_t seconds_since_epoch,
                                         bool in_utc,
                                         BrokenDownDateTime* result);

  // Converts a broken down date into the seconds since epoch (midnight,
  // January 1, 1970 UTC). Returns true if the conversion succeeds, false
  // otherwise.
  static bool BrokenDownToSecondsSinceEpoch(
      const BrokenDownDateTime& broken_down, bool in_utc, time_t* result);

  // Returns the current time in milliseconds measured
  // from midnight January 1, 1970 UTC.
  static int64_t GetCurrentTimeMillis();

  // Returns the current time in microseconds measured
  // from midnight January 1, 1970 UTC.
  static int64_t GetCurrentTimeMicros();

  // Returns the activation frame alignment constraint or zero if
  // the platform doesn't care. Guaranteed to be a power of two.
  static word ActivationFrameAlignment();

  // Returns the preferred code alignment or zero if
  // the platform doesn't care. Guaranteed to be a power of two.
  static word PreferredCodeAlignment();

  // Returns the stack size limit.
  static uword GetStackSizeLimit();

  // Returns number of available processor cores.
  static int NumberOfAvailableProcessors();

  // Sleep the currently executing thread for millis ms.
  static void Sleep(int64_t millis);

  // Debug break.
  static void DebugBreak();

  // Print formatted output to stdout/stderr for debugging.
  static void Print(const char* format, ...);
  static void PrintErr(const char* format, ...);
  static void VFPrint(FILE* stream, const char* format, va_list args);
  // Print formatted output info a buffer.
  // Does not write more than size characters (including the trailing '\0').
  // Returns the number of characters (excluding the trailing '\0') that would
  // been written if the buffer had been big enough.
  // If the return value is greater or equal than the given size then the output
  // has been truncated.
  // The buffer will always be terminated by a '\0', unless the buffer is of
  // size 0.
  // The buffer might be NULL if the size is 0.
  // This specification conforms to C99 standard which is implemented by
  // glibc 2.1+.
  static int SNPrint(char* str, size_t size, const char* format, ...);
  static int VSNPrint(char* str, size_t size,
                      const char* format,
                      va_list args);

  // Initialize the OS class.
  static void InitOnce();

  // Shut down the OS class.
  static void Shutdown();

  static void Abort();

  static void Exit(int code);
};

}  // namespace dart

#endif  // VM_OS_H_
