// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

interface ServerSocket factory _ServerSocket {
  /*
   * Constructs a new server socket, binds it to a given address and port,
   * and listens on it.
   */
  ServerSocket(String bindAddress, int port, int backlog);

  /*
   * Accepts a connection to this socket.
   */
  Socket accept();

  /*
   * The connection handler gets executed when there are incoming connections
   * on the socket.
   */
  void setConnectionHandler(void callback());

  /*
   * The error handler gets executed when a socket error occurs.
   */
  void setErrorHandler(void callback());

  /*
   * Returns the port used by this socket.
   */
  int get port();

  /*
   * Closes the socket.
   */
  void close();
}


interface Socket factory _Socket {
  /*
   * Constructs a new socket and connects it to the given host on the given
   * port.
   */
  Socket(String host, int port);

  /*
   * Returns the number of received and non-read bytes in the socket that
   * can be read.
   */
  int available();

  /*
   * Reads up to [count] bytes of data from the socket and stores them into
   * buffer after buffer offset [offset]. The number of successfully read
   * bytes is returned. This function is non-blocking and will only read data
   * if data is available.
   */
  int readList(List<int> buffer, int offset, int count);

  /*
   * Writes up to [count] bytes of the buffer from [offset] buffer offset to
   * the socket. The number of successfully written bytes is returned. This
   * function is non-blocking and will only write data if buffer space is
   * available in the socket. It will return 0 if an error occurs, e.g., no
   * buffer space available.
   */
  int writeList(List<int> buffer, int offset, int count);

  /*
   * The connect handler gets executed when connection to a given host
   * succeeded.
   */
  void setConnectHandler(void callback());

  /*
   * The data handler gets executed when data becomes available at the socket.
   */
  void setDataHandler(void callback());

  /*
   * The write handler gets executed when the socket becomes available for
   * writing.
   */
  void setWriteHandler(void callback());

  /*
   * The error handler gets executed when a socket error occurs.
   */
  void setErrorHandler(void callback());

  /*
   * The close handler gets executed when the socket was closed.
   */
  void setCloseHandler(void callback());

  /*
   * Returns input stream to the socket.
   */
  InputStream get inputStream();

  /*
   * Returns output stream of the socket.
   */
  OutputStream get outputStream();

  /*
   * Returns the port used by this socket.
   */
  int get port();

  /*
   * Closes the socket
   */
  void close();
}
