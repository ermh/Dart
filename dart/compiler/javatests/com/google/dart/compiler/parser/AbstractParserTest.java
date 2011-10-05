// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

package com.google.dart.compiler.parser;

import com.google.dart.compiler.CompilerTestCase;
import com.google.dart.compiler.ast.DartDirective;
import com.google.dart.compiler.ast.DartImportDirective;
import com.google.dart.compiler.ast.DartLibraryDirective;
import com.google.dart.compiler.ast.DartResourceDirective;
import com.google.dart.compiler.ast.DartSourceDirective;
import com.google.dart.compiler.ast.DartUnit;

import java.net.URL;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

/**
 * Tests for the parser, which simply assert that valid source units parse
 * correctly. All tests invoking {@code parseUnit} are designed such that
 * they will throw an exception if anything goes wrong in the parser.
 */
public abstract class AbstractParserTest extends CompilerTestCase {

  public void testClasses() {
    parseUnit("ClassesInterfaces.dart");
  }

  public void testMethodSignatures() {
    parseUnit("MethodSignatures.dart");
  }

  public void testFunctionTypes() {
    parseUnit("FunctionTypes.dart");
  }

  public void testFormalParameters() {
    parseUnit("FormalParameters.dart");
  }

  public void testSuperCalls() {
    parseUnit("SuperCalls.dart");
  }

  public void testGenericTypedef() {
    parseUnit("GenericTypedef.dart");
  }

  public void testGenericTypes() {
    parseUnit("GenericTypes.dart");
  }

  public void testShifting() {
    parseUnit("Shifting.dart");
  }

  public void testFunctionInterfaces() {
    parseUnit("FunctionInterfaces.dart");
  }

  public void testStringBuffer() {
    parseUnit("StringBuffer.dart");
  }

  public void testListObjectLiterals() {
    parseUnit("ListObjectLiterals.dart");
  }

  public void testCatchFinally() {
    parseUnit("CatchFinally.dart");
  }

  public void testStrings() {
    parseUnit("Strings.dart");
  }

  public void testNewWithPrefix() {
    parseUnit("NewWithPrefix.dart");
  }

  public void testRedirectedConstructor() {
    parseUnit("RedirectedConstructor.dart");
  }

  public void testTopLevel() {
    parseUnit("TopLevel.dart");
  }

  public void testDirectives() {
    DartUnit unit = parseUnit("Directives.dart");
    Iterator<DartDirective> iter = unit.getDirectives().iterator();

    DartDirective directive = iter.next();
    assertEquals(DartLibraryDirective.class, directive.getClass());
    assertEquals("a-directives-test", ((DartLibraryDirective) directive).getName().getValue());

    directive = iter.next();
    assertEquals(DartImportDirective.class, directive.getClass());
    assertEquals("dart:core", ((DartImportDirective) directive).getLibraryUri().getValue());
    assertEquals(null, ((DartImportDirective) directive).getPrefix());

    directive = iter.next();
    assertEquals(DartSourceDirective.class, directive.getClass());
    assertEquals("ListObjectLiterals.dart", ((DartSourceDirective) directive).getSourceUri().getValue());

    directive = iter.next();
    assertEquals(DartResourceDirective.class, directive.getClass());
    assertEquals("myimage.gif", ((DartResourceDirective) directive).getResourceUri().getValue());
  }

  public void testDirectives2() {
    DartUnit unit = parseUnit("Directives2.dart");
    Iterator<DartDirective> iter = unit.getDirectives().iterator();

    DartDirective directive = iter.next();
    assertEquals(DartLibraryDirective.class, directive.getClass());
    assertEquals("b-directives-test", ((DartLibraryDirective) directive).getName().getValue());

    directive = iter.next();
    assertEquals(DartImportDirective.class, directive.getClass());
    assertEquals("dart:core", ((DartImportDirective) directive).getLibraryUri().getValue());
    assertEquals(null, ((DartImportDirective) directive).getPrefix());

    directive = iter.next();
    assertEquals(DartSourceDirective.class, directive.getClass());
    assertEquals("SomeClass.dart", ((DartSourceDirective) directive).getSourceUri().getValue());

    directive = iter.next();
    assertEquals(DartResourceDirective.class, directive.getClass());
    assertEquals("myimage2.gif", ((DartResourceDirective) directive).getResourceUri().getValue());
  }

  public abstract void testStringsErrors();

  public void testTiming() {
    String[] inputs = new String[]{
        "ClassesInterfaces.dart", "MethodSignatures.dart",
        "FunctionTypes.dart", "FormalParameters.dart", "SuperCalls.dart",
        "GenericTypes.dart", "Shifting.dart", "FunctionInterfaces.dart",
        "StringBuffer.dart", "ListObjectLiterals.dart", "CatchFinally.dart",
        "Strings.dart",};
    StringBuilder out = new StringBuilder();
    for (String input : inputs) {
      URL url = inputUrlFor(getClass(), input);
      String source = readUrl(url);
      for (int i = 0; i < 50; ++i) {
        out.append(source);
      }
    }
    String megaSource = out.toString();
    long start = System.currentTimeMillis();
    parseUnit("Mega.dart", megaSource);
    System.out.format("%s ms for '%s.%s()'%n", System.currentTimeMillis() - start,
                      getClass().getName(), getName());
  }

  @Override
  protected DartParser makeParser(ParserContext context) {
    Set<String> set = new HashSet<String>();
    set.add("prefix");
    return new DartParser(context, set);
  }
}
