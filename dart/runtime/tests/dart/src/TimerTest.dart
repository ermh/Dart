// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

class TimerTest {

  static final int _STARTTIMEOUT = 1050;
  static final int _DECREASE = 200;
  static final int _ITERATIONS = 5;

  static void testSimpleTimer() {

    void timeoutHandler(Timer timer) {
      int endTime = (new Date.now()).value;
      Expect.equals(true, (endTime - _startTime) >= _timeout);
      if (_iteration < _ITERATIONS) {
        _iteration++;
        _timeout = _timeout - _DECREASE;
        _startTime = (new Date.now()).value;
        new Timer(timeoutHandler, _timeout, false);
      }
    }

    _iteration = 0;
    _timeout = _STARTTIMEOUT;
    _startTime = (new Date.now()).value;
     new Timer(timeoutHandler, _timeout, false);
  }

  static void testMain() {
    testSimpleTimer();
  }

  static int _startTime;
  static int _timeout;
  static int _iteration;
}

main() {
  TimerTest.testMain();
}
