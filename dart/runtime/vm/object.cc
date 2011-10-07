// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/object.h"

#include "vm/assembler.h"
#include "vm/assert.h"
#include "vm/bigint_operations.h"
#include "vm/bootstrap.h"
#include "vm/code_generator.h"
#include "vm/code_index_table.h"
#include "vm/code_patcher.h"
#include "vm/compiler.h"
#include "vm/compiler_stats.h"
#include "vm/class_finalizer.h"
#include "vm/dart.h"
#include "vm/debuginfo.h"
#include "vm/growable_array.h"
#include "vm/heap.h"
#include "vm/ic_stubs.h"
#include "vm/object_store.h"
#include "vm/parser.h"
#include "vm/runtime_entry.h"
#include "vm/scopes.h"
#include "vm/timer.h"
#include "vm/unicode.h"

namespace dart {

DEFINE_FLAG(bool, expose_core_impl, false,
    "Enables access to core implementation library (only for testing).");
DEFINE_FLAG(bool, generate_gdb_symbols, false,
    "Generate symbols of generated dart functions for debugging with GDB");

cpp_vtable Object::handle_vtable_ = 0;
cpp_vtable Smi::handle_vtable_ = 0;

// These are initialized to a value that will force a illegal memory access if
// they are being used.
#if defined(RAW_NULL)
#error RAW_NULL should not be defined.
#endif
#define RAW_NULL kHeapObjectTag
RawObject* Object::null_ = reinterpret_cast<RawInstance*>(RAW_NULL);
RawInstance* Object::sentinel_ = reinterpret_cast<RawInstance*>(RAW_NULL);
RawInstance* Object::transition_sentinel_ =
    reinterpret_cast<RawInstance*>(RAW_NULL);
RawClass* Object::class_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::null_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::type_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::parameterized_type_class_ =
    reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::type_parameter_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::instantiated_type_class_ =
    reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::type_arguments_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::type_array_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::instantiated_type_arguments_class_ =
    reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::function_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::field_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::token_stream_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::script_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::library_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::library_prefix_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::code_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::instructions_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::pc_descriptors_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::exception_handlers_class_ =
    reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::context_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
RawClass* Object::context_scope_class_ = reinterpret_cast<RawClass*>(RAW_NULL);
#undef RAW_NULL

int Object::GetSingletonClassIndex(const RawClass* raw_class) {
  ASSERT(raw_class->IsHeapObject());
  if (raw_class == class_class()) {
    return kClassClass;
  } else if (raw_class == null_class()) {
    return kNullClass;
  } else if (raw_class == type_class()) {
    return kTypeClass;
  } else if (raw_class == parameterized_type_class()) {
    return kParameterizedTypeClass;
  } else if (raw_class == type_parameter_class()) {
    return kTypeParameterClass;
  } else if (raw_class == instantiated_type_class()) {
    return kInstantiatedTypeClass;
  } else if (raw_class == type_arguments_class()) {
    return kTypeArgumentsClass;
  } else if (raw_class == type_array_class()) {
    return kTypeArrayClass;
  } else if (raw_class == instantiated_type_arguments_class()) {
    return kInstantiatedTypeArgumentsClass;
  } else if (raw_class == function_class()) {
    return kFunctionClass;
  } else if (raw_class == field_class()) {
    return kFieldClass;
  } else if (raw_class == token_stream_class()) {
    return kTokenStreamClass;
  } else if (raw_class == script_class()) {
    return kScriptClass;
  } else if (raw_class == library_class()) {
    return kLibraryClass;
  } else if (raw_class == library_prefix_class()) {
    return kLibraryPrefixClass;
  } else if (raw_class == code_class()) {
    return kCodeClass;
  } else if (raw_class == instructions_class()) {
    return kInstructionsClass;
  } else if (raw_class == pc_descriptors_class()) {
    return kPcDescriptorsClass;
  } else if (raw_class == exception_handlers_class()) {
    return kExceptionHandlersClass;
  } else if (raw_class == context_class()) {
    return kContextClass;
  } else if (raw_class == context_scope_class()) {
    return kContextScopeClass;
  }
  return kInvalidIndex;
}


RawClass* Object::GetSingletonClass(int index) {
  switch (index) {
    case kClassClass: return class_class();
    case kNullClass: return null_class();
    case kTypeClass: return type_class();
    case kParameterizedTypeClass: return parameterized_type_class();
    case kTypeParameterClass: return type_parameter_class();
    case kInstantiatedTypeClass: return instantiated_type_class();
    case kTypeArgumentsClass: return type_arguments_class();
    case kTypeArrayClass: return type_array_class();
    case kInstantiatedTypeArgumentsClass:
        return instantiated_type_arguments_class();
    case kFunctionClass: return function_class();
    case kFieldClass: return field_class();
    case kTokenStreamClass: return token_stream_class();
    case kScriptClass: return script_class();
    case kLibraryClass: return library_class();
    case kLibraryPrefixClass: return library_prefix_class();
    case kCodeClass: return code_class();
    case kInstructionsClass: return instructions_class();
    case kPcDescriptorsClass: return pc_descriptors_class();
    case kExceptionHandlersClass: return exception_handlers_class();
    case kContextClass: return context_class();
    case kContextScopeClass: return context_scope_class();
    default: break;
  }
  UNREACHABLE();
  return reinterpret_cast<RawClass*>(kHeapObjectTag);  // return RAW_NULL.
}


const char* Object::GetSingletonClassName(int index) {
  switch (index) {
    case kClassClass: return "Class";
    case kNullClass: return "Null";
    case kTypeClass: return "Type";
    case kParameterizedTypeClass: return "ParameterizedType";
    case kTypeParameterClass: return "TypeParameter";
    case kInstantiatedTypeClass: return "InstantiatedType";
    case kTypeArgumentsClass: return "TypeArguments";
    case kTypeArrayClass: return "TypeArray";
    case kInstantiatedTypeArgumentsClass: return "InstantiatedTypeArguments";
    case kFunctionClass: return "Function";
    case kFieldClass: return "Field";
    case kTokenStreamClass: return "TokenStream";
    case kScriptClass: return "Script";
    case kLibraryClass: return "Library";
    case kLibraryPrefixClass: return "LibraryPrefix";
    case kCodeClass: return "Code";
    case kInstructionsClass: return "Instructions";
    case kPcDescriptorsClass: return "PcDescriptors";
    case kExceptionHandlersClass: return "ExceptionHandlers";
    case kContextClass: return "Context";
    case kContextScopeClass: return "ContextScope";
    default: break;
  }
  UNREACHABLE();
  return NULL;
}


void Object::InitOnce() {
  // TODO(iposva): NoGCScope needs to be added here.
  ASSERT(class_class() == null_);
  // Initialize the static vtable values.
  {
    Object fake_object;
    Smi fake_smi;
    Object::handle_vtable_ = fake_object.vtable();
    Smi::handle_vtable_ = fake_smi.vtable();
  }

  Heap* heap = Isolate::Current()->heap();
  // Allocate and initialize the null instance, except its class_ field.
  // 'null_' must be the first object allocated as it is used in allocation to
  // clear the object.
  {
    uword address = heap->Allocate(Instance::InstanceSize(), Heap::kOld);
    null_ = reinterpret_cast<RawInstance*>(address + kHeapObjectTag);
    InitializeObject(address, Instance::InstanceSize());  // Using 'null_'.
  }

  // Initialize object_store empty array to null_ in order to be able to check
  // if the empty array was allocated (RAW_NULL is not available).
  Isolate::Current()->object_store()->set_empty_array(Array::Handle());

  Class& cls = Class::Handle();

  // Allocate and initialize the class class.
  // At this point, class_class_ is still RAW_NULL, i.e. different from 'null_',
  // since 'null_' is not RAW_NULL anymore. However, class_class_ == null_ must
  // be true before calling Class::New<Class>(), or it will fail.
  class_class_ = Class::Handle().raw();  // Set 'class_class_' to 'null_'.
  cls = Class::New<Class>();
  cls.set_is_finalized();
  class_class_ = cls.raw();
  // Make the class_ field point to itself.
  class_class_->ptr()->class_ = class_class_;

  // Allocate and initialize the null class.
  cls = Class::New<Instance>();
  null_class_ = cls.raw();

  // Complete initialization of null_ instance, i.e. initialize its class_
  // field.
  null_->ptr()->class_ = null_class_;

  // Allocate and initialize the sentinel values of an instance class.
  {
    cls = Class::New<Instance>();
    Instance& sentinel = Instance::Handle();
    sentinel ^= Object::Allocate(cls, Instance::InstanceSize(), Heap::kOld);
    sentinel_ = sentinel.raw();

    Instance& transition_sentinel = Instance::Handle();
    transition_sentinel ^=
        Object::Allocate(cls, Instance::InstanceSize(), Heap::kOld);
    transition_sentinel_ = transition_sentinel.raw();
  }

  // Allocate the remaining VM internal classes.
  cls = Class::New<Type>();
  type_class_ = cls.raw();

  cls = Class::New<ParameterizedType>();
  parameterized_type_class_ = cls.raw();

  cls = Class::New<TypeParameter>();
  type_parameter_class_ = cls.raw();

  cls = Class::New<InstantiatedType>();
  instantiated_type_class_ = cls.raw();

  cls = Class::New<TypeArguments>();
  type_arguments_class_ = cls.raw();

  cls = Class::New<TypeArray>();
  type_array_class_ = cls.raw();

  cls = Class::New<InstantiatedTypeArguments>();
  instantiated_type_arguments_class_ = cls.raw();

  cls = Class::New<Function>();
  function_class_ = cls.raw();

  cls = Class::New<Field>();
  field_class_ = cls.raw();

  cls = Class::New<TokenStream>();
  token_stream_class_ = cls.raw();

  cls = Class::New<Script>();
  script_class_ = cls.raw();

  cls = Class::New<Library>();
  library_class_ = cls.raw();

  cls = Class::New<LibraryPrefix>();
  library_prefix_class_ = cls.raw();

  cls = Class::New<Code>();
  code_class_ = cls.raw();

  cls = Class::New<Instructions>();
  instructions_class_ = cls.raw();

  cls = Class::New<PcDescriptors>();
  pc_descriptors_class_ = cls.raw();

  cls = Class::New<ExceptionHandlers>();
  exception_handlers_class_ = cls.raw();

  cls = Class::New<Context>();
  context_class_ = cls.raw();

  cls = Class::New<ContextScope>();
  context_scope_class_ = cls.raw();

  ASSERT(class_class() != null_);
}


void Object::Init(Isolate* isolate) {
  TIMERSCOPE(time_bootstrap);
  ObjectStore* object_store = isolate->object_store();

  Class& cls = Class::Handle();
  Type& type = Type::Handle();
  String& name = String::Handle();
  Array& array = Array::Handle();

  // All RawArray fields will be initialized to an empty array, therefore
  // initialize array class first.
  cls = Class::New<Array>();
  object_store->set_array_class(cls);

  // Array and ImmutableArray are the only VM classes that are parameterized.
  // Since they are pre-finalized, CalculateFieldOffsets() is not called, so we
  // need to set the offset of their type_arguments_ field, which is explicitly
  // declared in RawArray.
  cls.set_type_arguments_instance_field_offset(Array::type_arguments_offset());

  Array& empty_array = Array::Handle();
  empty_array = Array::New(0, Heap::kOld);
  object_store->set_empty_array(empty_array);

  // Re-initialize fields of the array class now that the empty array
  // has been created.
  cls.InitEmptyFields();

  cls = Class::New<ImmutableArray>();
  object_store->set_immutable_array_class(cls);
  cls.set_type_arguments_instance_field_offset(Array::type_arguments_offset());

  // Allocate and initialize the object class and type.
  cls = Class::New<Instance>();
  object_store->set_object_class(cls);
  // The bootstrap script is not compiled yet, so the superclass of Object does
  // not yet point to itself, therefore, Type::NewNonParameterizedType(cls)
  // can safely assert that cls.NumTypeArguments() == 0 without entering an
  // endless loop.
  ASSERT(cls.SuperClass() == Class::null());
  type = Type::NewNonParameterizedType(cls);
  object_store->set_object_type(type);

  cls = Class::New<Smi>();
  object_store->set_smi_class(cls);

  cls = Class::New<Mint>();
  object_store->set_mint_class(cls);

  cls = Class::New<Bigint>();
  object_store->set_bigint_class(cls);

  cls = Class::New<Double>();
  object_store->set_double_class(cls);

  cls = Class::New<OneByteString>();
  object_store->set_one_byte_string_class(cls);

  cls = Class::New<TwoByteString>();
  object_store->set_two_byte_string_class(cls);

  cls = Class::New<FourByteString>();
  object_store->set_four_byte_string_class(cls);

  cls = Class::New<Bool>();
  object_store->set_bool_class(cls);

  cls = Class::New<UnhandledException>();
  object_store->set_unhandled_exception_class(cls);

  cls = Class::New<Stacktrace>();
  object_store->set_stacktrace_class(cls);
  // Set the super type so that the 'toString' method is implemented.
  type = object_store->object_type();
  cls.set_super_type(type);

  cls = Class::New<JSRegExp>();
  object_store->set_jsregexp_class(cls);

  // Setup the symbol table used within the String class.
  const int kInitialSymbolTableSize = 16;
  array = Array::New(kInitialSymbolTableSize + 1);
  // Last element contains the count of used slots.
  array.SetAt(kInitialSymbolTableSize, Smi::Handle(Smi::New(0)));
  object_store->set_symbol_table(array);

  // Basic infrastructure has been setup, initialize the class dictionary.
  Library::InitCoreLibrary(isolate);
  Library& core_lib = Library::Handle(isolate->object_store()->core_library());
  ASSERT(!core_lib.IsNull());
  Library& core_impl_lib = Library::Handle(Library::CoreImplLibrary());
  ASSERT(!core_impl_lib.IsNull());

  // Allocate pre-initialized values.
  Bool& bool_value = Bool::Handle();
  bool_value = Bool::New(true);
  object_store->set_true_value(bool_value);
  bool_value = Bool::New(false);
  object_store->set_false_value(bool_value);

  object_store->set_pending_classes(Array::Handle(Array::Empty()));

  Context& context = Context::Handle(Context::New(0));
  object_store->set_empty_context(context);

  // Now that the String class is initialized and the dictionary has been setup,
  // add the names to preallocated classes and register them in the dictionary.
  const Script& impl_script = Script::Handle(Bootstrap::LoadImplScript());

  name = String::NewSymbol("Smi");
  cls = object_store->smi_class();
  cls.set_name(name);
  cls.set_script(impl_script);
  core_impl_lib.AddClass(cls);

  name = String::NewSymbol("OneByteString");
  cls = object_store->one_byte_string_class();
  cls.set_name(name);
  cls.set_script(impl_script);
  core_impl_lib.AddClass(cls);

  name = String::NewSymbol("TwoByteString");
  cls = object_store->two_byte_string_class();
  cls.set_name(name);
  cls.set_script(impl_script);
  core_impl_lib.AddClass(cls);

  name = String::NewSymbol("FourByteString");
  cls = object_store->four_byte_string_class();
  cls.set_name(name);
  cls.set_script(impl_script);
  core_impl_lib.AddClass(cls);

  name = String::NewSymbol("Mint");
  cls = object_store->mint_class();
  cls.set_name(name);
  cls.set_script(impl_script);
  core_impl_lib.AddClass(cls);

  name = String::NewSymbol("Bigint");
  cls = object_store->bigint_class();
  cls.set_name(name);
  cls.set_script(impl_script);
  core_impl_lib.AddClass(cls);

  name = String::NewSymbol("Double");
  cls = object_store->double_class();
  cls.set_name(name);
  cls.set_script(impl_script);
  core_impl_lib.AddClass(cls);

  name = String::NewSymbol("Bool");
  cls = object_store->bool_class();
  cls.set_name(name);
  cls.set_script(impl_script);
  core_impl_lib.AddClass(cls);

  name = String::NewSymbol("ObjectArray");
  cls = object_store->array_class();
  cls.set_name(name);
  cls.set_script(impl_script);
  core_impl_lib.AddClass(cls);

  name = String::NewSymbol("ImmutableArray");
  cls = object_store->immutable_array_class();
  ASSERT(object_store->immutable_array_class() != object_store->array_class());
  cls.set_name(name);
  cls.set_script(impl_script);
  core_impl_lib.AddClass(cls);

  name = String::NewSymbol("UnhandledException");
  cls = object_store->unhandled_exception_class();
  cls.set_name(name);
  cls.set_script(impl_script);
  core_impl_lib.AddClass(cls);

  name = String::NewSymbol("Stacktrace");
  cls = object_store->stacktrace_class();
  cls.set_name(name);
  cls.set_script(impl_script);
  core_impl_lib.AddClass(cls);

  name = String::NewSymbol("JSSyntaxRegExp");
  cls = object_store->jsregexp_class();
  cls.set_name(name);
  cls.set_script(impl_script);
  core_impl_lib.AddClass(cls);

  // Initialize the base interfaces used by the core VM classes.
  const Script& script = Script::Handle(Bootstrap::LoadScript());

  name = String::NewSymbol("Object");
  cls = object_store->object_class();
  cls.set_name(name);
  cls.set_script(script);
  core_lib.AddClass(cls);

  name = String::NewSymbol("Function");
  cls = Class::NewInterface(name, script);
  core_lib.AddClass(cls);
  type = Type::NewNonParameterizedType(cls);
  object_store->set_function_interface(type);

  name = String::NewSymbol("num");
  cls = Class::NewInterface(name, script);
  core_lib.AddClass(cls);
  type = Type::NewNonParameterizedType(cls);
  object_store->set_number_interface(type);

  name = String::NewSymbol("int");
  cls = Class::NewInterface(name, script);
  core_lib.AddClass(cls);
  type = Type::NewNonParameterizedType(cls);
  object_store->set_int_interface(type);

  name = String::NewSymbol("double");
  cls = Class::NewInterface(name, script);
  core_lib.AddClass(cls);

  name = String::NewSymbol("String");
  cls = Class::NewInterface(name, script);
  core_lib.AddClass(cls);

  name = String::NewSymbol("bool");
  cls = Class::NewInterface(name, script);
  core_lib.AddClass(cls);
  type = Type::NewNonParameterizedType(cls);
  object_store->set_bool_interface(type);

  name = String::NewSymbol("Array");
  cls = Class::NewInterface(name, script);
  core_impl_lib.AddClass(cls);

  // The 'null' class is not registered in the class dictionary and is not
  // named, but a corresponding type is stored in the object store.
  cls = null_class_;
  type = Type::NewNonParameterizedType(cls);
  object_store->set_null_type(type);

  // The 'var' and 'void' classes are not registered in the class dictionary,
  // but they are named and their corresponding types are stored in the object
  // store.
  name = String::NewSymbol("var");
  cls = Class::New<Instance>();
  cls.set_name(name);
  type = Type::NewNonParameterizedType(cls);
  object_store->set_var_type(type);

  name = String::NewSymbol("void");
  cls = Class::New<Instance>();
  cls.set_name(name);
  type = Type::NewNonParameterizedType(cls);
  object_store->set_void_type(type);

  // Finish the initialization by compiling the bootstrap script containing the
  // implementation of the internal classes.
  Bootstrap::Compile(Library::Handle(Library::CoreLibrary()), script);
  Bootstrap::Compile(Library::Handle(Library::CoreImplLibrary()), impl_script);

  Bootstrap::SetupNativeResolver();

  // Remove the Object superclass cycle by setting the super type to null (not
  // to the type of null).
  cls = object_store->object_class();
  cls.set_super_type(Type::Handle());

  ClassFinalizer::VerifyBootstrapClasses();
}


void Object::InitFromSnapshot(Isolate* isolate) {
  TIMERSCOPE(time_bootstrap);
  ObjectStore* object_store = isolate->object_store();

  Class& cls = Class::Handle();

  // Set up empty classes in the object store, these will get
  // initialized correctly when we read from the snapshot.
  // This is done do allow bootstrapping of reading classes from
  // the snapshot.
  cls = Class::New<Array>();
  object_store->set_array_class(cls);

  Array& empty_array = Array::Handle();
  empty_array = Array::New(0);
  object_store->set_empty_array(empty_array);

  cls = Class::New<ImmutableArray>();
  object_store->set_immutable_array_class(cls);

  cls = Class::New<Instance>();
  object_store->set_object_class(cls);

  cls = Class::New<Smi>();
  object_store->set_smi_class(cls);

  cls = Class::New<Mint>();
  object_store->set_mint_class(cls);

  cls = Class::New<Double>();
  object_store->set_double_class(cls);

  cls = Class::New<Bigint>();
  object_store->set_bigint_class(cls);

  cls = Class::New<OneByteString>();
  object_store->set_one_byte_string_class(cls);

  cls = Class::New<TwoByteString>();
  object_store->set_two_byte_string_class(cls);

  cls = Class::New<FourByteString>();
  object_store->set_four_byte_string_class(cls);

  cls = Class::New<Bool>();
  object_store->set_bool_class(cls);

  cls = Class::New<UnhandledException>();
  object_store->set_unhandled_exception_class(cls);

  cls = Class::New<Stacktrace>();
  object_store->set_stacktrace_class(cls);

  cls = Class::New<JSRegExp>();
  object_store->set_jsregexp_class(cls);

  // Allocate pre-initialized values.
  Bool& bool_value = Bool::Handle();
  bool_value = Bool::New(true);
  object_store->set_true_value(bool_value);
  bool_value = Bool::New(false);
  object_store->set_false_value(bool_value);
}


void Object::Print() const {
  OS::Print("%s\n", ToCString());
}


void Object::InitializeObject(uword address, intptr_t size) {
  uword initial_value = reinterpret_cast<uword>(null_);
  uword cur = address;
  uword end = address + size;
  while (cur < end) {
    *reinterpret_cast<uword*>(cur) = initial_value;
    cur += kWordSize;
  }
}


RawObject* Object::Allocate(const Class& cls,
                            intptr_t size,
                            Heap::Space space) {
  Isolate* isolate = Isolate::Current();
  Heap* heap = isolate->heap();

  // TODO(iposva): Get a proper halt instruction from the assembler.
  uword address = heap->Allocate(size, space);
  NoGCScope no_gc;
  InitializeObject(address, size);
  RawObject* raw_obj = reinterpret_cast<RawObject*>(address + kHeapObjectTag);
  raw_obj->ptr()->class_ = cls.raw();
  return raw_obj;
}


#if defined(DEBUG)
void Object::ValidateHeapObject(RawObject* raw_obj) {
  uword addr = RawObject::ToAddr(raw_obj);
  ASSERT(Isolate::Current()->heap()->Contains(addr) ||
         Dart::vm_isolate()->heap()->Contains(addr));
}
#endif  // defined(DEBUG)


RawString* Class::Name() const {
  if (raw_ptr()->name_ != String::null()) {
    return raw_ptr()->name_;
  }
  ASSERT(class_class() != null_);  // Or GetSingletonClassIndex will not work.
  intptr_t index = GetSingletonClassIndex(raw());
  return String::NewSymbol(GetSingletonClassName(index));
}


template <class FakeObject>
RawClass* Class::New() {
  Class& class_class = Class::Handle(Object::class_class());
  Class& result = Class::Handle();
  {
    RawObject* raw = Object::Allocate(class_class,
                                      Class::InstanceSize(),
                                      Heap::kOld);
    NoGCScope no_gc;
    if (class_class.IsNull()) {
      // Allocating class_class, avoid using uninitialized class vtable.
      result.raw_ = raw;
    } else {
      result ^= raw;
    }
  }
  FakeObject fake;
  result.set_handle_vtable(fake.vtable());
  result.set_instance_size(FakeObject::InstanceSize());
  result.set_next_field_offset(FakeObject::InstanceSize());
  result.set_instance_kind(FakeObject::kInstanceKind);
  result.raw_ptr()->is_const_ = false;
  result.raw_ptr()->is_interface_ = false;
  // VM backed classes are almost ready: run checks and resolve class
  // references, but do not recompute size.
  result.raw_ptr()->class_state_ = RawClass::kPreFinalized;
  result.raw_ptr()->type_arguments_instance_field_offset_ = kNoTypeArguments;
  result.raw_ptr()->num_constants_ = 0;
  result.raw_ptr()->num_native_fields_ = 0;
  result.InitEmptyFields();
  return result.raw();
}


// Initialize class fields of type Array with empty array.
void Class::InitEmptyFields() {
  const Array& empty_array = Array::Handle(Array::Empty());
  if (empty_array.IsNull()) {
    // The empty array has not been initialized yet.
    return;
  }
  StorePointer(&raw_ptr()->interfaces_, empty_array.raw());
  // TODO(srdjan): Make functions_cache growable and start with a smaller size.
  Array& fcache =
      Array::Handle(Array::New(FunctionsCache::kNumEntries * 32, Heap::kOld));
  StorePointer(&raw_ptr()->functions_cache_, fcache.raw());
  StorePointer(&raw_ptr()->constants_, empty_array.raw());
  StorePointer(&raw_ptr()->functions_, empty_array.raw());
  StorePointer(&raw_ptr()->fields_, empty_array.raw());
}

void Class::SetFunctions(const Array& value) const {
  ASSERT(!value.IsNull());
  // Bind all the functions in the array to this class.
  Function& func = Function::Handle();
  intptr_t len = value.Length();
  for (intptr_t i = 0; i < len; i++) {
    func ^= value.At(i);
    func.set_owner(*this);
  }
  StorePointer(&raw_ptr()->functions_, value.raw());
}


void Class::set_signature_function(const Function& value) const {
  ASSERT(value.IsClosureFunction() || value.IsSignatureFunction());
  StorePointer(&raw_ptr()->signature_function_, value.raw());
}


void Class::set_class_state(int8_t state) const {
  ASSERT(state == RawClass::kAllocated ||
         state == RawClass::kPreFinalized ||
         state == RawClass::kFinalized);
  raw_ptr()->class_state_ = state;
}


void Class::set_library(const Library& value) const {
  StorePointer(&raw_ptr()->library_, value.raw());
}


void Class::set_type_parameters(const Array& value) const {
  StorePointer(&raw_ptr()->type_parameters_, value.raw());
}


void Class::set_type_parameter_extends(const TypeArray& value) const {
  StorePointer(&raw_ptr()->type_parameter_extends_, value.raw());
}


intptr_t Class::NumTypeParameters() const {
  const Array& type_params = Array::Handle(type_parameters());
  if (type_params.IsNull()) {
    return 0;
  } else {
    return type_params.Length();
  }
}


intptr_t Class::NumTypeArguments() const {
  intptr_t num_type_args = NumTypeParameters();
  const Class& superclass = Class::Handle(SuperClass());
  if (!superclass.IsNull()) {
    num_type_args += superclass.NumTypeArguments();
  }
  return num_type_args;
}


RawClass* Class::SuperClass() const {
  const Type& sup_type = Type::Handle(super_type());
  if (sup_type.IsNull()) {
    return Class::null();
  }
  return sup_type.type_class();
}


void Class::set_super_type(const Type& value) const {
  StorePointer(&raw_ptr()->super_type_, value.raw());
}


RawClass* Class::FactoryClass() const {
  const Type& fact_type = Type::Handle(factory_type());
  if (fact_type.IsNull()) {
    return Class::null();
  }
  return fact_type.type_class();
}


void Class::set_factory_type(const Type& value) const {
  StorePointer(&raw_ptr()->factory_type_, value.raw());
}


// Return a TypeParameter if the type_name is a type parameter of this class.
// Return null otherwise.
RawTypeParameter* Class::LookupTypeParameter(const String& type_name) const {
  ASSERT(!type_name.IsNull());
  const Array& type_params = Array::Handle(type_parameters());
  if (!type_params.IsNull()) {
    intptr_t num_type_params = type_params.Length();
    String& type_param = String::Handle();
    for (intptr_t i = 0; i < num_type_params; i++) {
      type_param ^= type_params.At(i);
      if (type_param.Equals(type_name)) {
        return TypeParameter::New(i, type_name);
      }
    }
  }
  return TypeParameter::null();
}


void Class::CalculateFieldOffsets() const {
  Array& flds = Array::Handle(fields());
  const Class& super = Class::Handle(SuperClass());
  intptr_t offset = 0;
  intptr_t type_args_field_offset = kNoTypeArguments;
  if (super.IsNull()) {
    offset = sizeof(RawObject);
  } else {
    type_args_field_offset = super.type_arguments_instance_field_offset();
    offset = super.next_field_offset();
    ASSERT(offset > 0);
    // We should never call CalculateFieldOffsets for native wrapper
    // classes, assert this.
    ASSERT(num_native_fields() == 0);
    set_num_native_fields(super.num_native_fields());
  }
  // If the super class is parameterized, use the same type_arguments field.
  if (type_args_field_offset == kNoTypeArguments) {
    const Array& type_params = Array::Handle(type_parameters());
    if (!type_params.IsNull()) {
      ASSERT(type_params.Length() > 0);
      // The instance needs a type_arguments field.
      type_args_field_offset = offset;
      offset += kWordSize;
    }
  }
  set_type_arguments_instance_field_offset(type_args_field_offset);
  ASSERT(offset != 0);
  Field& field = Field::Handle();
  intptr_t len = flds.Length();
  for (intptr_t i = 0; i < len; i++) {
    field ^= flds.At(i);
    // Offset is computed only for instance fields.
    if (!field.is_static()) {
      ASSERT(field.Offset() == 0);
      field.SetOffset(offset);
      offset += kWordSize;
    }
  }
  set_instance_size(RoundedAllocationSize(offset));
  set_next_field_offset(offset);
}


void Class::Finalize() const {
  ASSERT(!is_finalized());
  // Prefinalized classes have a VM internal representation and no Dart fields.
  // Their instance size  is precomputed and field offsets are known.
  if (!is_prefinalized()) {
    // Compute offsets of instance fields and instance size.
    CalculateFieldOffsets();
  }
  set_is_finalized();
}


void Class::SetFields(const Array& value) const {
  ASSERT(!value.IsNull());
  Field& field = Field::Handle();
  intptr_t len = value.Length();
  for (intptr_t i = 0; i < len; i++) {
    field ^= value.At(i);
    field.set_owner(*this);
    // Only static const fields may contain the Object::sentinel value.
    ASSERT(!(field.is_static() && field.is_final()) ||
           (field.value() == Object::sentinel()));
  }
  // The value of static fields is already initialized to null.
  StorePointer(&raw_ptr()->fields_, value.raw());
}


template <class FakeInstance>
RawClass* Class::New(const String& name, const Script& script) {
  Class& class_class = Class::Handle(Object::class_class());
  Class& result = Class::Handle();
  {
    RawObject* raw = Object::Allocate(class_class,
                                      Class::InstanceSize(),
                                      Heap::kOld);
    NoGCScope no_gc;
    result ^= raw;
  }
  FakeInstance fake;
  ASSERT(fake.IsInstance());
  result.set_handle_vtable(fake.vtable());
  result.set_instance_size(FakeInstance::InstanceSize());
  result.set_next_field_offset(FakeInstance::InstanceSize());
  result.set_instance_kind(FakeInstance::kInstanceKind);
  result.set_name(name);
  result.set_script(script);
  result.raw_ptr()->is_const_ = false;
  result.raw_ptr()->is_interface_ = false;
  result.raw_ptr()->class_state_ = RawClass::kAllocated;
  result.raw_ptr()->type_arguments_instance_field_offset_ = kNoTypeArguments;
  result.raw_ptr()->num_constants_ = 0;
  result.raw_ptr()->num_native_fields_ = 0;
  result.InitEmptyFields();
  return result.raw();
}


RawClass* Class::New(const String& name, const Script& script) {
  Class& result = Class::Handle(New<Instance>(name, script));
  return result.raw();
}


RawClass* Class::NewInterface(const String& name, const Script& script) {
  Class& result = Class::Handle(New<Instance>(name, script));
  result.set_is_interface();
  return result.raw();
}


RawClass* Class::NewSignatureClass(const String& name,
                                   const Function& signature_function,
                                   const Script& script,
                                   intptr_t token_index) {
  ASSERT(!signature_function.IsNull());
  Array& type_parameters = Array::Handle();
  TypeArray& type_parameter_extends = TypeArray::Handle();
  if (!signature_function.is_static()) {
    const Class& function_class =
        Class::Handle(signature_function.owner());
    ASSERT(!function_class.IsNull());
    type_parameters = function_class.type_parameters();
    type_parameter_extends = function_class.type_parameter_extends();
  }
  Class& result = Class::Handle(New<Closure>(name, script));
  result.set_signature_function(signature_function);
  result.set_type_parameters(type_parameters);
  result.set_type_parameter_extends(type_parameter_extends);
  result.SetFields(Array::Handle(Array::Empty()));
  result.SetFunctions(Array::Handle(Array::Empty()));
  // Set super class to Object.
  result.set_super_type(Type::Handle(Type::ObjectType()));
  // Implements interface Function.
  const Type& function_interface = Type::Handle(Type::FunctionInterface());
  const Array& interfaces = Array::Handle(Array::New(1, Heap::kOld));
  interfaces.SetAt(0, function_interface);
  result.set_interfaces(interfaces);
  return result.raw();
}


RawClass* Class::GetClass(ObjectKind kind) {
  ObjectStore* object_store = Isolate::Current()->object_store();
  switch (kind) {
    case kUnhandledException:
      ASSERT(object_store->unhandled_exception_class() != Class::null());
      return object_store->unhandled_exception_class();
    case kSmi:
      ASSERT(object_store->smi_class() != Class::null());
      return object_store->smi_class();
    case kMint:
      ASSERT(object_store->mint_class() != Class::null());
      return object_store->mint_class();
    case kBigint:
      ASSERT(object_store->bigint_class() != Class::null());
      return object_store->bigint_class();
    case kDouble:
      ASSERT(object_store->double_class() != Class::null());
      return object_store->double_class();
    case kOneByteString:
      ASSERT(object_store->one_byte_string_class() != Class::null());
      return object_store->one_byte_string_class();
    case kTwoByteString:
      ASSERT(object_store->two_byte_string_class() != Class::null());
      return object_store->two_byte_string_class();
    case kFourByteString:
      ASSERT(object_store->four_byte_string_class() != Class::null());
      return object_store->four_byte_string_class();
    case kBool:
      ASSERT(object_store->bool_class() != Class::null());
      return object_store->bool_class();
    case kArray:
      ASSERT(object_store->array_class() != Class::null());
      return object_store->array_class();
    case kImmutableArray:
      ASSERT(object_store->immutable_array_class() != Class::null());
      return object_store->immutable_array_class();
    case kStacktrace:
      ASSERT(object_store->stacktrace_class() != Class::null());
      return object_store->stacktrace_class();
    case kJSRegExp:
      ASSERT(object_store->jsregexp_class() != Class::null());
      return object_store->jsregexp_class();
    case kClosure:
      return Class::New<Closure>();
    case kInstance:
      return Class::New<Instance>();
    default:
      UNREACHABLE();
  }
  return Class::null();
}


RawClass* Class::NewNativeWrapper(Library* library,
                                  const String& name,
                                  int field_count) {
  Class& cls = Class::Handle(library->LookupClass(name));
  if (cls.IsNull()) {
    cls = New<Instance>(name, Script::Handle());
    cls.SetFields(Array::Handle(Array::Empty()));
    cls.SetFunctions(Array::Handle(Array::Empty()));
    // Set super class to Object.
    cls.set_super_type(Type::Handle(Type::ObjectType()));
    // Compute instance size.
    intptr_t instance_size = (field_count * kWordSize) + sizeof(RawObject);
    cls.set_instance_size(RoundedAllocationSize(instance_size));
    cls.set_next_field_offset(instance_size);
    cls.set_num_native_fields(field_count);
    cls.set_is_finalized();
    library->AddClass(cls);
    return cls.raw();
  } else {
    return Class::null();
  }
}


void Class::set_name(const String& value) const {
  ASSERT(value.IsSymbol());
  StorePointer(&raw_ptr()->name_, value.raw());
}


void Class::set_script(const Script& value) const {
  StorePointer(&raw_ptr()->script_, value.raw());
}


void Class::set_is_interface() const {
  raw_ptr()->is_interface_ = true;
}


void Class::set_is_const() const {
  raw_ptr()->is_const_ = true;
}


void Class::set_is_finalized() const {
  ASSERT(!is_finalized());
  set_class_state(RawClass::kFinalized);
}


void Class::set_interfaces(const Array& value) const {
  // Verification and resolving of interfaces occurs in finalizer.
  ASSERT(!value.IsNull());
  StorePointer(&raw_ptr()->interfaces_, value.raw());
}


void Class::set_functions_cache(const Array& value) const {
  ASSERT(!value.IsNull());
  StorePointer(&raw_ptr()->functions_cache_, value.raw());
}


RawArray* Class::constants() const {
  return raw_ptr()->constants_;
}

void Class::set_constants(const Array& value) const {
  ASSERT(!value.IsNull());
  StorePointer(&raw_ptr()->constants_, value.raw());
}


intptr_t Class::num_constants() const {
  return raw_ptr()->num_constants_;
}


void Class::set_num_constants(intptr_t value) const {
  raw_ptr()->num_constants_ = value;
}


void Class::set_allocation_stub(const Code& value) const {
  ASSERT(!value.IsNull());
  ASSERT(raw_ptr()->allocation_stub_ == Code::null());
  StorePointer(&raw_ptr()->allocation_stub_, value.raw());
}


bool Class::IsVarClass() const {
  return raw() == Type::Handle(Type::VarType()).type_class();
}


bool Class::IsObjectClass() const {
  return raw() == Type::Handle(Type::ObjectType()).type_class();
}


// TODO(regis): We can probably merge this function with IsSubtypeOf, but since
// the spec is not definitive, we still follow it somewhat closely, to make it
// easier to implement spec changes.
bool Class::IsMoreSpecificThan(
    const TypeArguments& type_arguments,
    const Class& other,
    const TypeArguments& other_type_arguments) const {
  // Check for VarType.
  // The VarType on the lefthand side is replaced by the bottom type, which is
  // more specific than any type.
  // Any type is more specific than the VarType on the righthand side.
  if (IsVarClass() || other.IsVarClass()) {
    return true;
  }
  // Check for reflexivity.
  if (raw() == other.raw()) {
    if (!IsParameterized()) {
      return true;
    }
    // Since we do not truncate the type argument vector of a subclass (see
    // below), we only check a prefix of the proper length.
    const intptr_t len = NumTypeArguments();
    // Check for covariance.
    if (type_arguments.IsNull() ||
        other_type_arguments.IsNull() ||
        type_arguments.IsVarTypes(len) ||
        other_type_arguments.IsVarTypes(len)) {
      return true;
    }
    return type_arguments.IsMoreSpecificThan(other_type_arguments, len);
  }
  // Check for 'direct super type' in the case of an interface and check for
  // transitivity at the same time.
  if (other.is_interface()) {
    Array& interfaces = Array::Handle(this->interfaces());
    Type& interface = Type::Handle();
    Class& interface_class = Class::Handle();
    TypeArguments& interface_args = TypeArguments::Handle();
    for (intptr_t i = 0; i < interfaces.Length(); i++) {
      interface ^= interfaces.At(i);
      interface_class = interface.type_class();
      interface_args = interface.arguments();
      if (!interface_args.IsNull() && !interface_args.IsInstantiated()) {
        // This type implements an interface that is parameterized with generic
        // type(s), e.g. it implements Array<T>.
        // The uninstantiated type T must be instantiated using the type
        // parameters of this type before performing the type test.
        if (type_arguments.IsNull()) {
          // This type is raw, so the uninstantiated type arguments of the
          // interface cannot be instantiated and we must check against a raw
          // interface.
          interface_args = TypeArguments::null();
        } else {
          // The type arguments of this type that are referred to by the type
          // parameters of the interface are at the end of the type vector,
          // after the type arguments of the super type of this type.
          const intptr_t offset = NumTypeArguments() - NumTypeParameters();
          interface_args = interface_args.InstantiateFrom(type_arguments,
                                                          offset);
          // TODO(regis): Check the subtyping constraints if any, i.e. if
          // interface.type_parameter_extends() is not an array of VarType.
          // Should we pass the constraints to InstantiateFrom and it would
          // return null on failure?
        }
      }
      if (interface_class.IsMoreSpecificThan(interface_args,
                                             other,
                                             other_type_arguments)) {
        return true;
      }
    }
  }
  if (IsSignatureClass() && other.IsSignatureClass()) {
    const Function& fun = Function::Handle(signature_function());
    const Function& other_fun = Function::Handle(other.signature_function());
    // TODO(regis): We need to consider the type arguments.
    return fun.IsSubtypeOf(other_fun);
  }
  if (is_interface()) {
    // We already checked the case where 'other' is an interface. Now, 'this',
    // an interface, cannot be more specific than a class, except class Object,
    // because although Object is not considered an interface by the vm, it is
    // one. In other words, all classes implementing this interface also extend
    // class Object. An interface is also more specific than the VarType.
    return (other.IsVarClass() || other.IsObjectClass());
  }
  const Class& super_class = Class::Handle(SuperClass());
  if (super_class.IsNull()) {
    return false;
  }
  // Instead of truncating the type argument vector to the length of the super
  // type argument vector, we make sure that the code works with a vector that
  // is longer than necessary.
  return super_class.IsMoreSpecificThan(type_arguments,
                                        other,
                                        other_type_arguments);
}


bool Class::IsTopLevel() const {
  return String::Handle(Name()).Length() == 0;
}


bool Class::TestType(TypeTestKind test,
                     const TypeArguments& type_arguments,
                     const Class& other,
                     const TypeArguments& other_type_arguments) const {
  if (test == kIsAssignableTo) {
    // TODO(regis): We do not follow the guide that says that "a type T is
    // assignable to a type S if T is a subtype of S or S is a subtype of T",
    // since this would lead to heap pollution. We only apply that rule to
    // parameter types when checking assignability of function types.
    // Revisit if necessary.
    if (IsSignatureClass() && other.IsSignatureClass()) {
      const Function& src_fun = Function::Handle(signature_function());
      const Function& dst_fun = Function::Handle(other.signature_function());
      // TODO(regis): We need to consider the type arguments.
      return src_fun.IsAssignableTo(dst_fun);
    }
    // Continue with a subtype test.
    test = kIsSubtypeOf;
  }
  ASSERT(test == kIsSubtypeOf);

  // Check for "more specific" relation.
  if (IsMoreSpecificThan(type_arguments, other, other_type_arguments)) {
    return true;
  }
  // TODO(regis): Merge IsMoreSpecificThan here after type checks for
  // function types are finalized and implemented.
  // For now, keep the assert below.

  // The optionality and dubious bliss rules described in the guide have
  // already been checked in IsMoreSpecificThan call above.
  if (raw() != other.raw()) {
    return false;
  }
  ASSERT(IsParameterized());  // Otherwise IsMoreSpecificThan would be true.
  return false;
}


RawFunction* Class::LookupDynamicFunction(const String& name) const {
  Function& function = Function::Handle(LookupFunction(name));
  if (function.IsNull() || function.is_static()) {
    return Function::null();
  }
  switch (function.kind()) {
    case RawFunction::kFunction:
    case RawFunction::kGetterFunction:
    case RawFunction::kSetterFunction:
    case RawFunction::kImplicitGetter:
    case RawFunction::kImplicitSetter:
      return function.raw();
    case RawFunction::kConstructor:
    case RawFunction::kConstImplicitGetter:
    case RawFunction::kAbstract:
      return Function::null();
    default:
      UNREACHABLE();
      return Function::null();
  }
}


RawFunction* Class::LookupStaticFunction(const String& name) const {
  Function& function = Function::Handle(LookupFunction(name));
  if (function.IsNull() || !function.is_static()) {
    return Function::null();
  }
  switch (function.kind()) {
    case RawFunction::kFunction:
    case RawFunction::kGetterFunction:
    case RawFunction::kSetterFunction:
    case RawFunction::kImplicitGetter:
    case RawFunction::kImplicitSetter:
    case RawFunction::kConstImplicitGetter:
      return function.raw();
    case RawFunction::kConstructor:
      return Function::null();
    default:
      UNREACHABLE();
      return Function::null();
  }
}


RawFunction* Class::LookupConstructor(const String& name) const {
  Function& function = Function::Handle(LookupFunction(name));
  if (function.IsNull() || !function.IsConstructor()) {
    return Function::null();
  }
  ASSERT(!function.is_static());
  return function.raw();
}


RawFunction* Class::LookupFactory(const String& name) const {
  Function& function = Function::Handle(LookupFunction(name));
  if (function.IsNull() || !function.IsFactory()) {
    return Function::null();
  }
  ASSERT(function.is_static());
  return function.raw();
}


static bool MatchesPrivateName(const String& name, const String& private_name) {
  intptr_t name_len = name.Length();
  intptr_t private_len = private_name.Length();
  // The private_name must at least have room for the separator and one key
  // character.
  if ((name_len < (private_len + 2)) || (name_len == 0) || (private_len == 0)) {
    return false;
  }

  // Check for the private key separator.
  if (name.CharAt(private_len) != Scanner::kPrivateKeySeparator) {
    return false;
  }

  for (intptr_t i = 0; i < private_len; i++) {
    if (name.CharAt(i) != private_name.CharAt(i)) {
      return false;
    }
  }
  return true;
}


RawFunction* Class::LookupFunction(const String& name) const {
  Array& funcs = Array::Handle(functions());
  Function& function = Function::Handle();
  String& function_name = String::Handle();
  intptr_t len = funcs.Length();
  for (intptr_t i = 0; i < len; i++) {
    function ^= funcs.At(i);
    function_name ^= function.name();
    if (function_name.Equals(name) || MatchesPrivateName(function_name, name)) {
      return function.raw();
    }
  }

  // No function found.
  return Function::null();
}


RawField* Class::LookupInstanceField(const String& name) const {
  ASSERT(is_finalized());
  const Field& field = Field::Handle(LookupField(name));
  if (!field.IsNull()) {
    if (field.is_static()) {
      // Name matches but it is not of the correct kind, return NULL.
      return Field::null();
    }
    return field.raw();
  }
  // No field found.
  return Field::null();
}


RawField* Class::LookupStaticField(const String& name) const {
  ASSERT(is_finalized());
  const Field& field = Field::Handle(LookupField(name));
  if (!field.IsNull()) {
    if (!field.is_static()) {
      // Name matches but it is not of the correct kind, return NULL.
      return Field::null();
    }
    return field.raw();
  }
  // No field found.
  return Field::null();
}


RawField* Class::LookupField(const String& name) const {
  const Array& flds = Array::Handle(fields());
  Field& field = Field::Handle();
  String& field_name = String::Handle();
  intptr_t len = flds.Length();
  for (intptr_t i = 0; i < len; i++) {
    field ^= flds.At(i);
    field_name ^= field.name();
    if (field_name.Equals(name) || MatchesPrivateName(field_name, name)) {
      return field.raw();
    }
  }
  // No field found.
  return Field::null();
}


RawLibraryPrefix* Class::LookupLibraryPrefix(const String& name) const {
  LibraryPrefix& lib_prefix = LibraryPrefix::Handle();
  const Library& lib = Library::Handle(library());
  Object& obj = Object::Handle(lib.LookupLocalObject(name));
  if (!obj.IsNull()) {
    if (obj.IsLibraryPrefix()) {
      lib_prefix ^= obj.raw();
    }
  }
  return lib_prefix.raw();
}


const char* Class::ToCString() const {
  const char* format = is_interface()
      ? "%s Interface: %s" : "%s Class: %s";
  const Library& lib = Library::Handle(library());
  const char* library_name = lib.IsNull() ? "" : lib.ToCString();
  const char* class_name = String::Handle(Name()).ToCString();
  intptr_t len = OS::SNPrint(NULL, 0, format, library_name, class_name) + 1;
  char* chars = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(len));
  OS::SNPrint(chars, len, format, library_name, class_name);
  return chars;
}


bool Type::IsResolved() const {
  // Type is an abstract class.
  UNREACHABLE();
  return false;
}


bool Type::HasResolvedTypeClass() const {
  // Type is an abstract class.
  UNREACHABLE();
  return false;
}


RawClass* Type::type_class() const {
  // Type is an abstract class.
  UNREACHABLE();
  return Class::null();
}


RawString* Type::unresolved_type_class() const {
  // Type is an abstract class.
  UNREACHABLE();
  return String::null();
}


RawTypeArguments* Type::arguments() const  {
  // Type is an abstract class.
  UNREACHABLE();
  return TypeArguments::null();
}


bool Type::IsInstantiated() const {
  // Type is an abstract class.
  UNREACHABLE();
  return false;
}


bool Type::IsFinalized() const {
  // Type is an abstract class.
  UNREACHABLE();
  return false;
}


RawType* Type::InstantiateFrom(
    const TypeArguments& instantiator_type_arguments,
    intptr_t offset) const {
  // Type is an abstract class.
  UNREACHABLE();
  return Type::null();
}


RawString* Type::Name() const {
  const TypeArguments& args = TypeArguments::Handle(arguments());
  const intptr_t num_args = args.IsNull() ? 0 : args.Length();
  String& class_name = String::Handle();
  intptr_t first_type_param_index;
  intptr_t num_type_params;  // Number of type parameters to print.
  if (HasResolvedTypeClass()) {
    const Class& cls = Class::Handle(type_class());
    class_name = cls.Name();
    num_type_params = cls.NumTypeParameters();  // Do not print the full vector.
    if (num_type_params > num_args) {
      ASSERT(!IsFinalized());
      // We fill up with "var".
      first_type_param_index = 0;
    } else {
      first_type_param_index = num_args - num_type_params;
    }
  } else {
    class_name = unresolved_type_class();
    num_type_params = num_args;
    first_type_param_index = 0;
  }
  String& type_name = String::Handle();
  if (num_type_params == 0) {
    type_name = class_name.raw();
  } else {
    const intptr_t num_strings = 2*num_type_params + 2;  // "C""<""T"", ""T"">".
    const Array& strings = Array::Handle(Array::New(num_strings));
    intptr_t s = 0;
    strings.SetAt(s++, class_name);
    strings.SetAt(s++, String::Handle(String::NewSymbol("<")));
    const String& kCommaSpace = String::Handle(String::NewSymbol(", "));
    Type& type = Type::Handle();
    for (intptr_t i = 0; i < num_type_params; i++) {
      if (first_type_param_index + i >= num_args) {
        type = VarType();
      } else {
        type = args.TypeAt(first_type_param_index + i);
      }
      type_name = type.Name();
      strings.SetAt(s++, type_name);
      if (i < num_type_params - 1) {
        strings.SetAt(s++, kCommaSpace);
      }
    }
    strings.SetAt(s++, String::Handle(String::NewSymbol(">")));
    ASSERT(s == num_strings);
    type_name = String::ConcatAll(strings);
  }
  // The name is only used for naming function types and for debugging purposes.
  // Unless profiling data shows otherwise, it is not worth caching the name in
  // the type.
  return String::NewSymbol(type_name);
}


intptr_t Type::Index() const {
  // Type is an abstract class.
  UNREACHABLE();
  return -1;
}


RawString* Type::ClassName() const {
  if (HasResolvedTypeClass()) {
    return Class::Handle(type_class()).Name();
  } else {
    return unresolved_type_class();
  }
}


bool Type::IsNullType() const {
  return raw() == Isolate::Current()->object_store()->null_type();
}


bool Type::IsVarType() const {
  return raw() == Isolate::Current()->object_store()->var_type();
}


bool Type::IsVoidType() const {
  return raw() == Isolate::Current()->object_store()->void_type();
}


bool Type::IsMoreSpecificThan(const Type& other) const {
  ASSERT(IsFinalized());
  ASSERT(other.IsFinalized());
  // Type parameters cannot be handled by Class::IsMoreSpecificThan().
  if (IsTypeParameter() || other.IsTypeParameter()) {
    return IsTypeParameter() && other.IsTypeParameter() &&
        (Index() == other.Index());
  }
  const Class& cls = Class::Handle(type_class());
  return cls.IsMoreSpecificThan(TypeArguments::Handle(arguments()),
                                Class::Handle(other.type_class()),
                                TypeArguments::Handle(other.arguments()));
}


bool Type::Test(TypeTestKind test, const Type& other) const {
  ASSERT(IsFinalized());
  ASSERT(other.IsFinalized());
  // Type parameters cannot be handled by Class::TestType().
  if (IsTypeParameter() || other.IsTypeParameter()) {
    return IsTypeParameter() && other.IsTypeParameter() &&
        (Index() == other.Index());
  }
  const Class& cls = Class::Handle(type_class());
  if (test == kIsSubtypeOf) {
    return cls.IsSubtypeOf(TypeArguments::Handle(arguments()),
                            Class::Handle(other.type_class()),
                            TypeArguments::Handle(other.arguments()));
  } else {
    ASSERT(test == kIsAssignableTo);
    return cls.IsAssignableTo(TypeArguments::Handle(arguments()),
                              Class::Handle(other.type_class()),
                              TypeArguments::Handle(other.arguments()));
  }
}


RawType* Type::NullType() {
  return Isolate::Current()->object_store()->null_type();
}


RawType* Type::VarType() {
  return Isolate::Current()->object_store()->var_type();
}


RawType* Type::VoidType() {
  return Isolate::Current()->object_store()->void_type();
}


RawType* Type::ObjectType() {
  return Isolate::Current()->object_store()->object_type();
}


RawType* Type::BoolInterface() {
  return Isolate::Current()->object_store()->bool_interface();
}


RawType* Type::FunctionInterface() {
  return Isolate::Current()->object_store()->function_interface();
}


RawType* Type::NewRawType(const Class& type_class) {
  const TypeArguments& type_arguments =
      TypeArguments::Handle(type_class.type_parameter_extends());
  return NewParameterizedType(Object::Handle(type_class.raw()), type_arguments);
}


RawType* Type::NewNonParameterizedType(const Class& type_class) {
  ASSERT(type_class.NumTypeArguments() == 0);
  const TypeArguments& no_type_arguments = TypeArguments::Handle();
  ParameterizedType& type = ParameterizedType::Handle();
  type ^= ParameterizedType::New(
      Object::Handle(type_class.raw()), no_type_arguments);
  type.set_is_finalized();
  return type.raw();
}


RawType* Type::NewParameterizedType(const Object& clazz,
                                    const TypeArguments& arguments) {
  return ParameterizedType::New(clazz, arguments);
}


RawType* Type::NewTypeParameter(intptr_t index, const String& name) {
  return TypeParameter::New(index, name);
}


RawType* Type::NewInstantiatedType(
    const Type& uninstantiated_type,
    const TypeArguments& instantiator_type_arguments) {
  return InstantiatedType::New(uninstantiated_type,
                               instantiator_type_arguments);
}


const char* Type::ToCString() const {
  // Type is an abstract class.
  UNREACHABLE();
  return "Type";
}


void ParameterizedType::set_is_finalized() const {
  ASSERT(!IsFinalized());
  set_type_state(RawParameterizedType::kFinalized);
}


void ParameterizedType::set_is_being_finalized() const {
  ASSERT(!IsFinalized() && !is_being_finalized());
  set_type_state(RawParameterizedType::kBeingFinalized);
}


bool ParameterizedType::IsResolved() const {
  if (!HasResolvedTypeClass()) {
    return false;
  }
  const TypeArguments& args = TypeArguments::Handle(arguments());
  return args.IsNull() || args.IsResolved();
}


bool ParameterizedType::HasResolvedTypeClass() const {
  const Object& type_class = Object::Handle(raw_ptr()->type_class_);
  return !type_class.IsNull() && type_class.IsClass();
}


RawClass* ParameterizedType::type_class() const {
  ASSERT(HasResolvedTypeClass());
  Class& type_class = Class::Handle();
  type_class ^= raw_ptr()->type_class_;
  return type_class.raw();
}


RawString* ParameterizedType::unresolved_type_class() const {
  ASSERT(!HasResolvedTypeClass());
  String& unresolved_type_class = String::Handle();
  unresolved_type_class ^= raw_ptr()->type_class_;
  return unresolved_type_class.raw();
}


RawTypeArguments* ParameterizedType::arguments() const {
  return raw_ptr()->arguments_;
}


bool ParameterizedType::IsInstantiated() const {
  const TypeArguments& args = TypeArguments::Handle(arguments());
  return args.IsNull() || args.IsInstantiated();
}


RawType* ParameterizedType::InstantiateFrom(
    const TypeArguments& instantiator_type_arguments,
    intptr_t offset) const {
  ASSERT(IsFinalized());
  ASSERT(!IsInstantiated());
  TypeArguments& type_arguments = TypeArguments::Handle();
  if (!instantiator_type_arguments.IsNull()) {
    type_arguments = arguments();
    type_arguments = type_arguments.InstantiateFrom(instantiator_type_arguments,
                                                    offset);
  }
  ParameterizedType& instantiated_type = ParameterizedType::Handle(
      ParameterizedType::New(Object::Handle(type_class()), type_arguments));
  instantiated_type.set_is_finalized();
  return instantiated_type.raw();
}


void ParameterizedType::set_type_class(const Object& value) const {
  ASSERT(!value.IsNull() && (value.IsClass() || value.IsString()));
  StorePointer(&raw_ptr()->type_class_, value.raw());
}


void ParameterizedType::set_arguments(const TypeArguments& value) const {
  StorePointer(&raw_ptr()->arguments_, value.raw());
}


RawParameterizedType* ParameterizedType::New() {
  const Class& parameterized_type_class =
      Class::Handle(Object::parameterized_type_class());
  RawObject* raw = Object::Allocate(parameterized_type_class,
                                    ParameterizedType::InstanceSize(),
                                    Heap::kOld);
  return reinterpret_cast<RawParameterizedType*>(raw);
}


RawParameterizedType* ParameterizedType::New(const Object& clazz,
                                             const TypeArguments& arguments) {
  const ParameterizedType& result =
      ParameterizedType::Handle(ParameterizedType::New());
  result.set_type_class(clazz);
  result.set_arguments(arguments);
  result.raw_ptr()->type_state_ = RawParameterizedType::kAllocated;
  return result.raw();
}


void ParameterizedType::set_type_state(int8_t state) const {
  ASSERT(state == RawParameterizedType::kAllocated ||
         state == RawParameterizedType::kBeingFinalized ||
         state == RawParameterizedType::kFinalized);
  raw_ptr()->type_state_ = state;
}


const char* ParameterizedType::ToCString() const {
  return "ParameterizedType";
}


void TypeParameter::set_index(intptr_t value) const {
  ASSERT(value >= 0);
  raw_ptr()->index_ = value;
}


void TypeParameter::set_name(const String& value) const {
  ASSERT(value.IsSymbol());
  StorePointer(&raw_ptr()->name_, value.raw());
}


RawType* TypeParameter::InstantiateFrom(
    const TypeArguments& instantiator_type_arguments,
    intptr_t offset) const {
  if (instantiator_type_arguments.IsNull()) {
    return VarType();
  }
  return instantiator_type_arguments.TypeAt(Index() + offset);
}


RawTypeParameter* TypeParameter::New() {
  const Class& type_parameter_class =
      Class::Handle(Object::type_parameter_class());
  RawObject* raw = Object::Allocate(type_parameter_class,
                                    TypeParameter::InstanceSize(),
                                    Heap::kOld);
  return reinterpret_cast<RawTypeParameter*>(raw);
}


RawTypeParameter* TypeParameter::New(intptr_t index, const String& name) {
  const TypeParameter& result = TypeParameter::Handle(TypeParameter::New());
  result.set_index(index);
  result.set_name(name);
  return result.raw();
}


const char* TypeParameter::ToCString() const {
  return "TypeParameter";
}


RawClass* InstantiatedType::type_class() const {
  return Type::Handle(uninstantiated_type()).type_class();
}


RawTypeArguments* InstantiatedType::arguments() const {
  return TypeArguments::NewInstantiatedTypeArguments(
      TypeArguments::Handle(Type::Handle(uninstantiated_type()).arguments()),
      TypeArguments::Handle(instantiator_type_arguments()));
}


void InstantiatedType::set_uninstantiated_type(const Type& value) const {
  StorePointer(&raw_ptr()->uninstantiated_type_, value.raw());
}


void InstantiatedType::set_instantiator_type_arguments(
    const TypeArguments& value) const {
  StorePointer(&raw_ptr()->instantiator_type_arguments_, value.raw());
}


RawInstantiatedType* InstantiatedType::New() {
  const Class& instantiated_type_class =
      Class::Handle(Object::instantiated_type_class());
  RawObject* raw = Object::Allocate(instantiated_type_class,
                                    InstantiatedType::InstanceSize(),
                                    Heap::kOld);
  return reinterpret_cast<RawInstantiatedType*>(raw);
}


RawInstantiatedType* InstantiatedType::New(
    const Type& uninstantiated_type,
    const TypeArguments& instantiator_type_arguments) {
  const InstantiatedType& result =
      InstantiatedType::Handle(InstantiatedType::New());
  result.set_uninstantiated_type(uninstantiated_type);
  result.set_instantiator_type_arguments(instantiator_type_arguments);
  return result.raw();
}


const char* InstantiatedType::ToCString() const {
  return "InstantiatedType";
}


intptr_t TypeArguments::Length() const  {
  // TypeArguments is an abstract class.
  UNREACHABLE();
  return -1;
}


RawType* TypeArguments::TypeAt(intptr_t index) const {
  // TypeArguments is an abstract class.
  UNREACHABLE();
  return Type::null();
}


void TypeArguments::SetTypeAt(intptr_t index, const Type& value) const {
  // TypeArguments is an abstract class.
  UNREACHABLE();
}


bool TypeArguments::IsResolved() const {
  // TypeArguments is an abstract class.
  UNREACHABLE();
  return false;
}


bool TypeArguments::IsInstantiated() const {
  // TypeArguments is an abstract class.
  UNREACHABLE();
  return false;
}


bool TypeArguments::IsUninstantiatedIdentity() const {
  // TypeArguments is an abstract class.
  UNREACHABLE();
  return false;
}


RawTypeArguments* TypeArguments::InstantiateFrom(
    const TypeArguments& instantiator_type_arguments,
    intptr_t offset) const {
  // TypeArguments is an abstract class.
  UNREACHABLE();
  return TypeArguments::null();
}


bool TypeArguments::IsVarTypes(intptr_t len) const {
  ASSERT(Length() >= len);
  Type& type = Type::Handle();
  Class& type_class = Class::Handle();
  for (intptr_t i = 0; i < len; i++) {
    type = TypeAt(i);
    ASSERT(!type.IsNull());
    if (!type.HasResolvedTypeClass()) {
      ASSERT(type.IsTypeParameter());
      return false;
    }
    type_class = type.type_class();
    if (!type_class.IsVarClass()) {
      return false;
    }
  }
  return true;
}


bool TypeArguments::IsMoreSpecificThan(const TypeArguments& other,
                                       intptr_t len) const {
  ASSERT(Length() >= len);
  ASSERT(!other.IsNull());
  ASSERT(other.Length() >= len);
  Type& type = Type::Handle();
  Type& other_type = Type::Handle();
  for (intptr_t i = 0; i < len; i++) {
    type = TypeAt(i);
    ASSERT(!type.IsNull());
    other_type = other.TypeAt(i);
    ASSERT(!other_type.IsNull());
    if (!type.IsMoreSpecificThan(other_type)) {
      return false;
    }
  }
  return true;
}


RawTypeArguments* TypeArguments::NewTypeArray(intptr_t len) {
  return TypeArray::New(len);
}


RawTypeArguments* TypeArguments::NewInstantiatedTypeArguments(
    const TypeArguments& uninstantiated_type_arguments,
    const TypeArguments& instantiator_type_arguments) {
  return InstantiatedTypeArguments::New(uninstantiated_type_arguments,
                                        instantiator_type_arguments);
}


const char* TypeArguments::ToCString() const {
  // TypeArguments is an abstract class.
  UNREACHABLE();
  return "TypeArguments";
}


intptr_t TypeArray::Length() const {
  ASSERT(!IsNull());
  return Smi::Value(raw_ptr()->length_);
}


RawType* TypeArray::TypeAt(intptr_t index) const {
  return *TypeAddr(index);
}


void TypeArray::SetTypeAt(intptr_t index, const Type& value) const {
  // TODO(iposva): Add storing NoGCScope.
  *TypeAddr(index) = value.raw();
}


bool TypeArray::IsResolved() const {
  Type& type = Type::Handle();
  intptr_t num_types = Length();
  for (intptr_t i = 0; i < num_types; i++) {
    type = TypeAt(i);
    if (!type.IsResolved()) {
      return false;
    }
  }
  return true;
}


bool TypeArray::IsInstantiated() const {
  Type& type = Type::Handle();
  intptr_t num_types = Length();
  for (intptr_t i = 0; i < num_types; i++) {
    type = TypeAt(i);
    if (!type.IsInstantiated()) {
      return false;
    }
  }
  return true;
}


bool TypeArray::IsUninstantiatedIdentity() const {
  ASSERT(!IsInstantiated());
  Type& type = Type::Handle();
  intptr_t num_types = Length();
  for (intptr_t i = 0; i < num_types; i++) {
    type = TypeAt(i);
    if (!type.IsTypeParameter() || (type.Index() != i)) {
      return false;
    }
  }
  return true;
}


RawTypeArguments* TypeArray::InstantiateFrom(
    const TypeArguments& instantiator_type_arguments,
    intptr_t offset) const {
  ASSERT(!IsInstantiated());
  if (instantiator_type_arguments.IsNull()) {
    return TypeArguments::null();
  }
  if ((offset == 0) &&
      !instantiator_type_arguments.IsNull() &&
      IsUninstantiatedIdentity() &&
      (instantiator_type_arguments.Length() == Length())) {
    return instantiator_type_arguments.raw();
  }
  const intptr_t num_types = Length();
  TypeArray& instantiated_array = TypeArray::Handle(TypeArray::New(num_types));
  Type& type = Type::Handle();
  for (intptr_t i = 0; i < num_types; i++) {
    type = TypeAt(i);
    if (!type.IsInstantiated()) {
      type = type.InstantiateFrom(instantiator_type_arguments, offset);
    }
    instantiated_array.SetTypeAt(i, type);
  }
  return instantiated_array.raw();
}


RawTypeArray* TypeArray::New(intptr_t len) {
  if ((len < 0) || (len > kMaxTypes)) {
    // TODO(iposva): Should we throw an illegal parameter exception?
    UNIMPLEMENTED();
    return null();
  }

  const Class& type_array_class = Class::Handle(Object::type_array_class());
  TypeArray& result = TypeArray::Handle();
  {
    RawObject* raw = Object::Allocate(type_array_class,
                                      TypeArray::InstanceSize(len),
                                      Heap::kOld);
    NoGCScope no_gc;
    result ^= raw;
    result.SetLength(len);
    for (intptr_t i = 0; i < len; i++) {
      *result.TypeAddr(i) = Type::null();
    }
  }
  return result.raw();
}


RawType** TypeArray::TypeAddr(intptr_t index) const {
  // TODO(iposva): Determine if we should throw an exception here.
  ASSERT((index >= 0) && (index < Length()));
  return &raw_ptr()->types_[index];
}


void TypeArray::SetLength(intptr_t value) {
  // This is only safe because we create a new Smi, which does not cause
  // heap allocation.
  raw_ptr()->length_ = Smi::New(value);
}


const char* TypeArray::ToCString() const {
  return "TypeArray";
}


intptr_t InstantiatedTypeArguments::Length() const {
  return TypeArguments::Handle(uninstantiated_type_arguments()).Length();
}


RawType* InstantiatedTypeArguments::TypeAt(intptr_t index) const {
  const Type& type = Type::Handle(
      TypeArguments::Handle(uninstantiated_type_arguments()).TypeAt(index));
  if (type.IsTypeParameter()) {
    TypeArguments& instantiator =
        TypeArguments::Handle(instantiator_type_arguments());
    return instantiator.TypeAt(type.Index());
  }
  if (!type.IsInstantiated()) {
    return InstantiatedType::New(
        type, TypeArguments::Handle(instantiator_type_arguments()));
  }
  return type.raw();
}


void InstantiatedTypeArguments::SetTypeAt(intptr_t index,
                                          const Type& value) const {
  // We only replace individual argument types during resolution at compile
  // time, when no type parameters are instantiated yet.
  UNREACHABLE();
}


void InstantiatedTypeArguments::set_uninstantiated_type_arguments(
    const TypeArguments& value) const {
  StorePointer(&raw_ptr()->uninstantiated_type_arguments_, value.raw());
}


void InstantiatedTypeArguments::set_instantiator_type_arguments(
    const TypeArguments& value) const {
  StorePointer(&raw_ptr()->instantiator_type_arguments_, value.raw());
}


RawInstantiatedTypeArguments* InstantiatedTypeArguments::New() {
  const Class& instantiated_type_arguments_class =
      Class::Handle(Object::instantiated_type_arguments_class());
  RawObject* raw = Object::Allocate(instantiated_type_arguments_class,
                                    InstantiatedTypeArguments::InstanceSize(),
                                    Heap::kOld);
  return reinterpret_cast<RawInstantiatedTypeArguments*>(raw);
}


RawInstantiatedTypeArguments* InstantiatedTypeArguments::New(
    const TypeArguments& uninstantiated_type_arguments,
    const TypeArguments& instantiator_type_arguments) {
  const InstantiatedTypeArguments& result =
      InstantiatedTypeArguments::Handle(InstantiatedTypeArguments::New());
  result.set_uninstantiated_type_arguments(uninstantiated_type_arguments);
  result.set_instantiator_type_arguments(instantiator_type_arguments);
  return result.raw();
}


const char* InstantiatedTypeArguments::ToCString() const {
  return "InstantiatedTypeArguments";
}


void Function::SetCode(const Code& value) const {
  StorePointer(&raw_ptr()->code_, value.raw());
  ASSERT(Function::Handle(value.function()).IsNull() ||
    (value.function() == this->raw()));
  value.set_function(*this);
}


void Function::set_unoptimized_code(const Code& value) const {
  StorePointer(&raw_ptr()->unoptimized_code_, value.raw());
}


void Function::set_context_scope(const ContextScope& value) const {
  StorePointer(&raw_ptr()->context_scope_, value.raw());
}


void Function::set_closure_allocation_stub(const Code& value) const {
  ASSERT(!value.IsNull());
  ASSERT(raw_ptr()->closure_allocation_stub_ == Code::null());
  StorePointer(&raw_ptr()->closure_allocation_stub_, value.raw());
}


void Function::set_implicit_closure_function(const Function& value) const {
  ASSERT(!value.IsNull());
  ASSERT(raw_ptr()->implicit_closure_function_ == Function::null());
  StorePointer(&raw_ptr()->implicit_closure_function_, value.raw());
}


void Function::set_parent_function(const Function& value) const {
  StorePointer(&raw_ptr()->parent_function_, value.raw());
}


void Function::set_signature_class(const Class& value) const {
  StorePointer(&raw_ptr()->signature_class_, value.raw());
}


bool Function::IsInFactoryScope() const {
  Function& outer_function = Function::Handle(raw());
  while (outer_function.IsLocalFunction()) {
    outer_function = outer_function.parent_function();
  }
  return outer_function.IsFactory();
}


bool Function::IsInStaticScope() const {
  Function& outer_function = Function::Handle(raw());
  while (outer_function.IsLocalFunction()) {
    outer_function = outer_function.parent_function();
  }
  return outer_function.is_static();
}


void Function::set_name(const String& value) const {
  ASSERT(value.IsSymbol());
  StorePointer(&raw_ptr()->name_, value.raw());
}


void Function::set_owner(const Class& value) const {
  ASSERT(!value.IsNull());
  StorePointer(&raw_ptr()->owner_, value.raw());
}


void Function::set_result_type(const Type& value) const {
  ASSERT(!value.IsNull());
  StorePointer(&raw_ptr()->result_type_, value.raw());
}


RawType* Function::ParameterTypeAt(intptr_t index) const {
  const Array& parameter_types = Array::Handle(raw_ptr()->parameter_types_);
  Type& parameter_type = Type::Handle();
  parameter_type ^= parameter_types.At(index);
  return parameter_type.raw();
}


void Function::SetParameterTypeAt(intptr_t index, const Type& value) const {
  ASSERT(!value.IsNull());
  const Array& parameter_types = Array::Handle(raw_ptr()->parameter_types_);
  parameter_types.SetAt(index, value);
}


void Function::set_parameter_types(const Array& value) const {
  StorePointer(&raw_ptr()->parameter_types_, value.raw());
}


RawString* Function::ParameterNameAt(intptr_t index) const {
  const Array& parameter_names = Array::Handle(raw_ptr()->parameter_names_);
  String& parameter_name = String::Handle();
  parameter_name ^= parameter_names.At(index);
  return parameter_name.raw();
}


void Function::SetParameterNameAt(intptr_t index, const String& value) const {
  ASSERT(!value.IsNull() && value.IsSymbol());
  const Array& parameter_names = Array::Handle(raw_ptr()->parameter_names_);
  parameter_names.SetAt(index, value);
}


void Function::set_parameter_names(const Array& value) const {
  StorePointer(&raw_ptr()->parameter_names_, value.raw());
}


void Function::set_kind(RawFunction::Kind value) const {
  raw_ptr()->kind_ = value;
}


void Function::set_is_static(bool is_static) const {
  raw_ptr()->is_static_ = is_static;
}


void Function::set_is_const(bool is_const) const {
  raw_ptr()->is_const_ = is_const;
}


void Function::set_token_index(intptr_t pos) const {
  ASSERT(pos >= 0);
  raw_ptr()->token_index_ = pos;
}


void Function::set_num_fixed_parameters(intptr_t n) const {
  ASSERT(n >= 0);
  raw_ptr()->num_fixed_parameters_ = n;
}


void Function::set_num_optional_parameters(intptr_t n) const {
  ASSERT(n >= 0);
  raw_ptr()->num_optional_parameters_ = n;
}


void Function::set_is_optimizable(bool value) const {
  raw_ptr()->is_optimizable_ = value;
}


intptr_t Function::NumberOfParameters() const {
  return num_fixed_parameters() + num_optional_parameters();
}


bool Function::AreValidArgumentCounts(int num_arguments,
                                      int num_named_arguments) const {
  if (num_arguments > NumberOfParameters()) {
    return false;  // Too many arguments.
  }
  const int num_positional_args = num_arguments - num_named_arguments;
  if (num_positional_args < num_fixed_parameters()) {
    return false;  // Too few arguments.
  }
  return true;
}


// TODO(regis): Return some sort of diagnosis information so that we
// can improve the error messages generated by the parser.
bool Function::AreValidArguments(int num_arguments,
                                 const Array& argument_names) const {
  const int num_named_arguments =
      argument_names.IsNull() ? 0 : argument_names.Length();
  if (!AreValidArgumentCounts(num_arguments, num_named_arguments)) {
    return false;
  }
  // Verify that all argument names are valid parameter names.
  String& argument_name = String::Handle();
  String& parameter_name = String::Handle();
  for (int i = 0; i < num_named_arguments; i++) {
    argument_name ^= argument_names.At(i);
    ASSERT(argument_name.IsSymbol());
    bool found = false;
    const int num_positional_args = num_arguments - num_named_arguments;
    const int num_parameters = NumberOfParameters();
    for (int j = num_positional_args; !found && (j < num_parameters); j++) {
      parameter_name ^= ParameterNameAt(j);
      ASSERT(argument_name.IsSymbol());
      if (argument_name.Equals(parameter_name)) {
        found = true;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}


// Helper allocating a C string buffer in the zone, printing the fully qualified
// name of a function in it, and replacing ':' by '_' to make sure the
// constructed name is a valid C++ identifier for debugging purpose.
// Set 'chars' to allocated buffer and return number of written characters.
static intptr_t ConstructFunctionFullyQualifiedCString(const Function& function,
                                                       char** chars,
                                                       intptr_t reserve_len) {
  const char* name = String::Handle(function.name()).ToCString();
  const char* function_format = (reserve_len == 0) ? "%s" : "%s_";
  reserve_len += OS::SNPrint(NULL, 0, function_format, name);
  const Function& parent = Function::Handle(function.parent_function());
  intptr_t written = 0;
  if (parent.IsNull()) {
    const Class& function_class = Class::Handle(function.owner());
    ASSERT(!function_class.IsNull());
    const char* class_name = String::Handle(function_class.Name()).ToCString();
    ASSERT(class_name != NULL);
    const Library& library = Library::Handle(function_class.library());
    ASSERT(!library.IsNull());
    const char* library_name = String::Handle(library.url()).ToCString();
    ASSERT(library_name != NULL);
    const char* lib_class_format =
        (library_name[0] == '\0') ? "%s%s_" : "%s_%s_";
    reserve_len +=
        OS::SNPrint(NULL, 0, lib_class_format, library_name, class_name);
    ASSERT(chars != NULL);
    *chars = reinterpret_cast<char*>(
        Isolate::Current()->current_zone()->Allocate(reserve_len + 1));
    written = OS::SNPrint(
        *chars, reserve_len, lib_class_format, library_name, class_name);
  } else {
    written = ConstructFunctionFullyQualifiedCString(parent,
                                                     chars,
                                                     reserve_len);
  }
  ASSERT(*chars != NULL);
  char* next = *chars + written;
  written += OS::SNPrint(next, reserve_len + 1, function_format, name);
  // Replace ":" with "_".
  while (true) {
    next = strchr(next, ':');
    if (next == NULL) break;
    *next = '_';
  }
  return written;
}


const char* Function::ToFullyQualifiedCString() const {
  char* chars = NULL;
  ConstructFunctionFullyQualifiedCString(*this, &chars, 0);
  return chars;
}


bool Function::HasCompatibleParametersWith(const Function& other) const {
  // The default values of optional parameters can differ.
  const intptr_t num_fixed_params = num_fixed_parameters();
  const intptr_t num_opt_params = num_optional_parameters();
  const intptr_t other_num_fixed_params = other.num_fixed_parameters();
  const intptr_t other_num_opt_params = other.num_optional_parameters();
  if ((num_fixed_params != other_num_fixed_params) ||
      (num_opt_params < other_num_opt_params)) {
    return false;
  }
  // Check that for each optional named parameter of the other function, there
  // exists an optional named parameter of this function with an identical name.
  // Note that SetParameterNameAt() guarantees that names are symbols, so we can
  // compare their raw pointers.
  const int num_params = num_fixed_params + num_opt_params;
  const int other_num_params = other_num_fixed_params + other_num_opt_params;
  bool found_param_name;
  String& other_param_name = String::Handle();
  for (intptr_t i = other_num_fixed_params; i < other_num_params; i++) {
    other_param_name = other.ParameterNameAt(i);
    found_param_name = false;
    for (intptr_t j = num_fixed_params; j < num_params; j++) {
      if (ParameterNameAt(j) == other_param_name.raw()) {
        found_param_name = true;
        break;
      }
    }
    if (!found_param_name) {
      return false;
    }
  }
  return true;
}


bool Function::TestType(TypeTestKind test, const Function& other) const {
  const intptr_t num_fixed_params = num_fixed_parameters();
  const intptr_t num_opt_params = num_optional_parameters();
  const intptr_t other_num_fixed_params = other.num_fixed_parameters();
  const intptr_t other_num_opt_params = other.num_optional_parameters();
  if ((num_fixed_params != other_num_fixed_params) ||
      ((test == Type::kIsSubtypeOf) &&
       (num_opt_params < other_num_opt_params))) {
    return false;
  }
  // TODO(regis): We currently ignore type parameters. We need to consider the
  // parameter type upper bound, if any.
  // Note that unless we use this code to check function overrides at compile
  // time, all parameter types should be instantiated and we can remove code
  // checking for type parameters.

  // Check the result type.
  const Type& other_res_type = Type::Handle(other.result_type());
  if (!other_res_type.IsTypeParameter() &&
      !other_res_type.IsVarType() &&
      !other_res_type.IsVoidType()) {
    const Type& res_type = Type::Handle(result_type());
    if (!res_type.IsTypeParameter() &&
        !res_type.IsVarType() &&
        (res_type.IsVoidType() || !res_type.IsSubtypeOf(other_res_type)) &&
        ((test == Type::kIsSubtypeOf) ||
         (!other_res_type.IsSubtypeOf(res_type)))) {
      return false;
    }
  }
  // Check the types of fixed parameters.
  Type& param_type = Type::Handle();
  Type& other_param_type = Type::Handle();
  for (intptr_t i = 0; i < num_fixed_params; i++) {
    param_type = ParameterTypeAt(i);
    if (param_type.IsTypeParameter() || param_type.IsVarType()) {
      continue;
    }
    other_param_type = other.ParameterTypeAt(i);
    if (other_param_type.IsTypeParameter() || other_param_type.IsVarType()) {
      continue;
    }
    // Subtyping and assignability rules are identical when applied to parameter
    // types.
    ASSERT((test == Type::kIsSubtypeOf) || (test == Type::kIsAssignableTo));
    if (!param_type.IsSubtypeOf(other_param_type) &&
        !other_param_type.IsSubtypeOf(param_type)) {
      return false;
    }
  }
  // Check the names and types of optional parameters.
  // First, check that for each optional named parameter of type T of the other
  // function type, there exists an optional named parameter of this function
  // type with an identical name and with a Type S that is a subtype or
  // supertype of T.
  // Note that SetParameterNameAt() guarantees that names are symbols, so we can
  // compare their raw pointers.
  const int num_params = num_fixed_params + num_opt_params;
  const int other_num_params = other_num_fixed_params + other_num_opt_params;
  bool is_subtype = true;
  bool found_param_name;
  String& other_param_name = String::Handle();
  for (intptr_t i = other_num_fixed_params; i < other_num_params; i++) {
    other_param_name = other.ParameterNameAt(i);
    found_param_name = false;
    for (intptr_t j = num_fixed_params; j < num_params; j++) {
      if (ParameterNameAt(j) == other_param_name.raw()) {
        found_param_name = true;
        param_type = ParameterTypeAt(j);
        if (param_type.IsTypeParameter() || param_type.IsVarType()) {
          break;
        }
        other_param_type = other.ParameterTypeAt(i);
        if (other_param_type.IsTypeParameter() ||
            other_param_type.IsVarType()) {
          break;
        }
        if (!param_type.IsSubtypeOf(other_param_type) &&
            !other_param_type.IsSubtypeOf(param_type)) {
          is_subtype = false;
        }
        break;
      }
    }
    if (!found_param_name) {
      is_subtype = false;
      break;
    }
  }
  // If this first checking step succeeds, return true, otherwise, this function
  // type is not a subtype of the other function type.
  if (is_subtype) {
    return true;
  }
  if (test == Type::kIsSubtypeOf) {
    return false;
  }
  // To verify that this function type is assignable to the other function type,
  // i.e whether the other function type is a subtype of this function type, we
  // repeat the checking step above after swapping the other function type with
  // this function type.
  ASSERT(test == Type::kIsAssignableTo);
  String& param_name = String::Handle();
  is_subtype = true;
  for (intptr_t i = num_fixed_params; i < num_params; i++) {
    param_name = ParameterNameAt(i);
    found_param_name = false;
    for (intptr_t j = other_num_fixed_params; j < other_num_params; j++) {
      if (other.ParameterNameAt(j) == param_name.raw()) {
        found_param_name = true;
        other_param_type = other.ParameterTypeAt(j);
        if (other_param_type.IsTypeParameter() ||
            other_param_type.IsVarType()) {
          break;
        }
        param_type = ParameterTypeAt(i);
        if (param_type.IsTypeParameter() || param_type.IsVarType()) {
          break;
        }
        if (!other_param_type.IsSubtypeOf(param_type) &&
            !param_type.IsSubtypeOf(other_param_type)) {
          return false;
        }
        break;
      }
    }
    if (!found_param_name) {
      return false;
    }
  }
  return true;
}


RawFunction* Function::New() {
  const Class& function_class = Class::Handle(Object::function_class());
  RawObject* raw = Object::Allocate(function_class,
                                    Function::InstanceSize(),
                                    Heap::kOld);
  return reinterpret_cast<RawFunction*>(raw);
}


RawFunction* Function::New(const String& name,
                           RawFunction::Kind kind,
                           bool is_static,
                           bool is_const,
                           intptr_t token_index) {
  const Function& result = Function::Handle(Function::New());
  result.set_parameter_types(Array::Handle(Array::Empty()));
  result.set_parameter_names(Array::Handle(Array::Empty()));
  result.set_name(name);
  result.set_kind(kind);
  result.set_is_static(is_static);
  result.set_is_const(is_const);
  result.set_token_index(token_index);
  result.set_num_fixed_parameters(0);
  result.set_num_optional_parameters(0);
  result.set_invocation_counter(0);
  result.set_deoptimization_counter(0);
  result.set_is_optimizable(true);
  return result.raw();
}


RawFunction* Function::NewClosureFunction(const String& name,
                                          const Function& parent,
                                          intptr_t token_index) {
  ASSERT(!parent.IsNull());
  const Class& parent_class = Class::Handle(parent.owner());
  ASSERT(!parent_class.IsNull());
  const Function& result = Function::Handle(
      Function::New(name,
                    RawFunction::kClosureFunction,
                    /* is_static = */ true,
                    /* is_const = */ false,
                    token_index));
  result.set_parent_function(parent);
  result.set_owner(parent_class);
  return result.raw();
}


RawFunction* Function::ImplicitClosureFunction() const {
  // Return the existing implicit closure function if any.
  if (raw_ptr()->implicit_closure_function_ != Function::null()) {
    return raw_ptr()->implicit_closure_function_;
  }
  ASSERT(!IsSignatureFunction() && !IsClosureFunction());
  // Create closure function.
  const String& closure_name = String::Handle(name());
  const Function& closure_function = Function::Handle(
      NewClosureFunction(closure_name, *this, token_index()));

  // Set closure function's context scope.
  ContextScope& context_scope = ContextScope::Handle();
  if (is_static()) {
    context_scope ^= ContextScope::New(0);
  } else {
    context_scope ^= LocalScope::CreateImplicitClosureScope(*this);
  }
  closure_function.set_context_scope(context_scope);

  // Set closure function's result type to this result type.
  closure_function.set_result_type(Type::Handle(result_type()));

  // Set closure function's formal parameters to this formal parameters,
  // removing the receiver if this is an instance method.
  const int has_receiver = is_static() ? 0 : 1;
  const int num_fixed_params = num_fixed_parameters() - has_receiver;
  const int num_optional_params = num_optional_parameters();
  const int num_params = num_fixed_params + num_optional_params;
  closure_function.set_num_fixed_parameters(num_fixed_params);
  closure_function.set_num_optional_parameters(num_optional_params);
  closure_function.set_parameter_types(Array::Handle(Array::New(num_params,
                                                                Heap::kOld)));
  closure_function.set_parameter_names(Array::Handle(Array::New(num_params,
                                                                Heap::kOld)));
  Type& param_type = Type::Handle();
  String& param_name = String::Handle();
  for (int i = 0; i < num_params; i++) {
    param_type = ParameterTypeAt(i + has_receiver);
    closure_function.SetParameterTypeAt(i, param_type);
    param_name = ParameterNameAt(i + has_receiver);
    closure_function.SetParameterNameAt(i, param_name);
  }

  // Lookup or create a new function type for the closure function in the
  // library of the owner class.
  const Class& owner_class = Class::Handle(owner());
  ASSERT(!owner_class.IsNull() && (owner() == closure_function.owner()));
  const Library& library = Library::Handle(owner_class.library());
  ASSERT(!library.IsNull());
  const String& signature = String::Handle(closure_function.Signature());
  Class& signature_class = Class::ZoneHandle(library.LookupClass(signature));
  if (signature_class.IsNull()) {
    const Script& script = Script::Handle(owner_class.script());
    signature_class = Class::NewSignatureClass(signature,
                                               closure_function,
                                               script,
                                               token_index());
    library.AddClass(signature_class);
  }
  ASSERT(!Function::Handle(signature_class.signature_function()).IsNull());
  ASSERT(Class::Handle(closure_function.signature_class()).IsNull());
  closure_function.set_signature_class(signature_class);
  set_implicit_closure_function(closure_function);
  return closure_function.raw();
}


template<typename T>
static RawArray* NewArray(const GrowableArray<T*>& objs) {
  Array& a = Array::Handle(Array::New(objs.length(), Heap::kOld));
  for (int i = 0; i < objs.length(); i++) {
    a.SetAt(i, *objs[i]);
  }
  return a.raw();
}


// Build a string of the form '<T>(A, [b: B, c: C]) => R)' representing the
// signature of the given function.
RawString* Function::Signature() const {
  GrowableArray<const String*> pieces;
  const String& kSpaceExtendsSpace =
      String::Handle(String::NewSymbol(" extends "));
  const String& kCommaSpace = String::Handle(String::NewSymbol(", "));
  const String& kColonSpace = String::Handle(String::NewSymbol(": "));
  const String& kLAngleBracket = String::Handle(String::NewSymbol("<"));
  const String& kRAngleBracket = String::Handle(String::NewSymbol(">"));
  const String& kLParen = String::Handle(String::NewSymbol("("));
  const String& kRParen = String::Handle(String::NewSymbol(") => "));
  const String& kLBracket = String::Handle(String::NewSymbol("["));
  const String& kRBracket = String::Handle(String::NewSymbol("]"));
  if (!is_static()) {
    const Class& function_class = Class::Handle(owner());
    ASSERT(!function_class.IsNull());
    const Array& type_parameters = Array::Handle(
        function_class.type_parameters());
    if (!type_parameters.IsNull()) {
      intptr_t num_type_parameters = type_parameters.Length();
      pieces.Add(&kLAngleBracket);
      const TypeArray& type_parameter_extends = TypeArray::Handle(
          function_class.type_parameter_extends());
      Type& parameter_extends = Type::Handle();
      for (intptr_t i = 0; i < num_type_parameters; i++) {
        String& type_parameter = String::ZoneHandle();
        type_parameter ^= type_parameters.At(i);
        pieces.Add(&type_parameter);
        parameter_extends = type_parameter_extends.TypeAt(i);
        if (!parameter_extends.IsNull() && !parameter_extends.IsVarType()) {
          pieces.Add(&kSpaceExtendsSpace);
          pieces.Add(&String::ZoneHandle(parameter_extends.Name()));
        }
        if (i < num_type_parameters - 1) {
          pieces.Add(&kCommaSpace);
        }
      }
      pieces.Add(&kRAngleBracket);
    }
  }
  Type& param_type = Type::Handle();
  const intptr_t num_params = NumberOfParameters();
  const intptr_t num_fixed_params = num_fixed_parameters();
  const intptr_t num_opt_params = num_optional_parameters();
  ASSERT((num_fixed_params + num_opt_params) == num_params);
  pieces.Add(&kLParen);
  for (intptr_t i = 0; i < num_fixed_params; i++) {
    param_type = ParameterTypeAt(i);
    ASSERT(!param_type.IsNull());
    pieces.Add(&String::ZoneHandle(param_type.Name()));
    if (i != (num_params - 1)) {
      pieces.Add(&kCommaSpace);
    }
  }
  if (num_opt_params > 0) {
    pieces.Add(&kLBracket);
    for (intptr_t i = num_fixed_params; i < num_params; i++) {
      pieces.Add(&String::ZoneHandle(ParameterNameAt(i)));
      pieces.Add(&kColonSpace);
      param_type = ParameterTypeAt(i);
      ASSERT(!param_type.IsNull());
      pieces.Add(&String::ZoneHandle(param_type.Name()));
      if (i != (num_params - 1)) {
        pieces.Add(&kCommaSpace);
      }
    }
    pieces.Add(&kRBracket);
  }
  pieces.Add(&kRParen);
  const Type& res_type = Type::Handle(result_type());
  pieces.Add(&String::Handle(res_type.Name()));
  const Array& strings = Array::Handle(NewArray<const String>(pieces));
  return String::NewSymbol(String::Handle(String::ConcatAll(strings)));
}


const char* Function::ToCString() const {
  const char* f0 = is_static() ? " static" : "";
  const char* f1 = NULL;
  const char* f2 = is_const() ? " const" : "";
  switch (kind()) {
    case RawFunction::kFunction:
    case RawFunction::kClosureFunction:
    case RawFunction::kGetterFunction:
    case RawFunction::kSetterFunction:
      f1 = "";
      break;
    case RawFunction::kSignatureFunction:
      f1 = " signature";
      break;
    case RawFunction::kAbstract:
      f1 = " abstract";
      break;
    case RawFunction::kConstructor:
      f1 = is_static() ? " factory" : " constructor";
      break;
    case RawFunction::kImplicitGetter:
      f1 = " getter";
      break;
    case RawFunction::kImplicitSetter:
      f1 = " setter";
      break;
    case RawFunction::kConstImplicitGetter:
      f1 = " const-getter";
      break;
    default:
      UNREACHABLE();
  }
  const char* kFormat = "Function '%s':%s%s%s.";
  const char* function_name = String::Handle(name()).ToCString();
  intptr_t len = OS::SNPrint(NULL, 0, kFormat, function_name, f0, f1, f2) + 1;
  char* chars = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(len));
  OS::SNPrint(chars, len, kFormat, function_name, f0, f1, f2);
  return chars;
}


RawString* Field::GetterName(const String& field_name) {
  String& str = String::Handle();
  str = String::New("get:");
  str = String::Concat(str, field_name);
  return String::NewSymbol(str);
}


RawString* Field::SetterName(const String& field_name) {
  String& str = String::Handle();
  str = String::New("set:");
  str = String::Concat(str, field_name);
  return String::NewSymbol(str);
}


void Field::set_name(const String& value) const {
  ASSERT(value.IsSymbol());
  StorePointer(&raw_ptr()->name_, value.raw());
}


RawInstance* Field::value() const {
  ASSERT(is_static());  // Valid only for static dart fields.
  return raw_ptr()->value_;
}


void Field::set_value(const Instance& value) const {
  ASSERT(is_static());  // Valid only for static dart fields.
  StorePointer(&raw_ptr()->value_, value.raw());
}


void Field::set_type(const Type& value) const {
  ASSERT(!value.IsNull());
  StorePointer(&raw_ptr()->type_, value.raw());
}


RawField* Field::New() {
  const Class& field_class = Class::Handle(Object::field_class());
  RawObject* raw = Object::Allocate(field_class,
                                    Field::InstanceSize(),
                                    Heap::kOld);
  return reinterpret_cast<RawField*>(raw);
}


RawField* Field::New(const String& name,
                     bool is_static,
                     bool is_final,
                     intptr_t token_index) {
  const Field& result = Field::Handle(Field::New());
  result.set_name(name);
  result.set_is_static(is_static);
  if (is_static) {
    result.set_value(Instance::Handle());
  } else {
    result.SetOffset(0);
  }
  result.set_is_final(is_final);
  result.set_token_index(token_index);
  result.set_has_initializer(false);
  return result.raw();
}


const char* Field::ToCString() const {
  const char* kF0 = is_static() ? " static" : "";
  const char* kF1 = is_final() ? " final" : "";
  const char* kFormat = "Field <%s.%s>:%s%s";
  const char* field_name = String::Handle(name()).ToCString();
  const Class& cls = Class::Handle(owner());
  const char* cls_name = String::Handle(cls.Name()).ToCString();
  intptr_t len =
      OS::SNPrint(NULL, 0, kFormat, cls_name, field_name, kF0, kF1) + 1;
  char* chars = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(len));
  OS::SNPrint(chars, len, kFormat, cls_name, field_name, kF0, kF1);
  return chars;
}


void TokenStream::SetLength(intptr_t value) const {
  raw_ptr()->length_ = Smi::New(value);
}


void TokenStream::SetTokenAt(intptr_t index,
                             Token::Kind kind,
                             const String& literal) {
  *(SmiAddr(index, RawTokenStream::kKindEntry)) = Smi::New(kind);
  StorePointer(EntryAddr(index, RawTokenStream::kLiteralEntry),
               reinterpret_cast<RawObject*>(literal.raw()));
}


RawTokenStream* TokenStream::New(intptr_t len) {
  const Class& token_stream_class = Class::Handle(Object::token_stream_class());
  TokenStream& result = TokenStream::Handle();
  {
    RawObject* raw = Object::Allocate(token_stream_class,
                                      TokenStream::InstanceSize(len),
                                      Heap::kOld);
    NoGCScope no_gc;
    result ^= raw;
    result.SetLength(len);
  }
  return result.raw();
}


RawTokenStream* TokenStream::New(const Scanner::GrowableTokenStream& tokens) {
  intptr_t len = tokens.length();

  TokenStream& result = TokenStream::Handle(New(len));
  // Copy the relevant data out of the scanner's token stream.
  for (intptr_t i = 0; i < len; i++) {
    Scanner::TokenDescriptor token = tokens[i];
    if (token.literal != NULL) {
      result.SetTokenAt(i, token.kind, *(token.literal));
    } else {
      result.SetTokenAt(i, token.kind, String::Handle());
    }
  }
  return result.raw();
}


const char* TokenStream::ToCString() const {
  return "TokenStream";
}


void Script::set_url(const String& value) const {
  StorePointer(&raw_ptr()->url_, value.raw());
}


void Script::set_source(const String& value) const {
  StorePointer(&raw_ptr()->source_, value.raw());
}


void Script::set_kind(RawScript::Kind value) const {
  raw_ptr()->kind_ = value;
}


void Script::set_tokens(const TokenStream& value) const {
  StorePointer(&raw_ptr()->tokens_, value.raw());
}


void Script::Tokenize(const String& private_key) const {
  const TokenStream& tkns = TokenStream::Handle(tokens());
  if (!tkns.IsNull()) {
    // Already tokenized.
    return;
  }

  // Get the source, scan and allocate the token stream.
  if (FLAG_compiler_stats) {
    CompilerStats::scanner_timer.Start();
  }
  const String& src = String::Handle(source());
  Scanner scanner(src, private_key);
  set_tokens(TokenStream::Handle(TokenStream::New(scanner.GetStream())));
  if (FLAG_compiler_stats) {
    CompilerStats::scanner_timer.Stop();
    CompilerStats::src_length += src.Length();
  }
}


void Script::GetTokenLocation(intptr_t token_index,
                              intptr_t* line,
                              intptr_t* column) const {
  const String& src = String::Handle(source());
  const String& dummy_key = String::Handle(String::New(""));
  Scanner scanner(src, dummy_key);
  scanner.ScanTo(token_index);
  *line = scanner.CurrentPosition().line;
  *column = scanner.CurrentPosition().column;
}


RawString* Script::GetLine(intptr_t line_number) const {
  const String& src = String::Handle(source());
  intptr_t current_line = 1;
  intptr_t line_start = -1;
  intptr_t last_char = -1;
  for (intptr_t ix = 0;
       (ix < src.Length()) && (current_line <= line_number);
       ix++) {
    if ((current_line == line_number) && (line_start < 0)) {
      line_start = ix;
    }
    if (src.CharAt(ix) == '\n') {
      current_line++;
    } else if (src.CharAt(ix) == '\r') {
      if ((ix + 1 != src.Length()) && (src.CharAt(ix + 1) != '\n')) {
        current_line++;
      }
    } else {
      last_char = ix;
    }
  }
  // Guarantee that returned string is never NULL.
  String& line = String::Handle(String::NewSymbol(""));
  if (line_start >= 0) {
    line = String::SubString(src, line_start, last_char - line_start + 1);
  }
  return line.raw();
}


RawString* Script::GetSnippet(intptr_t from_line,
                              intptr_t from_column,
                              intptr_t to_line,
                              intptr_t to_column) const {
  const String& src = String::Handle(source());
  intptr_t length = src.Length();
  intptr_t line = 1;
  intptr_t column = 1;
  intptr_t lookahead = 0;
  intptr_t snippet_start = -1;
  intptr_t snippet_end = -1;
  char c = src.CharAt(lookahead);
  while (lookahead != length) {
    if (snippet_start == -1) {
      if ((line == from_line) && (column == from_column)) {
        snippet_start = lookahead;
      }
    } else if ((line == to_line) && (column == to_column)) {
      snippet_end = lookahead;
      break;
    }
    if (c == '\n') {
      line++;
      column = 0;
    }
    column++;
    lookahead++;
    if (lookahead != length) {
      // Replace '\r' with '\n' and a sequence of '\r' '\n' with a single '\n'.
      if (src.CharAt(lookahead) == '\r') {
        c = '\n';
        if (lookahead + 1 != length && src.CharAt(lookahead) == '\n') {
          lookahead++;
        }
      } else {
        c = src.CharAt(lookahead);
      }
    }
  }
  String& snippet = String::Handle();
  if ((snippet_start != -1) && (snippet_end != -1)) {
    snippet =
        String::SubString(src, snippet_start, snippet_end - snippet_start);
  }
  return snippet.raw();
}


RawScript* Script::New() {
  const Class& script_class = Class::Handle(Object::script_class());
  RawObject* raw = Object::Allocate(script_class,
                                    Script::InstanceSize(),
                                    Heap::kOld);
  return reinterpret_cast<RawScript*>(raw);
}


RawScript* Script::New(const String& url,
                       const String& source,
                       RawScript::Kind kind) {
  const Script& result = Script::Handle(Script::New());
  result.set_url(String::Handle(String::NewSymbol(url)));
  result.set_source(source);
  result.set_kind(kind);
  return result.raw();
}


const char* Script::ToCString() const {
  return "Script";
}


ClassDictionaryIterator::ClassDictionaryIterator(const Library& library)
    : array_(Array::Handle(library.dictionary())),
      // Last element in array is a Smi.
      size_(Array::Handle(library.dictionary()).Length() - 1),
      next_ix_(0) {
  MoveToNextClass();
}


RawClass* ClassDictionaryIterator::GetNext() {
  ASSERT(HasNext());
  int ix = next_ix_++;
  MoveToNextClass();
  ASSERT(array_.At(ix) != Object::null());
  Class& cls = Class::Handle();
  cls ^= array_.At(ix);
  return cls.raw();
}


void ClassDictionaryIterator::MoveToNextClass() {
  Object& obj = Object::Handle(array_.At(next_ix_));
  while (!obj.IsClass() && HasNext()) {
    next_ix_++;
    obj = array_.At(next_ix_);
  }
}


void Library::SetName(const String& name) const {
  // Only set name once.
  ASSERT(raw_ptr()->name_ == raw_ptr()->url_);
  ASSERT(name.IsSymbol());
  StorePointer(&raw_ptr()->name_, name.raw());
}


void Library::GrowDictionary(const Array& dict, intptr_t dict_size) const {
  // TODO(iposva): Avoid exponential growth.
  intptr_t new_dict_size = dict_size * 2;
  const Array& new_dict =
      Array::Handle(Array::New(new_dict_size + 1, Heap::kOld));
  // Rehash all elements from the original dictionary
  // to the newly allocated array.
  Object& entry = Class::Handle();
  String& entry_name = String::Handle();
  Object& new_entry = Object::Handle();
  Class& cls = Class::Handle();
  Function& func = Function::Handle();
  Field& field = Field::Handle();
  LibraryPrefix& prefix = LibraryPrefix::Handle();
  for (intptr_t i = 0; i < dict_size; i++) {
    entry = dict.At(i);
    if (!entry.IsNull()) {
      if (entry.IsClass()) {
        cls ^= entry.raw();
        entry_name = cls.Name();
      } else if (entry.IsFunction()) {
        func ^= entry.raw();
        entry_name = func.name();
      } else if (entry.IsField()) {
        field ^= entry.raw();
        entry_name = field.name();
      } else if (entry.IsLibraryPrefix()) {
        prefix ^= entry.raw();
        entry_name = prefix.name();
      } else {
        UNREACHABLE();
      }
      intptr_t hash = entry_name.Hash();
      intptr_t index = hash % new_dict_size;
      new_entry = new_dict.At(index);
      while (!new_entry.IsNull()) {
        index = (index + 1) % new_dict_size;  // Move to next element.
        new_entry = new_dict.At(index);
      }
      new_dict.SetAt(index, entry);
    }
  }
  // Copy used count.
  new_entry = dict.At(dict_size);
  new_dict.SetAt(new_dict_size, new_entry);
  // Remember the new dictionary now.
  StorePointer(&raw_ptr()->dictionary_, new_dict.raw());
}


void Library::AddObject(const Object& obj, const String& name) const {
  ASSERT(obj.IsClass() ||
         obj.IsFunction() ||
         obj.IsField() ||
         obj.IsLibraryPrefix());
  ASSERT((LookupObject(name) == Object::null()) ||
         (obj.IsLibraryPrefix() &&
          (LookupLocalObject(name) == Object::null())));
  const Array& dict = Array::Handle(dictionary());
  intptr_t dict_size = dict.Length() - 1;
  intptr_t index = name.Hash() % dict_size;

  Object& entry = Object::Handle();
  entry = dict.At(index);
  // An empty spot will be found because we keep the hash set at most 75% full.
  while (!entry.IsNull()) {
    index = (index + 1) % dict_size;
    entry = dict.At(index);
  }

  // Insert the object at the empty slot.
  dict.SetAt(index, obj);
  Smi& used = Smi::Handle();
  used ^= dict.At(dict_size);
  intptr_t used_elements = used.Value() + 1;  // One more element added.
  used = Smi::New(used_elements);
  dict.SetAt(dict_size, used);  // Update used count.

  // Rehash if symbol_table is 75% full.
  if (used_elements > ((dict_size / 4) * 3)) {
    GrowDictionary(dict, dict_size);
  }
}


void Library::AddClass(const Class& cls) const {
  AddObject(cls, String::Handle(cls.Name()));
  // Link class to this library.
  cls.set_library(*this);
}


RawObject* Library::LookupLocalObject(const String& name) const {
  const Array& dict = Array::Handle(dictionary());
  intptr_t dict_size = dict.Length() - 1;
  intptr_t index = name.Hash() % dict_size;

  Object& entry = Object::Handle();
  Class& cls = Class::Handle();
  Function& func = Function::Handle();
  Field& field = Field::Handle();
  LibraryPrefix& library_prefix = LibraryPrefix::Handle();
  String& entry_name = String::Handle();
  entry = dict.At(index);
  // Search the entry in the hash set.
  while (!entry.IsNull()) {
    // TODO(hausner): find a better way to handle this polymorphism.
    // Either introduce a common base class for Class, Function, Field
    // and LibraryPrefix or make the name() function virtual in Object.
    if (entry.IsClass()) {
      cls ^= entry.raw();
      entry_name = cls.Name();
    } else if (entry.IsFunction()) {
      func ^= entry.raw();
      entry_name = func.name();
    } else if (entry.IsField()) {
      field ^= entry.raw();
      entry_name = field.name();
    } else if (entry.IsLibraryPrefix()) {
      library_prefix ^= entry.raw();
      entry_name = library_prefix.name();
    } else {
      UNREACHABLE();
    }
    if (entry_name.Equals(name)) {
      return entry.raw();
    }
    index = (index + 1) % dict_size;
    entry = dict.At(index);
  }
  return Class::null();
}


RawObject* Library::LookupObject(const String& name) const {
  Object& obj = Object::Handle(LookupLocalObject(name));
  if (!obj.IsNull()) {
    return obj.raw();
  }
  Library& import = Library::Handle();
  Array& imports = Array::Handle(this->imports());
  for (intptr_t i = 0; i < num_imports(); i++) {
    import ^= imports.At(i);
    obj = import.LookupLocalObject(name);
    if (!obj.IsNull()) {
      return obj.raw();
    }
  }
  return Object::null();
}


RawClass* Library::LookupClass(const String& name) const {
  Object& obj = Object::Handle(LookupObject(name));
  if (!obj.IsNull() && obj.IsClass()) {
    return Class::CheckedHandle(obj.raw()).raw();
  }
  return Class::null();
}


RawClass* Library::LookupLocalClass(const String& name) const {
  Object& obj = Object::Handle(LookupLocalObject(name));
  if (!obj.IsNull() && obj.IsClass()) {
    return Class::CheckedHandle(obj.raw()).raw();
  }
  return Class::null();
}


void Library::AddAnonymousClass(const Class& cls) const {
  intptr_t num_anonymous = this->raw_ptr()->num_anonymous_;
  Array& anon_array = Array::Handle(this->raw_ptr()->anonymous_classes_);
  if (num_anonymous == anon_array.Length()) {
    intptr_t new_len = (num_anonymous == 0) ? 4 : num_anonymous * 2;
    anon_array = Array::Grow(anon_array, new_len);
    StorePointer(&raw_ptr()->anonymous_classes_, anon_array.raw());
  }
  anon_array.SetAt(num_anonymous, cls);
  num_anonymous++;
  raw_ptr()->num_anonymous_ = num_anonymous;
}


RawLibrary* Library::LookupImport(const String& url) const {
  const Array& imports = Array::Handle(this->imports());
  intptr_t num_imports = this->num_imports();
  Library& lib = Library::Handle();
  String& import_url = String::Handle();
  for (int i = 0; i < num_imports; i++) {
    lib ^= imports.At(i);
    import_url = lib.url();
    if (url.Equals(import_url)) {
      return lib.raw();
    }
  }
  return Library::null();
}


void Library::AddImport(const Library& library) const {
  Array& imports = Array::Handle(this->imports());
  intptr_t capacity = imports.Length();
  if (num_imports() == capacity) {
    capacity = capacity + kImportsCapacityIncrement;
    imports = Array::Grow(imports, capacity);
    StorePointer(&raw_ptr()->imports_, imports.raw());
  }
  intptr_t index = num_imports();
  imports.SetAt(index, library);
  set_num_imports(index + 1);
}


void Library::InitClassDictionary() const {
  // The last element of the dictionary specifies the number of in use slots.
  // TODO(iposva): Find reasonable initial size.
  const int kInitialElementCount = 16;

  const Array& dictionary =
      Array::Handle(Array::New(kInitialElementCount + 1, Heap::kOld));
  dictionary.SetAt(kInitialElementCount, Smi::Handle(Smi::New(0)));
  StorePointer(&raw_ptr()->dictionary_, dictionary.raw());
}


void Library::InitImportList() const {
  const Array& imports =
      Array::Handle(Array::New(kInitialImportsCapacity, Heap::kOld));
  StorePointer(&raw_ptr()->imports_, imports.raw());
  raw_ptr()->num_imports_ = 0;
}


RawLibrary* Library::New() {
  const Class& library_class = Class::Handle(Object::library_class());
  RawObject* raw = Object::Allocate(library_class,
                                    Library::InstanceSize(),
                                    Heap::kOld);
  return reinterpret_cast<RawLibrary*>(raw);
}


RawLibrary* Library::NewLibraryHelper(const String& url,
                                      bool import_core_lib) {
  const Library& result = Library::Handle(Library::New());
  result.raw_ptr()->name_ = url.raw();
  result.raw_ptr()->url_ = url.raw();
  result.raw_ptr()->private_key_ = Scanner::AllocatePrivateKey(result);
  result.raw_ptr()->dictionary_ = Array::Empty();
  result.raw_ptr()->anonymous_classes_ = Array::Empty();
  result.raw_ptr()->num_anonymous_ = 0;
  result.raw_ptr()->imports_ = Array::Empty();
  result.raw_ptr()->next_registered_ = Library::null();
  result.set_native_entry_resolver(NULL);
  result.raw_ptr()->corelib_imported_ = true;
  result.raw_ptr()->loaded_ = false;
  result.InitClassDictionary();
  result.InitImportList();
  if (import_core_lib) {
    Library& core_lib = Library::Handle(Library::CoreLibrary());
    ASSERT(!core_lib.IsNull());
    result.AddImport(core_lib);
    if (FLAG_expose_core_impl) {
      // Make implementation corelib visible to Dart code.
      result.AddImport(Library::Handle(Library::CoreImplLibrary()));
    }
  }
  return result.raw();
}


RawLibrary* Library::New(const String& url) {
  return NewLibraryHelper(url, true);
}


void Library::InitCoreLibrary(Isolate* isolate) {
  const String& core_lib_url = String::Handle(String::NewSymbol("dart:core"));
  const Library& core_lib =
      Library::Handle(Library::NewLibraryHelper(core_lib_url, false));
  core_lib.Register();
  isolate->object_store()->set_core_library(core_lib);
  const String& core_impl_lib_url =
      String::Handle(String::NewSymbol("dart:coreimpl"));
  const Library& core_impl_lib =
      Library::Handle(Library::NewLibraryHelper(core_impl_lib_url, false));
  isolate->object_store()->set_core_impl_library(core_impl_lib);
  core_impl_lib.Register();
  core_lib.AddImport(core_impl_lib);
  core_impl_lib.AddImport(core_lib);
  isolate->object_store()->set_root_library(Library::Handle());
}


RawLibrary* Library::LookupLibrary(const String &url) {
  Library& lib = Library::Handle();
  String& lib_url = String::Handle();
  lib = Isolate::Current()->object_store()->registered_libraries();
  while (!lib.IsNull()) {
    lib_url = lib.url();
    if (lib_url.Equals(url)) {
      return lib.raw();
    }
    lib = lib.next_registered();
  }
  return Library::null();
}


bool Library::IsKeyUsed(intptr_t key) {
  intptr_t lib_key;
  Library& lib = Library::Handle();
  lib = Isolate::Current()->object_store()->registered_libraries();
  String& lib_url = String::Handle();
  while (!lib.IsNull()) {
    lib_url ^= lib.url();
    lib_key = lib_url.Hash();
    if (lib_key == key) {
      return true;
    }
    lib = lib.next_registered();
  }
  return false;
}


void Library::Register() const {
  ASSERT(Library::LookupLibrary(String::Handle(url())) == Library::null());
  raw_ptr()->next_registered_ =
      Isolate::Current()->object_store()->registered_libraries();
  Isolate::Current()->object_store()->set_registered_libraries(*this);
}


RawLibrary* Library::CoreLibrary() {
  return Isolate::Current()->object_store()->core_library();
}


RawLibrary* Library::CoreImplLibrary() {
  return Isolate::Current()->object_store()->core_impl_library();
}


const char* Library::ToCString() const {
  const char* kFormat = "Library:'%s'";
  const String& name = String::Handle(url());
  intptr_t len = OS::SNPrint(NULL, 0, kFormat, name.ToCString()) + 1;
  char* chars = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(len));
  OS::SNPrint(chars, len, kFormat, name.ToCString());
  return chars;
}


RawLibraryPrefix* LibraryPrefix::New() {
  const Class& library_prefix_class =
      Class::Handle(Object::library_prefix_class());
  RawObject* raw = Object::Allocate(library_prefix_class,
                                    LibraryPrefix::InstanceSize(),
                                    Heap::kOld);
  return reinterpret_cast<RawLibraryPrefix*>(raw);
}


RawLibraryPrefix* LibraryPrefix::New(const String& name, const Library& lib) {
  const LibraryPrefix& result = LibraryPrefix::Handle(LibraryPrefix::New());
  result.set_name(name);
  result.set_library(lib);
  return result.raw();
}


const char* LibraryPrefix::ToCString() const {
  const char* kFormat = "LibraryPrefix:'%s'";
  const String& prefix = String::Handle(name());
  intptr_t len = OS::SNPrint(NULL, 0, kFormat, prefix.ToCString()) + 1;
  char* chars = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(len));
  OS::SNPrint(chars, len, kFormat, prefix.ToCString());
  return chars;
}


void LibraryPrefix::set_name(const String& value) const {
  ASSERT(value.IsSymbol());
  StorePointer(&raw_ptr()->name_, value.raw());
}


void LibraryPrefix::set_library(const Library& value) const {
  StorePointer(&raw_ptr()->library_, value.raw());
}


void Library::CompileAll() {
  Library& lib = Library::Handle(
      Isolate::Current()->object_store()->registered_libraries());
  Class& cls = Class::Handle();
  while (!lib.IsNull()) {
    ClassDictionaryIterator it(lib);
    while (it.HasNext()) {
      cls ^= it.GetNext();
      if (!cls.is_interface()) {
        Compiler::CompileAllFunctions(cls);
      }
    }
    Array& anon_classes = Array::Handle(lib.raw_ptr()->anonymous_classes_);
    for (int i = 0; i < lib.raw_ptr()->num_anonymous_; i++) {
      cls ^= anon_classes.At(i);
      ASSERT(!cls.is_interface());
      Compiler::CompileAllFunctions(cls);
    }
    lib = lib.next_registered();
  }
}


RawInstructions* Instructions::New(intptr_t size) {
  const Class& instructions_class = Class::Handle(Object::instructions_class());
  Instructions& result = Instructions::Handle();
  {
    uword aligned_size = Instructions::InstanceSize(size);
    RawObject* raw = Object::Allocate(instructions_class,
                                      aligned_size,
                                      Heap::kExecutable);
    NoGCScope no_gc;
    result ^= raw;
    result.set_size(size);
  }
  return result.raw();
}


const char* Instructions::ToCString() const {
  return "Instructions";
}


intptr_t PcDescriptors::Length() const {
  return Smi::Value(raw_ptr()->length_);
}


void PcDescriptors::SetLength(intptr_t value) const {
  // This is only safe because we create a new Smi, which does not cause
  // heap allocation.
  raw_ptr()->length_ = Smi::New(value);
}


intptr_t PcDescriptors::PC(intptr_t index) const {
  return *(EntryAddr(index, kPcEntry));
}


void PcDescriptors::SetPC(intptr_t index, intptr_t value) const {
  *(EntryAddr(index, kPcEntry)) = value;
}


PcDescriptors::Kind PcDescriptors::DescriptorKind(intptr_t index) const {
  return static_cast<PcDescriptors::Kind>(*(EntryAddr(index, kKindEntry)));
}


void PcDescriptors::SetKind(intptr_t index, PcDescriptors::Kind value) const {
  *(EntryAddr(index, kKindEntry)) = value;
}


intptr_t PcDescriptors::NodeId(intptr_t index) const {
  return Smi::Value(*SmiAddr(index, kNodeIdEntry));
}


void PcDescriptors::SetNodeId(intptr_t index, intptr_t value) const {
  *SmiAddr(index, kNodeIdEntry) = Smi::New(value);
}


intptr_t PcDescriptors::TokenIndex(intptr_t index) const {
  return Smi::Value(*SmiAddr(index, kTokenIndexEntry));
}


void PcDescriptors::SetTokenIndex(intptr_t index, intptr_t value) const {
  *SmiAddr(index, kTokenIndexEntry) = Smi::New(value);
}


intptr_t PcDescriptors::TryIndex(intptr_t index) const {
  return *(EntryAddr(index, kTryIndexEntry));
}


void PcDescriptors::SetTryIndex(intptr_t index, intptr_t value) const {
  *(EntryAddr(index, kTryIndexEntry)) = value;
}


RawPcDescriptors* PcDescriptors::New(intptr_t num_descriptors) {
  const Class& cls = Class::Handle(Object::pc_descriptors_class());
  PcDescriptors& result = PcDescriptors::Handle();
  {
    uword size = PcDescriptors::InstanceSize(num_descriptors);
    RawObject* raw = Object::Allocate(cls, size, Heap::kOld);
    NoGCScope no_gc;
    result ^= raw;
    result.SetLength(num_descriptors);
  }
  return result.raw();
}


const char* PcDescriptors::KindAsStr(intptr_t index) const {
  switch (DescriptorKind(index)) {
    case (PcDescriptors::kDeopt) : return "deopt";
    case (PcDescriptors::kPatchCode) : return "patch";
    case (PcDescriptors::kIcCall) : return "ic-call";
    case (PcDescriptors::kOther) : return "other";
  }
  UNREACHABLE();
  return "";
}


const char* PcDescriptors::ToCString() const {
  if (Length() == 0) {
    return "No pc descriptors\n";
  }
  const char* kFormat = "0x%x, %s %ld %ld, %ld\n";
  // First compute the buffer size required.
  intptr_t len = 0;
  for (intptr_t i = 0; i < Length(); i++) {
    len += OS::SNPrint(NULL, 0, kFormat,
        PC(i), KindAsStr(i), NodeId(i), TryIndex(i), TokenIndex(i));
  }
  // Allocate the buffer.
  char* buffer = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(len + 1));
  // Layout the fields in the buffer.
  intptr_t index = 0;
  for (intptr_t i = 0; i < Length(); i++) {
    index += OS::SNPrint((buffer + index), (len - index), kFormat,
        PC(i), KindAsStr(i), NodeId(i), TryIndex(i), TokenIndex(i));
  }
  return buffer;
}


intptr_t ExceptionHandlers::Length() const {
  return Smi::Value(raw_ptr()->length_);
}


void ExceptionHandlers::SetLength(intptr_t value) const {
  // This is only safe because we create a new Smi, which does not cause
  // heap allocation.
  raw_ptr()->length_ = Smi::New(value);
}


intptr_t ExceptionHandlers::TryIndex(intptr_t index) const {
  return *(EntryAddr(index, kTryIndexEntry));
}


void ExceptionHandlers::SetTryIndex(intptr_t index, intptr_t value) const {
  *(EntryAddr(index, kTryIndexEntry)) = value;
}


intptr_t ExceptionHandlers::HandlerPC(intptr_t index) const {
  return *(EntryAddr(index, kHandlerPcEntry));
}


void ExceptionHandlers::SetHandlerPC(intptr_t index,
                                     intptr_t value) const {
  *(EntryAddr(index, kHandlerPcEntry)) = value;
}


RawExceptionHandlers* ExceptionHandlers::New(intptr_t num_handlers) {
  const Class& cls = Class::Handle(Object::exception_handlers_class());
  ExceptionHandlers& result = ExceptionHandlers::Handle();
  {
    uword size = ExceptionHandlers::InstanceSize(num_handlers);
    RawObject* raw = Object::Allocate(cls, size, Heap::kOld);
    NoGCScope no_gc;
    result ^= raw;
    result.SetLength(num_handlers);
  }
  return result.raw();
}


const char* ExceptionHandlers::ToCString() const {
  if (Length() == 0) {
    return "No exception handlers\n";
  }
  // First compute the buffer size required.
  intptr_t len = 0;
  for (intptr_t i = 0; i < Length(); i++) {
    len += OS::SNPrint(NULL, 0, "%ld => 0x%x\n",
                       TryIndex(i), HandlerPC(i));
  }
  // Allocate the buffer.
  char* buffer = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(len + 1));
  // Layout the fields in the buffer.
  intptr_t index = 0;
  for (intptr_t i = 0; i < Length(); i++) {
    index += OS::SNPrint((buffer + index),
                         (len - index),
                         "%ld => 0x%x\n",
                         TryIndex(i),
                         HandlerPC(i));
  }
  return buffer;
}


RawCode* Code::New(int pointer_offsets_length) {
  const Class& cls = Class::Handle(Object::code_class());
  Code& result = Code::Handle();
  {
    uword size = Code::InstanceSize(pointer_offsets_length);
    RawObject* raw = Object::Allocate(cls, size, Heap::kOld);
    NoGCScope no_gc;
    result ^= raw;
    result.set_pointer_offsets_length(pointer_offsets_length);
    result.set_is_optimized(false);
  }
  result.raw_ptr()->ic_data_ = Array::Empty();
  result.raw_ptr()->class_ic_stubs_ = Array::Empty();
  return result.raw();
}


RawCode* Code::FinalizeCode(const char* name, Assembler* assembler) {
  ASSERT(assembler != NULL);

  // Allocate the Instructions object.
  Instructions& instrs =
      Instructions::ZoneHandle(Instructions::New(assembler->CodeSize()));

  // Copy the instructions into the instruction area and apply all fixups.
  // Embedded pointers are still in handles at this point.
  MemoryRegion region(reinterpret_cast<void*>(instrs.EntryPoint()),
                      instrs.size());
  assembler->FinalizeInstructions(region);
  DebugInfo* pprof_symbol_generator = Dart::pprof_symbol_generator();
  if (pprof_symbol_generator != NULL) {
    pprof_symbol_generator->AddCode(instrs.EntryPoint(), instrs.size());
    pprof_symbol_generator->AddCodeRegion(name,
                                          instrs.EntryPoint(),
                                          instrs.size());
  }
  if (FLAG_generate_gdb_symbols) {
    intptr_t prolog_offset = assembler->prolog_offset();
    if (prolog_offset > 0) {
      // In order to ensure that gdb sees the first instruction of a function
      // as the prolog sequence we register two symbols for the cases when
      // the prolog sequence is not the first instruction:
      // <name>_entry is used for code preceding the prolog sequence.
      // <name> for rest of the code (first instruction is prolog sequence).
      const char* kFormat = "%s_%s";
      intptr_t len = OS::SNPrint(NULL, 0, kFormat, name, "entry");
      char* pname = reinterpret_cast<char*>(
          Isolate::Current()->current_zone()->Allocate(len + 1));
      OS::SNPrint(pname, (len + 1), kFormat, name, "entry");
      DebugInfo::RegisterSection(pname, instrs.EntryPoint(), prolog_offset);
      DebugInfo::RegisterSection(name,
                                 (instrs.EntryPoint() + prolog_offset),
                                 (instrs.size() - prolog_offset));
    } else {
      DebugInfo::RegisterSection(name, instrs.EntryPoint(), instrs.size());
    }
  }

  const ZoneGrowableArray<int>& pointer_offsets =
      assembler->GetPointerOffsets();

  // Allocate the code object.
  Code& code = Code::ZoneHandle(Code::New(pointer_offsets.length()));
  {
    NoGCScope no_gc;

    // Set pointer offsets list in Code object and resolve all handles in
    // the instruction stream to raw objects.
    ASSERT(code.pointer_offsets_length() == pointer_offsets.length());
    for (int i = 0; i < pointer_offsets.length(); i++) {
      int offset_in_instrs = pointer_offsets[i];
      code.SetPointerOffsetAt(i, offset_in_instrs);
      const Object* object = region.Load<const Object*>(offset_in_instrs);
      region.Store<RawObject*>(offset_in_instrs, object->raw());
    }

    // Hook up Code and Instruction objects.
    instrs.set_code(code.raw());
    code.set_instructions(instrs.raw());
  }
  return code.raw();
}


RawArray* Code::ic_data() const {
  return raw_ptr()->ic_data_;
}


void Code::set_ic_data(const Array& ic_data) const {
  ASSERT(!ic_data.IsNull());
  StorePointer(&raw_ptr()->ic_data_, ic_data.raw());
}


RawArray* Code::class_ic_stubs() const {
  return raw_ptr()->class_ic_stubs_;
}


void Code::set_class_ic_stubs(const Array& class_ic_stubs) const {
  ASSERT(!class_ic_stubs.IsNull());
  StorePointer(&raw_ptr()->class_ic_stubs_, class_ic_stubs.raw());
}


intptr_t Code::GetTokenIndexOfPC(uword pc) const {
  intptr_t token_index = -1;
  const PcDescriptors& descriptors = PcDescriptors::Handle(pc_descriptors());
  for (intptr_t i = 0; i < descriptors.Length(); i++) {
    if (static_cast<uword>(descriptors.PC(i)) == pc) {
      token_index = descriptors.TokenIndex(i);
      break;
    }
  }
  return token_index;
}


uword Code::GetDeoptPcAtNodeId(intptr_t node_id) const {
  const PcDescriptors& descriptors = PcDescriptors::Handle(pc_descriptors());
  for (intptr_t i = 0; i < descriptors.Length(); i++) {
    if ((descriptors.NodeId(i) == node_id) &&
        (descriptors.DescriptorKind(i) == PcDescriptors::kDeopt)) {
      return descriptors.PC(i);
    }
  }
  return 0;
}


const char* Code::ToCString() const {
  const char* kFormat = "Code entry:0x%d icstubs: %d";
  intptr_t len = OS::SNPrint(NULL, 0, kFormat,
      EntryPoint(),
      (Array::Handle(class_ic_stubs()).Length() / 2)) + 1;
  char* chars = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(len));
  OS::SNPrint(chars, len, kFormat,
      EntryPoint(),
      (Array::Handle(class_ic_stubs()).Length() / 2));
  return chars;
}


uword Code::GetPatchCodePc() const {
  const PcDescriptors& descriptors = PcDescriptors::Handle(pc_descriptors());
  for (intptr_t i = 0; i < descriptors.Length(); i++) {
    if (descriptors.DescriptorKind(i) == PcDescriptors::kPatchCode) {
      return descriptors.PC(i);
    }
  }
  return 0;
}


bool Code::ObjectExistInArea(intptr_t start_offset, intptr_t end_offset) const {
  for (intptr_t i = 0; i < this->pointer_offsets_length(); i++) {
    const intptr_t offset = this->GetPointerOffsetAt(i);
    if ((start_offset <= offset) && (offset < end_offset)) {
      return false;
    }
  }
  return true;
}


void Code::ExtractTypesAtIcCalls(
    GrowableArray<intptr_t>* node_ids,
    GrowableArray<ZoneGrowableArray<const Class*>*>* type_arrays) const {
  ASSERT(node_ids != NULL);
  ASSERT(type_arrays != NULL);
  const PcDescriptors& descriptors =
      PcDescriptors::Handle(this->pc_descriptors());
  String& function_name = String::Handle();
  for (intptr_t i = 0; i < descriptors.Length(); i++) {
    if (descriptors.DescriptorKind(i) == PcDescriptors::kIcCall) {
      int num_arguments = -1;
      int num_named_arguments = -1;
      uword caller_target = 0;
      CodePatcher::GetInstanceCallAt(descriptors.PC(i),
                                     &function_name,
                                     &num_arguments,
                                     &num_named_arguments,
                                     &caller_target);
      GrowableArray<const Class*> classes;
      GrowableArray<const Function*> targets;
      bool is_ic = ICStubs::RecognizeICStub(caller_target, &classes, &targets);
      ASSERT(is_ic);
      ZoneGrowableArray<const Class*>* types =
          new ZoneGrowableArray<const Class*>();
      for (intptr_t k = 0; k < classes.length(); k++) {
        types->Add(classes[k]);
      }
      node_ids->Add(descriptors.NodeId(i));
      type_arrays->Add(types);
    }
  }
}


RawContext* Context::New(intptr_t num_variables, Heap::Space space) {
  ASSERT(num_variables >= 0);

  const Class& context_class = Class::Handle(Object::context_class());
  Context& result = Context::Handle();
  {
    RawObject* raw = Object::Allocate(context_class,
                                      Context::InstanceSize(num_variables),
                                      space);
    NoGCScope no_gc;
    result ^= raw;
    result.set_num_variables(num_variables);
  }
  result.set_isolate(Isolate::Current());
  return result.raw();
}


const char* Context::ToCString() const {
  return "Context";
}


RawContextScope* ContextScope::New(intptr_t num_variables) {
  const Class& context_scope_class =
    Class::Handle(Object::context_scope_class());
  intptr_t size = ContextScope::InstanceSize(num_variables);
  ContextScope& result = ContextScope::Handle();
  {
    RawObject* raw = Object::Allocate(context_scope_class, size, Heap::kOld);
    NoGCScope no_gc;
    result ^= raw;
    result.set_num_variables(num_variables);
  }
  return result.raw();
}


intptr_t ContextScope::TokenIndexAt(intptr_t scope_index) const {
  return Smi::Value(VariableDescAddr(scope_index)->token_index);
}


void ContextScope::SetTokenIndexAt(intptr_t scope_index,
                                   intptr_t token_index) const {
  VariableDescAddr(scope_index)->token_index = Smi::New(token_index);
}


RawString* ContextScope::NameAt(intptr_t scope_index) const {
  return VariableDescAddr(scope_index)->name;
}


void ContextScope::SetNameAt(intptr_t scope_index, const String& name) const {
  VariableDescAddr(scope_index)->name = name.raw();
}


bool ContextScope::IsFinalAt(intptr_t scope_index) const {
  return Bool::Handle(VariableDescAddr(scope_index)->is_final).value();
}


void ContextScope::SetIsFinalAt(intptr_t scope_index, bool is_final) const {
  VariableDescAddr(scope_index)->is_final = Bool::Get(is_final);
}


RawType* ContextScope::TypeAt(intptr_t scope_index) const {
  return VariableDescAddr(scope_index)->type;
}


void ContextScope::SetTypeAt(intptr_t scope_index, const Type& type) const {
  VariableDescAddr(scope_index)->type = type.raw();
}


intptr_t ContextScope::ContextIndexAt(intptr_t scope_index) const {
  return Smi::Value(VariableDescAddr(scope_index)->context_index);
}


void ContextScope::SetContextIndexAt(intptr_t scope_index,
                                     intptr_t context_index) const {
  VariableDescAddr(scope_index)->context_index = Smi::New(context_index);
}


intptr_t ContextScope::ContextLevelAt(intptr_t scope_index) const {
  return Smi::Value(VariableDescAddr(scope_index)->context_level);
}


void ContextScope::SetContextLevelAt(intptr_t scope_index,
                                     intptr_t context_level) const {
  VariableDescAddr(scope_index)->context_level = Smi::New(context_level);
}


const char* ContextScope::ToCString() const {
  return "ContextScope";
}


RawUnhandledException* UnhandledException::New(const Instance& exception,
                                               const Instance& stacktrace,
                                               Heap::Space space) {
  Isolate* isolate = Isolate::Current();
  const Class& cls = Class::Handle(
      isolate->object_store()->unhandled_exception_class());
  UnhandledException& result = UnhandledException::Handle();
  {
    RawObject* raw = Object::Allocate(cls,
                                      UnhandledException::InstanceSize(),
                                      space);
    NoGCScope no_gc;
    result ^= raw;
  }
  result.set_exception(exception);
  result.set_stacktrace(stacktrace);
  return result.raw();
}


void UnhandledException::set_exception(const Instance& exception) const {
  StorePointer(&raw_ptr()->exception_, exception.raw());
}


void UnhandledException::set_stacktrace(const Instance& stacktrace) const {
  StorePointer(&raw_ptr()->stacktrace_, stacktrace.raw());
}


const char* UnhandledException::ToCString() const {
  return "UnhandledException";
}


bool Instance::Equals(const Instance& other) const {
  if (this->raw() == other.raw()) {
    return true;  // "===".
  }

  if (other.IsNull() || (this->clazz() != other.clazz())) {
    return false;
  }

  {
    NoGCScope no_gc;
    // Raw bits compare.
    const intptr_t instance_size = Class::Handle(this->clazz()).instance_size();
    ASSERT(instance_size != 0);
    uword this_addr = reinterpret_cast<uword>(this->raw_ptr());
    uword other_addr = reinterpret_cast<uword>(other.raw_ptr());
    for (intptr_t offset = 0; offset < instance_size; offset += kWordSize) {
      if ((*reinterpret_cast<RawObject**>(this_addr + offset)) !=
          (*reinterpret_cast<RawObject**>(other_addr + offset))) {
        return false;
      }
    }
  }
  return true;
}


RawInstance* Instance::Canonicalize() const {
  const Class& cls = Class::Handle(this->clazz());
  Array& constants = Array::Handle(cls.constants());
  const intptr_t num_constants = cls.num_constants();
  ASSERT(constants.Length() >= num_constants);
  // Linear search to see whether this value is already present in the
  // list of canonicalized constants.
  Instance& norm_value = Instance::Handle();
  for (int i = 0; i < num_constants; i++) {
    norm_value ^= constants.At(i);
    ASSERT(!norm_value.IsNull());
    if (this->Equals(norm_value)) {
      return norm_value.raw();
    }
  }
  // The value needs to be added to the list. Grow the list if
  // it is full.
  if (constants.Length() == num_constants) {
    const intptr_t kInitialConstLength = 4;
    const intptr_t old_length = constants.Length();
    const intptr_t new_length =
        (old_length == 0) ? kInitialConstLength : old_length * 2;
    const Array& new_constants =
        Array::Handle(Array::Grow(constants, new_length, Heap::kOld));
    cls.set_constants(new_constants);
    new_constants.SetAt(num_constants, *this);
  } else {
    constants.SetAt(num_constants, *this);
  }
  cls.set_num_constants(num_constants + 1);
  return this->raw();
}


RawType* Instance::GetType() const {
  if (IsNull()) {
    return Type::NullType();
  }
  const Class& cls = Class::Handle(clazz());
  TypeArguments& type_arguments = TypeArguments::Handle();
  if (cls.IsParameterized()) {
    type_arguments = GetTypeArguments();
  }
  return Type::NewParameterizedType(cls, type_arguments);
}


RawTypeArguments* Instance::GetTypeArguments() const {
  const Class& cls = Class::Handle(clazz());
  intptr_t field_offset = cls.type_arguments_instance_field_offset();
  ASSERT(field_offset != Class::kNoTypeArguments);
  TypeArguments& type_arguments = TypeArguments::Handle();
  type_arguments ^= *FieldAddrAtOffset(field_offset);
  return type_arguments.raw();
}


void Instance::SetTypeArguments(const TypeArguments& value) const {
  const Class& cls = Class::Handle(clazz());
  intptr_t field_offset = cls.type_arguments_instance_field_offset();
  ASSERT(field_offset != Class::kNoTypeArguments);
  *FieldAddrAtOffset(field_offset) = value.raw();
}


bool Instance::TestType(TypeTestKind test,
                        const Type& other,
                        const TypeArguments& other_instantiator) const {
  ASSERT(other.IsFinalized());
  ASSERT(!other.IsVarType());
  ASSERT(!other.IsVoidType());
  if (IsNull()) {
    if (test == Type::kIsSubtypeOf) {
      const Type& object_type =
          Type::Handle(Isolate::Current()->object_store()->object_type());
      if (other.IsInstantiated() && object_type.IsSubtypeOf(other)) {
        ASSERT(other_instantiator.IsNull());
        // null is an instance of the Object class.
        return true;
      }
      return false;
    } else {
      ASSERT(test == Type::kIsAssignableTo);
      return true;
    }
  }
  const Class& cls = Class::Handle(clazz());
  TypeArguments& type_arguments = TypeArguments::Handle();
  if (cls.IsParameterized()) {
    type_arguments = GetTypeArguments();
    ASSERT(type_arguments.IsNull() ||
           (type_arguments.Length() == cls.NumTypeArguments()));
  }
  Class& other_class = Class::Handle();
  TypeArguments& other_type_arguments = TypeArguments::Handle();
  // In case 'other' is not instantiated, we could simply call
  // other.InstantiateFrom(other_instantiator, 0), however, we can save the
  // allocation of a new Type by inlining the code.
  if (other.IsTypeParameter()) {
    Type& instantiated_other = Type::Handle();
    if (!other_instantiator.IsNull()) {
      instantiated_other = other_instantiator.TypeAt(other.Index());
      ASSERT(instantiated_other.IsInstantiated());
    } else {
      instantiated_other = Type::VarType();
    }
    other_class = instantiated_other.type_class();
    other_type_arguments = instantiated_other.arguments();
  } else {
    other_class = other.type_class();
    other_type_arguments = other.arguments();
    if (!other_type_arguments.IsNull() &&
        !other_type_arguments.IsInstantiated()) {
      other_type_arguments =
          other_type_arguments.InstantiateFrom(other_instantiator, 0);
    }
  }
  return cls.TestType(test, type_arguments, other_class, other_type_arguments);
}


bool Instance::IsValidNativeIndex(int index) const {
  const Class& cls = Class::Handle(clazz());
  return (index >= 0 && index < cls.num_native_fields());
}


  RawInstance* Instance::New(const Class& cls, Heap::Space space) {
  Instance& result = Instance::Handle();
  {
    intptr_t instance_size = cls.instance_size();
    ASSERT(instance_size > 0);
    RawObject* raw = Object::Allocate(cls, instance_size, space);
    NoGCScope no_gc;
    result ^= raw;
    uword addr = reinterpret_cast<uword>(result.raw_ptr());
    // Initialize fields.
    intptr_t offset = sizeof(RawObject);
    // Initialize all native fields to NULL.
    for (intptr_t i = 0; i < cls.num_native_fields(); i++) {
      *reinterpret_cast<uword*>(addr + offset) = 0;
      offset += kWordSize;
    }
    // Initialize all dart fields to null.
    while (offset < instance_size) {
      *reinterpret_cast<RawObject**>(addr + offset) = Object::null();
      offset += kWordSize;
    }
  }
  return result.raw();
}


bool Instance::IsValidFieldOffset(int offset) const {
  const Class& cls = Class::Handle(clazz());
  return (offset >= 0 && offset <= (cls.instance_size() - kWordSize));
}


const char* Instance::ToCString() const {
  if (IsNull()) {
    return "null";
  } else {
    // TODO(regis): Print type arguments if any.
    const char* kFormat = "Instance of '%s'";
    Class& cls = Class::Handle(clazz());
    // Calculate the size of the string.
    intptr_t len = OS::SNPrint(NULL, 0, kFormat, cls.ToCString()) + 1;
    char* chars = reinterpret_cast<char*>(
        Isolate::Current()->current_zone()->Allocate(len));
    OS::SNPrint(chars, len, kFormat, cls.ToCString());
    return chars;
  }
}


const char* Number::ToCString() const {
  // Number is an interface. No instances of Number should exist.
  UNREACHABLE();
  return "Number";
}


const char* Integer::ToCString() const {
  // Integer is an interface. No instances of Integer should exist.
  UNREACHABLE();
  return "Integer";
}


RawInteger* Integer::New(const String& str) {
  const Bigint& big = Bigint::Handle(Bigint::New(str));
  if (BigintOperations::FitsIntoSmi(big)) {
    return BigintOperations::ToSmi(big);
  } else if (BigintOperations::FitsIntoInt64(big)) {
    return Mint::New(BigintOperations::ToInt64(big));
  } else {
    return big.raw();
  }
}


RawInteger* Integer::New(int64_t value) {
  if ((value <= Smi::kMaxValue) && (value >= Smi::kMinValue)) {
    return Smi::New(value);
  }
  return Mint::New(value);
}


double Integer::AsDoubleValue() const {
  UNIMPLEMENTED();
  return 0.0;
}


int64_t Integer::AsInt64Value() const {
  UNIMPLEMENTED();
  return 0;
}


int Integer::CompareWith(const Integer& other) const {
  UNIMPLEMENTED();
  return 0;
}


bool Smi::Equals(const Instance& other) const {
  if (other.IsNull() || !other.IsSmi()) {
    return false;
  }

  Smi& other_smi = Smi::Handle();
  other_smi ^= other.raw();
  return (this->Value() == other_smi.Value());
}


bool Smi::IsValid(intptr_t value) {
  return (value >= kMinValue) && (value <= kMaxValue);
}


bool Smi::IsValid64(int64_t value) {
  return (value >= kMinValue) && (value <= kMaxValue);
}


double Smi::AsDoubleValue() const {
  return static_cast<double>(this->Value());
}


int64_t Smi::AsInt64Value() const {
  return this->Value();
}


static bool FitsIntoSmi(const Integer& integer) {
  if (integer.IsSmi()) {
    return true;
  }
  if (integer.IsMint()) {
    int64_t mint_value = integer.AsInt64Value();
    return Smi::IsValid64(mint_value);
  }
  if (integer.IsBigint()) {
    Bigint& big = Bigint::Handle();
    big ^= integer.raw();
    return BigintOperations::FitsIntoSmi(big);
  }
  UNREACHABLE();
  return false;
}


int Smi::CompareWith(const Integer& other) const {
  if (other.IsSmi()) {
    Smi& smi = Smi::Handle();
    smi ^= other.raw();
    if (this->Value() < smi.Value()) {
      return -1;
    } else if (this->Value() > smi.Value()) {
      return 1;
    } else {
      return 0;
    }
  }
  ASSERT(!FitsIntoSmi(other));
  if (other.IsMint() || other.IsBigint()) {
    if (this->IsNegative() == other.IsNegative()) {
      return this->IsNegative() ? 1 : -1;
    }
    return this->IsNegative() ? -1 : 1;
  }
  UNREACHABLE();
  return 0;
}


const char* Smi::ToCString() const {
  const char* kFormat = "%ld";
  // Calculate the size of the string.
  intptr_t len = OS::SNPrint(NULL, 0, kFormat, Value()) + 1;
  char* chars = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(len));
  OS::SNPrint(chars, len, kFormat, Value());
  return chars;
}


RawClass* Smi::Class() {
  return Isolate::Current()->object_store()->smi_class();
}


void Mint::set_value(int64_t value) const {
  raw_ptr()->value_ = value;
}


RawMint* Mint::New(int64_t val, Heap::Space space) {
  // Do not allocate a Mint if Smi would do.
  ASSERT(!Smi::IsValid64(val));
  Isolate* isolate = Isolate::Current();
  const Class& cls =
      Class::Handle(isolate->object_store()->mint_class());
  Mint& result = Mint::Handle();
  {
    RawObject* raw = Object::Allocate(cls, Mint::InstanceSize(), space);
    NoGCScope no_gc;
    result ^= raw;
  }
  result.set_value(val);
  return result.raw();
}


bool Mint::Equals(const Instance& other) const {
  if (this->raw() == other.raw()) {
    // Both handles point to the same raw instance.
    return true;
  }

  if (!other.IsMint() || other.IsNull()) {
    return false;
  }

  Mint& other_mint = Mint::Handle();
  other_mint ^= other.raw();

  return value() == other_mint.value();
}


double Mint::AsDoubleValue() const {
  return static_cast<double>(this->value());
}


int64_t Mint::AsInt64Value() const {
  return this->value();
}


int Mint::CompareWith(const Integer& other) const {
  ASSERT(!FitsIntoSmi(*this));
  if (other.IsMint() || other.IsSmi()) {
    int64_t a = AsInt64Value();
    int64_t b = other.AsInt64Value();
    if (a < b) {
      return -1;
    } else if (a > b) {
      return 1;
    } else {
      return 0;
    }
  }
  if (other.IsBigint()) {
    Bigint& bigi = Bigint::Handle();
    bigi ^= other.raw();
    ASSERT(!BigintOperations::FitsIntoInt64(bigi));
    if (this->IsNegative() == other.IsNegative()) {
      return this->IsNegative() ? 1 : -1;
    }
    return this->IsNegative() ? -1 : 1;
  }
  UNREACHABLE();
  return 0;
}


const char* Mint::ToCString() const {
  const char* kFormat = "%lld";
  // Calculate the size of the string.
  intptr_t len = OS::SNPrint(NULL, 0, kFormat, value()) + 1;
  char* chars = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(len));
  OS::SNPrint(chars, len, kFormat, value());
  return chars;
}


void Double::set_value(double value) const {
  raw_ptr()->value_ = value;
}


RawDouble* Double::New(double d, Heap::Space space) {
  Isolate* isolate = Isolate::Current();
  const Class& cls =
      Class::Handle(isolate->object_store()->double_class());
  Double& result = Double::Handle();
  {
    RawObject* raw = Object::Allocate(cls, Double::InstanceSize(), space);
    NoGCScope no_gc;
    result ^= raw;
  }
  result.set_value(d);
  return result.raw();
}


RawDouble* Double::New(const String& str, Heap::Space space) {
  // TODO(regis): For now, we use strtod to convert a string to double.
  const char* nptr = str.ToCString();
  char* endptr = NULL;
  double double_value = strtod(nptr, &endptr);
  // We do not treat overflow or underflow as an error and therefore do not
  // check errno for ERANGE.
  if ((*endptr != '\0')) {
    return Double::Handle().raw();
  }
  return New(double_value, space);
}


const char* Double::ToCString() const {
  if (isnan(value())) {
    return "NaN";
  }
  if (isinf(value())) {
    return value() < 0 ? "-Infinity" : "Infinity";
  }
  const char* kFormat = "%f";
  // Calculate the size of the string.
  intptr_t len = OS::SNPrint(NULL, 0, kFormat, value()) + 1;
  char* chars = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(len));
  OS::SNPrint(chars, len, kFormat, value());
  return chars;
}


bool Bigint::Equals(const Instance& other) const {
  if (this->raw() == other.raw()) {
    // Both handles point to the same raw instance.
    return true;
  }

  if (!other.IsBigint() || other.IsNull()) {
    return false;
  }

  Bigint& other_bgi = Bigint::Handle();
  other_bgi ^= other.raw();

  return BN_cmp(BNAddr(), other_bgi.BNAddr()) == 0;
}


RawBigint* Bigint::New(const BIGNUM *bn, Heap::Space space) {
  Isolate* isolate = Isolate::Current();
  const Class& cls = Class::Handle(isolate->object_store()->bigint_class());
  Bigint& result = Bigint::Handle();
  {
    RawObject* raw = Object::Allocate(cls,
                                      Bigint::InstanceSize(bn),
                                      space);
    NoGCScope no_gc;
    result ^= raw;
    // Danger Will Robinson! Use of OpenSSL internals!
    // Copy the OpenSSL BIGNUM to our own heap. Don't fix up our d
    // pointer, that'll get done for us.
    BIGNUM* our_bn = result.MutableBNAddr();
    // memcpy would be sufficient.
    memmove(our_bn, bn, sizeof *bn);
    memmove(result.BNMemory(), bn->d, bn->top * sizeof(BN_ULONG));
    // We only allocated/copied the active part.
    our_bn->dmax = our_bn->top;
  }

  return result.raw();
}


RawBigint* Bigint::New(const String& str, Heap::Space space) {
  return BigintOperations::NewFromCString(str.ToCString(), space);
}


RawBigint* Bigint::New(int64_t value, Heap::Space space) {
  return BigintOperations::NewFromInt64(value, space);
}


double Bigint::AsDoubleValue() const {
  return Double::Handle(BigintOperations::ToDouble(*this)).value();
}


int64_t Bigint::AsInt64Value() const {
  if (!BigintOperations::FitsIntoInt64(*this)) {
    UNREACHABLE();
  }
  return BigintOperations::ToInt64(*this);
}


// For positive values: Smi < Mint < Bigint.
int Bigint::CompareWith(const Integer& other) const {
  ASSERT(!FitsIntoSmi(*this));
  ASSERT(!BigintOperations::FitsIntoInt64(*this));
  if (other.IsBigint()) {
    Bigint& big = Bigint::Handle();
    big ^= other.raw();
    return BigintOperations::Compare(*this, big);
  }
  if (this->IsNegative() == other.IsNegative()) {
    return this->IsNegative() ? -1 : 1;
  }
  return this->IsNegative() ? -1 : 1;
}


static uword ZoneAllocator(intptr_t size) {
  Zone* zone = Isolate::Current()->current_zone();
  return zone->Allocate(size);
}


const char* Bigint::ToCString() const {
  return BigintOperations::ToHexCString(*this, &ZoneAllocator);
}


class StringHasher : ValueObject {
 public:
  StringHasher() : hash_(0) {}
  void Add(int32_t ch) {
    hash_ += ch;
    hash_ += hash_ << 10;
    hash_ ^= hash_ >> 6;
  }
  // Return a non-zero hash of at most 'bits' bits.
  intptr_t Finalize(int bits) {
    ASSERT(1 <= bits && bits <= (kBitsPerWord - 1));
    hash_ += hash_ << 3;
    hash_ ^= hash_ >> 11;
    hash_ += hash_ << 15;
    hash_ = hash_ & ((static_cast<intptr_t>(1) << bits) - 1);
    ASSERT(hash_ >= 0);
    return hash_ == 0 ? 1 : hash_;
  }
 private:
  intptr_t hash_;
};


intptr_t String::Hash() const {
  intptr_t result = Smi::Value(raw_ptr()->hash_);
  if (result != 0) {
    return result;
  }
  result = String::Hash(*this, 0, this->Length());
  this->SetHash(result);
  return result;
}


intptr_t String::Hash(const String& str, intptr_t begin_index, intptr_t len) {
  ASSERT(begin_index >= 0);
  ASSERT(len >= 0);
  ASSERT((begin_index + len) <= str.Length());
  StringHasher hasher;
  for (intptr_t i = 0; i < len; i++) {
    hasher.Add(str.CharAt(begin_index + i));
  }
  return hasher.Finalize(String::kHashBits);
}


template<typename T>
static intptr_t HashImpl(const T* characters, intptr_t len) {
  ASSERT(len >= 0);
  StringHasher hasher;
  for (intptr_t i = 0; i < len; i++) {
    hasher.Add(characters[i]);
  }
  return hasher.Finalize(String::kHashBits);
}


intptr_t String::Hash(const uint8_t* characters, intptr_t len) {
  return HashImpl(characters, len);
}


intptr_t String::Hash(const uint16_t* characters, intptr_t len) {
  return HashImpl(characters, len);
}


intptr_t String::Hash(const uint32_t* characters, intptr_t len) {
  return HashImpl(characters, len);
}


int32_t String::CharAt(intptr_t index) const {
  // String is an abstract class.
  UNREACHABLE();
  return 0;
}


bool String::Equals(const Instance& other) const {
  if (this->raw() == other.raw()) {
    // Both handles point to the same raw instance.
    return true;
  }

  if (!other.IsString() || other.IsNull()) {
    return false;
  }

  String& other_string = String::Handle();
  other_string ^= other.raw();
  if (this->HasHash() && other_string.HasHash() &&
      (this->Hash() != other_string.Hash())) {
    // Both sides have a hash code and it does not match.
    return false;
  }

  intptr_t len = this->Length();
  if (len != other_string.Length()) {
    // Lengths don't match.
    return false;
  }

  for (intptr_t i = 0; i < len; i++) {
    if (this->CharAt(i) != other_string.CharAt(i)) {
      return false;
    }
  }
  return true;
}


bool String::Equals(const String& str,
                    intptr_t begin_index,
                    intptr_t len) const {
  ASSERT(begin_index >= 0);
  ASSERT(begin_index == 0 || begin_index < str.Length());
  ASSERT(len >= 0);
  ASSERT(len <= str.Length());
  if (len != this->Length()) {
    // Lengths don't match.
    return false;
  }

  for (intptr_t i = 0; i < len; i++) {
    if (this->CharAt(i) != str.CharAt(begin_index + i)) {
      return false;
    }
  }
  return true;
}


bool String::Equals(const char* str) const {
  for (intptr_t i = 0; i < this->Length(); ++i) {
    if (*str == '\0') {
      // Lengths don't match.
      return false;
    }
    int32_t ch;
    intptr_t consumed = Utf8::Decode(str, &ch);
    if (consumed == 0 || this->CharAt(i) != ch) {
      return false;
    }
    str += consumed;
  }
  return true;
}


bool String::Equals(const uint8_t* characters, intptr_t len) const {
  if (len != this->Length()) {
    // Lengths don't match.
    return false;
  }

  for (intptr_t i = 0; i < len; i++) {
    if (this->CharAt(i) != characters[i]) {
      return false;
    }
  }
  return true;
}


bool String::Equals(const uint16_t* characters, intptr_t len) const {
  if (len != this->Length()) {
    // Lengths don't match.
    return false;
  }

  for (intptr_t i = 0; i < len; i++) {
    if (this->CharAt(i) != characters[i]) {
      return false;
    }
  }
  return true;
}


bool String::Equals(const uint32_t* characters, intptr_t len) const {
  if (len != this->Length()) {
    // Lengths don't match.
    return false;
  }

  for (intptr_t i = 0; i < len; i++) {
    if (this->CharAt(i) != static_cast<int32_t>(characters[i])) {
      return false;
    }
  }
  return true;
}


intptr_t String::CompareTo(const String& other) const {
  const intptr_t this_len = this->Length();
  const intptr_t other_len = other.IsNull() ? 0 : other.Length();
  const intptr_t len = (this_len < other_len) ? this_len : other_len;
  for (intptr_t i = 0; i < len; i++) {
    int32_t this_code_point = this->CharAt(i);
    int32_t other_code_point = other.CharAt(i);
    if (this_code_point < other_code_point) {
      return -1;
    }
    if (this_code_point > other_code_point) {
      return 1;
    }
  }
  if (this_len < other_len) return -1;
  if (this_len > other_len) return 1;
  return 0;
}


bool String::StartsWith(const String& other) const {
  if (other.IsNull() || (other.Length() > this->Length())) {
    return false;
  }
  intptr_t slen = other.Length();
  for (int i = 0; i < slen; i++) {
    if (this->CharAt(i) != other.CharAt(i)) {
      return false;
    }
  }
  return true;
}


RawInstance* String::Canonicalize() const {
  return NewSymbol(*this);
}


bool String::IsSymbol() const {
  if (!HasHash()) {
    // All symbols have had their hash calculated.
    return false;
  }

  // Get the hash for this string.
  intptr_t hash = Hash();

  ObjectStore* object_store = Isolate::Current()->object_store();
  const Array& symbol_table = Array::Handle(object_store->symbol_table());
  // Last element of the array is the number of used elements.
  intptr_t table_size = symbol_table.Length() - 1;
  intptr_t index = hash % table_size;

  // Try to find this string object in the symbol table. The symbol table is
  // never entirely full so this loop will terminate.
  String& symbol = String::Handle();
  symbol ^= symbol_table.At(index);
  while (!symbol.IsNull() && (raw_ptr() != symbol.raw_ptr())) {
    index = (index + 1) % table_size;  // Move to next element.
    symbol ^= symbol_table.At(index);
  }

  // This string is a symbol if we found a matching entry.
  return !symbol.IsNull();
}


RawString* String::New(const char* str, Heap::Space space) {
  intptr_t width = 0;
  intptr_t len = Utf8::CodePointCount(str, &width);
  if (width == 1) {
    OneByteString& onestr
        = OneByteString::Handle(OneByteString::New(len, space));
    if (len > 0) {
      NoGCScope no_gc;
      Utf8::Decode(str, onestr.CharAddr(0), len);
    }
    return onestr.raw();
  } else if (width == 2) {
    TwoByteString& twostr =
        TwoByteString::Handle(TwoByteString::New(len, space));
    NoGCScope no_gc;
    Utf8::Decode(str, twostr.CharAddr(0), len);
    return twostr.raw();
  }
  ASSERT(width == 4);
  FourByteString& fourstr =
      FourByteString::Handle(FourByteString::New(len, space));
  NoGCScope no_gc;
  Utf8::Decode(str, fourstr.CharAddr(0), len);
  return fourstr.raw();
}


RawString* String::New(const uint8_t* characters,
                       intptr_t len,
                       Heap::Space space) {
  return OneByteString::New(characters, len, space);
}


RawString* String::New(const uint16_t* characters,
                       intptr_t len,
                       Heap::Space space) {
  bool is_one_byte_string = true;
  for (intptr_t i = 0; i < len; ++i) {
    if (characters[i] > 0xFF) {
      is_one_byte_string = false;
      break;
    }
  }
  if (is_one_byte_string) {
    return OneByteString::New(characters, len, space);
  }
  return TwoByteString::New(characters, len, space);
}


RawString* String::New(const uint32_t* characters,
                       intptr_t len,
                       Heap::Space space) {
  bool is_one_byte_string = true;
  bool is_two_byte_string = true;
  for (intptr_t i = 0; i < len; ++i) {
    if (characters[i] > 0xFFFF) {
      is_two_byte_string = false;
      is_one_byte_string = false;
      break;
    } else if (characters[i] > 0xFF) {
      is_one_byte_string = false;
    }
  }
  if (is_one_byte_string) {
    return OneByteString::New(characters, len, space);
  } else if (is_two_byte_string) {
    return TwoByteString::New(characters, len, space);
  }
  return FourByteString::New(characters, len, space);
}


RawString* String::New(const String& str, Heap::Space space) {
  // Currently this just creates a copy of the string in the correct space.
  // Once we have external string support, this will also create a heap copy of
  // the string if necessary. Some optimizations are possible, such as not
  // copying internal strings into the same space.
  if (str.IsOneByteString()) {
    OneByteString& one_byte_str = OneByteString::Handle();
    one_byte_str ^= str.raw();
    return OneByteString::New(one_byte_str, space);
  } else if (str.IsTwoByteString()) {
    TwoByteString& two_byte_str = TwoByteString::Handle();
    two_byte_str ^= str.raw();
    return TwoByteString::New(two_byte_str, space);
  }
  ASSERT(str.IsFourByteString());
  FourByteString& four_byte_str = FourByteString::Handle();
  four_byte_str ^= str.raw();
  return FourByteString::New(four_byte_str, space);
}


void String::Copy(const String& dst, intptr_t dst_offset,
                  const uint8_t* characters,
                  intptr_t len) {
  ASSERT(dst_offset >= 0);
  ASSERT(len >= 0);
  ASSERT(len <= (dst.Length() - dst_offset));
  if (dst.IsOneByteString()) {
    OneByteString& onestr = OneByteString::Handle();
    onestr ^= dst.raw();
    NoGCScope no_gc;
    if (len > 0) {
      memmove(onestr.CharAddr(dst_offset), characters, len);
    }
  } else if (dst.IsTwoByteString()) {
    TwoByteString& twostr = TwoByteString::Handle();
    twostr ^= dst.raw();
    NoGCScope no_gc;
    for (intptr_t i = 0; i < len; ++i) {
      *twostr.CharAddr(i + dst_offset) = characters[i];
    }
  } else {
    ASSERT(dst.IsFourByteString());
    FourByteString& fourstr = FourByteString::Handle();
    fourstr ^= dst.raw();
    NoGCScope no_gc;
    for (intptr_t i = 0; i < len; ++i) {
      *fourstr.CharAddr(i + dst_offset) = characters[i];
    }
  }
}


void String::Copy(const String& dst, intptr_t dst_offset,
                  const uint16_t* characters,
                  intptr_t len) {
  ASSERT(dst_offset >= 0);
  ASSERT(len >= 0);
  ASSERT(len <= (dst.Length() - dst_offset));
  if (dst.IsOneByteString()) {
    OneByteString& onestr = OneByteString::Handle();
    onestr ^= dst.raw();
    NoGCScope no_gc;
    for (intptr_t i = 0; i < len; ++i) {
      ASSERT(characters[i] <= 0xFF);
      *onestr.CharAddr(i + dst_offset) = characters[i];
    }
  } else if (dst.IsTwoByteString()) {
    TwoByteString& twostr = TwoByteString::Handle();
    twostr ^= dst.raw();
    NoGCScope no_gc;
    if (len > 0) {
      memmove(twostr.CharAddr(dst_offset), characters, len * 2);
    }
  } else {
    ASSERT(dst.IsFourByteString());
    FourByteString& fourstr = FourByteString::Handle();
    fourstr ^= dst.raw();
    NoGCScope no_gc;
    for (intptr_t i = 0; i < len; ++i) {
      *fourstr.CharAddr(i + dst_offset) = characters[i];
    }
  }
}


void String::Copy(const String& dst, intptr_t dst_offset,
                  const uint32_t* characters,
                  intptr_t len) {
  ASSERT(dst_offset >= 0);
  ASSERT(len >= 0);
  ASSERT(len <= (dst.Length() - dst_offset));
  if (dst.IsOneByteString()) {
    OneByteString& onestr = OneByteString::Handle();
    onestr ^= dst.raw();
    NoGCScope no_gc;
    for (intptr_t i = 0; i < len; ++i) {
      ASSERT(characters[i] <= 0xFF);
      *onestr.CharAddr(i + dst_offset) = characters[i];
    }
  } else if (dst.IsTwoByteString()) {
    TwoByteString& twostr = TwoByteString::Handle();
    twostr ^= dst.raw();
    NoGCScope no_gc;
    for (intptr_t i = 0; i < len; ++i) {
      ASSERT(characters[i] <= 0xFFFF);
      *twostr.CharAddr(i + dst_offset) = characters[i];
    }
  } else {
    ASSERT(dst.IsFourByteString());
    FourByteString& fourstr = FourByteString::Handle();
    fourstr ^= dst.raw();
    NoGCScope no_gc;
    if (len > 0) {
      memmove(fourstr.CharAddr(dst_offset), characters, len * 4);
    }
  }
}


void String::Copy(const String& dst, intptr_t dst_offset,
                  const String& src, intptr_t src_offset,
                  intptr_t len) {
  ASSERT(dst_offset >= 0);
  ASSERT(src_offset >= 0);
  ASSERT(len >= 0);
  ASSERT(len <= (dst.Length() - dst_offset));
  ASSERT(len <= (src.Length() - src_offset));
  if (len > 0) {
    if (src.IsOneByteString()) {
      OneByteString& onestr = OneByteString::Handle();
      onestr ^= src.raw();
      NoGCScope no_gc;
      String::Copy(dst, dst_offset, onestr.CharAddr(0) + src_offset, len);
    } else if (src.IsTwoByteString()) {
      TwoByteString& twostr = TwoByteString::Handle();
      twostr ^= src.raw();
      NoGCScope no_gc;
      String::Copy(dst, dst_offset, twostr.CharAddr(0) + src_offset, len);
    } else {
      ASSERT(src.IsFourByteString());
      FourByteString& fourstr = FourByteString::Handle();
      fourstr ^= src.raw();
      NoGCScope no_gc;
      String::Copy(dst, dst_offset, fourstr.CharAddr(0) + src_offset, len);
    }
  }
}


static void GrowSymbolTable(const Array& symbol_table, intptr_t table_size) {
  // TODO(iposva): Avoid exponential growth.
  intptr_t new_table_size = table_size * 2;
  Array& new_symbol_table = Array::Handle(Array::New(new_table_size + 1));
  // Copy all elements from the original symbol table to the newly allocated
  // array.
  String& element = String::Handle();
  Object& new_element = Object::Handle();
  for (intptr_t i = 0; i < table_size; i++) {
    element ^= symbol_table.At(i);
    if (!element.IsNull()) {
      intptr_t hash = element.Hash();
      intptr_t index = hash % new_table_size;
      new_element = new_symbol_table.At(index);
      while (!new_element.IsNull()) {
        index = (index + 1) % new_table_size;  // Move to next element.
        new_element = new_symbol_table.At(index);
      }
      new_symbol_table.SetAt(index, element);
    }
  }
  // Copy used count.
  new_element = symbol_table.At(table_size);
  new_symbol_table.SetAt(new_table_size, new_element);
  // Remember the new symbol table now.
  Isolate::Current()->object_store()->set_symbol_table(new_symbol_table);
}


static void InsertIntoSymbolTable(const Array& symbol_table,
                                  const String& symbol,
                                  intptr_t index,
                                  intptr_t table_size) {
  symbol_table.SetAt(index, symbol);  // Remember the new symbol.
  Smi& used = Smi::Handle();
  used ^= symbol_table.At(table_size);
  intptr_t used_elements = used.Value() + 1;  // One more element added.
  used = Smi::New(used_elements);
  symbol_table.SetAt(table_size, used);  // Update used count.

  // Rehash if symbol_table is 75% full.
  if (used_elements > ((table_size / 4) * 3)) {
    GrowSymbolTable(symbol_table, table_size);
  }
}


RawString* String::NewSymbol(const char* str) {
  intptr_t width = 0;
  intptr_t len = Utf8::CodePointCount(str, &width);
  intptr_t size = len * width;
  Zone* zone = Isolate::Current()->current_zone();
  if (len == 0) {
    return String::NewSymbol(reinterpret_cast<uint8_t*>(NULL), 0);
  } else if (width == 1) {
    uint8_t* characters = reinterpret_cast<uint8_t*>(zone->Allocate(size));
    Utf8::Decode(str, characters, len);
    return NewSymbol(characters, len);
  } else if (width == 2) {
    uint16_t* characters = reinterpret_cast<uint16_t*>(zone->Allocate(size));
    Utf8::Decode(str, characters, len);
    return NewSymbol(characters, len);
  }
  ASSERT(width == 4);
  uint32_t* characters = reinterpret_cast<uint32_t*>(zone->Allocate(size));
  Utf8::Decode(str, characters, len);
  return NewSymbol(characters, len);
}


template<typename T>
RawString* String::NewSymbol(const T* characters, intptr_t len) {
  Isolate* isolate = Isolate::Current();

  // Calculate the String hash for this sequence of characters.
  intptr_t hash = Hash(characters, len);

  const Array& symbol_table =
      Array::Handle(isolate->object_store()->symbol_table());
  // Last element of the array is the number of used elements.
  intptr_t table_size = symbol_table.Length() - 1;
  intptr_t index = hash % table_size;

  String& symbol = String::Handle();
  symbol ^= symbol_table.At(index);
  while (!symbol.IsNull() && !symbol.Equals(characters, len)) {
    index = (index + 1) % table_size;  // Move to next element.
    symbol ^= symbol_table.At(index);
  }
  // Since we leave enough room in the table to guarantee, that we find an
  // empty spot, index is the insertion point if symbol is null.
  if (symbol.IsNull()) {
    // Allocate new result string.
    symbol = String::New(characters, len, Heap::kOld);
    symbol.SetHash(hash);  // Remember the calculated hash value.
    InsertIntoSymbolTable(symbol_table, symbol, index, table_size);
  }
  ASSERT(symbol.IsSymbol());
  return symbol.raw();
}

template RawString* String::NewSymbol(const uint8_t* characters, intptr_t len);
template RawString* String::NewSymbol(const uint16_t* characters, intptr_t len);
template RawString* String::NewSymbol(const uint32_t* characters, intptr_t len);


RawString* String::NewSymbol(const String& str) {
  return NewSymbol(str, 0, str.Length());
}


RawString* String::NewSymbol(const String& str,
                             intptr_t begin_index,
                             intptr_t len) {
  ASSERT(begin_index >= 0);
  ASSERT(len >= 0);
  ASSERT((begin_index + len) <= str.Length());
  Isolate* isolate = Isolate::Current();

  // Calculate the String hash for this sequence of characters.
  intptr_t hash = String::Hash(str, begin_index, len);

  const Array& symbol_table =
      Array::Handle(isolate->object_store()->symbol_table());
  // Last element of the array is the number of used elements.
  intptr_t table_size = symbol_table.Length() - 1;
  intptr_t index = hash % table_size;

  String& symbol = String::Handle();
  symbol ^= symbol_table.At(index);
  while (!symbol.IsNull() && !symbol.Equals(str, begin_index, len)) {
    index = (index + 1) % table_size;  // Move to next element.
    symbol ^= symbol_table.At(index);
  }
  // Since we leave enough room in the table to guarantee, that we find an
  // empty spot, index is the insertion point if symbol is null.
  if (symbol.IsNull()) {
    if (str.IsOld() && begin_index == 0 && len == str.Length()) {
      // Reuse the incoming str as the symbol value.
      symbol = str.raw();
    } else {
      // Allocate a copy in old space.
      symbol = String::SubString(str, begin_index, len, Heap::kOld);
    }
    symbol.SetHash(hash);
    InsertIntoSymbolTable(symbol_table, symbol, index, table_size);
  }
  ASSERT(symbol.IsSymbol());
  return symbol.raw();
}


RawString* String::Concat(const String& str1,
                          const String& str2,
                          Heap::Space space) {
  ASSERT(!str1.IsNull() && !str2.IsNull());
  bool is_two_byte_string = false;
  bool is_four_byte_string = false;
  if (str1.IsFourByteString() || str2.IsFourByteString()) {
    is_four_byte_string = true;
  } else if (str1.IsTwoByteString() || str2.IsTwoByteString()) {
    is_two_byte_string = true;
  }
  if (is_four_byte_string) {
    return FourByteString::Concat(str1, str2, space);
  } else if (is_two_byte_string) {
    return TwoByteString::Concat(str1, str2, space);
  }
  ASSERT(str1.IsOneByteString() && str2.IsOneByteString());
  return OneByteString::Concat(str1, str2, space);
}


RawString* String::ConcatAll(const Array& strings,
                             Heap::Space space) {
  ASSERT(!strings.IsNull());
  bool is_one_byte_string = true;
  bool is_two_byte_string = true;
  intptr_t result_len = 0;
  intptr_t strings_len = strings.Length();
  String& str = String::Handle();
  for (intptr_t i = 0; i < strings_len; i++) {
    str ^= strings.At(i);
    result_len += str.Length();
    if (str.IsFourByteString()) {
      is_one_byte_string = false;
      is_two_byte_string = false;
    } else if (str.IsTwoByteString()) {
      is_one_byte_string = false;
    }
  }
  if (is_one_byte_string) {
    return OneByteString::ConcatAll(strings, result_len, space);
  } else if (is_two_byte_string) {
    return TwoByteString::ConcatAll(strings, result_len, space);
  }
  return FourByteString::ConcatAll(strings, result_len, space);
}


RawString* String::SubString(const String& str,
                             intptr_t begin_index,
                             Heap::Space space) {
  ASSERT(!str.IsNull());
  if (begin_index >= str.Length()) {
    return String::null();
  }
  return String::SubString(str, begin_index, (str.Length() - begin_index));
}


RawString* String::SubString(const String& str,
                             intptr_t begin_index,
                             intptr_t length,
                             Heap::Space space) {
  ASSERT(!str.IsNull());
  ASSERT(begin_index >= 0);
  ASSERT(length >= 0);
  if (begin_index >= str.Length()) {
    return String::null();
  }
  if (str.IsOneByteString()) {
    OneByteString& obstr = OneByteString::Handle();
    obstr ^= str.raw();
    return OneByteString::SubString(obstr, begin_index, length, space);
  } else if (str.IsTwoByteString()) {
    TwoByteString& twostr = TwoByteString::Handle();
    twostr ^= str.raw();
    return TwoByteString::SubString(twostr, begin_index, length, space);
  }
  ASSERT(str.IsFourByteString());
  FourByteString& fourstr = FourByteString::Handle();
  fourstr ^= str.raw();
  return FourByteString::SubString(fourstr, begin_index, length, space);
}


const char* String::ToCString() const {
  intptr_t len = Utf8::Length(*this);
  Zone* zone = Isolate::Current()->current_zone();
  char* result = reinterpret_cast<char*>(zone->Allocate(len + 1));
  Utf8::Encode(*this, result, len);
  result[len] = 0;
  return result;
}


RawOneByteString* OneByteString::New(intptr_t len,
                                     Heap::Space space) {
  Isolate* isolate = Isolate::Current();

  const Class& cls =
      Class::Handle(isolate->object_store()->one_byte_string_class());
  OneByteString& result = OneByteString::Handle();
  {
    RawObject* raw = Object::Allocate(cls,
                                      OneByteString::InstanceSize(len),
                                      space);
    NoGCScope no_gc;
    result ^= raw;
    result.SetLength(len);
    result.SetHash(0);
  }
  return result.raw();
}


RawOneByteString* OneByteString::New(const uint8_t* characters,
                                     intptr_t len,
                                     Heap::Space space) {
  OneByteString& result = OneByteString::Handle(OneByteString::New(len, space));
  String::Copy(result, 0, characters, len);
  return result.raw();
}


RawOneByteString* OneByteString::New(const uint16_t* characters,
                                     intptr_t len,
                                     Heap::Space space) {
  OneByteString& result = OneByteString::Handle(OneByteString::New(len, space));
  String::Copy(result, 0, characters, len);
  return result.raw();
}


RawOneByteString* OneByteString::New(const uint32_t* characters,
                                     intptr_t len,
                                     Heap::Space space) {
  OneByteString& result = OneByteString::Handle(OneByteString::New(len, space));
  String::Copy(result, 0, characters, len);
  return result.raw();
}


RawOneByteString* OneByteString::New(const OneByteString& str,
                                     Heap::Space space) {
  intptr_t len = str.Length();
  OneByteString& result = OneByteString::Handle(OneByteString::New(len, space));
  String::Copy(result, 0, str, 0, len);
  return result.raw();
}


RawOneByteString* OneByteString::Concat(const String& str1,
                                        const String& str2,
                                        Heap::Space space) {
  intptr_t len1 = str1.Length();
  intptr_t len2 = str2.Length();
  intptr_t len = len1 + len2;
  OneByteString& result = OneByteString::Handle(OneByteString::New(len, space));
  String::Copy(result, 0, str1, 0, len1);
  String::Copy(result, len1, str2, 0, len2);
  return result.raw();
}


RawOneByteString* OneByteString::ConcatAll(const Array& strings,
                                           intptr_t len,
                                           Heap::Space space) {
  OneByteString& result = OneByteString::Handle(OneByteString::New(len, space));
  OneByteString& str = OneByteString::Handle();
  intptr_t strings_len = strings.Length();
  intptr_t pos = 0;
  for (intptr_t i = 0; i < strings_len; i++) {
    str ^= strings.At(i);
    intptr_t str_len = str.Length();
    String::Copy(result, pos, str, 0, str_len);
    pos += str_len;
  }
  return result.raw();
}


RawOneByteString* OneByteString::SubString(const OneByteString& str,
                                           intptr_t begin_index,
                                           intptr_t length,
                                           Heap::Space space) {
  ASSERT(!str.IsNull());
  ASSERT(begin_index < str.Length());
  OneByteString& result = OneByteString::Handle();
  if (length <= (str.Length() - begin_index)) {
    result ^= OneByteString::New(length, space);
    String::Copy(result, 0, str, begin_index, length);
  }
  // TODO(5418937): return a non-null object on error.
  return result.raw();
}


const char* OneByteString::ToCString() const {
  return String::ToCString();
}


RawTwoByteString* TwoByteString::New(intptr_t len,
                                     Heap::Space space) {
  Isolate* isolate = Isolate::Current();

  const Class& cls =
      Class::Handle(isolate->object_store()->two_byte_string_class());
  TwoByteString& result = TwoByteString::Handle();
  {
    RawObject* raw = Object::Allocate(cls,
                                      TwoByteString::InstanceSize(len),
                                      space);
    NoGCScope no_gc;
    result ^= raw;
    result.SetLength(len);
    result.SetHash(0);
  }
  return result.raw();
}


RawTwoByteString* TwoByteString::New(const uint16_t* characters,
                                     intptr_t len,
                                     Heap::Space space) {
  TwoByteString& result = TwoByteString::Handle(TwoByteString::New(len, space));
  String::Copy(result, 0, characters, len);
  return result.raw();
}


RawTwoByteString* TwoByteString::New(const uint32_t* characters,
                                     intptr_t len,
                                     Heap::Space space) {
  TwoByteString& result = TwoByteString::Handle(TwoByteString::New(len, space));
  String::Copy(result, 0, characters, len);
  return result.raw();
}


RawTwoByteString* TwoByteString::New(const TwoByteString& str,
                                     Heap::Space space) {
  intptr_t len = str.Length();
  TwoByteString& result = TwoByteString::Handle(TwoByteString::New(len, space));
  String::Copy(result, 0, str, 0, len);
  return result.raw();
}


RawTwoByteString* TwoByteString::Concat(const String& str1,
                                        const String& str2,
                                        Heap::Space space) {
  TwoByteString& result = TwoByteString::Handle();
  intptr_t len1 = str1.Length();
  intptr_t len2 = str2.Length();
  intptr_t len = len1 + len2;
  result ^= TwoByteString::New(len, space);
  String::Copy(result, 0, str1, 0, len1);
  String::Copy(result, len1, str2, 0, len2);
  return result.raw();
}


RawTwoByteString* TwoByteString::ConcatAll(const Array& strings,
                                           intptr_t len,
                                           Heap::Space space) {
  TwoByteString& result = TwoByteString::Handle(TwoByteString::New(len, space));
  String& str = String::Handle();
  intptr_t strings_len = strings.Length();
  intptr_t pos = 0;
  for (intptr_t i = 0; i < strings_len; i++) {
    str ^= strings.At(i);
    intptr_t str_len = str.Length();
    String::Copy(result, pos, str, 0, str_len);
    pos += str_len;
  }
  return result.raw();
}


RawTwoByteString* TwoByteString::SubString(const TwoByteString& str,
                                           intptr_t begin_index,
                                           intptr_t length,
                                           Heap::Space space) {
  ASSERT(!str.IsNull());
  ASSERT(begin_index < str.Length());
  TwoByteString& result = TwoByteString::Handle();
  if (length <= (str.Length() - begin_index)) {
    result ^= TwoByteString::New(length, space);
    String::Copy(result, 0, str, begin_index, length);
  }
  // TODO(5418937): return a non-null object on error.
  return result.raw();
}


const char* TwoByteString::ToCString() const {
  return String::ToCString();
}



RawFourByteString* FourByteString::New(intptr_t len,
                                       Heap::Space space) {
  Isolate* isolate = Isolate::Current();

  const Class& cls =
      Class::Handle(isolate->object_store()->four_byte_string_class());
  FourByteString& result = FourByteString::Handle();
  {
    RawObject* raw = Object::Allocate(cls,
                                      FourByteString::InstanceSize(len),
                                      space);
    NoGCScope no_gc;
    result ^= raw;
    result.SetLength(len);
    result.SetHash(0);
  }
  return result.raw();
}


RawFourByteString* FourByteString::New(const uint32_t* characters,
                                       intptr_t len,
                                       Heap::Space space) {
  FourByteString& result =
      FourByteString::Handle(FourByteString::New(len, space));
  String::Copy(result, 0, characters, len);
  return result.raw();
}


RawFourByteString* FourByteString::New(const FourByteString& str,
                                       Heap::Space space) {
  return FourByteString::New(str.CharAddr(0), str.Length(), space);
}


RawFourByteString* FourByteString::Concat(const String& str1,
                                          const String& str2,
                                          Heap::Space space) {
  intptr_t len1 = str1.Length();
  intptr_t len2 = str2.Length();
  intptr_t len = len1 + len2;
  FourByteString& result =
      FourByteString::Handle(FourByteString::New(len, space));
  String::Copy(result, 0, str1, 0, len1);
  String::Copy(result, len1, str2, 0, len2);
  return result.raw();
}


RawFourByteString* FourByteString::ConcatAll(const Array& strings,
                                             intptr_t len,
                                             Heap::Space space) {
  FourByteString& result =
      FourByteString::Handle(FourByteString::New(len, space));
  String& str = String::Handle();
  {
    intptr_t strings_len = strings.Length();
    intptr_t pos = 0;
    for (intptr_t i = 0; i < strings_len; i++) {
      str ^= strings.At(i);
      intptr_t str_len = str.Length();
      String::Copy(result, pos, str, 0, str_len);
      pos += str_len;
    }
  }
  return result.raw();
}


RawFourByteString* FourByteString::SubString(const FourByteString& str,
                                             intptr_t begin_index,
                                             intptr_t length,
                                             Heap::Space space) {
  ASSERT(!str.IsNull());
  ASSERT(begin_index < str.Length());
  FourByteString& result = FourByteString::Handle();
  if (length <= (str.Length() - begin_index)) {
    result ^= FourByteString::New(length, space);
    String::Copy(result, 0, str, begin_index, length);
  }
  // TODO(5418937): return a non-null object on error.
  return result.raw();
}


const char* FourByteString::ToCString() const {
  return String::ToCString();
}


RawBool* Bool::True() {
  return Isolate::Current()->object_store()->true_value();
}


RawBool* Bool::False() {
  return Isolate::Current()->object_store()->false_value();
}


RawBool* Bool::New(bool value) {
  Isolate* isolate = Isolate::Current();

  const Class& cls = Class::Handle(isolate->object_store()->bool_class());
  Bool& result = Bool::Handle();
  {
    // Since the two boolean instances are singletons we allocate them straight
    // in the old generation.
    RawObject* raw = Object::Allocate(cls, Bool::InstanceSize(), Heap::kOld);
    NoGCScope no_gc;
    result ^= raw;
  }
  result.set_value(value);
  return result.raw();
}


const char* Bool::ToCString() const {
  return value() ? "true" : "false";
}


bool Array::Equals(const Instance& other) const {
  if (this->raw() == other.raw()) {
    // Both handles point to the same raw instance.
    return true;
  }

  if (!other.IsArray() || other.IsNull()) {
    return false;
  }

  Array& other_arr = Array::Handle();
  other_arr ^= other.raw();

  intptr_t len = this->Length();
  if (len != other_arr.Length()) {
    return false;
  }

  for (intptr_t i = 0; i < len; i++) {
    if (this->At(i) != other_arr.At(i)) {
      return false;
    }
  }
  return true;
}


RawArray* Array::New(word len, bool immutable, Heap::Space space) {
  if ((len < 0) || (len > kMaxArrayElements)) {
    // TODO(iposva): Should we throw an illegal parameter exception?
    UNIMPLEMENTED();
    return null();
  }

  Isolate* isolate = Isolate::Current();
  Class& cls = Class::Handle();
  if (immutable) {
    cls = isolate->object_store()->immutable_array_class();
  } else {
    cls = isolate->object_store()->array_class();
  }
  Array& result = Array::Handle();
  {
    RawObject* raw = Object::Allocate(cls,
                                      Array::InstanceSize(len),
                                      space);
    NoGCScope no_gc;
    result ^= raw;
    result.SetLength(len);
    for (intptr_t i = 0; i < len; i++) {
      *result.ObjectAddr(i) = Object::null();
    }
  }
  return result.raw();
}


void Array::MakeImmutable() const {
  Isolate* isolate = Isolate::Current();
  raw()->ptr()->class_ = isolate->object_store()->immutable_array_class();
}


const char* Array::ToCString() const {
  return "Array";
}


RawArray* Array::Grow(const Array& source, int new_length, Heap::Space space) {
  intptr_t len = source.IsNull() ? 0 : source.Length();
  ASSERT(new_length >= len);  // Cannot copy 'source' into new array.
  ASSERT(new_length != len);  // Unnecessary copying of array.
  const Array& result = Array::Handle(Array::New(new_length, space));
  Object& obj = Object::Handle();
  for (int i = 0; i < len; i++) {
    obj = source.At(i);
    result.SetAt(i, obj);
  }
  return result.raw();
}


RawArray* Array::Empty() {
  return Isolate::Current()->object_store()->empty_array();
}


const char* ImmutableArray::ToCString() const {
  return "ImmutableArray";
}


RawClosure* Closure::New(const Function& function,
                         const Context& context,
                         Heap::Space space) {
  Isolate* isolate = Isolate::Current();
  ASSERT(context.isolate() == isolate);

  const Class& cls = Class::Handle(function.signature_class());
  Closure& result = Closure::Handle();
  {
    RawObject* raw = Object::Allocate(cls, Closure::InstanceSize(), space);
    NoGCScope no_gc;
    result ^= raw;
  }
  result.set_function(function);
  result.set_context(context);
  return result.raw();
}


void Closure::set_context(const Context& value) const {
  raw_ptr()->context_ = value.raw();
}


void Closure::set_function(const Function& value) const {
  raw_ptr()->function_ = value.raw();
}


const char* Closure::ToCString() const {
  return "Closure";
}


intptr_t Stacktrace::Length() const {
  const Array& code_array = Array::Handle(raw_ptr()->code_array_);
  return code_array.Length();
}


RawFunction* Stacktrace::FunctionAtFrame(intptr_t frame_index) const {
  const Array& function_array = Array::Handle(raw_ptr()->function_array_);
  return reinterpret_cast<RawFunction*>(function_array.At(frame_index));
}


RawCode* Stacktrace::CodeAtFrame(intptr_t frame_index) const {
  const Array& code_array = Array::Handle(raw_ptr()->code_array_);
  return reinterpret_cast<RawCode*>(code_array.At(frame_index));
}


RawSmi* Stacktrace::PcOffsetAtFrame(intptr_t frame_index) const {
  const Array& pc_offset_array = Array::Handle(raw_ptr()->pc_offset_array_);
  return reinterpret_cast<RawSmi*>(pc_offset_array.At(frame_index));
}


void Stacktrace::set_function_array(const Array& function_array) const {
  StorePointer(&raw_ptr()->function_array_, function_array.raw());
}


void Stacktrace::set_code_array(const Array& code_array) const {
  StorePointer(&raw_ptr()->code_array_, code_array.raw());
}


void Stacktrace::set_pc_offset_array(const Array& pc_offset_array) const {
  StorePointer(&raw_ptr()->pc_offset_array_, pc_offset_array.raw());
}


void Stacktrace::SetupStacktrace(intptr_t index,
                                 const GrowableArray<uword>& frame_pcs) const {
  ASSERT(Isolate::Current() != NULL);
  CodeIndexTable* code_index_table = Isolate::Current()->code_index_table();
  ASSERT(code_index_table != NULL);
  Function& function = Function::Handle();
  Code& code = Code::Handle();
  Smi& pc_offset = Smi::Handle();
  const Array& function_array = Array::Handle(raw_ptr()->function_array_);
  const Array& code_array = Array::Handle(raw_ptr()->code_array_);
  const Array& pc_offset_array = Array::Handle(raw_ptr()->pc_offset_array_);
  for (intptr_t i = 0; i < frame_pcs.length(); i++) {
    function = code_index_table->LookupFunction(frame_pcs[i]);
    function_array.SetAt((index + i), function);
    code = function.code();
    code_array.SetAt((index + i), code);
    pc_offset = Smi::New(frame_pcs[i] - code.EntryPoint());
    pc_offset_array.SetAt((index + i), pc_offset);
  }
}


RawStacktrace* Stacktrace::New(const GrowableArray<uword>& stack_frame_pcs,
                               Heap::Space space) {
  const Class& cls = Class::Handle(
      Isolate::Current()->object_store()->stacktrace_class());
  Stacktrace& result = Stacktrace::Handle();
  {
    RawObject* raw = Object::Allocate(cls,
                                      Stacktrace::InstanceSize(),
                                      space);
    NoGCScope no_gc;
    result ^= raw;
  }
  intptr_t length = stack_frame_pcs.length();
  // Create arrays for the function, code and pc_offset triplet for each frame.
  const Array& function_array = Array::Handle(Array::New(length));
  const Array& code_array = Array::Handle(Array::New(length));
  const Array& pc_offset_array = Array::Handle(Array::New(length));
  result.set_function_array(function_array);
  result.set_code_array(code_array);
  result.set_pc_offset_array(pc_offset_array);
  // Now populate the arrays with appropriate values from each frame.
  result.SetupStacktrace(0, stack_frame_pcs);
  return result.raw();
}


void Stacktrace::Append(const GrowableArray<uword>& stack_frame_pcs) const {
  intptr_t old_length = Length();
  intptr_t new_length = old_length + stack_frame_pcs.length();

  // Grow the arrays for function, code and pc_offset triplet to accommodate
  // the new stack frames.
  Array& function_array = Array::Handle(raw_ptr()->function_array_);
  Array& code_array = Array::Handle(raw_ptr()->code_array_);
  Array& pc_offset_array = Array::Handle(raw_ptr()->pc_offset_array_);
  function_array = Array::Grow(function_array, new_length);
  code_array = Array::Grow(code_array, new_length);
  pc_offset_array = Array::Grow(pc_offset_array, new_length);
  set_function_array(function_array);
  set_code_array(code_array);
  set_pc_offset_array(pc_offset_array);
  // Now populate the arrays with appropriate values from each new frame.
  SetupStacktrace(old_length, stack_frame_pcs);
}


const char* Stacktrace::ToCStringInternal(bool verbose) const {
  Function& function = Function::Handle();
  Code& code = Code::Handle();
  Class& function_class = Class::Handle();
  Script& script = Script::Handle();
  String& function_name = String::Handle();
  String& class_name = String::Handle();
  String& url = String::Handle();

  // Iterate through the stack frames and create C string description
  // for each frame.
  intptr_t total_len = 0;
  const char* kFormat = verbose ?
      " %d. Function: '%s%s%s' url: '%s' line:%d col:%d code-entry: 0x%x\n" :
      " %d. Function: '%s%s%s' url: '%s' line:%d col:%d\n";
  GrowableArray<char*> frame_strings;
  for (intptr_t i = 0; i < Length(); i++) {
    function = FunctionAtFrame(i);
    code = CodeAtFrame(i);
    uword pc = code.EntryPoint() + Smi::Value(PcOffsetAtFrame(i));
    intptr_t token_index = code.GetTokenIndexOfPC(pc);
    function_class = function.owner();
    script = function_class.script();
    function_name = function.name();
    class_name = function_class.Name();
    url = script.url();
    intptr_t line = -1;
    intptr_t column = -1;
    if (token_index >= 0) {
      script.GetTokenLocation(token_index, &line, &column);
    }
    intptr_t len = OS::SNPrint(NULL, 0, kFormat,
                               i,
                               class_name.ToCString(),
                               function_class.IsTopLevel() ? "" : ".",
                               function_name.ToCString(),
                               url.ToCString(),
                               line, column,
                               code.EntryPoint());
    total_len += len;
    char* chars = reinterpret_cast<char*>(
        Isolate::Current()->current_zone()->Allocate(len + 1));
    OS::SNPrint(chars, (len + 1), kFormat,
                i,
                class_name.ToCString(),
                function_class.IsTopLevel() ? "" : ".",
                function_name.ToCString(),
                url.ToCString(),
                line, column,
                code.EntryPoint());
    frame_strings.Add(chars);
  }

  // Now concatentate the frame descriptions into a single C string.
  char* chars = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(total_len + 1));
  intptr_t index = 0;
  for (intptr_t i = 0; i < frame_strings.length(); i++) {
    index += OS::SNPrint((chars + index),
                         (total_len + 1 - index),
                         "%s",
                         frame_strings[i]);
  }
  return chars;
}


const char* Stacktrace::ToCString() const {
  return ToCStringInternal(false);
}


void JSRegExp::set_pattern(const String& pattern) const {
  StorePointer(&raw_ptr()->pattern_, pattern.raw());
}


void JSRegExp::set_num_bracket_expressions(intptr_t value) const {
  raw_ptr()->num_bracket_expressions_ = Smi::New(value);
}


RawJSRegExp* JSRegExp::New(intptr_t len, Heap::Space space) {
  const Class& cls = Class::Handle(
      Isolate::Current()->object_store()->jsregexp_class());
  JSRegExp& result = JSRegExp::Handle();
  {
    RawObject* raw = Object::Allocate(cls,
                                      JSRegExp::InstanceSize(len),
                                      space);
    NoGCScope no_gc;
    result ^= raw;
    result.set_type(kUnitialized);
    result.set_flags(0);
    result.SetLength(len);
  }
  return result.raw();
}


void* JSRegExp::GetDataStartAddress() const {
  intptr_t addr = reinterpret_cast<intptr_t>(raw_ptr());
  return reinterpret_cast<void*>(addr + sizeof(RawJSRegExp));
}


RawJSRegExp* JSRegExp::FromDataStartAddress(void* data) {
  JSRegExp& regexp = JSRegExp::Handle();
  intptr_t addr = reinterpret_cast<intptr_t>(data) - sizeof(RawJSRegExp);
  regexp ^= RawObject::FromAddr(addr);
  return regexp.raw();
}


const char* JSRegExp::Flags() const {
  switch (raw_ptr()->flags_) {
    case kGlobal | kIgnoreCase | kMultiLine :
    case kIgnoreCase | kMultiLine :
      return "im";
    case kGlobal | kIgnoreCase :
    case kIgnoreCase:
      return "i";
    case kGlobal | kMultiLine :
    case kMultiLine:
      return "m";
    default:
      break;
  }
  return "";
}


bool JSRegExp::Equals(const Instance& other) const {
  if (this->raw() == other.raw()) {
    return true;  // "===".
  }
  if (other.IsNull() || !other.IsJSRegExp()) {
    return false;
  }
  JSRegExp& other_js = JSRegExp::Handle();
  other_js ^= other.raw();
  // Match the pattern.
  const String& str1 = String::Handle(pattern());
  const String& str2 = String::Handle(other_js.pattern());
  if (!str1.Equals(str2)) {
    return false;
  }
  // Match the flags.
  if ((is_global() != other_js.is_global()) ||
      (is_ignore_case() != other_js.is_ignore_case()) ||
      (is_multi_line() != other_js.is_multi_line())) {
    return false;
  }
  return true;
}


const char* JSRegExp::ToCString() const {
  const String& str = String::Handle(pattern());
  const char* format = "JSRegExp: pattern=%s flags=%s";
  intptr_t len = OS::SNPrint(NULL, 0, format, str.ToCString(), Flags());
  char* chars = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(len + 1));
  OS::SNPrint(chars, (len + 1), format, str.ToCString(), Flags());
  return chars;
}

}  // namespace dart
