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
package com.google.dart.tools.core.generator;

import com.google.dart.tools.core.DartCore;
import com.google.dart.tools.core.internal.util.Extensions;
import com.google.dart.tools.core.internal.util.ResourceUtil;
import com.google.dart.tools.core.internal.util.StatusUtil;
import com.google.dart.tools.core.model.DartLibrary;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.runtime.Assert;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.SubMonitor;

import java.io.File;
import java.text.MessageFormat;
import java.util.HashMap;

/**
 * Instances of <code>ApplicationGenerator</code> are used to create a new Application in an after
 * validating the name and location.
 */
public class ApplicationGenerator extends AbstractGenerator {

  public static final String HTML_FILENAME_EXTENSION = ".html"; //$NON-NLS-1$

  public static final String DESCRIPTION = GeneratorMessages.ApplicationGenerator_description;

  private String applicationName;

  private String applicationLocation;

  private IFile iApplicationFile = null;

  /**
   * Construct a new instance.
   */
  public ApplicationGenerator() {
  }

  /**
   * Create the folder to contain the library and the library declaration file
   * 
   * @param monitor the monitor to which activity is reported
   */
  @Override
  public void execute(IProgressMonitor monitor) throws CoreException {
    // Sanity Check
    Assert.isNotNull(applicationName);
    Assert.isNotNull(applicationLocation);
    IStatus status = validate();
    if (status.getSeverity() == IStatus.ERROR) {
      throw new IllegalStateException(status.getMessage());
    }

    String applicationFileName = appendIfNoExtension(applicationName, Extensions.DOT_DART);

    SubMonitor subMonitor = SubMonitor.convert(monitor,
        GeneratorMessages.ApplicationGenerator_message, 100);
    String className = applicationFileName.substring(0, applicationFileName.indexOf('.'));
    final HashMap<String, String> substitutions = new HashMap<String, String>();
    substitutions.put("className", className); //$NON-NLS-1$
    substitutions.put("extends", ""); //$NON-NLS-1$ //$NON-NLS-2$
    substitutions.put("implements", ""); //$NON-NLS-1$ //$NON-NLS-2$

    File applicationFile = getSystemFile(applicationFileName);
    execute("generated-dart-class-main.txt", applicationFile, substitutions, monitor); //$NON-NLS-1$
    subMonitor.newChild(100);
    subMonitor.done();

    // html file
    subMonitor = SubMonitor.convert(monitor,
        GeneratorMessages.ApplicationGenerator_htmlFileMessage, 100);
    String htmlFileName = appendIfNoExtension(applicationName, HTML_FILENAME_EXTENSION);
    File iHtmlFile = getSystemFile(htmlFileName);
    substitutions.put("title", className);
    substitutions.put("dartPath", className + ".app.js");
    execute("generated-html.txt", iHtmlFile, substitutions, monitor); //$NON-NLS-1$
    subMonitor.newChild(100);
    subMonitor.done();

    DartLibrary library = DartCore.openLibrary(applicationFile, monitor);
    IFile[] files = ResourceUtil.getResources(applicationFile);
    iApplicationFile = files[0];

  }

  public IFile getFile() {

    return iApplicationFile;
  }

  public String getLibraryFileName() {
    return applicationName;
  }

  /**
   * Answer the file to be created
   * 
   * @return the file or <code>null</code> if a file cannot be created
   */

  public File getSystemFile(String fileName) {
    // Fail fast for null elements
    Assert.isNotNull(fileName);
    return new File(applicationLocation + File.separator + fileName);
  }

  public void setApplicationLocation(String applicationLocation) {
    this.applicationLocation = applicationLocation;
  }

  public void setApplicationName(String applicationName) {
    this.applicationName = applicationName;
  }

  /**
   * Checks that the library location and file name are both valid.
   * 
   * @see #validateLocation()
   * @see #validateName()
   * @return IStatus corresponding to any errors/warnings that would make the lib/app invalid
   */
  @Override
  public IStatus validate() {
    IStatus status = StatusUtil.getMoreSevere(Status.OK_STATUS, validateLocation());
    status = StatusUtil.getMoreSevere(status, validateName());
    return status;
  }

  /**
   * Validate the application location.
   * 
   * @return {@link Status#OK_STATUS} if the name is valid, or a status indicating an error or
   *         warning.
   */
  private IStatus validateLocation() {
    // Validate that:
    // 1) The file is non-empty
    // 2) The directory is a valid path
    if (applicationLocation == null || applicationLocation.isEmpty()) {
      return new Status(IStatus.ERROR, DartCore.PLUGIN_ID, DESCRIPTION);
    }
    Path libraryPath = new Path(applicationLocation);
    if (!libraryPath.isAbsolute() || !libraryPath.isValidPath(applicationLocation)) {
      return new Status(IStatus.ERROR, DartCore.PLUGIN_ID,
          GeneratorMessages.ApplicationGenerator_directoryMessage);
    }
    return Status.OK_STATUS;
  }

  /**
   * Validate the application name.
   * 
   * @return {@link Status#OK_STATUS} if the name is valid, or a status indicating an error or
   *         warning.
   */
  private IStatus validateName() {
    // Validate that:
    // 1) The file is non-empty;
    // 2) The file does not contain any whitespace;
    // 3) The file does not exist yet.
    if (applicationName == null || applicationName.isEmpty()) {
      return new Status(IStatus.ERROR, DartCore.PLUGIN_ID, DESCRIPTION);
    } else if (containsWhitespace(applicationName)) {
      return new Status(IStatus.ERROR, DartCore.PLUGIN_ID,
          GeneratorMessages.ApplicationGenerator_noWhiteSpace);
    }
    Path path = new Path(applicationLocation);
    if (path.append(appendIfNoExtension(applicationName, Extensions.DOT_DART)).toFile().exists()) {
      return new Status(IStatus.ERROR, DartCore.PLUGIN_ID, MessageFormat.format(
          GeneratorMessages.ApplicationGenerator_fileExists, new Object[] {applicationName
              + Extensions.DOT_DART}));
    }
    if (path.append(appendIfNoExtension(applicationName, HTML_FILENAME_EXTENSION)).toFile().exists()) {
      return new Status(IStatus.ERROR, DartCore.PLUGIN_ID, MessageFormat.format(
          GeneratorMessages.ApplicationGenerator_fileExists, new Object[] {applicationName
              + HTML_FILENAME_EXTENSION}));
    }
    IStatus status = DartIdentifierUtil.validateIdentifier(applicationName);
    if (status != Status.OK_STATUS) {
      return status;
    }
    return Status.OK_STATUS;
  }

}
