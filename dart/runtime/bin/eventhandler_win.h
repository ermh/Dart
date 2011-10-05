// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef BIN_EVENTHANDLER_WIN_H_
#define BIN_EVENTHANDLER_WIN_H_

#include <winsock2.h>
#include <mswsock.h>

#include "bin/builtin.h"


// Forward declarations.
class EventHandlerImplementation;
class Handle;
class FileHandle;
class SocketHandle;
class ClientSocket;
class ListenSocket;


struct InterruptMessage {
  intptr_t id;
  Dart_Port dart_port;
  int64_t data;
};


// An IOBuffer encapsulates the OVERLAPPED structure and the
// associated data buffer. For accept it also contains the pre-created
// socket for the client.
class IOBuffer {
 public:
  enum Operation { kAccept, kRead, kWrite };

  static IOBuffer* AllocateAcceptBuffer(int buffer_size);
  static IOBuffer* AllocateReadBuffer(int buffer_size);
  static IOBuffer* AllocateWriteBuffer(int buffer_size);
  static void DisposeBuffer(IOBuffer* buffer);

  // Find the IO buffer from the OVERLAPPED address.
  static IOBuffer* GetFromOverlapped(OVERLAPPED* overlapped);

  // Read data from a buffer which has been received. It will read up
  // to num_bytes bytes of data returning the actual number of bytes
  // read. This will update the index of the next byte in the buffer
  // so calling Read several times will keep returning new data from
  // the buffer until all data have been read.
  int Read(void* buffer, int num_bytes);

  // Write data to a buffer before sending it. Returns the number of bytes
  // actually written to the buffer. Calls to Write will always write to
  // the buffer from the begining.
  int Write(const void* buffer, int num_bytes);

  // Check the amount of data in a read buffer which has not been read yet.
  int GetRemainingLength();
  bool IsEmpty() { return GetRemainingLength() == 0; }

  Operation operation() { return operation_; }
  SOCKET client() { return client_; }
  char* GetBufferStart() { return reinterpret_cast<char*>(&buffer_data_); }
  int GetBufferSize() { return buflen_; }

  // Returns the address of the OVERLAPPED structure with all fields
  // initialized to zero.
  OVERLAPPED* GetCleanOverlapped() {
    memset(&overlapped_, 0, sizeof(overlapped_));
    return &overlapped_;
  }

  // Returns a WASBUF structure initialized with the data in this IO buffer.
  WSABUF* GetWASBUF() {
    wbuf_.buf = GetBufferStart();
    wbuf_.len = GetBufferSize();
    return &wbuf_;
  };

  void set_data_length(int data_length) { data_length_ = data_length; }

 private:
  IOBuffer(int buffer_size, Operation operation)
      : operation_(operation), buflen_(buffer_size) {
    memset(GetBufferStart(), 0, GetBufferSize());
    index_ = 0;
    data_length_ = 0;
    if (operation_ == kAccept) {
      client_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
  }

  void* operator new(size_t size, int buffer_size) {
    return malloc(size + buffer_size);
  }

  void operator delete(void* buffer) {
    free(buffer);
  }

  static IOBuffer* AllocateBuffer(int buffer_size, Operation operation);

  OVERLAPPED overlapped_;  // OVERLAPPED structure for overlapped IO.
  SOCKET client_;  // Used for AcceptEx client socket.
  int buflen_;  // Length of the buffer.
  Operation operation_;  // Type of operation issued.

  int index_;  // Index for next read from read buffer.
  int data_length_;  // Length of the actual data in the buffer.

  WSABUF wbuf_;  // Structure for passing buffer to WSA functions.

  // Buffer for recv/send/AcceptEx. This must be at the end of the
  // object as the object is allocated larger than it's definition
  // indicate to extend this array.
  uint8_t buffer_data_[1];
};


// Abstract super class for holding information on listen and connected
// sockets.
class Handle {
 public:
  enum Type { kFile, kClientSocket, kListenSocket };

  class ScopedLock {
   public:
    explicit ScopedLock(Handle* handle)
        : handle_(handle) {
      handle_->Lock();
    }
    ~ScopedLock() {
      handle_->Unlock();
    }

   private:
    Handle* handle_;
  };

  virtual ~Handle();

  // Internal interface used by the event handler.
  virtual bool IssueRead();
  virtual bool IssueWrite();
  bool HasPendingRead();
  bool HasPendingWrite();
  void ReadComplete(IOBuffer* buffer);
  void WriteComplete(IOBuffer* buffer);

  virtual void EnsureInitialized(
    EventHandlerImplementation* event_handler) = 0;

  HANDLE handle() { return handle_; }
  Dart_Port port() { return port_; }
  EventHandlerImplementation* event_handler() { return event_handler_; }

  void Lock();
  void Unlock();

  bool CreateCompletionPort(HANDLE completion_port);

  void close();
  virtual bool IsClosed() = 0;

  void SetPortAndMask(Dart_Port port, intptr_t mask) {
    port_ = port;
    mask_ = mask;
  }
  Type type() { return type_; }
  bool is_file() { return type_ == kFile; }
  bool is_socket() { return type_ == kListenSocket || type_ == kClientSocket; }
  bool is_listen_socket() { return type_ == kListenSocket; }
  bool is_client_socket() { return type_ == kClientSocket; }
  intptr_t mask() { return mask_; }

  bool is_closing() { return closing_; }

 protected:
  explicit Handle(HANDLE handle);
  Handle(HANDLE handle, Dart_Port port);

  virtual void AfterClose() = 0;

  Type type_;
  HANDLE handle_;
  bool closing_;  // Is this handle in the process of closing?
  Dart_Port port_;  // Dart port to communicate events for this socket.
  intptr_t mask_;  // Mask of events to report through the port.
  HANDLE completion_port_;
  CRITICAL_SECTION cs_;  // Critical section protecting this object.
  EventHandlerImplementation* event_handler_;

  IOBuffer* data_ready_;  // IO buffer for data ready to be read.
  IOBuffer* pending_read_;  // IO buffer for pending read.
  IOBuffer* pending_write_;  // IO buffer for pending write
};


class FileHandle : public Handle {
 public:
  explicit FileHandle(HANDLE handle)
      : Handle(reinterpret_cast<HANDLE>(handle)) { type_ = kFile; }
  FileHandle(HANDLE handle, Dart_Port port)
      : Handle(reinterpret_cast<HANDLE>(handle), port) { type_ = kFile; }
};


class SocketHandle : public Handle {
 public:
  SOCKET socket() { return reinterpret_cast<SOCKET>(handle_); }

 protected:
  explicit SocketHandle(SOCKET s) : Handle(reinterpret_cast<HANDLE>(s)) {}
  SocketHandle(SOCKET s, Dart_Port port)
      : Handle(reinterpret_cast<HANDLE>(s), port) {}
};


// Information on listen sockets.
class ListenSocket : public SocketHandle {
 public:
  explicit ListenSocket(SOCKET s) : SocketHandle(s),
                                    AcceptEx_(NULL),
                                    pending_accept_count_(0),
                                    accepted_head_(NULL),
                                    accepted_tail_(NULL) {
    type_ = kListenSocket;
  }
  virtual ~ListenSocket() {
    ASSERT(!HasPendingAccept());
    ASSERT(accepted_head_ == NULL);
    ASSERT(accepted_tail_ == NULL);
  };

  // Socket interface exposing normal socket operations.
  ClientSocket* Accept();

  // Internal interface used by the event handler.
  bool HasPendingAccept() { return pending_accept_count_ > 0; }
  bool IssueAccept();
  void AcceptComplete(IOBuffer* buffer, HANDLE completion_port);

  virtual void EnsureInitialized(
    EventHandlerImplementation* event_handler);
  virtual bool IsClosed();

  int pending_accept_count() { return pending_accept_count_; }

 private:
  bool LoadAcceptEx();
  virtual void AfterClose();

  LPFN_ACCEPTEX AcceptEx_;
  int pending_accept_count_;
  // Linked list of accepted connections provided by completion code. Ready to
  // be handed over through accept.
  ClientSocket* accepted_head_;
  ClientSocket* accepted_tail_;
};


// Information on connected sockets.
class ClientSocket : public SocketHandle {
 public:
  explicit ClientSocket(SOCKET s)
      : SocketHandle(s),
        next_(NULL) { type_ = kClientSocket; }

  ClientSocket(SOCKET s, Dart_Port port)
      : SocketHandle(s, port),
        next_(NULL) { type_ = kClientSocket; }

  virtual ~ClientSocket() {
    // Don't delete this object until all pending requests have been handled.
    ASSERT(!HasPendingRead());
    ASSERT(!HasPendingWrite());
    ASSERT(next_ == NULL);
  };

  // Socket interface exposing normal socket operations.
  int Available();
  int Read(void* buffer, int num_bytes);
  int Write(const void* buffer, int num_bytes);

  // Internal interface used by the event handler.
  virtual bool IssueRead();
  virtual bool IssueWrite();

  virtual void EnsureInitialized(
    EventHandlerImplementation* event_handler);
  virtual bool IsClosed();

  ClientSocket* next() { return next_; }
  void set_next(ClientSocket* next) { next_ = next; }

 private:
  virtual void AfterClose();

  ClientSocket* next_;
};


// Event handler.
class EventHandlerImplementation {
 public:
  EventHandlerImplementation();
  virtual ~EventHandlerImplementation() {}

  void SendData(intptr_t id, Dart_Port dart_port, intptr_t data);
  void StartEventHandler();

  DWORD GetTimeout();
  void HandleInterrupt(InterruptMessage* msg);
  void HandleTimeout();
  void HandleAccept(ListenSocket* listen_socket, IOBuffer* buffer);
  void HandleClosed(Handle* handle);
  void HandleRead(ClientSocket* client_socket, int bytes, IOBuffer* buffer);
  void HandleWrite(ClientSocket* client_socket, int bytes, IOBuffer* buffer);
  void HandleClose(ClientSocket* client_socket);
  void HandleIOCompletion(DWORD bytes, ULONG_PTR key, OVERLAPPED* overlapped);

  HANDLE completion_port() { return completion_port_; }

 private:
  ClientSocket* client_sockets_head_;

  int64_t timeout_;  // Time for next timeout.
  Dart_Port timeout_port_;
  HANDLE completion_port_;
};


#endif  // BIN_EVENTHANDLER_WIN_H_
