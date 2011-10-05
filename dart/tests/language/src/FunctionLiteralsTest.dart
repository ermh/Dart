// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

/**
 * Test various forms of function literals.
 */
typedef int IntFunc(int);

class FunctionLiteralsTest {
  static void checkIntFunction(expected, int f(x), arg) {
    Expect.equals(expected, f(arg));
  }

  static void checkIntFuncFunction(expected, IntFunc f(x), arg) {
    Expect.equals(expected, f(arg)(arg));
  }

  int func1(int x) => x;

  int func2(x) => x;

  int func3(int x) {
    return x;
  }

  int func4(x) {
    return x;
  }

  FunctionLiteralsTest() {}

  static void testMain() {
    var test = new FunctionLiteralsTest();
    test.testArrow();
    test.testArrowArrow();
    test.testArrowBlock();
    test.testBlock();
    test.testBlockArrow();
    test.testBlockBlock();
    test.testFunctionRef();
  }

  void testArrow() {
    checkIntFunction(42, (x) => x, 42);
    checkIntFunction(42, _(x) => x, 42);
    checkIntFunction(42, int f(x) => x, 42);
    checkIntFunction(42, (int x) => x, 42);
    checkIntFunction(42, _(int x) => x, 42);
    checkIntFunction(42, int f(int x) => x, 42);
  }

  void testArrowArrow() {
    checkIntFuncFunction(84, (x) => (y) => x+y, 42);
    checkIntFuncFunction(84, _(x) => (y) => x+y, 42);
    checkIntFuncFunction(84, IntFunc f(x) => (y) => x+y, 42);
    checkIntFuncFunction(84, (int x) => (y) => x+y, 42);
    checkIntFuncFunction(84, _(int x) => (y) => x+y, 42);
    checkIntFuncFunction(84, IntFunc f(int x) => (y) => x+y, 42);
    checkIntFuncFunction(84, (x) => f(y) => x+y, 42);
    checkIntFuncFunction(84, _(x) => f(y) => x+y, 42);
    checkIntFuncFunction(84, IntFunc f(x) => f(y) => x+y, 42);
    checkIntFuncFunction(84, (int x) => f(y) => x+y, 42);
    checkIntFuncFunction(84, _(int x) => f(y) => x+y, 42);
    checkIntFuncFunction(84, IntFunc f(int x) => f(y) => x+y, 42);
    checkIntFuncFunction(84, (x) => int f(y) => x+y, 42);
    checkIntFuncFunction(84, _(x) => int f(y) => x+y, 42);
    checkIntFuncFunction(84, IntFunc f(x) => int f(y) => x+y, 42);
    checkIntFuncFunction(84, (int x) => int f(y) => x+y, 42);
    checkIntFuncFunction(84, _(int x) => int f(y) => x+y, 42);
    checkIntFuncFunction(84, IntFunc f(int x) => int f(y) => x+y, 42);
    checkIntFuncFunction(84, (int x) => int f(int y) => x+y, 42);
    checkIntFuncFunction(84, _(int x) => int f(int y) => x+y, 42);
    checkIntFuncFunction(84, IntFunc f(int x) => int f(int y) => x+y, 42);
  }

  void testArrowBlock() {
    checkIntFuncFunction(84, (x) => (y) { return x+y; }, 42);
    checkIntFuncFunction(84, _(x) => (y) { return x+y; }, 42);
    checkIntFuncFunction(84, IntFunc f(x) => (y) { return x+y; }, 42);
    checkIntFuncFunction(84, (int x) => (y) { return x+y; }, 42);
    checkIntFuncFunction(84, _(int x) => (y) { return x+y; }, 42);
    checkIntFuncFunction(84, IntFunc f(int x) => (y) { return x+y; }, 42);
    checkIntFuncFunction(84, (x) => f(y) { return x+y; }, 42);
    checkIntFuncFunction(84, _(x) => f(y) { return x+y; }, 42);
    checkIntFuncFunction(84, IntFunc f(x) => f(y) { return x+y; }, 42);
    checkIntFuncFunction(84, (int x) => f(y) { return x+y; }, 42);
    checkIntFuncFunction(84, _(int x) => f(y) { return x+y; }, 42);
    checkIntFuncFunction(84, IntFunc f(int x) => f(y) { return x+y; }, 42);
    checkIntFuncFunction(84, (x) => int f(y) { return x+y; }, 42);
    checkIntFuncFunction(84, _(x) => int f(y) { return x+y; }, 42);
    checkIntFuncFunction(84, IntFunc f(x) => int f(y) { return x+y; }, 42);
    checkIntFuncFunction(84, (int x) => int f(y) { return x+y; }, 42);
    checkIntFuncFunction(84, _(int x) => int f(y) { return x+y; }, 42);
    checkIntFuncFunction(84, IntFunc f(int x) => int f(y) { return x+y; }, 42);
  }

  void testBlock() {
    checkIntFunction(42, (x) { return x; }, 42);
    checkIntFunction(42, _(x) { return x; }, 42);
    checkIntFunction(42, int f(x) { return x; }, 42);
    checkIntFunction(42, (int x) { return x; }, 42);
    checkIntFunction(42, _(int x) { return x; }, 42);
    checkIntFunction(42, int f(int x) { return x; }, 42);
  }

  void testBlockArrow() {
    checkIntFuncFunction(84, (x) { return (y) => x+y; }, 42);
    checkIntFuncFunction(84, _(x) { return (y) => x+y; }, 42);
    checkIntFuncFunction(84, IntFunc f(x) { return (y) => x+y; }, 42);
    checkIntFuncFunction(84, (int x) { return (y) => x+y; }, 42);
    checkIntFuncFunction(84, _(int x) { return (y) => x+y; }, 42);
    checkIntFuncFunction(84, IntFunc f(int x) { return (y) => x+y; }, 42);
    checkIntFuncFunction(84, (x) { return f(y) => x+y; }, 42);
    checkIntFuncFunction(84, _(x) { return f(y) => x+y; }, 42);
    checkIntFuncFunction(84, IntFunc f(x) { return f(y) => x+y; }, 42);
    checkIntFuncFunction(84, (int x) { return f(y) => x+y; }, 42);
    checkIntFuncFunction(84, _(int x) { return f(y) => x+y; }, 42);
    checkIntFuncFunction(84, IntFunc f(int x) { return f(y) => x+y; }, 42);
    checkIntFuncFunction(84, (x) { return int f(y) => x+y; }, 42);
    checkIntFuncFunction(84, _(x) { return int f(y) => x+y; }, 42);
    checkIntFuncFunction(84, IntFunc f(x) { return int f(y) => x+y; }, 42);
    checkIntFuncFunction(84, (int x) { return int f(y) => x+y; }, 42);
    checkIntFuncFunction(84, _(int x) { return int f(y) => x+y; }, 42);
    checkIntFuncFunction(84, IntFunc f(int x) { return int f(y) => x+y; }, 42);
  }

  void testBlockBlock() {
    checkIntFuncFunction(84, (x) { return (y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, _(x) { return (y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, IntFunc f(x) { return (y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, (int x) { return (y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, _(int x) { return (y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, IntFunc f(int x) { return (y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, (x) { return f(y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, _(x) { return f(y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, IntFunc f(x) { return f(y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, (int x) { return f(y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, _(int x) { return f(y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, IntFunc f(int x) { return f(y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, (x) { return int f(y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, _(x) { return int f(y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, IntFunc f(x) { return int f(y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, (int x) { return int f(y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, _(int x) { return int f(y) { return x+y; }; }, 42);
    checkIntFuncFunction(84, IntFunc f(int x) { return int f(y) { return x+y; }; }, 42);
  }

  void testFunctionRef() {
    checkIntFunction(42, func1, 42);
    checkIntFunction(42, func2, 42);
    checkIntFunction(42, func3, 42);
    checkIntFunction(42, func4, 42);
  }
}


main() {
  FunctionLiteralsTest.testMain();
}
