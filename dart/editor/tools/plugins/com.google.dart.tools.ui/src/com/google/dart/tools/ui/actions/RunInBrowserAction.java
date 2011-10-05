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

import com.google.dart.tools.core.DartCore;
import com.google.dart.tools.core.internal.model.DartLibraryImpl;
import com.google.dart.tools.core.internal.model.DartModelManager;
import com.google.dart.tools.core.model.DartElement;
import com.google.dart.tools.core.model.DartLibrary;
import com.google.dart.tools.core.model.DartModelException;
import com.google.dart.tools.core.model.HTMLFile;
import com.google.dart.tools.ui.DartPluginImages;
import com.google.dart.tools.ui.DartToolsPlugin;
import com.google.dart.tools.ui.ImportedDartLibraryContainer;
import com.google.dart.tools.ui.internal.util.ExceptionHandler;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.IEditorRegistry;
import org.eclipse.ui.IFileEditorInput;
import org.eclipse.ui.IPartListener;
import org.eclipse.ui.ISelectionListener;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.dialogs.ListDialog;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.ui.part.FileEditorInput;
import org.eclipse.ui.progress.UIJob;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * A menu for opening html files in the system browser.
 */
public class RunInBrowserAction extends Action implements ISelectionChangedListener,
    ISelectionListener, IPartListener {

  class RunInBrowserJob extends UIJob {
    private IWorkbenchPage page;
    private IFile file;

    public RunInBrowserJob(IWorkbenchPage page, IFile file) {
      super(page.getWorkbenchWindow().getShell().getDisplay(),
          ActionMessages.OpenInBrowserAction_jobTitle);

      this.page = page;
      this.file = file;

      // Synchronize on the workspace root to catch any builds that are in progress.
      setRule(ResourcesPlugin.getWorkspace().getRoot());

      // Make sure we display a progress dialog if we do block.
      setUser(true);
    }

    @Override
    public IStatus runInUIThread(IProgressMonitor monitor) {
      try {
        String editorId = IEditorRegistry.SYSTEM_EXTERNAL_EDITOR_ID;
        page.openEditor(new FileEditorInput(file), editorId, true, MATCH_BOTH);
      } catch (PartInitException e) {
        ExceptionHandler.handle(e, window.getShell(), ActionMessages.OpenInBrowserAction_title,
            ActionMessages.OpenInBrowserAction_couldNotOpenFile);
      }

      return Status.OK_STATUS;
    }
  }

  /**
   * The id of this action.
   */
  public static final String ACTION_ID = DartToolsPlugin.PLUGIN_ID + ".runInBrowserAction"; //$NON-NLS-1$

  private IWorkbenchWindow window;

  private Object selectedObject;

  /**
   * Match both the input and id, so that different types of editor can be opened on the same input.
   */
  private static final int MATCH_BOTH = IWorkbenchPage.MATCH_INPUT | IWorkbenchPage.MATCH_ID;

  public RunInBrowserAction(IWorkbenchWindow window) {
    this.window = window;

    setText(ActionMessages.OpenInBrowserAction_title);
    setId(ACTION_ID);
    setDescription(ActionMessages.OpenInBrowserAction_description);
    setToolTipText(ActionMessages.OpenInBrowserAction_toolTip);
    setImageDescriptor(DartPluginImages.DESC_TOOL_RUN);
    setEnabled(false);

    window.getPartService().addPartListener(this);
    window.getSelectionService().addSelectionListener(this);
  }

  @Override
  public void partActivated(IWorkbenchPart part) {
    if (part instanceof IEditorPart) {
      handleEditorActivated((IEditorPart) part);
    }
  }

  @Override
  public void partBroughtToTop(IWorkbenchPart part) {

  }

  @Override
  public void partClosed(IWorkbenchPart part) {

  }

  @Override
  public void partDeactivated(IWorkbenchPart part) {

  }

  @Override
  public void partOpened(IWorkbenchPart part) {

  }

  @Override
  public void run() {
    openInBrowser(window.getActivePage());
  }

  @Override
  public void selectionChanged(IWorkbenchPart part, ISelection selection) {
    if (selection instanceof IStructuredSelection) {
      handleSelectionChanged((IStructuredSelection) selection);
    }
  }

  @Override
  public void selectionChanged(SelectionChangedEvent event) {
    if (event.getSelection() instanceof IStructuredSelection) {
      handleSelectionChanged((IStructuredSelection) event.getSelection());
    }
  }

  void openInBrowser(IWorkbenchPage page) {
    try {
      List<IFile> files = getFileResources();

      IFile file = null;

      if (files.size() == 0) {
        MessageDialog.openError(window.getShell(), ActionMessages.OpenInBrowserAction_noFileTitle,
            ActionMessages.OpenInBrowserAction_noFileMessage);
      } else if (files.size() == 1) {
        file = files.get(0);
      } else {
        file = chooseHtmlFile(files);
      }

      if (file != null) {
        RunInBrowserJob job = new RunInBrowserJob(page, file);

        job.schedule();
      }
    } catch (DartModelException e) {
      ExceptionHandler.handle(e, window.getShell(), ActionMessages.OpenInBrowserAction_title,
          ActionMessages.OpenInBrowserAction_couldNotOpenFile);
    }
  }

  private IFile chooseHtmlFile(List<IFile> htmlFiles) {
    ListDialog dialog = new ListDialog(window.getShell());

    dialog.setTitle(ActionMessages.OpenInBrowserAction_selectFileTitle);
    dialog.setMessage(ActionMessages.OpenInBrowserAction_selectFileMessage);
    dialog.setLabelProvider(new WorkbenchLabelProvider());
    dialog.setContentProvider(new ArrayContentProvider());
    dialog.setInput(htmlFiles);

    dialog.open();

    Object[] result = dialog.getResult();

    if (result == null || result.length == 0) {
      return null;
    }

    return (IFile) result[0];
  }

  private List<IFile> getAllAvailableHtmlFiles() throws DartModelException {
    Set<IFile> files = new HashSet<IFile>();

    for (DartLibrary library : DartModelManager.getInstance().getDartModel().getDartLibraries()) {
      files.addAll(getHtmlFilesFor(library));
    }

    return new ArrayList<IFile>(files);
  }

  private List<IFile> getFileResources() throws DartModelException {
    IResource resource = null;
    DartElement element = null;

    if (selectedObject instanceof IResource) {
      resource = (IResource) selectedObject;
    }

    if (resource != null) {
      // html file
      if (isHtmlFile(resource)) {
        return Collections.singletonList((IFile) resource);
      }

      // other resource
      element = DartCore.create(resource);
    }

    if (selectedObject instanceof DartElement) {
      element = (DartElement) selectedObject;
    }

    // HTMLFile
    if (element instanceof HTMLFile) {
      HTMLFile htmlFile = (HTMLFile) element;

      return Collections.singletonList((IFile) htmlFile.getCorrespondingResource());
    }

    if (selectedObject instanceof ImportedDartLibraryContainer) {
      element = ((ImportedDartLibraryContainer) selectedObject).getDartLibrary();
    }

    if (element == null) {
      return getAllAvailableHtmlFiles();
    } else {
      // DartElement in a library
      DartLibrary library = element.getAncestor(DartLibrary.class);

      if (library != null) {
        List<IFile> htmlFiles = getHtmlFilesFor(library);

        if (htmlFiles.size() > 0) {
          return htmlFiles;
        }
      }

      return getAllAvailableHtmlFiles();
    }
  }

  private List<IFile> getHtmlFilesFor(DartLibrary library) throws DartModelException {
    Set<IFile> files = new HashSet<IFile>();

    for (HTMLFile file : ((DartLibraryImpl) library).getChildrenOfType(HTMLFile.class)) {
      files.add((IFile) file.getUnderlyingResource());
    }

    return new ArrayList<IFile>(files);
  }

  private void handleEditorActivated(IEditorPart editorPart) {
    if (editorPart.getEditorInput() instanceof IFileEditorInput) {
      IFileEditorInput input = (IFileEditorInput) editorPart.getEditorInput();

      handleSelectionChanged(new StructuredSelection(input.getFile()));
    }
  }

  private void handleSelectionChanged(IStructuredSelection selection) {
    if (selection != null && !selection.isEmpty()) {
      selectedObject = selection.getFirstElement();

      setEnabled(true);
    } else {
      selectedObject = null;

      setEnabled(false);
    }
  }

  private boolean isHtmlFile(IResource resource) {
    return resource instanceof IFile && resource.getName().endsWith(".html");
  }

}
