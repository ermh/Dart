// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Dart test program testing closures.

class LocalFunction3Test {
  static testExceptions() {
    var f = (int n) { return n + 1; };
    Expect.equals(true, f is Object);
    bool exception_caught = false;
    try {
      f.xyz(0);
    } catch (NoSuchMethodException e) {
      exception_caught = true;
    }
    Expect.equals(true, exception_caught);
    exception_caught = false;
    String f_string;
    try {
      f_string = f.toString();
    } catch (NoSuchMethodException e) {
      exception_caught = true;
    }
    Expect.equals(false, exception_caught);
    Expect.equals("Closure", f_string);
  }

  static testMain() {
    testExceptions();
  }
}

main() {
  LocalFunction3Test.testMain();
}

