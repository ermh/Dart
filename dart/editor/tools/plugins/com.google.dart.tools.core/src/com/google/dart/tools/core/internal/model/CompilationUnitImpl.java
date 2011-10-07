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
package com.google.dart.tools.core.internal.model;

import com.google.dart.compiler.DartCompilationError;
import com.google.dart.compiler.ast.DartClass;
import com.google.dart.compiler.ast.DartContext;
import com.google.dart.compiler.ast.DartExpression;
import com.google.dart.compiler.ast.DartField;
import com.google.dart.compiler.ast.DartFieldDefinition;
import com.google.dart.compiler.ast.DartFunction;
import com.google.dart.compiler.ast.DartFunctionExpression;
import com.google.dart.compiler.ast.DartIdentifier;
import com.google.dart.compiler.ast.DartLibraryDirective;
import com.google.dart.compiler.ast.DartMethodDefinition;
import com.google.dart.compiler.ast.DartNode;
import com.google.dart.compiler.ast.DartParameter;
import com.google.dart.compiler.ast.DartPropertyAccess;
import com.google.dart.compiler.ast.DartTypeNode;
import com.google.dart.compiler.ast.DartUnit;
import com.google.dart.compiler.ast.DartVariable;
import com.google.dart.compiler.ast.DartVariableStatement;
import com.google.dart.compiler.ast.DartVisitor;
import com.google.dart.compiler.ast.Modifiers;
import com.google.dart.compiler.resolver.ClassElement;
import com.google.dart.compiler.resolver.Element;
import com.google.dart.compiler.type.FunctionAliasType;
import com.google.dart.compiler.type.FunctionType;
import com.google.dart.compiler.type.InterfaceType;
import com.google.dart.compiler.type.TypeVariable;
import com.google.dart.tools.core.DartCore;
import com.google.dart.tools.core.buffer.Buffer;
import com.google.dart.tools.core.completion.CompletionRequestor;
import com.google.dart.tools.core.internal.buffer.BufferManager;
import com.google.dart.tools.core.internal.model.info.ASTHolderCUInfo;
import com.google.dart.tools.core.internal.model.info.CompilationUnitInfo;
import com.google.dart.tools.core.internal.model.info.DartElementInfo;
import com.google.dart.tools.core.internal.model.info.DartFieldInfo;
import com.google.dart.tools.core.internal.model.info.DartFunctionInfo;
import com.google.dart.tools.core.internal.model.info.DartFunctionTypeAliasInfo;
import com.google.dart.tools.core.internal.model.info.DartMethodInfo;
import com.google.dart.tools.core.internal.model.info.DartTypeInfo;
import com.google.dart.tools.core.internal.model.info.DartVariableInfo;
import com.google.dart.tools.core.internal.model.info.OpenableElementInfo;
import com.google.dart.tools.core.internal.operation.BecomeWorkingCopyOperation;
import com.google.dart.tools.core.internal.operation.CommitWorkingCopyOperation;
import com.google.dart.tools.core.internal.operation.DiscardWorkingCopyOperation;
import com.google.dart.tools.core.internal.operation.ReconcileWorkingCopyOperation;
import com.google.dart.tools.core.internal.problem.CategorizedProblem;
import com.google.dart.tools.core.internal.util.CharOperation;
import com.google.dart.tools.core.internal.util.MementoTokenizer;
import com.google.dart.tools.core.internal.util.Messages;
import com.google.dart.tools.core.internal.util.Util;
import com.google.dart.tools.core.internal.workingcopy.DefaultWorkingCopyOwner;
import com.google.dart.tools.core.model.CompilationUnit;
import com.google.dart.tools.core.model.DartConventions;
import com.google.dart.tools.core.model.DartElement;
import com.google.dart.tools.core.model.DartFunctionTypeAlias;
import com.google.dart.tools.core.model.DartLibrary;
import com.google.dart.tools.core.model.DartModelException;
import com.google.dart.tools.core.model.DartModelStatusConstants;
import com.google.dart.tools.core.model.DartVariableDeclaration;
import com.google.dart.tools.core.model.Type;
import com.google.dart.tools.core.problem.ProblemRequestor;
import com.google.dart.tools.core.utilities.ast.DartElementLocator;
import com.google.dart.tools.core.utilities.compiler.DartCompilerUtilities;
import com.google.dart.tools.core.utilities.performance.PerformanceManager;
import com.google.dart.tools.core.workingcopy.WorkingCopyOwner;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.PerformanceStats;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Instances of the class <code>CompilationUnitImpl</code> implement the representation of files
 * containing Dart source code that needs to be compiled.
 */
public class CompilationUnitImpl extends SourceFileElementImpl<CompilationUnit> implements
    CompilationUnit {
  /**
   * Instances of the class <code>CompilationUnitStructureBuilder</code> are used to build the
   * structure corresponding to a compilation unit by visiting the AST structure for the compilation
   * unit.
   */
  private static class CompilationUnitStructureBuilder extends StructureBuilder {
    /**
     * The compilation unit in which the top level elements are declared.
     */
    private CompilationUnitImpl compilationUnit;

    /**
     * A flag indicating whether this compilation unit contains a #library directive.
     */
    private boolean definesLibrary = false;

    /**
     * A list containing all of the top level elements that were found while parsing the compilation
     * unit.
     */
    private ArrayList<DartElementImpl> topLevelElements = new ArrayList<DartElementImpl>();

    /**
     * Initialize a newly created type finder to find top level elements within the given
     * compilation unit.
     * 
     * @param compilationUnit the compilation unit in which the top level elements are declared
     * @param newElements the map to which new elements are to be added
     */
    public CompilationUnitStructureBuilder(CompilationUnitImpl compilationUnit,
        Map<DartElement, DartElementInfo> newElements) {
      super(newElements);
      this.compilationUnit = compilationUnit;
    }

    @Override
    public void endVisit(DartUnit node, DartContext ctx) {
      try {
        CompilationUnitInfo info = (CompilationUnitInfo) compilationUnit.getElementInfo();
        info.setDefinesLibrary(definesLibrary);
        info.setChildren(topLevelElements.toArray(new DartElement[topLevelElements.size()]));
      } catch (DartModelException exception) {
        // This should never happen
      }
    }

    /**
     * Parse a top-level function type alias and add a DartFunctionTypeAlias as a child of the
     * compilation unit to represent it.
     */
    @Override
    public boolean visit(com.google.dart.compiler.ast.DartFunctionTypeAlias node, DartContext ctx) {
      DartFunctionTypeAliasImpl aliasImpl = new DartFunctionTypeAliasImpl(compilationUnit,
          node.getName().getTargetName());
      DartFunctionTypeAliasInfo aliasInfo = new DartFunctionTypeAliasInfo();
      int start = node.getSourceStart();
      aliasInfo.setSourceRangeStart(start);
      aliasInfo.setSourceRangeEnd(start + node.getSourceLength() - 1);
      aliasInfo.setNameRange(new SourceRangeImpl(node.getName()));
      aliasInfo.setReturnTypeName(extractTypeName(node.getReturnTypeNode(), false));
      List<DartElementImpl> parameters = getParameters(aliasImpl, node.getParameters());
      aliasInfo.setChildren(parameters.toArray(new DartElementImpl[parameters.size()]));
      newElements.put(aliasImpl, aliasInfo);
      topLevelElements.add(aliasImpl);
      return false;
    }

    /**
     * Parse a top-level type and add a Type as a child of the compilation unit to represent it.
     */
    @Override
    public boolean visit(DartClass node, DartContext ctx) {
      String className = node.getClassName();
      DartTypeImpl typeImpl = new DartTypeImpl(compilationUnit, className);
      DartTypeInfo typeInfo = new DartTypeInfo();
      ArrayList<DartElementImpl> children = new ArrayList<DartElementImpl>();
      newElements.put(typeImpl, typeInfo);

      boolean constructorFound = false;
      List<DartNode> members = node.getMembers();
      for (DartNode member : members) {
        if (member instanceof DartFieldDefinition) {
          DartFieldDefinition fieldListNode = (DartFieldDefinition) member;
          for (DartField fieldNode : fieldListNode.getFields()) {
            DartFieldImpl fieldImpl = new DartFieldImpl(typeImpl, fieldNode.getName().toString());
            DartFieldInfo fieldInfo = new DartFieldInfo();
            fieldInfo.setSourceRangeStart(fieldNode.getSourceStart());
            fieldInfo.setSourceRangeEnd(fieldNode.getSourceStart() + fieldNode.getSourceLength());
            fieldInfo.setNameRange(new SourceRangeImpl(fieldNode.getName()));
            fieldInfo.setTypeName(extractTypeName(fieldListNode.getType(), false));
            fieldInfo.setModifiers(fieldNode.getModifiers());
            children.add(fieldImpl);
            newElements.put(fieldImpl, fieldInfo);

            FunctionGatherer functionGatherer = new FunctionGatherer(fieldNode, fieldImpl,
                newElements);
            functionGatherer.accept(fieldNode);
            List<DartFunctionImpl> functions = functionGatherer.getFunctions();
            fieldInfo.setChildren(functions.toArray(new DartElementImpl[functions.size()]));
          }
        } else if (member instanceof DartMethodDefinition) {
          DartMethodDefinition methodNode = (DartMethodDefinition) member;
          DartMethodImpl methodImpl = new DartMethodImpl(typeImpl, methodNode.getName().toString());
          DartMethodInfo methodInfo = new DartMethodInfo();
          methodInfo.setSourceRangeStart(methodNode.getSourceStart());
          methodInfo.setSourceRangeEnd(methodNode.getSourceStart() + methodNode.getSourceLength());
          methodInfo.setNameRange(new SourceRangeImpl(methodNode.getName()));
          methodInfo.setModifiers(methodNode.getModifiers());
          boolean isConstructor = isConstructor(className, methodNode);
          methodInfo.setConstructor(isConstructor);
          constructorFound = constructorFound || isConstructor;
          methodInfo.setReturnTypeName(extractTypeName(
              methodNode.getFunction().getReturnTypeNode(), false));
          children.add(methodImpl);
          newElements.put(methodImpl, methodInfo);

          List<DartElementImpl> methodChildren = getParameters(methodImpl, methodNode.getFunction());

          FunctionGatherer functionGatherer = new FunctionGatherer(methodNode, methodImpl,
              newElements);
          functionGatherer.accept(methodNode);
          methodChildren.addAll(functionGatherer.getFunctions());

          LocalVariableGatherer variableGatherer = new LocalVariableGatherer(methodNode,
              methodImpl, newElements);
          variableGatherer.accept(methodNode);
          methodChildren.addAll(variableGatherer.getLocalVariables());

          methodInfo.setChildren(methodChildren.toArray(new DartElementImpl[methodChildren.size()]));
        } else {
          // This should never happen, but if it does we need to know about it.
          DartCore.logError("Unexpected type of node found as member of type: "
              + member.getClass().getName(), new Throwable());
        }
      }
      boolean isInterface = node.isInterface();
      if (!constructorFound && !isInterface) {
        DartIdentifier typeName = node.getName();
        if (typeName != null) {
          DartMethodImpl methodImpl = new DartMethodImpl(typeImpl, typeName.getTargetName());
          DartMethodInfo methodInfo = new DartMethodInfo();
          methodInfo.setSourceRangeStart(typeName.getSourceStart());
          methodInfo.setSourceRangeEnd(typeName.getSourceStart() + typeName.getSourceLength());
          methodInfo.setNameRange(new SourceRangeImpl(typeName));
          methodInfo.setModifiers(Modifiers.NONE);
          methodInfo.setConstructor(true);
          methodInfo.setImplicit(true);
          methodInfo.setReturnTypeName(typeName.getTargetName().toCharArray());
          children.add(methodImpl);
          newElements.put(methodImpl, methodInfo);
        }
      }

      typeInfo.setIsInterface(isInterface);
      typeInfo.setSuperclassName(extractTypeName(node.getSuperclass(), false));
      typeInfo.setInterfaceNames(extractTypeNames(node.getInterfaces(), false));
      typeInfo.setSourceRangeStart(node.getSourceStart());
      typeInfo.setSourceRangeEnd(node.getSourceStart() + node.getSourceLength());
      DartIdentifier typeName = node.getName();
      typeInfo.setNameRange(new SourceRangeImpl(typeName));
      typeInfo.setChildren(children.toArray(new DartElementImpl[children.size()]));
      topLevelElements.add(typeImpl);
      return false;
    }

    /**
     * Parse a top-level variable and add a DartVariableImpl as a child of the compilation unit to
     * represent it.
     */
    @Override
    public boolean visit(DartFieldDefinition node, DartContext ctx) {
      for (DartField fieldNode : node.getFields()) {
        DartVariableImpl variableImpl = new DartVariableImpl(compilationUnit,
            fieldNode.getName().toString());
        DartVariableInfo variableInfo = new DartVariableInfo();
        variableInfo.setSourceRangeStart(fieldNode.getSourceStart());
        variableInfo.setSourceRangeEnd(fieldNode.getSourceStart() + fieldNode.getSourceLength());
        variableInfo.setNameRange(new SourceRangeImpl(fieldNode.getName()));
        variableInfo.setTypeName(extractTypeName(node.getType(), false));
        variableInfo.setModifiers(fieldNode.getModifiers());

        FunctionGatherer functionGatherer = new FunctionGatherer(fieldNode, variableImpl,
            newElements);
        functionGatherer.accept(fieldNode);
        List<DartFunctionImpl> functions = functionGatherer.getFunctions();
        variableInfo.setChildren(functions.toArray(new DartElementImpl[functions.size()]));

        newElements.put(variableImpl, variableInfo);
        topLevelElements.add(variableImpl);
      }
      return false;
    }

    /**
     * Record the existence of the #library directive.
     */
    @Override
    public boolean visit(DartLibraryDirective node, DartContext ctx) {
      definesLibrary = true;
      return false;
    }

    /**
     * Parse a top-level function, getter or setter and add a DartFunction as a child of the
     * compilation unit to represent it.
     */
    @Override
    public boolean visit(DartMethodDefinition node, DartContext ctx) {
      DartExpression functionName = node.getName();
      DartFunctionImpl functionImpl = new DartFunctionImpl(compilationUnit, functionName == null
          ? null : ((DartIdentifier) functionName).getTargetName());
      DartFunctionInfo functionInfo = new DartFunctionInfo();
      int start = node.getSourceStart();
      functionInfo.setSourceRangeStart(start);
      functionInfo.setSourceRangeEnd(start + node.getSourceLength() - 1);
      functionInfo.setNameRange(functionName == null ? null : new SourceRangeImpl(functionName));
      functionInfo.setReturnTypeName(extractTypeName(node.getFunction().getReturnTypeNode(), false));

      List<DartElementImpl> functionChildren = getParameters(functionImpl, node.getFunction());

      LocalVariableGatherer variableGatherer = new LocalVariableGatherer(node, functionImpl,
          newElements);
      variableGatherer.accept(node);
      functionChildren.addAll(variableGatherer.getLocalVariables());

      FunctionGatherer functionGatherer = new FunctionGatherer(node, functionImpl, newElements);
      functionGatherer.accept(node);
      functionChildren.addAll(functionGatherer.getFunctions());

      functionInfo.setChildren(functionChildren.toArray(new DartElementImpl[functionChildren.size()]));

      newElements.put(functionImpl, functionInfo);
      topLevelElements.add(functionImpl);
      return false;
    }

    /**
     * Return <code>true</code> if the given method definition is defining a constructor in a class
     * with the given name.
     * 
     * @param className the name of the class containing the method definition
     * @param method the method definition being tested
     * @return <code>true</code> if the method defines a constructor
     */
    private boolean isConstructor(String className, DartMethodDefinition method) {
      if (method.getModifiers().isFactory()) {
        return true;
      }
      DartExpression name = method.getName();
      if (name instanceof DartIdentifier) {
        return ((DartIdentifier) name).getTargetName().equals(className);
      } else if (name instanceof DartPropertyAccess) {
        DartPropertyAccess property = (DartPropertyAccess) name;
        DartNode qualifier = property.getQualifier();
        if (qualifier instanceof DartIdentifier) {
          return ((DartIdentifier) qualifier).getTargetName().equals(className);
        }
      }
      return false;
    }
  }

  private static class FunctionGatherer extends StructureBuilder {
    private int functionCount;

    /**
     * A list containing the function elements that were created.
     */
    private ArrayList<DartFunctionImpl> functions = new ArrayList<DartFunctionImpl>();

    /**
     * The parent of the function elements being created.
     */
    private DartElementImpl parentElement;

    /**
     * The top-level node being visited in search of functions.
     */
    private DartNode parentNode;

    /**
     * Initialize a newly created gatherer to find the functions declared as direct children of the
     * given parent node.
     * 
     * @param parentNode the parent node in which functions can be found
     * @param parent the element corresponding to the node
     * @param newElements the map to which new elements are to be added
     */
    public FunctionGatherer(DartNode parentNode, DartElementImpl parent,
        Map<DartElement, DartElementInfo> newElements) {
      super(newElements);
      this.parentNode = parentNode;
      this.parentElement = parent;
      this.functionCount = 1; // function count is relative to parent node
    }

    /**
     * Return a list containing all of the functions that have been found.
     * 
     * @return a list containing all of the functions that have been found
     */
    public List<DartFunctionImpl> getFunctions() {
      return functions;
    }

    @Override
    public boolean visit(DartFunctionExpression node, DartContext ctx) {
      if (node == parentNode) {
        return true;
      }
      DartFunctionImpl functionImpl = new DartFunctionImpl(parentElement, node.getFunctionName());
      functionImpl.occurrenceCount = functionCount++;
      DartFunctionInfo functionInfo = new DartFunctionInfo();
      functionInfo.setSourceRangeStart(node.getSourceStart());
      functionInfo.setSourceRangeEnd(node.getSourceStart() + node.getSourceLength());
      DartIdentifier name = node.getName();
      if (name != null) {
        functionInfo.setNameRange(new SourceRangeImpl(name));
      }
      functionInfo.setReturnTypeName(extractTypeName(node.getFunction().getReturnTypeNode(), false));

      List<DartElementImpl> functionChildren = getParameters(functionImpl, node.getFunction());

      FunctionGatherer functionGatherer = new FunctionGatherer(node, functionImpl, newElements);
      functionGatherer.accept(node);
      functionChildren.addAll(functionGatherer.getFunctions());

      functionInfo.setChildren(functionChildren.toArray(new DartElementImpl[functionChildren.size()]));

      newElements.put(functionImpl, functionInfo);
      functions.add(functionImpl);
      return false;
    }
  }

  private static class LocalVariableGatherer extends StructureBuilder {
    /**
     * A list containing the function elements that were created.
     */
    private ArrayList<DartVariableImpl> variables = new ArrayList<DartVariableImpl>();

    /**
     * The parent of the function elements being created.
     */
    private DartElementImpl parentElement;

    /**
     * Initialize a newly created gatherer to find the functions declared as direct children of the
     * given parent node.
     * 
     * @param parentNode the parent node in which functions can be found
     * @param parent the element corresponding to the node
     * @param newElements the map to which new elements are to be added
     */
    public LocalVariableGatherer(DartNode parentNode, DartElementImpl parent,
        Map<DartElement, DartElementInfo> newElements) {
      super(newElements);
      //this.parentNode = parentNode;
      this.parentElement = parent;
    }

    /**
     * Return a list containing all of the local variables that have been found.
     * 
     * @return a list containing all of the local variables that have been found
     */
    public ArrayList<DartVariableImpl> getLocalVariables() {
      return variables;
    }

    @Override
    public boolean visit(DartFunctionExpression node, DartContext ctx) {
      return false;
    }

    @Override
    public boolean visit(DartVariableStatement node, DartContext ctx) {
      for (DartVariable variable : node.getVariables()) {
        DartVariableImpl variableImpl = new DartVariableImpl(parentElement, new String(
            variable.getVariableName().toCharArray()));
        DartVariableInfo variableInfo = new DartVariableInfo();
        DartExpression variableName = variable.getName();
        int start = variable.getSourceStart();
        variableInfo.setSourceRangeStart(start);
        variableInfo.setSourceRangeEnd(start + variable.getSourceLength() - 1);
        variableInfo.setNameRange(new SourceRangeImpl(variableName.getSourceStart(),
            variableName.getSourceLength()));
        variableInfo.setParameter(false);
        char[] typeName = extractTypeName(variable.getType(), true);
        variableInfo.setTypeName(typeName == null ? CharOperation.NO_CHAR : typeName);

        FunctionGatherer functionGatherer = new FunctionGatherer(variable, variableImpl,
            newElements);
        functionGatherer.accept(variable);
        List<DartFunctionImpl> functions = functionGatherer.getFunctions();
        variableInfo.setChildren(functions.toArray(new DartElementImpl[functions.size()]));

        newElements.put(variableImpl, variableInfo);
        variables.add(variableImpl);
      }
      return true;
    }
  }

  /**
   * The abstract class <code>StructureBuilder</code> are used to build the structure corresponding
   * to a compilation unit by visiting the AST structure for the compilation unit.
   */
  private static abstract class StructureBuilder extends DartVisitor {
    /**
     * A table mapping newly created elements to the info objects that correspond to them.
     */
    protected Map<DartElement, DartElementInfo> newElements;

    /**
     * Initialize a newly created structure builder to add elements that are built to the given map.
     * 
     * @param newElements
     */
    public StructureBuilder(Map<DartElement, DartElementInfo> newElements) {
      this.newElements = newElements;
    }

    /**
     * Create the parameters declared by the given function node.
     * 
     * @param parent the element that should be the parent of the parameters that are created
     * @param functionNode the function node in which the parameters are declared
     * @return a list containing all of the parameters that were created
     */
    protected List<DartElementImpl> getParameters(DartElementImpl parent, DartFunction functionNode) {
      return getParameters(parent, functionNode.getParams());
    }

    /**
     * Create the parameters declared by the given function node.
     * 
     * @param parent the element that should be the parent of the parameters that are created
     * @param parameterNodes the nodes for the parameters that are declared
     * @return a list containing all of the parameters that were created
     */
    protected List<DartElementImpl> getParameters(DartElementImpl parent,
        List<DartParameter> parameterNodes) {
      ArrayList<DartElementImpl> parameters = new ArrayList<DartElementImpl>();
      for (DartParameter parameter : parameterNodes) {
        DartVariableImpl variableImpl = new DartVariableImpl(parent, new String(
            parameter.getParameterName().toCharArray()));
        DartVariableInfo variableInfo = new DartVariableInfo();
        DartExpression parameterName = parameter.getName();
        int start = parameter.getSourceStart();
        variableInfo.setSourceRangeStart(start);
        variableInfo.setSourceRangeEnd(start + parameter.getSourceLength() - 1);
        variableInfo.setNameRange(new SourceRangeImpl(parameterName.getSourceStart(),
            parameterName.getSourceLength()));
        variableInfo.setParameter(true);
        char[] typeName = extractTypeName(parameter.getTypeNode(), true);
        // If function parameters are defined, then append the parameters to the type name.
        List<DartParameter> functionParameters = parameter.getFunctionParameters();
        if (functionParameters != null) {
          typeName = getParameterTypes(typeName, functionParameters);
        }
        variableInfo.setTypeName(typeName == null ? CharOperation.NO_CHAR : typeName);

        FunctionGatherer functionGatherer = new FunctionGatherer(parameter, variableImpl,
            newElements);
        functionGatherer.accept(parameter);
        List<DartFunctionImpl> functions = functionGatherer.getFunctions();
        variableInfo.setChildren(functions.toArray(new DartElementImpl[functions.size()]));

        newElements.put(variableImpl, variableInfo);
        parameters.add(variableImpl);
      }
      return parameters;
    }
  }

  /**
   * An empty array of compilation units.
   */
  public static final CompilationUnitImpl[] EMPTY_ARRAY = new CompilationUnitImpl[0];

  /**
   * The default type name in Dart.
   */
  private static final char[] VAR = "var".toCharArray();

  /**
   * The id for the code select operation.
   */
  private static final String CODE_SELECT_ID = DartCore.PLUGIN_ID + ".codeSelect";

  /**
   * Given a node that is used for the name of some other node, extract the textual representation
   * of the name from the node.
   * 
   * @param identifier the node used as the identifier for another node
   * @return the name represented by the given node
   */
  private static String extractName(DartNode identifier) {
    if (identifier instanceof DartIdentifier) {
      return ((DartIdentifier) identifier).getTargetName();
    } else if (identifier instanceof DartPropertyAccess) {
      DartPropertyAccess access = (DartPropertyAccess) identifier;
      return extractName(access.getQualifier()) + access.getName().getTargetName();
    }
    throw new IllegalArgumentException();
  }

  /**
   * Extracts and returns the name of the given type.
   * 
   * @param type the type whose name will be returned
   * @param returnStrVar if <code>true</code>, <code>"var"</code> is returned instead of
   *          <code>null</code> in cases where the name can't be extracted
   * @return the name of the given type
   */
  private static char[] extractTypeName(com.google.dart.compiler.type.Type type,
      boolean returnStrVar) {
    Element element = null;
    List<? extends com.google.dart.compiler.type.Type> typeParmeters = null;
    List<? extends com.google.dart.compiler.type.Type> parmeterTypes = null;
    switch (type.getKind()) {
      case DYNAMIC:
        break;
      case FUNCTION:
        FunctionType functionType = (FunctionType) type;
        element = functionType.getElement();
        parmeterTypes = functionType.getParameterTypes();
        break;
      case FUNCTION_ALIAS:
        element = ((FunctionAliasType) type).getElement();
        break;
      case INTERFACE:
        ClassElement classElement = ((InterfaceType) type).getElement();
        element = classElement;
        typeParmeters = classElement.getTypeParameters();
        break;
      case NONE:
        break;
      case VARIABLE:
        element = ((TypeVariable) type).getTypeVariableElement();
        break;
    }
    if (element != null) {
      String typeName = element.getName();
      if (typeName != null) {
        if (typeParmeters != null && !typeParmeters.isEmpty()) {
          StringBuilder builder = new StringBuilder();
          builder.append(typeName);
          builder.append('<');
          boolean needsSeparator = false;
          for (com.google.dart.compiler.type.Type typeParmeter : typeParmeters) {
            builder.append(extractTypeName(typeParmeter, true));
            if (needsSeparator) {
              builder.append(',');
            } else {
              needsSeparator = true;
            }
          }
          builder.append('>');
          typeName = builder.toString();
        }
        if (parmeterTypes != null) {
          StringBuilder builder = new StringBuilder();
          builder.append(typeName);
          builder.append('(');
          boolean needsSeparator = false;
          for (com.google.dart.compiler.type.Type parmeterType : parmeterTypes) {
            builder.append(extractTypeName(parmeterType, true));
            if (needsSeparator) {
              builder.append(',');
            } else {
              needsSeparator = true;
            }
          }
          builder.append(')');
          typeName = builder.toString();
        }
        return typeName.toCharArray();
      }
    }
    return returnStrVar ? VAR : null;
  }

  /**
   * Extracts and returns the name of the given type node.
   * 
   * @param type the type whose name will be returned
   * @param returnStrVar if <code>true</code>, <code>"var"</code> is returned instead of
   *          <code>null</code> in cases where the name can't be extracted
   * @return the name of the given type
   */
  private static char[] extractTypeName(DartTypeNode type, boolean returnStrVar) {
    String typeName;
    if (type == null) {
      return returnStrVar ? VAR : null;
    }
    DartNode id = type.getIdentifier();
    if (id == null) {
      return returnStrVar ? VAR : null;
    }
    if (id instanceof DartPropertyAccess) {
      DartPropertyAccess p = (DartPropertyAccess) id;
      DartIdentifier q = (DartIdentifier) p.getQualifier();
      DartIdentifier n = p.getName();
      typeName = q.getTargetName() + "." + n.getTargetName();
    } else {
      typeName = ((DartIdentifier) id).getTargetName();
    }
    List<DartTypeNode> typeArgs = type.getTypeArguments();
    if (typeArgs != null && !typeArgs.isEmpty()) {
      StringBuilder builder = new StringBuilder(60);
      builder.append(typeName);
      builder.append('<');
      for (Iterator<DartTypeNode> iterator = typeArgs.iterator(); iterator.hasNext();) {
        DartTypeNode dartTypeNode = iterator.next();
        builder.append(extractTypeName(dartTypeNode, true));
        if (iterator.hasNext()) {
          builder.append(',');
        }
      }
      builder.append('>');
      typeName = builder.toString();
    }
    return typeName.toCharArray();
  }

  /**
   * Extracts and returns the names of the given type nodes.
   * 
   * @param types the types whose name will be returned
   * @param returnStrVar <code>true</code> if <code>"var"</code> should be returned instead of
   *          <code>null</code> in cases where the name can't be extracted
   * @return the names of the given types
   */
  private static char[][] extractTypeNames(List<DartTypeNode> types, boolean returnStrVar) {
    if (types == null) {
      return CharOperation.NO_CHAR_CHAR;
    }
    List<char[]> typeNames = new ArrayList<char[]>();
    for (DartTypeNode type : types) {
      typeNames.add(extractTypeName(type, returnStrVar));
    }
    return typeNames.toArray(new char[typeNames.size()][]);
  }

  private static char[] getParameterTypes(char[] typeName, List<DartParameter> parameters) {
    StringBuilder builder = new StringBuilder(16);
    builder.append(typeName);
    getParameterTypes(builder, parameters);
    return builder.toString().toCharArray();
  }

  private static void getParameterTypes(StringBuilder builder, List<DartParameter> parameters) {
    builder.append('(');
    for (Iterator<DartParameter> iterator = parameters.iterator(); iterator.hasNext();) {
      DartParameter dartParameter = iterator.next();
      List<DartParameter> functionParameters = dartParameter.getFunctionParameters();
      char[] typeName = extractTypeName(dartParameter.getTypeNode(), true);
      // append the type name
      builder.append(typeName);
      if (functionParameters != null) {
        // If there are nested function parameters, then recursively append the
        // functionParameters to the builder.
        getParameterTypes(builder, functionParameters);
      }
      // if this is not the last dartParameter in the list, append ", "
      if (iterator.hasNext()) {
        builder.append(", ");
      }
    }
    builder.append(')');
  }

  /**
   * Initialize a newly created compilation unit to be an element in the given container.
   * 
   * @param container the library or library folder containing the compilation unit
   * @param file the file represented by the compilation unit
   * @param owner the working copy owner
   */
  public CompilationUnitImpl(CompilationUnitContainer container, IFile file, WorkingCopyOwner owner) {
    super(container, file, owner);
  }

  @Override
  public void becomeWorkingCopy(ProblemRequestor problemRequestor, IProgressMonitor monitor)
      throws DartModelException {
    DartModelManager manager = DartModelManager.getInstance();
    PerWorkingCopyInfo perWorkingCopyInfo = manager.getPerWorkingCopyInfo(this, false, true, null);
    if (perWorkingCopyInfo == null) {
      // close cu and its children
      close();
      BecomeWorkingCopyOperation operation = new BecomeWorkingCopyOperation(this, problemRequestor);
      operation.runOperation(monitor);
    }
  }

  @Override
  public boolean canBeRemovedFromCache() {
    if (getPerWorkingCopyInfo() != null) {
      // working copies should remain in the cache until they are destroyed
      return false;
    }
    return super.canBeRemovedFromCache();
  }

  @Override
  public boolean canBufferBeRemovedFromCache(Buffer buffer) {
    if (getPerWorkingCopyInfo() != null) {
      // working copy buffers should remain in the cache until working copy is
      // destroyed
      return false;
    }
    return super.canBufferBeRemovedFromCache(buffer);
  }

  /**
   * Clone this handle so that it caches its contents in memory.
   * <p>
   * DO NOT PASS TO CLIENTS
   */
  public CompilationUnitImpl cloneCachingContents() {
    return new CompilationUnitImpl((DartLibraryImpl) getParent(), getFile(), this.owner) {
      private char[] cachedContents;

      @Override
      public char[] getContents() {
        if (this.cachedContents == null) {
          this.cachedContents = CompilationUnitImpl.this.getContents();
        }
        return this.cachedContents;
      }

//      public CompilationUnit originalFromClone() {
//        return CompilationUnitImpl.this;
//      }
    };
  }

  @Override
  public void close() throws DartModelException {
    if (getPerWorkingCopyInfo() != null) {
      // a working copy must remain opened until it is discarded
      return;
    }
    super.close();
  }

  @Override
  public void codeComplete(int offset, CompletionRequestor requestor) throws DartModelException {
    codeComplete(offset, requestor, DefaultWorkingCopyOwner.getInstance());
  }

  @Override
  public void codeComplete(int offset, CompletionRequestor requestor, IProgressMonitor monitor)
      throws DartModelException {
    codeComplete(offset, requestor, DefaultWorkingCopyOwner.getInstance(), monitor);
  }

  @Override
  public void codeComplete(int offset, CompletionRequestor requestor,
      WorkingCopyOwner workingCopyOwner) throws DartModelException {
    codeComplete(offset, requestor, workingCopyOwner, null);
  }

  @Override
  public void codeComplete(int offset, CompletionRequestor requestor,
      WorkingCopyOwner workingCopyOwner, IProgressMonitor monitor) throws DartModelException {
    CompilationUnit orig = isWorkingCopy() ? (CompilationUnit) getOriginalElement() : this;
    codeComplete(this, orig, offset, requestor, workingCopyOwner, monitor);
  }

  @Override
  public DartElement[] codeSelect(DartUnit ast, int offset, int length,
      WorkingCopyOwner workingCopyOwner) throws DartModelException {
    DartCore.notYetImplemented();
    // TODO(brianwilkerson) This is probably not the right semantics for this method for all clients
    // because we will only ever return a single element, but it works for the Open Declaration
    // action.
    PerformanceManager.Timer timer = PerformanceManager.getInstance().start(CODE_SELECT_ID);
    try {
      DartUnit unit = ast;
      if (unit == null) {
        unit = DartCompilerUtilities.resolveUnit(this);
      }
      if (unit != null) {
        DartElementLocator locator = new DartElementLocator(this, offset, offset + length);
        DartElement element = locator.searchWithin(unit);
        if (element != null) {
          return new DartElement[] {element};
        }
      }
      return new DartElement[0];
    } finally {
      timer.end();
    }
  }

  @Override
  public DartElement[] codeSelect(int offset, int length) throws DartModelException {
    return codeSelect(offset, length, DefaultWorkingCopyOwner.getInstance());
  }

  @Override
  public DartElement[] codeSelect(int offset, int length, WorkingCopyOwner workingCopyOwner)
      throws DartModelException {
    return codeSelect(null, offset, length, workingCopyOwner);
  }

  @Override
  public void commitWorkingCopy(boolean force, IProgressMonitor monitor) throws DartModelException {
    CommitWorkingCopyOperation op = new CommitWorkingCopyOperation(this, force);
    op.runOperation(monitor);
  }

  @Override
  public void copy(DartElement container, DartElement sibling, String rename, boolean force,
      IProgressMonitor monitor) throws DartModelException {
    if (container == null) {
      throw new IllegalArgumentException(Messages.operation_nullContainer);
    }
    DartElement[] elements = new DartElement[] {this};
    DartElement[] containers = new DartElement[] {container};
    String[] renamings = null;
    if (rename != null) {
      renamings = new String[] {rename};
    }
    getDartModel().copy(elements, containers, null, renamings, force, monitor);
  }

  @Override
  public DartElementInfo createElementInfo() {
    return new CompilationUnitInfo();
  }

  @Override
  public Type createType(String content, DartElement sibling, boolean force,
      IProgressMonitor monitor) throws DartModelException {
    DartCore.notYetImplemented();
    return null;
    // if (!exists()) {
    // // autogenerate this compilation unit
    // IPackageFragment pkg = (IPackageFragment) getParent();
    //      String source = ""; //$NON-NLS-1$
    // if (!pkg.isDefaultPackage()) {
    // // not the default package...add the package declaration
    // String lineSeparator = Util.getLineSeparator(
    // null/* no existing source */, getDartProject());
    //        source = "package " + pkg.getElementName() + ";" + lineSeparator + lineSeparator; //$NON-NLS-1$ //$NON-NLS-2$
    // }
    // CreateCompilationUnitOperation op = new CreateCompilationUnitOperation(
    // pkg, name, source, force);
    // op.runOperation(monitor);
    // }
    // CreateTypeOperation op = new CreateTypeOperation(this, content, force);
    // if (sibling != null) {
    // op.createBefore(sibling);
    // }
    // op.runOperation(monitor);
    // return (Type) op.getResultElements()[0];
  }

  @Override
  public boolean definesLibrary() {
    try {
      return ((CompilationUnitInfo) getElementInfo()).getDefinesLibrary();
    } catch (DartModelException exception) {
      return false;
    }
  }

  @Override
  public void delete(boolean force, IProgressMonitor monitor) throws DartModelException {
    DartElement[] elements = new DartElement[] {this};
    getDartModel().delete(elements, force, monitor);
  }

  @Override
  public void discardWorkingCopy() throws DartModelException {
    // discard working copy and its children
    DiscardWorkingCopyOperation op = new DiscardWorkingCopyOperation(this);
    op.runOperation(null);
  }

  @Override
  public boolean equals(Object obj) {
    if (!(obj instanceof CompilationUnitImpl)) {
      return false;
    }
    return super.equals(obj);
  }

  /**
   * Finds the elements in this Dart file that correspond to the given element. An element A
   * corresponds to an element B if:
   * <ul>
   * <li>A has the same element name as B.
   * <li>If A is a method, A must have the same number of arguments as B and the simple names of the
   * argument types must be equal, if known.
   * <li>The parent of A corresponds to the parent of B recursively up to their respective Dart
   * files.
   * <li>A exists.
   * </ul>
   * Returns <code>null</code> if no such Dart elements can be found or if the given element is not
   * included in a Dart file.
   * 
   * @param element the given element
   * @return the found elements in this Dart file that correspond to the given element
   */
  @Override
  public DartElement[] findElements(DartElement element) {
    ArrayList<DartElement> children = new ArrayList<DartElement>();
    while (element != null && element.getElementType() != DartElement.COMPILATION_UNIT) {
      children.add(element);
      element = element.getParent();
    }
    if (element == null) {
      return null;
    }
    DartElement currentElement = this;
    for (int i = children.size() - 1; i >= 0; i--) {
      DartCore.notYetImplemented();
      // SourceRefElement child = (SourceRefElement) children.get(i);
      // switch (child.getElementType()) {
      // case DartElement.PACKAGE_DECLARATION:
      // currentElement = ((CompilationUnit)
      // currentElement).getPackageDeclaration(child.getElementName());
      // break;
      // case DartElement.IMPORT_CONTAINER:
      // currentElement = ((CompilationUnit)
      // currentElement).getImportContainer();
      // break;
      // case DartElement.IMPORT_DECLARATION:
      // currentElement = ((ImportContainer)
      // currentElement).getImport(child.getElementName());
      // break;
      // case DartElement.TYPE:
      // switch (currentElement.getElementType()) {
      // case DartElement.COMPILATION_UNIT:
      // currentElement = ((CompilationUnit)
      // currentElement).getType(child.getElementName());
      // break;
      // case DartElement.TYPE:
      // currentElement = ((Type)
      // currentElement).getType(child.getElementName());
      // break;
      // case DartElement.FIELD:
      // case DartElement.INITIALIZER:
      // case DartElement.METHOD:
      // currentElement = ((Member) currentElement).getType(
      // child.getElementName(), child.occurrenceCount);
      // break;
      // }
      // break;
      // case DartElement.INITIALIZER:
      // currentElement = ((Type)
      // currentElement).getInitializer(child.occurrenceCount);
      // break;
      // case DartElement.FIELD:
      // currentElement = ((Type)
      // currentElement).getField(child.getElementName());
      // break;
      // case DartElement.METHOD:
      // currentElement = ((Type) currentElement).getMethod(
      // child.getElementName(), ((Method) child).getParameterTypes());
      // break;
      // }
    }
    if (currentElement != null && currentElement.exists()) {
      return new DartElement[] {currentElement};
    } else {
      return null;
    }
  }

  @Override
  public CompilationUnit findWorkingCopy(WorkingCopyOwner workingCopyOwner) {
    CompilationUnitImpl cu = new CompilationUnitImpl((DartLibraryImpl) getParent(), getFile(),
        workingCopyOwner);
    if (workingCopyOwner == DefaultWorkingCopyOwner.getInstance()) {
      return cu;
    } else {
      // must be a working copy
      PerWorkingCopyInfo perWorkingCopyInfo = cu.getPerWorkingCopyInfo();
      if (perWorkingCopyInfo != null) {
        return perWorkingCopyInfo.getWorkingCopy();
      } else {
        return null;
      }
    }
  }

  @Override
  public CompilationUnit getCompilationUnit() {
    return this;
  }

  public char[] getContents() {
    Buffer buffer = getBufferManager().getBuffer(this);
    if (buffer == null) {
      // no need to force opening of CU to get the content
      // also this cannot be a working copy, as its buffer is never closed while
      // the working copy is alive
      IFile file = (IFile) getResource();
      // Get encoding from file
      String encoding;
      try {
        encoding = file.getCharset();
      } catch (CoreException ce) {
        // do not use any encoding
        encoding = null;
      }
      try {
        return Util.getResourceContentsAsCharArray(file, encoding);
      } catch (DartModelException e) {
        // if (DartModelManager.getInstance().abortOnMissingSource.get() ==
        // Boolean.TRUE) {
        // IOException ioException = e.getDartModelStatus().getCode() ==
        // DartModelStatusConstants.IO_EXCEPTION
        // ? (IOException) e.getException()
        // : new IOException(e.getMessage());
        // throw new AbortCompilationUnit(null, ioException, encoding);
        // } else {
        // Util.log(e, Messages.bind(Messages.file_notFound,
        // file.getFullPath().toString()));
        // }
        DartCore.notYetImplemented();
        return CharOperation.NO_CHAR;
      }
    }
    char[] contents = buffer.getCharacters();
    if (contents == null) {
      // see https://bugs.eclipse.org/bugs/show_bug.cgi?id=129814
      // if (DartModelManager.getInstance().abortOnMissingSource.get() ==
      // Boolean.TRUE) {
      // IOException ioException = new IOException(Messages.buffer_closed);
      // IFile file = (IFile) getResource();
      // // Get encoding from file
      // String encoding;
      // try {
      // encoding = file.getCharset();
      // } catch (CoreException ce) {
      // // do not use any encoding
      // encoding = null;
      // }
      // throw new AbortCompilationUnit(null, ioException, encoding);
      // }
      DartCore.notYetImplemented();
      return CharOperation.NO_CHAR;
    }
    return contents;
  }

  @Override
  public DartElement getElementAt(int position) throws DartModelException {
    DartElement element = getSourceElementAt(position);
    if (element == this) {
      return null;
    } else {
      return element;
    }
  }

  @Override
  public int getElementType() {
    return DartElement.COMPILATION_UNIT;
  }

  public char[] getFileName() {
    return getPath().toString().toCharArray();
  }

  @Override
  public DartFunctionTypeAlias[] getFunctionTypeAliases() throws DartModelException {
    List<DartFunctionTypeAlias> typeList = getChildrenOfType(DartFunctionTypeAlias.class);
    return typeList.toArray(new DartFunctionTypeAlias[typeList.size()]);
  }

  @Override
  public DartVariableDeclaration[] getGlobalVariables() throws DartModelException {
    List<DartVariableDeclaration> variableList = getChildrenOfType(DartVariableDeclaration.class);
    return variableList.toArray(new DartVariableDeclaration[variableList.size()]);
  }

  @Override
  public DartLibrary getLibrary() {
    return getAncestor(DartLibrary.class);
  }

  @Override
  public PerWorkingCopyInfo getPerWorkingCopyInfo() {
    return DartModelManager.getInstance().getPerWorkingCopyInfo(this, false, false, null);
  }

  @Override
  public DartElement getPrimaryElement(boolean checkOwner) {
    if (checkOwner && isPrimary()) {
      return this;
    }
    return new CompilationUnitImpl((DartLibraryImpl) getParent(), getFile(),
        DefaultWorkingCopyOwner.getInstance());
  }

  @Override
  public Type getType(String typeName) {
    return new DartTypeImpl(this, typeName);
  }

  @Override
  public Type[] getTypes() throws DartModelException {
    List<Type> typeList = getChildrenOfType(Type.class);
    return typeList.toArray(new Type[typeList.size()]);
  }

  @Override
  public IResource getUnderlyingResource() throws DartModelException {
    if (isWorkingCopy() && !isPrimary()) {
      return null;
    }
    return super.getUnderlyingResource();
  }

  @Override
  public boolean hasResourceChanged() {
    if (!isWorkingCopy()) {
      return false;
    }
    // if the resource was deleted, then #getModificationStamp() will return
    // IResource.NULL_STAMP, which is always different from the cached timestamp
    Object info = DartModelManager.getInstance().getInfo(this);
    if (info == null) {
      return false;
    }
    IResource resource = getResource();
    if (resource == null) {
      return false;
    }
    return ((CompilationUnitInfo) info).getTimestamp() != resource.getModificationStamp();
  }

  /**
   * Return <code>true</code> if the element is consistent with its underlying resource or buffer.
   * The element is consistent when opened, and is consistent if the underlying resource or buffer
   * has not been modified since it was last consistent.
   * <p>
   * NOTE: Child consistency is not considered. For example, a package fragment responds
   * <code>true</code> when it knows about all of its compilation units present in its underlying
   * folder. However, one or more of the compilation units could be inconsistent.
   * 
   * @return <code>true</code> if the element is consistent with its underlying resource or buffer
   */
  @Override
  public boolean isConsistent() {
    return !DartModelManager.getInstance().getElementsOutOfSynchWithBuffers().contains(this);
  }

  public DartUnit makeConsistent(boolean resolveBindings, boolean forceProblemDetection,
      HashMap<String, CategorizedProblem[]> problems, IProgressMonitor monitor)
      throws DartModelException {
    if (isConsistent()) {
      return null;
    }
    DartModelManager manager = DartModelManager.getInstance();
    try {
      manager.abortOnMissingSource.set(Boolean.TRUE);
      // create a new info and make it the current info
      // (this will remove the info and its children just before storing the new infos)
      if (problems != null) {
        ASTHolderCUInfo info = new ASTHolderCUInfo();
        info.resolveBindings = resolveBindings;
        info.forceProblemDetection = forceProblemDetection;
        info.problems = problems;
        openWhenClosed(info, monitor);
        DartUnit result = info.ast;
        info.ast = null;
        return result;
      } else {
        openWhenClosed(createElementInfo(), monitor);
        return null;
      }
    } finally {
      manager.abortOnMissingSource.set(null);
    }
  }

  @Override
  public void makeConsistent(IProgressMonitor monitor) throws DartModelException {
    makeConsistent(false, false, null, monitor);
  }

  @Override
  public void move(DartElement container, DartElement sibling, String rename, boolean force,
      IProgressMonitor monitor) throws DartModelException {
    if (container == null) {
      throw new IllegalArgumentException(Messages.operation_nullContainer);
    }
    DartElement[] elements = new DartElement[] {this};
    DartElement[] containers = new DartElement[] {container};

    String[] renamings = null;
    if (rename != null) {
      renamings = new String[] {rename};
    }
    getDartModel().move(elements, containers, null, renamings, force, monitor);
  }

  public void reconcile(boolean forceProblemDetection, IProgressMonitor monitor)
      throws DartModelException {
    reconcile(forceProblemDetection, null, monitor);
  }

  public DartUnit reconcile(boolean forceProblemDetection, WorkingCopyOwner workingCopyOwner,
      IProgressMonitor monitor) throws DartModelException {
    if (!isWorkingCopy()) {
      // Reconciling is not supported on non working copies
      return null;
    }
    if (workingCopyOwner == null) {
      workingCopyOwner = DefaultWorkingCopyOwner.getInstance();
    }
    PerformanceStats stats = null;
    if (ReconcileWorkingCopyOperation.PERF) {
      stats = PerformanceStats.getStats(DartModelManager.RECONCILE_PERF, this);
      stats.startRun(new String(getFileName()));
    }
    ReconcileWorkingCopyOperation op = new ReconcileWorkingCopyOperation(this,
        forceProblemDetection, workingCopyOwner);
    op.runOperation(monitor);
    if (ReconcileWorkingCopyOperation.PERF) {
      stats.endRun();
    }
    return op.ast;
  }

  @Override
  public void rename(String newName, boolean force, IProgressMonitor monitor)
      throws DartModelException {
    if (newName == null) {
      throw new IllegalArgumentException(Messages.operation_nullName);
    }
    DartElement[] elements = new DartElement[] {this};
    DartElement[] dests = new DartElement[] {getParent()};
    String[] renamings = new String[] {newName};
    getDartModel().rename(elements, dests, renamings, force, monitor);
  }

  @Override
  public void restore() throws DartModelException {
    if (!isWorkingCopy()) {
      return;
    }
    CompilationUnitImpl original = (CompilationUnitImpl) getOriginalElement();
    Buffer buffer = getBuffer();
    if (buffer == null) {
      return;
    }
    buffer.setContents(original.getContents());
    updateTimeStamp(original);
    makeConsistent(null);
  }

  @Override
  public void save(IProgressMonitor monitor, boolean force) throws DartModelException {
    if (isWorkingCopy()) {
      // No need to save the buffer for a working copy (this is a noop). Not
      // simply makeConsistent, also computes fine-grain deltas in case the
      // working copy is being reconciled already (if not it would miss one
      // iteration of deltas).
      reconcile(false, null, null);
    } else {
      super.save(monitor, force);
    }
  }

  public void updateTimeStamp(CompilationUnitImpl original) throws DartModelException {
    long timeStamp = ((IFile) original.getResource()).getModificationStamp();
    if (timeStamp == IResource.NULL_STAMP) {
      throw new DartModelException(new DartModelStatusImpl(
          DartModelStatusConstants.INVALID_RESOURCE));
    }
    ((CompilationUnitInfo) getElementInfo()).setTimestamp(timeStamp);
  }

  @Override
  protected boolean buildStructure(OpenableElementInfo info, IProgressMonitor pm,
      Map<DartElement, DartElementInfo> newElements, IResource underlyingResource)
      throws DartModelException {
    CompilationUnitInfo unitInfo = (CompilationUnitInfo) info;
    //
    // Ensure that the buffer is opened so that it can be accessed indirectly
    // later.
    //
    Buffer buffer = getBufferManager().getBuffer(this);
    if (buffer == null) {
      // Open the buffer independently from the info, since we are building the
      // info
      openBuffer(pm, unitInfo);
    }
    //
    // Generate the structure.
    //
    String source = getSource();
    DartUnit unit = null;
    try {
      unit = DartCompilerUtilities.parseSource(getElementName(), source,
          new ArrayList<DartCompilationError>());
    } catch (Exception exception) {
      return false;
    }
    if (unit == null) {
      return false;
    }
    new CompilationUnitStructureBuilder(this, newElements).accept(unit);
    //
    // Update the time stamp (might be IResource.NULL_STAMP if original does not
    // exist).
    //
    if (underlyingResource == null) {
      underlyingResource = getResource();
    }
    //
    // The underlying resource should never be null.
    //
    if (underlyingResource != null) {
      unitInfo.setTimestamp(underlyingResource.getModificationStamp());
    }
    unitInfo.setSourceLength(source.length());
    return true;
  }

  @Override
  protected void closing(DartElementInfo info) throws DartModelException {
    if (getPerWorkingCopyInfo() == null) {
      super.closing(info);
    }
    // else the buffer of a working copy must remain open for the lifetime of
    // the working copy
  }

  @Override
  protected DartElement getHandleFromMemento(String token, MementoTokenizer tokenizer,
      WorkingCopyOwner owner) {
    switch (token.charAt(0)) {
      case MEMENTO_DELIMITER_FUNCTION:
        if (!tokenizer.hasMoreTokens()) {
          return this;
        }
        DartFunctionImpl function = new DartFunctionImpl(this, tokenizer.nextToken());
        return function.getHandleFromMemento(tokenizer, owner);
      case MEMENTO_DELIMITER_FUNCTION_TYPE_ALIAS:
        if (!tokenizer.hasMoreTokens()) {
          return this;
        }
        DartFunctionTypeAliasImpl alias = new DartFunctionTypeAliasImpl(this, tokenizer.nextToken());
        return alias.getHandleFromMemento(tokenizer, owner);
      case MEMENTO_DELIMITER_TYPE:
        if (!tokenizer.hasMoreTokens()) {
          return this;
        }
        DartTypeImpl type = new DartTypeImpl(this, tokenizer.nextToken());
        return type.getHandleFromMemento(tokenizer, owner);
      case MEMENTO_DELIMITER_VARIABLE:
        if (!tokenizer.hasMoreTokens()) {
          return this;
        }
        DartVariableImpl variable = new DartVariableImpl(this, tokenizer.nextToken());
        return variable.getHandleFromMemento(tokenizer, owner);
    }
    return null;
  }

  @Override
  protected char getHandleMementoDelimiter() {
    return MEMENTO_DELIMITER_COMPILATION_UNIT;
  }

  @Override
  protected String getHandleMementoName() {
    // Because the compilation unit can be anywhere relative to the library or
    // application that contains it we need to specify the full path.
    return getFile().getProjectRelativePath().toPortableString();
  }

  @Override
  protected CompilationUnit getWorkingCopy(WorkingCopyOwner workingCopyOwner,
      ProblemRequestor problemRequestor, IProgressMonitor monitor) throws DartModelException {
    if (!isPrimary()) {
      return this;
    }
    DartModelManager manager = DartModelManager.getInstance();
    CompilationUnitImpl workingCopy = new CompilationUnitImpl((DartLibraryImpl) getParent(),
        getFile(), workingCopyOwner);
    PerWorkingCopyInfo perWorkingCopyInfo = manager.getPerWorkingCopyInfo(workingCopy, false, true,
        null);
    if (perWorkingCopyInfo != null) {
      // return existing handle instead of the one created above
      return perWorkingCopyInfo.getWorkingCopy();
    }
    BecomeWorkingCopyOperation op = new BecomeWorkingCopyOperation(workingCopy, problemRequestor);
    op.runOperation(monitor);
    return workingCopy;
  }

  @Override
  protected boolean hasBuffer() {
    return true;
  }

  @Override
  protected boolean isSourceElement() {
    return true;
  }

  @Override
  protected void openAncestors(HashMap<DartElement, DartElementInfo> newElements,
      IProgressMonitor monitor) throws DartModelException {
    if (!isWorkingCopy()) {
      super.openAncestors(newElements, monitor);
    }
    // else don't open ancestors for a working copy to speed up the first
    // becomeWorkingCopy
    // (see https://bugs.eclipse.org/bugs/show_bug.cgi?id=89411)
  }

  @Override
  protected Buffer openBuffer(IProgressMonitor pm, DartElementInfo info) throws DartModelException {
    // create buffer
    BufferManager bufManager = getBufferManager();
    boolean isWorkingCopy = isWorkingCopy();
    Buffer buffer = isWorkingCopy ? owner.createBuffer(this) : BufferManager.createBuffer(this);
    if (buffer == null) {
      return null;
    }
    CompilationUnitImpl original = null;
    boolean mustSetToOriginalContent = false;
    if (isWorkingCopy) {
      // ensure that isOpen() is called outside the bufManager synchronized
      // block see https://bugs.eclipse.org/bugs/show_bug.cgi?id=237772
      mustSetToOriginalContent = !isPrimary()
          && (original = new CompilationUnitImpl((DartLibraryImpl) getParent(), getFile(),
              DefaultWorkingCopyOwner.getInstance())).isOpen();
    }
    // synchronize to ensure that 2 threads are not putting 2 different buffers
    // at the same time see https://bugs.eclipse.org/bugs/show_bug.cgi?id=146331
    synchronized (bufManager) {
      Buffer existingBuffer = bufManager.getBuffer(this);
      if (existingBuffer != null) {
        return existingBuffer;
      }
      // set the buffer source
      if (buffer.getCharacters() == null) {
        if (mustSetToOriginalContent) {
          buffer.setContents(original.getSource());
        } else {
          readBuffer(buffer, isWorkingCopy);
        }
      }

      // add buffer to buffer cache
      // note this may cause existing buffers to be removed from the buffer
      // cache, but only primary compilation unit's buffer
      // can be closed, thus no call to a client's IBuffer#close() can be done
      // in this synchronized block.
      bufManager.addBuffer(buffer);

      // listen to buffer changes
      buffer.addBufferChangedListener(this);
    }
    return buffer;
  }

  /**
   * Debugging purposes
   */
  @Override
  protected void toStringInfo(int tab, StringBuilder builder, DartElementInfo info,
      boolean showResolvedInfo) {
    if (!isPrimary()) {
      builder.append(tabString(tab));
      builder.append("[Working copy] "); //$NON-NLS-1$
      toStringName(builder);
    } else {
      if (isWorkingCopy()) {
        builder.append(tabString(tab));
        builder.append("[Working copy] "); //$NON-NLS-1$
        toStringName(builder);
        if (info == null) {
          builder.append(" (not open)"); //$NON-NLS-1$
        }
      } else {
        super.toStringInfo(tab, builder, info, showResolvedInfo);
      }
    }
  }

  protected IStatus validateCompilationUnit(IResource resource) {
    DartCore.notYetImplemented();
    // IPackageFragmentRoot root = getPackageFragmentRoot();
    // // root never null as validation is not done for working copies
    // try {
    // if (root.getKind() != IPackageFragmentRoot.K_SOURCE)
    // return new
    // DartModelStatusImpl(DartModelStatusConstants.INVALID_ELEMENT_TYPES,
    // root);
    // } catch (DartModelException e) {
    // return e.getDartModelStatus();
    // }
    if (resource != null) {
      // char[][] inclusionPatterns =
      // ((PackageFragmentRoot)root).fullInclusionPatternChars();
      // char[][] exclusionPatterns =
      // ((PackageFragmentRoot)root).fullExclusionPatternChars();
      // if (Util.isExcluded(resource, inclusionPatterns, exclusionPatterns))
      // return new
      // DartModelStatusImpl(DartModelStatusConstants.ELEMENT_NOT_ON_CLASSPATH,
      // this);
      if (!resource.isAccessible()) {
        return new DartModelStatusImpl(DartModelStatusConstants.ELEMENT_DOES_NOT_EXIST, this);
      }
    }
    return DartConventions.validateCompilationUnitName(getElementName());
  }

  @Override
  protected IStatus validateExistence(IResource underlyingResource) {
    // check if this compilation unit can be opened
    if (!isWorkingCopy()) { // no check is done on root kind or exclusion
                            // pattern for working copies
      IStatus status = validateCompilationUnit(underlyingResource);
      if (!status.isOK()) {
        return status;
      }
    }
    // prevents reopening of non-primary working copies (they are closed when
    // they are discarded and should not be reopened)
    if (!isPrimary() && getPerWorkingCopyInfo() == null) {
      return newDoesNotExistStatus();
    }
    return DartModelStatusImpl.VERIFIED_OK;
  }

  private DartElement getOriginalElement() {
    // backward compatibility
    if (!isWorkingCopy()) {
      return null;
    }
    return getPrimaryElement();
  }
}
