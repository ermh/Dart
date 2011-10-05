// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Dart test program testing string interpolation.


class WhatchamaCallIt {
  WhatchamaCallIt() { }

  void foo() {
    return "Hansel and $name";  // Field name is defined in subclass.
  }
}

class ThingamaBob extends WhatchamaCallIt {
  ThingamaBob(String s) : super(), name = s { }
  String name;
}

class StringInterpolateTest {

  static final String A = "svin";
  static final String B = "hest";
  static final int N = 1 + 1;
  static final String Printers = "Printers: $A and $B";
  static final String AAR_Printers = "AAR has $N $Printers.";

  static testMain() {
    var x = 1;
    var s = "eins und \$x macht zwei.";
    print(s);
    Expect.equals(@"eins und $x macht zwei.", s);

    s = "eins und $x macht zwei.";
    print(s);
    Expect.equals(@"eins und 1 macht zwei.", s);

    print(AAR_Printers);
    Expect.equals(@"AAR has 2 Printers: svin and hest.", AAR_Printers);

    var s$eins = "eins";
    var $1 = 1;
    var zw = "zw";
    var ei = "ei";
    var zw$ei = "\"Martini, dry? Nai zwai.\"";
    s = "${s$eins} und ${$1} macht $zw$ei.";
    print(s);
    Expect.equals(@"eins und 1 macht zwei.", s);

    var t = new ThingamaBob("Gretel");
    print(t.foo());
    Expect.equals(t.foo(), "Hansel and Gretel");
  }
}

main() {
  StringInterpolateTest.testMain();
}
