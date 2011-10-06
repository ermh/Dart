// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/code_generator.h"

#include "vm/code_index_table.h"
#include "vm/code_patcher.h"
#include "vm/compiler.h"
#include "vm/dart_entry.h"
#include "vm/exceptions.h"
#include "vm/ic_stubs.h"
#include "vm/object_store.h"
#include "vm/resolver.h"
#include "vm/runtime_entry.h"
#include "vm/stack_frame.h"
#include "vm/verifier.h"

namespace dart {

DEFINE_FLAG(bool, inline_cache, true, "enable inline caches");
DEFINE_FLAG(bool, trace_deopt, false, "Trace deoptimization");
DEFINE_FLAG(bool, trace_ic, false, "trace IC handling");
DEFINE_FLAG(bool, trace_patching, false, "Trace patching of code.");
DEFINE_FLAG(bool, trace_runtime_calls, false, "Trace runtime calls.");


const Array& CodeGenerator::ArgumentsDescriptor(
    int num_arguments,
    const Array& optional_arguments_names) {
  const intptr_t num_named_args =
      optional_arguments_names.IsNull() ? 0 : optional_arguments_names.Length();
  const intptr_t num_pos_args = num_arguments - num_named_args;

  // Build the argument descriptor array, which consists of the total number of
  // arguments, the number of positional arguments, alphabetically sorted
  // pairs of name/position, and a terminating null.
  const int descriptor_len = 3 + (2 * num_named_args);
  Array& descriptor = Array::ZoneHandle(Array::New(descriptor_len));

  // Set total number of passed arguments.
  descriptor.SetAt(0, Smi::Handle(Smi::New(num_arguments)));
  // Set number of positional arguments.
  descriptor.SetAt(1, Smi::Handle(Smi::New(num_pos_args)));
  // Set alphabetically sorted pairs of name/position for named arguments.
  String& name = String::Handle();
  Smi& pos = Smi::Handle();
  for (int i = 0; i < num_named_args; i++) {
    name ^= optional_arguments_names.At(i);
    pos = Smi::New(num_pos_args + i);
    int j = i;
    // Shift already inserted pairs with "larger" names.
    String& name_j = String::Handle();
    Smi& pos_j = Smi::Handle();
    while (--j >= 0) {
      name_j ^= descriptor.At(2 + (2 * j));
      const intptr_t result = name.CompareTo(name_j);
      ASSERT(result != 0);  // Duplicate argument names checked in parser.
      if (result > 0) break;
      pos_j ^= descriptor.At(3 + (2 * j));
      descriptor.SetAt(2 + (2 * (j + 1)), name_j);
      descriptor.SetAt(3 + (2 * (j + 1)), pos_j);
    }
    // Insert pair in descriptor array.
    descriptor.SetAt(2 + (2 * (j + 1)), name);
    descriptor.SetAt(3 + (2 * (j + 1)), pos);
  }
  // Set terminating null.
  descriptor.SetAt(descriptor_len - 1, Object::Handle());

  // Share the immutable descriptor when possible by canonicalizing it.
  descriptor.MakeImmutable();
  descriptor ^= descriptor.Canonicalize();
  return descriptor;
}


DEFINE_RUNTIME_ENTRY(TraceFunctionEntry, 1) {
  ASSERT(arguments.Count() == kTraceFunctionEntryRuntimeEntry.argument_count());
  const Function& function = Function::CheckedHandle(arguments.At(0));
  const String& function_name = String::Handle(function.name());
  const String& class_name =
      String::Handle(Class::Handle(function.owner()).Name());
  OS::Print("> Entering '%s.%s'\n",
      class_name.ToCString(), function_name.ToCString());
}


DEFINE_RUNTIME_ENTRY(TraceFunctionExit, 1) {
  ASSERT(arguments.Count() == kTraceFunctionExitRuntimeEntry.argument_count());
  const Function& function = Function::CheckedHandle(arguments.At(0));
  const String& function_name = String::Handle(function.name());
  const String& class_name =
      String::Handle(Class::Handle(function.owner()).Name());
  OS::Print("< Exiting '%s.%s'\n",
      class_name.ToCString(), function_name.ToCString());
}


// Allocation of a fixed length array of given element type.
// Arg0: array length.
// Arg1: array element type.
// Arg2: type arguments of the instantiator.
// Return value: newly allocated array of length arg0.
DEFINE_RUNTIME_ENTRY(AllocateArray, 3) {
  ASSERT(arguments.Count() == kAllocateArrayRuntimeEntry.argument_count());
  const Smi& length = Smi::CheckedHandle(arguments.At(0));
  const Array& array = Array::Handle(Array::New(length.Value()));
  arguments.SetReturn(array);
  TypeArguments& element_type = TypeArguments::CheckedHandle(arguments.At(1));
  if (element_type.IsNull()) {
    // No instantiator required for a raw type.
    ASSERT(TypeArguments::CheckedHandle(arguments.At(2)).IsNull());
    return;
  }
  // An Array takes only one type argument.
  ASSERT(element_type.Length() == 1);
  const TypeArguments& instantiator =
      TypeArguments::CheckedHandle(arguments.At(2));
  if (instantiator.IsNull()) {
    // Either the type element is instantiated (use it), or the instantiator is
    // of a raw type and we cannot instantiate the element type (leave as null).
    if (element_type.IsInstantiated()) {
      array.SetTypeArguments(element_type);
    }
    return;
  }
  ASSERT(!element_type.IsInstantiated());
  // If possible, use the instantiator as the type argument vector.
  if (element_type.IsUninstantiatedIdentity() && (instantiator.Length() == 1)) {
    // No need to check that the instantiator is a TypeArray, since the virtual
    // call to Length() handles other cases that are harder to inline.
    element_type = instantiator.raw();
  } else {
    element_type = TypeArguments::NewInstantiatedTypeArguments(element_type,
                                                               instantiator);
  }
  array.SetTypeArguments(element_type);
}


// Allocate a new object.
// Arg0: class of the object that needs to be allocated.
// Arg1: type arguments of the object that needs to be allocated.
// Arg2: type arguments of the instantiator.
// Return value: newly allocated object.
DEFINE_RUNTIME_ENTRY(AllocateObject, 3) {
  ASSERT(arguments.Count() == kAllocateObjectRuntimeEntry.argument_count());
  const Class& cls = Class::CheckedHandle(arguments.At(0));
  const Instance& instance = Instance::Handle(Instance::New(cls));
  arguments.SetReturn(instance);
  if (!cls.IsParameterized()) {
    // No type arguments required for a non-parameterized type.
    ASSERT(Instance::CheckedHandle(arguments.At(1)).IsNull());
    return;
  }
  TypeArguments& type_arguments = TypeArguments::CheckedHandle(arguments.At(1));
  if (type_arguments.IsNull()) {
    // No instantiator is required for a raw type.
    ASSERT(Instance::CheckedHandle(arguments.At(2)).IsNull());
    return;
  }
  ASSERT(type_arguments.Length() == cls.NumTypeArguments());
  const TypeArguments& instantiator =
      TypeArguments::CheckedHandle(arguments.At(2));
  if (instantiator.IsNull()) {
    // Either the type argument vector is instantiated (use it), or the
    // instantiator is of a raw type and we cannot instantiate the type argument
    // vector (leave it as null).
    if (type_arguments.IsInstantiated()) {
      instance.SetTypeArguments(type_arguments);
    }
    return;
  }
  ASSERT(!type_arguments.IsInstantiated());
  // If possible, use the instantiator as the type argument vector.
  if (instantiator.IsTypeArray()) {
    // Code inlined in the caller should have optimized the case where the
    // instantiator is a TypeArray and can be used as type argument vector.
    ASSERT(!type_arguments.IsUninstantiatedIdentity() ||
           (instantiator.Length() != type_arguments.Length()));
    type_arguments = TypeArguments::NewInstantiatedTypeArguments(type_arguments,
                                                                 instantiator);
  } else {
    if (type_arguments.IsUninstantiatedIdentity() &&
        (instantiator.Length() == type_arguments.Length())) {
      type_arguments = instantiator.raw();
    } else {
      type_arguments =
          TypeArguments::NewInstantiatedTypeArguments(type_arguments,
                                                      instantiator);
    }
  }
  instance.SetTypeArguments(type_arguments);
}


// Instantiate type arguments.
// Arg0: uninstantiated type arguments.
// Arg1: instantiator type arguments.
// Return value: instantiated type arguments.
DEFINE_RUNTIME_ENTRY(InstantiateTypeArguments, 2) {
  ASSERT(arguments.Count() ==
         kInstantiateTypeArgumentsRuntimeEntry.argument_count());
  TypeArguments& type_arguments = TypeArguments::CheckedHandle(arguments.At(0));
  const TypeArguments& instantiator =
      TypeArguments::CheckedHandle(arguments.At(1));
  ASSERT(!type_arguments.IsNull() &&
         !type_arguments.IsInstantiated() &&
         !instantiator.IsNull());
  // Code inlined in the caller should have optimized the case where the
  // instantiator can be used as type argument vector.
  ASSERT(!type_arguments.IsUninstantiatedIdentity() ||
         !instantiator.IsTypeArray() ||
         (instantiator.Length() != type_arguments.Length()));
  type_arguments = TypeArguments::NewInstantiatedTypeArguments(type_arguments,
                                                               instantiator);
  arguments.SetReturn(type_arguments);
}


// Allocate a new closure.
// Arg0: local function.
// TODO(regis): Arg1: type arguments of the closure.
// TODO(regis): Arg2: type arguments of the instantiator.
// Return value: newly allocated closure.
DEFINE_RUNTIME_ENTRY(AllocateClosure, 1) {
  ASSERT(arguments.Count() == kAllocateClosureRuntimeEntry.argument_count());
  const Function& function = Function::CheckedHandle(arguments.At(0));
  // TODO(regis): Process type arguments unless the closure is static.
  // The current context was saved in the Isolate structure when entering the
  // runtime.
  const Context& context = Context::Handle(Isolate::Current()->top_context());
  ASSERT(!context.IsNull());
  arguments.SetReturn(Closure::Handle(Closure::New(function, context)));
}


// Allocate a new static implicit closure.
// Arg0: local function.
// Return value: newly allocated closure.
DEFINE_RUNTIME_ENTRY(AllocateStaticImplicitClosure, 1) {
  ASSERT(arguments.Count() ==
         kAllocateStaticImplicitClosureRuntimeEntry.argument_count());
  ObjectStore* object_store = Isolate::Current()->object_store();
  ASSERT(object_store != NULL);
  const Function& function = Function::CheckedHandle(arguments.At(0));
  ASSERT(function.is_static());  // Closure functions are always static for now.
  const Context& context = Context::Handle(object_store->empty_context());
  arguments.SetReturn(Closure::Handle(Closure::New(function, context)));
}


// Allocate a new implicit closure.
// Arg0: local function.
// Arg1: receiver object.
// TODO(regis): Arg2: type arguments of the closure.
// TODO(regis): Arg3: type arguments of the instantiator.
// Return value: newly allocated closure.
DEFINE_RUNTIME_ENTRY(AllocateImplicitClosure, 2) {
  ASSERT(arguments.Count() ==
         kAllocateImplicitClosureRuntimeEntry.argument_count());
  const Function& function = Function::CheckedHandle(arguments.At(0));
  ASSERT(function.is_static());  // Closure functions are always static for now.
  const Instance& receiver = Instance::CheckedHandle(arguments.At(1));
  Context& context = Context::Handle();
  context = Context::New(1);
  context.SetAt(0, receiver);
  arguments.SetReturn(Closure::Handle(Closure::New(function, context)));
  // TODO(regis): Set type arguments.
}


// Allocate a new context large enough to hold the given number of variables.
// Arg0: number of variables.
// Return value: newly allocated context.
DEFINE_RUNTIME_ENTRY(AllocateContext, 1) {
  CHECK_STACK_ALIGNMENT;
  ASSERT(arguments.Count() == kAllocateContextRuntimeEntry.argument_count());
  const Smi& num_variables = Smi::CheckedHandle(arguments.At(0));
  arguments.SetReturn(Context::Handle(Context::New(num_variables.Value())));
}


// Check that the given instance is an instance of the given type.
// Tested instance may not be null, because the null test is inlined.
// Arg0: instance being checked.
// Arg1: type.
// Arg2: type arguments of the instantiator of the type.
// Return value: true or false.
DEFINE_RUNTIME_ENTRY(Instanceof, 3) {
  ASSERT(arguments.Count() == kInstanceofRuntimeEntry.argument_count());
  const Instance& instance = Instance::CheckedHandle(arguments.At(0));
  const Type& type = Type::CheckedHandle(arguments.At(1));
  const TypeArguments& type_instantiator =
      TypeArguments::CheckedHandle(arguments.At(2));
  ASSERT(type.IsFinalized());
  ASSERT(!instance.IsNull());
  const Bool& result = Bool::Handle(
      instance.IsInstanceOf(type, type_instantiator) ?
      Bool::True() : Bool::False());
  arguments.SetReturn(result);
}


DEFINE_RUNTIME_ENTRY(Throw, 1) {
  ASSERT(arguments.Count() == kThrowRuntimeEntry.argument_count());
  const Instance& exception = Instance::CheckedHandle(arguments.At(0));
  Exceptions::Throw(exception);
}


DEFINE_RUNTIME_ENTRY(ReThrow, 2) {
  ASSERT(arguments.Count() == kReThrowRuntimeEntry.argument_count());
  const Instance& exception = Instance::CheckedHandle(arguments.At(0));
  const Instance& stacktrace = Instance::CheckedHandle(arguments.At(1));
  Exceptions::ReThrow(exception, stacktrace);
}


DEFINE_RUNTIME_ENTRY(PatchStaticCall, 0) {
  // This function is called after successful resolving and compilation of
  // the target method.
  ASSERT(arguments.Count() == kPatchStaticCallRuntimeEntry.argument_count());
  DartFrameIterator iterator;
  DartFrame* caller_frame = iterator.NextFrame();
  ASSERT(caller_frame != NULL);
  uword target = 0;
  Function& target_function = Function::Handle();
  CodePatcher::GetStaticCallAt(caller_frame->pc(), &target_function, &target);
  ASSERT(target_function.HasCode());
  uword new_target = Code::Handle(target_function.code()).EntryPoint();
  // Verify that we are not patching repeatedly.
  ASSERT(target != new_target);
  CodePatcher::PatchStaticCallAt(caller_frame->pc(), new_target);
  if (FLAG_trace_patching) {
    OS::Print("PatchStaticCall: patching from 0x%x to '%s' 0x%x\n",
        caller_frame->pc(),
        target_function.ToFullyQualifiedCString(),
        new_target);
  }
}


// Resolves and compiles the target function of an instance call, updates
// function cache of the receiver's class and returns the compiled code or null.
// Only the number of named arguments is checked, but not the actual names.
static RawCode* ResolveCompileInstanceCallTarget(const Instance& receiver) {
  DartFrameIterator iterator;
  DartFrame* caller_frame = iterator.NextFrame();
  ASSERT(caller_frame != NULL);
  int num_arguments = -1;
  int num_named_arguments = -1;
  uword target = 0;
  String& function_name = String::Handle();
  CodePatcher::GetInstanceCallAt(caller_frame->pc(),
                                 &function_name,
                                 &num_arguments,
                                 &num_named_arguments,
                                 &target);
  ASSERT(function_name.IsSymbol());
  Class& receiver_class = Class::Handle();
  if (receiver.IsNull()) {
    // TODO(srdjan): Clarify behavior of null objects.
    receiver_class = Isolate::Current()->object_store()->object_class();
  } else {
    receiver_class = receiver.clazz();
  }
  FunctionsCache functions_cache(receiver_class);
  Code& code = Code::Handle();
      code = functions_cache.LookupCode(function_name,
                                        num_arguments,
                                        num_named_arguments);
  if (!code.IsNull()) {
    // Function's code found in the cache.
    return code.raw();
  }

  Function& function = Function::Handle();
  function = Resolver::ResolveDynamic(receiver,
                                      function_name,
                                      num_arguments,
                                      num_named_arguments);
  if (function.IsNull()) {
    return Code::null();
  } else {
    if (!function.HasCode()) {
      Compiler::CompileFunction(function);
    }
    functions_cache.AddCompiledFunction(function,
                                        num_arguments,
                                        num_named_arguments);
    return function.code();
  }
}


// Result of an invoke may be an unhandled exception, in which case we
// rethrow it.
static void CheckResultException(const Instance& result) {
  if (result.IsUnhandledException()) {
    const UnhandledException& unhandled  = UnhandledException::Handle(
        reinterpret_cast<RawUnhandledException*>(result.raw()));
    const Instance& excp = Instance::Handle(unhandled.exception());
    const Instance& stack = Instance::Handle(unhandled.stacktrace());
    Exceptions::ReThrow(excp, stack);
  }
}


// Resolves an instance function and compiles it if necessary.
//   Arg0: receiver object.
//   Returns: RawCode object or NULL (method not found or not compileable).
// This is called by the megamorphic stub when instance call does not need to be
// patched.
DEFINE_RUNTIME_ENTRY(ResolveCompileInstanceFunction, 1) {
  ASSERT(arguments.Count() ==
         kResolveCompileInstanceFunctionRuntimeEntry.argument_count());
  const Instance& receiver = Instance::CheckedHandle(arguments.At(0));
  const Code& code = Code::Handle(ResolveCompileInstanceCallTarget(receiver));
  arguments.SetReturn(Code::Handle(code.raw()));
}


// Resolve instance call and patch it to jump to IC stub or megamorphic stub.
// After patching the caller's instance call instruction, that call will
// be reexecuted and ran through the created IC stub. The null receivers
// have special handling, i.e., they lead to megamorphic lookup that implements
// the appropriate null behavior.
//   Arg0: receiver object.
DEFINE_RUNTIME_ENTRY(ResolvePatchInstanceCall, 1) {
  ASSERT(arguments.Count() ==
         kResolvePatchInstanceCallRuntimeEntry.argument_count());
  const Instance& receiver = Instance::CheckedHandle(arguments.At(0));
  const Code& code = Code::Handle(ResolveCompileInstanceCallTarget(receiver));
  DartFrameIterator iterator;
  DartFrame* caller_frame = iterator.NextFrame();
  String& function_name = String::Handle();
  if ((!receiver.IsNull() && code.IsNull()) || !FLAG_inline_cache) {
    // We did not find a method; it means either that we need to invoke
    // noSuchMethod or that we have encountered a situation with implicit
    // closures. All these cases are handled by the megamorphic lookup stub.
    CodePatcher::PatchInstanceCallAt(
        caller_frame->pc(), StubCode::MegamorphicLookupEntryPoint());
    if (FLAG_trace_ic) {
      OS::Print("IC: cannot find function at 0x%x -> megamorphic lookup.\n",
          caller_frame->pc());
    }
    if (FLAG_trace_patching) {
      OS::Print("ResolvePatchInstanceCall: patching 0x%x to megamorphic\n",
          caller_frame->pc());
    }
  } else {
    int num_arguments = -1;
    int num_named_arguments = -1;
    uword caller_target = 0;
    CodePatcher::GetInstanceCallAt(caller_frame->pc(),
                                   &function_name,
                                   &num_arguments,
                                   &num_named_arguments,
                                   &caller_target);
    // If caller_target is not in CallInstanceFunction stub (resolve call)
    // then it must be pointing to an IC stub.
    const Class& receiver_class = Class::ZoneHandle(receiver.clazz());
    const bool ic_miss =
        !StubCode::InCallInstanceFunctionStubCode(caller_target);
    GrowableArray<const Class*> classes;
    GrowableArray<const Function*> targets;
    if (ic_miss) {
      bool is_ic =
          ICStubs::RecognizeICStub(caller_target, &classes, &targets);
      ASSERT(is_ic);
      ASSERT(classes.length() == targets.length());
      // The returned classes array can be empty if the first patch occured
      // with a null class. 'receiver_class' should not exists.
      ASSERT(ICStubs::IndexOfClass(classes, receiver_class) < 0);
      ASSERT(!code.IsNull());
      ASSERT(!receiver_class.IsNullClass());
      const Function& function = Function::ZoneHandle(code.function());
      targets.Add(&function);
      classes.Add(&receiver_class);
    } else {
      // First patch of instance call.
      // Do not add classes for null receiver. For first IC patch it means that
      // the IC will always miss and jump to megamorphic lookup (null handling).
      if (!receiver_class.IsNullClass()) {
        ASSERT(!code.IsNull());
        const Function& function = Function::ZoneHandle(code.function());
        targets.Add(&function);
        classes.Add(&receiver_class);
      }
    }
    const Code& ic_code = Code::Handle(ICStubs::GetICStub(classes, targets));
    if (FLAG_trace_ic) {
      CodeIndexTable* ci_table = Isolate::Current()->code_index_table();
      ASSERT(ci_table != NULL);
      const Function& caller =
          Function::Handle(ci_table->LookupFunction(caller_frame->pc()));
      const char* patch_kind = ic_miss ? "miss" : "patch";
      OS::Print("IC %s at 0x%x '%s' (receiver:'%s' function:'%s')",
          patch_kind,
          caller_frame->pc(),
          String::Handle(caller.name()).ToCString(),
          receiver.ToCString(),
          function_name.ToCString());
      OS::Print(" patched to 0x%x\n", ic_code.EntryPoint());
      if (ic_miss) {
        for (int i = 0; i < classes.length(); i++) {
          OS::Print("  IC Miss on %s\n", classes[i]->ToCString());
        }
      }
    }
    CodePatcher::PatchInstanceCallAt(caller_frame->pc(), ic_code.EntryPoint());
    if (FLAG_trace_patching) {
      OS::Print("ResolvePatchInstanceCall: patching 0x%x to ic 0x%x\n",
          caller_frame->pc(), ic_code.EntryPoint());
    }
  }
}


static RawFunction* LookupDynamicFunction(const Class& in_cls,
                                          const String& name) {
  Class& cls = Class::Handle();
  // For lookups treat null as an instance of class Object.
  if (in_cls.IsNullClass()) {
    cls = Isolate::Current()->object_store()->object_class();
  } else {
    cls = in_cls.raw();
  }

  Function& function = Function::Handle();
  while (!cls.IsNull()) {
    // Check if function exists.
    function = cls.LookupDynamicFunction(name);
    if (!function.IsNull()) {
      break;
    }
    cls = cls.SuperClass();
  }
  return function.raw();
}


// Resolve an implicit closure by checking if an instance function
// of the same name exists and creating a closure object of the function.
// Arg0: receiver object.
// Arg1: original function name.
// Returns: Closure object or NULL (instance function not found).
// This is called by the megamorphic stub when it is unable to resolve an
// instance method. This is done just before the call to noSuchMethod.
DEFINE_RUNTIME_ENTRY(ResolveImplicitClosureFunction, 2) {
  ASSERT(arguments.Count() ==
         kResolveImplicitClosureFunctionRuntimeEntry.argument_count());
  const Instance& receiver = Instance::CheckedHandle(arguments.At(0));
  const String& original_func_name = String::CheckedHandle(arguments.At(1));
  const String& getter_prefix = String::Handle(String::New("get:"));
  Closure& closure = Closure::Handle();
  if (!original_func_name.StartsWith(getter_prefix)) {
    // This is not a getter so can't be the case where we are trying to
    // create an implicit closure of an instance function.
    arguments.SetReturn(closure);
    return;
  }
  Class& receiver_class = Class::Handle();
  receiver_class ^= receiver.clazz();
  ASSERT(!receiver_class.IsNull());
  String& func_name = String::Handle();
  func_name = String::SubString(original_func_name, getter_prefix.Length());
  func_name = String::NewSymbol(func_name);
  const Function& function =
      Function::Handle(LookupDynamicFunction(receiver_class, func_name));
  if (function.IsNull()) {
    // There is no function of the same name so can't be the case where
    // we are trying to create an implicit closure of an instance function.
    arguments.SetReturn(closure);
    return;
  }
  Function& implicit_closure_function =
      Function::Handle(function.ImplicitClosureFunction());
  // Create a closure object for the implicit closure function.
  const Context& context = Context::Handle(Context::New(1));
  context.SetAt(0, receiver);
  closure = Closure::New(implicit_closure_function, context);
  arguments.SetReturn(closure);
}


// Resolve an implicit closure by invoking getter and checking if the return
// value from getter is a closure.
// Arg0: receiver object.
// Arg1: original function name.
// Returns: Closure object or NULL (closure not found).
// This is called by the megamorphic stub when it is unable to resolve an
// instance method. This is done just before the call to noSuchMethod.
DEFINE_RUNTIME_ENTRY(ResolveImplicitClosureThroughGetter, 2) {
  ASSERT(arguments.Count() ==
         kResolveImplicitClosureThroughGetterRuntimeEntry.argument_count());
  const Instance& receiver = Instance::CheckedHandle(arguments.At(0));
  const String& original_function_name = String::CheckedHandle(arguments.At(1));
  const int kNumArguments = 1;
  const int kNumNamedArguments = 0;
  const String& getter_function_name =
      String::Handle(Field::GetterName(original_function_name));
  Function& function = Function::ZoneHandle(
      Resolver::ResolveDynamic(receiver,
                               getter_function_name,
                               kNumArguments,
                               kNumNamedArguments));
  Code& code = Code::Handle();
  if (function.IsNull()) {
    arguments.SetReturn(code);
    return;  // No getter function found so can't be an implicit closure.
  }
  GrowableArray<const Object*> invoke_arguments(0);
  const Array& kNoArgumentNames = Array::Handle();
  const Instance& result =
      Instance::Handle(
          DartEntry::InvokeDynamic(receiver,
                                   function,
                                   invoke_arguments,
                                   kNoArgumentNames));
  if (result.IsUnhandledException()) {
    arguments.SetReturn(code);
    return;  // Error accessing getter, treat as no such method.
  }
  if (!result.IsSmi()) {
    const Class& cls = Class::Handle(result.clazz());
    ASSERT(!cls.IsNull());
    function = cls.signature_function();
    if (!function.IsNull()) {
      arguments.SetReturn(result);
      return;  // Return closure object.
    }
  }
  Exceptions::ThrowByType(Exceptions::kObjectNotClosure, invoke_arguments);
}


// Invoke Implicit Closure function.
// Arg0: closure object.
// Arg1: arguments descriptor (originally passed as dart instance invocation).
// Arg2: arguments array (originally passed to dart instance invocation).
DEFINE_RUNTIME_ENTRY(InvokeImplicitClosureFunction, 3) {
  ASSERT(arguments.Count() ==
         kInvokeImplicitClosureFunctionRuntimeEntry.argument_count());
  const Closure& closure = Closure::CheckedHandle(arguments.At(0));
  const Array& arg_descriptor = Array::CheckedHandle(arguments.At(1));
  const Array& func_arguments = Array::CheckedHandle(arguments.At(2));
  const Function& function = Function::Handle(closure.function());
  ASSERT(!function.IsNull());
  if (!function.HasCode()) {
    Compiler::CompileFunction(function);
  }
  const Context& context = Context::Handle(closure.context());
  const Code& code = Code::Handle(function.code());
  ASSERT(!code.IsNull());
  const Instructions& instrs = Instructions::Handle(code.instructions());
  ASSERT(!instrs.IsNull());

  // Adjust arguments descriptor array to account for removal of the receiver
  // parameter. Since the arguments descriptor array is canonicalized, create a
  // new one instead of patching the original one.
  const intptr_t len = arg_descriptor.Length();
  const intptr_t num_named_args = (len - 3) / 2;
  const Array& adjusted_arg_descriptor = Array::Handle(Array::New(len));
  Smi& smi = Smi::Handle();
  smi ^= arg_descriptor.At(0);  // Get argument length.
  smi = Smi::New(smi.Value() - 1);  // Adjust argument length.
  ASSERT(smi.Value() == func_arguments.Length());
  adjusted_arg_descriptor.SetAt(0, smi);
  smi ^= arg_descriptor.At(1);  // Get number of positional parameters.
  smi = Smi::New(smi.Value() - 1);  // Adjust number of positional params.
  adjusted_arg_descriptor.SetAt(1, smi);
  // Adjust name/position pairs for each named argument.
  String& named_arg_name = String::Handle();
  Smi& named_arg_pos = Smi::Handle();
  for (intptr_t i = 0; i < num_named_args; i++) {
    const int index = 2 + (2 * i);
    named_arg_name ^= arg_descriptor.At(index);
    ASSERT(named_arg_name.IsSymbol());
    adjusted_arg_descriptor.SetAt(index, named_arg_name);
    named_arg_pos ^= arg_descriptor.At(index + 1);
    named_arg_pos = Smi::New(named_arg_pos.Value() - 1);
    adjusted_arg_descriptor.SetAt(index + 1, named_arg_pos);
  }
  adjusted_arg_descriptor.SetAt(len - 1, Object::Handle(Object::null()));
  // It is too late to share the descriptor by canonicalizing it. However, it is
  // important that the argument names are canonicalized (i.e. are symbols).

  // Receiver parameter has already been skipped by caller.
  GrowableArray<const Object*> invoke_arguments(0);
  for (intptr_t i = 0; i < func_arguments.Length(); i++) {
    const Object& value = Object::Handle(func_arguments.At(i));
    invoke_arguments.Add(&value);
  }

  // Now Call the invoke stub which will invoke the closure.
  DartEntry::invokestub entrypoint = reinterpret_cast<DartEntry::invokestub>(
      StubCode::InvokeDartCodeEntryPoint());
  ASSERT(context.isolate() == Isolate::Current());
  const Instance& result = Instance::Handle(
      entrypoint(instrs.EntryPoint(),
                 adjusted_arg_descriptor,
                 invoke_arguments.data(),
                 context));
  CheckResultException(result);
  arguments.SetReturn(result);
}


// Invoke appropriate noSuchMethod function.
// Arg0: receiver.
// Arg1: original function name.
// Arg2: original arguments descriptor array.
// Arg3: original arguments array.
DEFINE_RUNTIME_ENTRY(InvokeNoSuchMethodFunction, 4) {
  ASSERT(arguments.Count() ==
         kInvokeNoSuchMethodFunctionRuntimeEntry.argument_count());
  const Instance& receiver = Instance::CheckedHandle(arguments.At(0));
  const String& original_function_name = String::CheckedHandle(arguments.At(1));
  ASSERT(!Array::CheckedHandle(arguments.At(2)).IsNull());
  const Array& orig_arguments = Array::CheckedHandle(arguments.At(3));
  // TODO(regis): The signature of the "noSuchMethod" method has to change from
  // noSuchMethod(String name, Array arguments) to something like
  // noSuchMethod(InvocationMirror call).
  const int kNumArguments = 3;
  const int kNumNamedArguments = 0;
  const Array& kNoArgumentNames = Array::Handle();
  const String& function_name =
      String::Handle(String::NewSymbol("noSuchMethod"));
  const Function& function = Function::ZoneHandle(
      Resolver::ResolveDynamic(receiver,
                               function_name,
                               kNumArguments,
                               kNumNamedArguments));
  ASSERT(!function.IsNull());
  GrowableArray<const Object*> invoke_arguments(2);
  invoke_arguments.Add(&original_function_name);
  invoke_arguments.Add(&orig_arguments);
  const Instance& result = Instance::Handle(
      DartEntry::InvokeDynamic(receiver,
                               function,
                               invoke_arguments,
                               kNoArgumentNames));
  CheckResultException(result);
  arguments.SetReturn(result);
}


// Report that an object is not a closure.
// Arg0: non-closure object.
// Arg1: arguments array.
DEFINE_RUNTIME_ENTRY(ReportObjectNotClosure, 2) {
  ASSERT(arguments.Count() ==
         kReportObjectNotClosureRuntimeEntry.argument_count());
  const Instance& bad_closure = Instance::CheckedHandle(arguments.At(0));
  // const Array& arguments = Array::CheckedHandle(arguments.At(1));
  OS::PrintErr("object '%s' is not a closure\n", bad_closure.ToCString());
  GrowableArray<const Object*> args;
  Exceptions::ThrowByType(Exceptions::kObjectNotClosure, args);
}


DEFINE_RUNTIME_ENTRY(ClosureArgumentMismatch, 0) {
  ASSERT(arguments.Count() ==
         kClosureArgumentMismatchRuntimeEntry.argument_count());
  GrowableArray<const Object*> args;
  Exceptions::ThrowByType(Exceptions::kClosureArgumentMismatch, args);
}


DEFINE_RUNTIME_ENTRY(StackOverflow, 0) {
  ASSERT(arguments.Count() ==
         kStackOverflowRuntimeEntry.argument_count());
  uword old_stack_limit = Isolate::Current()->stack_limit();
  Isolate::Current()->AdjustStackLimitForException();
  // Recursive stack overflow check.
  ASSERT(old_stack_limit != Isolate::Current()->stack_limit());
  GrowableArray<const Object*> args;
  Exceptions::ThrowByType(Exceptions::kStackOverflow, args);
  Isolate::Current()->ResetStackLimitAfterException();
}


static void DisableOldCode(const Function& function,
                           const Code& old_code,
                           const Code& new_code) {
  const Array& class_ic_stubs = Array::Handle(old_code.class_ic_stubs());
  if (function.IsClosureFunction()) {
    // Nothing to do, code may not have inline caches.
    ASSERT(class_ic_stubs.Length() == 0);
    return;
  }
  if (function.is_static() || function.IsConstructor()) {
    ASSERT(class_ic_stubs.Length() == 0);
    return;
  }
  Code& ic_stub = Code::Handle();
  for (int i = 0; i < class_ic_stubs.Length(); i += 2) {
    // i: array of classes, i + 1: ic stub code.
    ic_stub ^= class_ic_stubs.At(i + 1);
    ICStubs::PatchTargets(ic_stub.EntryPoint(),
                          old_code.EntryPoint(),
                          new_code.EntryPoint());
  }
  new_code.set_class_ic_stubs(class_ic_stubs);
  old_code.set_class_ic_stubs(Array::Handle(Array::Empty()));
}


// Only unoptimized code has invocation counter threshold checking.
// Once the invocation counter threshold is reached any entry into the
// unoptimized code is redirected to this function.
DEFINE_RUNTIME_ENTRY(OptimizeInvokedFunction, 1) {
  ASSERT(arguments.Count() ==
         kOptimizeInvokedFunctionRuntimeEntry.argument_count());
  const Function& function = Function::CheckedHandle(arguments.At(0));
  ASSERT(function.is_optimizable());
  ASSERT(!Code::Handle(function.code()).is_optimized());
  const Code& unoptimized_code = Code::Handle(function.code());
  // Compilation patches the entry of unoptimized code.
  Compiler::CompileOptimizedFunction(function);
  const Code& optimized_code = Code::Handle(function.code());
  ASSERT(!optimized_code.IsNull());
  ASSERT(!unoptimized_code.IsNull());
  DisableOldCode(function, unoptimized_code, optimized_code);
}


// The caller must be a static call in a Dart frame, or an entry frame.
// Patch static call to point to 'new_entry_point'.
DEFINE_RUNTIME_ENTRY(FixCallersTarget, 1) {
  ASSERT(arguments.Count() == kFixCallersTargetRuntimeEntry.argument_count());
  const Function& function = Function::CheckedHandle(arguments.At(0));
  ASSERT(!function.IsNull());
  ASSERT(function.HasCode());

  StackFrameIterator iterator(StackFrameIterator::kDontValidateFrames);
  StackFrame* frame = iterator.NextFrame();
  while (frame != NULL && !frame->IsDartFrame() && !frame->IsEntryFrame()) {
    frame = iterator.NextFrame();
  }
  ASSERT(frame != NULL);
  if (frame->IsDartFrame()) {
    uword target = 0;
    Function& target_function = Function::Handle();
    CodePatcher::GetStaticCallAt(frame->pc(), &target_function, &target);
    const uword new_entry_point = Code::Handle(function.code()).EntryPoint();
    ASSERT(target != new_entry_point);  // Why patch otherwise.
    ASSERT(target_function.HasCode());
    CodePatcher::PatchStaticCallAt(frame->pc(), new_entry_point);
    if (FLAG_trace_patching) {
      OS::Print("FixCallersTarget: patching from 0x%x to '%s' 0x%x\n",
          frame->pc(),
          target_function.ToFullyQualifiedCString(),
          new_entry_point);
    }
  }
}


// The top Dart frame belongs to the optimized method that needs to be
// deoptimized. The pc of the Dart frame points to the deoptimization point.
// Find the node id of the deoptimization point and find the continuation
// pc in the unoptimized code.
// Since both unoptimized and optimized code have the same layout, we need only
// to patch the pc of the Dart frame and to disable/enable appropriate code.
DEFINE_RUNTIME_ENTRY(Deoptimize, 0) {
  ASSERT(arguments.Count() == kDeoptimizeRuntimeEntry.argument_count());
  DartFrameIterator iterator;
  DartFrame* caller_frame = iterator.NextFrame();
  ASSERT(caller_frame != NULL);
  CodeIndexTable* ci_table = Isolate::Current()->code_index_table();
  const Code& optimized_code =
      Code::Handle(ci_table->LookupCode(caller_frame->pc()));
  const Function& function = Function::Handle(optimized_code.function());
  ASSERT(!function.IsNull());
  const Code& unoptimized_code = Code::Handle(function.unoptimized_code());
  ASSERT(!optimized_code.IsNull() && optimized_code.is_optimized());
  ASSERT(!unoptimized_code.IsNull() && !unoptimized_code.is_optimized());
  const PcDescriptors& descriptors =
      PcDescriptors::Handle(optimized_code.pc_descriptors());
  ASSERT(!descriptors.IsNull());
  // Locate node id at deoptimization point inside optimized code.
  intptr_t deopt_node_id = AstNode::kInvalidId;
  for (int i = 0; i < descriptors.Length(); i++) {
    if (static_cast<uword>(descriptors.PC(i)) == caller_frame->pc()) {
      deopt_node_id = descriptors.NodeId(i);
      break;
    }
  }
  ASSERT(deopt_node_id != AstNode::kInvalidId);
  uword continue_at_pc =
      unoptimized_code.GetDeoptPcAtNodeId(deopt_node_id);
  ASSERT(continue_at_pc != 0);
  if (FLAG_trace_deopt) {
    OS::Print("Deoptimizing at pc 0x%x id %d '%s' -> continue at 0x%x \n",
        caller_frame->pc(), deopt_node_id, function.ToFullyQualifiedCString(),
        continue_at_pc);
  }
  caller_frame->set_pc(continue_at_pc);
  // Clear invocation counter so that the function gets optimized after
  // types/classes have been collected.
  function.set_invocation_counter(0);
  function.set_deoptimization_counter(function.deoptimization_counter() + 1);

  // Get unoptimized code. Compilation restores (reenables) the entry of
  // unoptimized code.
  Compiler::CompileFunction(function);

  DisableOldCode(function, optimized_code, unoptimized_code);
  if (FLAG_trace_deopt) {
    OS::Print("After patching ->0x%x:\n", continue_at_pc);
  }
}


// We are entering function name for a valid argument count.
void FunctionsCache::EnterFunctionAt(int i,
                                     const Array& cache,
                                     const Function& function,
                                     int num_arguments,
                                     int num_named_arguments) {
  ASSERT((i % kNumEntries) == 0);
  ASSERT(function.AreValidArgumentCounts(num_arguments, num_named_arguments));
  cache.SetAt(i + FunctionsCache::kFunctionName,
      String::Handle(function.name()));
  cache.SetAt(i + FunctionsCache::kArgCount,
      Smi::Handle(Smi::New(num_arguments)));
  cache.SetAt(i + FunctionsCache::kNamedArgCount,
      Smi::Handle(Smi::New(num_named_arguments)));
  cache.SetAt(i + FunctionsCache::kFunction, function);
}


void FunctionsCache::AddCompiledFunction(const Function& function,
                                         int num_arguments,
                                         int num_named_arguments) {
  ASSERT(function.HasCode());
  Array& cache = Array::Handle(class_.functions_cache());
  // Search for first free slot. Last entry is always NULL object.
  for (intptr_t i = 0; i < (cache.Length() - kNumEntries); i += kNumEntries) {
    if (Object::Handle(cache.At(i)).IsNull()) {
      EnterFunctionAt(i,
                      cache,
                      function,
                      num_arguments,
                      num_named_arguments);
      return;
    }
  }
  intptr_t ix = cache.Length() - kNumEntries;
  // Grow by 8 entries.
  cache = Array::Grow(cache, cache.Length() + (8 * kNumEntries));
  class_.set_functions_cache(cache);
  EnterFunctionAt(ix,
                  cache,
                  function,
                  num_arguments,
                  num_named_arguments);
}


// TODO(regis): The actual names of named arguments must match as well.
RawCode* FunctionsCache::LookupCode(const String& function_name,
                                    int num_arguments,
                                    int num_named_arguments) {
  const Array& cache = Array::Handle(class_.functions_cache());
  String& test_name = String::Handle();
  for (intptr_t i = 0; i < cache.Length(); i += kNumEntries) {
    test_name ^= cache.At(i + FunctionsCache::kFunctionName);
    if (test_name.IsNull()) {
      // Found NULL, no more entries to check, abort lookup.
      return Code::null();
    }
    if (function_name.Equals(test_name)) {
      Smi& smi = Smi::Handle();
      smi ^= cache.At(i + FunctionsCache::kArgCount);
      if (num_arguments == smi.Value()) {
        smi ^= cache.At(i + FunctionsCache::kNamedArgCount);
        if (num_named_arguments == smi.Value()) {
          Function& result = Function::Handle();
          result ^= cache.At(i + FunctionsCache::kFunction);
          ASSERT(!result.IsNull());
          ASSERT(result.HasCode());
          return result.code();
        }
      }
    }
  }
  // The cache is null terminated, therefore the loop above should never
  // terminate by itself.
  UNREACHABLE();
  return Code::null();
}

}  // namespace dart
