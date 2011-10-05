// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/assert.h"
#include "vm/ast_printer.h"
#include "vm/heap.h"
#include "vm/isolate.h"
#include "vm/object.h"
#include "vm/object_store.h"
#include "vm/unit_test.h"

namespace dart {

TEST_CASE(AstPrinter) {
  const intptr_t kPos = 1;  // Dummy token index in non-existing source.
  LocalVariable* v =
      new LocalVariable(kPos,
                        String::ZoneHandle(String::New("wurscht")),
                        Type::ZoneHandle(Type::VarType()));
  v->set_index(5);
  LoadLocalNode* ll = new LoadLocalNode(kPos, *v);
  ReturnNode* r = new ReturnNode(kPos, ll);
  AstPrinter::PrintNode(r);

  AstNode* l = new LiteralNode(kPos, Smi::ZoneHandle(Smi::New(3)));
  ReturnNode* rl = new ReturnNode(kPos, l);
  AstPrinter::PrintNode(rl);

  AstPrinter::PrintNode(new ReturnNode(kPos));

  AstPrinter::PrintNode(new BinaryOpNode(kPos,
                          Token::kADD,
                          new LiteralNode(kPos, Smi::ZoneHandle(Smi::New(3))),
                          new LiteralNode(kPos, Smi::ZoneHandle(Smi::New(5)))));
  AstPrinter::PrintNode(new UnaryOpNode(kPos, Token::kSUB, ll));
}

}  // namespace dart