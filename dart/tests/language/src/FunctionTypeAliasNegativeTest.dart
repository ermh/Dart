// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Dart test for illegally self referencing function type alias.

typedef Handle Handle(String command);

class FunctionTypeAliasNegativeTest {
  static void testMain() {
  }
}


main() {
  FunctionTypeAliasNegativeTest.testMain();
}
