// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

package com.google.dart.compiler.resolver;

import com.google.dart.compiler.common.Symbol;

/**
 * Kinds of elements. Use kinds instead of instanceof for maximum flexibility
 * and sharing of similar implementation classes.
 */
public enum ElementKind {
  CLASS,
  CONSTRUCTOR,
  FIELD,
  FUNCTION_OBJECT,
  LABEL,
  METHOD,
  PARAMETER,
  TYPE_VARIABLE,
  VARIABLE,
  FUNCTION_TYPE_ALIAS,
  DYNAMIC,
  LIBRARY,
  SUPER,
  NONE,
  VOID;

  public static ElementKind of(Symbol symbol) {
    if (symbol instanceof Element) {
      Element element = (Element) symbol;
      return element.getKind();
    } else {
      return NONE;
    }
  }
}
