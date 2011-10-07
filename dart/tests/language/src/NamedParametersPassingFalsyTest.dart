// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// Dart test program for testing named parameters with various values that might
// be implemented as 'falsy' values in a JavaScript implementation.


class TestClass {
  TestClass();

  num method([value = 100]) => value;

  static num staticMethod([value = 200]) => value;
}

num globalMethod([value = 300]) => value;

final testValues = const [0, 0.0, '', false, null];

testFunction(f) {
  Expect.isTrue(f() >= 100);
  for (var v in testValues) {
    Expect.equals(v, f(v));
    Expect.equals(v, f(value: v));
  }
}

main() {
  var obj = new TestClass();

  Expect.equals(100, obj.method());
  Expect.equals(200, TestClass.staticMethod());
  Expect.equals(300, globalMethod());

  for (var v in testValues) {
    Expect.equals(v, obj.method(v));
    Expect.equals(v, obj.method(value: v));
    Expect.equals(v, TestClass.staticMethod(v));
    Expect.equals(v, TestClass.staticMethod(value: v));
    Expect.equals(v, globalMethod(v));
    Expect.equals(v, globalMethod(value: v));
  }

  // Test via indirect call.
  testFunction(obj.method);
  testFunction(TestClass.staticMethod);
  testFunction(globalMethod);
}
