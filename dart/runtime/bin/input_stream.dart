// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

/**
 * Input is read from a given input stream. Such an input stream can
 * be an endpoint, e.g., a socket or a file, or another input stream.
 * Multiple input streams can be chained together to operate collaboratively
 * on a given input.
 */
interface InputStream {
  /**
   * Reads data from the stream. Returns a system allocated buffer
   * with up to [len] bytes. If no value is passed for [len] all
   * available data will be returned. If no data is available null will
   * be returned.
   */
  List<int> read([int len]);

  /**
   * Reads up to [len] bytes into buffer [buffer] starting at offset
   * [offset]. Returns the number of bytes actually read which might
   * be zero. If [offset] is not specified 0 is used. If [len] is not
   * specified the length of [buffer] is used.
   */
  int readInto(List<int> buffer, [int offset, int len]);

  /**
   * Returns the number of bytes available for immediate reading.
   */
  int available();

  /**
   * Returns whether the stream has been closed. There might still be
   * more data to read.
   */
  bool closed();

  /**
   * Sets the data which handler gets called when data is available.
   */
  void set dataHandler(void callback());

  /**
   * The close handler gets called when the underlying communication
   * channel is closed and no more data will become available. Not all
   * types of communication channels will emit close events.
   */
  void set closeHandler(void callback());

  /**
   * The error handler gets called when the underlying communication
   * channel gets into some kind of error situation.
   */
  void set errorHandler(void callback());
}


interface StringInputStream factory _StringInputStream {
  /**
   * Decodes a binary input stream into characters using the specified
   * encoding.
   */
  StringInputStream(InputStream input, [String encoding]);

  /**
   * Reads as many characters as is available from the stream. If no data is
   * available null will be returned.
   */
  String read();

  /**
   * Reads the next line from the stream. The line ending characters
   * will not be part og the returned string. If a full line is not
   * available null will be returned.
   */
  String readLine();

  /**
   * Returns whether the stream has been closed. There might still be
   * more data to read.
   */
  bool get closed();

  /**
   * Returns the encoding used to decode the binary data into characters.
   */
  String get encoding();

  /**
   * The data handler gets called when data is available.
   */
  void set dataHandler(void callback());

  /**
   * The close handler gets called when the underlying communication
   * channel is closed and no more data will become available. Not all
   * types of communication channels will emit close events.
   */
  void set closeHandler(void callback());

  /**
   * The error handler gets called when the underlying communication
   * channel gets into some kind of error situation.
   */
  void set errorHandler(void callback());
}


class StreamException implements Exception {
  const StreamException([String this.message = ""]);
  String toString() => "StreamException: $message";
  final String message;
}
