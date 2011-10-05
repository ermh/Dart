// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
//

// Type parameters can shadow a library prefix.

#import("library10.dart", prefix:"T");
class P<T> {
  P.named(T this.fld);
  T fld;
  main() {
    var i = new T.Library10(10);  // This should be an error.
    Expect.equals(10, i.fld);
  }
}

main() {
  var i = new P<int>.named(10);
  i.main();
}