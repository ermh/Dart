// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

package com.google.dart.compiler.ast;

import com.google.dart.compiler.resolver.Element;

/**
 * Common supertype for most declarations. A declaration introduces a new name
 * in a scope. Certain tools, such as the IDE, need to know the location of this
 * name, but the name should otherwise be considered a part of the declaration,
 * not an independent node. So the name is not visited when traversing the AST.
 */
public abstract class DartDeclaration<N extends DartExpression> extends DartNode {

  private N name; // Not visited.

  protected DartDeclaration(N name) {
    this.name = becomeParentOf(name);
  }

  public final N getName() {
    return name;
  }

  public final void setName(N newName) {
    name = becomeParentOf(newName);
  }

  @Override
  public abstract Element getSymbol();
}
