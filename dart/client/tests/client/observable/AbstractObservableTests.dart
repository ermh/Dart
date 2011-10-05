// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

class AbstractObservableTests extends ObservableTestSetBase {
  // TODO(rnystrom): Remove this when default constructors are supported.
  AbstractObservableTests() : super();

  setup() {
    addTest(testObservableListeners);
    addTest(testObservableFiresImmediatelyIfNoBatch);

    group('addChangeListener()', () {
      test('adding the same listener twice returns false the second time', () {
        final target = new AbstractObservable();
        final listener = (e) { };

        expect(target.addChangeListener(listener)) == true;
        expect(target.addChangeListener(listener)) == false;
      });
    });
  }

  void testObservableListeners() {
    // check that add/remove works, see contents of listeners too
    final target = new AbstractObservable();
    final l1 = (e) { };
    final l2 = (e) { };
    final l3 = (e) { };
    final l4 = (e) { };

    expect(target.listeners).equalsCollection([]);

    target.addChangeListener(l1);
    expect(target.listeners).equalsCollection([l1]);

    target.addChangeListener(l2);
    expect(target.listeners).equalsCollection([l1, l2]);

    target.addChangeListener(l3);
    target.addChangeListener(l4);
    expect(target.listeners).equalsCollection([l1, l2, l3, l4]);

    target.removeChangeListener(l4);
    expect(target.listeners).equalsCollection([l1, l2, l3]);

    target.removeChangeListener(l2);
    expect(target.listeners).equalsCollection([l1, l3]);

    target.removeChangeListener(l1);
    expect(target.listeners).equalsCollection([l3]);

    target.removeChangeListener(l3);
    expect(target.listeners).equalsCollection([]);
  }

  void testObservableFiresImmediatelyIfNoBatch() {
    // If no batch is created, a summary should be automatically created and
    // fired on each property change.
    final target = new AbstractObservable();
    EventSummary res = null;
    target.addChangeListener((summary) {
      expect(res) == null;
      res = summary;
      expect(res).isNotNull();
    });

    target.recordPropertyUpdate("pM", 10, 11);

    expect(res).isNotNull();
    expect(res.events.length) == 1;
    checkEvent(res.events[0],
        target, "pM", null, ChangeEvent.UPDATE, 10, 11);
    res = null;

    target.recordPropertyUpdate("pL", "11", "13");

    expect(res).isNotNull();
    expect(res.events.length) == 1;
    checkEvent(res.events[0],
        target, "pL", null, ChangeEvent.UPDATE, "11", "13");
  }
}
