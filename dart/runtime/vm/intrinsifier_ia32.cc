// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
//
// The intrinsic code below is executed before a method has built its frame.
// The return address is on the stack and the arguments below it.
// Registers EDX (arguments descriptor) and ECX (function) must be preserved.
// Each intrinsification method returns true if the corresponding
// Dart method was intrinsified.

#include "vm/globals.h"  // Needed here to get TARGET_ARCH_IA32.
#if defined(TARGET_ARCH_IA32)

#include "vm/intrinsifier.h"

#include "vm/assembler.h"
#include "vm/assembler_macros.h"
#include "vm/object.h"
#include "vm/object_store.h"
#include "vm/os.h"
#include "vm/stub_code.h"

namespace dart {

DEFINE_FLAG(bool, intrinsify, true, "Instrinsify when possible");

// List of intrinsics: (class-name, function-name, intrinsification method).
#define INTRINSIC_LIST(V)                                                      \
  V(IntegerImplementation, addFromInteger, Integer_addFromInteger)             \
  V(IntegerImplementation, +, Integer_addFromInteger)                          \
  V(IntegerImplementation, subFromInteger, Integer_subFromInteger)             \
  V(IntegerImplementation, -, Integer_sub)                                     \
  V(IntegerImplementation, mulFromInteger, Integer_mulFromInteger)             \
  V(IntegerImplementation, *, Integer_mulFromInteger)                          \
  V(IntegerImplementation, %, Integer_modulo)                                  \
  V(IntegerImplementation, negate, Integer_negate)                             \
  V(IntegerImplementation, bitAndFromInteger, Integer_bitAndFromInteger)       \
  V(IntegerImplementation, &, Integer_bitAndFromInteger)                       \
  V(IntegerImplementation, bitOrFromInteger, Integer_bitOrFromInteger)         \
  V(IntegerImplementation, |, Integer_bitOrFromInteger)                        \
  V(IntegerImplementation, bitXorFromInteger, Integer_bitXorFromInteger)       \
  V(IntegerImplementation, ^, Integer_bitXorFromInteger)                       \
  V(IntegerImplementation, greaterThanFromInteger, Integer_lessThan)           \
  V(IntegerImplementation, >, Integer_greaterThan)                             \
  V(IntegerImplementation, ==, Integer_equalToInteger)                         \
  V(IntegerImplementation, equalToInteger, Integer_equalToInteger)             \
  V(IntegerImplementation, <, Integer_lessThan)                                \
  V(IntegerImplementation, <=, Integer_lessEqualThan)                          \
  V(IntegerImplementation, >=, Integer_greaterEqualThan)                       \
  V(IntegerImplementation, <<, Integer_shl)                                    \
  V(IntegerImplementation, >>, Integer_sar)                                    \
  V(Smi, ~, Smi_bitNegate)                                                     \
  V(Double, >, Double_greaterThan)                                             \
  V(Double, >=, Double_greaterEqualThan)                                       \
  V(Double, <, Double_lessThan)                                                \
  V(Double, <=, Double_lessEqualThan)                                          \
  V(Double, ==, Double_equal)                                                  \
  V(Double, +, Double_add)                                                     \
  V(Double, -, Double_sub)                                                     \
  V(Double, *, Double_mul)                                                     \
  V(Double, /, Double_div)                                                     \
  V(Double, toDouble, Double_toDouble)                                         \
  V(Double, mulFromInteger, Double_mulFromInteger)                             \
  V(ObjectArray, ObjectArray., ObjectArray_Allocate)                           \
  V(ObjectArray, get:length, Array_getLength)                                  \
  V(ObjectArray, [], Array_getIndexed)                                         \
  V(ObjectArray, []=, Array_setIndexed)                                        \
  V(GrowableObjectArray, get:length, GrowableArray_getLength)                  \
  V(GrowableObjectArray, [], GrowableArray_getIndexed)                         \
  V(ImmutableArray, [], Array_getIndexed)                                      \
  V(ImmutableArray, get:length, Array_getLength)                               \
  V(Math, sqrt, Math_sqrt)                                                     \
  V(Object, ==, Object_equal)                                                  \
  V(FixedSizeArrayIterator, next, FixedSizeArrayIterator_next)                 \
  V(FixedSizeArrayIterator, hasNext, FixedSizeArrayIterator_hasNext)           \

#define __ assembler->

static bool ObjectArray_Allocate(Assembler* assembler) {
  // This snippet of inlined code uses the following registers:
  // EAX, EBX, EDI
  // and the newly allocated object is returned in EAX.
  const intptr_t kTypeArgumentsOffset = 2 * kWordSize;
  const intptr_t kArrayLengthOffset = 1 * kWordSize;
  Label fall_through;

  // Compute the size to be allocated, it is based on the array length
  // and it computed as:
  // RoundedAllocationSize((array_length * kwordSize) + sizeof(RawArray)).
  __ movl(EDI, Address(ESP, kArrayLengthOffset));  // Array Length.
  // Assert that length is a Smi.
  __ testl(EDI, Immediate(kSmiTagSize));
  __ j(NOT_ZERO, &fall_through, Assembler::kNearJump);
  intptr_t fixed_size = sizeof(RawArray) + kObjectAlignment - 1;
  __ leal(EDI, Address(EDI, TIMES_2, fixed_size));  // EDI is a Smi.
  ASSERT(kSmiTagShift == 1);
  __ andl(EDI, Immediate(-kObjectAlignment));

  Heap* heap = Isolate::Current()->heap();

  // EDI: size to allocate.
  __ movl(EAX, Address::Absolute(heap->TopAddress()));
  __ leal(EBX, Address(EAX, EDI, TIMES_1, 0));

  // Check if the allocation fits into the remaining space.
  // EAX: potential new object start.
  // EBX: potential next object start.
  __ cmpl(EBX, Address::Absolute(heap->EndAddress()));
  __ j(ABOVE_EQUAL, &fall_through, Assembler::kNearJump);

  // Successfully allocated the object(s), now update top to point to
  // next object start and initialize the object.
  __ movl(Address::Absolute(heap->TopAddress()), EBX);
  __ addl(EAX, Immediate(kHeapObjectTag));

  // EAX: new object start as a tagged pointer.
  // EBX: new object end address.
  // Store class value for array.
  __ movl(EDI, FieldAddress(CTX, Context::isolate_offset()));
  __ movl(EDI, Address(EDI, Isolate::object_store_offset()));
  __ movl(EDI, Address(EDI, ObjectStore::array_class_offset()));
  __ movl(FieldAddress(EAX, Instance::class_offset()), EDI);

  // Store the type argument field.
  __ movl(EDI, Address(ESP, kTypeArgumentsOffset));  // type argument.
  __ movl(FieldAddress(EAX, Array::type_arguments_offset()), EDI);

  // Set the length field.
  __ movl(EDI, Address(ESP, kArrayLengthOffset));  // Array Length.
  __ movl(FieldAddress(EAX, Array::length_offset()), EDI);

  // Initialize all array elements to raw_null.
  // EAX: new object start as a tagged pointer.
  // EBX: new object end address.
  // EDI: iterator which initially points to the start of the variable
  // data area to be initialized.
  const Immediate raw_null =
      Immediate(reinterpret_cast<intptr_t>(Object::null()));
  __ leal(EDI, FieldAddress(EAX, sizeof(RawArray)));
  Label done;
  Label init_loop;
  __ Bind(&init_loop);
  __ cmpl(EDI, EBX);
  __ j(ABOVE_EQUAL, &done, Assembler::kNearJump);
  __ movl(Address(EDI, 0), raw_null);
  __ addl(EDI, Immediate(kWordSize));
  __ jmp(&init_loop, Assembler::kNearJump);
  __ Bind(&done);
  __ ret();  // returns the newly allocated object in EAX.

  __ Bind(&fall_through);
  return false;
}


static bool Array_getLength(Assembler* assembler) {
  __ movl(EAX, Address(ESP, + 1 * kWordSize));
  __ movl(EAX, FieldAddress(EAX, Array::length_offset()));
  __ ret();
  return true;
}


static bool Array_getIndexed(Assembler* assembler) {
  Label fall_through;
  __ movl(EBX, Address(ESP, + 1 * kWordSize));  // Index.
  __ movl(EAX, Address(ESP, + 2 * kWordSize));  // Array.
  __ testl(EBX, Immediate(kSmiTagMask));
  __ j(NOT_ZERO, &fall_through, Assembler::kNearJump);  // Non-smi index.
  // Range check.
  __ cmpl(EBX, FieldAddress(EAX, Array::length_offset()));
  // Runtime throws exception.
  __ j(ABOVE_EQUAL, &fall_through, Assembler::kNearJump);
  // Note that EBX is Smi, i.e, times 2.
  ASSERT(kSmiTagShift == 1);
  __ movl(EAX, FieldAddress(EAX, EBX, TIMES_2, sizeof(RawArray)));
  __ ret();
  __ Bind(&fall_through);
  return false;
}


// Intrinsify only for Smi value and index. Non-smi values need a store buffer
// update. Array length is always a Smi.
static bool Array_setIndexed(Assembler* assembler) {
  const Immediate raw_null =
      Immediate(reinterpret_cast<intptr_t>(Object::null()));
  Label fall_through;
  __ movl(EAX, Address(ESP, + 1 * kWordSize));  // Value.
  __ movl(EBX, Address(ESP, + 2 * kWordSize));  // Index.
  __ orl(EAX, EBX);
  __ testl(EBX, Immediate(kSmiTagMask));
  // Value or index not Smi.
  __ j(NOT_ZERO, &fall_through, Assembler::kNearJump);
  __ movl(EAX, Address(ESP, + 3 * kWordSize));  // Array.
  // Range check.
  __ cmpl(EBX, FieldAddress(EAX, Array::length_offset()));
  // Runtime throws exception.
  __ j(ABOVE_EQUAL, &fall_through, Assembler::kNearJump);
  // Note that EBX is Smi, i.e, times 2.
  ASSERT(kSmiTagShift == 1);
  // Destroy ECX as we will not continue in the function.
  __ movl(ECX, Address(ESP, + 1 * kWordSize));
  __ movl(FieldAddress(EAX, EBX, TIMES_2, sizeof(RawArray)), ECX);
  // Caller is responsible of preserving the value if necessary.
  __ ret();
  __ Bind(&fall_through);
  return false;
}


static intptr_t GetOffsetForField(const char* class_name_p,
                                  const char* field_name_p) {
  const String& class_name = String::Handle(String::NewSymbol(class_name_p));
  const String& field_name = String::Handle(String::NewSymbol(field_name_p));
  const Class& cls = Class::Handle(Library::Handle(
      Library::CoreImplLibrary()).LookupClass(class_name));
  ASSERT(!cls.IsNull());
  const Field& field = Field::ZoneHandle(cls.LookupInstanceField(field_name));
  ASSERT(!field.IsNull());
  return field.Offset();
}


static const char* kGrowableArrayClassName = "GrowableObjectArray";
static const char* kGrowableArrayLengthFieldName = "_length";
static const char* kGrowableArrayArrayFieldName = "backingArray";

// Read the length_ instance field.
static bool GrowableArray_getLength(Assembler* assembler) {
  intptr_t length_offset = GetOffsetForField(kGrowableArrayClassName,
                                             kGrowableArrayLengthFieldName);
  __ movl(EAX, Address(ESP, + 1 * kWordSize));
  __ movl(EAX, FieldAddress(EAX, length_offset));
  __ ret();
  return true;
}


static bool GrowableArray_getIndexed(Assembler* assembler) {
  intptr_t length_offset = GetOffsetForField(kGrowableArrayClassName,
                                             kGrowableArrayLengthFieldName);
  intptr_t array_offset = GetOffsetForField(kGrowableArrayClassName,
                                            kGrowableArrayArrayFieldName);
  Label fall_through;
  __ movl(EBX, Address(ESP, + 1 * kWordSize));  // Index.
  __ movl(EAX, Address(ESP, + 2 * kWordSize));  // GrowableArray.
  __ testl(EBX, Immediate(kSmiTagMask));
  __ j(NOT_ZERO, &fall_through, Assembler::kNearJump);  // Non-smi index.
  // Range check.
  __ cmpl(EBX, FieldAddress(EAX, length_offset));
  // Runtime throws exception.
  __ j(ABOVE_EQUAL, &fall_through, Assembler::kNearJump);
  __ movl(EAX, FieldAddress(EAX, array_offset));  // backingArray.

  // Note that EBX is Smi, i.e, times 2.
  ASSERT(kSmiTagShift == 1);
  __ movl(EAX, FieldAddress(EAX, EBX, TIMES_2, sizeof(RawArray)));
  __ ret();
  __ Bind(&fall_through);
  return false;
}


// Tests if two top most arguments are smis, jumps to label not_smi if not.
// Topmost argument is in EAX.
static void TestBothArgumentsSmis(Assembler* assembler, Label* not_smi) {
  __ movl(EAX, Address(ESP, + 1 * kWordSize));
  __ movl(EBX, Address(ESP, + 2 * kWordSize));
  __ orl(EBX, EAX);
  __ testl(EBX, Immediate(kSmiTagMask));
  __ j(NOT_ZERO, not_smi, Assembler::kNearJump);
}


static bool Integer_addFromInteger(Assembler* assembler) {
  Label fall_through;
  TestBothArgumentsSmis(assembler, &fall_through);
  __ addl(EAX, Address(ESP, + 2 * kWordSize));
  __ j(OVERFLOW, &fall_through, Assembler::kNearJump);
  // Result is in EAX.
  __ ret();
  __ Bind(&fall_through);
  return false;
}


static bool Integer_subFromInteger(Assembler* assembler) {
  Label fall_through;
  TestBothArgumentsSmis(assembler, &fall_through);
  __ subl(EAX, Address(ESP, + 2 * kWordSize));
  __ j(OVERFLOW, &fall_through, Assembler::kNearJump);
  // Result is in EAX.
  __ ret();
  __ Bind(&fall_through);
  return false;
}


static bool Integer_sub(Assembler* assembler) {
  Label fall_through;
  TestBothArgumentsSmis(assembler, &fall_through);
  __ movl(EBX, EAX);
  __ movl(EAX, Address(ESP, + 2 * kWordSize));
  __ subl(EAX, EBX);
  __ j(OVERFLOW, &fall_through, Assembler::kNearJump);
  // Result is in EAX.
  __ ret();
  __ Bind(&fall_through);
  return false;
}



static bool Integer_mulFromInteger(Assembler* assembler) {
  Label fall_through;
  TestBothArgumentsSmis(assembler, &fall_through);
  ASSERT(kSmiTag == 0);  // Adjust code below if not the case.
  __ SmiUntag(EAX);
  __ imull(EAX, Address(ESP, + 2 * kWordSize));
  __ j(OVERFLOW, &fall_through, Assembler::kNearJump);
  // Result is in EAX.
  __ ret();
  __ Bind(&fall_through);
  return false;
}


// Simple implementation: for positive dividend values greater than divisor,
// return dividend.
static bool Integer_modulo(Assembler* assembler) {
  Label fall_through, return_zero;
  TestBothArgumentsSmis(assembler, &fall_through);
  // EAX: right argument (divisor)
  __ movl(EBX, Address(ESP, + 2 * kWordSize));  // Left argument (dividend).
  __ cmpl(EBX, Immediate(0));
  __ j(LESS, &fall_through, Assembler::kNearJump);
  __ cmpl(EBX, EAX);
  __ j(EQUAL, &return_zero, Assembler::kNearJump);
  __ j(GREATER, &fall_through, Assembler::kNearJump);
  __ movl(EAX, EBX);  // Return dividend.
  __ ret();
  __ Bind(&return_zero);
  __ xorl(EAX, EAX);  // Return zero.
  __ ret();
  __ Bind(&fall_through);
  return false;
}


static bool Integer_negate(Assembler* assembler) {
  Label fall_through;
  __ movl(EAX, Address(ESP, + 1 * kWordSize));
  __ testl(EAX, Immediate(kSmiTagMask));
  __ j(NOT_ZERO, &fall_through, Assembler::kNearJump);  // Non-smi value.
  __ negl(EAX);
  __ j(OVERFLOW, &fall_through, Assembler::kNearJump);
  // Result is in EAX.
  __ ret();
  __ Bind(&fall_through);
  return false;
}


static bool Integer_bitAndFromInteger(Assembler* assembler) {
  Label fall_through;
  TestBothArgumentsSmis(assembler, &fall_through);
  __ movl(EBX, Address(ESP, + 2 * kWordSize));
  __ andl(EAX, EBX);
  // Result is in EAX.
  __ ret();
  __ Bind(&fall_through);
  return false;
}


static bool Integer_bitOrFromInteger(Assembler* assembler) {
  Label fall_through;
  TestBothArgumentsSmis(assembler, &fall_through);
  __ movl(EBX, Address(ESP, + 2 * kWordSize));
  __ orl(EAX, EBX);
  // Result is in EAX.
  __ ret();
  __ Bind(&fall_through);
  return false;
}


static bool Integer_bitXorFromInteger(Assembler* assembler) {
  Label fall_through;
  TestBothArgumentsSmis(assembler, &fall_through);
  __ movl(EBX, Address(ESP, + 2 * kWordSize));
  __ xorl(EAX, EBX);
  // Result is in EAX.
  __ ret();
  __ Bind(&fall_through);
  return false;
}


static bool Integer_shl(Assembler* assembler) {
  ASSERT(kSmiTagShift == 1);
  ASSERT(kSmiTag == 0);
  Label fall_through, overflow;
  TestBothArgumentsSmis(assembler, &fall_through);
  // Shift value is in EAX. Compare with tagged Smi.
  __ cmpl(EAX, Immediate(Smi::RawValue(Smi::kBits)));
  __ j(ABOVE_EQUAL, &fall_through, Assembler::kNearJump);

  __ SmiUntag(EAX);
  __ movl(ECX, EAX);  // Shift amount must be in ECX.
  __ movl(EAX, Address(ESP, + 2 * kWordSize));  // Value.

  // Overflow test - all the shifted-out bits must be same as the sign bit.
  __ movl(EBX, EAX);
  __ shll(EAX, ECX);
  __ sarl(EAX, ECX);
  __ cmpl(EAX, EBX);
  __ j(NOT_EQUAL, &overflow, Assembler::kNearJump);

  __ shll(EAX, ECX);  // Shift for result now we know there is no overflow.

  // EAX is a correctly tagged Smi.
  __ ret();

  __ Bind(&overflow);
  // Arguments are Smi but the shift produced an overflow to Mint.
  __ cmpl(EBX, Immediate(0));
  // TODO(srdjan): Implement negative values, for now fall through.
  __ j(LESS, &fall_through, Assembler::kNearJump);
  __ SmiUntag(EBX);
  __ movl(EAX, EBX);
  __ shll(EBX, ECX);
  __ xorl(EDI, EDI);
  __ shld(EDI, EAX);
  // Result in EDI (high) and EBX (low).
  const Class& mint_class = Class::ZoneHandle(
      Isolate::Current()->object_store()->mint_class());
  __ LoadObject(ECX, mint_class);
  AssemblerMacros::TryAllocate(assembler,
                               mint_class,
                               ECX,  // Class register.
                               &fall_through,
                               EAX);  // Result register.
  __ movl(FieldAddress(EAX, Mint::value_offset()), EBX);
  __ movl(FieldAddress(EAX, Mint::value_offset() + kWordSize), EDI);
  __ ret();
  __ Bind(&fall_through);
  return false;
}


static bool CompareIntegers(Assembler* assembler, Condition true_condition) {
  Label fall_through, true_label;
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  const Bool& bool_false = Bool::ZoneHandle(Bool::False());
  TestBothArgumentsSmis(assembler, &fall_through);
  // EAX contains the right argument.
  __ cmpl(Address(ESP, + 2 * kWordSize), EAX);
  __ j(true_condition, &true_label, Assembler::kNearJump);
  __ LoadObject(EAX, bool_false);
  __ ret();
  __ Bind(&true_label);
  __ LoadObject(EAX, bool_true);
  __ ret();
  __ Bind(&fall_through);
  return false;
}


static bool Integer_lessThan(Assembler* assembler) {
  return CompareIntegers(assembler, LESS);
}


static bool Integer_greaterThan(Assembler* assembler) {
  return CompareIntegers(assembler, GREATER);
}


static bool Integer_lessEqualThan(Assembler* assembler) {
  return CompareIntegers(assembler, LESS_EQUAL);
}


static bool Integer_greaterEqualThan(Assembler* assembler) {
  return CompareIntegers(assembler, GREATER_EQUAL);
}


// This is called for Smi, Mint and Bigint receivers. Bigints are not handled.
static bool Integer_equalToInteger(Assembler* assembler) {
  Label fall_through, true_label, check_for_mint;
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  const Bool& bool_false = Bool::ZoneHandle(Bool::False());
  // For integer receiver '===' check first.
  __ movl(EAX, Address(ESP, + 1 * kWordSize));
  __ cmpl(EAX, Address(ESP, + 2 * kWordSize));
  __ j(EQUAL, &true_label, Assembler::kNearJump);
  __ movl(EBX, Address(ESP, + 2 * kWordSize));
  __ orl(EAX, EBX);
  __ testl(EAX, Immediate(kSmiTagMask));
  __ j(NOT_ZERO, &check_for_mint, Assembler::kNearJump);
  // Both arguments are smi, '===' is good enough.
  __ LoadObject(EAX, bool_false);
  __ ret();
  __ Bind(&true_label);
  __ LoadObject(EAX, bool_true);
  __ ret();

  // At least one of the arguments was not Smi, inline code for Smi/Mint
  // equality comparison.
  ObjectStore* object_store = Isolate::Current()->object_store();
  Label receiver_not_smi;
  __ Bind(&check_for_mint);
  __ movl(EAX, Address(ESP, + 2 * kWordSize));  // Receiver.
  __ testl(EAX, Immediate(kSmiTagMask));
  __ j(NOT_ZERO, &receiver_not_smi);

  // Note that an instance of Mint never contains a value that can be
  // represented by Smi.
  // Left is Smi, return false if right is Mint, otherwise fall through.
  __ movl(EAX, Address(ESP, + 1 * kWordSize));  // Right argument.
  __ movl(EAX, FieldAddress(EAX, Object::class_offset()));
  __ CompareObject(EAX, Class::ZoneHandle(object_store->mint_class()));
  __ j(NOT_EQUAL, &fall_through);
  __ LoadObject(EAX, bool_false);  // Smi == Mint -> false.
  __ ret();

  __ Bind(&receiver_not_smi);
  // EAX:: receiver.
  __ movl(EAX, FieldAddress(EAX, Object::class_offset()));
  __ CompareObject(EAX, Class::ZoneHandle(object_store->mint_class()));
  __ j(NOT_EQUAL, &fall_through);
  // Receiver is Mint, return false if right is Smi.
  __ movl(EAX, Address(ESP, + 1 * kWordSize));  // Right argument.
  __ testl(EAX, Immediate(kSmiTagMask));
  __ j(NOT_ZERO, &fall_through);
  __ LoadObject(EAX, bool_false);  // Smi == Mint -> false.
  __ ret();
  // TODO(srdjan): Implement Mint == Mint comparison.

  __ Bind(&fall_through);
  return false;
}


static bool Integer_sar(Assembler* assembler) {
  Label fall_through, shift_count_ok;
  TestBothArgumentsSmis(assembler, &fall_through);
  // Can destroy ECX since we are not falling through.
  Immediate count_limit = Immediate(0x1F);
  // Check that the count is not larger than what the hardware can handle.
  // For shifting right a Smi the result is the same for all numbers
  // >= count_limit.
  __ SmiUntag(EAX);
  __ cmpl(EAX, count_limit);
  __ j(LESS_EQUAL, &shift_count_ok, Assembler::kNearJump);
  __ movl(EAX, count_limit);
  __ Bind(&shift_count_ok);
  __ movl(ECX, EAX);  // Shift amount must be in ECX.
  __ movl(EAX, Address(ESP, + 2 * kWordSize));  // Value.
  __ SmiUntag(EAX);  // Value.
  __ sarl(EAX, ECX);
  __ SmiTag(EAX);
  __ ret();
  __ Bind(&fall_through);
  return false;
}


static bool Smi_bitNegate(Assembler* assembler) {
  Label fall_through;
  __ movl(EAX, Address(ESP, + 1 * kWordSize));  // Index.
  __ testl(EAX, Immediate(kSmiTagMask));
  __ j(NOT_ZERO, &fall_through, Assembler::kNearJump);  // Non-smi.
  __ notl(EAX);
  __ andl(EAX, Immediate(~kSmiTagMask));  // Remove inverted smi-tag.
  __ ret();
  __ Bind(&fall_through);
  return false;
}


// Check if the last argument is a double, otherwise jumps to Label
// 'not_double'. Returns the last argument in EAX.
static void TestLastArgumentsIsDouble(Assembler* assembler, Label* not_double) {
  __ movl(EAX, Address(ESP, + 1 * kWordSize));
  __ testl(EAX, Immediate(kSmiTagMask));
  __ j(ZERO, not_double, Assembler::kNearJump);  // Jump if Smi.
  __ LoadObject(EBX, Class::ZoneHandle(
      Isolate::Current()->object_store()->double_class()));
  __ cmpl(EBX, FieldAddress(EAX, Object::class_offset()));
  __ j(NOT_EQUAL, not_double, Assembler::kNearJump);  // Jump if not double.
  // Fall through if double.
}


// Both arguments on stack, arg0 (left) is a double, arg1 (right) is of unknown
// type. Return true or false object in the register EAX. Any NaN argument
// returns false. Any non-double arg1 causes control flow to fall through to the
// slow case (compiled method body).
static bool CompareDoubles(Assembler* assembler, Condition true_condition) {
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  const Bool& bool_false = Bool::ZoneHandle(Bool::False());
  Label fall_through, is_false, is_true;
  TestLastArgumentsIsDouble(assembler, &fall_through);
  // Both arguments are double, right operand is in EAX.
  __ movsd(XMM1, FieldAddress(EAX, Double::value_offset()));
  __ movl(EAX, Address(ESP, + 2 * kWordSize));  // Left argument.
  __ movsd(XMM0, FieldAddress(EAX, Double::value_offset()));
  __ comisd(XMM0, XMM1);
  __ j(PARITY_EVEN, &is_false, Assembler::kNearJump);  // NaN -> false;
  __ j(true_condition, &is_true, Assembler::kNearJump);
  // Fall through false.
  __ Bind(&is_false);
  __ LoadObject(EAX, bool_false);
  __ ret();
  __ Bind(&is_true);
  __ LoadObject(EAX, bool_true);
  __ ret();
  __ Bind(&fall_through);
  return false;
}


// arg0 is Double, arg1 is unknown.
static bool Double_greaterThan(Assembler* assembler) {
  return CompareDoubles(assembler, ABOVE);
}


// arg0 is Double, arg1 is unknown.
static bool Double_greaterEqualThan(Assembler* assembler) {
  return CompareDoubles(assembler, ABOVE_EQUAL);
}


// arg0 is Double, arg1 is unknown.
static bool Double_lessThan(Assembler* assembler) {
  return CompareDoubles(assembler, BELOW);
}


// arg0 is Double, arg1 is unknown.
static bool Double_equal(Assembler* assembler) {
  return CompareDoubles(assembler, EQUAL);
}


// arg0 is Double, arg1 is unknown.
static bool Double_lessEqualThan(Assembler* assembler) {
  return CompareDoubles(assembler, BELOW_EQUAL);
}


static bool Double_toDouble(Assembler* assembler) {
  __ movl(EAX, Address(ESP, + 1 * kWordSize));
  __ ret();
  return true;
}


// Expects EAX to contain right argument, left argument is on stack. Left
// argument is double, right argument is of unknown type.
static bool DoubleArithmeticOperations(Assembler* assembler, Token::Kind kind) {
  Label fall_through;
  TestLastArgumentsIsDouble(assembler, &fall_through);
  // Both arguments are double, right operand is in EAX, class in EBX.
  __ movsd(XMM1, FieldAddress(EAX, Double::value_offset()));
  __ movl(EAX, Address(ESP, + 2 * kWordSize));  // Left argument.
  __ movsd(XMM0, FieldAddress(EAX, Double::value_offset()));
  switch (kind) {
    case Token::kADD: __ addsd(XMM0, XMM1); break;
    case Token::kSUB: __ subsd(XMM0, XMM1); break;
    case Token::kMUL: __ mulsd(XMM0, XMM1); break;
    case Token::kDIV: __ divsd(XMM0, XMM1); break;
    default: UNREACHABLE();
  }
  const Class& double_class = Class::ZoneHandle(
      Isolate::Current()->object_store()->double_class());
  AssemblerMacros::TryAllocate(assembler,
                               double_class,
                               EBX,  // Class register.
                               &fall_through,
                               EAX);  // Result register.
  __ movsd(FieldAddress(EAX, Double::value_offset()), XMM0);
  __ ret();
  __ Bind(&fall_through);
  return false;
}


static bool Double_add(Assembler* assembler) {
  return DoubleArithmeticOperations(assembler, Token::kADD);
}


static bool Double_mul(Assembler* assembler) {
  return DoubleArithmeticOperations(assembler, Token::kMUL);
}


static bool Double_sub(Assembler* assembler) {
  return DoubleArithmeticOperations(assembler, Token::kSUB);
}


static bool Double_div(Assembler* assembler) {
  return DoubleArithmeticOperations(assembler, Token::kDIV);
}


// Left is double right is integer (bigint or Smi)
static bool Double_mulFromInteger(Assembler* assembler) {
  Label fall_through;
  // Only Smi-s allowed.
  __ movl(EAX, Address(ESP, + 1 * kWordSize));
  __ testl(EAX, Immediate(kSmiTagMask));
  __ j(NOT_ZERO, &fall_through, Assembler::kNearJump);
  // Is Smi.
  __ SmiUntag(EAX);
  __ cvtsi2sd(XMM1, EAX);
  __ movl(EAX, Address(ESP, + 2 * kWordSize));
  __ movsd(XMM0, FieldAddress(EAX, Double::value_offset()));
  __ mulsd(XMM0, XMM1);
  const Class& double_class = Class::ZoneHandle(
      Isolate::Current()->object_store()->double_class());
  __ LoadObject(EBX, double_class);
  AssemblerMacros::TryAllocate(assembler,
                               double_class,
                               EBX,  // Class register.
                               &fall_through,
                               EAX);  // Result register.
  __ movsd(FieldAddress(EAX, Double::value_offset()), XMM0);
  __ ret();
  __ Bind(&fall_through);
  return false;
}


// Argument type is not known
static bool Math_sqrt(Assembler* assembler) {
  Label fall_through;
  TestLastArgumentsIsDouble(assembler, &fall_through);
  // Argument is double and is in EAX, class in EBX.
  __ movsd(XMM1, FieldAddress(EAX, Double::value_offset()));
  __ sqrtsd(XMM0, XMM1);
  const Class& double_class = Class::ZoneHandle(
      Isolate::Current()->object_store()->double_class());
  AssemblerMacros::TryAllocate(assembler,
                               double_class,
                               EBX,  // Class register.
                               &fall_through,
                               EAX);  // Result register.
  __ movsd(FieldAddress(EAX, Double::value_offset()), XMM0);
  __ ret();
  __ Bind(&fall_through);
  return false;
}


// Identity comparison.
static bool Object_equal(Assembler* assembler) {
  Label is_true;
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  const Bool& bool_false = Bool::ZoneHandle(Bool::False());
  __ movl(EAX, Address(ESP, + 1 * kWordSize));
  __ cmpl(EAX, Address(ESP, + 2 * kWordSize));
  __ j(EQUAL, &is_true, Assembler::kNearJump);
  __ LoadObject(EAX, bool_false);
  __ ret();
  __ Bind(&is_true);
  __ LoadObject(EAX, bool_true);
  __ ret();
  return true;
}


static const char* kFixedSizeArrayIteratorClassName = "FixedSizeArrayIterator";


// Class 'FixedSizeArrayIterator':
//   T next() {
//     return _array[_pos++];
//   }
// Intrinsify: return _array[_pos++];
// TODO(srdjan): Throw a 'NoMoreElementsException' exception if the iterator
// has no more elements.
static bool FixedSizeArrayIterator_next(Assembler* assembler) {
  Label fall_through;
  intptr_t array_offset =
      GetOffsetForField(kFixedSizeArrayIteratorClassName, "_array");
  intptr_t pos_offset =
      GetOffsetForField(kFixedSizeArrayIteratorClassName, "_pos");
  ASSERT(array_offset >= 0 && pos_offset >= 0);
  // Receiver is not NULL.
  __ movl(EAX, Address(ESP, + 1 * kWordSize));  // Receiver.
  __ movl(EBX, FieldAddress(EAX, pos_offset));  // Field _pos.
  // '_pos' cannot be greater than array length and therefore is always Smi.
#if defined(DEBUG)
  Label pos_ok;
  __ testl(EBX, Immediate(kSmiTagMask));
  __ j(ZERO, &pos_ok, Assembler::kNearJump);
  __ Stop("pos must be Smi");
  __ Bind(&pos_ok);
#endif
  // Check that we are not trying to call 'next' when 'hasNext' is false.
  __ movl(EAX, FieldAddress(EAX, array_offset));  // Field _array.
  __ cmpl(EBX, FieldAddress(EAX, Array::length_offset()));  // Range check.
  __ j(ABOVE_EQUAL, &fall_through, Assembler::kNearJump);

  // EBX is Smi, i.e, times 2.
  ASSERT(kSmiTagShift == 1);
  __ movl(EDI, FieldAddress(EAX, EBX, TIMES_2, sizeof(RawArray)));  // Result.
  const Immediate value = Immediate(reinterpret_cast<int32_t>(Smi::New(1)));
  __ addl(EBX, value);  // _pos++.
  __ j(OVERFLOW, &fall_through, Assembler::kNearJump);
  __ movl(EAX, Address(ESP, + 1 * kWordSize));  // Receiver.
  __ movl(FieldAddress(EAX, pos_offset), EBX);  // Store _pos.
  __ movl(EAX, EDI);
  __ ret();
  __ Bind(&fall_through);
  return false;
}


// Class 'FixedSizeArrayIterator':
//   bool hasNext() {
//     return _length > _pos;
//   }
static bool FixedSizeArrayIterator_hasNext(Assembler* assembler) {
  Label fall_through, is_true;
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  const Bool& bool_false = Bool::ZoneHandle(Bool::False());
  intptr_t length_offset =
      GetOffsetForField(kFixedSizeArrayIteratorClassName, "_length");
  intptr_t pos_offset =
      GetOffsetForField(kFixedSizeArrayIteratorClassName, "_pos");
  __ movl(EAX, Address(ESP, + 1 * kWordSize));     // Receiver.
  __ movl(EBX, FieldAddress(EAX, length_offset));  // Field _length.
  __ movl(EAX, FieldAddress(EAX, pos_offset));    // Field _pos.
  __ movl(EDI, EAX);
  __ orl(EDI, EBX);
  __ testl(EDI, Immediate(kSmiTagMask));
  __ j(NOT_ZERO, &fall_through, Assembler::kNearJump);  // Non-smi _length.
  __ cmpl(EBX, EAX);     // _length > _pos.
  __ j(GREATER, &is_true, Assembler::kNearJump);
  __ LoadObject(EAX, bool_false);
  __ ret();
  __ Bind(&is_true);
  __ LoadObject(EAX, bool_true);
  __ ret();
  __ Bind(&fall_through);
  return false;
}


#undef __


bool Intrinsifier::Intrinsify(const Function& function, Assembler* assembler) {
  if (!FLAG_intrinsify) return false;
  const char* function_name = String::Handle(function.name()).ToCString();
  const Class& function_class = Class::Handle(function.owner());
  const char* class_name = String::Handle(function_class.Name()).ToCString();
#define FIND_INTRINSICS(test_class_name, test_function_name, destination)      \
  if ((strcmp(#test_function_name, function_name) == 0) &&                     \
      (strcmp(#test_class_name, class_name) == 0)) {                           \
    return destination(assembler);                                             \
  }                                                                            \

INTRINSIC_LIST(FIND_INTRINSICS);
#undef FIND_INTRINSICS
  return false;
}

}  // namespace dart

#endif  // defined TARGET_ARCH_IA32
