// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

package com.google.dart.compiler.type;

import com.google.dart.compiler.resolver.ClassElement;
import com.google.dart.compiler.resolver.Element;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * An interface type.
 */
class InterfaceTypeImplementation extends AbstractType implements InterfaceType {
  private final ClassElement element;
  private final List<? extends Type> arguments;

  InterfaceTypeImplementation(ClassElement element, List<? extends Type> arguments) {
    this.element = element;
    this.arguments = arguments;
  }

  @Override
  public ClassElement getElement() {
    return element;
  }

  @Override
  public List<? extends Type> getArguments() {
    return arguments;
  }

  @Override
  public String toString() {
    if (getArguments().isEmpty()) {
      return getElement().getName();
    } else {
      StringBuilder sb = new StringBuilder();
      sb.append(getElement().getName());
      Types.printTypesOn(sb, getArguments(), "<", ">");
      return sb.toString();
    }
  }
  
  @Override
  public boolean hasDynamicTypeArgs() {
    for (Type t : getArguments()) {
      if (t.getKind() == TypeKind.DYNAMIC) {
        return true;
      }
    }
    return false;
  }

  @Override
  public boolean isRaw() {
    return getArguments().size() != getElement().getTypeParameters().size();
  }

  @Override
  public InterfaceType subst(List<? extends Type> arguments, List<? extends Type> parameters) {
    if (arguments.isEmpty() && parameters.isEmpty()) {
      return this;
    }
    List<Type> substitutedArguments = Types.subst(getArguments(), arguments, parameters);
    return new InterfaceTypeImplementation(getElement(), substitutedArguments);
  }

  @Override
  public boolean equals(Object obj) {
    if (obj instanceof InterfaceType) {
      InterfaceType other = (InterfaceType) obj;
      return getElement().equals(other.getElement()) && getArguments().equals(other.getArguments());
    }
    return false;
  }

  @Override
  public int hashCode() {
    int hashCode = 31;
    hashCode += getElement().hashCode();
    hashCode += 31 * hashCode + getArguments().hashCode();
    return hashCode;
  }

  @Override
  public InterfaceType asRawType() {
    return new InterfaceTypeImplementation(getElement(), Arrays.<Type>asList());
  }

  @Override
  public TypeKind getKind() {
    return TypeKind.INTERFACE;
  }

  @Override
  public Member lookupMember(String name) {
    Element element = getElement().lookupLocalElement(name);
    if (element != null) {
      return new MemberImplementation(this, element);
    }
    InterfaceType supertype = getSupertype();
    if (supertype != null) {
      Member member = supertype.lookupMember(name);
      if (member != null) {
        return member;
      }
    }
    for (InterfaceType intrface : getInterfaces()) {
      Member member = intrface.lookupMember(name);
      if (member != null) {
        return member;
      }
    }
    return null;
  }

  private InterfaceType getSupertype() {
    InterfaceType supertype = getElement().getSupertype();
    if (supertype == null) {
      return null;
    } else {
      return supertype.subst(getArguments(), getElement().getTypeParameters());
    }
  }

  private List<InterfaceType> getInterfaces() {
    List<InterfaceType> interfaces = getElement().getInterfaces();
    List<InterfaceType> result = new ArrayList<InterfaceType>(interfaces.size());
    List<? extends Type> typeArguments = getArguments();
    List<? extends Type> typeParameters = getElement().getTypeParameters();
    for (InterfaceType type : interfaces) {
      result.add(type.subst(typeArguments, typeParameters));
    }
    return result;
  }

  private static class MemberImplementation implements Member {
    private final InterfaceType holder;
    private final Element member;

    MemberImplementation(InterfaceType holder, Element member) {
      this.holder = holder;
      this.member = member;
    }

    @Override
    public InterfaceType getHolder() {
      return holder;
    }

    @Override
    public Element getElement() {
      return member;
    }

    @Override
    public Type getType() {
      List<? extends Type> typeArguments = getHolder().getArguments();
      List<? extends Type> typeParameters = getHolder().getElement().getTypeParameters();
      return getElement().getType().subst(typeArguments, typeParameters);
    }
  }
}
