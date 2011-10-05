// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef VM_OBJECT_STORE_H_
#define VM_OBJECT_STORE_H_

#include "vm/object.h"

namespace dart {

// Forward declarations.
class Isolate;
class ObjectPointerVisitor;

// The object store is a per isolate instance which stores references to
// objects used by the VM.
// TODO(iposva): Move the actual store into the object heap for quick handling
// by snapshots eventually.
class ObjectStore {
 public:
  // Index for objects/types/classes stored in the object store,
  // this index is used in snapshots to refer to classes or objects directly.
  enum {
    kTrueValue = Object::kMaxId,
    kFalseValue,
    kObjectType,
    kNullType,
    kVarType,
    kVoidType,
    kFunctionInterface,
    kNumberInterface,
    kIntInterface,
    kBoolInterface,
    kObjectClass,
    kSmiClass,
    kMintClass,
    kBigintClass,
    kDoubleClass,
    kOneByteStringClass,
    kTwoByteStringClass,
    kFourByteStringClass,
    kBoolClass,
    kArrayClass,
    kImmutableArrayClass,
    kUnhandledExceptionClass,
    kStacktraceClass,
    kJSRegExpClass,
    kMaxId,
    kInvalidIndex = -1,
  };

  ~ObjectStore();

  RawClass* object_class() const { return object_class_; }
  void set_object_class(const Class& value) { object_class_ = value.raw(); }
  static intptr_t object_class_offset() {
    return OFFSET_OF(ObjectStore, object_class_);
  }

  RawType* object_type() const { return object_type_; }
  void set_object_type(const Type& value) { object_type_ = value.raw(); }

  RawType* null_type() const { return null_type_; }
  void set_null_type(const Type& value) { null_type_ = value.raw(); }

  RawType* var_type() const { return var_type_; }
  void set_var_type(const Type& value) { var_type_ = value.raw(); }

  RawType* void_type() const { return void_type_; }
  void set_void_type(const Type& value) { void_type_ = value.raw(); }

  RawType* function_interface() const { return function_interface_; }
  void set_function_interface(const Type& value) {
    function_interface_ = value.raw();
  }

  RawType* number_interface() const { return number_interface_; }
  void set_number_interface(const Type& value) {
    number_interface_ = value.raw();
  }

  RawType* int_interface() const { return int_interface_; }
  void set_int_interface(const Type& value) { int_interface_ = value.raw(); }

  RawClass* smi_class() const { return smi_class_; }
  void set_smi_class(const Class& value) { smi_class_ = value.raw(); }
  static intptr_t smi_class_offset() {
    return OFFSET_OF(ObjectStore, smi_class_);
  }

  RawClass* double_class() const { return double_class_; }
  void set_double_class(const Class& value) { double_class_ = value.raw(); }

  RawClass* mint_class() const { return mint_class_; }
  void set_mint_class(const Class& value) { mint_class_ = value.raw(); }

  RawClass* bigint_class() const { return bigint_class_; }
  void set_bigint_class(const Class& value) { bigint_class_ = value.raw(); }

  RawClass* one_byte_string_class() const { return one_byte_string_class_; }
  void set_one_byte_string_class(const Class& value) {
    one_byte_string_class_ = value.raw();
  }

  RawClass* two_byte_string_class() const { return two_byte_string_class_; }
  void set_two_byte_string_class(const Class& value) {
    two_byte_string_class_ = value.raw();
  }

  RawClass* four_byte_string_class() const { return four_byte_string_class_; }
  void set_four_byte_string_class(const Class& value) {
    four_byte_string_class_ = value.raw();
  }

  RawType* bool_interface() const { return bool_interface_; }
  void set_bool_interface(const Type& value) { bool_interface_ = value.raw(); }

  RawClass* bool_class() const { return bool_class_; }
  void set_bool_class(const Class& value) { bool_class_ = value.raw(); }

  RawClass* array_class() const { return array_class_; }
  void set_array_class(const Class& value) { array_class_ = value.raw(); }
  static intptr_t array_class_offset() {
    return OFFSET_OF(ObjectStore, array_class_);
  }

  RawClass* immutable_array_class() const { return immutable_array_class_; }
  void set_immutable_array_class(const Class& value) {
    immutable_array_class_ = value.raw();
  }

  RawClass* unhandled_exception_class() const {
    return unhandled_exception_class_;
  }
  void set_unhandled_exception_class(const Class& value) {
    unhandled_exception_class_ = value.raw();
  }
  static intptr_t unhandled_exception_class_offset() {
    return OFFSET_OF(ObjectStore, unhandled_exception_class_);
  }

  RawClass* stacktrace_class() const {
    return stacktrace_class_;
  }
  void set_stacktrace_class(const Class& value) {
    stacktrace_class_ = value.raw();
  }
  static intptr_t stacktrace_class_offset() {
    return OFFSET_OF(ObjectStore, stacktrace_class_);
  }

  RawClass* jsregexp_class() const {
    return jsregexp_class_;
  }
  void set_jsregexp_class(const Class& value) {
    jsregexp_class_ = value.raw();
  }
  static intptr_t jsregexp_class_offset() {
    return OFFSET_OF(ObjectStore, jsregexp_class_);
  }

  RawArray* symbol_table() const { return symbol_table_; }
  void set_symbol_table(const Array& value) { symbol_table_ = value.raw(); }

  RawLibrary* core_library() const { return core_library_; }
  void set_core_library(const Library& value) {
    core_library_ = value.raw();
  }

  RawLibrary* core_impl_library() const { return core_impl_library_; }
  void set_core_impl_library(const Library& value) {
    core_impl_library_ = value.raw();
  }

  RawLibrary* root_library() const { return root_library_; }
  void set_root_library(const Library& value) {
    root_library_ = value.raw();
  }

  // Returns head of list of registered libraries.
  RawLibrary* registered_libraries() const { return registered_libraries_; }
  void set_registered_libraries(const Library& value) {
    registered_libraries_ = value.raw();
  }

  RawArray* pending_classes() const { return pending_classes_; }
  void set_pending_classes(const Array& value) {
    ASSERT(!value.IsNull());
    pending_classes_ = value.raw();
  }

  RawString* sticky_error() const { return sticky_error_; }
  void set_sticky_error(const String& value) {
    ASSERT(!value.IsNull());
    sticky_error_ = value.raw();
  }

  RawBool* true_value() const { return true_value_; }
  void set_true_value(const Bool& value) { true_value_ = value.raw(); }

  RawBool* false_value() const { return false_value_; }
  void set_false_value(const Bool& value) { false_value_ = value.raw(); }

  RawArray* empty_array() const { return empty_array_; }
  void set_empty_array(const Array& value) { empty_array_ = value.raw(); }

  RawContext* empty_context() const { return empty_context_; }
  void set_empty_context(const Context& value) {
    empty_context_ = value.raw();
  }

  // Visit all object pointers.
  void VisitObjectPointers(ObjectPointerVisitor* visitor);

  RawClass* GetClass(int index);
  int GetClassIndex(const RawClass* raw_class);
  RawType* GetType(int index);
  int GetTypeIndex(const RawType* raw_type);

  static void Init(Isolate* isolate);

 private:
  ObjectStore();

  RawObject** from() { return reinterpret_cast<RawObject**>(&object_class_); }
  RawClass* object_class_;
  RawType* object_type_;
  RawType* null_type_;
  RawType* var_type_;
  RawType* void_type_;
  RawType* function_interface_;
  RawType* number_interface_;
  RawType* int_interface_;
  RawClass* smi_class_;
  RawClass* mint_class_;
  RawClass* bigint_class_;
  RawClass* double_class_;
  RawClass* one_byte_string_class_;
  RawClass* two_byte_string_class_;
  RawClass* four_byte_string_class_;
  RawType* bool_interface_;
  RawClass* bool_class_;
  RawClass* array_class_;
  RawClass* immutable_array_class_;
  RawClass* unhandled_exception_class_;
  RawClass* stacktrace_class_;
  RawClass* jsregexp_class_;
  RawBool* true_value_;
  RawBool* false_value_;
  RawArray* empty_array_;
  RawArray* symbol_table_;
  RawLibrary* core_library_;
  RawLibrary* core_impl_library_;
  RawLibrary* root_library_;
  RawLibrary* registered_libraries_;
  RawArray* pending_classes_;
  RawString* sticky_error_;
  RawContext* empty_context_;
  RawObject** to() { return reinterpret_cast<RawObject**>(&empty_context_); }

  friend class SnapshotReader;

  DISALLOW_COPY_AND_ASSIGN(ObjectStore);
};

}  // namespace dart

#endif  // VM_OBJECT_STORE_H_
