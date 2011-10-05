// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

class ListTest {

  static testMain() {
    testList();
    testExpandableList();
  }

  static void expectValues(list, val1, val2, val3, val4) {
    Expect.equals(true, list.length == 4);
    Expect.equals(true, list.length == 4);
    Expect.equals(true, !list.isEmpty());
    Expect.equals(list[0], val1);
    Expect.equals(list[1], val2);
    Expect.equals(list[2], val3);
    Expect.equals(list[3], val4);
  }

  static void testClosures(List list) {
    testFilter(val) { return val == 3; }
    Collection filtered = list.filter(testFilter);
    Expect.equals(filtered.length, 1);

    testEvery(val) { return val != 11; }
    bool test = list.every(testEvery);
    Expect.equals(true, test);

    testSome(val) { return val == 1; }
    test = list.some(testSome);
    Expect.equals(true, test);

    testSomeFirst(val) { return val == 0; }
    test = list.some(testSomeFirst);
    Expect.equals(true, test);

    testSomeLast(val) { return val == (list.length - 1); }
    test = list.some(testSomeLast);
    Expect.equals(true, test);
  }

  static void testList() {
    List list = new List(4);
    Expect.equals(list.length, 4);
    list[0] = 4;
    expectValues(list, 4, null, null, null);
    String val = "fisk";
    list[1] = val;
    expectValues(list, 4, val, null, null);
    double d = 2.0;
    list[3] = d;
    expectValues(list, 4, val, null, d);

    for (int i = 0; i < list.length; i++) {
      list[i] = i;
    }

    for (int i = 0; i < 4; i++) {
      Expect.equals(list[i], i);
    }

    testClosures(list);

    var exception = null;
    try {
      list.clear();
    } catch (UnsupportedOperationException e) {
      exception = e;
    }
    Expect.equals(true, exception != null);
  }

  static void testExpandableList() {
    List list = new List();
    Expect.equals(true, list.isEmpty());
    Expect.equals(list.length, 0);
    list.add(4);
    Expect.equals(1, list.length);
    Expect.equals(true, !list.isEmpty());
    Expect.equals(list.length, 1);
    Expect.equals(list.length, 1);
    Expect.equals(list.removeLast(), 4);

    for (int i = 0; i < 10; i++) {
      list.add(i);
    }

    Expect.equals(list.length, 10);
    for (int i = 0; i < 10; i++) {
      Expect.equals(list[i], i);
    }

    testClosures(list);

    Expect.equals(list.removeLast(), 9);
    list.clear();
    Expect.equals(list.length, 0);
    Expect.equals(list.length, 0);
    Expect.equals(true, list.isEmpty());
  }
}

main() {
  ListTest.testMain();
}
