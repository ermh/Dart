// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "bin/socket.h"
#include "bin/dartutils.h"

#include "include/dart_api.h"


void FUNCTION_NAME(Socket_CreateConnect)(Dart_NativeArguments args) {
  Dart_EnterScope();
  Dart_Handle socketobj = Dart_GetNativeArgument(args, 0);
  const char* host =
      DartUtils::GetStringValue(Dart_GetNativeArgument(args, 1));
  intptr_t port = DartUtils::GetIntegerValue(Dart_GetNativeArgument(args, 2));
  intptr_t socket = Socket::CreateConnect(host, port);
  DartUtils::SetIntegerInstanceField(
      socketobj, DartUtils::kIdFieldName, socket);
  Dart_SetReturnValue(args, Dart_NewBoolean(socket >= 0));
  Dart_ExitScope();
}


void FUNCTION_NAME(Socket_Available)(Dart_NativeArguments args) {
  Dart_EnterScope();
  intptr_t socket =
      DartUtils::GetIntegerInstanceField(Dart_GetNativeArgument(args, 0),
                                      DartUtils::kIdFieldName);
  intptr_t available = Socket::Available(socket);
  Dart_SetReturnValue(args, Dart_NewInteger(available));
  Dart_ExitScope();
}


void FUNCTION_NAME(Socket_ReadList)(Dart_NativeArguments args) {
  Dart_EnterScope();
  intptr_t socket =
      DartUtils::GetIntegerInstanceField(Dart_GetNativeArgument(args, 0),
                                      DartUtils::kIdFieldName);
  Dart_Handle buffer_obj = Dart_GetNativeArgument(args, 1);
  assert(Dart_IsArray(buffer_obj));
  intptr_t offset =
      DartUtils::GetIntegerValue(Dart_GetNativeArgument(args, 2));
  intptr_t length =
      DartUtils::GetIntegerValue(Dart_GetNativeArgument(args, 3));
  Dart_Result result = Dart_GetLength(buffer_obj);
  assert(Dart_IsValidResult(result));
  assert((offset + length) <= Dart_GetResultAsCIntptr(result));

  if (Dart_IsVMFlagSet("short_socket_read")) {
    length = (length + 1) / 2;
  }

  uint8_t* buffer = new uint8_t[length];
  intptr_t bytes_read = Socket::Read(socket, buffer, length);
  if (bytes_read > 0) {
    Dart_Result result = Dart_ArraySet(buffer_obj, offset, buffer, bytes_read);
    ASSERT(Dart_IsValidResult(result));
  } else if (bytes_read < 0) {
    bytes_read = 0;
  }
  delete[] buffer;
  Dart_SetReturnValue(args, Dart_NewInteger(bytes_read));
  Dart_ExitScope();
}


void FUNCTION_NAME(Socket_WriteList)(Dart_NativeArguments args) {
  Dart_EnterScope();
  intptr_t socket =
      DartUtils::GetIntegerInstanceField(Dart_GetNativeArgument(args, 0),
                                      DartUtils::kIdFieldName);
  Dart_Handle buffer_obj = Dart_GetNativeArgument(args, 1);
  assert(Dart_IsArray(buffer_obj));
  intptr_t offset =
      DartUtils::GetIntegerValue(Dart_GetNativeArgument(args, 2));
  intptr_t length =
      DartUtils::GetIntegerValue(Dart_GetNativeArgument(args, 3));
  Dart_Result result = Dart_GetLength(buffer_obj);
  assert(Dart_IsValidResult(result));
  assert((offset + length) <= Dart_GetResultAsCIntptr(result));

  if (Dart_IsVMFlagSet("short_socket_write")) {
    length = (length + 1) / 2;
  }

  uint8_t* buffer = new uint8_t[length];
  result = Dart_ArrayGet(buffer_obj, offset, buffer, length);
  ASSERT(Dart_IsValidResult(result));
  intptr_t total_bytes_written =
      Socket::Write(socket, reinterpret_cast<void*>(buffer), length);
  delete[] buffer;
  if (total_bytes_written < 0) {
    total_bytes_written = 0;
  }
  Dart_SetReturnValue(args, Dart_NewInteger(total_bytes_written));
  Dart_ExitScope();
}


void FUNCTION_NAME(Socket_GetPort)(Dart_NativeArguments args) {
  Dart_EnterScope();
  intptr_t socket =
      DartUtils::GetIntegerInstanceField(Dart_GetNativeArgument(args, 0),
                                      DartUtils::kIdFieldName);
  intptr_t port = Socket::GetPort(socket);
  Dart_SetReturnValue(args, Dart_NewInteger(port));
  Dart_ExitScope();
}


void FUNCTION_NAME(ServerSocket_CreateBindListen)(Dart_NativeArguments args) {
  Dart_EnterScope();
  Dart_Handle socketobj = Dart_GetNativeArgument(args, 0);
  const char* bindAddress =
      DartUtils::GetStringValue(Dart_GetNativeArgument(args, 1));
  intptr_t port = DartUtils::GetIntegerValue(Dart_GetNativeArgument(args, 2));
  intptr_t backlog =
      DartUtils::GetIntegerValue(Dart_GetNativeArgument(args, 3));
  intptr_t socket =
      ServerSocket::CreateBindListen(bindAddress, port, backlog);
  DartUtils::SetIntegerInstanceField(
      socketobj, DartUtils::kIdFieldName, socket);
  Dart_SetReturnValue(args, Dart_NewBoolean(socket >= 0));
  Dart_ExitScope();
}


void FUNCTION_NAME(ServerSocket_Accept)(Dart_NativeArguments args) {
  Dart_EnterScope();
  intptr_t socket =
      DartUtils::GetIntegerInstanceField(Dart_GetNativeArgument(args, 0),
                                      DartUtils::kIdFieldName);
  Dart_Handle socketobj = Dart_GetNativeArgument(args, 1);
  intptr_t newSocket = ServerSocket::Accept(socket);
  if (newSocket >= 0) {
    DartUtils::SetIntegerInstanceField(
        socketobj, DartUtils::kIdFieldName, newSocket);
  }
  Dart_SetReturnValue(args, Dart_NewBoolean(newSocket >= 0));
  Dart_ExitScope();
}
