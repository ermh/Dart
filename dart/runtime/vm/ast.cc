// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/ast.h"
#include "vm/compiler.h"
#include "vm/dart_entry.h"
#include "vm/isolate.h"
#include "vm/object_store.h"


namespace dart {

#define DEFINE_VISIT_FUNCTION(type, name)                                      \
  void type::Visit(AstNodeVisitor* visitor) {                                  \
    visitor->Visit##type(this);                                                \
  }
NODE_LIST(DEFINE_VISIT_FUNCTION)
#undef DEFINE_VISIT_FUNCTION


#define DEFINE_NAME_FUNCTION(type, name)                                       \
  const char* type::ShortName() const {                                        \
    return name;                                                               \
  }
NODE_LIST(DEFINE_NAME_FUNCTION)
#undef DEFINE_NAME_FUNCTION


// A visitor class to collect all the nodes (including children) into an
// array.
class AstNodeCollector : public AstNodeVisitor {
 public:
  explicit AstNodeCollector(GrowableArray<AstNode*>* nodes)
    : nodes_(nodes) { }

#define DEFINE_VISITOR_FUNCTION(type, name)                                    \
  virtual void Visit##type(type* node) {                                       \
    nodes_->Add(node);                                                         \
    node->VisitChildren(this);                                                 \
  }
NODE_LIST(DEFINE_VISITOR_FUNCTION)
#undef DEFINE_VISITOR_FUNCTION

 private:
  GrowableArray<AstNode*>* nodes_;
  DISALLOW_COPY_AND_ASSIGN(AstNodeCollector);
};


void SequenceNode::CollectAllNodes(GrowableArray<AstNode*>* nodes) {
  AstNodeCollector node_collector(nodes);
  this->Visit(&node_collector);
}


void SequenceNode::VisitChildren(AstNodeVisitor* visitor) const {
  for (intptr_t i = 0; i < this->length(); i++) {
    NodeAt(i)->Visit(visitor);
  }
}


void PrimaryNode::VisitChildren(AstNodeVisitor* visitor) const {
}


void ArgumentListNode::VisitChildren(AstNodeVisitor* visitor) const {
  for (intptr_t i = 0; i < this->length(); i++) {
    NodeAt(i)->Visit(visitor);
  }
}


void ArrayNode::VisitChildren(AstNodeVisitor* visitor) const {
  for (intptr_t i = 0; i < this->length(); i++) {
    ElementAt(i)->Visit(visitor);
  }
}


// TODO(srdjan): Add code for logical negation.
AstNode* LiteralNode::ApplyUnaryOp(Token::Kind unary_op_kind) {
  if (unary_op_kind == Token::kSUB) {
    if (literal().IsSmi()) {
      Smi& smi = Smi::Handle();
      smi ^= literal().raw();
      const Instance& literal =
          Instance::ZoneHandle(Integer::New(-smi.Value()));
      return new LiteralNode(this->token_index(), literal);
    }
    if (literal().IsDouble()) {
      Double& dbl = Double::Handle();
      dbl ^= literal().raw();
      // Preserve negative zero.
      const Instance& literal =
          Instance::ZoneHandle(Double::New(0.0 - dbl.value()));
      return new LiteralNode(this->token_index(), literal);
    }
  }
  return NULL;
}


bool ComparisonNode::IsKindValid() const {
  return Token::IsRelationalOperator(kind_)
      || Token::IsEqualityOperator(kind_)
      || Token::IsInstanceofOperator(kind_);
}


const char* ComparisonNode::Name() const {
  return Token::Str(kind_);
}


const Instance* ComparisonNode::EvalConstExpr() const {
  const Instance* left_val = this->left()->EvalConstExpr();
  if (left_val == NULL) {
    return NULL;
  }
  const Instance* right_val = this->right()->EvalConstExpr();
  if (right_val == NULL) {
    return NULL;
  }
  switch (kind_) {
    case Token::kLT:
    case Token::kGT:
    case Token::kLTE:
    case Token::kGTE:
      if (left_val->IsNumber() && right_val->IsNumber()) {
        return &Bool::ZoneHandle(Bool::False());
      }
      return NULL;
    case Token::kEQ:
    case Token::kNE:
    case Token::kEQ_STRICT:
    case Token::kNE_STRICT:
      if ((left_val->IsNumber() && right_val->IsNumber()) ||
          (left_val->IsString() && right_val->IsString()) ||
          (left_val->IsBool()   && right_val->IsBool())) {
        return &Bool::ZoneHandle(Bool::False());
      }
      return NULL;
    default:
      return NULL;
  }
  return NULL;
}



bool BinaryOpNode::IsKindValid() const {
  switch (kind_) {
    case Token::kADD:
    case Token::kSUB:
    case Token::kMUL:
    case Token::kDIV:
    case Token::kTRUNCDIV:
    case Token::kMOD:
    case Token::kOR:
    case Token::kAND:
    case Token::kBIT_OR:
    case Token::kBIT_XOR:
    case Token::kBIT_AND:
    case Token::kSHL:
    case Token::kSAR:
    case Token::kSHR:
      return true;
    default:
      return false;
  }
}


const char* BinaryOpNode::Name() const {
  return Token::Str(kind_);
}


const Instance* StringConcatNode::EvalConstExpr() const {
  for (int i = 0; i < values()->length(); i++) {
    if (!values()->ElementAt(i)->IsLiteralNode()) {
      return NULL;
    }
  }
  // All nodes are literals, so this is a compile time constant string.
  // We just return the first literal as value approximation.
  return &values()->ElementAt(0)->AsLiteralNode()->literal();
}


const Instance* BinaryOpNode::EvalConstExpr() const {
  const Instance* left_val = this->left()->EvalConstExpr();
  if (left_val == NULL) {
    return NULL;
  }
  if (!left_val->IsNumber() && !left_val->IsBool()) {
    return NULL;
  }
  const Instance* right_val = this->right()->EvalConstExpr();
  if (right_val == NULL) {
    return NULL;
  }
  switch (kind_) {
    case Token::kADD:
    case Token::kSUB:
    case Token::kMUL:
    case Token::kDIV:
    case Token::kMOD:
      if (left_val->IsInteger()) {
        if (right_val->IsInteger()) {
          return left_val;
        } else if (right_val->IsNumber()) {
          return right_val;
        }
      } else if (left_val->IsNumber() &&
                 right_val->IsNumber()) {
        return left_val;
      }
      return NULL;
    case Token::kTRUNCDIV:
    case Token::kBIT_OR:
    case Token::kBIT_XOR:
    case Token::kBIT_AND:
    case Token::kSHL:
    case Token::kSAR:
    case Token::kSHR:
      if (left_val->IsInteger() &&
          right_val->IsInteger()) {
        return right_val;
      }
      return NULL;
    case Token::kOR:
    case Token::kAND:
      if (left_val->IsBool() && right_val->IsBool()) {
        return left_val;
      }
      return NULL;
    default:
      UNREACHABLE();
      return NULL;
  }
  return NULL;
}


AstNode* UnaryOpNode::UnaryOpOrLiteral(intptr_t token_index,
                                           Token::Kind kind,
                                           AstNode* operand) {
  AstNode* new_operand = operand->ApplyUnaryOp(kind);
  if (new_operand != NULL) {
    return new_operand;
  }
  return new UnaryOpNode(token_index, kind, operand);
}


bool UnaryOpNode::IsKindValid() const {
  switch (kind_) {
    case Token::kADD:
    case Token::kSUB:
    case Token::kNOT:
    case Token::kBIT_NOT:
      return true;
    default:
      return false;
  }
}


const Instance* UnaryOpNode::EvalConstExpr() const {
  const Instance* val = this->operand()->EvalConstExpr();
  if (val == NULL) {
    return NULL;
  }
  switch (kind_) {
    case Token::kADD:
    case Token::kSUB:
      return val->IsNumber() ? val : NULL;
    case Token::kNOT:
      return val->IsBool() ? val : NULL;
    case Token::kBIT_NOT:
      return val->IsInteger() ? val : NULL;
    default:
      return NULL;
  }
}


const char* UnaryOpNode::Name() const {
  return Token::Str(kind_);
}


const char* IncrOpLocalNode::Name() const {
  switch (kind_) {
    case Token::kINCR:
      return prefix_ ? "local_pre_++" : "local_post_++";
    case Token::kDECR:
      return prefix_ ? "local_pre_--" : "local_post_--";
    default:
      UNREACHABLE();
      return NULL;
  }
}


const char* IncrOpInstanceFieldNode::Name() const {
  switch (kind_) {
    case Token::kINCR:
      return prefix_ ? "instance_field_pre_++" : "instance_field_post_++";
    case Token::kDECR:
      return prefix_ ? "instance_field_pre_--" : "instance_field_post_--";
    default:
      UNREACHABLE();
      return NULL;
  }
}


const char* IncrOpStaticFieldNode::Name() const {
  switch (kind_) {
    case Token::kINCR:
      return prefix_ ? "static_field_pre_++" : "static_field_post_++";
    case Token::kDECR:
      return prefix_ ? "static_field_pre_--" : "static_field_post_--";
    default:
      UNREACHABLE();
      return NULL;
  }
}


const char* JumpNode::Name() const {
  return Token::Str(kind_);
}


const char* IncrOpIndexedNode::Name() const {
  switch (kind_) {
    case Token::kINCR:
      return prefix_ ? "indexed_pre_++" : "indexed_post_++";
    case Token::kDECR:
      return prefix_ ? "indexed_pre_--" : "indexed_post_--";
    default:
      UNREACHABLE();
      return NULL;
  }
}


AstNode* LoadLocalNode::MakeAssignmentNode(AstNode* rhs) {
  if (local().is_final()) {
    return NULL;
  }
  return new StoreLocalNode(token_index(), local(), rhs);
}


AstNode* LoadLocalNode::MakeIncrOpNode(intptr_t token_index,
                                       Token::Kind kind,
                                       bool is_prefix) {
  if (local().is_final()) {
    return NULL;
  }
  return new IncrOpLocalNode(token_index, kind, is_prefix, local());
}


AstNode* LoadStaticFieldNode::MakeAssignmentNode(AstNode* rhs) {
  return new StoreStaticFieldNode(token_index(), field(), rhs);
}


AstNode* LoadStaticFieldNode::MakeIncrOpNode(intptr_t token_index,
                                             Token::Kind kind,
                                             bool is_prefix) {
  return new IncrOpStaticFieldNode(token_index, kind, is_prefix, field());
}


AstNode* InstanceGetterNode::MakeAssignmentNode(AstNode* rhs) {
  return new InstanceSetterNode(token_index(), receiver(), field_name(), rhs);
}


AstNode* InstanceGetterNode::MakeIncrOpNode(intptr_t token_index,
                                            Token::Kind kind,
                                            bool is_prefix) {
  return new IncrOpInstanceFieldNode(token_index,
                                     kind,
                                     is_prefix,
                                     receiver(),
                                     field_name());
}


AstNode* LoadIndexedNode::MakeAssignmentNode(AstNode* rhs) {
  return new StoreIndexedNode(token_index(), array(), index_expr(), rhs);
}


AstNode* LoadIndexedNode::MakeIncrOpNode(intptr_t token_index,
                                         Token::Kind kind,
                                         bool is_prefix) {
  return new IncrOpIndexedNode(token_index,
                               kind,
                               is_prefix,
                               array(),
                               index_expr());
}


AstNode* StaticGetterNode::MakeAssignmentNode(AstNode* rhs) {
  return new StaticSetterNode(token_index(), cls(), field_name(), rhs);
}


AstNode* StaticGetterNode::MakeIncrOpNode(intptr_t token_index,
                                          Token::Kind kind,
                                          bool is_prefix) {
  return new IncrOpStaticFieldNode(token_index,
                                   kind,
                                   is_prefix,
                                   cls(),
                                   field_name());
}


const Instance* StaticGetterNode::EvalConstExpr() const {
  const String& getter_name =
      String::Handle(Field::GetterName(this->field_name()));
  const Function& getter_func =
      Function::Handle(this->cls().LookupStaticFunction(getter_name));
  if (getter_func.IsNull() || !getter_func.is_const()) {
    return NULL;
  }
  GrowableArray<const Object*> arguments;
  const Array& kNoArgumentNames = Array::Handle();
  const Instance& field_value =
      Instance::ZoneHandle(DartEntry::InvokeStatic(getter_func,
                                                   arguments,
                                                   kNoArgumentNames));
  if (field_value.IsUnhandledException()) {
    return NULL;
  }
  if (!field_value.IsNull()) {
    return &field_value;
  }
  return NULL;
}

}  // namespace dart
