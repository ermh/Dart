// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#library("TestFramework");
#import("dart:coreimpl");


typedef void AsynchronousTestFunction(TestExpectation check);


void runTests(List tests) {
  TestRunner runner = new TestRunner(new TestSuite(tests));
  TestResult result = new TestResult(runner);
  runner.run(result);
}


class TestSuite {

  TestSuite([List tests = const []]) : testCases = <TestCase>[] {
    for (var test in tests) {
      addTest(test);
    }
  }

  void addTest(var test) {
    if (test is Function) {
      addAsynchronousTestCase(test);
    } else {
      test.addToTestSuite(this);
    }
  }

  void addTestCase(TestCase test) {
    testCases.add(test);
  }

  void addAsynchronousTestCase(AsynchronousTestFunction test) {
    addTestCase(new AsynchronousTestCase(test));
  }

  void run(TestResult result) {
    for (TestCase test in testCases) {
      test.run(result);
    }
  }

  final List<TestCase> testCases;

}


class TestCase {

  TestCase();

  void setUp() { }
  abstract void performTest();
  void tearDown() { }

  void run(TestResult result) {
    setUp();
    result.runGuarded(this, () {
      performTest();
      tearDown();
    });
  }

  void addToTestSuite(TestSuite suite) {
    suite.addTestCase(this);
  }

}


class TestResult {

  TestResult(this.runner) : errors = [], failures = [];

  void error(String message, TestCase testCase) {
    errors.add([message, testCase]);
  }

  void failure(String message, TestCase testCase) {
    failures.add([message, testCase]);
  }

  runGuarded(TestCase testCase, Function fn) {
    var result = null;
    try {
      result = fn();
    } catch (ExpectException exception) {
      failure(exception.toString(), testCase);
      testCase.tearDown();
    } catch (var exception) {
      error(exception.toString(), testCase);
      testCase.tearDown();
    }
    return result;
  }

  bool hasDefects() {
    return !(errors.isEmpty() && failures.isEmpty());
  }

  final TestRunner runner;
  final List errors;
  final List failures;

}


class TestRunner {

  TestRunner(this.suite);

  void run(TestResult result) {
    if (waitForDoneCallback !== null) {
      waitForDoneCallback();
    }
    suite.run(result);
    if (AsynchronousTestCase.running == 0) {
      done(result);
    }
  }

  void done(TestResult result) {
    if (result.hasDefects()) {
      printDefects(result);
      Expect.fail("Test suite failed.");
    }
    if (doneCallback !== null) {
      doneCallback();
    }
  }

  void printDefects(TestResult result) {
    printDefectList("Errors", result.errors);
    printDefectList("Failures", result.failures);
  }

  static void printDefectList(String type, List defects) {
    if (!defects.isEmpty()) {
      print("$type #${defects.length}:");
      for (List defect in defects) {
        print(" - ${defect[0]}");
      }
    }
  }

  final TestSuite suite;
  static Function waitForDoneCallback;
  static Function doneCallback;

}


class TestExpectation {

  TestExpectation(this.testCase, this.result);

  void succeeded() {
    Expect.equals(0, pendingCallbacks);
    hasSucceeded = true;
    testCase.tearDown();
  }

  void failed() {
    testCase.tearDown();
  }

  Promise completes(Promise promise) {
    Promise result = new TestPromise(this);
    promise.then((value) { result.complete(value); });
    return result;
  }

  Function runs0(Function fn) {
    bool ran = false;  // We only check that the function is executed once.
    pendingCallbacks++;
    return () {
      if (!ran) pendingCallbacks--;
      ran = true;
      return result.runGuarded(testCase, () => fn());
    };
  }

  Function runs1(Function fn) {
    bool ran = false;  // We only check that the function is executed once.
    pendingCallbacks++;
    return (a0) {
      if (!ran) pendingCallbacks--;
      ran = true;
      return result.runGuarded(testCase, () => fn(a0));
    };
  }

  Function runs2(Function fn) {
    bool ran = false;  // We only check that the function is executed once.
    pendingCallbacks++;
    return (a0, a1) {
      if (!ran) pendingCallbacks--;
      ran = true;
      return result.runGuarded(testCase, () => fn(a0, a1));
    };
  }

  bool hasPendingCallbacks() {
    return pendingCallbacks > 0;
  }

  final AsynchronousTestCase testCase;
  final TestResult result;

  int pendingCallbacks = 0;
  bool hasSucceeded = false;

}


class AsynchronousTestCase extends TestCase {

  AsynchronousTestCase(this.test) : super();

  void run(TestResult result) {
    setUp();
    result.runGuarded(this, () {
      addRunning(result);
      TestExpectation expect = new TestExpectation(this, result);
      test(expect);
      if (!expect.hasPendingCallbacks()) {
        Expect.isTrue(expect.hasSucceeded);
        tearDown();
      }
    });
  }

  void tearDown() {
    removeRunning();
  }

  void addRunning(TestResult result) {
    if (running++ == 0) {
      keepalive = new ReceivePort.singleShot();
      keepalive.receive((message, replyTo) {
        result.runner.done(result);
      });
    }
  }

  void removeRunning() {
    if (--running == 0) {
      keepalive.toSendPort().send(null, null);
      keepalive = null;
    }
  }

  AsynchronousTestFunction test;

  static int running = 0;
  static ReceivePort keepalive = null;

}


class TestPromise<T> extends PromiseImpl<T> {

  TestPromise(this.expect) : super();

  void addCompleteHandler(void completeHandler(T result)) {
    super.addCompleteHandler(expect.runs1((T result) {
      completeHandler(result);
    }));
  }

  final TestExpectation expect;

}
