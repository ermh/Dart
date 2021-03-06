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

import com.google.dart.tools.ui.dialogs.AboutDartDialog;

import org.eclipse.jface.action.Action;
import org.eclipse.swt.widgets.Event;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.actions.ActionFactory.IWorkbenchAction;

/**
 * Triggers the About Dart Dialog
 */
public class AboutDartAction extends Action implements IWorkbenchAction {
  private IWorkbenchWindow window;

  /**
   * Create an action that triggers the About Dart dialog.
   */
  public AboutDartAction(IWorkbenchWindow window) {
    this.window = window;

    setId("about"); //$NON-NLS-1$
    setActionDefinitionId("org.eclipse.ui.help.aboutAction"); //$NON-NLS-1$
    setText(ActionMessages.AboutDartAction_about_text);
    setToolTipText(ActionMessages.AboutDartAction_about_tooltip);
    setImageDescriptor(null);
  }

  @Override
  public void dispose() {
    //do nothing
  }

  @Override
  public void run() {
    openDialog();
  }

  @Override
  public void runWithEvent(Event event) {
    openDialog();
  }

  private void openDialog() {
    new AboutDartDialog(window.getShell()).open();
  }

}
