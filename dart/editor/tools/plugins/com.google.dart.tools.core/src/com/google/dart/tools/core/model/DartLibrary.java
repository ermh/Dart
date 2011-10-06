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
package com.google.dart.tools.core.model;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.runtime.IProgressMonitor;

import java.io.File;
import java.net.URI;

/**
 * The interface <code>DartLibrary</code> defines the behavior of objects representing a Dart
 * library. A Dart library is an artificial container for a single {@link LibraryConfigurationFile}
 * and all of the compilation units that are contained in it.
 */
public interface DartLibrary extends OpenableElement, ParentElement {
  /**
   * An empty array of libraries.
   */
  public static final DartLibrary[] EMPTY_LIBRARY_ARRAY = new DartLibrary[0];

  /**
   * Add a #resource directive that will cause the given file to be included in this library. Return
   * the file representing the resource that was added.
   * 
   * @param file the file to be added to the library
   * @param monitor the progress monitor used to provide feedback to the user, or <code>null</code>
   *          if no feedback is desired
   * @return the file representing the resource that was added
   * @throws DartModelException if the directive cannot be added
   */
  public DartResource addResource(File file, IProgressMonitor monitor) throws DartModelException;

  /**
   * Add a #source directive that will cause the given file to be included in this library. This
   * method does not verify that it is valid for the file to be included using a #source directive
   * (does not itself contain any directives). Return the compilation unit representing the source
   * file that was added. Note that if the changes to the compilation unit defining the library were
   * not saved (typically because the file is being edited) that the compilation unit that is
   * returned will not yet be a child of this library.
   * 
   * @param file the file to be added to the library
   * @param monitor the progress monitor used to provide feedback to the user, or <code>null</code>
   *          if no feedback is desired
   * @return the compilation unit representing the source file that was added
   * @throws DartModelException if the directive cannot be added
   */
  public CompilationUnit addSource(File file, IProgressMonitor monitor) throws DartModelException;

  /**
   * Delete this library. This has the effect of deleting the library's project and any derived
   * files associated with the library, except that the final compiled form of the library is not
   * deleted.
   * 
   * @param monitor the progress monitor used to provide feedback to the user, or <code>null</code>
   *          if no feedback is desired
   * @throws DartModelException if the library cannot be deleted for some reason
   */
  public void delete(IProgressMonitor monitor) throws DartModelException;

  /**
   * Return the type with the given name that is visible within this library, or <code>null</code>
   * if there is no such type defined in this library.
   * 
   * @param typeName the name of the type to be returned
   * @return the type with the given name that is visible within this library
   * @throws DartModelException if the types defined in this library cannot be determined for some
   *           reason
   */
  public Type findType(String typeName) throws DartModelException;

  /**
   * Return the compilation unit with the specified file in this library (for example, some IFile
   * with file name <code>"Object.dart"</code>). The name has to be a valid compilation unit name.
   * This is a handle-only method. The compilation unit may or may not be present.
   * 
   * @param file the file of the compilation unit to be returned
   * @return the compilation unit with the specified name in this package
   */
  public CompilationUnit getCompilationUnit(IFile file);

  /**
   * Return the compilation unit with the specified name in this library (for example,
   * <code>"Object.dart"</code>). The name has to be a valid compilation unit name. This is a
   * handle-only method. The compilation unit may or may not be present.
   * 
   * @param name the name of the compilation unit to be returned
   * @return the compilation unit with the specified name in this package
   */
  public CompilationUnit getCompilationUnit(String name);

  /**
   * Return an array containing all of the compilation units defined in this library.
   * 
   * @return an array containing all of the compilation units defined in this library
   * @throws DartModelException if the compilation units defined in this library cannot be
   *           determined for some reason
   */
  public CompilationUnit[] getCompilationUnits() throws DartModelException;

  /**
   * Return the compilation unit that defines this library.
   * 
   * @return the compilation unit that defines this library
   * @throws DartModelException if the defining compilation unit cannot be determined
   */
  public CompilationUnit getDefiningCompilationUnit() throws DartModelException;

  /**
   * Return the name of this element as it should appear in the user interface. Typically, this is
   * the same as {@link #getElementName()}. This is a handle-only method.
   * 
   * @return the name of this element
   */
  public String getDisplayName();

  /**
   * Return an array containing all of the libraries imported by this library. The returned
   * libraries are not included in the list of children for the library.
   * 
   * @return an array containing the imported libraries (not <code>null</code>, contains no
   *         <code>null</code>s)
   * @throws DartModelException if the imported libraries cannot be determined
   */
  public DartLibrary[] getImportedLibraries() throws DartModelException;

  /**
   * Return a resource corresponding to the given URI. This is a handle-only method. The resource
   * may or may not be present.
   * 
   * @param uri the URI corresponding to the resource to be returned
   * @return a resource corresponding to the given URI
   */
  public DartResource getResource(URI uri);

  /**
   * Return an array containing all of the resources that are included in this library.
   * 
   * @return an array containing all of the resources that are included in this library
   * @throws DartModelException if the list of resources could not be determined for some reason
   */
  public DartResource[] getResources() throws DartModelException;

  /**
   * Return <code>true</code> if this library is defined in a workspace resource, or
   * <code>false</code> if it does not exist in the workspace. Libraries on disk, but not mapped
   * into the workspace and libraries bundled in a plugin are considered non-local.
   * 
   * @return <code>true</code> if the library exists in the workspace
   */
  public boolean isLocal();
}
