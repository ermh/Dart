/*
 * Copyright (c) 2011, the Dart project authors.
 * 
 * Licensed under the Eclipse Public License v1.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 * 
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package com.google.dart.tools.core.internal.completion;

import com.google.dart.compiler.DartCompilationError;
import com.google.dart.compiler.DartCompilerListener;
import com.google.dart.compiler.DartSource;
import com.google.dart.compiler.LibrarySource;
import com.google.dart.compiler.UrlDartSource;
import com.google.dart.compiler.ast.DartBlock;
import com.google.dart.compiler.ast.DartBooleanLiteral;
import com.google.dart.compiler.ast.DartClass;
import com.google.dart.compiler.ast.DartClassMember;
import com.google.dart.compiler.ast.DartExprStmt;
import com.google.dart.compiler.ast.DartExpression;
import com.google.dart.compiler.ast.DartField;
import com.google.dart.compiler.ast.DartFunction;
import com.google.dart.compiler.ast.DartFunctionTypeAlias;
import com.google.dart.compiler.ast.DartIdentifier;
import com.google.dart.compiler.ast.DartIfStatement;
import com.google.dart.compiler.ast.DartMethodDefinition;
import com.google.dart.compiler.ast.DartMethodInvocation;
import com.google.dart.compiler.ast.DartNewExpression;
import com.google.dart.compiler.ast.DartNode;
import com.google.dart.compiler.ast.DartNodeTraverser;
import com.google.dart.compiler.ast.DartParameter;
import com.google.dart.compiler.ast.DartPropertyAccess;
import com.google.dart.compiler.ast.DartReturnStatement;
import com.google.dart.compiler.ast.DartSuperConstructorInvocation;
import com.google.dart.compiler.ast.DartThisExpression;
import com.google.dart.compiler.ast.DartTypeNode;
import com.google.dart.compiler.ast.DartTypeParameter;
import com.google.dart.compiler.ast.DartUnit;
import com.google.dart.compiler.ast.DartUnqualifiedInvocation;
import com.google.dart.compiler.ast.DartVariable;
import com.google.dart.compiler.ast.DartVariableStatement;
import com.google.dart.compiler.parser.DartScannerParserContext;
import com.google.dart.compiler.parser.ParserContext;
import com.google.dart.compiler.resolver.ClassElement;
import com.google.dart.compiler.resolver.ConstructorElement;
import com.google.dart.compiler.resolver.CoreTypeProvider;
import com.google.dart.compiler.resolver.CoreTypeProviderImplementation;
import com.google.dart.compiler.resolver.Element;
import com.google.dart.compiler.resolver.ElementKind;
import com.google.dart.compiler.resolver.EnclosingElement;
import com.google.dart.compiler.resolver.FieldElement;
import com.google.dart.compiler.resolver.MethodElement;
import com.google.dart.compiler.resolver.ResolutionContext;
import com.google.dart.compiler.resolver.Resolver;
import com.google.dart.compiler.resolver.Scope;
import com.google.dart.compiler.resolver.VariableElement;
import com.google.dart.compiler.type.InterfaceType;
import com.google.dart.compiler.type.Type;
import com.google.dart.compiler.type.TypeAnalyzer;
import com.google.dart.compiler.type.TypeKind;
import com.google.dart.tools.core.DartCore;
import com.google.dart.tools.core.completion.CompletionProposal;
import com.google.dart.tools.core.completion.CompletionRequestor;
import com.google.dart.tools.core.dom.NodeFinder;
import com.google.dart.tools.core.internal.compiler.SilentDartCompilerListener;
import com.google.dart.tools.core.internal.completion.LocalVariableFinder.LocalName;
import com.google.dart.tools.core.internal.completion.ast.BlockCompleter;
import com.google.dart.tools.core.internal.completion.ast.FunctionCompleter;
import com.google.dart.tools.core.internal.completion.ast.MethodInvocationCompleter;
import com.google.dart.tools.core.internal.completion.ast.ParameterCompleter;
import com.google.dart.tools.core.internal.completion.ast.PropertyAccessCompleter;
import com.google.dart.tools.core.internal.completion.ast.TypeCompleter;
import com.google.dart.tools.core.internal.model.DartLibraryImpl;
import com.google.dart.tools.core.internal.search.listener.GatheringSearchListener;
import com.google.dart.tools.core.internal.util.CharOperation;
import com.google.dart.tools.core.internal.util.Messages;
import com.google.dart.tools.core.internal.util.TypeUtil;
import com.google.dart.tools.core.model.CompilationUnit;
import com.google.dart.tools.core.model.DartElement;
import com.google.dart.tools.core.model.DartModelException;
import com.google.dart.tools.core.model.DartProject;
import com.google.dart.tools.core.model.Method;
import com.google.dart.tools.core.search.MatchKind;
import com.google.dart.tools.core.search.MatchQuality;
import com.google.dart.tools.core.search.SearchEngine;
import com.google.dart.tools.core.search.SearchEngineFactory;
import com.google.dart.tools.core.search.SearchException;
import com.google.dart.tools.core.search.SearchMatch;
import com.google.dart.tools.core.search.SearchPatternFactory;
import com.google.dart.tools.core.search.SearchScope;
import com.google.dart.tools.core.search.SearchScopeFactory;
import com.google.dart.tools.core.utilities.compiler.DartCompilerUtilities;
import com.google.dart.tools.core.workingcopy.WorkingCopyOwner;

import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.NullProgressMonitor;
import org.eclipse.core.runtime.OperationCanceledException;
import org.eclipse.core.runtime.Platform;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;
import java.util.Stack;

/**
 * The analysis engine for code completion. It uses visitors to break the analysis up into single
 * AST node-sized chunks.
 * <p>
 * Within the visitors the analyses are annotated with a shorthand of the AST form required to hit
 * that analysis block. A '!' is used to indicate the completion location. There may be multiple '!'
 * in a single example to indicate several distinct completions points that drive the same analysis.
 * A single space indicates arbitrary whitespace; image each as a newline to get a better idea of
 * the code patterns involved.
 */
public class CompletionEngine {

  /**
   * In cases where the analysis is driven by an identifier, a finer-grained analysis is required.
   * The primary analyzer defers to this analyzer to propose completions based on the structure of
   * the parent of the identifier.
   */
  private class IdentifierCompletionProposer extends DartNodeTraverser<Void> {
    private DartIdentifier identifier;

    private IdentifierCompletionProposer(DartIdentifier node) {
      this.identifier = node;
    }

    @Override
    public Void visitBlock(DartBlock node) {
      // between statements
      return null;
    }

    @Override
    public Void visitExpression(DartExpression node) {
      // { xc = yc = MA! } and many others
      proposeIdentifierPrefixCompletions(identifier);
      return null;
    }

    @Override
    public Void visitExprStmt(DartExprStmt completionNode) {
      // { v! } or { x; v! }
      proposeIdentifierPrefixCompletions(completionNode);
      return null;
    }

    @Override
    public Void visitField(DartField node) {
      if (node.getValue() == identifier) {
        // { int f = Ma! }
        proposeIdentifierPrefixCompletions(node);
      } else if (node.getName() == identifier) {
        // { static final num MA! }
        proposeIdentifierPrefixCompletions(node);
      }
      return null;
    }

    @Override
    public Void visitIfStatement(DartIfStatement completionNode) {
      // { if (v!) }
      proposeIdentifierPrefixCompletions(completionNode);
      return null;
    }

    @Override
    public Void visitMethodInvocation(DartMethodInvocation completionNode) {
      if (completionNode instanceof MethodInvocationCompleter) {
        DartIdentifier methodName = completionNode.getFunctionName();
        if (methodName == identifier) {
          // { x.y! }
          Type type = analyzeType(completionNode.getTarget());
          if (TypeKind.of(type) == TypeKind.VOID) {
            DartExpression exp = completionNode.getTarget();
            if (exp instanceof DartIdentifier) {
              Element element = ((DartIdentifier) exp).getTargetSymbol();
              type = element.getType();
            }
          }
          createCompletionsForQualifiedMemberAccess(methodName, type, false);
        } else {
          // { x!.y } or { x.y(a!) } or { x.y(a, b, C!) }
          // TODO Consider using proposeIdentifierPrefixCompletions() here
          proposeVariables(completionNode, identifier, resolvedMember);
          DartClass classDef = (DartClass) resolvedMember.getParent();
          ClassElement elem = classDef.getSymbol();
          Type type = elem.getType();
          createCompletionsForPropertyAccess(identifier, type, false, false);
        }
      }
      return null;
    }

    @Override
    public Void visitNewExpression(DartNewExpression node) {
      // { new x! }
      List<SearchMatch> matches = findTypesWithPrefix(identifier);
      if (matches == null || matches.size() == 0) {
        return null;
      }
      for (SearchMatch match : matches) {
        String prefix = extractFilterPrefix(identifier);
        createTypeCompletionsForConstructor(identifier, match, prefix);
      }
      return null;
    }

    @Override
    public Void visitNode(DartNode node) {
      visitorNotImplementedYet(node, identifier, getClass());
      return null;
    }

    @Override
    public Void visitPropertyAccess(DartPropertyAccess completionNode) {
      if (completionNode instanceof PropertyAccessCompleter) {
        DartIdentifier propertyName = completionNode.getName();
        if (propertyName == identifier) {
          Type type = analyzeType(completionNode.getQualifier());
          if (type.getKind() == TypeKind.DYNAMIC || type.getKind() == TypeKind.VOID) {
            // TODO Reconsider this code; it seems fragile. We want the type Array from "Array.f!"
            if (completionNode.getQualifier() instanceof DartIdentifier) {
              Element element = ((DartIdentifier) completionNode.getQualifier()).getTargetSymbol();
              if (ElementKind.of(element) == ElementKind.CLASS) {
                type = element.getType();
                if (type instanceof InterfaceType) {
                  // { Array.! } or { Array.f! }
                  createCompletionsForFactoryInvocation(propertyName, (InterfaceType) type);
                  createCompletionsForQualifiedMemberAccess(propertyName, type, false);
                }
              }
            }
          } else {
            // { a.! } or { a.x! }
            boolean isInstance = completionNode.getQualifier() instanceof DartThisExpression;
            createCompletionsForQualifiedMemberAccess(propertyName, type, isInstance);
          }
        } else {
          DartNode q = completionNode.getQualifier();
          if (q instanceof DartIdentifier) {
            DartIdentifier qualifier = (DartIdentifier) q;
            proposeIdentifierPrefixCompletions(qualifier);
          }
        }
      }
      return null;
    }

    @Override
    public Void visitReturnStatement(DartReturnStatement completionNode) {
      // { return v! }
      proposeIdentifierPrefixCompletions(completionNode);
      return null;
    }

    @Override
    public Void visitSuperConstructorInvocation(DartSuperConstructorInvocation node) {
      // { super.x!(); }
      ConstructorElement cn = node.getSymbol();
      if (cn != null) {
        ClassElement ce = cn.getConstructorType();
        if (ce != null) {
          // TODO Restrict proposals to constructors
          createCompletionsForMethodInvocation(identifier, ce.getType(), true, false);
        }
      }
      return null;
    }

    @Override
    public Void visitTypeNode(DartTypeNode completionNode) {
      if (completionNode instanceof TypeCompleter) {
        TypeCompleter typeCompleter = (TypeCompleter) completionNode;
        Stack<Mark> s = typeCompleter.getCompletionParsingContext();
        if (s.size() < 2) {
          throw new IllegalArgumentException(); // TODO Clean up: this should never happen
        }
        if (s.peek() == Mark.ConstructorName) {
          // { new x! }
          typeCompleter.getParent().accept(this);
        } else {
          Mark m = s.elementAt(s.size() - 2);
          switch (m) {
            case FormalParameter:
              // bar(x,!) {} or bar(!, int x) {} or bar(B! x) {}
              if (identifier.getTargetName().length() == 0) {
                proposeTypesForNewParam();
                break;
              } else {
                proposeTypesForPrefix(identifier);
              }
              break;
            case TypeFunctionOrVariable:
              // { x; v! x; } or { v! x; }
              proposeIdentifierPrefixCompletions(typeCompleter);
              break;
            case ClassBody:
              // class x extends A! (A may be empty string)
              // class x implements I! (parser error if I is empty string)
              // interface x extends I!
              DartClass classDef = (DartClass) typeCompleter.getParent();
              boolean isClassDef = classDef.getSuperclass() == typeCompleter;
              proposeClassOrInterfaceNamesForPrefix(identifier, isClassDef);
              break;
            case ClassMember:
              // class x { ! }
              // TODO check for supertype methods whose name starts with identifier
              // if found propose a new method matching its signature
              proposeTypesForPrefix(identifier);
              break;
            case TopLevelElement:
              if (completionNode.getParent() instanceof DartFunctionTypeAlias) {
                // typedef T!
                proposeTypesForPrefix(identifier);
              }
              break;
          }
        }
      }
      return null;
    }

    @Override
    public Void visitUnqualifiedInvocation(DartUnqualifiedInvocation node) {
      // { bar!(); } or { bar(z!); }
      proposeIdentifierPrefixCompletions(node);
      return null;
    }

    @Override
    public Void visitVariable(DartVariable node) {
      // { X a = b! }
      proposeIdentifierPrefixCompletions(identifier);
      return null;
    }

    @Override
    public Void visitVariableStatement(DartVariableStatement completionNode) {
      if (completionNode instanceof DartVariableStatement) {
//      DartIdentifier propertyName = completionNode.getName();
//      if (propertyName.getSourceStart() > actualCompletionPosition) {
//        propertyName = null;
//      }
//      Type type = analyzeType(completionNode.getQualifier());
//      createCompletionsForQualifiedMemberAccess(propertyName, type);
      }
      return null;
    }

    private void proposeIdentifierPrefixCompletions(DartNode node) {
      // Complete an unqualified identifier prefix.
      // We know there is a prefix, otherwise the parser would not produce a DartIdentifier.
      // TODO Determine if statics are being handled properly
      boolean isStatic = resolvedMember.getModifiers().isStatic();
      createCompletionsForLocalVariables(identifier, identifier, resolvedMember);
      Element parentElement = resolvedMember.getSymbol().getEnclosingElement();
      if (parentElement instanceof ClassElement) {
        Type type = ((ClassElement) parentElement).getType();
        createCompletionsForPropertyAccess(identifier, type, false, isStatic);
        createCompletionsForMethodInvocation(identifier, type, false, isStatic);
        proposeTypesForPrefix(identifier, false);
      } else {
        // TODO top-level element
      }
    }
  }

  /**
   * In most cases completion processing begins at an identifier. The identifier itself is not very
   * informative so most identifiers defer to their parent node for analysis.
   */
  private class OuterCompletionProposer extends DartNodeTraverser<Void> {
    private DartNode completionNode;

    private OuterCompletionProposer(DartNode node) {
      this.completionNode = node;
    }

    @Override
    public Void visitBlock(DartBlock node) {
      if (node instanceof BlockCompleter) {
        BlockCompleter block = (BlockCompleter) node;
        Stack<Mark> stack = block.getCompletionParsingContext();
        if (stack.isEmpty() || stack.peek() == Mark.Block) {
          // between statements: { ! } or { ! x; ! y; ! }
          boolean isStatic = resolvedMember.getModifiers().isStatic();
          createCompletionsForLocalVariables(block, null, resolvedMember);
          Element parentElement = resolvedMember.getSymbol().getEnclosingElement();
          if (parentElement instanceof ClassElement) {
            Type type = ((ClassElement) parentElement).getType();
            createCompletionsForPropertyAccess(null, type, false, isStatic);
            createCompletionsForMethodInvocation(null, type, false, isStatic);
            // Types are legal here but we are not proposing them since they are optional
          } else {
            // TODO top-level element
          }
        }
      }
      return null;
    }

    @Override
    public Void visitBooleanLiteral(DartBooleanLiteral node) {
      createProposalsForLiterals(node, "false", "true");
      return null;
    }

    @Override
    public Void visitClass(DartClass node) {
      String classSrc = source.substring(node.getSourceStart(), actualCompletionPosition + 1);
      boolean beforeBrace = classSrc.indexOf('{') < 0;
      if (!beforeBrace) {
        // for top-level elements, try type names
        proposeTypesForNewParam();
        createProposalsForLiterals(node, "void");
      }
      return null;
    }

    @Override
    public Void visitExprStmt(DartExprStmt node) {
      // TODO Determine if there are any valid completions
      return null;
    }

    @Override
    public Void visitField(DartField node) {
      // { int f = Ma! }
      DartIdentifier name = node.getName();
      int begin = name.getSourceStart();
      int len = name.getSourceLength();
      if (begin <= actualCompletionPosition && actualCompletionPosition <= begin + len) {
        // bug in visitor does not visit name
        return node.accept(new IdentifierCompletionProposer(name));
      }
      return null;
    }

    @Override
    public Void visitFunction(DartFunction node) {
      if (node instanceof FunctionCompleter) {
        // new parameter: bar(!) {} or bar(! int x) {} or bar(x, B !) {}
        List<DartParameter> params = node.getParams();
        if (params.isEmpty()) {
          proposeTypesForNewParam();
        } else {
          DartParameter param = params.get(0);
          boolean beforeFirstParam = actualCompletionPosition < param.getSourceStart();
          if (beforeFirstParam) {
            if (node.getParent() instanceof DartMethodDefinition) {
              DartMethodDefinition methodDef = (DartMethodDefinition) node.getParent();
              DartExpression methodName = methodDef.getName();
              if (methodName instanceof DartIdentifier) {
                DartIdentifier methodId = (DartIdentifier) methodName;
                // TODO check for supertype methods whose name starts with identifier and
                // matches the return type, if found propose a new method matching its signature
                proposeTypesForPrefix(methodId);
              } else {
                // TODO qualified names
              }
            } else {
              proposeTypesForNewParam();
              createProposalsForLiterals(node, "void");
            }
          } else {
            param = params.get(params.size() - 1);
            int end = param.getSourceStart() + param.getSourceLength();
            boolean afterLastParam = actualCompletionPosition >= end;
            if (afterLastParam) {
              proposeTypesForNewParam();
            }
          }
        }
      }
      return null;
    }

    @Override
    public Void visitFunctionTypeAlias(DartFunctionTypeAlias node) {
      return null;
    }

    @Override
    public Void visitIdentifier(DartIdentifier node) {
      DartNode parent = node.getParent();
      return parent.accept(new IdentifierCompletionProposer(node));
    }

    @Override
    public Void visitMethodInvocation(DartMethodInvocation completionNode) {
      if (completionNode instanceof MethodInvocationCompleter) {
        DartIdentifier functionName = completionNode.getFunctionName();
        int nameStart = functionName.getSourceStart();
        if (actualCompletionPosition >= nameStart + functionName.getSourceLength()) {
          createCompletionsForLocalVariables(completionNode, null, resolvedMember);
          if (resolvedMember.getParent() instanceof DartClass) {
            DartClass classDef = (DartClass) resolvedMember.getParent();
            ClassElement elem = classDef.getSymbol();
            Type type = elem.getType();
            createCompletionsForPropertyAccess(null, type, false, false);
          } else {
            // top-level element
            return null;
          }
        } else {
          if (nameStart > actualCompletionPosition) {
            functionName = null;
          }
          // { foo.! doFoo(); }
          Type type = analyzeType(completionNode.getTarget());
          createCompletionsForQualifiedMemberAccess(functionName, type, false);
        }
      }
      return null;
    }

    @Override
    public Void visitNewExpression(DartNewExpression node) {
      // { new ! }
      List<SearchMatch> matches = findTypesWithPrefix(null);
      if (matches == null || matches.size() == 0) {
        return null;
      }
      for (SearchMatch match : matches) {
        createTypeCompletionsForConstructor(null, match, "");
      }
      return null;
    }

    @Override
    public Void visitNode(DartNode node) {
      visitorNotImplementedYet(node, this.completionNode, getClass());
      return null;
    }

    @Override
    public Void visitParameter(DartParameter node) {
      // parameter type prefix: bar(B!) {} or bar(1, B!) {}
      if (node instanceof ParameterCompleter) {
        ParameterCompleter param = (ParameterCompleter) node;
        DartTypeNode type = param.getTypeNode();
        if (type == null) {
          // when completion is requested on the first word of a param decl we assume it is a type
          DartExpression typeName = param.getName();
          if (typeName.getSourceStart() <= actualCompletionPosition
              && typeName.getSourceStart() + typeName.getSourceLength() >= actualCompletionPosition) {
            if (typeName instanceof DartIdentifier) {
              DartIdentifier typeId = (DartIdentifier) typeName;
              List<SearchMatch> matches = findTypesWithPrefix(typeId);
              if (matches == null || matches.size() == 0) {
                return null;
              }
              for (SearchMatch match : matches) {
                String prefix = extractFilterPrefix(typeId);
                createTypeCompletionsForParameterDecl(typeId, match, prefix);
              }
            } else if (typeName instanceof DartPropertyAccess) {
              // { class X { X(this.!c) : super() {}}
              typeName.accept(this);
            }
          }
        }
      }
      return null;
    }

    @Override
    public Void visitPropertyAccess(DartPropertyAccess completionNode) {
      if (completionNode instanceof PropertyAccessCompleter) {
        DartIdentifier propertyName = completionNode.getName();
        if (propertyName.getSourceStart() > actualCompletionPosition) {
          propertyName = null;
        }
        // { foo.! } or { class X { X(this.!c) : super() {}}
        Type type = analyzeType(completionNode.getQualifier());
        if (TypeKind.of(type) == TypeKind.VOID) {
          DartIdentifier name = (DartIdentifier) completionNode.getQualifier();
          Element element = name.getTargetSymbol();
          type = element.getType();
        }
        createCompletionsForQualifiedMemberAccess(propertyName, type, false);
      }
      return null;
    }

    @Override
    public Void visitThisExpression(DartThisExpression node) {
      // { this! } the only legal continuation is punctuation, which we do not propose
      // you can't get here directly, this occurs when backspacing from {this.!}
      return null;
    }

    @Override
    public Void visitTypeParameter(DartTypeParameter node) {
      // need new AST node
      // typedef T Deserializer<T!>(InputDataStream input);
      return null;
    }

    @Override
    public Void visitUnit(DartUnit node) {
      return null;
    }

    @Override
    public Void visitUnqualifiedInvocation(DartUnqualifiedInvocation node) {
      // { bar( ! ); } or { bar(! x); } or { bar(x !); } or { bar(x,! y); }
      Element baseElement = node.getTarget().getTargetSymbol();
      if (ElementKind.of(baseElement) == ElementKind.METHOD) {
        MethodElement methodElement = (MethodElement) baseElement;
        List<VariableElement> paramDefs = methodElement.getParameters();
        if (paramDefs.size() == 0) {
          // assume a new param will be added to method def
          createCompletionsForLocalVariables(node, null, resolvedMember);
        } else {
          List<DartExpression> args = node.getArgs();
          if (args.size() == 0) {
            createCompletionsForLocalVariables(node, null, resolvedMember);
          } else {
            // could do positional type matching to order proposals
            createCompletionsForLocalVariables(node, null, resolvedMember);
          }
        }
      }
      return null;
    }

    @Override
    public Void visitVariable(DartVariable node) {
      if (source.charAt(actualCompletionPosition) == '=') {
        // { num y =! } Note: no space after equals
        proposeVariables(completionNode, null, resolvedMember);
      }
      return null;
    }

    @Override
    public Void visitVariableStatement(DartVariableStatement completionNode) {
      if (completionNode instanceof DartVariableStatement) {
        List<DartVariable> vars = completionNode.getVariables();
        if (vars.size() > 0) {
          DartVariable var = vars.get(vars.size() - 1);
          if (var.getSourceStart() + var.getSourceLength() <= actualCompletionPosition) {
            // { num theta = i * ! }
            proposeVariables(completionNode, null, resolvedMember);
            proposeTypesForNewParam();
          }
        }
        return null;
      }
      return null;
    }
  }

  private static final boolean DEBUG = "true".equalsIgnoreCase(Platform.getDebugOption("com.google.dart.tools.ui/debug/CompletionEngine"));
  private static final boolean DEBUG_TIMING = true | "true".equalsIgnoreCase(Platform.getDebugOption("com.google.dart.tools.ui/debug/ResultCollector"));

  public static char[][] createDefaultParameterNames(int length) {
    char[][] names = new char[length][];
    for (int i = 0; i < length; i++) {
      names[i] = ("p" + (i + 1)).toCharArray();
    }
    return names;
  }

  static private List<Element> getAllElements(Type type) {
    Map<String, Element> map = new HashMap<String, Element>();
    List<Element> list = new ArrayList<Element>();
    List<InterfaceType> types = TypeUtil.allSupertypes((InterfaceType) type);
    for (InterfaceType itype : types) {
      ClassElement cls = itype.getElement();
      Iterable<Element> members = cls.getMembers();
      for (Element elem : members) {
        String name = elem.getName();
        if (!map.containsKey(name)) {
          map.put(name, elem);
          list.add(elem);
        }
      }
    }
    return list;
  }

  static private List<Element> getConstructors(InterfaceType type) {
    List<Element> list = new ArrayList<Element>();
    for (Element elem : type.getElement().getConstructors()) {
      list.add(elem);
    }
    return list;
  }

  static private char[][] getParameterNames(Method method) {
    try {
      String[] paramNames = method.getParameterNames();
      int count = paramNames.length;
      char[][] names = new char[count][];
      for (int i = 0; i < count; i++) {
        names[i] = paramNames[i].toCharArray();
      }
      return names;
    } catch (DartModelException exception) {
      return CharOperation.NO_CHAR_CHAR;
    }
  }

  static private char[][] getParameterNames(MethodElement method) {
    List<VariableElement> params = method.getParameters();
    char[][] names = new char[method.getParameters().size()][];
    for (int i = 0; i < method.getParameters().size(); i++) {
      names[i] = params.get(i).getName().toCharArray();
    }
    return names;
  }

  static private char[][] getParameterTypeNames(Method method) {
    try {
      String[] paramNames = method.getParameterTypeNames();
      int count = paramNames.length;
      char[][] names = new char[count][];
      for (int i = 0; i < count; i++) {
        names[i] = paramNames[i].toCharArray();
      }
      return names;
    } catch (DartModelException exception) {
      return CharOperation.NO_CHAR_CHAR;
    }
  }

  static private char[][] getParameterTypeNames(MethodElement method) {
    List<VariableElement> params = method.getParameters();
    char[][] names = new char[method.getParameters().size()][];
    for (int i = 0; i < method.getParameters().size(); i++) {
      names[i] = params.get(i).getType().getElement().getName().toCharArray();
    }
    return names;
  }

  // keys are either String or char[]; values are Object unless Type works
  public HashMap<Object, Object> typeCache;

  private CompletionEnvironment environment;

  private CompletionRequestor requestor;
  private DartProject project;
  private WorkingCopyOwner owner;
  private IProgressMonitor monitor;
  private IPath fileName;
  private int actualCompletionPosition;
  private int offset;
  private String source;
  private ErrorRecordingContext context = new ErrorRecordingContext();
  private DartUnit resolvedUnit;
  private DartClassMember<? extends DartExpression> resolvedMember;
  private CoreTypeProvider typeProvider;
  private ClassElement classElement;
  private boolean isCompletionAfterDot;
  private DartUnit parsedUnit;
  private CompilationUnit currentCompilationUnit;

  /**
   * @param options
   */
  public CompletionEngine(CompletionEnvironment environment, CompletionRequestor requestor,
      Hashtable<String, String> options, DartProject project, WorkingCopyOwner owner,
      IProgressMonitor monitor) {
    this.environment = environment;
    this.requestor = requestor;
    this.project = project;
    this.owner = owner;
    this.monitor = monitor;
    typeCache = new HashMap<Object, Object>();
  }

  public void complete(CompilationUnit sourceUnit, int completionPosition, int pos)
      throws DartModelException {

    if (DEBUG) {
      System.out.print("COMPLETION IN "); //$NON-NLS-1$
      System.out.print(sourceUnit.getPath());
      System.out.print(" AT POSITION "); //$NON-NLS-1$
      System.out.println(completionPosition);
      System.out.println("COMPLETION - Source :"); //$NON-NLS-1$
      System.out.println(sourceUnit.getSource());
    }
    if (monitor != null) {
      monitor.beginTask(Messages.engine_completing, IProgressMonitor.UNKNOWN);
    }
    try {
      fileName = sourceUnit.getPath();
      // look for the node that ends before the cursor position,
      // which may mean we start looking at a single-char node that is just
      // before the cursor position

      checkCancel();

      currentCompilationUnit = sourceUnit;
      String sourceParam = sourceUnit.getSource();
      DartSource sourceFile;
      LibrarySource library = ((DartLibraryImpl) sourceUnit.getLibrary()).getLibrarySourceFile();
      if (sourceUnit.getResource() == null) {
        sourceFile = sourceUnit.getSourceRef();
      } else {
        // TODO Find a better way to get the File?
        File file = sourceUnit.getBuffer().getUnderlyingResource().getRawLocation().toFile();
        sourceFile = new UrlDartSource(file, library);
      }

      complete(library, sourceFile, sourceParam, completionPosition, pos);

    } catch (IndexOutOfBoundsException e) {
      DartCore.logError(e);
      if (DEBUG) {
        System.out.println("Exception caught by CompletionEngine:"); //$NON-NLS-1$
        e.printStackTrace(System.out);
      }
    } catch (NullPointerException ex) {
      DartCore.logError(ex);
      if (DEBUG) {
        System.out.println("Exception caught by CompletionEngine:"); //$NON-NLS-1$
        ex.printStackTrace(System.out);
      }
    }
  }

  /*
   * Visible for testing
   */
  public void complete(LibrarySource library, DartSource sourceFile, String sourceContent,
      int completionPosition, int pos) throws DartModelException {
    source = sourceContent;
    actualCompletionPosition = completionPosition - 1;
    offset = pos;
    isCompletionAfterDot = source.charAt(actualCompletionPosition) == '.';

    DartCompilerListener listener = SilentDartCompilerListener.INSTANCE;
    ParserContext ctx = new DartScannerParserContext(sourceFile, source, listener);
    CompletionParser parser = new CompletionParser(ctx);
    parser.setCompletionPosition(completionPosition);
    parsedUnit = parser.parseUnit(sourceFile);

    if (parsedUnit == null) {
      return;
    }
    if (parsedUnit.getTopLevelNodes().isEmpty()) {
      return;
    }
    Collection<DartCompilationError> parseErrors = new ArrayList<DartCompilationError>();
    Collection<DartUnit> parsedUnits = new ArrayList<DartUnit>();
    parsedUnits.add(parsedUnit);
    resolvedUnit = parsedUnit;

    NodeFinder finder = NodeFinder.find(parsedUnit, completionPosition, 0);
    DartNode resolvedNode = finder.selectNode();
    resolvedMember = finder.getEnclosingMethod();
    if (resolvedMember == null) {
      resolvedMember = finder.getEnclosingField();
    }
    long resolutionStartTime = DEBUG_TIMING ? System.currentTimeMillis() : 0L;
    DartNode analyzedNode = DartCompilerUtilities.analyzeDelta(library, source, parsedUnit,
        resolvedNode, completionPosition, parseErrors);
    if (DEBUG_TIMING) {
      System.out.println("Code Assist (resolve library): "
          + (System.currentTimeMillis() - resolutionStartTime));
    }
    if (analyzedNode == null) {
      DartCore.logError("Could not resolve AST: " + fileName, null);
      for (DartCompilationError err : parseErrors) {
        DartCore.logError(err.getMessage(), null);
        System.out.println(err.getMessage());
        System.out.println(err.getSource().getUri());
      }
      return;
    }
    Scope unitScope = resolvedUnit.getLibrary().getElement().getScope();
    typeProvider = new CoreTypeProviderImplementation(unitScope,
        SilentDartCompilerListener.INSTANCE);

    classElement = null;
    if (resolvedMember != null) {
      EnclosingElement encElement = resolvedMember.getSymbol().getEnclosingElement();
      if (encElement instanceof ClassElement) {
        classElement = (ClassElement) encElement;
      }
    } else {
      DartClass resolvedClass = finder.getEnclosingClass();
      if (resolvedClass != null) {
        classElement = resolvedClass.getSymbol();
      }
    }
    context.reset();
    Resolver resolver = new Resolver(context, unitScope, typeProvider);

    ResolutionContext resolutionContext = new ResolutionContext(unitScope, context, typeProvider);
    if (classElement != null) {
      resolutionContext = resolutionContext.extend(classElement);
    }
    Element member = resolvedMember == null ? classElement : resolvedMember.getSymbol();
    try {
      resolver.resolveMember(classElement, member, resolutionContext);
    } catch (AssertionError ex) {
      // Expected if completing an extends or implements clause of type declaration
    } catch (NullPointerException ex) {
      // Expected if completing an extends or implements clause of type declaration
    } catch (Throwable ex) {
      DartCore.logError(ex);
      ex.printStackTrace();
    }
    requestor.beginReporting();
    requestor.acceptContext(new InternalCompletionContext());
    resolvedNode.accept(new OuterCompletionProposer(resolvedNode));
    requestor.endReporting();
  }

  public CompletionEnvironment getEnvironment() {
    return environment;
  }

  public IProgressMonitor getMonitor() {
    return monitor;
  }

  public WorkingCopyOwner getOwner() {
    return owner;
  }

  public DartProject getProject() {
    return project;
  }

  public CompletionRequestor getRequestor() {
    return requestor;
  }

  public HashMap<Object, Object> getTypeCache() {
    return typeCache;
  }

  private Type analyzeType(DartNode target) {
    InterfaceType currentType;
    if (classElement != null) {
      currentType = classElement.getType();
    } else {
      currentType = null;
    }
    Type type = TypeAnalyzer.analyze(target, typeProvider, context, currentType);
    if (TypeKind.of(type) == TypeKind.VOID || TypeKind.of(type) == TypeKind.DYNAMIC) {
      if (target instanceof DartIdentifier) {
        Element element = ((DartIdentifier) target).getTargetSymbol();
        type = element.getType();
      }
    }
    return type;
  }

  private void checkCancel() {
    if (monitor != null && monitor.isCanceled()) {
      throw new OperationCanceledException();
    }
  }

  private void createCompletionsForFactoryInvocation(DartIdentifier memberName, InterfaceType itype) {
    String prefix = extractFilterPrefix(memberName);
    List<Element> members = getConstructors(itype);
    if (!isCompletionAfterDot && memberName == null) {
      return;
    }
    for (Element elem : members) {
      MethodElement method = (MethodElement) elem;
      String name = method.getName();
      if (prefix != null && !name.startsWith(prefix)) {
        continue;
      }
      if (prefix == null && name.length() == 0) {
        continue;
      }
      int kind = CompletionProposal.METHOD_REF;
      InternalCompletionProposal proposal = (InternalCompletionProposal) CompletionProposal.create(
          kind, actualCompletionPosition - offset);
      proposal.setDeclarationSignature(method.getEnclosingElement().getName().toCharArray());
      proposal.setSignature(name.toCharArray());
      proposal.setCompletion(name.toCharArray());
      proposal.setName(name.toCharArray());
      proposal.setIsContructor(true);
      proposal.setIsGetter(false);
      proposal.setIsSetter(false);
      proposal.setParameterNames(getParameterNames(method));
      proposal.setParameterTypeNames(getParameterTypeNames(method));
      String returnTypeName = itype.getElement().getName();
      proposal.setTypeName(returnTypeName.toCharArray());
      proposal.setDeclarationTypeName(returnTypeName.toCharArray());
      setSourceLoc(proposal, memberName, prefix);
      proposal.setRelevance(1);
      requestor.accept(proposal);
    }
  }

  private void createCompletionsForLocalVariables(DartNode terminalNode, DartIdentifier node,
      DartClassMember<? extends DartExpression> method) {
    String prefix = extractFilterPrefix(node);
    LocalVariableFinder vars = new LocalVariableFinder();
    terminalNode.accept(vars);
    Map<String, LocalName> localNames = vars.getLocals();
    for (LocalName para : localNames.values()) {
      String name = para.getName();
      if (prefix != null && !name.startsWith(prefix)) {
        continue;
      }
      Element element = para.getSymbol();
      String typeName = ((InterfaceType) element.getType()).toString();
      InternalCompletionProposal proposal = (InternalCompletionProposal) CompletionProposal.create(
          CompletionProposal.LOCAL_VARIABLE_REF, actualCompletionPosition - offset);
      proposal.setSignature(typeName.toCharArray());
      proposal.setCompletion(name.toCharArray());
      proposal.setName(name.toCharArray());
      setSourceLoc(proposal, node, prefix);
      proposal.setRelevance(1);
      requestor.accept(proposal);
    }
  }

  private void createCompletionsForMethodInvocation(DartIdentifier node, Type type,
      boolean isQualifiedByThis, boolean isMethodStatic) {
    String prefix = extractFilterPrefix(node);
    InterfaceType itype = (InterfaceType) type;
    List<Element> members = getAllElements(itype);
    for (Element elem : members) {
      if (!(elem instanceof MethodElement)) {
        continue;
      }
      MethodElement method = (MethodElement) elem;
      boolean candidateMethodIsStatic = elem.getModifiers().isStatic();
      if (isMethodStatic && !candidateMethodIsStatic
          || (isQualifiedByThis && candidateMethodIsStatic)) {
        continue;
      }
      String name = method.getName();
      if (prefix != null && !name.startsWith(prefix)) {
        continue;
      }
      // TODO Filtering: No operators appear following '.'; only operators if no '.'
      boolean isOperator = elem.getModifiers().isOperator();
      if (isCompletionAfterDot && isOperator) {
        continue;
      }
      if (isOperator && node == null) {
        continue;
      }
      boolean isSetter = method.getModifiers().isSetter();
      boolean isGetter = method.getModifiers().isGetter();
      int kind = (isGetter || isSetter) ? CompletionProposal.FIELD_REF
          : CompletionProposal.METHOD_REF;
      InternalCompletionProposal proposal = (InternalCompletionProposal) CompletionProposal.create(
          kind, actualCompletionPosition - offset);
      proposal.setDeclarationSignature(method.getEnclosingElement().getName().toCharArray());
      proposal.setSignature(name.toCharArray());
      proposal.setCompletion(name.toCharArray());
      proposal.setName(name.toCharArray());
      proposal.setIsContructor(method.isConstructor());
      proposal.setIsGetter(isGetter);
      proposal.setIsSetter(isSetter);
      proposal.setParameterNames(getParameterNames(method));
      proposal.setParameterTypeNames(getParameterTypeNames(method));
      String returnTypeName;
      if (method.getReturnType().getKind() == TypeKind.DYNAMIC) {
        // TODO Remove this code when the element says it has void return type
        DartMethodDefinition methodNode = (DartMethodDefinition) method.getNode();
        try {
          returnTypeName = methodNode.getFunction().getReturnTypeNode().toString();
        } catch (NullPointerException ex) {
          returnTypeName = method.getReturnType().getElement().getName();
        }
      } else {
        returnTypeName = method.getReturnType().getElement().getName();
      }
      proposal.setTypeName(returnTypeName.toCharArray());
      proposal.setDeclarationTypeName(method.getEnclosingElement().getName().toCharArray());
      setSourceLoc(proposal, node, prefix);
      proposal.setRelevance(1);
      requestor.accept(proposal);
    }
  }

  private void createCompletionsForPropertyAccess(DartIdentifier node, Type type,
      boolean isQualifiedByThis, boolean isMethodStatic) {
    if (!(type instanceof InterfaceType)) {
      return;
    }
    String prefix = extractFilterPrefix(node);
    InterfaceType itype = (InterfaceType) type;
    List<Element> members = getAllElements(itype);
    for (Element elem : members) {
      if (!(elem instanceof FieldElement)) {
        continue;
      }
      FieldElement field = (FieldElement) elem;
      boolean fieldIsStatic = field.getModifiers().isStatic();
      if (isMethodStatic && !fieldIsStatic || isQualifiedByThis && fieldIsStatic) {
        continue;
      }
      String name = field.getName();
      if (prefix != null && !name.startsWith(prefix)) {
        continue;
      }
      InternalCompletionProposal proposal = (InternalCompletionProposal) CompletionProposal.create(
          CompletionProposal.FIELD_REF, actualCompletionPosition - offset);
      proposal.setDeclarationSignature(field.getEnclosingElement().getName().toCharArray());
      proposal.setSignature(name.toCharArray());
      proposal.setCompletion(name.toCharArray());
      proposal.setName(name.toCharArray());
      proposal.setIsContructor(false);
      proposal.setIsGetter(true);
      proposal.setIsSetter(true);
      proposal.setTypeName(field.getType().getElement().getName().toCharArray());
      proposal.setDeclarationTypeName(field.getEnclosingElement().getName().toCharArray());
      setSourceLoc(proposal, node, prefix);
      proposal.setRelevance(1);
      requestor.accept(proposal);
    }
  }

  private void createCompletionsForQualifiedMemberAccess(DartIdentifier memberName, Type type,
      boolean isInstance) {
    // At the completion point, the language allows both field and method access.
    // The parser needs more look-ahead to disambiguate. Those tokens may not have
    // been typed yet.
    createCompletionsForPropertyAccess(memberName, type, isInstance, false);
    createCompletionsForMethodInvocation(memberName, type, isInstance, false);
  }

  private void createCompletionsForStaticVariables(DartIdentifier identifier, DartClass classDef) {
    ClassElement elem = classDef.getSymbol();
    Type type = elem.getType();
    createCompletionsForPropertyAccess(identifier, type, false, false);
  }

  private void createProposalsForLiterals(DartNode node, String... names) {
    String prefix = extractFilterPrefix(node);
    for (String name : names) {
      InternalCompletionProposal proposal = (InternalCompletionProposal) CompletionProposal.create(
          CompletionProposal.LOCAL_VARIABLE_REF, actualCompletionPosition - offset);
      proposal.setSignature(name.toCharArray());
      proposal.setCompletion(name.toCharArray());
      proposal.setName(name.toCharArray());
      setSourceLoc(proposal, node, prefix);
      proposal.setRelevance(1);
      requestor.accept(proposal);
    }
  }

  private void createTypeCompletionsForConstructor(DartNode node, SearchMatch match, String prefix) {
    DartElement element = match.getElement();
    if (!(element instanceof com.google.dart.tools.core.model.Type)) {
      return;
    }
    boolean disallowPrivate = true;
    if (prefix != null) {
      disallowPrivate = !prefix.startsWith("_");
      if (prefix.length() == 0) {
        prefix = null;
      }
    }
    com.google.dart.tools.core.model.Type type = (com.google.dart.tools.core.model.Type) element;
    String name = type.getElementName();
    if (disallowPrivate && name.startsWith("_")) {
      return;
    }
    try {
      for (com.google.dart.tools.core.model.Method method : type.getMethods()) {
        if (method.isConstructor()) {
          InternalCompletionProposal proposal = (InternalCompletionProposal) CompletionProposal.create(
              CompletionProposal.METHOD_REF, actualCompletionPosition - offset);
          char[] declaringTypeName = method.getDeclaringType().getElementName().toCharArray();
          char[] methodName = name.toCharArray();
          proposal.setDeclarationSignature(declaringTypeName);
          proposal.setSignature(methodName);
          proposal.setCompletion(methodName);
          proposal.setName(methodName);
          proposal.setIsContructor(method.isConstructor());
          proposal.setIsGetter(false);
          proposal.setIsSetter(false);
          proposal.setParameterNames(getParameterNames(method));
          proposal.setParameterTypeNames(getParameterTypeNames(method));
          proposal.setTypeName(CharOperation.toCharArray(method.getReturnTypeName()));
          proposal.setDeclarationTypeName(declaringTypeName);
          setSourceLoc(proposal, node, prefix);
          proposal.setRelevance(1);
          requestor.setAllowsRequiredProposals(CompletionProposal.CONSTRUCTOR_INVOCATION,
              CompletionProposal.TYPE_REF, true);
          requestor.accept(proposal);
        }
      }
    } catch (DartModelException exception) {
      InternalCompletionProposal proposal = (InternalCompletionProposal) CompletionProposal.create(
          CompletionProposal.TYPE_REF, actualCompletionPosition - offset);
      char[] nameChars = name.toCharArray();
      proposal.setCompletion(nameChars);
      proposal.setSignature(nameChars);
      setSourceLoc(proposal, node, prefix);
      proposal.setRelevance(1);
      requestor.accept(proposal);
    }
  }

  private void createTypeCompletionsForParameterDecl(DartNode node, SearchMatch match, String prefix) {
    DartElement element = match.getElement();
    if (!(element instanceof com.google.dart.tools.core.model.Type)) {
      return;
    }
    boolean disallowPrivate = true;
    if (prefix != null) {
      disallowPrivate = !prefix.startsWith("_");
      if (prefix.length() == 0) {
        prefix = null;
      }
    }
    com.google.dart.tools.core.model.Type type = (com.google.dart.tools.core.model.Type) element;
    String name = type.getElementName();
    if (disallowPrivate && name.startsWith("_")) {
      return;
    }
    InternalCompletionProposal proposal = (InternalCompletionProposal) CompletionProposal.create(
        CompletionProposal.TYPE_REF, actualCompletionPosition - offset);
    char[] nameChars = name.toCharArray();
    proposal.setCompletion(nameChars);
    proposal.setSignature(nameChars);
    try {
      proposal.setIsInterface(type.isInterface());
    } catch (DartModelException ex) {
      // no one cares
    }
    setSourceLoc(proposal, node, prefix);
    proposal.setRelevance(1);
    requestor.accept(proposal);
  }

  private void createTypeCompletionsForTypeDecl(DartNode node, SearchMatch match, String prefix,
      boolean isClassOnly, boolean isInterfaceOnly) {
    DartElement element = match.getElement();
    if (!(element instanceof com.google.dart.tools.core.model.Type)) {
      return;
    }
    boolean disallowPrivate = true;
    if (prefix != null) {
      disallowPrivate = !prefix.startsWith("_");
      if (prefix.length() == 0) {
        prefix = null;
      }
    }
    com.google.dart.tools.core.model.Type type = (com.google.dart.tools.core.model.Type) element;
    boolean isInterface = false;
    try {
      isInterface = type.isInterface();
      if (isClassOnly && isInterface) {
        return;
      }
      if (isInterfaceOnly && !isInterface) {
        return;
      }
    } catch (DartModelException ex) {
      // no one cares
    }
    String name = type.getElementName();
    if (disallowPrivate && name.startsWith("_")) {
      return;
    }
    InternalCompletionProposal proposal = (InternalCompletionProposal) CompletionProposal.create(
        CompletionProposal.TYPE_REF, actualCompletionPosition - offset);
    char[] nameChars = name.toCharArray();
    proposal.setIsInterface(isInterface);
    proposal.setCompletion(nameChars);
    proposal.setSignature(nameChars);
    setSourceLoc(proposal, node, prefix);
    proposal.setRelevance(1);
    requestor.accept(proposal);
  }

  private String extractFilterPrefix(DartNode node) {
    if (node == null || isCompletionAfterDot) {
      return null;
    }
    int begin = node.getSourceStart();
    int dot = actualCompletionPosition + 1;
    String name = source.substring(begin, begin + node.getSourceLength());
    String prefix = name.substring(0, Math.min(name.length(), dot - begin));
    return prefix.length() == 0 ? null : prefix;
  }

  private List<SearchMatch> findTypesWithPrefix(DartIdentifier id) {
    SearchEngine engine = SearchEngineFactory.createSearchEngine();
    SearchScope scope = SearchScopeFactory.createWorkspaceScope();
    GatheringSearchListener listener = new GatheringSearchListener();
    String prefix = extractFilterPrefix(id);
    if (prefix == null) {
      prefix = "";
    }
    try {
      engine.searchTypeDeclarations(scope, SearchPatternFactory.createPrefixPattern(prefix, true),
          null, listener, new NullProgressMonitor());
    } catch (SearchException ex) {
      return null;
    }
    List<SearchMatch> matches = listener.getMatches();
    try {
      int idx = 0;
      // TODO remove this loop if the current comp unit has no additional types
      for (com.google.dart.tools.core.model.Type localType : getCurrentCompilationUnit().getTypes()) {
        String typeName = localType.getElementName();
        if (typeName.startsWith(prefix)) { // this test is case sensitive
          SearchMatch match = new SearchMatch(MatchQuality.EXACT, MatchKind.NOT_A_REFERENCE,
              localType, localType.getSourceRange());
          boolean found = false;
          for (SearchMatch foundMatch : matches) {
            if (foundMatch.getElement().getElementName().equals(typeName)) {
              found = true;
              break;
            }
          }
          if (!found) {
            matches.add(idx++, match);
          }
        }
      }
    } catch (DartModelException ex) {
      // no one cares
    } catch (NullPointerException ex) {
      // happens during tests because currentCompilationUnit is null
    }
    return matches;
  }

  private CompilationUnit getCurrentCompilationUnit() {
    // TODO verify that this is actually useful -- if type search finds local defs it is not
    return currentCompilationUnit;
  }

  private void proposeClassOrInterfaceNamesForPrefix(DartIdentifier identifier, boolean isClass) {
    List<SearchMatch> matches = findTypesWithPrefix(identifier);
    if (matches == null || matches.size() == 0) {
      return;
    }
    String prefix = extractFilterPrefix(identifier);
    for (SearchMatch match : matches) {
      createTypeCompletionsForTypeDecl(identifier, match, prefix, isClass, !isClass);
    }
  }

  private void proposeTypesForNewParam() {
    // TODO Combine with proposeTypesForPrefix()
    List<SearchMatch> matches = findTypesWithPrefix(null);
    if (matches == null || matches.size() == 0) {
      return;
    }
    for (SearchMatch match : matches) {
      createTypeCompletionsForParameterDecl(null, match, "");
    }
  }

  private void proposeTypesForPrefix(DartIdentifier identifier) {
    proposeTypesForPrefix(identifier, true);
  }

  private void proposeTypesForPrefix(DartIdentifier identifier, boolean allowVoid) {
    List<SearchMatch> matches = findTypesWithPrefix(identifier);
    if (matches == null || matches.size() == 0) {
      return;
    }
    String prefix = extractFilterPrefix(identifier);
    for (SearchMatch match : matches) {
      createTypeCompletionsForParameterDecl(identifier, match, prefix);
    }
    if (allowVoid) {
      if (prefix == null || prefix.length() == 0) {
        createProposalsForLiterals(identifier, "void");
      } else {
        String id = identifier.getTargetName();
        if (id.length() <= "void".length() && "void".startsWith(id)) {
          createProposalsForLiterals(identifier, "void");
        }
      }
    }
  }

  private void proposeVariables(DartNode completionNode, DartIdentifier identifier,
      DartClassMember<? extends DartExpression> method) {
    createCompletionsForLocalVariables(completionNode, identifier, method);
    DartClass classDef = (DartClass) method.getParent();
    createCompletionsForStaticVariables(identifier, classDef);
  }

  private void setSourceLoc(InternalCompletionProposal proposal, DartNode name, String prefix) {
    // Bug in source positions causes name node to have its parent's source locations.
    // That causes sourceLoc to be incorrect, which also causes completion list to close
    // when the next char is typed rather than filtering the list based on that char.
    // It also causes editing to fail when a completion is selected.
    if (prefix == null) {
      name = null;
    }
    int sourceLoc = (name == null) ? actualCompletionPosition + 1 : name.getSourceStart();
    int length = (name == null) ? 0 : name.getSourceLength();
    proposal.setReplaceRange(sourceLoc - offset, length + sourceLoc - offset);
    proposal.setTokenRange(sourceLoc - offset, length + sourceLoc - offset);
  }

  private void visitorNotImplementedYet(DartNode node, DartNode sourceNode,
      Class<? extends DartNodeTraverser<Void>> astClass) {
    // TODO Remove debugging println, or convert to trace output
    System.out.print("Need visitor for node: " + node.getClass().getSimpleName());
    if (sourceNode != node) {
      System.out.print(" for " + sourceNode.getClass().getSimpleName());
    }
    System.out.println(" in " + astClass.getSimpleName());
  }
}
