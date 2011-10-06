// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

package com.google.dart.compiler.resolver;

import com.google.dart.compiler.CompilerTestCase;
import com.google.dart.compiler.DartCompilationError;
import com.google.dart.compiler.ast.DartUnit;
import com.google.dart.compiler.testing.TestCompilerContext;

import java.util.ArrayList;
import java.util.List;

// TODO(ngeoffray): Move these tests to the VM tests once we can run VM tests.
public class NegativeResolverTest extends CompilerTestCase {
  List<DartCompilationError> errors = new ArrayList<DartCompilationError>();
  List<DartCompilationError> typeErrors = new ArrayList<DartCompilationError>();

  public void checkNumErrors(String fileName, int expectedErrorCount) {
    DartUnit unit = parseUnit(fileName);
    unit.addTopLevelNode(ResolverTestCase.makeClass("Object", null));
    unit.addTopLevelNode(ResolverTestCase.makeClass("Function", null));
    ResolverTestCase.resolve(unit, getContext());
    assertEquals(new ArrayList<DartCompilationError>(), typeErrors);
    if (errors.size() != expectedErrorCount) {
      fail(String.format("Expected %s errors, but got %s: %s",
                         expectedErrorCount, errors.size(), errors));
    }
  }

  public void testInitializer1() {
    checkNumErrors("Initializer1NegativeTest.dart", 1);
  }

  public void testInitializer2() {
    checkNumErrors("Initializer2NegativeTest.dart", 1);
  }

  public void testInitializer3() {
    checkNumErrors("Initializer3NegativeTest.dart", 1);
  }

  public void testInitializer4() {
    checkNumErrors("Initializer4NegativeTest.dart", 1);
  }

  public void testInitializer5() {
    checkNumErrors("Initializer5NegativeTest.dart", 1);
  }

  public void testInitializer6() {
    checkNumErrors("Initializer6NegativeTest.dart", 1);
  }

  public void testCall1() {
    checkNumErrors("StaticInstanceCallNegativeTest.dart", 1);
  }

  public void testClassExtendsInterfaceNegativeTest() {
    checkNumErrors("ClassExtendsInterfaceNegativeTest.dart", 1);
  }

  public void tesClassImplementsUnknownInterfaceNegativeTest() {
    checkNumErrors("ClassImplementsUnknownInterfaceNegativeTest.dart", 1);
  }

  public void testConstSuperNegativeTest1() {
    checkNumErrors("ConstSuperNegativeTest1.dart", 0);
  }

  public void testConstSuperNegativeTest2() {
    checkNumErrors("ConstSuperNegativeTest2.dart", 1);
  }

  public void testConstSuperTest() {
    checkNumErrors("ConstSuperTest.dart", 0);
  }

  public void testParameterInitializerNegativeTest1() {
    checkNumErrors("ParameterInitializerNegativeTest1.dart", 1);
  }

  public void testParameterInitializerNegativeTest2() {
    checkNumErrors("ParameterInitializerNegativeTest2.dart", 1);
  }

  public void testParameterInitializerNegativeTest3() {
    checkNumErrors("ParameterInitializerNegativeTest3.dart", 1);
  }

  public void testStaticToInstanceInvocationNegativeTest1() {
    checkNumErrors("StaticToInstanceInvocationNegativeTest1.dart", 1);
  }

  public void testStaticToInstanceInvocationNegativeTest2() {
    checkNumErrors("StaticToInstanceInvocationNegativeTest2.dart", 1);
  }

  public void testConstVariableInitializationNegativeTest1() {
    checkNumErrors("ConstVariableInitializationNegativeTest1.dart", 1);
  }

  public void testConstVariableInitializationNegativeTest2() {
    checkNumErrors("ConstVariableInitializationNegativeTest2.dart", 1);
  }

  public void testNameShadowNegativeTest1() {
    checkNumErrors("NameShadowNegativeTest1.dart", 1);
  }

  public void testNameShadowNegativeTest2() {
    checkNumErrors("NameShadowNegativeTest2.dart", 1);
  }

  public void testNameShadowNegativeTest4() {
    checkNumErrors("NameShadowNegativeTest4.dart", 1);
  }

  public void testNameShadowNegativeTest5() {
    checkNumErrors("NameShadowNegativeTest5.dart", 1);
  }

  public void testNameShadowNegativeTest6() {
    checkNumErrors("NameShadowNegativeTest6.dart", 1);
  }

  public void testNameShadowNegativeTest7() {
    checkNumErrors("NameShadowNegativeTest7.dart", 1);
  }

  public void testNameShadowNegativeTest8() {
    checkNumErrors("NameShadowNegativeTest8.dart", 1);
  }

  public void testNameShadowNegativeTest9() {
    checkNumErrors("NameShadowNegativeTest9.dart", 1);
  }

  public void testNameShadowNegativeTest10() {
    checkNumErrors("NameShadowNegativeTest10.dart", 1);
  }

  public void testNameShadowNegativeTest11() {
    checkNumErrors("NameShadowNegativeTest11.dart", 1);
  }

  public void testUnresolvedSuperFieldNegativeTest() {
    checkNumErrors("UnresolvedSuperFieldNegativeTest.dart", 1);
  }

  public void testStaticSuperFieldNegativeTest() {
    checkNumErrors("StaticSuperFieldNegativeTest.dart", 1);
  }

  public void testStaticSuperGetterNegativeTest() {
    checkNumErrors("StaticSuperGetterNegativeTest.dart", 1);
  }

  public void testStaticSuperMethodNegativeTest() {
    checkNumErrors("StaticSuperMethodNegativeTest.dart", 1);
  }

  public void testBadNamedConstructorNegativeTest() {
    checkNumErrors("BadNamedConstructorNegativeTest.dart", 1);
  }

  public void testCyclicRedirectedConstructorNegativeTest() {
    checkNumErrors("CyclicRedirectedConstructorNegativeTest.dart", 3);
  }

  public void testConstRedirectedConstructorNegativeTest() {
    checkNumErrors("ConstRedirectedConstructorNegativeTest.dart", 1);
  }

  private TestCompilerContext getContext() {
    return new TestCompilerContext() {
      @Override
      public void compilationError(DartCompilationError event) {
        errors.add(event);
      }

      @Override
      public void typeError(DartCompilationError event) {
        typeErrors.add(event);
      }
    };
  }
}
