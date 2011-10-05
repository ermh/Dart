// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/raw_object.h"

#include "vm/object.h"
#include "vm/visitor.h"


namespace dart {

void RawObject::Validate() const {
  // All Smi values are valid.
  if (!IsHeapObject()) {
    return;
  }
  // Validate that the class_ field is sensible.
  RawClass* raw_class = ptr()->class_;
  ASSERT(raw_class->IsHeapObject());
  RawClass* raw_class_class = raw_class->ptr()->class_;
  ASSERT(raw_class_class->IsHeapObject());
  ASSERT(raw_class_class->ptr()->instance_kind_ == kClass);
}


intptr_t RawObject::Size() const {
  NoHandleScope no_handles;

  // Only reasonable to be called on heap objects.
  ASSERT(IsHeapObject());

  RawClass* raw_class = ptr()->class_;
  intptr_t instance_size = raw_class->ptr()->instance_size_;
  if (instance_size == 0) {
    switch (raw_class->ptr()->instance_kind_) {
      case kTokenStream: {
        const RawTokenStream* raw_tokens =
            reinterpret_cast<const RawTokenStream*>(this);
        intptr_t tokens_length = Smi::Value(raw_tokens->ptr()->length_);
        instance_size = TokenStream::InstanceSize(tokens_length);
        break;
      }
      case kCode: {
        const RawCode* raw_code = reinterpret_cast<const RawCode*>(this);
        intptr_t pointer_offsets_length =
            raw_code->ptr()->pointer_offsets_length_;
        instance_size = Code::InstanceSize(pointer_offsets_length);
        break;
      }
      case kInstructions: {
        const RawInstructions* raw_instructions =
            reinterpret_cast<const RawInstructions*>(this);
        intptr_t instructions_size = raw_instructions->ptr()->size_;
        instance_size = Instructions::InstanceSize(instructions_size);
        break;
      }
      case kContext: {
        const RawContext* raw_context =
            reinterpret_cast<const RawContext*>(this);
        intptr_t num_variables = raw_context->ptr()->num_variables_;
        instance_size = Context::InstanceSize(num_variables);
        break;
      }
      case kContextScope: {
        const RawContextScope* raw_context_scope =
            reinterpret_cast<const RawContextScope*>(this);
        intptr_t num_variables = raw_context_scope->ptr()->num_variables_;
        instance_size = ContextScope::InstanceSize(num_variables);
        break;
      }
      case kBigint: {
        const RawBigint* raw_bgi = reinterpret_cast<const RawBigint*>(this);
        const BIGNUM* bn_ptr = &raw_bgi->ptr()->bn_;
        instance_size = Bigint::InstanceSize(bn_ptr);
        break;
      }
      case kOneByteString: {
        const RawOneByteString* raw_string =
            reinterpret_cast<const RawOneByteString*>(this);
        intptr_t string_length = Smi::Value(raw_string->ptr()->length_);
        instance_size = OneByteString::InstanceSize(string_length);
        break;
      }
      case kTwoByteString: {
        const RawTwoByteString* raw_string =
            reinterpret_cast<const RawTwoByteString*>(this);
        intptr_t string_length = Smi::Value(raw_string->ptr()->length_);
        instance_size = TwoByteString::InstanceSize(string_length);
        break;
      }
      case kFourByteString: {
        const RawFourByteString* raw_string =
            reinterpret_cast<const RawFourByteString*>(this);
        intptr_t string_length = Smi::Value(raw_string->ptr()->length_);
        instance_size = FourByteString::InstanceSize(string_length);
        break;
      }
      case kArray:
      case kImmutableArray: {
        const RawArray* raw_array = reinterpret_cast<const RawArray*>(this);
        intptr_t array_length = Smi::Value(raw_array->ptr()->length_);
        instance_size = Array::InstanceSize(array_length);
        break;
      }
      case kPcDescriptors: {
        const RawPcDescriptors* raw_descriptors =
            reinterpret_cast<const RawPcDescriptors*>(this);
        intptr_t num_descriptors = Smi::Value(raw_descriptors->ptr()->length_);
        instance_size = PcDescriptors::InstanceSize(num_descriptors);
        break;
      }
      case kExceptionHandlers: {
        const RawExceptionHandlers* raw_handlers =
            reinterpret_cast<const RawExceptionHandlers*>(this);
        intptr_t num_handlers = Smi::Value(raw_handlers->ptr()->length_);
        instance_size = ExceptionHandlers::InstanceSize(num_handlers);
        break;
      }
      case kJSRegExp: {
        const RawJSRegExp* raw_jsregexp =
            reinterpret_cast<const RawJSRegExp*>(this);
        intptr_t data_length = Smi::Value(raw_jsregexp->ptr()->data_length_);
        instance_size = JSRegExp::InstanceSize(data_length);
        break;
      }
      default:
        UNREACHABLE();
        break;
    }
  }
  ASSERT(instance_size != 0);
  return instance_size;
}


intptr_t RawObject::VisitPointers(ObjectPointerVisitor* visitor) {
  intptr_t size = 0;
  NoHandleScope no_handles;

  // Only reasonable to be called on heap objects.
  ASSERT(IsHeapObject());

  // Read the necessary data out of the class before visting the class itself.
  ObjectKind kind = ptr()->class_->ptr()->instance_kind_;

  // Visit the class before visting the fields.
  visitor->VisitPointer(reinterpret_cast<RawObject**>(&ptr()->class_));

  switch (kind) {
#define RAW_VISITPOINTERS(clazz) \
    case clazz::kInstanceKind: { \
      Raw##clazz* raw_obj = reinterpret_cast<Raw##clazz*>(this); \
      size = Raw##clazz::Visit##clazz##Pointers(raw_obj, visitor); \
      break; \
    }
    CLASS_LIST_NO_OBJECT(RAW_VISITPOINTERS)
#undef RAW_VISITPOINTERS
    default:
      UNREACHABLE();
      break;
  }

  ASSERT(size != 0);
  return size;
}


intptr_t RawClass::VisitClassPointers(RawClass* raw_obj,
                                      ObjectPointerVisitor* visitor) {
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return Class::InstanceSize();
}


intptr_t RawType::VisitTypePointers(RawType* raw_obj,
                                    ObjectPointerVisitor* visitor) {
  // RawType is an abstract class.
  UNREACHABLE();
  return 0;
}


intptr_t RawParameterizedType::VisitParameterizedTypePointers(
    RawParameterizedType* raw_obj, ObjectPointerVisitor* visitor) {
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return ParameterizedType::InstanceSize();
}


intptr_t RawTypeParameter::VisitTypeParameterPointers(
    RawTypeParameter* raw_obj, ObjectPointerVisitor* visitor) {
  visitor->VisitPointer(reinterpret_cast<RawObject**>(&raw_obj->ptr()->name_));
  return TypeParameter::InstanceSize();
}


intptr_t RawInstantiatedType::VisitInstantiatedTypePointers(
    RawInstantiatedType* raw_obj, ObjectPointerVisitor* visitor) {
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return InstantiatedType::InstanceSize();
}


intptr_t RawTypeArguments::VisitTypeArgumentsPointers(
    RawTypeArguments* raw_obj, ObjectPointerVisitor* visitor) {
  // RawTypeArguments is an abstract class.
  UNREACHABLE();
  return 0;
}


intptr_t RawTypeArray::VisitTypeArrayPointers(RawTypeArray* raw_obj,
                                              ObjectPointerVisitor* visitor) {
  intptr_t length = Smi::Value(raw_obj->ptr()->length_);
  visitor->VisitPointers(raw_obj->from(), raw_obj->to(length));
  return TypeArray::InstanceSize(length);
}


intptr_t RawInstantiatedTypeArguments::VisitInstantiatedTypeArgumentsPointers(
    RawInstantiatedTypeArguments* raw_obj, ObjectPointerVisitor* visitor) {
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return InstantiatedTypeArguments::InstanceSize();
}


intptr_t RawFunction::VisitFunctionPointers(RawFunction* raw_obj,
                                            ObjectPointerVisitor* visitor) {
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return Function::InstanceSize();
}


intptr_t RawField::VisitFieldPointers(RawField* raw_obj,
                                      ObjectPointerVisitor* visitor) {
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return Field::InstanceSize();
}


intptr_t RawTokenStream::VisitTokenStreamPointers(
    RawTokenStream* raw_obj, ObjectPointerVisitor* visitor) {
  intptr_t length = Smi::Value(raw_obj->ptr()->length_);
  visitor->VisitPointers(raw_obj->from(), raw_obj->to(length));
  return TokenStream::InstanceSize(length);
}


intptr_t RawScript::VisitScriptPointers(RawScript* raw_obj,
                                        ObjectPointerVisitor* visitor) {
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return Script::InstanceSize();
}


intptr_t RawLibrary::VisitLibraryPointers(RawLibrary* raw_obj,
                                          ObjectPointerVisitor* visitor) {
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return Library::InstanceSize();
}


intptr_t RawLibraryPrefix::VisitLibraryPrefixPointers(
    RawLibraryPrefix* raw_obj, ObjectPointerVisitor* visitor) {
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return LibraryPrefix::InstanceSize();
}


intptr_t RawCode::VisitCodePointers(RawCode* raw_obj,
                                    ObjectPointerVisitor* visitor) {
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());

  // Also visit all the embedded pointers in the corresponding instructions.
  RawCode* obj = raw_obj->ptr();
  intptr_t length = obj->pointer_offsets_length_;
  uword entry_point = reinterpret_cast<uword>(obj->instructions_->ptr()) +
      Instructions::HeaderSize();
  for (intptr_t i = 0; i < length; i++) {
    int32_t offset = obj->data_[i];
    visitor->VisitPointer(reinterpret_cast<RawObject**>(entry_point + offset));
  }
  return Code::InstanceSize(length);
}


intptr_t RawInstructions::VisitInstructionsPointers(
    RawInstructions* raw_obj, ObjectPointerVisitor* visitor) {
  RawInstructions* obj = raw_obj->ptr();
  visitor->VisitPointer(reinterpret_cast<RawObject**>(&obj->code_));
  return Instructions::InstanceSize(obj->size_);
}


intptr_t RawPcDescriptors::VisitPcDescriptorsPointers(
    RawPcDescriptors* raw_obj, ObjectPointerVisitor* visitor) {
  RawPcDescriptors* obj = raw_obj->ptr();
  intptr_t length = Smi::Value(obj->length_);
  visitor->VisitPointer(reinterpret_cast<RawObject**>(&obj->length_));
  return PcDescriptors::InstanceSize(length);
}


intptr_t RawExceptionHandlers::VisitExceptionHandlersPointers(
    RawExceptionHandlers* raw_obj, ObjectPointerVisitor* visitor) {
  RawExceptionHandlers* obj = raw_obj->ptr();
  intptr_t length = Smi::Value(obj->length_);
  visitor->VisitPointer(reinterpret_cast<RawObject**>(&obj->length_));
  return ExceptionHandlers::InstanceSize(length);
}


intptr_t RawContext::VisitContextPointers(RawContext* raw_obj,
                                          ObjectPointerVisitor* visitor) {
  intptr_t num_variables = raw_obj->ptr()->num_variables_;
  visitor->VisitPointers(raw_obj->from(), raw_obj->to(num_variables));
  return Context::InstanceSize(num_variables);
}


intptr_t RawContextScope::VisitContextScopePointers(
    RawContextScope* raw_obj, ObjectPointerVisitor* visitor) {
  intptr_t num_variables = raw_obj->ptr()->num_variables_;
  visitor->VisitPointers(raw_obj->from(), raw_obj->to(num_variables));
  return ContextScope::InstanceSize(num_variables);
}


intptr_t RawUnhandledException::VisitUnhandledExceptionPointers(
    RawUnhandledException* raw_obj, ObjectPointerVisitor* visitor) {
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return UnhandledException::InstanceSize();
}


intptr_t RawInstance::VisitInstancePointers(RawInstance* raw_obj,
                                            ObjectPointerVisitor* visitor) {
  // Make sure that we got here with the tagged pointer as this.
  ASSERT(raw_obj->IsHeapObject());
  RawInstance* obj = raw_obj->ptr();
  intptr_t instance_size = obj->class_->ptr()->instance_size_;
  intptr_t num_native_fields = obj->class_->ptr()->num_native_fields_;

  // Calculate the first and last raw object pointer fields.
  uword obj_addr = RawObject::ToAddr(raw_obj);
  uword from = obj_addr +  ((num_native_fields + 1) * kWordSize);
  uword to = obj_addr + instance_size - kWordSize;
  visitor->VisitPointers(reinterpret_cast<RawObject**>(from),
                         reinterpret_cast<RawObject**>(to));
  return instance_size;
}


intptr_t RawNumber::VisitNumberPointers(RawNumber* raw_obj,
                                        ObjectPointerVisitor* visitor) {
  // Number is an abstract class.
  UNREACHABLE();
  return 0;
}


intptr_t RawInteger::VisitIntegerPointers(RawInteger* raw_obj,
                                          ObjectPointerVisitor* visitor) {
  // Integer is an abstract class.
  UNREACHABLE();
  return 0;
}


intptr_t RawSmi::VisitSmiPointers(RawSmi* raw_obj,
                                  ObjectPointerVisitor* visitor) {
  // Smi does not have a heap representation.
  UNREACHABLE();
  return 0;
}


intptr_t RawMint::VisitMintPointers(RawMint* raw_obj,
                                    ObjectPointerVisitor* visitor) {
  // Make sure that we got here with the tagged pointer as this.
  ASSERT(raw_obj->IsHeapObject());
  return Mint::InstanceSize();
}


intptr_t RawBigint::VisitBigintPointers(RawBigint* raw_obj,
                                        ObjectPointerVisitor* visitor) {
  // Make sure that we got here with the tagged pointer as this.
  ASSERT(raw_obj->IsHeapObject());
  RawBigint* obj = raw_obj->ptr();
  return Bigint::InstanceSize(&obj->bn_);
}


intptr_t RawDouble::VisitDoublePointers(RawDouble* raw_obj,
                                        ObjectPointerVisitor* visitor) {
  // Make sure that we got here with the tagged pointer as this.
  ASSERT(raw_obj->IsHeapObject());
  return Double::InstanceSize();
}


intptr_t RawString::VisitStringPointers(RawString* raw_obj,
                                        ObjectPointerVisitor* visitor) {
  // String is an abstract class.
  UNREACHABLE();
  return 0;
}


intptr_t RawOneByteString::VisitOneByteStringPointers(
    RawOneByteString* raw_obj, ObjectPointerVisitor* visitor) {
  intptr_t length = Smi::Value(raw_obj->ptr()->length_);
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return OneByteString::InstanceSize(length);
}


intptr_t RawTwoByteString::VisitTwoByteStringPointers(
    RawTwoByteString* raw_obj, ObjectPointerVisitor* visitor) {
  intptr_t length = Smi::Value(raw_obj->ptr()->length_);
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return TwoByteString::InstanceSize(length);
}


intptr_t RawFourByteString::VisitFourByteStringPointers(
    RawFourByteString* raw_obj, ObjectPointerVisitor* visitor) {
  intptr_t length = Smi::Value(raw_obj->ptr()->length_);
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return FourByteString::InstanceSize(length);
}


intptr_t RawBool::VisitBoolPointers(RawBool* raw_obj,
                                    ObjectPointerVisitor* visitor) {
  // Make sure that we got here with the tagged pointer as this.
  ASSERT(raw_obj->IsHeapObject());
  return Bool::InstanceSize();
}


intptr_t RawArray::VisitArrayPointers(RawArray* raw_obj,
                                      ObjectPointerVisitor* visitor) {
  intptr_t length = Smi::Value(raw_obj->ptr()->length_);
  visitor->VisitPointers(raw_obj->from(), raw_obj->to(length));
  return Array::InstanceSize(length);
}


intptr_t RawImmutableArray::VisitImmutableArrayPointers(
    RawImmutableArray* raw_obj, ObjectPointerVisitor* visitor) {
  return RawArray::VisitArrayPointers(raw_obj, visitor);
}


intptr_t RawClosure::VisitClosurePointers(RawClosure* raw_obj,
                                          ObjectPointerVisitor* visitor) {
  // Make sure that we got here with the tagged pointer as this.
  ASSERT(raw_obj->IsHeapObject());
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return Closure::InstanceSize();
}


intptr_t RawStacktrace::VisitStacktracePointers(RawStacktrace* raw_obj,
                                                ObjectPointerVisitor* visitor) {
  // Make sure that we got here with the tagged pointer as this.
  ASSERT(raw_obj->IsHeapObject());
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return Stacktrace::InstanceSize();
}


intptr_t RawJSRegExp::VisitJSRegExpPointers(RawJSRegExp* raw_obj,
                                            ObjectPointerVisitor* visitor) {
  // Make sure that we got here with the tagged pointer as this.
  ASSERT(raw_obj->IsHeapObject());
  intptr_t length = Smi::Value(raw_obj->ptr()->data_length_);
  visitor->VisitPointers(raw_obj->from(), raw_obj->to());
  return JSRegExp::InstanceSize(length);
}

}  // namespace dart
