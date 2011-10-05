/*
 * Copyright (c) 2011, the Dart project authors.
 *
 * Licensed under the Eclipse Public License v1.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
package com.google.dart.tools.debug.core.breakpoints;

import com.google.dart.tools.debug.core.DartDebugCorePlugin;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IMarker;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.IWorkspaceRunnable;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.debug.core.model.IBreakpoint;
import org.eclipse.debug.core.model.ILineBreakpoint;
import org.eclipse.debug.core.model.LineBreakpoint;
import org.eclipse.osgi.util.NLS;

/**
 * The implementation of a Dart line breakpoint.
 * 
 * @see ILineBreakpoint
 */
public class DartBreakpoint extends LineBreakpoint {

  /**
   * A default constructor is required for the breakpoint manager to re-create persisted
   * breakpoints. After instantiating a breakpoint, the setMarker method is called to restore this
   * breakpoint's attributes.
   */
  public DartBreakpoint() {

  }

  /**
   * Create a new DartBreakpoint.
   * 
   * @throws CoreException
   */
  public DartBreakpoint(final IResource resource, final int line) throws CoreException {
    IWorkspaceRunnable runnable = new IWorkspaceRunnable() {
      @Override
      public void run(IProgressMonitor monitor) throws CoreException {
        IMarker marker = resource.createMarker(DartDebugCorePlugin.DEBUG_MARKER_ID);

        setMarker(marker);

        marker.setAttribute(IBreakpoint.ENABLED, Boolean.TRUE);
        marker.setAttribute(IMarker.LINE_NUMBER, line);
        marker.setAttribute(IBreakpoint.ID, getModelIdentifier());
        marker.setAttribute(IMarker.MESSAGE,
            NLS.bind("Line Breakpoint: {0} [line: {1}]", resource.getName(), line));
      }
    };

    run(getMarkerRule(resource), runnable);
  }

  public IFile getFile() {
    return (IFile) getMarker().getResource();
  }

  public int getLine() {
    IMarker m = getMarker();
    if (m != null) {
      return m.getAttribute(IMarker.LINE_NUMBER, -1);
    }
    return -1;
  }

  @Override
  public String getModelIdentifier() {
    return DartDebugCorePlugin.DEBUG_MODEL_ID;
  }

  public boolean isBreakpointEnabled() {
    return getMarker().getAttribute(ENABLED, false);
  }

  @Override
  public String toString() {
    if (getFile() != null) {
      return getFile().getName() + ":" + getLine();
    } else {
      return getFile() + ":" + getLine();
    }
  }

}
