// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <errno.h>
#include <limits.h>
#include <mach/mach.h>
#include <mach/clock.h>
#include <mach/mach_time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include "vm/isolate.h"
#include "vm/os.h"
#include "vm/utils.h"

namespace dart {

bool OS::BreakDownSecondsSinceEpoch(time_t seconds_since_epoch,
                                    bool in_utc,
                                    BrokenDownDateTime* result) {
  struct tm tm_result;
  struct tm* error_code;
  if (in_utc) {
    error_code = gmtime_r(&seconds_since_epoch, &tm_result);
  } else {
    // TODO(floitsch): we should be able to call tzset only once during
    // initialization.
    tzset();  // Make sure the libc knows about the local zone.
    error_code = localtime_r(&seconds_since_epoch, &tm_result);
  }
  result->year = tm_result.tm_year;
  result->month= tm_result.tm_mon;
  result->day = tm_result.tm_mday;
  result->hours = tm_result.tm_hour;
  result->minutes = tm_result.tm_min;
  result->seconds = tm_result.tm_sec;
  return error_code != NULL;
}


bool OS::BrokenDownToSecondsSinceEpoch(
    const BrokenDownDateTime& broken_down, bool in_utc, time_t* result) {
  struct tm tm_broken_down;
  // mktime takes the years since 1900.
  tm_broken_down.tm_year = broken_down.year;
  tm_broken_down.tm_mon = broken_down.month;
  tm_broken_down.tm_mday = broken_down.day;
  tm_broken_down.tm_hour = broken_down.hours;
  tm_broken_down.tm_min = broken_down.minutes;
  tm_broken_down.tm_sec = broken_down.seconds;
  // Set wday to an impossible day, so that we can catch bad input.
  tm_broken_down.tm_wday = -1;
  // Make sure the libc knows about the local zone.
  // In case of 'in_utc' this call is mainly for multi-threading issues. If
  // another thread uses a time-function it will set the timezone. The timezone
  // adjustement below would then not work anymore.
  // TODO(floitsch): we should be able to call tzset only once during
  // initialization.
  tzset();
  if (in_utc) {
    // Disable daylight saving in utc mode.
    tm_broken_down.tm_isdst = 0;
    // mktime assumes that the given date is local time zone.
    *result = mktime(&tm_broken_down);
    // Remove the timezone.
    *result -= timezone;
  } else {
    // Let libc figure out if daylight saving is active.
    tm_broken_down.tm_isdst = -1;
    *result = mktime(&tm_broken_down);
  }
  if ((*result == -1) && (tm_broken_down.tm_wday == -1)) {
    return false;
  }
  return true;
}


int64_t OS::GetCurrentTimeMillis() {
  return GetCurrentTimeMicros() / 1000;
}


int64_t OS::GetCurrentTimeMicros() {
  // gettimeofday has microsecond resolution.
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0) {
    UNREACHABLE();
    return 0;
  }
  return (static_cast<int64_t>(tv.tv_sec) * 1000000) + tv.tv_usec;
}


word OS::ActivationFrameAlignment() {
  // OS X activation frames must be 16 byte-aligned; see "Mac OS X ABI
  // Function Call Guide".
  return 16;
}


word OS::PreferredCodeAlignment() {
  return 32;
}


uword OS::GetStackSizeLimit() {
  struct rlimit stack_limit;
  int retval = getrlimit(RLIMIT_STACK, &stack_limit);
  ASSERT(retval == 0);
  if (stack_limit.rlim_cur > INT_MAX) {
    retval = INT_MAX;
  } else {
    retval = stack_limit.rlim_cur;
  }
  return retval;
}


int OS::NumberOfAvailableProcessors() {
  return sysconf(_SC_NPROCESSORS_ONLN);
}


void OS::Sleep(int64_t millis) {
  // TODO(5411554):  For now just use usleep we may have to revisit this.
  usleep(millis * 1000);
}


void OS::DebugBreak() {
#if defined(HOST_ARCH_X64) || defined(HOST_ARCH_IA32)
  asm("int $3");
#else
#error Unsupported architecture.
#endif
}


void OS::Print(const char* format, ...) {
  va_list args;
  va_start(args, format);
  VFPrint(stdout, format, args);
  va_end(args);
}


void OS::VFPrint(FILE* stream, const char* format, va_list args) {
  vfprintf(stream, format, args);
  fflush(stream);
}


int OS::SNPrint(char* str, size_t size, const char* format, ...) {
  va_list args;
  va_start(args, format);
  int retval = VSNPrint(str, size, format, args);
  va_end(args);
  return retval;
}


int OS::VSNPrint(char* str, size_t size, const char* format, va_list args) {
  return vsnprintf(str, size, format, args);
}


void OS::PrintErr(const char* format, ...) {
  va_list args;
  va_start(args, format);
  VFPrint(stderr, format, args);
  va_end(args);
}


void OS::InitOnce() {
  // TODO(5411554): For now we check that initonce is called only once,
  // Once there is more formal mechanism to call InitOnce we can move
  // this check there.
  static bool init_once_called = false;
  ASSERT(init_once_called == false);
  init_once_called = true;
}


void OS::Shutdown() {
}


void OS::Abort() {
  abort();
}


void OS::Exit(int code) {
  exit(code);
}

}  // namespace dart
