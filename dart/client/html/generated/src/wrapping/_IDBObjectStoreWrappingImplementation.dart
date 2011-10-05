// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// WARNING: Do not edit - generated code.

class IDBObjectStoreWrappingImplementation extends DOMWrapperBase implements IDBObjectStore {
  IDBObjectStoreWrappingImplementation._wrap(ptr) : super._wrap(ptr) {}

  String get keyPath() { return _ptr.keyPath; }

  String get name() { return _ptr.name; }

  IDBRequest add(String value, IDBKey key) {
    return LevelDom.wrapIDBRequest(_ptr.add(value, LevelDom.unwrap(key)));
  }

  IDBRequest clear() {
    return LevelDom.wrapIDBRequest(_ptr.clear());
  }

  IDBIndex createIndex(String name, String keyPath) {
    return LevelDom.wrapIDBIndex(_ptr.createIndex(name, keyPath));
  }

  IDBRequest delete(IDBKey key) {
    return LevelDom.wrapIDBRequest(_ptr.delete(LevelDom.unwrap(key)));
  }

  void deleteIndex(String name) {
    _ptr.deleteIndex(name);
    return;
  }

  IDBIndex index(String name) {
    return LevelDom.wrapIDBIndex(_ptr.index(name));
  }

  IDBRequest openCursor(IDBKeyRange range, int direction) {
    return LevelDom.wrapIDBRequest(_ptr.openCursor(LevelDom.unwrap(range), direction));
  }

  IDBRequest put(String value, IDBKey key) {
    return LevelDom.wrapIDBRequest(_ptr.put(value, LevelDom.unwrap(key)));
  }

  String get typeName() { return "IDBObjectStore"; }
}