// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <stdio.h>

#include "vm/unit_test.h"

#include "vm/assembler.h"
#include "vm/ast_printer.h"
#include "vm/code_generator.h"
#include "vm/code_index_table.h"
#include "vm/compiler.h"
#include "vm/disassembler.h"
#include "vm/longjump.h"
#include "vm/parser.h"
#include "vm/virtual_memory.h"


namespace dart {

DECLARE_FLAG(bool, disassemble);


static const intptr_t kPos = 1;  // Dummy token index in non-existing source.


TestCaseBase* TestCaseBase::first_ = NULL;
TestCaseBase* TestCaseBase::tail_ = NULL;


TestCaseBase::TestCaseBase(const char* name) : next_(NULL), name_(name) {
  if (first_ == NULL) {
    first_ = this;
  } else {
    tail_->next_ = this;
  }
  tail_ = this;
}


void TestCaseBase::RunAll() {
  TestCaseBase* test = first_;
  while (test != NULL) {
    test->RunTest();
    test = test->next_;
  }
}


Dart_Handle TestCase::LoadTestScript(const char* script,
                                     Dart_NativeEntryResolver resolver) {
  Dart_Handle url = Dart_NewString(TestCase::url());
  Dart_Handle source = Dart_NewString(script);
  Dart_Result result = Dart_LoadScript(url, source, NULL);
  assert(Dart_IsValidResult(result));
  Dart_Handle lib = Dart_GetResult(result);
  result = Dart_SetNativeResolver(lib, resolver);
  assert(Dart_IsValidResult(result));
  return lib;
}


Dart_Handle TestCase::lib() {
  Dart_Handle url = Dart_NewString(TestCase::url());
  Dart_Result result = Dart_LookupLibrary(url);
  assert(Dart_IsValidResult(result));
  Dart_Handle lib = Dart_GetResult(result);
  assert(Dart_IsLibrary(lib));
  return lib;
}


uword AssemblerTest::Assemble() {
  const Code& code = Code::Handle(Code::FinalizeCode(name_, assembler_));
  if (FLAG_disassemble) {
    OS::Print("Code for test '%s' {\n", name_);
    const Instructions& instructions =
        Instructions::Handle(code.instructions());
    uword start = instructions.EntryPoint();
    Disassembler::Disassemble(start, start + assembler_->CodeSize());
    OS::Print("}\n");
  }
  const Instructions& instructions = Instructions::Handle(code.instructions());
  return instructions.EntryPoint();
}


CodeGenTest::CodeGenTest(const char* name)
  : function_(Function::ZoneHandle()),
    node_sequence_(new SequenceNode(kPos, new LocalScope(NULL, 0, 0))),
    default_parameter_values_(Array::ZoneHandle()) {
  ASSERT(name != NULL);
  const String& function_name = String::ZoneHandle(String::NewSymbol(name));
  function_ = Function::New(
      function_name, RawFunction::kFunction, true, false, 0);
  function_.set_result_type(Type::Handle(Type::VarType()));
  // Add function to a class and that class to the class dictionary so that
  // frame walking can be used.
  Class& cls = Class::ZoneHandle();
  const Script& script = Script::Handle();
  cls = Class::New(function_name, script);
  const Array& functions = Array::Handle(Array::New(1));
  functions.SetAt(0, function_);
  cls.SetFunctions(functions);
  Library& lib = Library::Handle(Library::CoreLibrary());
  lib.AddClass(cls);
}


void CodeGenTest::Compile() {
  Assembler assembler;
  ParsedFunction parsed_function(function_);
  parsed_function.set_node_sequence(node_sequence_);
  parsed_function.set_instantiator(NULL);
  parsed_function.set_default_parameter_values(default_parameter_values_);
  bool retval;
  Isolate* isolate = Isolate::Current();
  EXPECT(isolate != NULL);
  LongJump* base = isolate->long_jump_base();
  LongJump jump;
  isolate->set_long_jump_base(&jump);
  if (setjmp(*jump.Set()) == 0) {
    CodeGenerator code_gen(&assembler, parsed_function);
    code_gen.GenerateCode();
    const char* function_fullname = function_.ToFullyQualifiedCString();
    const Code& code =
        Code::Handle(Code::FinalizeCode(function_fullname, &assembler));
    if (FLAG_disassemble) {
      OS::Print("Code for function '%s' {\n", function_fullname);
      const Instructions& instructions =
          Instructions::Handle(code.instructions());
      uword start = instructions.EntryPoint();
      Disassembler::Disassemble(start, start + assembler.CodeSize());
      OS::Print("}\n");
    }
    function_.SetCode(code);
    CodeIndexTable* code_index_table = isolate->code_index_table();
    ASSERT(code_index_table != NULL);
    code_index_table->AddFunction(function_);
    retval = true;
  } else {
    retval = false;
  }
  EXPECT(retval);
  isolate->set_long_jump_base(base);
}


bool CompilerTest::TestCompileScript(const Library& library,
                                     const Script& script) {
  bool retval;
  Isolate* isolate = Isolate::Current();
  ASSERT(isolate != NULL);
  LongJump* base = isolate->long_jump_base();
  LongJump jump;
  isolate->set_long_jump_base(&jump);
  if (setjmp(*jump.Set()) == 0) {
    Compiler::Compile(library, script);
    retval = true;
  } else {
    retval = false;
  }
  isolate->set_long_jump_base(base);
  return retval;
}


bool CompilerTest::TestCompileFunction(const Function& function) {
  bool retval;
  Isolate* isolate = Isolate::Current();
  ASSERT(isolate != NULL);
  LongJump* base = isolate->long_jump_base();
  LongJump jump;
  isolate->set_long_jump_base(&jump);
  if (setjmp(*jump.Set()) == 0) {
    Compiler::CompileFunction(function);
    retval = true;
  } else {
    retval = false;
  }
  isolate->set_long_jump_base(base);
  return retval;
}

}  // namespace dart
