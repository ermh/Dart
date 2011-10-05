// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

package com.google.dart.compiler.type;

import com.google.dart.compiler.DartCompilationError;
import com.google.dart.compiler.DartCompilerListener;
import com.google.dart.compiler.ErrorCode;
import com.google.dart.compiler.resolver.ClassElement;
import com.google.dart.compiler.resolver.Elements;
import com.google.dart.compiler.resolver.TypeVariableElement;
import com.google.dart.compiler.testing.TestCompilerContext;

import junit.framework.TestCase;

import org.junit.Assert;

import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Common superclass for type tests.
 */
abstract class TypeTestCase extends TestCase {

  final Map<String, ClassElement> coreElements = new HashMap<String, ClassElement>();
  final ClassElement object = element("Object", null);
  final ClassElement function = element("Function", itype(object));
  final ClassElement number = element("num", itype(object));
  final ClassElement intElement = element("int", itype(number));
  final ClassElement doubleElement = element("double", itype(number));
  final ClassElement bool = element("bool", itype(object));
  final ClassElement string = element("String", itype(object));
  final ClassElement array = element("Array", itype(object), typeVar("E", itype(object)));
  final ClassElement growableArray = makeGrowableArray(array);
  final ClassElement map = element("Map", itype(object),
                                   typeVar("K", itype(object)), typeVar("V", itype(object)));
  final ClassElement stackTrace = element("StackTrace", itype(object));
  final ClassElement reverseMap = makeReverseMap(map);
  final InterfaceType objectArray = itype(array, itype(object));
  final InterfaceType growableObjectArray = itype(growableArray, itype(object));
  final InterfaceType objectMap = itype(map, itype(object), itype(object));
  final InterfaceType reverseObjectMap = itype(reverseMap, itype(object), itype(object));
  final InterfaceType stringIntMap = itype(map, itype(string), itype(intElement));
  final InterfaceType intStringMap = itype(map, itype(intElement), itype(string));
  final InterfaceType stringIntReverseMap = itype(reverseMap, itype(string), itype(intElement));
  final FunctionType returnObject = ftype(function, itype(object), null, null);
  final FunctionType returnString = ftype(function, itype(string), null, null);
  final FunctionType objectToObject = ftype(function, itype(object), null, null, itype(object));
  final FunctionType objectToString = ftype(function, itype(string), null, null, itype(object));
  final FunctionType stringToObject = ftype(function, itype(object), null, null, itype(string));
  final FunctionType stringAndIntToBool = ftype(function, itype(bool),
                                                null, null, itype(string), itype(intElement));
  final FunctionType stringAndIntToMap = ftype(function, stringIntMap,
                                               null, null, itype(string), itype(intElement));
  private int expectedTypeErrors = 0;

  abstract Types getTypes();

  protected void setExpectedTypeErrorCount(int count) {
    checkExpectedTypeErrorCount();
    expectedTypeErrors = count;
  }

  protected void checkExpectedTypeErrorCount(String message) {
    assertEquals(message, 0, expectedTypeErrors);
  }

  protected void checkExpectedTypeErrorCount() {
    checkExpectedTypeErrorCount(null);
  }

  static TypeVariable typeVar(String name, Type bound) {
    TypeVariableElement element = Elements.typeVariableElement(null, name, null);
    element.setBound(bound);
    return new TypeVariableImplementation(element);
  }

  private ClassElement makeGrowableArray(ClassElement array) {
    TypeVariable E = typeVar("E", itype(object));
    return element("GrowableArray", itype(array, E), E);
  }

  private ClassElement makeReverseMap(ClassElement map) {
    TypeVariable K = typeVar("K", itype(object));
    TypeVariable V = typeVar("V", itype(object));
    return element("ReverseMap", itype(map, V, K), K, V);
  }

  static InterfaceType itype(ClassElement element, Type... arguments) {
    return new InterfaceTypeImplementation(element, Arrays.asList(arguments));
  }

  static FunctionType ftype(ClassElement element, Type returnType,
                            Map<String, Type> namedParameterTypes, Type rest, Type... arguments) {
    return FunctionTypeImplementation.of(element, Arrays.asList(arguments), namedParameterTypes,
                                         rest, returnType, null);
  }

  static Map<String, Type> named(Object... pairs) {
    Map<String, Type> named = new LinkedHashMap<String, Type>();
    for (int i = 0; i < pairs.length; i++) {
      Type type = (Type) pairs[i++];
      String name = (String) pairs[i];
      named.put(name, type);
    }
    return named;
  }

  ClassElement element(String name, InterfaceType supertype, TypeVariable... parameters) {
    ClassElement element = Elements.classNamed(name);
    element.setSupertype(supertype);
    element.setType(itype(element, parameters));
    coreElements.put(name, element);
    return element;
  }

  void checkSubtype(Type t, Type s) {
    Assert.assertTrue(getTypes().isSubtype(t, s));
  }

  void checkStrictSubtype(Type t, Type s) {
    checkSubtype(t, s);
    checkNotSubtype(s, t);
  }

  void checkNotSubtype(Type t, Type s) {
    Assert.assertFalse(getTypes().isSubtype(t, s));
  }

  void checkNotAssignable(Type t, Type s) {
    checkNotSubtype(t, s);
    checkNotSubtype(s, t);
  }

  final DartCompilerListener listener = new DartCompilerListener() {
    @Override
    public void compilationError(DartCompilationError event) {
      throw new AssertionError(event);
    }

    @Override
    public void compilationWarning(DartCompilationError event) {
      compilationError(event);
    }

    @Override
    public void typeError(DartCompilationError event) {
      compilationError(event);
    }
  };

  final TestCompilerContext context = new TestCompilerContext() {
    @Override
    public void typeError(DartCompilationError event) {
      getErrorCodes().add(event.getErrorCode());
      expectedTypeErrors--;
      if (expectedTypeErrors < 0) {
        throw new TestTypeError(event);
      }
    }
  };

  static class TestTypeError extends RuntimeException {
    final DartCompilationError event;

    TestTypeError(DartCompilationError event) {
      super(String.valueOf(event));
      this.event = event;
    }

    ErrorCode getErrorCode() {
      return event.getErrorCode();
    }
  }
}
