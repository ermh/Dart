// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Dart program testing stack overflow.

class StackOverflowTest {

  static void curseTheRecurse(a, b, c) {
    curseTheRecurse(b, c, a);
  }

  static void testMain() {
    bool exceptionCaught = false;
    try {
      curseTheRecurse(1, 2, 3);
    } catch (StackOverflowException e) {
      exceptionCaught = true;
    }
    Expect.equals(true, exceptionCaught);
  }
}

main() {
  StackOverflowTest.testMain();
}
