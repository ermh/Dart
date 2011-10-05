// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// Test that type variables aren't in scope of static methods and factories.

class Foo<T> {
  // T is not in scope for a static method.
  static
  Foo<T> /// 00: compile-time error
  m(
    Foo<T> /// 01: compile-time error
    f) {
    I<T> x; /// 02: compile-time error
  }

  // T is not in scope for a static method.
  factory I(
            I<T> /// 03: compile-time error
            i) {
    I<T> x; /// 04: compile-time error
  }

  // T is not in scope for a static field.
  static Foo<T> f1; /// 05: compile-time error

  static
  Foo<T> /// 06: compile-time error
  get f() { return null; }

  static void set f(
                    Foo<T> /// 07: compile-time error
		    value) {}
}

interface I<X> factory Foo<X> {
  I(I<X> i);
}

main() {
  Foo.m(null);
}
