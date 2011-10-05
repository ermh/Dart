// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
// Dart test for testing bitwise operations.

class BitOperationsTest {
  static testMain() {
    Expect.equals(3, (3 & 7));
    Expect.equals(7, (3 | 7));
    Expect.equals(4, (3 ^ 7));
    Expect.equals(25, (100 >> 2));
    Expect.equals(400, (100 << 2));
    Expect.equals(-25, (-100 >> 2));
    Expect.equals(-101, ~100);
    Expect.equals(0x10000000000000000, 1 << 64);
    Expect.equals(-0x10000000000000000, -1 << 64);
    Expect.equals(0, ~-1);
    Expect.equals(-1, ~0);

    Expect.equals(0, 1 >> 160);
    Expect.equals(-1, -1 >> 160);

    Expect.equals(0x100000000000000001,
        0x100000000000000001 & 0x100000100F00000001);
    Expect.equals(0x1, 0x1 & 0x100000100F00000001);
    Expect.equals(0x1, 0x100000100F00000001 & 0x1);

    Expect.equals(0x100000100F00000001,
        0x100000000000000001 | 0x100000100F00000001);
    Expect.equals(0x100000100F00000011, 0x11 | 0x100000100F00000001);
    Expect.equals(0x100000100F00000011, 0x100000100F00000001 | 0x11);

    Expect.equals(0x0F000F00000000000000,
        0x0F00F00000000000001 ^ 0xFF00000000000000001);
    Expect.equals(0x31, 0xF00F00000000000001 ^ 0xF00F00000000000030);
    Expect.equals(0xF00F00000000000031, 0xF00F00000000000001 ^ 0x30);
    Expect.equals(0xF00F00000000000031, 0x30 ^ 0xF00F00000000000001);

    Expect.equals(0xF0000000000000000F, 0xF0000000000000000F7 >> 4);
    Expect.equals(15, 0xF00000000 >> 32);
    Expect.equals(1030792151040, 16492674416655 >> 4);

    Expect.equals(0xF0000000000000000F0, 0xF0000000000000000F << 4);
    Expect.equals(0xF00000000, 15 << 32);

    TestNegativeValueShifts();
    TestPositiveValueShifts();
    TestNoMaskingOfShiftCount();
  }

  static void TestNegativeValueShifts() {
    for (int value = 0; value > -100; value--) {
      for (int i = 0; i < 300; i++) {
        int b = (value << i) >> i;
        Expect.equals(value, b);
      }
    }
  }

  static void TestPositiveValueShifts() {
    for (int value = 0; value < 100; value++) {
      for (int i = 0; i < 300; i++) {
        int b = (value << i) >> i;
        Expect.equals(value, b);
      }
    }
  }

  static void TestNoMaskingOfShiftCount() {
    // Shifts which would behave differently if shift count was masked into a
    // range.
    Expect.equals(0, 0 >> 256);
    Expect.equals(0, 1 >> 256);
    Expect.equals(0, 2 >> 256);
    Expect.equals(0, ShiftRight(0, 256));
    Expect.equals(0, ShiftRight(1, 256));
    Expect.equals(0, ShiftRight(2, 256));

    for (int shift = 1; shift <= 256; shift++) {
      Expect.equals(0, ShiftRight(1, shift));
      Expect.equals(-1, ShiftRight(-1, shift));
      Expect.equals(true, ShiftLeft(1, shift) > ShiftLeft(1, shift - 1));
    }
  }

  static int ShiftLeft(int a, int b) { return a << b; }
  static int ShiftRight(int a, int b) { return a >> b; }
}

main() {
  BitOperationsTest.testMain();
}
