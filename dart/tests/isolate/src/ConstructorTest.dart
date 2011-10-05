// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#library("ConstructorTest");
#import("TestFramework.dart");

class ConstructorTest extends Isolate {
  final int field;
  ConstructorTest() : super(), field = 499;

  void main() {
    this.port.receive((ignoredMessage, reply) {
      reply.send(field, null);
      this.port.close();
    });
  }
}

void test(TestExpectation expect) {
  ConstructorTest test = new ConstructorTest();
  expect.completes(test.spawn()).then((SendPort port) {
    ReceivePort reply = port.call("ignored");
    reply.receive(expect.runs2((message, replyPort) {
      Expect.equals(499, message);
      expect.succeeded();
    }));
  });
}

main() {
  runTests([test]);
}
