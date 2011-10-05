// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Dart test program for testing try/catch statement without any exceptions
// being thrown. (Nested try/catch blocks).

interface TestException {
  String getMessage();
}

class MyException implements TestException {
  const MyException([String message = ""]) : message_ = message;
  String getMessage() { return message_; }
  final String message_;
}

class StackTrace {
  StackTrace() { }
}

class Helper {
  static int f1(int i) {
    try {
      int j;
      j = f2();
      i = i + 1;
      try {
        j = f2() + f3() + j;
        i = i + 1;
      } catch (TestException e, StackTrace trace) {
        j = 50;
      }
      j = f3() + j;
    } catch (MyException exception) {
      i = 100;
    } catch (TestException e, StackTrace trace) {
      i = 200;
    }
    return i;
  }

  static int f2() {
    return 2;
  }

  static int f3() {
    int i = 0;
    while (i < 10) {
      i++;
    }
    return i;
  }
}

class TryCatch2Test {
  static testMain() {
    Expect.equals(3, Helper.f1(1));
  }
}

main() {
  TryCatch2Test.testMain();
}
