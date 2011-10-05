// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// Test of parameterized factory methods.

class Foo<F extends num> {
  Foo();

  // F is not assignable to num.
  factory IFoo<F extends String>.bad() { return null; } /// 00: compile-time error

  factory IFoo<F extends num>.good() { return null; }

  // The bound of F is Object which is assignable to num.
  factory IFoo<F>.ish() { return null; }
}

interface IFoo<X extends num> factory Foo {
}

class FBound<F extends FBound<F>> {}

class Bar extends FBound<Bar> {}

class SubBar extends Bar {}

// String is not assignable to num.
class Baz extends Foo<String> {} /// 01: compile-time error

class Biz extends Foo<int> {}

Foo<int> fi;

// String is not assignable to num.
Foo<String> fs; /// 02: static type error

FBound<SubBar> fb; /// 03: static type error

class Box<T> {

  // Box.T is not assignable to num.
  Foo<T> t; /// 04: static type error

  makeFoo() {
    // Box.T is not assignable to num.
    return new Foo<T>(); /// 05: compile-time error
  }
}

class TypeVariableBoundsTest {
  static testMain() {
    // String is not assignable to num.
    var v1 = new Foo<String>(); /// 06: compile-time error

    // String is not assignable to num.
    Foo<String> v2 = null; /// 07: static type error
  }
}

main() {
  TypeVariableBoundsTest.testMain();
}
