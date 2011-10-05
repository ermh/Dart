/*
 * Copyright (c) 2011, the Dart project authors.
 * 
 * Licensed under the Eclipse Public License v1.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 * 
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package com.google.dart.tools.ui.feedback;

import com.google.dart.tools.ui.DartToolsPlugin;

import org.eclipse.core.runtime.Platform;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.RandomAccessFile;
import java.io.StringWriter;
import java.io.UnsupportedEncodingException;

/**
 * A utility for reading eclipse platform logs.
 */
public class LogReader {

  private static class TailInputStream extends InputStream {

    private final RandomAccessFile randomAccessFile;

    private final long maxLength;

    public TailInputStream(File file, long maxLength) throws IOException {
      super();
      this.maxLength = maxLength;
      randomAccessFile = new RandomAccessFile(file, "r"); //$NON-NLS-1$
      skipHead(file);
    }

    @Override
    public void close() throws IOException {
      randomAccessFile.close();
    }

    @Override
    public int read() throws IOException {
      byte[] b = new byte[1];
      int len = randomAccessFile.read(b, 0, 1);
      if (len < 0) {
        return len;
      }
      return b[0];
    }

    @Override
    public int read(byte[] b) throws IOException {
      return randomAccessFile.read(b, 0, b.length);
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException {
      return randomAccessFile.read(b, off, len);
    }

    private void skipHead(File file) throws IOException {
      if (file.length() > maxLength) {
        randomAccessFile.seek(file.length() - maxLength);
        // skip bytes until a new line to be sure we start from a beginning of valid UTF-8 character
        int c = read();
        while (c != '\n' && c != 'r' && c != -1) {
          c = read();
        }
      }
    }

  }

  public static final long MAX_FILE_LENGTH = 32 * 1024;

  private static final int DEFAULT_BUFFER_SIZE = 1024 * 4;

  /**
   * Tail the contents of the log into a String up to {@link #MAX_FILE_LENGTH} characters long.
   * 
   * @return the log contents
   * @throws UnsupportedEncodingException
   * @throws IOException
   */
  public static String readLog() throws UnsupportedEncodingException, IOException {
    File file = Platform.getLogFileLocation().toFile();
    return toString(new TailInputStream(file, MAX_FILE_LENGTH), "UTF-8");//$NON-NLS-1$
  }

  /**
   * Tail the contents of the log into a String up to {@link #MAX_FILE_LENGTH} characters long. If
   * an exception occurs in reading the log it is consumed, logged (if possible), and an error
   * string is returned.
   * 
   * @return the log contents, or an error string on fail
   */
  public static String readLogSafely() {
    try {
      return LogReader.readLog();
    } catch (Exception e) {
      DartToolsPlugin.log("unable to access log", e); //$NON-NLS-1$
      return "log unavailable: " + e.getMessage(); //$NON-NLS-1$
    }
  }

  private static String toString(InputStream input, String encoding) throws IOException {
    StringWriter out = new StringWriter();
    InputStreamReader in = new InputStreamReader(input);
    char[] buffer = new char[DEFAULT_BUFFER_SIZE];
    int n = 0;
    while (-1 != (n = in.read(buffer))) {
      out.write(buffer, 0, n);
    }
    return out.toString();
  }

}
