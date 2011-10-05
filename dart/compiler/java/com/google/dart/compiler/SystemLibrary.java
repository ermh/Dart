// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

package com.google.dart.compiler;

import java.io.File;
import java.net.URI;
import java.net.URISyntaxException;

/**
 * A library accessible via the "dart:<libname>.lib" protocol.
 */
public class SystemLibrary {

  private final String shortName;
  private final String host;
  private final String pathToLib;
  private final File dirOrZip;

  /**
   * Define a new system library such that dart:[shortLibName] will automatically be expanded to
   * dart://[host]/[pathToLib]. For example this call
   *
   * <pre>
   *    new SystemLibrary("dom.lib", "dom", "dart_dom.lib");
   * </pre>
   *
   * will define a new system library such that "dart:dom.lib" to automatically be expanded to
   * "dart://dom/dart_dom.lib". The dirOrZip argument is either the root directory or a zip file
   * containing all files for this library.
   */
  public SystemLibrary(String shortName, String host, String pathToLib, File dirOrZip) {
    this.shortName = shortName;
    this.host = host;
    this.pathToLib = pathToLib;
    this.dirOrZip = dirOrZip;
  }

  public String getHost() {
    return host;
  }

  public String getPathToLib() {
    return pathToLib;
  }

  public String getShortName() {
    return shortName;
  }

  public URI translateUri(URI dartUri) {
    if (!dirOrZip.exists()) {
      throw new RuntimeException("System library for " + dartUri + " does not exist: " + dirOrZip.getPath());
    }
    String spec = "file:" + dirOrZip.getPath();
    if (dirOrZip.isFile()) {
      spec = "jar:" + spec + "!";
    }
    try {
      return new URI(spec + dartUri.getPath());
    } catch (URISyntaxException e) {
      throw new AssertionError();
    }
  }
  
  public File getFile() {
    return this.dirOrZip;
  }
}
