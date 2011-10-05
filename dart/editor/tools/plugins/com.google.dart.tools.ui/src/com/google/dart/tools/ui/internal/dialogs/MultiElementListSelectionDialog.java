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
package com.google.dart.tools.ui.internal.dialogs;

import com.google.dart.tools.ui.DartUIMessages;
import com.google.dart.tools.ui.Messages;

import org.eclipse.core.runtime.Assert;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.dialogs.AbstractElementListSelectionDialog;
import org.eclipse.ui.dialogs.FilteredList;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/**
 * A class to select elements out of a list of elements, organized on multiple pages.
 */
public class MultiElementListSelectionDialog extends AbstractElementListSelectionDialog {

  private static class Page {
    private Object[] elements;
    public String filter;
    public boolean okState = false;

    public Page(Object[] elements) {
      this.elements = elements;
    }
  }

  private Page[] fPages;
  private int fCurrentPage;
  private int fNumberOfPages;

  private Button fFinishButton;
  private Button fBackButton;
  private Button fNextButton;

  private Label fPageInfoLabel;
  private String fPageInfoMessage = DartUIMessages.MultiElementListSelectionDialog_pageInfoMessage;
  private Comparator fComparator;

  /**
   * Constructs a multi-page list selection dialog.
   * 
   * @param parent The parent shell
   * @param renderer the label renderer.
   */
  public MultiElementListSelectionDialog(Shell parent, ILabelProvider renderer) {
    super(parent, renderer);
  }

  /**
   * Gets the current Page.
   * 
   * @return Returns a int
   */
  public int getCurrentPage() {
    return fCurrentPage;
  }

  /*
   * @see Window#open()
   */
  @Override
  public int open() {
    List selection = getInitialElementSelections();
    if (selection == null || selection.size() != fNumberOfPages) {
      setInitialSelections(new Object[fNumberOfPages]);
      selection = getInitialElementSelections();
    }

    Assert.isTrue(selection.size() == fNumberOfPages);

    return super.open();
  }

  /**
   * Set the <code>Comparator</code> used to sort the elements in the List.
   * 
   * @param comparator the comparator to use, not null.
   */
  public void setComparator(Comparator comparator) {
    fComparator = comparator;
    if (fFilteredList != null) {
      fFilteredList.setComparator(fComparator);
    }
  }

  /**
   * Sets the elements to be displayed in the dialog.
   * 
   * @param elements an array of pages holding arrays of elements
   */
  public void setElements(Object[][] elements) {
    fNumberOfPages = elements.length;
    fPages = new Page[fNumberOfPages];
    for (int i = 0; i != fNumberOfPages; i++) {
      fPages[i] = new Page(elements[i]);
    }

    initializeResult(fNumberOfPages);
  }

  /**
   * Sets message shown in the right top corner. Use {0} and {1} as placeholders for the current and
   * the total number of pages.
   * 
   * @param message the message.
   */
  public void setPageInfoMessage(String message) {
    fPageInfoMessage = message;
  }

  /*
   * @see org.eclipse.jface.dialogs.Dialog#buttonPressed(int)
   */
  @Override
  protected void buttonPressed(int buttonId) {
    if (buttonId == IDialogConstants.BACK_ID) {
      turnPage(false);
    } else if (buttonId == IDialogConstants.NEXT_ID) {
      turnPage(true);
    } else {
      super.buttonPressed(buttonId);
    }
  }

  /*
   * @see org.eclipse.ui.dialogs.SelectionStatusDialog#computeResult()
   */
  @Override
  protected void computeResult() {
    setResult(fCurrentPage, getSelectedElements());
  }

  /*
   * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(Composite)
   */
  @Override
  protected void createButtonsForButtonBar(Composite parent) {
    fBackButton = createButton(parent, IDialogConstants.BACK_ID, IDialogConstants.BACK_LABEL, false);
    fNextButton = createButton(parent, IDialogConstants.NEXT_ID, IDialogConstants.NEXT_LABEL, true);
    fFinishButton = createButton(parent, IDialogConstants.OK_ID, IDialogConstants.FINISH_LABEL,
        false);
    createButton(parent, IDialogConstants.CANCEL_ID, IDialogConstants.CANCEL_LABEL, false);
  }

  /*
   * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(Composite)
   */
  @Override
  protected Control createDialogArea(Composite parent) {
    Composite contents = (Composite) super.createDialogArea(parent);

    createMessageArea(contents);
    createFilterText(contents);
    createFilteredList(contents);

    fCurrentPage = 0;
    setPageData();

    applyDialogFont(contents);
    return contents;
  }

  /**
   * {@inheritDoc}
   */
  @Override
  protected FilteredList createFilteredList(Composite parent) {
    FilteredList filteredList = super.createFilteredList(parent);
    if (fComparator != null) {
      filteredList.setComparator(fComparator);
    }
    return filteredList;
  }

  /*
   * @see org.eclipse.ui.dialogs.SelectionDialog#createMessageArea(Composite)
   */
  @Override
  protected Label createMessageArea(Composite parent) {
    Composite composite = new Composite(parent, SWT.NONE);

    GridLayout layout = new GridLayout();
    layout.marginHeight = 0;
    layout.marginWidth = 0;
    layout.horizontalSpacing = 5;
    layout.numColumns = 2;
    composite.setLayout(layout);

    GridData data = new GridData(GridData.HORIZONTAL_ALIGN_FILL);
    composite.setLayoutData(data);

    Label messageLabel = super.createMessageArea(composite);

    fPageInfoLabel = new Label(composite, SWT.NULL);
    fPageInfoLabel.setText(getPageInfoMessage());

    data = new GridData(GridData.HORIZONTAL_ALIGN_FILL);
    data.horizontalAlignment = GridData.END;
    fPageInfoLabel.setLayoutData(data);
    applyDialogFont(messageLabel);
    return messageLabel;
  }

  /**
   * @see AbstractElementListSelectionDialog#handleDefaultSelected()
   */
  @Override
  protected void handleDefaultSelected() {
    if (validateCurrentSelection()) {
      if (fCurrentPage == fNumberOfPages - 1) {
        buttonPressed(IDialogConstants.OK_ID);
      } else {
        buttonPressed(IDialogConstants.NEXT_ID);
      }
    }
  }

  /**
   * @see AbstractElementListSelectionDialog#updateButtonsEnableState(IStatus)
   */
  @Override
  protected void updateButtonsEnableState(IStatus status) {
    boolean isOK = !status.matches(IStatus.ERROR);
    fPages[fCurrentPage].okState = isOK;

    boolean isAllOK = isOK;
    for (int i = 0; i != fNumberOfPages; i++) {
      isAllOK = isAllOK && fPages[i].okState;
    }

    fFinishButton.setEnabled(isAllOK);

    boolean nextButtonEnabled = isOK && (fCurrentPage < fNumberOfPages - 1);

    fNextButton.setEnabled(nextButtonEnabled);
    fBackButton.setEnabled(fCurrentPage != 0);

    if (nextButtonEnabled) {
      getShell().setDefaultButton(fNextButton);
    } else if (isAllOK) {
      getShell().setDefaultButton(fFinishButton);
    }
  }

  private String getPageInfoMessage() {
    if (fPageInfoMessage == null) {
      return ""; //$NON-NLS-1$
    }

    String[] args = new String[] {
        Integer.toString(fCurrentPage + 1), Integer.toString(fNumberOfPages)};
    return Messages.format(fPageInfoMessage, args);
  }

  private void initializeResult(int length) {
    List result = new ArrayList(length);
    for (int i = 0; i != length; i++) {
      result.add(null);
    }

    setResult(result);
  }

  private void setPageData() {
    Page page = fPages[fCurrentPage];

    // 1. set elements
    setListElements(page.elements);

    // 2. apply filter
    String filter = page.filter;
    if (filter == null) {
      filter = ""; //$NON-NLS-1$
    }
    setFilter(filter);

    // 3. select elements
    Object[] selectedElements = (Object[]) getInitialElementSelections().get(fCurrentPage);
    setSelection(selectedElements);
    fFilteredList.setFocus();
  }

  private void turnPage(boolean toNextPage) {
    Page page = fPages[fCurrentPage];

    // store filter
    String filter = getFilter();
    if (filter == null) {
      filter = ""; //$NON-NLS-1$
    }
    page.filter = filter;

    // store selection
    Object[] selectedElements = getSelectedElements();
    List list = getInitialElementSelections();
    list.set(fCurrentPage, selectedElements);

    // store result
    setResult(fCurrentPage, getSelectedElements());

    if (toNextPage) {
      if (fCurrentPage + 1 >= fNumberOfPages) {
        return;
      }

      fCurrentPage++;
    } else {
      if (fCurrentPage - 1 < 0) {
        return;
      }

      fCurrentPage--;
    }

    if (fPageInfoLabel != null && !fPageInfoLabel.isDisposed()) {
      fPageInfoLabel.setText(getPageInfoMessage());
    }

    setPageData();

    validateCurrentSelection();
  }

}
