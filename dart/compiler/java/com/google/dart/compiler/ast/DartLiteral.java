// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

package com.google.dart.compiler.ast;

import com.google.dart.compiler.type.Type;

/**
 * Abstract base class for Dart literal values.
 */
public abstract class DartLiteral extends DartExpression {
  private Type type;

  @Override
  public void setType(Type type) {
    this.type = type;
  }

  @Override
  public Type getType() {
    return type;
  }
}
