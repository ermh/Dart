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
package com.google.dart.tools.ui.omni;

import com.google.dart.tools.core.DartCore;
import com.google.dart.tools.core.model.DartElement;
import com.google.dart.tools.core.model.DartModelException;
import com.google.dart.tools.core.model.Type;
import com.google.dart.tools.ui.DartPluginImages;
import com.google.dart.tools.ui.DartToolsPlugin;

import org.eclipse.core.resources.IFile;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.ISharedImages;
import org.eclipse.ui.PlatformUI;

/**
 * Access-point for Omnibox images.
 */
public class OmniBoxImages {

  private static final ImageDescriptor CLASS_ICON = DartPluginImages.DESC_DART_CLASS_PUBLIC;
  private static final ImageDescriptor INTERFACE_ICON = DartPluginImages.DESC_OBJS_INTERFACE;

  /**
   * Generic file image descriptor.
   */
  private static ImageDescriptor GENERIC_FILE_DESC;

  /**
   * Get an image descriptor for the given file.
   * 
   * @param file the file to display
   * @return a file type specific image descriptor or {@link #GENERIC_FILE_DESC} if none can be
   *         found
   */
  public static ImageDescriptor getFileImageDescriptor(IFile file) {
    DartElement element = DartCore.create(file);
    if (element != null) {
      switch (element.getElementType()) {
        case DartElement.HTML_FILE:
          return DartPluginImages.DESC_DART_HTML_FILE;
        case DartElement.COMPILATION_UNIT:
          return DartPluginImages.DESC_DART_COMP_UNIT;
      }
    }
    return getGenericFileDescriptor();
  }

  /**
   * Get an image descriptor for the given type.
   * 
   * @param type the type to display
   * @return an image descriptor for the given type
   */
  public static ImageDescriptor getTypeImageDescriptor(Type type) {
    try {
      return type.isInterface() ? INTERFACE_ICON : CLASS_ICON;
    } catch (DartModelException e) {
      DartToolsPlugin.log(e);
    }
    return null;
  }

  /**
   * Get a generic file image descriptor for use as a default when no more specific descriptor can
   * be found.
   */
  private static ImageDescriptor getGenericFileDescriptor() {
    if (GENERIC_FILE_DESC == null) {
      GENERIC_FILE_DESC = PlatformUI.getWorkbench().getSharedImages().getImageDescriptor(
          ISharedImages.IMG_OBJ_FILE);
    }
    return GENERIC_FILE_DESC;
  }

}
