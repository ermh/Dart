// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// All things regarding compile time constant expressions.

interface Roman {
  static final I = 1;
  static final II = 2;
  static final III = 3;
  static final IV = 4;
  static final V = 5;

  static final VivaItalia = const {"green": 1, "red": 3, "white": 2};
}


class Point {
  static final int zero = 0;

  static final origin = const Point(0, 0);
  static final origin2 = const Point(zero, Roman.IV - 4);

  const Point(x, y) : x_= x, y_ = y;
  const Point.X(x) : x_ = x, y_ = Roman.V - Roman.II - 3;

  bool operator ==(final Point other) {
    return (this.x_ == other.x_) && (this.y_ == other.y_);
  }

  final int x_, y_;
}


class Line {
  const Line(Point begin, Point end) : beg_ = begin, end_ = end;
  final Point beg_;
  final Point end_;
}


class CTConstTest {
  static int getZero() { return 0; }
  static final naught = null;

  static testMain() {
    Expect.equals(0, Point.zero);
    Expect.equals(0, Point.origin.x_);
    Expect.equals(true, Point.origin === Point.origin2);
    var p1 = const Point(0, 0);
    Expect.equals(true, Point.origin === p1);

    Expect.equals(false, Point.origin == const Point(1, 1));
    Expect.equals(false, Point.origin === const Point(1, 1));

    var p2 = new Point(0, getZero());
    Expect.equals(true, Point.origin == p2);  // Point.operator==

    Expect.equals(true, const Point.X(5) === const Point(5, 0));

    Line l1 = const Line(Point.origin, const Point(1, 1));
    Line l2 = const Line(const Point(0, 0), const Point(1, 1));
    Line l3 = new Line(const Point(0, 0), const Point(1, 1));
    Expect.equals(true, l1 === l2);

    final evenNumbers = const <int>[2, 2*2, 2*3, 2*4, 2*5];
    Expect.equals(true, evenNumbers === const [2, 4, 6, 8, 10]);

    final c11dGermany1 = const {"black": 1, "red": 2, "yellow": 3};
    Expect.equals(true,
        c11dGermany1 === const {"black": 1, "red": 2, "yellow": 3});

    final c11dGermany2 = const {"black": 1, "red": 2, "yellow": 3};
    Expect.equals(true, c11dGermany1 === c11dGermany2);

    final c11dBelgium = const {"black": 1, "yellow": 2, "red": 3};
    Expect.equals(false, c11dGermany1 == c11dBelgium);
    Expect.equals(false, c11dGermany1 === c11dBelgium);

    final c11dItaly = const {"green": 1, "red": 3, "white": 2};
    Expect.equals(true,
        c11dItaly === const {"green": 1, "red": 3, "white": 2});
    Expect.equals(true, c11dItaly === Roman.VivaItalia);

    Expect.equals(3, c11dItaly.length);
    Expect.equals(3, c11dItaly.getKeys().length);
    Expect.equals(true, c11dItaly.containsKey("white"));
    Expect.equals(false, c11dItaly.containsKey("black"));

    // Make sure the map object is immutable.
    bool caughtException = false;
    try {
      c11dItaly["green"] = 0;
    } catch (IllegalAccessException e) {
      caughtException = true;
    }
    Expect.equals(true, caughtException);
    Expect.equals(1, c11dItaly["green"]);

    caughtException = false;
    try {
      c11dItaly.clear();
    } catch (IllegalAccessException e) {
      caughtException = true;
    }
    Expect.equals(true, caughtException);
    Expect.equals(1, c11dItaly["green"]);

    caughtException = false;
    try {
      c11dItaly.remove("orange");
    } catch (IllegalAccessException e) {
      caughtException = true;
    }
    Expect.equals(true, caughtException);
    Expect.equals(1, c11dItaly["green"]);

    Expect.equals(true, null === naught);
  }
}

main() {
  CTConstTest.testMain();
}
