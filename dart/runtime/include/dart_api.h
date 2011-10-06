// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef INCLUDE_DART_API_H_
#define INCLUDE_DART_API_H_

#ifdef __cplusplus
#define DART_EXTERN_C extern "C"
#else
#define DART_EXTERN_C
#endif

#if defined(__CYGWIN__)
#error Tool chain and platform not supported.
#elif defined(_WIN32)
typedef signed __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#if defined(DART_SHARED_LIB)
#define DART_EXPORT DART_EXTERN_C __declspec(dllexport)
#else
#define DART_EXPORT DART_EXTERN_C
#endif
#else
#include <inttypes.h>
#if __GNUC__ >= 4
#if defined(DART_SHARED_LIB)
#define DART_EXPORT DART_EXTERN_C __attribute__ ((visibility("default")))
#else
#define DART_EXPORT DART_EXTERN_C
#endif
#else
#error Tool chain not supported.
#endif
#endif

#include <assert.h>

typedef void* Dart_Isolate;
typedef void* Dart_Handle;
typedef void* Dart_NativeArguments;
typedef enum {
  kRetCIntptr = 0,
  kRetCBool,
  kRetCString,
  kRetObject,
  kRetCInt64,
  kRetCDouble,
  kRetLibSpec,
  kRetError,
} Dart_ReturnType;

typedef struct {
  Dart_ReturnType type_;
  union {
    intptr_t int_value;
    bool bool_value;
    const char* str_value;
    Dart_Handle obj_value;
    int64_t int64_value;
    double double_value;
    const char* errmsg;
  } retval_;
} Dart_Result;

typedef enum {
  kLibraryTag = 0,
  kImportTag,
  kSourceTag,
  kCanonicalizeUrl,
} Dart_LibraryTag;

typedef void Dart_Snapshot;

typedef int64_t Dart_Port;

// Allow the embedder to intercept isolate creation. Both at startup and when
// spawning new isolates from Dart code.
// The result returned from this callback is handed to all isolates spawned
// from the isolate currently being initialized.
// Return NULL if an error is encountered. The isolate being initialized will
// be shutdown. No Dart code will execute in before it is shutdown.
// TODO(iposva): Pass a specification of the app file being spawned.
typedef void* (*Dart_IsolateInitCallback)(void* data);

typedef void (*Dart_NativeFunction)(Dart_NativeArguments arguments);
typedef Dart_NativeFunction (*Dart_NativeEntryResolver)(Dart_Handle name,
                                                        int num_of_arguments);
typedef Dart_Result (*Dart_LibraryTagHandler)(Dart_LibraryTag tag,
                                              Dart_Handle library,
                                              Dart_Handle url);

// TODO(iposva): This is a placeholder for the eventual external Dart API.

// Return value handling after a Dart API call.
inline bool Dart_IsValidResult(const Dart_Result& result) {
  return (result.type_ != kRetError);
}
inline intptr_t Dart_GetResultAsCIntptr(const Dart_Result& result) {
  assert(result.type_ == kRetCIntptr);  // Valid to access result as C int.
  return result.retval_.int_value;
}
inline  Dart_Result Dart_ResultAsCIntptr(intptr_t value) {
  Dart_Result result;
  result.type_ = kRetCIntptr;
  result.retval_.int_value = value;
  return result;
}
inline bool Dart_GetResultAsCBoolean(const Dart_Result& result) {
  assert(result.type_ == kRetCBool);  // Valid to access result as C bool.
  return result.retval_.bool_value;
}
inline  Dart_Result Dart_ResultAsCBoolean(bool value) {
  Dart_Result result;
  result.type_ = kRetCBool;
  result.retval_.bool_value = value;
  return result;
}
inline const char* Dart_GetResultAsCString(const Dart_Result& result) {
  assert(result.type_ == kRetCString);  // Valid to access result as C string.
  return result.retval_.str_value;
}
inline  Dart_Result Dart_ResultAsCString(const char* value) {
  // Assumes passed in string value will be available on return.
  Dart_Result result;
  result.type_ = kRetCString;
  result.retval_.str_value = value;
  return result;
}
inline Dart_Handle Dart_GetResult(const Dart_Result& result) {
  assert(result.type_ == kRetObject);  // Valid to access result as Dart obj.
  return result.retval_.obj_value;
}
inline  Dart_Result Dart_ResultAsObject(Dart_Handle value) {
  Dart_Result result;
  result.type_ = kRetObject;
  result.retval_.obj_value = value;
  return result;
}
inline int64_t Dart_GetResultAsCInt64(const Dart_Result& result) {
  assert(result.type_ == kRetCInt64);  // Valid to access result as C int64_t.
  return result.retval_.int64_value;
}
inline  Dart_Result Dart_ResultAsCInt64(int64_t value) {
  Dart_Result result;
  result.type_ = kRetCInt64;
  result.retval_.int64_value = value;
  return result;
}
inline double Dart_GetResultAsCDouble(const Dart_Result& result) {
  assert(result.type_ == kRetCDouble);  // Valid to access result as C double.
  return result.retval_.double_value;
}
inline  Dart_Result Dart_ResultAsCDouble(double value) {
  Dart_Result result;
  result.type_ = kRetCDouble;
  result.retval_.double_value = value;
  return result;
}
inline const char* Dart_GetErrorCString(const Dart_Result& result) {
  assert(result.type_ == kRetError);  // Valid to access only on failure.
  return result.retval_.errmsg;
}
inline  Dart_Result Dart_ErrorResult(const char* value) {
  // Assumes passed in string value will be available on return.
  Dart_Result result;
  result.type_ = kRetError;
  result.retval_.errmsg = value;
  return result;
}


// Initialize the VM with commmand line flags.
DART_EXPORT bool Dart_Initialize(int argc, char** argv,
                                 Dart_IsolateInitCallback callback);


// Isolate handling.
DART_EXPORT Dart_Isolate Dart_CreateIsolate(const Dart_Snapshot* snapshot,
                                            void* data);
DART_EXPORT void Dart_ShutdownIsolate();

DART_EXPORT Dart_Isolate Dart_CurrentIsolate();
DART_EXPORT void Dart_EnterIsolate(Dart_Isolate isolate);
DART_EXPORT void Dart_ExitIsolate();

DART_EXPORT Dart_Result Dart_RunLoop();


// Object.
DART_EXPORT Dart_Result Dart_ObjectToString(Dart_Handle object);
DART_EXPORT bool Dart_IsNull(Dart_Handle object);


// Returns true if the two objects are equal.
DART_EXPORT Dart_Result Dart_Objects_Equal(Dart_Handle obj1, Dart_Handle obj2);


// Classes.
DART_EXPORT Dart_Result Dart_GetClass(Dart_Handle library, Dart_Handle name);
DART_EXPORT Dart_Result Dart_IsInstanceOf(Dart_Handle object, Dart_Handle cls);


// Number.
DART_EXPORT bool Dart_IsNumber(Dart_Handle object);


// Integer.
DART_EXPORT bool Dart_IsInteger(Dart_Handle object);
DART_EXPORT Dart_Handle Dart_NewInteger(int64_t value);
DART_EXPORT Dart_Handle Dart_NewIntegerFromHexCString(const char* value);
DART_EXPORT Dart_Result Dart_IntegerValue(Dart_Handle integer);
DART_EXPORT Dart_Result Dart_IntegerFitsIntoInt64(Dart_Handle integer);


// Boolean.
DART_EXPORT bool Dart_IsBoolean(Dart_Handle object);
DART_EXPORT Dart_Handle Dart_NewBoolean(bool value);
DART_EXPORT Dart_Result Dart_BooleanValue(Dart_Handle bool_object);


// Double.
DART_EXPORT bool Dart_IsDouble(Dart_Handle object);
DART_EXPORT Dart_Handle Dart_NewDouble(double value);
DART_EXPORT Dart_Result Dart_DoubleValue(Dart_Handle integer);


// String.
DART_EXPORT bool Dart_IsString(Dart_Handle object);

DART_EXPORT Dart_Result Dart_StringLength(Dart_Handle str);

DART_EXPORT Dart_Handle Dart_NewString(const char* str);
DART_EXPORT Dart_Handle Dart_NewString8(const uint8_t* codepoints,
                                        intptr_t length);
DART_EXPORT Dart_Handle Dart_NewString16(const uint16_t* codepoints,
                                         intptr_t length);
DART_EXPORT Dart_Handle Dart_NewString32(const uint32_t* codepoints,
                                         intptr_t length);

// The functions below test whether the object is a String and its codepoints
// all fit into 8 or 16 bits respectively.
DART_EXPORT bool Dart_IsString8(Dart_Handle object);
DART_EXPORT bool Dart_IsString16(Dart_Handle object);

DART_EXPORT Dart_Result Dart_StringGet8(Dart_Handle str,
                                        uint8_t* codepoints,
                                        intptr_t length);
DART_EXPORT Dart_Result Dart_StringGet16(Dart_Handle str,
                                         uint16_t* codepoints,
                                         intptr_t length);
DART_EXPORT Dart_Result Dart_StringGet32(Dart_Handle str,
                                         uint32_t* codepoints,
                                         intptr_t length);

DART_EXPORT Dart_Result Dart_StringToCString(Dart_Handle str);


// Array.
DART_EXPORT bool Dart_IsArray(Dart_Handle object);
DART_EXPORT Dart_Handle Dart_NewArray(intptr_t length);
DART_EXPORT Dart_Result Dart_GetLength(Dart_Handle array);
DART_EXPORT Dart_Result Dart_ArrayGetAt(Dart_Handle array,
                                        intptr_t index);
DART_EXPORT Dart_Result Dart_ArrayGet(Dart_Handle array,
                                      intptr_t offset,
                                      uint8_t* native_array,
                                      intptr_t length);
DART_EXPORT Dart_Result Dart_ArraySetAt(Dart_Handle array,
                                        intptr_t index,
                                        Dart_Handle value);
DART_EXPORT Dart_Result Dart_ArraySet(Dart_Handle array,
                                      intptr_t offset,
                                      uint8_t* native_array,
                                      intptr_t length);

// Closure.
DART_EXPORT bool Dart_IsClosure(Dart_Handle object);
// DEPRECATED: The API below is a temporary hack.
DART_EXPORT Dart_Result Dart_ClosureSmrck(Dart_Handle object);
DART_EXPORT void Dart_ClosureSetSmrck(Dart_Handle object, int64_t value);


// Invocation of methods.
DART_EXPORT Dart_Result Dart_InvokeStatic(Dart_Handle library,
                                          Dart_Handle class_name,
                                          Dart_Handle function_name,
                                          int number_of_arguments,
                                          Dart_Handle* arguments);
DART_EXPORT Dart_Result Dart_InvokeDynamic(Dart_Handle receiver,
                                           Dart_Handle function_name,
                                           int number_of_arguments,
                                           Dart_Handle* arguments);
DART_EXPORT Dart_Result Dart_InvokeClosure(Dart_Handle closure,
                                           int number_of_arguments,
                                           Dart_Handle* arguments);


// Interaction with native methods.
DART_EXPORT Dart_Handle Dart_GetNativeArgument(Dart_NativeArguments args,
                                               int index);
DART_EXPORT void Dart_SetReturnValue(Dart_NativeArguments args,
                                     Dart_Handle retval);

// Library.
DART_EXPORT bool Dart_IsLibrary(Dart_Handle object);
DART_EXPORT Dart_Result Dart_LibraryUrl(Dart_Handle library);
DART_EXPORT Dart_Result Dart_LibraryImportLibrary(Dart_Handle library,
                                                  Dart_Handle import);

DART_EXPORT Dart_Result Dart_LookupLibrary(Dart_Handle url);

DART_EXPORT Dart_Result Dart_LoadLibrary(Dart_Handle url,
                                         Dart_Handle source);
DART_EXPORT Dart_Result Dart_LoadSource(Dart_Handle library,
                                        Dart_Handle url,
                                        Dart_Handle source);
DART_EXPORT Dart_Result Dart_SetNativeResolver(
    Dart_Handle library,
    Dart_NativeEntryResolver resolver);


// Script handling.
DART_EXPORT Dart_Result Dart_LoadScript(Dart_Handle url,
                                        Dart_Handle source,
                                        Dart_LibraryTagHandler handler);

// Compile all loaded classes and functions eagerly.
DART_EXPORT Dart_Result Dart_CompileAll();

// Exception related.
DART_EXPORT bool Dart_ExceptionOccurred(Dart_Handle result);
DART_EXPORT Dart_Result Dart_GetException(Dart_Handle result);
DART_EXPORT Dart_Result Dart_GetStacktrace(Dart_Handle unhandled_exception);
DART_EXPORT Dart_Result Dart_ThrowException(Dart_Handle exception);
DART_EXPORT Dart_Result Dart_ReThrowException(Dart_Handle exception,
                                              Dart_Handle stacktrace);

// Global Handles and Scope for local handles and zone based memory allocation.
DART_EXPORT void Dart_EnterScope();
DART_EXPORT void Dart_ExitScope();

DART_EXPORT Dart_Handle Dart_NewPersistentHandle(Dart_Handle object);
DART_EXPORT Dart_Handle Dart_MakeWeakPersistentHandle(Dart_Handle object);
DART_EXPORT Dart_Handle Dart_MakePersistentHandle(Dart_Handle object);
DART_EXPORT void Dart_DeletePersistentHandle(Dart_Handle object);

// Fields.
DART_EXPORT Dart_Result Dart_GetStaticField(Dart_Handle cls, Dart_Handle name);
DART_EXPORT Dart_Result Dart_SetStaticField(Dart_Handle cls,
                                            Dart_Handle name,
                                            Dart_Handle value);
DART_EXPORT Dart_Result Dart_GetInstanceField(Dart_Handle obj,
                                              Dart_Handle name);
DART_EXPORT Dart_Result Dart_SetInstanceField(Dart_Handle obj,
                                              Dart_Handle name,
                                              Dart_Handle value);

// Native fields.
DART_EXPORT Dart_Result Dart_CreateNativeWrapperClass(Dart_Handle library,
                                                      Dart_Handle class_name,
                                                      int field_count);
DART_EXPORT Dart_Result Dart_GetNativeInstanceField(Dart_Handle obj,
                                                    int index);
DART_EXPORT Dart_Result Dart_SetNativeInstanceField(Dart_Handle obj,
                                                    int index,
                                                    intptr_t value);

// Snapshot creation.
DART_EXPORT Dart_Result Dart_CreateSnapshot(uint8_t** snaphot_buffer,
                                            intptr_t* snapshot_size);

// Message communication.
DART_EXPORT Dart_Result Dart_PostIntArray(Dart_Port port,
                                          int field_count,
                                          intptr_t* data);

DART_EXPORT Dart_Result Dart_Post(Dart_Port port, Dart_Handle value);

// External pprof support for gathering and dumping symbolic information
// that can be used for better profile reports for dynamically generated
// code.
DART_EXPORT void Dart_InitPprofSupport();
DART_EXPORT void Dart_GetPprofSymbolInfo(void** buffer, int* buffer_size);

// Check set vm flags.
DART_EXPORT bool Dart_IsVMFlagSet(const char* flag_name);

#endif  // INCLUDE_DART_API_H_
