// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

package com.google.dart.compiler.parser;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.io.CharStreams;
import com.google.dart.compiler.DartCompilationError;
import com.google.dart.compiler.DartCompilerErrorCode;
import com.google.dart.compiler.DartCompilerListener;
import com.google.dart.compiler.DartSource;
import com.google.dart.compiler.ErrorCode;
import com.google.dart.compiler.InternalCompilerException;
import com.google.dart.compiler.LibrarySource;
import com.google.dart.compiler.Source;
import com.google.dart.compiler.ast.DartArrayAccess;
import com.google.dart.compiler.ast.DartArrayLiteral;
import com.google.dart.compiler.ast.DartAssertion;
import com.google.dart.compiler.ast.DartBinaryExpression;
import com.google.dart.compiler.ast.DartBlock;
import com.google.dart.compiler.ast.DartBooleanLiteral;
import com.google.dart.compiler.ast.DartBreakStatement;
import com.google.dart.compiler.ast.DartCase;
import com.google.dart.compiler.ast.DartCatchBlock;
import com.google.dart.compiler.ast.DartClass;
import com.google.dart.compiler.ast.DartConditional;
import com.google.dart.compiler.ast.DartContinueStatement;
import com.google.dart.compiler.ast.DartDeclaration;
import com.google.dart.compiler.ast.DartDefault;
import com.google.dart.compiler.ast.DartDoWhileStatement;
import com.google.dart.compiler.ast.DartDoubleLiteral;
import com.google.dart.compiler.ast.DartEmptyStatement;
import com.google.dart.compiler.ast.DartExprStmt;
import com.google.dart.compiler.ast.DartExpression;
import com.google.dart.compiler.ast.DartField;
import com.google.dart.compiler.ast.DartFieldDefinition;
import com.google.dart.compiler.ast.DartForInStatement;
import com.google.dart.compiler.ast.DartForStatement;
import com.google.dart.compiler.ast.DartFunction;
import com.google.dart.compiler.ast.DartFunctionExpression;
import com.google.dart.compiler.ast.DartFunctionObjectInvocation;
import com.google.dart.compiler.ast.DartFunctionTypeAlias;
import com.google.dart.compiler.ast.DartIdentifier;
import com.google.dart.compiler.ast.DartIfStatement;
import com.google.dart.compiler.ast.DartImportDirective;
import com.google.dart.compiler.ast.DartInitializer;
import com.google.dart.compiler.ast.DartIntegerLiteral;
import com.google.dart.compiler.ast.DartInvocation;
import com.google.dart.compiler.ast.DartLabel;
import com.google.dart.compiler.ast.DartLibraryDirective;
import com.google.dart.compiler.ast.DartMapLiteral;
import com.google.dart.compiler.ast.DartMapLiteralEntry;
import com.google.dart.compiler.ast.DartMethodDefinition;
import com.google.dart.compiler.ast.DartMethodInvocation;
import com.google.dart.compiler.ast.DartNamedExpression;
import com.google.dart.compiler.ast.DartNativeBlock;
import com.google.dart.compiler.ast.DartNativeDirective;
import com.google.dart.compiler.ast.DartNewExpression;
import com.google.dart.compiler.ast.DartNode;
import com.google.dart.compiler.ast.DartNullLiteral;
import com.google.dart.compiler.ast.DartParameter;
import com.google.dart.compiler.ast.DartParameterizedNode;
import com.google.dart.compiler.ast.DartParenthesizedExpression;
import com.google.dart.compiler.ast.DartPropertyAccess;
import com.google.dart.compiler.ast.DartRedirectConstructorInvocation;
import com.google.dart.compiler.ast.DartResourceDirective;
import com.google.dart.compiler.ast.DartReturnStatement;
import com.google.dart.compiler.ast.DartSourceDirective;
import com.google.dart.compiler.ast.DartStatement;
import com.google.dart.compiler.ast.DartStringInterpolation;
import com.google.dart.compiler.ast.DartStringLiteral;
import com.google.dart.compiler.ast.DartSuperConstructorInvocation;
import com.google.dart.compiler.ast.DartSuperExpression;
import com.google.dart.compiler.ast.DartSwitchMember;
import com.google.dart.compiler.ast.DartSwitchStatement;
import com.google.dart.compiler.ast.DartSyntheticErrorExpression;
import com.google.dart.compiler.ast.DartSyntheticErrorStatement;
import com.google.dart.compiler.ast.DartThisExpression;
import com.google.dart.compiler.ast.DartThrowStatement;
import com.google.dart.compiler.ast.DartTryStatement;
import com.google.dart.compiler.ast.DartTypeExpression;
import com.google.dart.compiler.ast.DartTypeNode;
import com.google.dart.compiler.ast.DartTypeParameter;
import com.google.dart.compiler.ast.DartUnaryExpression;
import com.google.dart.compiler.ast.DartUnit;
import com.google.dart.compiler.ast.DartUnqualifiedInvocation;
import com.google.dart.compiler.ast.DartVariable;
import com.google.dart.compiler.ast.DartVariableStatement;
import com.google.dart.compiler.ast.DartWhileStatement;
import com.google.dart.compiler.ast.LibraryNode;
import com.google.dart.compiler.ast.LibraryUnit;
import com.google.dart.compiler.ast.Modifiers;
import com.google.dart.compiler.parser.DartScanner.Location;
import com.google.dart.compiler.util.Lists;

import java.io.IOException;
import java.io.Reader;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * The Dart parser. Parses a single compilation unit and produces a {@link DartUnit}.
 * The grammar rules are taken from Dart.g revision 557.
 */
public class DartParser extends CompletionHooksParserBase {

  private Set<Integer> errorHistory = new HashSet<Integer>();
  private Set<String> prefixes;
  private boolean isDietParse;
  private boolean isParsingInterface;

  /**
   * Determines the maximum number of errors before terminating the parser. See
   * {@link #reportError(com.google.dart.compiler.parser.DartScanner.Position, ErrorCode,
   * Object...)}.
   */
  final int MAX_DEFAULT_ERRORS = 100;

  // Pseudo-keywords that should also be valid identifiers.
  private static final String ABSTRACT_KEYWORD = "abstract";
  private static final String ASSERT_KEYWORD = "assert";
  private static final String CLASS_KEYWORD = "class";
  private static final String EXTENDS_KEYWORD = "extends";
  private static final String FACTORY_KEYWORD = "factory";
  private static final String GETTER_KEYWORD = "get";
  private static final String IMPLEMENTS_KEYWORD = "implements";
  private static final String INTERFACE_KEYWORD = "interface";
  private static final String NATIVE_KEYWORD = "native";
  private static final String NEGATE_KEYWORD = "negate";
  private static final String OPERATOR_KEYWORD = "operator";
  private static final String PREFIX_KEYWORD = "prefix";
  private static final String SETTER_KEYWORD = "set";
  private static final String STATIC_KEYWORD = "static";
  private static final String TYPEDEF_KEYWORD = "typedef";

  public static final String[] PSEUDO_KEYWORDS = {
    ABSTRACT_KEYWORD,
    ASSERT_KEYWORD,
    CLASS_KEYWORD,
    EXTENDS_KEYWORD,
    FACTORY_KEYWORD,
    GETTER_KEYWORD,
    IMPLEMENTS_KEYWORD,
    INTERFACE_KEYWORD,
    NEGATE_KEYWORD,
    NATIVE_KEYWORD,
    OPERATOR_KEYWORD,
    PREFIX_KEYWORD,
    SETTER_KEYWORD,
    STATIC_KEYWORD,
    TYPEDEF_KEYWORD
  };

  public DartParser(Source source,
                    String sourceCode,
                    DartCompilerListener listener) {
    this(new DartScannerParserContext(source, sourceCode, listener));
  }

  public DartParser(ParserContext ctx) {
    this(ctx, false);
  }

  public DartParser(ParserContext ctx, Set<String> prefixes) {
    this(ctx, false, prefixes);
  }

  public DartParser(ParserContext ctx, boolean isDietParse) {
    this(ctx, isDietParse, Collections.<String>emptySet());
  }

  public DartParser(ParserContext ctx, boolean isDietParse, Set<String> prefixes) {
    super(ctx);
    this.isDietParse = isDietParse;
    this.prefixes = prefixes;
  }

  private DartParser(Source source, DartCompilerListener listener) throws IOException {
    this(source, source.getSourceReader(), listener);
  }
  
  private DartParser(Source source,
                    Reader sourceReader,
                    DartCompilerListener listener) throws IOException {
    this(new DartScannerParserContext(source, read(sourceReader), listener));
  }

  public static DartParser getSourceParser(Source source, DartCompilerListener listener)
      throws IOException {
    return new DartParser(source, listener);
  }

  private static String read(Reader reader) throws IOException {
    try {
      return CharStreams.toString(reader);
    } finally {
      reader.close();
    }
  }

  /**
   * A flag indicating whether function expressions are allowed.  See
   * {@link #setAllowFunctionExpression(boolean)}.
   */
  private boolean allowFunctionExpression = true;

  /**
   * Set the {@link #allowFunctionExpression} flag indicating whether function expressions are
   * allowed, returning the old value. This is required to avoid ambiguity in a few places in the
   * grammar.
   *
   * @param allow true if function expressions are allowed, false if not
   * @return previous value of the flag, which should be restored
   */
  private boolean setAllowFunctionExpression(boolean allow) {
    boolean old = allowFunctionExpression;
    allowFunctionExpression = allow;
    return old;
  }

  /**
   * <pre>
   * compilationUnit
   *     : libraryDeclaration? topLevelDefinition* EOF
   *     ;
   *
   * libraryDeclaration
   *     : libraryDirective? importDirective* sourceDirective* resourceDirective* nativeDirective*
   *
   * topLevelDefinition
   *     : classDefinition
   *     | interfaceDefinition
   *     | functionTypeAlias
   *     | methodOrConstructorDeclaration functionStatementBody
   *     | type? getOrSet identifier formalParameterList functionStatementBody
   *     | CONST type? staticConstDeclarationList ';'
   *     | variableDeclaration ';'
   *     ;
   * </pre>
   */
  public DartUnit parseUnit(DartSource source) {
    beginCompilationUnit();
    DartUnit unit = new DartUnit(source);

    // parse any directives at the beginning of the source
    parseDirectives(unit);

    while (!EOS()) {
      DartNode node = null;
      beginTopLevelElement();
      isParsingInterface = false;
      if (optionalPseudoKeyword(CLASS_KEYWORD)) {
        node = done(parseClass());
      } else if (optionalPseudoKeyword(INTERFACE_KEYWORD)) {
        isParsingInterface = true;
        node = done(parseClass());
      } else if (optionalPseudoKeyword(TYPEDEF_KEYWORD)) {
        node = done(parseFunctionTypeAlias());
      } else {
        node = done(parseFieldOrMethod(false));
      }
      if (node != null) {
        unit.addTopLevelNode(node);
      }
    }
    expect(Token.EOS);
    return done(unit);
  }

  /**
   * A version of the parser which only parses the directives of a library.
   * 
   * TODO(jbrosenberg): consider parsing the whole file here, in order to avoid
   * duplicate work.  Probably requires removing use of LibraryUnit's, etc.
   * Also, this minimal parse does have benefit in the incremental compilation
   * case.
   */
  public LibraryUnit preProcessLibraryDirectives(LibrarySource source) {
    beginCompilationUnit();
    LibraryUnit libUnit = new LibraryUnit(source);
    if (peek(0) == Token.LIBRARY) {
      beginLibraryDirective();
      DartLibraryDirective libDirective = done(parseLibraryDirective());
      libUnit.setName(libDirective.getName().getValue());
    }
    while (peek(0) == Token.IMPORT) {
      beginImportDirective();
      DartImportDirective importDirective = done(parseImportDirective());
      LibraryNode importPath;
      if (importDirective.getPrefix() != null) {
        importPath =
            new LibraryNode(importDirective.getLibraryUri().getValue(), importDirective.getPrefix()
                .getValue());
      } else {
        importPath = new LibraryNode(importDirective.getLibraryUri().getValue());
      }
      libUnit.addImportPath(importPath);
    }
    while (peek(0) == Token.SOURCE) {
      beginSourceDirective();
      DartSourceDirective sourceDirective = done(parseSourceDirective());
      LibraryNode sourcePath = new LibraryNode(sourceDirective.getSourceUri().getValue());
      libUnit.addSourcePath(sourcePath);
    }
    while (peek(0) == Token.RESOURCE) {
      beginResourceDirective();
      DartResourceDirective resourceDirective = done(parseResourceDirective());
      LibraryNode resourcePath = new LibraryNode(resourceDirective.getResourceUri().getValue());
      libUnit.addResourcePath(resourcePath);
    }
    while (peek(0) == Token.NATIVE) {
      beginNativeDirective();
      DartNativeDirective nativeDirective = done(parseNativeDirective());
      LibraryNode nativePath = new LibraryNode(nativeDirective.getNativeUri().getValue());
      libUnit.addNativePath(nativePath);
    }

    // add ourselves to the list of sources, so inline dart code will be parsed
    libUnit.addSourcePath(libUnit.getSelfSourcePath());
    return done(libUnit);
  }

  private void parseDirectives(DartUnit unit) {
    if (peek(0) == Token.LIBRARY) {
      beginLibraryDirective();
      unit.addDirective(done(parseLibraryDirective()));
    }
    while (peek(0) == Token.IMPORT) {
      beginImportDirective();
      unit.addDirective(done(parseImportDirective()));
    }
    while (peek(0) == Token.SOURCE) {
      beginSourceDirective();
      unit.addDirective(done(parseSourceDirective()));
    }
    while (peek(0) == Token.RESOURCE) {
      beginResourceDirective();
      unit.addDirective(done(parseResourceDirective()));
    }
    while (peek(0) == Token.NATIVE) {
      beginResourceDirective();
      unit.addDirective(done(parseNativeDirective()));
    }
  }

  private DartLibraryDirective parseLibraryDirective() {
    expect(Token.LIBRARY);
    expect(Token.LPAREN);
    beginLiteral();
    expect(Token.STRING);
    DartStringLiteral libname = done(DartStringLiteral.get(ctx.getTokenString()));
    expectCloseParen();
    expect(Token.SEMICOLON);
    return new DartLibraryDirective(libname);
  }

  private DartImportDirective parseImportDirective() {
    expect(Token.IMPORT);
    expect(Token.LPAREN);
    beginLiteral();
    expect(Token.STRING);
    DartStringLiteral libUri = done(DartStringLiteral.get(ctx.getTokenString()));
    DartStringLiteral prefix = null;
    if (optional(Token.COMMA)) {
      if (!optionalPseudoKeyword(PREFIX_KEYWORD)) {
        reportError(position(), DartCompilerErrorCode.EXPECTED_PREFIX_KEYWORD);
      }
      expect(Token.COLON);
      beginLiteral();
      expect(Token.STRING);
      prefix = done(DartStringLiteral.get(ctx.getTokenString()));
    }
    expectCloseParen();
    expect(Token.SEMICOLON);
    return new DartImportDirective(libUri, prefix);
  }

  private DartSourceDirective parseSourceDirective() {
    expect(Token.SOURCE);
    expect(Token.LPAREN);
    beginLiteral();
    expect(Token.STRING);
    DartStringLiteral sourceUri = done(DartStringLiteral.get(ctx.getTokenString()));
    expectCloseParen();
    expect(Token.SEMICOLON);
    return new DartSourceDirective(sourceUri);
  }

  private DartResourceDirective parseResourceDirective() {
    expect(Token.RESOURCE);
    expect(Token.LPAREN);
    beginLiteral();
    expect(Token.STRING);
    DartStringLiteral resourceUri = done(DartStringLiteral.get(ctx.getTokenString()));
    expectCloseParen();
    expect(Token.SEMICOLON);
    return new DartResourceDirective(resourceUri);
  }

  private DartNativeDirective parseNativeDirective() {
    expect(Token.NATIVE);
    expect(Token.LPAREN);
    beginLiteral();
    expect(Token.STRING);
    DartStringLiteral nativeUri = done(DartStringLiteral.get(ctx.getTokenString()));
    expect(Token.RPAREN);
    expect(Token.SEMICOLON);
    return new DartNativeDirective(nativeUri);
  }

  /**
   * <pre>
   * typeParameter
   *     : identifier (EXTENDS type)?
   *     ;
   *
   * typeParameters
   *     : '<' typeParameter (',' typeParameter)* '>'
   *     ;
   * </pre>
   */
  private List<DartTypeParameter> parseTypeParameters() {
    List<DartTypeParameter> types = new ArrayList<DartTypeParameter>();
    expect(Token.LT);
    do {
      beginTypeParameter();
      DartIdentifier name = parseIdentifier();
      DartTypeNode bound = null;
      if (optionalPseudoKeyword(EXTENDS_KEYWORD)) {
        bound = parseTypeAnnotation();
      }
      types.add(done(new DartTypeParameter(name, bound)));
    } while (optional(Token.COMMA));
    expect(Token.GT);
    return types;
  }

  private List<DartTypeParameter> parseTypeParametersOpt() {
    return (peek(0) == Token.LT)
        ? parseTypeParameters()
        : Collections.<DartTypeParameter>emptyList();
  }

  /**
   * <pre>
   * classDefinition
   *     : CLASS identifier typeParameters? superclass? interfaces?
   *       '{' classMemberDefinition* '}'
   *     ;
   *
   * superclass
   *     : EXTENDS type
   *     ;
   *
   * interfaces
   *     : IMPLEMENTS typeList
   *     ;
   *
   * superinterfaces
   *     : EXTENDS typeList
   *     ;
   *
   * classMemberDefinition
   *     : declaration ';'
   *     | methodDeclaration blockOrNative
   *
   * interfaceDefinition
   *     : INTERFACE identifier typeParameters? superinterfaces?
   *       (DEFAULT type)? '{' (interfaceMemberDefinition)* '}'
   *     ;
   * </pre>
   */
  private DartDeclaration<?> parseClass() {
    beginClassBody();

    DartIdentifier name = parseIdentifier();
    List<DartTypeParameter> typeParameters = parseTypeParametersOpt();

    // Parse the extends and implements clauses.
    DartTypeNode superType = null;
    List<DartTypeNode> interfaces = null;
    if (isParsingInterface) {
      if (optionalPseudoKeyword(EXTENDS_KEYWORD)) {
        interfaces = parseTypeAnnotationList();
      }
    } else {
      if (optionalPseudoKeyword(EXTENDS_KEYWORD)) {
        superType = parseTypeAnnotation();
      }
      if (optionalPseudoKeyword(IMPLEMENTS_KEYWORD)) {
        interfaces = parseTypeAnnotationList();
      }
    }

    // Deal with factory clause for interfaces.
    DartTypeNode factory = null;
    if (isParsingInterface && optionalPseudoKeyword(FACTORY_KEYWORD)) {
      factory = parseTypeAnnotation();
    }

    // Deal with native clause for classes.
    DartStringLiteral nativeName = null;
    if (!isParsingInterface && optionalPseudoKeyword(NATIVE_KEYWORD)) {
      beginLiteral();
      expect(Token.STRING);
      nativeName = done(DartStringLiteral.get(ctx.getTokenString()));
      if (superType != null) {
        reportError(position(), DartCompilerErrorCode.EXTENDED_NATIVE_CLASS);
      }
    }

    // Parse the members.
    expect(Token.LBRACE);
    List<DartNode> members = new ArrayList<DartNode>();
    while (!match(Token.RBRACE) && !EOS()) {
      DartNode member = parseFieldOrMethod(true);
      if (member != null) {
        members.add(member);
      }
    }
    expectCloseBrace();

    if (isParsingInterface) {
      return done(new DartClass(name, superType, interfaces, members, typeParameters, factory));
    } else {
      return done(new DartClass(name, nativeName, superType, interfaces, members, typeParameters));
    }
  }

  private List<DartTypeNode> parseTypeAnnotationList() {
    List<DartTypeNode> result = new ArrayList<DartTypeNode>();
    do {
      result.add(parseTypeAnnotation());
    } while (optional(Token.COMMA));
    return result;
  }

  /**
   * Look ahead to detect if we are seeing ident [ TypeParameters ] "(".
   * We need this lookahead to distinguish between the optional return type
   * and the alias name of a function type alias.
   * Token position remains unchanged.
   *
   * @return true if the next tokens should be parsed as a type
   */
  private boolean isFunctionTypeAliasName() {
    startLookahead();
    try {
      if (peek(0) == Token.IDENTIFIER && peek(1) == Token.LPAREN) {
        return true;
      }
      if (peek(0) == Token.IDENTIFIER && peek(1) == Token.LT) {
        consume(Token.IDENTIFIER);
        // isTypeParameter leaves the position advanced if it matches
        if (isTypeParameter() && peek(0) == Token.LPAREN) {
          return true;
        }
      }
      return false;
    } finally {
      rollback();
    }
  }

  /**
   * Returns true if the current and next tokens can be parsed as type
   * parameters. Current token position is not saved and restored.
   */
  private boolean isTypeParameter() {
    if (peek(0) == Token.LT) {
      // We are possibly looking at type parameters. Find closing ">".
      consume(Token.LT);
      int nestingLevel = 1;
      while (nestingLevel > 0) {
        switch (peek(0)) {
          case LT:
            nestingLevel++;
            break;
          case GT:
            nestingLevel--;
            break;
          case SAR:   // >>
            nestingLevel -= 2;
            break;
          case SHR:   // >>>
            nestingLevel -= 3;
            break;
          case COMMA:
          case IDENTIFIER:
            break;
          default:
            // We are looking at something other than type parameters.
            return false;
        }
        next();
        if (nestingLevel < 0) {
          return false;
        }
      }
    }
    return true;
  }

  /**
   * <pre>
   * functionTypeAlias
   *     : TYPEDEF functionPrefix typeParameters?
   *       formalParameterList ';'
   *
   * functionPrefix
   *     : returnType? identifier
   * </pre>
   */
  private DartFunctionTypeAlias parseFunctionTypeAlias() {
    beginFunctionTypeInterface();

    DartTypeNode returnType = null;
    if (peek(0) == Token.VOID) {
      returnType = parseVoidType();
    } else if (!isFunctionTypeAliasName()) {
      returnType = parseTypeAnnotation();
    }

    DartIdentifier name = parseIdentifier();
    List<DartTypeParameter> typeParameters = parseTypeParametersOpt();
    List<DartParameter> params = parseFormalParameterList();
    expect(Token.SEMICOLON);

    return done(new DartFunctionTypeAlias(name, returnType, params, typeParameters));
  }

  /**
   * Parse a field or method, which may be inside a class or at the top level.
   * 
   * <pre>
   * // This rule is organized in a way that may not be most readable, but
   * // gives the best error messages.
   * classMemberDefinition
   *     : declaration ';'
   *     | methodDeclaration bodyOrNative
   *     ;
   *
   * // Note: this syntax is not official, but used in dart_interpreter. It
   * // is unlikely that Dart will support numbered natives.
   * bodyOrNative
   *     : error=NATIVE (':' (STRING | RATIONAL_NUMBER))? ';'
   *       { legacy($error, "native not supported (yet)"); }
   *     | functionStatementBody
   *     ;
   *
   * // A method, operator, or constructor (which all should be followed by
   * // a function body).
   * methodDeclaration
   *     : factoryConstructorDeclaration
   *     | STATIC methodOrConstructorDeclaration
   *     | specialSignatureDefinition
   *     | methodOrConstructorDeclaration initializers?
   *     | namedConstructorDeclaration initializers?
   *     ;
   *
   *
   * // An abstract method/operator, a field, or const constructor (which
   * // all should be followed by a semicolon).
   * declaration
   *     : constantConstructorDeclaration initializers?
   *     | ABSTRACT specialSignatureDefinition
   *     | ABSTRACT methodOrConstructorDeclaration
   *     | STATIC CONST type? staticConstDeclarationList
   *     | STATIC? variableDeclaration
   *     ;
   *
   * interfaceMemberDefinition
   *     : STATIC CONST type? initializedIdentifierList ';'
   *     | methodOrConstructorDeclaration ';'
   *     | constantConstructorDeclaration ';'
   *     | namedConstructorDeclaration ';'
   *     | specialSignatureDefinition ';'
   *     | variableDeclaration ';'
   *     ;
   *
   * variableDeclaration
   *     : constVarOrType identifierList
   *     ;
   *
   * methodOrConstructorDeclaration
   *     : typeOrFunction? identifier formalParameterList
   *     ;
   *
   * factoryConstructorDeclaration
   *     : FACTORY qualified ('.' identifier)? formalParameterList
   *     ;
   *
   * namedConstructorDeclaration
   *     : identifier '.' identifier formalParameterList
   *     ;
   *
   * constructorDeclaration
   *     : identifier formalParameterList
   *     | namedConstructorDeclaration
   *     ;
   *
   * constantConstructorDeclaration
   *     : CONST qualified formalParameterList
   *     ;
   *
   * specialSignatureDefinition
   *     : STATIC? type? getOrSet identifier formalParameterList
   *     | type? OPERATOR operator formalParameterList
   *     ;
   *
   * getOrSet
   *     : GET
   *     | SET
   *     ;
   *
   * operator
   *     : unaryOperator
   *     | binaryOperator
   *     | '[' ']' { "[]".equals($text) }?
   *     | '[' ']' '=' { "[]=".equals($text) }?
   *     | NEGATE
   *     ;
   * </pre>
   * 
   * @param allowStatic true if the static modifier is allowed
   * @return a {@link DartNode} representing the grammar fragment above
   */
  private DartNode parseFieldOrMethod(boolean allowStatic) {
    beginClassMember();
    Modifiers modifiers = Modifiers.NONE;
    if (optionalPseudoKeyword(STATIC_KEYWORD)) {
      if (!allowStatic) {
        reportError(position(), DartCompilerErrorCode.TOP_LEVEL_IS_STATIC);
      } else {
        if (isParsingInterface
            && (peek(0) != Token.FINAL)) {
          reportError(position(), DartCompilerErrorCode.NON_FINAL_STATIC_MEMBER_IN_INTERFACE);
        }
        modifiers = modifiers.makeStatic();
      }
    } else if (optionalPseudoKeyword(ABSTRACT_KEYWORD)) {
      if (isParsingInterface) {
        reportError(position(), DartCompilerErrorCode.ABSTRACT_MEMBER_IN_INTERFACE);
      }
      modifiers = modifiers.makeAbstract();
    } else if (optionalPseudoKeyword(FACTORY_KEYWORD)) {
      if (isParsingInterface) {
        reportError(position(), DartCompilerErrorCode.FACTORY_MEMBER_IN_INTERFACE);
      }
      modifiers = modifiers.makeFactory();
    }

    if (match(Token.VAR) || match(Token.FINAL)) {
      if (modifiers.isAbstract()) {
        reportError(position(), DartCompilerErrorCode.DISALLOWED_ABSTRACT_KEYWORD);
      } else if (modifiers.isFactory()) {
        reportError(position(), DartCompilerErrorCode.DISALLOWED_FACTORY_KEYWORD);
      }
    }

    if (modifiers.isFactory()) {
      return done(parseFactory(modifiers));
    }

    final DartNode member;

    switch (peek(0)) {
      case VAR: {
        consume(Token.VAR);
        member = parseFieldDeclaration(modifiers, null);
        expectStatmentTerminator();
        break;
      }

      case CONST: {
        consume(Token.CONST);
        modifiers = modifiers.makeConstant();
        member = done(parseMethod(modifiers, null));
        break;
      }

      case FINAL: {
        consume(Token.FINAL);
        modifiers = modifiers.makeFinal();
        DartTypeNode type = null;
        if (peek(1) != Token.COMMA
            && peek(1) != Token.ASSIGN
            && peek(1) != Token.SEMICOLON) {
          type = parseTypeAnnotation();
        }
        member = parseFieldDeclaration(modifiers, type);
        expectStatmentTerminator();
        break;
      }

      case VOID:
      case IDENTIFIER: {
        // Check to see if it's a method/ctor.
        if (peek(1) == Token.LPAREN
            || peek(1) == Token.PERIOD
            || peekPseudoKeyword(0, OPERATOR_KEYWORD)
            || peekPseudoKeyword(0, GETTER_KEYWORD)
            || peekPseudoKeyword(0, SETTER_KEYWORD)) {
          member = parseMethodOrAccessor(modifiers, null);
          break;
        }

        // The next token must be a type specification: either a method or field.
        boolean isVoidType = peek(0) == Token.VOID;
        DartTypeNode type = isVoidType ? parseVoidType() : parseTypeAnnotation();
        if (peek(1) == Token.SEMICOLON
            || peek(1) == Token.COMMA
            || peek(1) == Token.ASSIGN) {
          if (modifiers.isAbstract()) {
            reportError(position(), DartCompilerErrorCode.INVALID_FIELD_DECLARATION);
          }
          member = parseFieldDeclaration(modifiers, type);
          if (isVoidType) {
            reportError(type, DartCompilerErrorCode.VOID_FIELD);
          }
          expectStatmentTerminator();
        } else {
          member = parseMethodOrAccessor(modifiers, type);
        }
        break;
      }

      default: {
        done(null);
        reportUnexpectedToken(position(), null, next());
        member = null;
        break;
      }
    }
    return member;
  }

  /**
   * <pre>
   * factoryConstructorDeclaration
   *     : FACTORY qualified ('.' identifier)? formalParameterList
   *     ;
   * </pre>
   */
  private DartMethodDefinition parseFactory(Modifiers modifiers) {
    beginMethodName();
    DartExpression name = parseQualified();
    List<DartTypeParameter> typeParameters = parseTypeParametersOpt();
    if (!typeParameters.isEmpty()) {
      name = doneWithoutConsuming(new DartParameterizedNode(name, typeParameters));
    }
    if (optional(Token.PERIOD)) {
      name = doneWithoutConsuming(new DartPropertyAccess(name, parseIdentifier()));
    }
    done(name);
    List<DartParameter> formals = parseFormalParameterList();
    DartFunction function;
    if (peekPseudoKeyword(0, NATIVE_KEYWORD)) {
      modifiers = modifiers.makeNative();
      function = new DartFunction(formals, parseNativeBlock(modifiers), null);
    } else {
      function = new DartFunction(formals, parseBlock(), null);
    }
    doneWithoutConsuming(function);
    return DartMethodDefinition.create(name, function, modifiers, null, typeParameters);
  }

  private DartIdentifier parseVoidIdentifier() {
    beginIdentifier();
    expect(Token.VOID);
    return done(new DartIdentifier(Token.VOID.getSyntax()));
  }

  private DartTypeNode parseVoidType() {
    beginTypeAnnotation();
    return done(new DartTypeNode(parseVoidIdentifier()));
  }

  private DartMethodDefinition parseMethod(Modifiers modifiers, DartTypeNode returnType) {
    DartExpression name = null;

    if (modifiers.isFactory()) {
      if (modifiers.isAbstract()) {
        reportError(position(), DartCompilerErrorCode.FACTORY_CANNOT_BE_ABSTRACT);
      }
      if (modifiers.isStatic()) {
        reportError(position(), DartCompilerErrorCode.FACTORY_CANNOT_BE_STATIC);
      }
    }

    int arity = -1;
    if (optionalPseudoKeyword(OPERATOR_KEYWORD)) {
      // Overloaded operator.
      if (modifiers.isStatic()) {
        reportError(position(), DartCompilerErrorCode.OPERATOR_CANNOT_BE_STATIC);
      }
      modifiers = modifiers.makeOperator();

      beginOperatorName();
      Token operation = next();
      if (operation.isUserDefinableOperator()) {
        name = done(new DartIdentifier(operation.getSyntax()));
        if (operation == Token.ASSIGN_INDEX) {
          arity = 2;
        } else if (operation.isBinaryOperator()) {
          arity = 1;
        } else if (operation == Token.INDEX) {
          arity = 1;
        } else {
          assert operation.isUnaryOperator();
          arity = 0;
        }
      } else if (operation == Token.IDENTIFIER
                 && ctx.getTokenString().equals(NEGATE_KEYWORD)) {
        name = done(new DartIdentifier(NEGATE_KEYWORD));
        arity = 0;
      } else {
        reportUnexpectedToken(position(), Token.COMMENT, operation);
        done(null);
      }
    } else {
      beginMethodName();
      // Check for getters and setters.
      if (optionalPseudoKeyword(GETTER_KEYWORD)) {
        name = parseIdentifier();
        modifiers = modifiers.makeGetter();
        arity = 0;
      } else if (optionalPseudoKeyword(SETTER_KEYWORD)) {
        name = parseIdentifier();
        modifiers = modifiers.makeSetter();
        arity = 1;
      } else {
        // Normal method or property.
        name = parseIdentifier();
      }

      // Check for named constructor.
      if (optional(Token.PERIOD)) {
        name = doneWithoutConsuming(new DartPropertyAccess(name, parseIdentifier()));

        if (optional(Token.PERIOD)) {
          name = doneWithoutConsuming(new DartPropertyAccess(name, parseIdentifier()));
        }
      }
      done(null);
    }

    // Parse the argument definitions.
    List<DartParameter> arguments = parseFormalParameterList();

    if (arity != -1 && arguments.size() != arity) {
      reportError(position(), DartCompilerErrorCode.ILLEGAL_NUMBER_OF_ARGUMENTS);
    }

    // Parse initializer expressions for constructors.
    List<DartInitializer> initializers = new ArrayList<DartInitializer>();
    if (match(Token.COLON) && !(isParsingInterface || modifiers.isFactory())) {
      boolean isRedirectedConstructor = parseInitializers(initializers);
      if (isRedirectedConstructor) {
        modifiers = modifiers.makeRedirectedConstructor();
      }
    }

    // Parse the body.
    DartBlock body = null;
    if (!optional(Token.SEMICOLON)) {
      if (peekPseudoKeyword(0, NATIVE_KEYWORD)) {
        modifiers = modifiers.makeNative();
        body = parseNativeBlock(modifiers);
      } else {
        body = parseFunctionStatementBody(true);
      }
    }

    DartFunction function = doneWithoutConsuming(new DartFunction(arguments, body, returnType));
    return DartMethodDefinition.create(name, function, modifiers, initializers, null);
  }

  private DartBlock parseNativeBlock(Modifiers modifiers) {
    beginNativeBody();
    if (!optionalPseudoKeyword(NATIVE_KEYWORD)) {
      throw new AssertionError();
    }
    if (optional(Token.SEMICOLON)) {
      return done(new DartNativeBlock());
    } else {
      if (!modifiers.isStatic()) {
        reportError(position(), DartCompilerErrorCode.EXPORTED_FUNCTIONS_MUST_BE_STATIC);
      }
      return done(parseFunctionStatementBody(true));
    }
  }

  private DartNode parseMethodOrAccessor(Modifiers modifiers, DartTypeNode returnType) {
    DartMethodDefinition method = done(parseMethod(modifiers, returnType));
    if (method.getModifiers().isGetter() || method.getModifiers().isSetter()) {
      DartField field = new DartField((DartIdentifier) method.getName(),
                                      method.getModifiers().makeAbstractField(), method, null);
      field.setSourceInfo(method);
      DartFieldDefinition fieldDefinition =
        new DartFieldDefinition(null, Lists.<DartField>create(field));
      fieldDefinition.setSourceInfo(field);
      return fieldDefinition;
    }
    return method;
  }

  /**
   * <pre>
   * fieldInitializer
   *     : (THIS '.')? identifier '=' conditionalExpression
   *     | THIS ('.' identifier)? arguments
   *     ;
   * </pre>
   *
   * @return true if initializer is a redirected constructor, false otherwise.
   */
  private boolean parseFieldInitializersOrRedirectedConstructor(List<DartInitializer> inits) {
    do {
      beginFieldInitializerOrRedirectedConstructor();
      boolean hasThisPrefix = optional(Token.THIS);
      if (hasThisPrefix) {
        if (match(Token.LPAREN)) {
          return parseRedirectedConstructorInvocation(null, inits);
        }
        expect(Token.PERIOD);
      }
      DartIdentifier name = parseIdentifier();
      if (hasThisPrefix && match(Token.LPAREN)) {
        return parseRedirectedConstructorInvocation(name, inits);
      } else {
        expect(Token.ASSIGN);
        boolean save = setAllowFunctionExpression(false);
        DartExpression initExpr = parseExpression();
        setAllowFunctionExpression(save);
        inits.add(done(new DartInitializer(name, initExpr)));
      }
    } while (optional(Token.COMMA));
    return false;
  }

  private boolean parseRedirectedConstructorInvocation(DartIdentifier name,
                                                       List<DartInitializer> inits) {
    if (inits.isEmpty()) {
      DartInvocation call =
        doneWithoutConsuming(new DartRedirectConstructorInvocation(name, parseArguments()));
      inits.add(done(new DartInitializer(null, call)));
      return true;
    } else {
      reportUnexpectedToken(position(), Token.ASSIGN, Token.LPAREN);
    }
    return false;
  }

  /**
   * <pre>
   * initializers : ':' superCallOrFirstFieldInitializer (',' fieldInitializer)*
   *              | THIS ('.' identifier) formalParameterList ;
   *
   * fieldInitializer : (THIS '.')? identifier '=' conditionalExpression ;
   *
   * superCallOrFirstFieldInitializer : SUPER arguments | SUPER '.' identifier
   * arguments | fieldInitializer ;
   * <pre>
   *
   * @return true if initializer is a redirect constructor, false otherwise.
   */
  private boolean parseInitializers(List<DartInitializer> initializers) {
    expect(Token.COLON);
    boolean callSuper = false;
    if (match(Token.SUPER)) {
      beginInitializer();
      beginSuperInitializer();
      expect(Token.SUPER);
      callSuper = true;
      DartIdentifier constructor = null;
      if (optional(Token.PERIOD)) {
        // Calling a super named constructor.
        constructor = parseIdentifier();
      }
      DartSuperConstructorInvocation call =
          done(new DartSuperConstructorInvocation(constructor, parseArguments()));
      initializers.add(done(new DartInitializer(null, call)));
    }
    if (!callSuper || optional(Token.COMMA)) {
      return parseFieldInitializersOrRedirectedConstructor(initializers);
    }
    return false;
  }

  /**
   * <pre>
   * variableDeclaration
   *    : constVarOrType identifierList
   *    ;
   * identifierList
   *    : identifier (',' identifier)*
   *    ;
   *
   * staticConstDeclarationList
   *    : staticConstDeclaration (',' staticConstDeclaration)*
   *    ;
   *
   * staticConstDeclaration
   *    : identifier '=' constantExpression
   *    ;
   *
   * // The compile-time expression production is used to mark certain expressions
   * // as only being allowed to hold a compile-time constant. The grammar cannot
   * // express these restrictions, so this will have to be enforced by a separate
   * // analysis phase.
   * constantExpression
   *    : expression
   *    ;
   * </pre>
   */
  private DartFieldDefinition parseFieldDeclaration(Modifiers modifiers, DartTypeNode type) {
    if (isParsingInterface) {
      modifiers = modifiers.makeFinal();
    }
    List<DartField> fields = new ArrayList<DartField>();
    do {
      beginVariableDeclaration();
      DartIdentifier name = parseIdentifier();
      DartExpression value = null;
      if (optional(Token.ASSIGN)) {
        value = parseExpression();
      }
      fields.add(done(new DartField(name, modifiers, null, value)));
    } while (optional(Token.COMMA));
    return done(new DartFieldDefinition(type, fields));
  }

  /**
   * <pre>
   * formalParameterList
   *     : '(' restFormalParameter? ')'
   *     | '(' namedFormalParameters ')'
   *     | '(' legacyNormalFormalParameter normalFormalParameterTail? ')'
   *     ;
   *
   * normalFormalParameterTail
   *     : ',' namedFormalParameters
   *     | ',' restFormalParameter
   *     | ',' legacyNormalFormalParameter normalFormalParameterTail?
   *     ;
   * </pre>
   */
  private List<DartParameter> parseFormalParameterList() {
    beginFormalParameterList();
    List<DartParameter> params = new ArrayList<DartParameter>();
    expect(Token.LPAREN);
    boolean done = optional(Token.RPAREN);
    boolean hasDefaultParameter = false;
    boolean hasNamed = false;
    while (!done) {
      if (!hasNamed && optional(Token.LBRACK)) {
        hasNamed = true;
      }

      DartParameter param = parseFormalParameter(hasNamed);
      params.add(param);

      done = optional(Token.RBRACK);
      if (done) {
        expectCloseParen();
      } else {
        done = optional(Token.RPAREN);
      }

      if (hasDefaultParameter) {
        if (!hasNamed) {
          // TODO(jgw): Add this check, and remove the one below, when we remove default
          //   positional parameters.
          // reportError(position(), DartCompilerErrorCode.DEFAULT_POSITIONAL_PARAMETER);
          if (param.getDefaultExpr() == null) {
            reportError(position(), DartCompilerErrorCode.DEFAULT_PARAMETER_BEFORE_NORMAL_PARAMETER);
          }
        }
      } else {
        hasDefaultParameter = (param.getDefaultExpr() != null);
      }

      if (!done) {
        // Ensure termination if token is anything other than COMMA.
        done = !expect(Token.COMMA);
      }
    }

    return done(params);
  }

  /**
   * <pre>
   * normalFormalParameter
   *     : functionDeclaration
   *     | fieldFormalParameter
   *     | simpleFormalParameter
   *     ;
   *
   * namedFormalParameters
   *     : '[' defaultFormalParameter (',' defaultFormalParameter)* ']'
   *     ;
   *
   * defaultFormalParameter
   *     : normalFormalParameter ('=' constantExpression)?
   *     ;
   * </pre>
   */
  private DartParameter parseFormalParameter(boolean isNamed) {
    beginFormalParameter();
    DartExpression paramName = null;
    DartTypeNode type = null;
    DartExpression initExpr = null;
    List<DartParameter> functionParams = null;
    boolean hasVar = false;
    Modifiers modifiers = Modifiers.NONE;

    if (isNamed) {
      modifiers = modifiers.makeNamed();
    }

    if (optional(Token.FINAL)) {
      modifiers = modifiers.makeFinal();
    } else if (optional(Token.VAR)) {
      hasVar = true;
    }

    boolean isVoidType = false;
    if (!hasVar) {
      isVoidType = (peek(0) == Token.VOID);
      if (isVoidType) {
        type = parseVoidType();
      } else if ((peek(0) != Token.ELLIPSIS)
                 && (peek(1) != Token.COMMA)
                 && (peek(1) != Token.RPAREN)
                 && (peek(1) != Token.RBRACK)
                 && (peek(1) != Token.ASSIGN)
                 && (peek(1) != Token.LPAREN)
                 && (peek(0) != Token.THIS)) {
        // Must be a type specification.
        type = parseTypeAnnotation();
      }
    }

    paramName = parseParameterName();

    if (peek(0) == Token.LPAREN) {
      // Function parameter.
      if (modifiers.isFinal()) {
        reportError(position(), DartCompilerErrorCode.FUNCTION_TYPED_PARAMETER_IS_FINAL);
      }
      if (hasVar) {
        reportError(position(), DartCompilerErrorCode.FUNCTION_TYPED_PARAMETER_IS_VAR);
      }
      functionParams = parseFormalParameterList();
    } else {
      // Not a function parameter.
      if (isVoidType) {
        reportError(type, DartCompilerErrorCode.VOID_PARAMETER);
      }
    }

    // Look for an initialization expression
    switch (peek(0)) {
      case COMMA:
      case RPAREN:
      case RBRACK:
        // It is a simple parameter.
        break;

      case ASSIGN:
        // Default parameter.

        // TODO(jgw): This makes legacy default parameters implicitly named, which will ease the
        // transition. Remove this as soon as positional default parameters are removed.
        modifiers = modifiers.makeNamed();

        consume(Token.ASSIGN);
        initExpr = parseExpression();

        break;

      default:
        reportUnexpectedToken(position(), null, peek(0));
        break;
    }

    return done(new DartParameter(paramName, type, functionParams, initExpr, modifiers));
  }

  /**
   * <pre>
   * simpleFormalParameter
   *     : declaredIdentifier
   *     | identifier
   *     ;
   *
   * fieldFormalParameter
   *    : finalVarOrType? THIS '.' identifier
   *    ;
   * </pre>
   */
  private DartExpression parseParameterName() {
    beginParameterName();
    if (optional(Token.THIS)) {
      beginThisExpression();
      expect(Token.PERIOD);
      return done(new DartPropertyAccess(done(DartThisExpression.get()), parseIdentifier()));
    }
    return done(parseIdentifier());
  }

  /**
   * Parse an expression.
   *
   * <pre>
   * expression
   *     : assignableExpression assignmentOperator expression
   *     | conditionalExpression
   *     ;
   *
   * assignableExpression
   *     : primary (arguments* assignableSelector)+
   *     | SUPER assignableSelector
   *     | identifier
   *     ;
   * </pre>
   *
   * @return an expression matching the {@code expression} production above
   */
  @VisibleForTesting
  public DartExpression parseExpression() {
    beginExpression();
    DartExpression result = parseConditionalExpression();
    Token token = peek(0);
    if (token.isAssignmentOperator()) {
      ensureAssignable(result);
      consume(token);
      result = done(new DartBinaryExpression(token, result, parseExpression()));
    } else {
      done(null);
    }
    return result;
  }

  /**
   * expressionList
   *     : expression (',' expression)*
   *     ;
   */
  private DartExpression parseExpressionList() {
    beginExpressionList();
    DartExpression result = parseExpression();
    while (optional(Token.COMMA)) {
      result = new DartBinaryExpression(Token.COMMA, result, parseExpression());
      if (match(Token.COMMA)) {
        result = doneWithoutConsuming(result);
      }
    }
    return done(result);
  }


  /**
   * Parse a binary expression.
   *
   * <pre>
   * logicalOrExpression
   *     : logicalAndExpression ('||' logicalAndExpression)*
   *     ;
   *
   * logicalAndExpression
   *     : bitwiseOrExpression ('&&' bitwiseOrExpression)*
   *     ;
   *
   * bitwiseOrExpression
   *     : bitwiseXorExpression ('|' bitwiseXorExpression)*
   *     ;
   *
   * bitwiseXorExpression
   *     : bitwiseAndExpression ('^' bitwiseAndExpression)*
   *     ;
   *
   * bitwiseAndExpression
   *     : equalityExpression ('&' equalityExpression)*
   *     ;
   *
   * equalityExpression
   *     : relationalExpression (equalityOperator relationalExpression)?
   *     ;
   *
   * relationalExpression
   *     : shiftExpression (isOperator type | relationalOperator shiftExpression)?
   *     ;
   *
   * shiftExpression
   *     : additiveExpression (shiftOperator additiveExpression)*
   *     ;
   *
   * additiveExpression
   *     : multiplicativeExpression (additiveOperator multiplicativeExpression)*
   *     ;
   *
   * multiplicativeExpression
   *     : unaryExpression (multiplicativeOperator unaryExpression)*
   *     ;
   * </pre>
   *
   * @return an expression matching one of the productions above
   */
  private DartExpression parseBinaryExpression(int precedence) {
    assert (precedence >= 4);
    beginBinaryExpression();
    DartExpression result = parseUnaryExpression();
    for (int level = peek(0).getPrecedence(); level >= precedence; level--) {
      while (peek(0).getPrecedence() == level) {
        Token token = next();
        DartExpression right;
        if (token == Token.IS) {
          beginTypeExpression();
          if (optional(Token.NOT)) {
            beginTypeExpression();
            DartTypeExpression typeExpression = done(new DartTypeExpression(parseTypeAnnotation()));
            right = done(new DartUnaryExpression(Token.NOT, typeExpression, true));
          } else {
            right = done(new DartTypeExpression(parseTypeAnnotation()));
          }
        } else {
          right = parseBinaryExpression(level + 1);
        }
        result = doneWithoutConsuming(new DartBinaryExpression(token, result, right));
        if ((token == Token.IS)
            || token.isRelationalOperator()
            || token.isEqualityOperator()) {
          // The operations cannot be chained.
          if (match(token)) {
            reportError(position(), DartCompilerErrorCode.INVALID_OPERATOR_CHAINING,
              token.toString().toLowerCase());
          }
          break;
        }
      }
    }
    done(null);
    return result;
  }

  /**
   * Parse the arguments passed to a function or method invocation.
   *
   * <pre>
   * arguments
   *    : '(' argumentList? ')'
   *    ;
   *
   * argumentList
   *    : expression (',' expression)* (',' spreadArgument)?
   *    | spreadArgument
   *    ;
   *
   * spreadArgument
   *    : '...' expression
   *    ;
   * </pre>
   *
   * @return a list of expressions containing the arguments to be passed
   */
  private List<DartExpression> parseArguments() {
    List<DartExpression> arguments = new ArrayList<DartExpression>();
    expect(Token.LPAREN);
    // SEMICOLON is for error recovery
    while (!match(Token.RPAREN) && !match(Token.EOS) && !match(Token.SEMICOLON)) {
      beginParameter();
      DartExpression expression;
      if (peek(1) == Token.COLON) {
        DartIdentifier name = parseIdentifier();
        expect(Token.COLON);
        expression = new DartNamedExpression(name, parseExpression());
      } else {
        expression = parseExpression();
      }
      arguments.add(done(expression));
      switch(peek(0)) {
        case COMMA:
          consume(Token.COMMA);
          break;
        case RPAREN:
          break;
        default:
          // Make sure that the parser's state is advanced.
          Token actual = peek(0);
          ctx.advance();
          reportError(ctx.getTokenLocation().getEnd(),
              DartCompilerErrorCode.EXPECTED_COMMA_OR_RIGHT_PAREN, actual);
          break;
      }
    }
    expectCloseParen();
    return arguments;
  }

  /**
   * Parse a conditional expression.
   *
   * <pre>
   * conditionalExpression
   *     : logicalOrExpression ('?' expression ':' expression)?
   *     ;
   * </pre>
   *
   * @return an expression matching the {@code conditionalExpression} production
   */
  private DartExpression parseConditionalExpression() {
    beginConditionalExpression();
    DartExpression result = parseBinaryExpression(4);
    if (peek(0) != Token.CONDITIONAL) {
      return done(result);
    }
    consume(Token.CONDITIONAL);
    DartExpression yes = parseExpression();
    expect(Token.COLON);
    DartExpression no = parseExpression();
    return done(new DartConditional(result, yes, no));
  }

  private DartExpression parseString() {
    switch(peek(0)) {
      case STRING:
        return parseLiteral();
      case STRING_SEGMENT:
      case STRING_EMBED_EXP_START:
        return parseStringInterpolation();
      default:
        DartExpression expression = parseExpression();
        reportError(position(), DartCompilerErrorCode.EXPECTED_STRING_LITERAL);
        return expression;
    }
  }

  /**
   * Parse any literal that is not a function literal (those have already been
   * handled before this method is called, so we don't need to handle them
   * here).
   *
   * <pre>
   * nonFunctionLiteral
   *   : NULL
   *   | TRUE
   *   | FALSE
   *   | HEX_NUMBER
   *   | RATIONAL_NUMBER
   *   | DOUBLE_NUMBER
   *   | STRING
   *   | mapLiteral
   *   | arrayLiteral
   *   ;
   * </pre>
   *
   * @return an expression matching the {@code literal} production above
   */
  private DartExpression parseLiteral() {
    beginLiteral();
    switch (peek(0)) {
      case NULL_LITERAL: {
        consume(Token.NULL_LITERAL);
        return done(DartNullLiteral.get());
      }

      case TRUE_LITERAL: {
        consume(Token.TRUE_LITERAL);
        return done(DartBooleanLiteral.get(true));
      }

      case FALSE_LITERAL: {
        consume(Token.FALSE_LITERAL);
        return done(DartBooleanLiteral.get(false));
      }

      case INTEGER_LITERAL: {
        consume(Token.INTEGER_LITERAL);
        String number = ctx.getTokenString();
        return done(DartIntegerLiteral.get(new BigInteger(number)));
      }

      case DOUBLE_LITERAL: {
        consume(Token.DOUBLE_LITERAL);
        String number = ctx.getTokenString();
        return done(DartDoubleLiteral.get(Double.parseDouble(number)));
      }

      case HEX_LITERAL: {
        consume(Token.HEX_LITERAL);
        String number = ctx.getTokenString();
        return done(DartIntegerLiteral.get(new BigInteger(number, 16)));
      }

      case STRING: {
        consume(Token.STRING);
        return done(DartStringLiteral.get(ctx.getTokenString()));
      }

      case LBRACE: {
        return done(parseMapLiteral(false, null));
      }

      case INDEX: {
        expect(peek(0));
        return done(new DartArrayLiteral(false, null, new ArrayList<DartExpression>()));
      }

      case LBRACK: {
        return done(parseArrayLiteral(false, null));
      }

      case VOID:
        // For better error recovery / code completion in the IDE, treat "void" as an identifier
        // here and let it get reported as a resolution error.
      case IDENTIFIER: {
        return done(parseIdentifier());
      }

      case SEMICOLON: {
        // this is separate from the default case for better error recovery,
        // leaving the semicolon for the caller to use for a statement boundary

        // we have to advance to get the proper position, but we want to leave
        // the semicolon
        startLookahead();
        next();
        reportUnexpectedToken(position(), null, Token.SEMICOLON);
        rollback();
        return done(new DartSyntheticErrorExpression(""));
      }

      default: {
        Token unexpected = peek(0);
        String unexpectedString = ctx.getTokenString();
        if (unexpectedString == null && unexpected != Token.EOS) {
          unexpectedString = unexpected.getSyntax();
        }
        next();
        reportUnexpectedToken(position(), null, unexpected);
        StringBuilder tokenStr = new StringBuilder();
        if (unexpectedString != null) {
          tokenStr.append(unexpectedString);
        }
        // TODO(jat): should we eat additional tokens here for error recovery?
        return done(new DartSyntheticErrorExpression(tokenStr.toString()));
      }
    }
  }

  /**
   * <pre>
   * mapLiteral
   *     : '{' (mapLiteralEntry (',' mapLiteralEntry)* ','?)? '}'
   *     ;
   * </pre>
   */
  private DartExpression parseMapLiteral(boolean isConst, List<DartTypeNode> typeArguments) {
    beginMapLiteral();
    expect(Token.LBRACE);
    boolean save = setAllowFunctionExpression(true);
    List<DartMapLiteralEntry> entries = new ArrayList<DartMapLiteralEntry>();

    while (!match(Token.RBRACE) && !match(Token.EOS)) {
      DartMapLiteralEntry entry = parseMapLiteralEntry();
      if (entry != null) {
        entries.add(entry);
      }
      switch(peek(0)) {
        case COMMA:
          consume(Token.COMMA);
          break;
        case RBRACE:
          break;
        default:
          if (entry == null) {
            // Ensure the parser makes progress.
            ctx.advance();
          }
          reportError(position(), DartCompilerErrorCode.EXPECTED_COMMA_OR_RIGHT_BRACE);
          break;
      }
    }

    expectCloseBrace();
    setAllowFunctionExpression(save);
    return done(new DartMapLiteral(isConst, typeArguments, entries));
  }

  /**
   * mapLiteralEntry
   *     : STRING ':' expression
   *     ;
   */
  private DartMapLiteralEntry parseMapLiteralEntry() {
    beginMapLiteralEntry();
    // Parse the key.
    DartExpression keyExpr = parseString();
    if (keyExpr == null) {
      return done(null);
    }
    // Parse the value.
    DartExpression value;
    if (expect(Token.COLON)) {
      value = parseExpression();
    } else {
      value = doneWithoutConsuming(DartNullLiteral.get());
    }
    return done(new DartMapLiteralEntry(keyExpr, value));
  }

  /**
   * // The array literal syntax doesn't allow elided elements, unlike
   * // in ECMAScript.
   *
   * <pre>
   * arrayLiteral
   *     : '[' expressionList? ']'
   *     ;
   * </pre>
   */
  private DartExpression parseArrayLiteral(boolean isConst, List<DartTypeNode> typeArguments) {
    beginArrayLiteral();
    expect(Token.LBRACK);
    boolean save = setAllowFunctionExpression(true);
    List<DartExpression> exprs = new ArrayList<DartExpression>();
    while (!match(Token.RBRACK) && !EOS()) {
      exprs.add(parseExpression());
      if (!optional(Token.COMMA)) {
        break;
      }
    }
    expect(Token.RBRACK);
    setAllowFunctionExpression(save);
    return done(new DartArrayLiteral(isConst, typeArguments, exprs));
  }

  /**
   * Parse a postfix expression.
   *
   * <pre>
   * postfixExpression
   *     | assignableExpression postfixOperator
   *     : primary selector*
   *     ;
   * </pre>
   *
   * @return an expression matching the {@code postfixExpression} production above
   */
  private DartExpression parsePostfixExpression() {
    beginPostfixExpression();
    DartExpression receiver = doneWithoutConsuming(parsePrimaryExpression());
    DartExpression result = receiver;
    do {
      receiver = result;
      result = doneWithoutConsuming(parseSelectorExpression(receiver));
    } while (receiver != result);

    Token token = peek(0);
    if (token.isCountOperator()) {
      ensureAssignable(result);
      consume(token);
      result = doneWithoutConsuming(new DartUnaryExpression(token, result, false));
    }

    return done(result);
  }

  /**
   * <pre>
   * typeParameters? (arrayLiteral | mapLiteral)
   * </pre>
   *
   * @param isConst <code>true</code> if a CONST expression
   *
   */
  private DartExpression tryParseTypedCompoundLiteral(boolean isConst) {
    beginLiteral();
    List<DartTypeNode> typeArguments = parseTypeArgumentsOpt();
    switch (peek(0)) {
      case INDEX:
        beginArrayLiteral();
        consume(Token.INDEX);
        return done(done(new DartArrayLiteral(isConst, null, new ArrayList<DartExpression>())));
      case LBRACK:
        return done(parseArrayLiteral(isConst, typeArguments));
      case LBRACE:
        return done(parseMapLiteral(isConst, typeArguments));
      default:
        if (typeArguments != null) {
          rollback();
          return null;
        }

    }
    // Doesn't look like a typed compound literal and no tokens consumed.
    return done(null);
  }

  /**
   * <pre>
   * string-interpolation
   *   : (STRING_SEGMENT? embedded-exp?)* STRING_LAST_SEGMENT
   *
   * embedded-exp
   *   : STRING_EMBED_EXP_START expression STRING_EMBED_EXP_END
   * </pre>
   */
  private DartExpression parseStringInterpolation() {
    // TODO(sigmund): generalize to parse string templates as well.
    if (peek(0) == Token.STRING_LAST_SEGMENT) {
      throw new InternalCompilerException("Invariant broken");
    }
    beginStringInterpolation();
    List<DartStringLiteral> strings = new ArrayList<DartStringLiteral>();
    List<DartExpression> expressions = new ArrayList<DartExpression>();

    boolean inString = true;
    while (inString) { // Iterate until we find the last string segment.
      switch (peek(0)) {
        case STRING_SEGMENT: {
          assert strings.size() == expressions.size() : "Invariant broken";
          beginStringSegment();
          consume(Token.STRING_SEGMENT);
          strings.add(done(DartStringLiteral.get(ctx.getTokenString())));
          break;
        }
        case STRING_LAST_SEGMENT: {
          assert strings.size() == expressions.size() : "Invariant broken";
          beginStringSegment();
          consume(Token.STRING_LAST_SEGMENT);
          strings.add(done(DartStringLiteral.get(ctx.getTokenString())));
          inString = false;
          break;
        }
        case STRING_EMBED_EXP_START: {
          consume(Token.STRING_EMBED_EXP_START);
          if (strings.size() == expressions.size()) {
            // Ensure that strings and expressions are alternating, add empty
            // strings if we see 2 consecutive expressions.
            beginStringSegment();
            strings.add(done(DartStringLiteral.get("")));
          }
          /*
           * We check for ILLEGAL specifically here to give nicer error
           * messages, and because the scanner doesn't generate a
           * STRING_EMBED_EXP_END to match the START in the case of an ILLEGAL
           * token.
           */
          if (peek(0) == Token.ILLEGAL) {
            reportError(position(), DartCompilerErrorCode.UNEXPECTED_TOKEN_IN_STRING_INTERPOLATION,
                next());
            expressions.add(new DartSyntheticErrorExpression(ctx.getTokenString()));
            break;
          } else {
            DartExpression expr = parseExpression();
            expressions.add(expr);
          }
          if (!expect(Token.STRING_EMBED_EXP_END)) {
            return done(new DartSyntheticErrorExpression());
          }
          break;
        }
        case EOS: {
          reportError(position(), DartCompilerErrorCode.INCOMPLETE_STRING_LITERAL);
          return done(null);
        }
        default: {
          reportError(position(), DartCompilerErrorCode.UNEXPECTED_TOKEN_IN_STRING_INTERPOLATION,
              next());
          break;
        }
      }
    }
    assert (strings.size() == expressions.size() + 1) : "Invariant broken";
    return done(new DartStringInterpolation(strings, expressions));
  }

  /**
   * Parse a return type, giving an error if the .
   *
   * @return a return type or null if the current text is not a return type
   */
  private DartTypeNode parseReturnType() {
    if (peek(0) == Token.VOID) {
      return parseVoidType();
    } else {
      return parseTypeAnnotation();
    }
  }

  /**
   * Check if the current text could be a return type, and advance past it if so.  The current
   * position is unchanged if it is not a return type.
   *
   * NOTE: if the grammar is changed for what constitutes an acceptable return type, this method
   * must be updated to match {@link #parseReturnType()}/etc.
   *
   * @return true if current text could be a return type, false otherwise
   */
  private boolean isReturnType() {
    beginReturnType();
    if (optional(Token.VOID)) {
      done(null);
      return true;
    }
    if (!optional(Token.IDENTIFIER)) {
      rollback();
      return false;
    }
    // handle prefixed identifiers
    if (optional(Token.PERIOD)) {
      if (!optional(Token.IDENTIFIER)) {
        rollback();
        return false;
      }
    }
    // skip over type arguments if they are present
    if (optional(Token.LT)) {
      int count = 1;
      while (count > 0) {
        switch (next()) {
          case EOS:
            rollback();
            return false;
          case LT:
            count++;
            break;
          case GT:
            count--;
            break;
          case SHL:
            count += 2;
            break;
          case SHR:  // >>>
            count -= 3;
            break;
          case SAR:  // >>
            count -= 2;
            break;
          case COMMA:
          case IDENTIFIER:
            // extends is a pseudokeyword, so shows up as IDENTIFIER
            break;
          default:
            rollback();
            return false;
        }
      }
      if (count < 0) {
        // if we had too many > (which can only be >> or >>>), can't be a return type
        rollback();
        return false;
      }
    }
    done(null);
    return true;
  }

  /**
   * Checks to see if the current text looks like a function expression:
   *
   * <pre>
   *   FUNCTION name? ( args ) < => | { >
   *   returnType name? ( args ) < => | { >
   *   name? ( args ) < => | { >
   * </pre>
   *
   * The current position is unchanged on return.
   *
   * NOTE: if the grammar for function expressions changes, this method must be
   * adapted to match the actual parsing code. It is acceptable for this method
   * to return true when the source text does not actually represent a function
   * expression (which would result in error messages assuming it was a function
   * expression, but it must not do so when the source text would be correct if
   * parsed as a non-function expression.
   *
   * @return true if the current text looks like a function expression, false
   *         otherwise
   */
  @VisibleForTesting
  boolean looksLikeFunctionExpression() {
    if (!allowFunctionExpression) {
      return false;
    }
    return looksLikeFunctionDeclarationOrExpression();
  }

  /**
   * Check to see if the following tokens could be a function expression, and if so try and parse
   * it as one.
   *
   * @return a function expression if found, or null (with no tokens consumed) if not
   */
  private DartExpression parseFunctionExpressionWithReturnType() {
    beginFunctionLiteral();
    DartIdentifier[] namePtr = new DartIdentifier[1];
    DartFunction function = parseFunctionDeclarationOrExpression(namePtr, false);
    if (function == null) {
      rollback();
      return null;
    }
    return done(new DartFunctionExpression(namePtr[0], doneWithoutConsuming(function), false));
  }

  /**
   * Parse a function declaration or expression, including the body.
   * <pre>
   *     ... | functionDeclaration functionBody
   *
   * functionDeclaration
   *    : returnType? identifier formalParameterList
   *    ;
   *
   * functionExpression
   *    : (returnType? identifier)? formalParameterList functionExpressionBody
   *    ;
   *
   * functionBody
   *    : '=>' expression ';'
   *    | block
   *    ;
   *
   * functionExpressionBody
   *    : '=>' expression
   *    | block
   *    ;
   * </pre>
   *
   * @param namePtr out parameter - parsed function name stored in namePtr[0]
   * @param isDeclaration true if this is a declaration (ie, a name is required and a trailing
   *     semicolon is needed for arrow syntax
   * @return a {@link DartFunction} containing the body of the function, or null
   *     if the next tokens cannot be parsed as a function declaration or expression
   */
  private DartFunction parseFunctionDeclarationOrExpression(DartIdentifier[] namePtr,
        boolean isDeclaration) {
    DartTypeNode returnType = null;
    namePtr[0] = null;
    switch (peek(0)) {
      case LPAREN:
        // no type or name, just the formal parameter list
        break;
      case IDENTIFIER:
        if (peek(1) == Token.LPAREN) {
          // if there is only one identifier, it must be the name
          namePtr[0] = parseIdentifier();
          break;
        }
        //$FALL-THROUGH$
      case VOID:
        returnType = parseReturnType();
        if (peek(0) == Token.IDENTIFIER) {
          namePtr[0] = parseIdentifier();
        }
        break;
      default:
        return null;
    }
    List<DartParameter> params = parseFormalParameterList();
    DartBlock body = parseFunctionStatementBody(isDeclaration);
    DartFunction function = new DartFunction(params, body, returnType);
    if (isDeclaration && namePtr[0] == null) {
      reportError(function, DartCompilerErrorCode.MISSING_FUNCTION_NAME);
    }
    return function;
  }

  /**
   * Parse a primary expression.
   *
   * <pre>
   * primary
   *   : THIS
   *   | SUPER assignableSelector
   *   | literal
   *   | identifier
   *   | NEW type ('.' identifier)? arguments
   *   | typeArguments? (arrayLiteral | mapLiteral)
   *   | CONST typeArguments? (arrayLiteral | mapLiteral)
   *   | CONST typeArguments? (arrayLiteral | mapLiteral)
   *   | CONST type ('.' identifier)? arguments
   *   | '(' expression ')'
   *   | string-interpolation
   *   | functionExpression
   *   ;
   * </pre>
   *
   * @return an expression matching the {@code primary} production above
   */
  private DartExpression parsePrimaryExpression() {
    if (looksLikeFunctionExpression()) {
      return parseFunctionExpressionWithReturnType();
    }
    switch (peek(0)) {
      case THIS: {
        beginThisExpression();
        consume(Token.THIS);
        return done(DartThisExpression.get());
      }

      case SUPER: {
        beginSuperExpression();
        consume(Token.SUPER);
        return done(parseAssignableSelector(doneWithoutConsuming(DartSuperExpression.get())));
      }

      case NEW: {
        beginNewExpression(); // DartNewExpression
        consume(Token.NEW);
        return done(parseConstructor(false));
      }

      case CONST: {
        beginConstExpression();
        consume(Token.CONST);

        DartExpression literal = tryParseTypedCompoundLiteral(true);
        if (literal != null) {
          return done(literal);
        }
        return done(parseConstructor(true));
      }

      case LPAREN: {
        beginParenthesizedExpression();
        consume(Token.LPAREN);
        beginExpression();
        // inside parens, function blocks are allowed again
        boolean save = setAllowFunctionExpression(true);
        DartExpression expression = done(parseExpression());
        setAllowFunctionExpression(save);
        expectCloseParen();
        return done(new DartParenthesizedExpression(expression));
      }

      case LT: {
        beginLiteral();
        DartExpression literal = tryParseTypedCompoundLiteral(false);
        if (literal == null) {
          reportError(position(), DartCompilerErrorCode.EXPECTED_ARRAY_OR_MAP_LITERAL);
        }
        return done(literal);
      }

      case STRING_SEGMENT:
      case STRING_LAST_SEGMENT:
      case STRING_EMBED_EXP_START: {
        return parseStringInterpolation();
      }

      default: {
        return parseLiteral();
      }
    }
  }

  private DartExpression parseConstructor(boolean isConst) {
    List<DartTypeNode> parts = new ArrayList<DartTypeNode>();
    beginConstructor();
    do {
      parts.add(new DartTypeNode(parseIdentifier(), parseTypeArgumentsOpt()));
    } while (optional(Token.PERIOD));
    assert parts.size() > 0;

    DartNode constructor;
    switch (parts.size()) {
      case 1:
        constructor = doneWithoutConsuming(parts.get(0));
        break;

      case 2: {
        // This case is ambiguous. It can either be prefix.Type or
        // Type.namedConstructor.
        boolean hasPrefix = false;
        DartTypeNode part1 = parts.get(0);
        DartTypeNode part2 = parts.get(1);
        if (prefixes.contains(((DartIdentifier) part1.getIdentifier()).getTargetName())) {
          hasPrefix = true;
        }
        if (!part2.getTypeArguments().isEmpty()) {
          // If the second part has type arguments, the first part must be a prefix.
          // If it isn't a prefix, the resolver will complain.
          hasPrefix = true;
        }
        if (hasPrefix) {
          constructor = doneWithoutConsuming(toPrefixedType(parts));
        } else {
          // Named constructor.
          DartIdentifier identifier = ensureIdentifier(part2);
          constructor = doneWithoutConsuming(new DartPropertyAccess(doneWithoutConsuming(part1),
                                                                    identifier));
        }
        break;
      }
      default: {
        // This case is unambiguous. It must be prefix.Type.namedConstructor.
        if (parts.size() > 3) {
          reportError(parts.get(3), DartCompilerErrorCode.EXPECTED_LEFT_PAREN);
        }
        DartTypeNode typeNode = doneWithoutConsuming(toPrefixedType(parts));
        DartIdentifier identifier = ensureIdentifier(parts.get(2));
        constructor = doneWithoutConsuming(new DartPropertyAccess(typeNode, identifier));
        break;
      }
    }

    return done(new DartNewExpression(constructor, parseArguments(), isConst));
  }

  private DartIdentifier ensureIdentifier(DartTypeNode node) {
    List<DartTypeNode> typeArguments = node.getTypeArguments();
    if (!typeArguments.isEmpty()) {
      reportError(typeArguments.get(0), DartCompilerErrorCode.UNEXPECTED_TYPE_ARGUMENT);
    }
    return (DartIdentifier) node.getIdentifier();
  }

  private DartTypeNode toPrefixedType(List<DartTypeNode> parts) {
    DartIdentifier part1 = ensureIdentifier(parts.get(0));
    DartTypeNode part2 = parts.get(1);
    DartIdentifier identifier = (DartIdentifier) part2.getIdentifier();
    DartPropertyAccess access = doneWithoutConsuming(new DartPropertyAccess(part1, identifier));
    return new DartTypeNode(access, part2.getTypeArguments());
  }

  /**
   * Parse a selector expression.
   *
   * <pre>
   * selector
   *    : assignableSelector
   *    | arguments
   *    ;
   * </pre>
   *
   * @return an expression matching the {@code selector} production above
   */
  private DartExpression parseSelectorExpression(DartExpression receiver) {
    DartExpression expression = tryParseAssignableSelector(receiver);
    if (expression != null) {
      return expression;
    }

    if (peek(0) == Token.LPAREN) {
      beginSelectorExpression();
      boolean save = setAllowFunctionExpression(true);
      List<DartExpression> args = parseArguments();
      setAllowFunctionExpression(save);
      if (receiver instanceof DartIdentifier) {
        return(done(new DartUnqualifiedInvocation((DartIdentifier) receiver, args)));
      } else {
        return(done(new DartFunctionObjectInvocation(receiver, args)));
      }
    }

    return receiver;
  }

  /**
   * <pre>
   * assignableSelector
   *    : '[' expression ']'
   *    | '.' identifier
   *    ;
   * </pre>
   */
  private DartExpression tryParseAssignableSelector(DartExpression receiver) {
    switch (peek(0)) {
      case PERIOD:
        consume(Token.PERIOD);
        switch (peek(0)) {
          case SEMICOLON:
          case RBRACE:
            reportError(position(), DartCompilerErrorCode.EXPECTED_IDENTIFIER);
            DartIdentifier error = doneWithoutConsuming(new DartIdentifier(""));
            return doneWithoutConsuming(new DartPropertyAccess(receiver, error));
        }
        DartIdentifier name = parseIdentifier();
        if (peek(0) == Token.LPAREN) {
          boolean save = setAllowFunctionExpression(true);
          DartMethodInvocation expr = doneWithoutConsuming(new DartMethodInvocation(receiver, name,
              parseArguments()));
          setAllowFunctionExpression(save);
          return expr;
        } else {
          return doneWithoutConsuming(new DartPropertyAccess(receiver, name));
        }

      case LBRACK:
        consume(Token.LBRACK);
        DartExpression key = parseExpression();
        expect(Token.RBRACK);
        return doneWithoutConsuming(new DartArrayAccess(receiver, key));

      default:
        return null;
    }
  }

  private DartExpression parseAssignableSelector(DartExpression receiver) {
    DartExpression expression = tryParseAssignableSelector(receiver);
    if (expression == null) {
      reportError(position(), DartCompilerErrorCode.EXPECTED_PERIOD_OR_LEFT_BRACKET);
    }
    return expression;
  }

  /**
   * <pre>
   * block
   *     : '{' statements deadCode* '}'
   *     ;
   *
   * statements
   *     : statement*
   *     ;
   *
   * deadCode
   *     : (normalCompletingStatement | abruptCompletingStatement)
   *     ;
   * </pre>
   */
  private DartBlock parseBlock() {
    if (isDietParse) {
      expect(Token.LBRACE);
      DartBlock emptyBlock = new DartBlock(new ArrayList<DartStatement>());
      int nesting = 1;
      while (nesting > 0) {
        Token token = next();
        switch (token) {
          case LBRACE:
            ++nesting;
            break;
          case RBRACE:
            --nesting;
            break;
          case EOS:
            return emptyBlock;
        }
      }
      // Return an empty block so we don't generate unparseable code.
      return emptyBlock;
    } else {
      beginBlock();
      List<DartStatement> statements = new ArrayList<DartStatement>();
      expect(Token.LBRACE);
      while (!match(Token.RBRACE) && !EOS()) {
        DartStatement newStatement = parseStatement();
        if (newStatement == null) {
          break;
        }
        statements.add(newStatement);
      }
      expectCloseBrace();
      return done(new DartBlock(statements));
    }
  }

  /**
   * Parse a function statement body.
   *
   * <pre>
   * functionStatementBody
   *    : '=>' expression ';'
   *    | block
   * </pre>
   *
   * @param requireSemicolonForArrow true if a semicolon is required after an arrow expression
   * @return {@link DartBlock} instance containing function body
   */
  private DartBlock parseFunctionStatementBody(boolean requireSemicolonForArrow) {
    if (isDietParse) {
      expect(Token.LBRACE);
      DartBlock emptyBlock = new DartBlock(new ArrayList<DartStatement>());
      int nesting = 1;
      while (nesting > 0) {
        Token token = next();
        switch (token) {
          case LBRACE:
            ++nesting;
            break;
          case RBRACE:
            --nesting;
            break;
          case EOS:
            return emptyBlock;
        }
      }
      // Return an empty block so we don't generate unparseable code.
      return emptyBlock;
    } else {
      beginFunctionStatementBody();
      if (optional(Token.ARROW)) {
        DartExpression expr = parseExpression();
        if (requireSemicolonForArrow) {
          expect(Token.SEMICOLON);
        }
        return done(makeReturnBlock(expr));
      } else {
        return done(parseBlock());
      }
    }
  }

  /**
   * Create a block containing a single return statement.
   *
   * @param returnVal return value expression
   * @return block containing a single return statement
   */
  private DartBlock makeReturnBlock(DartExpression returnVal) {
    // TODO(jat): consider making a different AST node to represent this
    List<DartStatement> statements = new ArrayList<DartStatement>();
    statements.add(new DartReturnStatement(returnVal));
    return new DartBlock(statements);
  }

  /**
   * <pre>
   * initializedVariableDeclaration
   *     : constVarOrType initializedIdentifierList
   *     ;
   *
   * initializedIdentifierList
   *     : initializedIdentifier (',' initializedIdentifier)*
   *     ;
   *
   * initializedIdentifier
   *     : IDENTIFIER ('=' assignmentExpression)?
   *     ;
   *  </pre>
   */
  private List<DartVariable> parseInitializedVariableList() {
    List<DartVariable> idents = new ArrayList<DartVariable>();
    do {
      beginVariableDeclaration();
      DartIdentifier name = parseIdentifier();
      DartExpression value = null;
      if (isParsingInterface) {
        expect(Token.ASSIGN);
        value = parseExpression();
      } else if (optional(Token.ASSIGN)) {
        value = parseExpression();
      }
      idents.add(done(new DartVariable(name, value)));
    } while (optional(Token.COMMA));

    return idents;
  }

  /**
   * <pre>
   * abruptCompletingStatement
   *     : BREAK identifier? ';'
   *     | CONTINUE identifier? ';'
   *     | RETURN expression? ';'
   *     | THROW expression? ';'
   *     ;
   *  </pre>
   */
  private DartBreakStatement parseBreakStatement() {
    beginBreakStatement();
    expect(Token.BREAK);
    DartIdentifier label = null;
    if (match(Token.IDENTIFIER)) {
      label = parseIdentifier();
    }
    expectStatmentTerminator();
    return done(new DartBreakStatement(label));
  }

  private DartContinueStatement parseContinueStatement() {
    beginContinueStatement();
    expect(Token.CONTINUE);
    DartIdentifier label = null;
    if (peek(0) == Token.IDENTIFIER) {
      label = parseIdentifier();
    }
    expectStatmentTerminator();
    return done(new DartContinueStatement(label));
  }

  private DartReturnStatement parseReturnStatement() {
    beginReturnStatement();
    expect(Token.RETURN);
    DartExpression value = null;
    if (peek(0) != Token.SEMICOLON) {
      value = parseExpression();
    }
    expectStatmentTerminator();
    return done(new DartReturnStatement(value));
  }

  private DartThrowStatement parseThrowStatement() {
    beginThrowStatement();
    expect(Token.THROW);
    DartExpression exception = null;
    if (peek(0) != Token.SEMICOLON) {
      exception = parseExpression();
    }
    expectStatmentTerminator();
    return done(new DartThrowStatement(exception));
  }

  /**
   * <pre>
   * statement
   *     : label* nonLabelledStatement
   *     ;
   *
   * label
   *     : identifier ':'
   *     ;
   * </pre>
   *
   * @return a {@link DartStatement}
   */
  @VisibleForTesting
  public DartStatement parseStatement() {
    if (peek(0) == Token.IDENTIFIER && peek(1) == Token.COLON) {
      beginLabel();
      DartIdentifier label = parseIdentifier();
      expect(Token.COLON);
      DartStatement statement = parseNonLabelledStatement();
      return done(new DartLabel(label, statement));
    }
    return parseNonLabelledStatement();
  }

  /**
   * <pre>
   * normalCompletingStatement
   *     : functionStatement
   *     | initializedVariableDeclaration ';'
   *     | simpleStatement
   *     ;
   *
   * functionStatement
   *     : typeOrFunction identifier formalParameterList block
   *     ;
   *     ;
   *
   * simpleStatement
   *     : ('{')=> block // Guard to break tie with map literal.
   *     | expression? ';'
   *     | tryStatement
   *     | ASSERT '(' conditionalExpression ')' ';'
   *     | abruptCompletingStatement
   *     ;
   * </pre>
   */
  private DartStatement parseNonLabelledStatement() {
    if (looksLikeFunctionDeclarationOrExpression()) {
      return parseFunctionDeclaration();
    }
    switch (peek(0)) {
      case IF:
        return parseIfStatement();

      case SWITCH:
        return parseSwitchStatement();

      case WHILE:
        return parseWhileStatement();

      case DO:
        return parseDoWhileStatement();

      case FOR:
        return parseForStatement();

      case VAR: {
        beginVarDeclaration();
        consume(Token.VAR);
        List<DartVariable> vars = parseInitializedVariableList();
        expectStatmentTerminator();
        return done(new DartVariableStatement(vars, null));
      }

      case FINAL: {
        beginFinalDeclaration();
        consume(peek(0));
        DartTypeNode type = null;
        if (peek(1) == Token.IDENTIFIER || peek(1) == Token.LT) {
          // We know we have a type.
          type = parseTypeAnnotation();
        }
        List<DartVariable> vars = parseInitializedVariableList();
        expectStatmentTerminator();
        return done(new DartVariableStatement(vars, type, Modifiers.NONE.makeFinal()));
      }

      case LBRACE:
        return parseBlock();

      case CONTINUE:
        return parseContinueStatement();

      case BREAK:
        return parseBreakStatement();

      case RETURN:
        return parseReturnStatement();

      case THROW:
        return parseThrowStatement();

      case TRY:
        return parseTryStatement();

      case SEMICOLON:
        beginEmptyStatement();
        consume(Token.SEMICOLON);
        return done(new DartEmptyStatement());

      // Things that we know can't be valid statements get a synthetic error statement
      case RBRACE:
        // no need to create a separate parser event as the AST node is enough
        beginEmptyStatement();
        // TODO(jat): other tokens that should be caught here?
        return done(parseErrorStatement());

      case IDENTIFIER:
        // we have already eliminated function declarations earlier, so just need to check for
        // variable declarations here.
        if (peek(1) == Token.LT || peek(1) == Token.IDENTIFIER) {
          beginTypeFunctionOrVariable();
          DartTypeNode type = tryTypeAnnotation();
          if (type != null) {
            List<DartVariable> vars = parseInitializedVariableList();
            expect(Token.SEMICOLON);
            return done(new DartVariableStatement(vars, type));
          } else {
            rollback();
          }
        }
        //$FALL-THROUGH$

      default:
        return parseExpressionStatement();
    }
  }

  /**
   * Check if succeeding tokens look like a function declaration - the parser state is unchanged
   * upon return.
   *
   * See {@link #parseFunctionDeclaration()}.
   *
   * @return true if the following tokens should be parsed as a function definition
   */
  private boolean looksLikeFunctionDeclarationOrExpression() {
    startLookahead();
    try {
      if (peek(0) == Token.IDENTIFIER && peek(1) == Token.LPAREN) {
        // just a name, no return type
        consume(Token.IDENTIFIER);
      } else if (isReturnType()) {
        if (!optional(Token.IDENTIFIER)) {
          // return types must be followed by a function name
          return false;
        }
      }
      // start of parameter list
      if (!optional(Token.LPAREN)) {
        return false;
      }
      // find matching parenthesis
      int count = 1;
      while (count != 0) {
        switch (next()) {
          case EOS:
            return false;
          case LPAREN:
            count++;
            break;
          case RPAREN:
            count--;
            break;
        }
      }
      return (peek(0) == Token.ARROW || peek(0) == Token.LBRACE);
    } finally {
      rollback();
    }
  }

  /**
   * Parse a function declaration.
   *
   * <pre>
   * nonLabelledStatement : ...
   *     | functionDeclaration functionBody
   *
   * functionDeclaration
   *    : FUNCTION identifier formalParameterList
   *      { legacy($start, "deprecated 'function' keyword"); }
   *    | returnType error=FUNCTION identifier? formalParameterList
   *      { legacy($error, "deprecated 'function' keyword"); }
   *    | returnType? identifier formalParameterList
   *    ;
   * </pre>
   *
   * @return a {@link DartStatement} representing the function declaration
   */
  private DartStatement parseFunctionDeclaration() {
    beginFunctionDeclaration();
    DartIdentifier[] namePtr = new DartIdentifier[1];
    DartFunction function = parseFunctionDeclarationOrExpression(namePtr, true);
    return done(new DartExprStmt(doneWithoutConsuming(new DartFunctionExpression(namePtr[0],
        doneWithoutConsuming(function), true))));
  }

  private DartStatement parseExpressionStatement() {
    beginExpressionStatement();
    if (peek(1) == Token.LPAREN && optionalPseudoKeyword(ASSERT_KEYWORD)) {
      consume(Token.LPAREN);
      DartExpression expression = parseConditionalExpression();
      DartExpression message = null;
      if (optional(Token.COMMA)) {
        message = parseConditionalExpression();
      }
      expectCloseParen();
      expectStatmentTerminator();
      return done(new DartAssertion(expression, message));
    }
    DartExpression expression = parseExpression();
    expectStatmentTerminator();
    return done(new DartExprStmt(expression));
  }

  /**
   * Expect a close paren, reporting an error and consuming tokens until a
   * plausible continuation is found if it isn't present.
   */
  private void expectCloseParen() {
    int parenCount = 1;
    switch (peek(0)) {
      case RPAREN:
        expect(Token.RPAREN);
        return;

      case EOS:
      case LBRACE:
      case SEMICOLON:
        reportErrorWithoutAdvancing(DartCompilerErrorCode.UNEXPECTED_TOKEN);
        return;

      case LPAREN:
        ++parenCount;
        //$FALL-THROUGH$
      default:
        reportErrorWithoutAdvancing(DartCompilerErrorCode.UNEXPECTED_TOKEN);
        break;
    }

    // eat tokens until we get a close paren or a plausible terminator (which
    // is not consumed)
    while (parenCount > 0) {
      switch (peek(0)) {
        case RPAREN:
          expect(Token.RPAREN);
          --parenCount;
          break;

        case LPAREN:
          expect(Token.LPAREN);
          ++parenCount;
          break;

        case LBRACE:
        case SEMICOLON:
          return;

        default:
          next();
          break;
      }
    }
  }

  /**
   * Expect a close brace, reporting an error and consuming tokens until a
   * plausible continuation is found if it isn't present.
   */
  private void expectCloseBrace() {
    int braceCount = 1;
    switch (peek(0)) {
      case RBRACE:
        expect(Token.RBRACE);
        return;

      case EOS:
      case SEMICOLON:
        reportErrorWithoutAdvancing(DartCompilerErrorCode.UNEXPECTED_TOKEN);
        return;

      case LBRACE:
        ++braceCount;
        //$FALL-THROUGH$
      default:
        reportErrorWithoutAdvancing(DartCompilerErrorCode.UNEXPECTED_TOKEN);
        break;
    }

    // eat tokens until we get a matching close brace or end of stream
    while (braceCount > 0) {
      switch (next()) {
        case RBRACE:
          braceCount--;
          break;

        case LBRACE:
          braceCount++;
          break;

        case EOS:
          return;
      }
    }
  }

  /**
   * Collect plausible statement tokens and return a synthetic error statement
   * containing them.
   * <p>
   * Note that this is a crude heuristic that needs to be improved for better
   * error recovery.
   * 
   * @return a {@link DartSyntheticErrorStatement}
   */
  private DartStatement parseErrorStatement() {
    StringBuilder buf = new StringBuilder();
    boolean done = false;
    int braceCount = 1;
    while (!done) {
      buf.append(getPeekTokenValue(0));
      next();
      switch (peek(0)) {
        case RBRACE:
          if (--braceCount == 0) {
            done = true;
          }
          break;
        case LBRACE:
          braceCount++;
          break;
        case EOS:
        case SEMICOLON:
          done = true;
          break;
      }
    }
    return new DartSyntheticErrorStatement(buf.toString());
  }


  /**
   * Look for a statement terminator, giving error messages and consuming tokens
   * for error recovery.
   */
  protected void expectStatmentTerminator() {
    Token token = peek(0);
    int braceCount = 1;
    switch (token) {
      case SEMICOLON:
        expect(Token.SEMICOLON);
        return;

      case EOS:
      case RBRACE:
        reportErrorWithoutAdvancing(DartCompilerErrorCode.EXPECTED_SEMICOLON);
        return;

      case LBRACE:
        ++braceCount;
        //$FALL-THROUGH$
      default:
        // give error message
        expect(Token.SEMICOLON);
        break;
    }

    while (true) { 
      switch (peek(0)) {
        case EOS:
          return;

        case SEMICOLON:
          if (braceCount < 2) {
            // if we have seen open braces while skipping, keep looking
            return;
          }
          next();
          break;

        case RBRACE:
          if (--braceCount == 0) {
            return;
          }
          break;
     
        case LBRACE:
          ++braceCount;
          //$FALL-THROUGH$
        default:
          next();
          break;
      }
    }
  }

  /**
   * Report an error without advancing past the next token.
   * 
   * @param errCode the error code to report, which may take a string parameter
   *     containing the actual token found
   */
  private void reportErrorWithoutAdvancing(DartCompilerErrorCode errCode) {
    startLookahead();
    Token actual = peek(0);
    next();
    reportError(position(), errCode, actual);
    rollback();
  }

  /**
   * <pre>
   * iterationStatement
   *     : WHILE '(' expression ')' statement
   *     | DO statement WHILE '(' expression ')' ';'
   *     | FOR '(' forLoopParts ')' statement
   *     ;
   *  </pre>
   */
  private DartWhileStatement parseWhileStatement() {
    beginWhileStatement();
    expect(Token.WHILE);
    expect(Token.LPAREN);
    DartExpression condition = parseExpression();
    expectCloseParen();
    DartStatement body = parseStatement();
    return done(new DartWhileStatement(condition, body));
  }

  /**
   * <pre>
   * iterationStatement
   *     : WHILE '(' expression ')' statement
   *     | DO statement WHILE '(' expression ')' ';'
   *     | FOR '(' forLoopParts ')' statement
   *     ;
   *  </pre>
   */
  private DartDoWhileStatement parseDoWhileStatement() {
    beginDoStatement();
    expect(Token.DO);
    DartStatement body = parseStatement();
    expect(Token.WHILE);
    expect(Token.LPAREN);
    DartExpression condition = parseExpression();
    expectCloseParen();
    expectStatmentTerminator();
    return done(new DartDoWhileStatement(condition, body));
  }

  /**
   * <pre>
   * iterationStatement
   *     : WHILE '(' expression ')' statement
   *     | DO statement WHILE '(' expression ')' ';'
   *     | FOR '(' forLoopParts ')' statement
   *     ;
   *
   * forLoopParts
   *     : forInitializerStatement expression? ';' expressionList?
   *     | constVarOrType? identifier IN expression
   *     ;
   *
   * forInitializerStatement
   *     : initializedVariableDeclaration ';'
   *     | expression? ';'
   *     ;
   * </pre>
   */
  private DartStatement parseForStatement() {
    beginForStatement();
    expect(Token.FOR);
    expect(Token.LPAREN);

    // Setup
    DartStatement setup = null;
    if (peek(0) != Token.SEMICOLON) {
      // Found a setup expression/statement
      beginForInitialization();
      Modifiers modifiers = Modifiers.NONE;
      if (optional(Token.VAR)) {
        setup = done(new DartVariableStatement(parseInitializedVariableList(), null, modifiers));
      } else {
        if (optional(Token.FINAL)) {
          modifiers = modifiers.makeFinal();
        }
        DartTypeNode type = (peek(1) == Token.IDENTIFIER || peek(1) == Token.LT)
            ? tryTypeAnnotation() : null;
        if (modifiers.isFinal() || type != null) {
          setup = done(new DartVariableStatement(parseInitializedVariableList(), type, modifiers));
        } else {
          setup = done(new DartExprStmt(parseExpression()));
        }
      }
    }

    if (optional(Token.IN)) {
      if (setup instanceof DartVariableStatement) {
        DartVariableStatement variableStatement = (DartVariableStatement) setup;
        List<DartVariable> variables = variableStatement.getVariables();
        if (variables.size() != 1) {
          reportError(variables.get(1), DartCompilerErrorCode.FOR_IN_WITH_MULTIPLE_VARIABLES);
        }
        DartExpression initializer = variables.get(0).getValue();
        if (initializer != null) {
          reportError(initializer, DartCompilerErrorCode.FOR_IN_WITH_VARIABLE_INITIALIZER);
        }
      } else {
        DartExpression expression = ((DartExprStmt) setup).getExpression();
        if (!(expression instanceof DartIdentifier)) {
          reportError(setup, DartCompilerErrorCode.FOR_IN_WITH_COMPLEX_VARIABLE);
        }
      }

      DartExpression iterable = parseExpression();
      expectCloseParen();

      DartStatement body = parseStatement();
      return done(new DartForInStatement(setup, iterable, body));

    } else if (optional(Token.SEMICOLON)) {

      // Condition
      DartExpression condition = null;
      if (peek(0) != Token.SEMICOLON) {
        condition = parseExpression();
      }
      expect(Token.SEMICOLON);

      // Next
      DartExpression next = null;
      if (peek(0) != Token.RPAREN) {
        next = parseExpressionList();
      }
      expectCloseParen();

      DartStatement body = parseStatement();
      return done(new DartForStatement(setup, condition, next, body));
    } else {
      reportUnexpectedToken(position(), null, peek(0));
      return done(parseErrorStatement());
    }
  }

  /**
   * <pre>
   * selectionStatement
   *    : IF '(' expression ')' statement ((ELSE)=> ELSE statement)?
   *    | SWITCH '(' expression ')' '{' switchCase* defaultCase? '}'
   *    ;
   * </pre>
   */
  private DartIfStatement parseIfStatement() {
    beginIfStatement();
    expect(Token.IF);
    expect(Token.LPAREN);
    DartExpression condition = parseExpression();
    expectCloseParen();
    DartStatement yes = parseStatement();
    DartStatement no = null;
    if (optional(Token.ELSE)) {
      no = parseStatement();
    }
    return done(new DartIfStatement(condition, yes, no));
  }

  /**
   * <pre>
   * caseStatements
   *    : normalCompletingStatement* abruptCompletingStatement
   *    ;
   * </pre>
   */
  private List<DartStatement> parseCaseStatements() {
    List<DartStatement> statements = new ArrayList<DartStatement>();
    DartStatement statement = null;
    while (true) {
      switch (peek(0)) {
        case CASE:
        case DEFAULT:
        case RBRACE:
        case EOS:
          return statements;
        default:
          if ((statement = parseStatement()) == null) {
            return statements;
          }
          statements.add(statement);
          if (statement.isAbruptCompletingStatement()) {
            /*
             * TODO(jat): is this correct? It seems like we would get better
             * error messages if we parsed dead code as part of this case block
             * and gave an error for unreachable code
             */
            return statements;
          }
      }
    }
  }

  /**
   * <pre>
   * switchCase
   *    : label? (CASE expression ':')+ caseStatements
   *    ;
   * </pre>
   */
  private DartSwitchMember parseCaseMember(DartLabel label) {
    // The begin() associated with the done() in this method is in the method
    // parseSwitchStatement(), called by beginSwitchMember().
    expect(Token.CASE);
    DartExpression caseExpr = parseExpression();
    expect(Token.COLON);
    return done(new DartCase(caseExpr, label, parseCaseStatements()));
  }

  /**
   * <pre>
   * defaultCase
   *    : label? (CASE expression ':')* DEFAULT ':' caseStatements
   *    ;
   * </pre>
   */
  private DartSwitchMember parseDefaultMember(DartLabel label) {
    // The begin() associated with the done() in this method is in the method
    // parseSwitchStatement(), called by beginSwitchMember().
    expect(Token.DEFAULT);
    expect(Token.COLON);
    return done(new DartDefault(label, parseCaseStatements()));
  }


  /**
   * <pre>
   * selectionStatement
   *    : IF '(' expression ')' statement ((ELSE)=> ELSE statement)?
   *    | SWITCH '(' expression ')' '{' switchCase* defaultCase? '}'
   *    ;
   * </pre>
   */
  private DartStatement parseSwitchStatement() {
    beginSwitchStatement();
    expect(Token.SWITCH);

    expect(Token.LPAREN);
    DartExpression expr = parseExpression();
    expectCloseParen();

    List<DartSwitchMember> members = new ArrayList<DartSwitchMember>();
    expect(Token.LBRACE);

    boolean done = optional(Token.RBRACE);
    while (!done) {
      DartLabel label = null;
      beginSwitchMember(); // switch member
      if (peek(0) == Token.IDENTIFIER) {
        beginLabel();
        DartIdentifier identifier = parseIdentifier();
        expect(Token.COLON);
        label = done(new DartLabel(identifier, null));
      }

      if (peek(0) == Token.CASE) {
        members.add(parseCaseMember(label));
      } else if (optional(Token.RBRACE)) {
        if (label != null) {
          reportError(position(), DartCompilerErrorCode.EXPECTED_CASE_OR_DEFAULT);
        }
        done = true;
        done(null);
      } else {
        if (peek(0) != Token.EOS) { 
          members.add(parseDefaultMember(label));
        }
        expectCloseBrace();
        done = true; // Ensure termination.
      }
    }
    return done(new DartSwitchStatement(expr, members));
  }

  /**
   * <pre>
   * catchParameter
   *    : FINAL type? identifier
   *    | VAR identifier
   *    | type identifier
   *    ;
   *  </pre>
   */
  private DartParameter parseCatchParameter() {
    beginCatchParameter();
    DartTypeNode type = null;
    Modifiers modifiers = Modifiers.NONE;
    boolean isDeclared = false;
    if (optional(Token.VAR)) {
      isDeclared = true;
    } else {
      if (optional(Token.FINAL)) {
        modifiers = modifiers.makeFinal();
        isDeclared = true;
      }
      if (peek(1) != Token.COMMA && peek(1) != Token.RPAREN) {
        type = parseTypeAnnotation();
        isDeclared = true;
      }
    }
    DartIdentifier name = parseIdentifier();
    if (!isDeclared) {
      reportError(name, DartCompilerErrorCode.EXPECTED_VAR_FINAL_OR_TYPE);
    }
    return done(new DartParameter(name, type, null, null, modifiers));
  }

  /**
   * <pre>
   * tryStatement
   *     : TRY block (catchPart+ finallyPart? | finallyPart)
   *     ;
   *
   * catchPart
   *     : CATCH '(' declaredIdentifier (',' declaredIdentifier)? ')' block
   *     ;
   *
   * finallyPart
   *     : FINALLY block
   *     ;
   * </pre>
   */
  private DartTryStatement parseTryStatement() {
    beginTryStatement();
    // Try.
    expect(Token.TRY);
    DartBlock tryBlock = parseBlock();

    List<DartCatchBlock> catches = new ArrayList<DartCatchBlock>();
    while (optional(Token.CATCH)) {
      beginCatchClause();
      expect(Token.LPAREN);
      DartParameter exception = parseCatchParameter();
      DartParameter stackTrace = null;
      if (optional(Token.COMMA)) {
        stackTrace = parseCatchParameter();
      }
      expectCloseParen();
      DartBlock block = parseBlock();
      catches.add(done(new DartCatchBlock(block, exception, stackTrace)));
    }

    // Finally.
    DartBlock finallyBlock = null;
    if (optional(Token.FINALLY)) {
      finallyBlock = parseBlock();
    }

    if ( catches.size() == 0 && finallyBlock == null) {
      reportError(new DartCompilationError(tryBlock.getSource(), new Location(position()),
        DartCompilerErrorCode.CATCH_OR_FINALLY_EXPECTED));
    }

    return done(new DartTryStatement(tryBlock, catches, finallyBlock));
  }

  /**
   * <pre>
   * unaryExpression
   *     : postfixExpression
   *     | prefixOperator unaryExpression
   *     | incrementOperator assignableExpression
   *     ;
   *
   *  @return an expression or null if noFail is true and the next tokens could not be parsed as an
   *      expression, leaving the state unchanged.
   *  </pre>
   */
  private DartExpression parseUnaryExpression() {
    // A '+' prefix does not have any effect.
    optional(Token.ADD);
    Token token = peek(0);
    if (token.isUnaryOperator() || token == Token.SUB) {
      beginUnaryExpression();
      consume(token);
      DartExpression unary = parseUnaryExpression();
      if (token.isCountOperator()) {
        ensureAssignable(unary);
      }
      return done(new DartUnaryExpression(token, unary, true));
    } else {
      return parsePostfixExpression();
    }
  }

  /**
   * <pre>
   * type
   *     : qualified typeArguments?
   *     ;
   * </pre>
   */
  private DartTypeNode parseTypeAnnotation() {
    beginTypeAnnotation();
    return done(new DartTypeNode(parseQualified(), parseTypeArgumentsOpt()));
  }

  /**
   * <pre>
   * typeArguments
   *     : '<' typeList '>'
   *     ;
   *
   * typeList
   *     : type (',' type)*
   *     ;
   * </pre>
   */
  private List<DartTypeNode> parseTypeArguments() {
    consume(Token.LT);
    List<DartTypeNode> arguments = new ArrayList<DartTypeNode>();
    do {
      arguments.add(parseTypeAnnotation());
    } while (optional(Token.COMMA));
    if (!tryParameterizedTypeEnd()) {
      expect(Token.GT);
    }
    return arguments;
  }

  /**
   * <pre>
   * typeArguments?
   * </pre>
   */
  private List<DartTypeNode> parseTypeArgumentsOpt() {
    return (peek(0) == Token.LT)
        ? parseTypeArguments()
        : Collections.<DartTypeNode>emptyList();
  }

  /**
   * <pre>
   * qualified
   *     : identifier ('.' identifier)?
   *     ;
   * </pre>
   */
  private DartExpression parseQualified() {
    beginQualifiedIdentifier();
    DartExpression qualified = parseIdentifier();
    if (optional(Token.PERIOD)) {
      // The previous identifier was a prefix.
      qualified = new DartPropertyAccess(qualified, parseIdentifier());
    }
    return done(qualified);
  }

  private boolean tryParameterizedTypeEnd() {
    switch (peek(0)) {
      case GT:
        consume(Token.GT);
        return true;
      case SAR:
        setPeek(0, Token.GT);
        return true;
      case SHR:
        setPeek(0, Token.SAR);
        return true;
      default:
        return false;
    }
  }

  private DartTypeNode tryTypeAnnotation() {
    if (peek(0) != Token.IDENTIFIER) {
      return null;
    }
    List<DartTypeNode> typeArguments = new ArrayList<DartTypeNode>();
    beginTypeAnnotation(); // to allow roll-back in case we're not at a type

    DartNode qualified = parseQualified();

    if (optional(Token.LT)) {
      if (peek(0) != Token.IDENTIFIER) {
        rollback();
        return null;
      }
      beginTypeArguments();
      DartNode qualified2 = parseQualified();
      DartTypeNode argument;
      switch (peek(0)) {
        case LT:
          // qualified < qualified2 <
          argument = done(new DartTypeNode(qualified2, parseTypeArguments()));
          break;

        case GT:
        case SAR:
        case SHR:
          // qualified < qualified2 >
        case COMMA:
          // qualified < qualified2 ,
          argument = done(new DartTypeNode(qualified2, Collections.<DartTypeNode>emptyList()));
          break;

        default:
          done(null);
          rollback();
          return null;
      }
      typeArguments.add(argument);

      while (optional(Token.COMMA)) {
        typeArguments.add(parseTypeAnnotation());
      }
      if (!tryParameterizedTypeEnd()) {
        expect(Token.GT);
      }
    }

    return done(new DartTypeNode(qualified, typeArguments));
  }

  private DartIdentifier parseIdentifier() {
    beginIdentifier();
    expect(Token.IDENTIFIER);
    String token = ctx.getTokenString();
    return done(new DartIdentifier(token != null ? token : ""));
  }

  public DartExpression parseEntryPoint() {
    beginEntryPoint();
    DartExpression entry = parseIdentifier();
    while (!EOS()) {
      expect(Token.PERIOD);
      entry = doneWithoutConsuming(new DartPropertyAccess(entry, parseIdentifier()));
    }
    return done(entry);
  }

  private void ensureAssignable(DartExpression expression) {
    if (expression != null && !expression.isAssignable()) {
      reportError(position(), DartCompilerErrorCode.ILLEGAL_ASSIGNMENT_TO_NON_ASSIGNABLE);
    }
  }

  private void reportError(DartCompilationError dartError) {
    if ((errorHistory.size() > MAX_DEFAULT_ERRORS) ||
        errorHistory.contains(dartError.hashCode())) {
      // Force parser termination if one of the conditions are observed:
      // 1) We have already reported the same error at the same location; This
      //    is an indication that the parser is not making progress.
      // 2) If we reached the absolute maximum number of errors.
      // TODO (fabiomfv) consider throwing AssertionError to terminate parsing.
    } else {
      ctx.error(dartError);
      errorHistory.add(dartError.hashCode());
    }
  }

  private void reportError(DartNode node, ErrorCode errorCode, Object... arguments) {
    reportError(new DartCompilationError(node, errorCode, arguments));
  }
}
