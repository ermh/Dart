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
package com.google.dart.tools.ui.actions;

import com.google.dart.tools.core.model.DartLibrary;
import com.google.dart.tools.core.model.DartModelException;
import com.google.dart.tools.ui.DartToolsPlugin;
import com.google.dart.tools.ui.internal.libraryview.LibraryExplorerPart;
import com.google.dart.tools.ui.internal.util.ExceptionHandler;

import org.eclipse.core.runtime.NullProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.ui.ISelectionListener;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.actions.ActionFactory.IWorkbenchAction;

/**
 * Close Library action. Removes a library from the Libraries view (but keeps the source code on
 * disk).
 */
public class CloseLibraryAction extends Action implements IWorkbenchAction, ISelectionListener,
    ISelectionChangedListener {

  public static final String ID = DartToolsPlugin.PLUGIN_ID + ".closeLibraryAction"; //$NON-NLS-1$

  IStructuredSelection selection;
  IWorkbenchWindow window;

  /**
   * Create an action that removes a library from the view.
   */
  public CloseLibraryAction(IWorkbenchWindow window) {
    setId(ID);
    setText(ActionMessages.CloseLibraryAction_label);
    setImageDescriptor(null);
    this.window = window;
    window.getSelectionService().addSelectionListener(this);
  }

  @Override
  public void dispose() {
    //do nothing
  }

  @Override
  public void run() {
    for (Object object : selection.toList()) {
      try {
        ((DartLibrary) object).delete(new NullProgressMonitor());
      } catch (DartModelException e) {

        ExceptionHandler.handle(e, window.getShell(),
            ActionMessages.CloseLibraryAction_error_title,
            ActionMessages.CloseLibraryAction_error_message);
      }
    }

    LibraryExplorerPart.getFromActivePerspective().getTreeViewer().refresh();

  }

  @Override
  public void selectionChanged(IWorkbenchPart part, ISelection selection) {
    if (selection instanceof IStructuredSelection) {
      handleSelectionChanged((IStructuredSelection) selection);
    } else {
      setEnabled(false);
    }
  }

  @Override
  public void selectionChanged(SelectionChangedEvent event) {
    if (event.getSelection() instanceof IStructuredSelection) {
      handleSelectionChanged((IStructuredSelection) event.getSelection());
    } else {
      setEnabled(false);
    }
  }

  private void handleSelectionChanged(IStructuredSelection selection) {
    this.selection = selection;
    if (selection == null || selection.isEmpty()) {
      setEnabled(false);
    } else {
      for (Object object : selection.toList()) {
        if (!(object instanceof DartLibrary)) {
          setEnabled(false);
          return;
        }
      }
      setEnabled(true);
    }
  }

}
