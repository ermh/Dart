// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/ast_printer.h"

#include "vm/handles.h"
#include "vm/object.h"
#include "vm/os.h"
#include "vm/parser.h"

namespace dart {

AstPrinter::AstPrinter() { }


AstPrinter::~AstPrinter() { }


void AstPrinter::VisitGenericAstNode(AstNode* node) {
  OS::Print("(%d: %s ", node->id(), node->Name());
  node->VisitChildren(this);
  OS::Print(")");
}


void AstPrinter::VisitSequenceNode(SequenceNode* node_sequence) {
  // TODO(regis): Make the output more readable by indenting the nested
  // sequences. This could be achieved using a AstPrinterContext similar to the
  // CodeGeneratorContext.
  ASSERT(node_sequence != NULL);
  for (int i = 0; i < node_sequence->length(); i++) {
    OS::Print("id %d: ",  node_sequence->NodeAt(i)->id());
    node_sequence->NodeAt(i)->Visit(this);
    OS::Print("\n");
  }
}


void AstPrinter::VisitArgumentListNode(ArgumentListNode* arguments) {
  VisitGenericAstNode(arguments);
}


void AstPrinter::VisitReturnNode(ReturnNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitGenericLocalNode(AstNode* node,
                                       const LocalVariable& var) {
  OS::Print("(%s %s%s '%s'",
            node->Name(),
            var.is_final() ? "final " : "",
            var.type().ToCString(),
            var.name().ToCString());
  if (var.HasIndex()) {
    OS::Print(" @%d", var.index());
    if (var.is_captured()) {
      OS::Print(" ctx %d", var.owner()->context_level());
    }
  }
  node->VisitChildren(this);
  OS::Print(")");
}


void AstPrinter::VisitLoadLocalNode(LoadLocalNode* node) {
  VisitGenericLocalNode(node, node->local());
}


void AstPrinter::VisitStoreLocalNode(StoreLocalNode* node) {
  VisitGenericLocalNode(node, node->local());
}


void AstPrinter::VisitGenericFieldNode(AstNode* node, const Field& field) {
  OS::Print("(%s %s%s '%s' ",
            node->Name(),
            field.is_final() ? "final " : "",
            Type::Handle(field.type()).ToCString(),
            String::Handle(field.name()).ToCString());
  node->VisitChildren(this);
  OS::Print(")");
}


void AstPrinter::VisitLoadInstanceFieldNode(LoadInstanceFieldNode* node) {
  VisitGenericFieldNode(node, node->field());
}


void AstPrinter::VisitStoreInstanceFieldNode(StoreInstanceFieldNode* node) {
  VisitGenericFieldNode(node, node->field());
}


void AstPrinter::VisitLoadStaticFieldNode(LoadStaticFieldNode* node) {
  VisitGenericFieldNode(node, node->field());
}


void AstPrinter::VisitStoreStaticFieldNode(StoreStaticFieldNode* node) {
  VisitGenericFieldNode(node, node->field());
}


void AstPrinter::VisitArrayNode(ArrayNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitLiteralNode(LiteralNode* node) {
  const Instance& literal = node->literal();
  OS::Print("'%s'", literal.ToCString());
}


void AstPrinter::VisitTypeNode(TypeNode* node) {
  const Type& type = node->type();
  OS::Print("'%s'", type.ToCString());
}


void AstPrinter::VisitPrimaryNode(PrimaryNode* node) {
  OS::Print("***** PRIMARY NODE IN AST ***** (%s '%s')",
      node->Name(), node->primary().ToCString());
}


void AstPrinter::VisitComparisonNode(ComparisonNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitBinaryOpNode(BinaryOpNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitStringConcatNode(StringConcatNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitUnaryOpNode(UnaryOpNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitConditionalExprNode(ConditionalExprNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitIfNode(IfNode* node) {
  OS::Print("(if ");
  node->condition()->Visit(this);
  OS::Print(" then ");
  node->true_branch()->Visit(this);
  OS::Print(" else ");
  if (node->false_branch() != NULL) {
    node->false_branch()->Visit(this);
  }
  OS::Print(")");
}


void AstPrinter::VisitCaseNode(CaseNode* node) {
  OS::Print("(case (");
  for (int i = 0; i < node->case_expressions()->length(); i++) {
    node->case_expressions()->NodeAt(i)->Visit(this);
  }
  if (node->contains_default()) {
    OS::Print(" default ");
  }
  OS::Print(")");
  node->statements()->Visit(this);
  OS::Print(")");
}


void AstPrinter::VisitSwitchNode(SwitchNode *node) {
  OS::Print("(switch ");
  node->body()->Visit(this);
  OS::Print(")");
}


void AstPrinter::VisitWhileNode(WhileNode* node) {
  OS::Print("(while ");
  node->condition()->Visit(this);
  OS::Print(" do ");
  node->body()->Visit(this);
  OS::Print(")");
}


void AstPrinter::VisitForNode(ForNode* node) {
  OS::Print("(for (init: ");
  node->initializer()->Visit(this);
  OS::Print("; cond:");
  if (node->condition() != NULL) {
    node->condition()->Visit(this);
  }
  OS::Print("; incr:");
  node->increment()->Visit(this);
  OS::Print(") body:");
  node->body()->Visit(this);
  OS::Print("endfor)");
}


void AstPrinter::VisitDoWhileNode(DoWhileNode* node) {
  OS::Print("(do ");
  node->body()->Visit(this);
  OS::Print(" while ");
  node->condition()->Visit(this);
  OS::Print(")");
}


void AstPrinter::VisitJumpNode(JumpNode* node) {
  OS::Print("(%s %s)", node->Name(), node->label()->name().ToCString());
}


void AstPrinter::VisitIncrOpLocalNode(IncrOpLocalNode* node) {
  VisitGenericLocalNode(node, node->local());
}


void AstPrinter::VisitIncrOpInstanceFieldNode(IncrOpInstanceFieldNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitIncrOpStaticFieldNode(IncrOpStaticFieldNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitIncrOpIndexedNode(IncrOpIndexedNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitInstanceCallNode(InstanceCallNode* node) {
  OS::Print("(%s '%s'(", node->Name(), node->function_name().ToCString());
  node->VisitChildren(this);
  OS::Print("))");
}


void AstPrinter::VisitStaticCallNode(StaticCallNode* node) {
  const char* function_fullname = node->function().ToFullyQualifiedCString();
  OS::Print("(%s '%s'(", node->Name(), function_fullname);
  node->VisitChildren(this);
  OS::Print("))");
}


void AstPrinter::VisitClosureNode(ClosureNode* node) {
  const char* function_fullname = node->function().ToFullyQualifiedCString();
  OS::Print("(%s '%s')", node->Name(), function_fullname);
}


void AstPrinter::VisitStaticImplicitClosureNode(
    StaticImplicitClosureNode* node) {
  const char* function_fullname = node->function().ToFullyQualifiedCString();
  OS::Print("static (%s '%s')", node->Name(), function_fullname);
}


void AstPrinter::VisitImplicitClosureNode(ImplicitClosureNode* node) {
  const char* function_fullname = node->function().ToFullyQualifiedCString();
  OS::Print("(%s '%s')", node->Name(), function_fullname);
}


void AstPrinter::VisitClosureCallNode(ClosureCallNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitConstructorCallNode(ConstructorCallNode* node) {
  const char* kind = node->constructor().IsFactory() ? "factory " : "";
  const char* constructor_name = node->constructor().ToFullyQualifiedCString();
  OS::Print("(%s %s'%s' ((this)", node->Name(), kind, constructor_name);
  node->VisitChildren(this);
  OS::Print("))");
}


void AstPrinter::VisitInstanceGetterNode(InstanceGetterNode* node) {
  OS::Print("(%s 'get %s'(", node->Name(), node->field_name().ToCString());
  node->VisitChildren(this);
  OS::Print("))");
}


void AstPrinter::VisitInstanceSetterNode(InstanceSetterNode* node) {
  OS::Print("(%s 'set %s'(", node->Name(), node->field_name().ToCString());
  node->VisitChildren(this);
  OS::Print("))");
}


void AstPrinter::VisitStaticGetterNode(StaticGetterNode* node) {
  String& class_name = String::Handle(node->cls().Name());
  OS::Print("(%s '%s.%s'(",
            node->Name(),
            class_name.ToCString(),
            node->field_name().ToCString());
  OS::Print("))");
}


void AstPrinter::VisitStaticSetterNode(StaticSetterNode* node) {
  String& class_name = String::Handle(node->cls().Name());
  OS::Print("(%s '%s.%s'(",
      node->Name(), class_name.ToCString(), node->field_name().ToCString());
  node->VisitChildren(this);
  OS::Print("))");
}


void AstPrinter::VisitLoadIndexedNode(LoadIndexedNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitStoreIndexedNode(StoreIndexedNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitNativeBodyNode(NativeBodyNode* node) {
  OS::Print("(native_c call '%s'(%d args))",
            node->native_c_function_name().ToCString(),
            node->argument_count());
}


void AstPrinter::VisitCatchClauseNode(CatchClauseNode* node) {
  node->VisitChildren(this);
}


void AstPrinter::VisitTryCatchNode(TryCatchNode* node) {
  OS::Print("(");

  // First visit the try block.
  OS::Print("(try block (");
  node->try_block()->Visit(this);
  OS::Print("))");

  // Now visit the catch block if it exists.
  if (node->catch_block() != NULL) {
    OS::Print("(catch block () (");
    node->catch_block()->Visit(this);
    OS::Print("))");
  }

  // Now visit the finally block if it exists.
  if (node->finally_block() != NULL) {
    OS::Print("(finally block () (");
    node->finally_block()->Visit(this);
    OS::Print("))");
  }

  OS::Print(")");
}


void AstPrinter::VisitThrowNode(ThrowNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::VisitInlinedFinallyNode(InlinedFinallyNode* node) {
  VisitGenericAstNode(node);
}


void AstPrinter::PrintNode(AstNode* node) {
  ASSERT(node != NULL);
  AstPrinter ast_printer;
  node->Visit(&ast_printer);
  OS::Print("\n");
}


void AstPrinter::PrintLocalScope(const LocalScope* scope,
                                 int start_index) {
  ASSERT(scope != NULL);
  for (int i = start_index; i < scope->num_variables(); i++) {
    LocalVariable* var = scope->VariableAt(i);
    OS::Print("(%s%s '%s'",
              var->is_final() ? "final " : "",
              var->type().ToCString(),
              var->name().ToCString());
    if (var->owner() != scope) {
      OS::Print(" alias");
    }
    if (var->HasIndex()) {
      OS::Print(" @%d", var->index());
      if (var->is_captured()) {
        OS::Print(" ctx %d", var->owner()->context_level());
      }
    } else if (var->owner()->function_level() != 0) {
      OS::Print(" lev %d", var->owner()->function_level());
    }
    OS::Print(")");
  }
  const LocalScope* child = scope->child();
  while (child != NULL) {
    OS::Print("{scope ");
    PrintLocalScope(child, 0);
    OS::Print("}");
    child = child->sibling();
  }
}


void AstPrinter::PrintFunctionScope(const ParsedFunction& parsed_function) {
  HANDLESCOPE();
  const Function& function = parsed_function.function();
  const Array& default_parameter_values =
      parsed_function.default_parameter_values();
  SequenceNode* node_sequence = parsed_function.node_sequence();
  ASSERT(node_sequence != NULL);
  const LocalScope* scope = node_sequence->scope();
  ASSERT(scope != NULL);
  const char* function_name = function.ToFullyQualifiedCString();
  OS::Print("Scope for function '%s' {\n", function_name);
  const int num_fixed_params = function.num_fixed_parameters();
  const int num_opt_params = function.num_optional_parameters();
  const int num_params = num_fixed_params + num_opt_params;
  // Parameters must be listed first and must all appear in the top scope.
  ASSERT(num_params <= scope->num_variables());
  int pos = 0;  // Current position of variable in scope.
  while (pos < num_params) {
    LocalVariable* param = scope->VariableAt(pos);
    ASSERT(param->owner() == scope);
    OS::Print("(param %s%s '%s'",
              param->is_final() ? "final " : "",
              param->type().ToCString(),
              param->name().ToCString());
    // Print the default value if the parameter is optional.
    if (pos >= num_fixed_params && pos < num_params) {
      const Object& default_parameter_value = Object::Handle(
          default_parameter_values.At(pos - num_fixed_params));
      OS::Print(" =%s", default_parameter_value.ToCString());
    }
    if (param->HasIndex()) {
      OS::Print(" @%d", param->index());
      if (param->is_captured()) {
        OS::Print(" ctx %d", param->owner()->context_level());
      }
    }
    OS::Print(")");
    pos++;
  }
  // Visit remaining non-parameter variables and children scopes.
  PrintLocalScope(scope, pos);
  OS::Print("}\n");
}


void AstPrinter::PrintFunctionNodes(const ParsedFunction& parsed_function) {
  HANDLESCOPE();
  SequenceNode* node_sequence = parsed_function.node_sequence();
  ASSERT(node_sequence != NULL);
  AstPrinter ast_printer;
  const char* function_name =
      parsed_function.function().ToFullyQualifiedCString();
  OS::Print("Ast for function '%s' {\n", function_name);
  node_sequence->Visit(&ast_printer);
  OS::Print("}\n");
}

}  // namespace dart
