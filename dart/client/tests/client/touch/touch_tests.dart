// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#library('touch_tests');

#import('dart:html'); // TODO(rnystrom): Only needed to tell architecture.py
                      // that this is a web test. Come up with cleaner solution.
#import('../../../testing/unittest/unittest.dart');
#import('../../../touch/touch.dart');

main() {
  test('Solver', () {
    expect(Solver.solve((x) => x * x, 81, 10)).approxEquals(9);
    expect(Solver.solve((x) => x * x, 0, 10)).approxEquals(0);
    expect(Solver.solve((x) => x * x, 1.5625, 10)).approxEquals(1.25);
    expect(Solver.solve((x) => 1 / x, 10, 1)).approxEquals(0.1);
  });

  group('Momentum', () {
    test('SingleDimensionPhysics', () {
      expect(new SingleDimensionPhysics().solve(0, 0, 1)).equals(0);
      expect(new SingleDimensionPhysics().solve(0, 5, 1)).equals(0);
      expect(new SingleDimensionPhysics().solve(0, 100, 1)).equals(0);
      expect(new SingleDimensionPhysics().solve(0, 100, 0.5)).equals(0);
    });

    test('TimeoutMomentum()', () {
      final delegate = new TestMomentumDelegate();
      final momentum = new TimeoutMomentum(null);
    });
  });
}

class TestMomentumDelegate {
  Function onDecelerateCallback;
  Function onDecelerationEndCallback;

  void onDecelerate(num x, num y,
                    [num duration = 0, String timingFunction = null]) {
     onDecelerateCallback(x, y, duration, timingFunction);
  }

  /**
   * Callback for end of deceleration.
   */
  void onDecelerationEnd() {
    onDecelerationEndCallback();
  }
}