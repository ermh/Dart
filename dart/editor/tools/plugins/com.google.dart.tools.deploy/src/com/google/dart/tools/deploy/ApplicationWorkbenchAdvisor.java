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
package com.google.dart.tools.deploy;

import com.google.dart.tools.core.DartCore;
import com.google.dart.tools.core.MessageConsole.MessageStream;

import org.eclipse.core.internal.resources.Workspace;
import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.resources.WorkspaceJob;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.IAdaptable;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.MultiStatus;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Preferences;
import org.eclipse.core.runtime.ProgressMonitorWrapper;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.dialogs.ErrorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IWorkbenchPreferenceConstants;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.application.IWorkbenchConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchAdvisor;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;
import org.eclipse.ui.console.ConsolePlugin;
import org.eclipse.ui.console.IConsole;
import org.eclipse.ui.console.MessageConsole;
import org.eclipse.ui.console.MessageConsoleStream;
import org.eclipse.ui.ide.IDE;
import org.eclipse.ui.internal.ide.IDEInternalPreferences;
import org.eclipse.ui.internal.ide.IDEInternalWorkbenchImages;
import org.eclipse.ui.internal.ide.IDEWorkbenchActivityHelper;
import org.eclipse.ui.internal.ide.IDEWorkbenchMessages;
import org.eclipse.ui.internal.ide.IDEWorkbenchPlugin;
import org.eclipse.ui.internal.ide.undo.WorkspaceUndoMonitor;
import org.eclipse.ui.internal.progress.ProgressMonitorJobsDialog;
import org.osgi.framework.Bundle;

import java.lang.reflect.InvocationTargetException;
import java.net.URL;

/**
 * IDE-specified workbench advisor which configures the workbench for use as an IDE.
 * <p>
 * Much of this code was copied out the Eclipses version of this class:
 * <code>IDEWorkbenchAdvistor</code> in <code>org.eclipse.ui.ide.application</code>.
 * </p>
 * <p>
 * Note: This class replaces <code>org.eclipse.ui.internal.Workbench</code>.
 * </p>
 */
@SuppressWarnings("restriction")
public class ApplicationWorkbenchAdvisor extends WorkbenchAdvisor {

  private class CancelableProgressMonitorJobsDialog extends ProgressMonitorJobsDialog {

    public CancelableProgressMonitorJobsDialog(Shell parent) {
      super(parent);
    }

    public void registerCancelButtonListener() {
      cancel.addSelectionListener(new SelectionAdapter() {
        @Override
        public void widgetSelected(SelectionEvent e) {
          subTaskLabel.setText(""); //$NON-NLS-1$
        }
      });
    }

    /**
     * @see org.eclipse.ui.internal.progress.ProgressMonitorJobsDialog#createDetailsButton(org.eclipse.swt.widgets.Composite)
     */
    @Override
    protected void createButtonsForButtonBar(Composite parent) {
      super.createButtonsForButtonBar(parent);
      registerCancelButtonListener();
    }
  }

  private class CancelableProgressMonitorWrapper extends ProgressMonitorWrapper {
    private double total = 0;
    private ProgressMonitorJobsDialog dialog;

    CancelableProgressMonitorWrapper(IProgressMonitor monitor, ProgressMonitorJobsDialog dialog) {
      super(monitor);
      this.dialog = dialog;
    }

    @Override
    public void beginTask(String name, int totalWork) {
      super.beginTask(name, totalWork);
      subTask(IDEWorkbenchMessages.IDEWorkbenchAdvisor_preHistoryCompaction);
    }

    /**
     * @see org.eclipse.core.runtime.ProgressMonitorWrapper#internalWorked(double)
     */
    @Override
    public void internalWorked(double work) {
      super.internalWorked(work);
      total += work;
      updateProgressDetails();
    }

    /**
     * @see org.eclipse.core.runtime.ProgressMonitorWrapper#worked(int)
     */
    @Override
    public void worked(int work) {
      super.worked(work);
      total += work;
      updateProgressDetails();
    }

    private void updateProgressDetails() {
      if (!isCanceled() && Math.abs(total - 4.0) < 0.0001 /* right before history compacting */) {
        subTask(IDEWorkbenchMessages.IDEWorkbenchAdvisor_cancelHistoryPruning);
        dialog.setCancelable(true);
      }
      if (Math.abs(total - 5.0) < 0.0001 /* history compacting finished */) {
        subTask(IDEWorkbenchMessages.IDEWorkbenchAdvisor_postHistoryCompaction);
        dialog.setCancelable(false);
      }
    }
  }

  private static final String PERSPECTIVE_ID = "com.google.dart.tools.ui.DartPerspective"; //$NON-NLS-1$

  private static ApplicationWorkbenchAdvisor workbenchAdvisor = null;

  /**
   * Helper for managing activities in response to workspace changes.
   */
  private IDEWorkbenchActivityHelper activityHelper = null;

  /**
   * Helper for managing work that is performed when the system is otherwise idle.
   */
  private DartIdleHelper idleHelper;

  /**
   * Support class for monitoring workspace changes and periodically validating the undo history
   */
  private WorkspaceUndoMonitor workspaceUndoMonitor;

  /**
   * Helper class used to process delayed events.
   */
  private DelayedEventsProcessor delayedEventsProcessor;

  /**
   * Creates a new workbench advisor instance.
   */
  public ApplicationWorkbenchAdvisor(DelayedEventsProcessor processor) {
    super();
    if (workbenchAdvisor != null) {
      throw new IllegalStateException();
    }
    workbenchAdvisor = this;
    this.delayedEventsProcessor = processor;
  }

  @Override
  public WorkbenchWindowAdvisor createWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer) {
    return new ApplicationWorkbenchWindowAdvisor(workbenchAdvisor, configurer);
  }

  /*
   * (non-Javadoc)
   * 
   * @see org.eclipse.ui.application.WorkbenchAdvisor#eventLoopIdle(org.eclipse.swt.widgets.Display)
   */
  @Override
  public void eventLoopIdle(Display display) {
    if (delayedEventsProcessor != null) {
      delayedEventsProcessor.catchUp(display);
    }
    super.eventLoopIdle(display);
  }

  /**
   * @see org.eclipse.ui.application.WorkbenchAdvisor#getDefaultPageInput
   */
  @Override
  public IAdaptable getDefaultPageInput() {
    // use workspace root
    return ResourcesPlugin.getWorkspace().getRoot();
  }

  /**
   * @see org.eclipse.ui.application.WorkbenchAdvisor#getInitialWindowPerspectiveId
   */
  @Override
  public String getInitialWindowPerspectiveId() {
    return PERSPECTIVE_ID;
  }

  /**
   * @see org.eclipse.ui.application.WorkbenchAdvisor#initialize
   */
  @Override
  public void initialize(IWorkbenchConfigurer configurer) {

    // make sure we always save and restore workspace state
    configurer.setSaveAndRestore(true);

    // register workspace adapters
    IDE.registerAdapters();

    // set our preferred preference settings
    initializePreferenceSettings();

    // register shared images
    declareWorkbenchImages();

    // initialize the activity helper
    activityHelper = IDEWorkbenchActivityHelper.getInstance();

    // initialize idle handler
    idleHelper = new DartIdleHelper(configurer);

    // initialize the workspace undo monitor
    workspaceUndoMonitor = WorkspaceUndoMonitor.getInstance();
  }

  /**
   * @see org.eclipse.ui.application.WorkbenchAdvisor#postShutdown
   */
  @Override
  public void postShutdown() {
    if (activityHelper != null) {
      activityHelper.shutdown();
      activityHelper = null;
    }
    if (idleHelper != null) {
      idleHelper.shutdown();
      idleHelper = null;
    }
    if (workspaceUndoMonitor != null) {
      workspaceUndoMonitor.shutdown();
      workspaceUndoMonitor = null;
    }
    if (IDEWorkbenchPlugin.getPluginWorkspace() != null) {
      disconnectFromWorkspace();
    }
  }

  /**
   * @see org.eclipse.ui.application.WorkbenchAdvisor#postStartup()
   */
  @Override
  public void postStartup() {
    try {
      refreshFromLocal();
      // TODO remove or comment in the following code: activate a proxy service?
      //activateProxyService();

      //((Workbench) PlatformUI.getWorkbench()).registerService(ISelectionConversionService.class,
      //    new IDESelectionConversionService());

      // TODO remove or comment in the following code: prompt user when certain settings are changed?
      //initializeSettingsChangeListener();

      //Display.getCurrent().addListener(SWT.Settings, settingsChangeListener);

      final MessageConsole console = new MessageConsole("", null); // empty string hides title bar
      ConsolePlugin.getDefault().getConsoleManager().addConsoles(new IConsole[] {console});
      final MessageConsoleStream stream = console.newMessageStream();
      stream.setActivateOnWrite(false);

      DartCore.getConsole().addStream(new MessageStream() {
        @Override
        public void clear() {
          console.clearConsole();
        }

        @Override
        public void print(String s) {
          stream.print(s);
        }

        @Override
        public void println() {
          stream.println();
        }

        @Override
        public void println(String s) {
          stream.println(s);
        }
      });

    } finally {// Resume background jobs after we startup
      Job.getJobManager().resume();
    }
  }

  /**
   * @see org.eclipse.ui.application.WorkbenchAdvisor#preShutdown()
   */
  @Override
  public boolean preShutdown() {
    //Display.getCurrent().removeListener(SWT.Settings, settingsChangeListener);
    return super.preShutdown();
  }

  /**
   * @see org.eclipse.ui.application.WorkbenchAdvisor#preStartup()
   */
  @Override
  public void preStartup() {

    // Suspend background jobs while we startup
    Job.getJobManager().suspend();

    // Register the build actions
//    IProgressService service = PlatformUI.getWorkbench().getProgressService();
//    ImageDescriptor newImage = DartWorkbenchImages.getImageDescriptor(DartWorkbenchImages.IMG_ETOOL_BUILD_EXEC);
//    service.registerIconForFamily(newImage, ResourcesPlugin.FAMILY_MANUAL_BUILD);
//    service.registerIconForFamily(newImage, ResourcesPlugin.FAMILY_AUTO_BUILD);
  }

  /**
   * Code originally copied over from IDEWorkbenchAdvisor.declareWorkbenchImage(..) in
   * <code>org.eclipse.ui.ide.application</code>.
   * <p>
   * Declares an IDE-specific workbench image.
   * </p>
   * 
   * @param symbolicName the symbolic name of the image
   * @param path the path of the image file; this path is relative to the base of the IDE plug-in
   * @param shared <code>true</code> if this is a shared image, and <code>false</code> if this is
   *          not a shared image
   * @see IWorkbenchConfigurer#declareImage
   */
  private void declareWorkbenchImage(Bundle ideBundle, String symbolicName, String path,
      boolean shared) {
    URL url = FileLocator.find(ideBundle, new Path(path), null);
    ImageDescriptor desc = ImageDescriptor.createFromURL(url);
    getWorkbenchConfigurer().declareImage(symbolicName, desc, shared);
  }

  /**
   * Code originally copied over from IDEWorkbenchAdvisor.declareWorkbenchImages() in
   * <code>org.eclipse.ui.ide.application</code>.
   * <p>
   * Declares all IDE-specific workbench images. This includes both "shared" images (named in
   * {@link IDE.SharedImages}) and internal images (named in
   * {@link org.eclipse.ui.internal.ide.IDEInternalWorkbenchImages}).
   * 
   * @see IWorkbenchConfigurer#declareImage
   */
  private void declareWorkbenchImages() {

    final String ICONS_PATH = "$nl$/icons/full/";//$NON-NLS-1$
    final String PATH_ELOCALTOOL = ICONS_PATH + "elcl16/"; // Enabled //$NON-NLS-1$

    // toolbar
    // icons.
    final String PATH_DLOCALTOOL = ICONS_PATH + "dlcl16/"; // Disabled //$NON-NLS-1$
    // //$NON-NLS-1$
    // toolbar
    // icons.
    final String PATH_ETOOL = ICONS_PATH + "etool16/"; // Enabled toolbar //$NON-NLS-1$
    // //$NON-NLS-1$
    // icons.
    final String PATH_DTOOL = ICONS_PATH + "dtool16/"; // Disabled toolbar //$NON-NLS-1$
    // //$NON-NLS-1$
    // icons.
    final String PATH_OBJECT = ICONS_PATH + "obj16/"; // Model object //$NON-NLS-1$
    // //$NON-NLS-1$
    // icons
    final String PATH_WIZBAN = ICONS_PATH + "wizban/"; // Wizard //$NON-NLS-1$
    // //$NON-NLS-1$
    // icons

    // View icons
    // Introduced in 3.7
    final String PATH_EVIEW = ICONS_PATH + "eview16/"; //$NON-NLS-1$

    Bundle ideBundle = Platform.getBundle(Activator.PLUGIN_ID);//(IDEWorkbenchPlugin.IDE_WORKBENCH);

    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_ETOOL_BUILD_EXEC, PATH_ETOOL
        + "build_exec.gif", false); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_ETOOL_BUILD_EXEC_HOVER, PATH_ETOOL
        + "build_exec.gif", false); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_ETOOL_BUILD_EXEC_DISABLED, PATH_DTOOL
        + "build_exec.gif", false); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_ETOOL_SEARCH_SRC, PATH_ETOOL
        + "search_src.gif", false); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_ETOOL_SEARCH_SRC_HOVER, PATH_ETOOL
        + "search_src.gif", false); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_ETOOL_SEARCH_SRC_DISABLED, PATH_DTOOL
        + "search_src.gif", false); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_ETOOL_NEXT_NAV, PATH_ETOOL
        + "next_nav.gif", false); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_ETOOL_PREVIOUS_NAV, PATH_ETOOL
        + "prev_nav.gif", false); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_WIZBAN_NEWPRJ_WIZ, PATH_WIZBAN
        + "newprj_wiz.png", false); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_WIZBAN_NEWFOLDER_WIZ, PATH_WIZBAN
        + "newfolder_wiz.png", false); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_WIZBAN_NEWFILE_WIZ, PATH_WIZBAN
        + "newfile_wiz.png", false); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_WIZBAN_IMPORTDIR_WIZ, PATH_WIZBAN
        + "importdir_wiz.png", false); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_WIZBAN_IMPORTZIP_WIZ, PATH_WIZBAN
        + "importzip_wiz.png", false); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_WIZBAN_EXPORTDIR_WIZ, PATH_WIZBAN
        + "exportdir_wiz.png", false); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_WIZBAN_EXPORTZIP_WIZ, PATH_WIZBAN
        + "exportzip_wiz.png", false); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_WIZBAN_RESOURCEWORKINGSET_WIZ,
        PATH_WIZBAN + "workset_wiz.png", false); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_DLGBAN_SAVEAS_DLG, PATH_WIZBAN
        + "saveas_wiz.png", false); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_DLGBAN_QUICKFIX_DLG, PATH_WIZBAN
        + "quick_fix.png", false); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, IDE.SharedImages.IMG_OBJ_PROJECT,
        PATH_OBJECT + "prj_obj.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, IDE.SharedImages.IMG_OBJ_PROJECT_CLOSED, PATH_OBJECT
        + "cprj_obj.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, IDE.SharedImages.IMG_OPEN_MARKER, PATH_ELOCALTOOL
        + "gotoobj_tsk.gif", true); //$NON-NLS-1$

    // Quick fix icons
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_ELCL_QUICK_FIX_ENABLED,
        PATH_ELOCALTOOL + "smartmode_co.gif", true); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_DLCL_QUICK_FIX_DISABLED,
        PATH_DLOCALTOOL + "smartmode_co.gif", true); //$NON-NLS-1$

    // Introduced in 3.7
    declareWorkbenchImage(ideBundle, IDEInternalWorkbenchImages.IMG_OBJS_FIXABLE_WARNING,
        PATH_OBJECT + "quickfix_warning_obj.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, IDEInternalWorkbenchImages.IMG_OBJS_FIXABLE_ERROR, PATH_OBJECT
        + "quickfix_error_obj.gif", true); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, IDE.SharedImages.IMG_OBJS_TASK_TSK, PATH_OBJECT
        + "taskmrk_tsk.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, IDE.SharedImages.IMG_OBJS_BKMRK_TSK, PATH_OBJECT
        + "bkmrk_tsk.gif", true); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_OBJS_COMPLETE_TSK, PATH_OBJECT
        + "complete_tsk.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_OBJS_INCOMPLETE_TSK, PATH_OBJECT
        + "incomplete_tsk.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_OBJS_WELCOME_ITEM, PATH_OBJECT
        + "welcome_item.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_OBJS_WELCOME_BANNER, PATH_OBJECT
        + "welcome_banner.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_OBJS_ERROR_PATH, PATH_OBJECT
        + "error_tsk.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_OBJS_WARNING_PATH, PATH_OBJECT
        + "warn_tsk.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_OBJS_INFO_PATH, PATH_OBJECT
        + "info_tsk.gif", true); //$NON-NLS-1$

    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_LCL_FLAT_LAYOUT, PATH_ELOCALTOOL
        + "flatLayout.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_LCL_HIERARCHICAL_LAYOUT,
        PATH_ELOCALTOOL + "hierarchicalLayout.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, DartWorkbenchImages.IMG_ETOOL_PROBLEM_CATEGORY, PATH_ETOOL
        + "problem_category.gif", true); //$NON-NLS-1$

    // Introduced in 3.7
    declareWorkbenchImage(ideBundle, IDEInternalWorkbenchImages.IMG_ETOOL_PROBLEMS_VIEW, PATH_EVIEW
        + "problems_view.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, IDEInternalWorkbenchImages.IMG_ETOOL_PROBLEMS_VIEW_ERROR,
        PATH_EVIEW + "problems_view_error.gif", true); //$NON-NLS-1$
    declareWorkbenchImage(ideBundle, IDEInternalWorkbenchImages.IMG_ETOOL_PROBLEMS_VIEW_WARNING,
        PATH_EVIEW + "problems_view_warning.gif", true); //$NON-NLS-1$
  }

  /**
   * Disconnect from the core workspace.
   */
  private void disconnectFromWorkspace() {
    // save the workspace
    final MultiStatus status = new MultiStatus(IDEWorkbenchPlugin.IDE_WORKBENCH, 1,
        IDEWorkbenchMessages.ProblemSavingWorkbench, null);
    try {
      final ProgressMonitorJobsDialog p = new CancelableProgressMonitorJobsDialog(null);

      final boolean applyPolicy = ResourcesPlugin.getWorkspace().getDescription().isApplyFileStatePolicy();

      IRunnableWithProgress runnable = new IRunnableWithProgress() {
        @Override
        public void run(IProgressMonitor monitor) {
          try {
            if (applyPolicy) {
              monitor = new CancelableProgressMonitorWrapper(monitor, p);
            }

            status.merge(((Workspace) ResourcesPlugin.getWorkspace()).save(true, true, monitor));
          } catch (CoreException e) {
            status.merge(e.getStatus());
          }
        }
      };

      p.run(true, false, runnable);
    } catch (InvocationTargetException e) {
      status.merge(new Status(IStatus.ERROR, IDEWorkbenchPlugin.IDE_WORKBENCH, 1,
          IDEWorkbenchMessages.InternalError, e.getTargetException()));
    } catch (InterruptedException e) {
      status.merge(new Status(IStatus.ERROR, IDEWorkbenchPlugin.IDE_WORKBENCH, 1,
          IDEWorkbenchMessages.InternalError, e));
    }
    ErrorDialog.openError(null, IDEWorkbenchMessages.ProblemsSavingWorkspace, null, status,
        IStatus.ERROR | IStatus.WARNING);
    if (!status.isOK()) {
      IDEWorkbenchPlugin.log(IDEWorkbenchMessages.ProblemsSavingWorkspace, status);
    }
  }

  private void initializePreferenceSettings() {
    // tab style setting
    PlatformUI.getPreferenceStore().setValue(
        IWorkbenchPreferenceConstants.SHOW_TRADITIONAL_STYLE_TABS, false);

    // auto-refresh setting
    Preferences preferences = ResourcesPlugin.getPlugin().getPluginPreferences();

    preferences.setValue(ResourcesPlugin.PREF_AUTO_REFRESH, true);
  }

  private void refreshFromLocal() {
    String[] commandLineArgs = Platform.getCommandLineArgs();
    IPreferenceStore store = IDEWorkbenchPlugin.getDefault().getPreferenceStore();
    boolean refresh = store.getBoolean(IDEInternalPreferences.REFRESH_WORKSPACE_ON_STARTUP);
    if (!refresh) {
      return;
    }

    // Do not refresh if it was already done by core on startup.
    for (int i = 0; i < commandLineArgs.length; i++) {
      if (commandLineArgs[i].equalsIgnoreCase("-refresh")) { //$NON-NLS-1$
        return;
      }
    }

    final IContainer root = ResourcesPlugin.getWorkspace().getRoot();
    Job job = new WorkspaceJob(IDEWorkbenchMessages.Workspace_refreshing) {
      @Override
      public IStatus runInWorkspace(IProgressMonitor monitor) throws CoreException {
        root.refreshLocal(IResource.DEPTH_INFINITE, monitor);
        return Status.OK_STATUS;
      }
    };
    job.setRule(root);
    job.schedule();
  }

}
