// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Redirection constructors must not initialize any fields.

class A {
  var x;
  A(this.x) {}
  A.named() : this(3), x = 5 {}
}

class ConstructorRedirect3NegativeTest {
  static testMain() {
  }
}

main() {
  ConstructorRedirect3NegativeTest.testMain();
}
