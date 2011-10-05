// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bin/fdutils.h"
#include "bin/socket.h"


bool Socket::Initialize() {
  // Nothing to do on Linux.
  return true;
}


intptr_t Socket::CreateConnect(const char* host, const intptr_t port) {
  intptr_t fd;
  struct hostent* server;
  struct sockaddr_in server_address;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    fprintf(stderr, "Error CreateConnect: %s\n", strerror(errno));
    return -1;
  }

  FDUtils::SetNonBlocking(fd);

  server = gethostbyname(host);
  if (server == NULL) {
    close(fd);
    fprintf(stderr, "Error CreateConnect: %s\n", strerror(errno));
    return -1;
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  bcopy(server->h_addr, &server_address.sin_addr.s_addr, server->h_length);
  memset(&server_address.sin_zero, 0, sizeof(server_address.sin_zero));
  intptr_t result = connect(fd,
      reinterpret_cast<struct sockaddr *>(&server_address),
      sizeof(server_address));
  if (result == 0 || errno == EINPROGRESS) {
    return fd;
  }
  return -1;
}


intptr_t Socket::Available(intptr_t fd) {
  return FDUtils::AvailableBytes(fd);
}


intptr_t Socket::Read(intptr_t fd,
                      void* buffer,
                      intptr_t num_bytes) {
  assert(fd >= 0);
  intptr_t read_bytes = read(fd, buffer, num_bytes);
  return read_bytes;
}


intptr_t Socket::Write(intptr_t fd, const void* buffer, intptr_t num_bytes) {
  assert(fd >= 0);
  return write(fd, buffer, num_bytes);
}


intptr_t Socket::GetPort(intptr_t fd) {
  assert(fd >= 0);
  struct sockaddr_in socket_address;
  socklen_t size = sizeof(socket_address);
  if (getsockname(fd, reinterpret_cast<struct sockaddr *>(&socket_address),
                  &size)) {
    fprintf(stderr, "Error getsockname: %s\n", strerror(errno));
    return 0;
  }
  return ntohs(socket_address.sin_port);
}


intptr_t ServerSocket::CreateBindListen(const char* host,
                                  intptr_t port,
                                  intptr_t backlog) {
  intptr_t fd;
  struct sockaddr_in server_address;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    fprintf(stderr, "Error CreateBind: %s\n", strerror(errno));
    return -1;
  }

  int optval = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = inet_addr(host);
  memset(&server_address.sin_zero, 0, sizeof(server_address.sin_zero));

  if (bind(fd, reinterpret_cast<struct sockaddr *>(&server_address),
           sizeof(server_address)) < 0) {
    close(fd);
    fprintf(stderr, "Error Bind: %s\n", strerror(errno));
    return -1;
  }

  if (listen(fd, backlog) != 0) {
    fprintf(stderr, "Error Listen: %s\n", strerror(errno));
    return -1;
  }

  FDUtils::SetNonBlocking(fd);
  return fd;
}

intptr_t ServerSocket::Accept(intptr_t fd) {
  intptr_t socket;
  struct sockaddr clientaddr;
  socklen_t addrlen = sizeof(clientaddr);
  socket = accept(fd, &clientaddr, &addrlen);
  if (socket < 0) {
    fprintf(stderr, "Error Accept: %s\n", strerror(errno));
  } else {
    FDUtils::SetNonBlocking(socket);
  }
  return socket;
}
