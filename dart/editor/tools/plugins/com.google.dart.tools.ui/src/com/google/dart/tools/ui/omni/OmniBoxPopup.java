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
package com.google.dart.tools.ui.omni;

import com.google.dart.tools.ui.DartToolsPlugin;
import com.google.dart.tools.ui.omni.elements.EditorProvider;
import com.google.dart.tools.ui.omni.elements.FileProvider;
import com.google.dart.tools.ui.omni.elements.HeaderElement;
import com.google.dart.tools.ui.omni.elements.TypeProvider;

import org.eclipse.core.commands.Command;
import org.eclipse.core.runtime.Assert;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.NullProgressMonitor;
import org.eclipse.jface.bindings.TriggerSequence;
import org.eclipse.jface.bindings.keys.KeySequence;
import org.eclipse.jface.bindings.keys.SWTKeySupport;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.layout.GridDataFactory;
import org.eclipse.jface.layout.GridLayoutFactory;
import org.eclipse.jface.layout.TableColumnLayout;
import org.eclipse.jface.resource.FontDescriptor;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.resource.LocalResourceManager;
import org.eclipse.jface.util.Util;
import org.eclipse.jface.viewers.ColumnWeightData;
import org.eclipse.osgi.util.NLS;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.BusyIndicator;
import org.eclipse.swt.events.KeyAdapter;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontMetrics;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.graphics.TextLayout;
import org.eclipse.swt.graphics.TextStyle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.swt.widgets.Text;
import org.eclipse.swt.widgets.Widget;
import org.eclipse.ui.IWorkbenchPreferenceConstants;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.internal.progress.ProgressManagerUtil;
import org.eclipse.ui.keys.IBindingService;
import org.eclipse.ui.themes.ColorUtil;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

public class OmniBoxPopup extends BasePopupDialog {
  private class PreviousPicksProvider extends OmniProposalProvider {

    @Override
    public OmniElement getElementForId(String id) {
      return null;
    }

    @Override
    public OmniElement[] getElements(String pattern) {
      return previousPicksList.toArray(new OmniElement[previousPicksList.size()]);
    }

    @Override
    public OmniElement[] getElementsSorted(String pattern) {
      return getElements(pattern);
    }

    @Override
    public String getId() {
      return PreviousPicksProvider.class.getName();
    }

    @Override
    public String getName() {
      return OmniBoxMessages.OmniBox_Previous;
    }
  }

  private static final int INITIAL_COUNT_PER_PROVIDER = 5;

  private static final int MAX_COUNT_TOTAL = 20;

//  protected Text filterText;
  private OmniProposalProvider[] providers;

  private final IWorkbenchWindow window;

  protected Table table;

  private LocalResourceManager resourceManager = new LocalResourceManager(
      JFaceResources.getResources());
  private static final String TEXT_ARRAY = "textArray"; //$NON-NLS-1$
  private static final String TEXT_ENTRIES = "textEntries"; //$NON-NLS-1$
  private static final String ORDERED_PROVIDERS = "orderedProviders"; //$NON-NLS-1$
  private static final String ORDERED_ELEMENTS = "orderedElements"; //$NON-NLS-1$
  static final int MAXIMUM_NUMBER_OF_ELEMENTS = 60;

  static final int MAXIMUM_NUMBER_OF_TEXT_ENTRIES_PER_ELEMENT = 3;

  protected String rememberedText;

  protected Map<Object, ArrayList<String>> textMap = new HashMap<Object, ArrayList<String>>();

  protected Map<String, Object> elementMap = new HashMap<String, Object>();

  private LinkedList<Object> previousPicksList = new LinkedList<Object>();
  protected Map<String, OmniProposalProvider> providerMap;
  // private Font italicsFont;
  private Color grayColor;
  private TextLayout textLayout;
  private TriggerSequence[] invokingCommandKeySequences;
  private Command invokingCommand;
  private KeyAdapter keyAdapter;

  private boolean showAllMatches = false;

  protected boolean resized = false;

  private Text filterControl;

  public OmniBoxPopup(IWorkbenchWindow window, final Command invokingCommand) {
    super(ProgressManagerUtil.getDefaultParent(), SWT.NONE, true, false, /* persist size */
    false, /* but not location */
    false, false, null, null /* OmniBoxMessages.OmniBox_StartTypingToFindMatches */);

    this.window = window;
    BusyIndicator.showWhile(window.getShell() == null ? null : window.getShell().getDisplay(),
        new Runnable() {
          @Override
          public void run() {
            OmniBoxPopup.this.providers = createProviders();
            providerMap = new HashMap<String, OmniProposalProvider>();
            for (int i = 0; i < providers.length; i++) {
              providerMap.put(providers[i].getId(), providers[i]);
            }
            restoreDialog();
            OmniBoxPopup.this.invokingCommand = invokingCommand;
            if (OmniBoxPopup.this.invokingCommand != null
                && !OmniBoxPopup.this.invokingCommand.isDefined()) {
              OmniBoxPopup.this.invokingCommand = null;
            } else {
              // Pre-fetch key sequence - do not change because scope will
              // change later.
              getInvokingCommandKeySequences();
            }
            // create early
            create();
          }
        });
    // Ugly hack to avoid bug 184045. If this gets fixed, replace the
    // following code with a call to refresh("").
    getShell().getDisplay().asyncExec(new Runnable() {
      @Override
      public void run() {
        final Shell shell = getShell();
        if (shell != null && !shell.isDisposed()) {
          Point size = shell.getSize();
          shell.setSize(size.x, size.y + 1);
        }
      }
    });
  }

  @Override
  public boolean close() {
    storeDialog(getDialogSettings());
    if (textLayout != null && !textLayout.isDisposed()) {
      textLayout.dispose();
    }
    if (resourceManager != null) {
      resourceManager.dispose();
      resourceManager = null;
    }
    return super.close();
  }

  public boolean isDisposed() {
    return textLayout.isDisposed();
  }

  public void sendKeyPress(KeyEvent e) {
    switch (e.keyCode) {
      case SWT.CR:
      case SWT.KEYPAD_CR:
        handleSelection();
        break;
      case SWT.ARROW_DOWN:
        int index = table.getSelectionIndex();
        int count = table.getItemCount();
        if (index == -1 || index >= count - 1) {
          break;
        }
        int nextIndex = index + 1;
        if (count > index + 2) {
          if (isHeader(table.getItem(nextIndex))) {
            nextIndex++;
          }
        }
        table.setSelection(nextIndex);
        break;
      case SWT.ARROW_UP:
        index = table.getSelectionIndex();
        if (index == -1) {
          break;
        }
        int previousIndex = index - 1;
        if (index >= 2) {
          if (isHeader(table.getItem(previousIndex))) {
            previousIndex--;
          }
        }
        table.setSelection(previousIndex);
        break;
      case SWT.ESC:
        close();
        break;
    }

  }

  @Override
  protected Control createDialogArea(Composite parent) {
    Composite composite = (Composite) super.createDialogArea(parent);

    boolean isWin32 = Util.isWindows();
    GridLayoutFactory.fillDefaults().extendedMargins(isWin32 ? 0 : 3, 3, 2, 2).applyTo(composite);
    Composite tableComposite = new Composite(composite, SWT.NONE);
    GridDataFactory.fillDefaults().grab(true, true).applyTo(tableComposite);

    TableColumnLayout tableColumnLayout = new TableColumnLayout();
    tableComposite.setLayout(tableColumnLayout);

    table = new Table(tableComposite, SWT.SINGLE | SWT.FULL_SELECTION);
    textLayout = new TextLayout(table.getDisplay());
    textLayout.setOrientation(getDefaultOrientation());
    Font boldFont = resourceManager.createFont(FontDescriptor.createFrom(
        JFaceResources.getDialogFont()).setStyle(SWT.BOLD));
    textLayout.setFont(table.getFont());
    textLayout.setText(OmniBoxMessages.OmniBox_Providers);
    int maxProviderWidth = (int) (textLayout.getBounds().width * 1.1);
    textLayout.setFont(boldFont);
    for (int i = 0; i < providers.length; i++) {
      OmniProposalProvider provider = providers[i];
      textLayout.setText(provider.getName());
      int width = (int) (textLayout.getBounds().width * 1.1);
      if (width > maxProviderWidth) {
        maxProviderWidth = width;
      }
    }

    //TODO (pquitslund): just a placeholder column for now
    tableColumnLayout.setColumnData(new TableColumn(table, SWT.NONE),
        new ColumnWeightData(0, 3 /* maxProviderWidth) */));
    tableColumnLayout.setColumnData(new TableColumn(table, SWT.NONE),
        new ColumnWeightData(100, 100));

//TODO (pquitslund): and with this goes the ability to resize...
//    table.getShell().addControlListener(new ControlAdapter() {
//      @Override
//      public void controlResized(ControlEvent e) {
//        if (!showAllMatches) {
//          if (!resized) {
//            resized = true;
//            e.display.timerExec(100, new Runnable() {
//              @Override
//              public void run() {
//                if (getShell() != null && !getShell().isDisposed()) {
//                  refresh(getFilterText());
//                }
//                resized = false;
//              }
//
//            });
//          }
//        }
//      }
//    });

    /*
     * Since the control is unfocused, we need to hijack paint events and draw our own selections.
     */
    final Color selectionColor = parent.getDisplay().getSystemColor(SWT.COLOR_LIST_SELECTION);
    table.addListener(SWT.EraseItem, new Listener() {
      @Override
      public void handleEvent(Event event) {
        event.detail &= ~SWT.HOT;
        if ((event.detail & SWT.SELECTED) == 0) {
          return; /* item not selected */
        }

        Widget item = event.item;
        if (item instanceof TableItem) {
          Object data = ((TableItem) item).getData();
          if (data instanceof OmniEntry) {
            if (((OmniEntry) data).element instanceof HeaderElement) {
              event.detail &= ~SWT.SELECTED;
              return;
            }
          }
        }

        int clientWidth = table.getClientArea().width;
        GC gc = event.gc;
        Color oldBackground = gc.getBackground();
        gc.setBackground(selectionColor);
        gc.fillRectangle(new Rectangle(0, event.y, clientWidth, event.height));
        gc.setBackground(oldBackground);
        event.detail &= ~SWT.SELECTED;
      }
    });

    table.addKeyListener(getKeyAdapter());
    table.addKeyListener(new KeyListener() {
      @Override
      public void keyPressed(KeyEvent e) {
        if (e.keyCode == SWT.ARROW_UP && table.getSelectionIndex() == 0) {
          setFilterFocus();
        } else if (e.character == SWT.ESC) {
          close();
        }
      }

      @Override
      public void keyReleased(KeyEvent e) {
        // do nothing
      }
    });
    table.addMouseListener(new MouseAdapter() {
      @Override
      public void mouseUp(MouseEvent e) {

        if (table.getSelectionCount() < 1) {
          return;
        }

        if (e.button != 1) {
          return;
        }

        if (table.equals(e.getSource())) {
          Object o = table.getItem(new Point(e.x, e.y));
          TableItem selection = table.getSelection()[0];
          if (selection.equals(o)) {
            handleSelection();
          }
        }
      }
    });
    table.addMouseMoveListener(new MouseMoveListener() {
      TableItem lastItem = null;

      @Override
      public void mouseMove(MouseEvent e) {
        if (table.equals(e.getSource())) {
          Object o = table.getItem(new Point(e.x, e.y));
          if (lastItem == null ^ o == null) {
            table.setCursor(o == null ? null : table.getDisplay().getSystemCursor(SWT.CURSOR_HAND));
          }
          if (o instanceof TableItem) {
            if (!o.equals(lastItem)) {
              lastItem = (TableItem) o;
              table.setSelection(new TableItem[] {lastItem});
            }
          } else if (o == null) {
            lastItem = null;
          }
        }
      }
    });

    table.addSelectionListener(new SelectionListener() {
      @Override
      public void widgetDefaultSelected(SelectionEvent e) {
        handleSelection();
      }

      @Override
      public void widgetSelected(SelectionEvent e) {
        if (Util.isMac()) {
          handleSelection();
        }
      }
    });

    final TextStyle boldStyle;
    if (PlatformUI.getPreferenceStore().getBoolean(IWorkbenchPreferenceConstants.USE_COLORED_LABELS)) {
      boldStyle = new TextStyle(boldFont, null, null);
      // italicsFont = resourceManager.createFont(FontDescriptor.createFrom(
      // table.getFont()).setStyle(SWT.ITALIC));
      grayColor = resourceManager.createColor(ColorUtil.blend(table.getBackground().getRGB(),
          table.getForeground().getRGB()));
    } else {
      boldStyle = null;
    }
    Listener listener = new Listener() {
      @Override
      public void handleEvent(Event event) {
        OmniEntry entry = (OmniEntry) event.item.getData();
        if (entry != null) {
          switch (event.type) {
            case SWT.MeasureItem:
              entry.measure(event, textLayout, resourceManager, boldStyle);
              break;
            case SWT.PaintItem:
              entry.paint(event, textLayout, resourceManager, boldStyle, grayColor);
              break;
            case SWT.EraseItem:
              entry.erase(event);
              break;
          }
        }
      }
    };

    table.addListener(SWT.MeasureItem, listener);
    table.addListener(SWT.EraseItem, listener);
    table.addListener(SWT.PaintItem, listener);

    return composite;
  }

  @Override
  protected Color getBackground() {
    return getShell().getDisplay().getSystemColor(SWT.COLOR_WHITE);
  }

  @Override
  protected Point getDefaultLocation(Point initialSize) {
    Point size = new Point(400, 400);
    Rectangle parentBounds = getParentShell().getBounds();
    int x = parentBounds.x + parentBounds.width / 2 - size.x / 2;
    int y = parentBounds.y + parentBounds.height / 2 - size.y / 2;
    return new Point(x, y);
  }

  @Override
  protected Point getDefaultSize() {
    GC gc = new GC(table);
    FontMetrics fontMetrics = gc.getFontMetrics();
    gc.dispose();
    int x = Dialog.convertHorizontalDLUsToPixels(fontMetrics, 300);
    x = Math.max(x, 350);
    int y = Dialog.convertVerticalDLUsToPixels(fontMetrics, 270);
    y = Math.max(y, 420);
    return new Point(x, y);
  }

  @Override
  protected IDialogSettings getDialogSettings() {
    final IDialogSettings dialogSettings = DartToolsPlugin.getDefault().getDialogSettings();
    IDialogSettings result = dialogSettings.getSection(getId());
    if (result == null) {
      result = dialogSettings.addNewSection(getId());
    }
    return result;
  }

  @Override
  protected Control getFocusControl() {
    if (filterControl != null) {
      return filterControl;
    }
    return super.getFocusControl();
//    return filterText;
  }

  protected String getId() {
    return getClass().getName();
  }

  final protected TriggerSequence[] getInvokingCommandKeySequences() {
    if (invokingCommandKeySequences == null) {
      if (invokingCommand != null) {
        IBindingService bindingService = (IBindingService) window.getWorkbench().getAdapter(
            IBindingService.class);
        invokingCommandKeySequences = bindingService.getActiveBindingsFor(invokingCommand.getId());
      }
    }
    return invokingCommandKeySequences;
  }

  protected void handleElementSelected(String text, Object selectedElement) {
    if (selectedElement instanceof OmniElement) {
      addPreviousPick(text, selectedElement);
      storeDialog(getDialogSettings());
      OmniElement element = (OmniElement) selectedElement;
      element.execute();
    }
  }

  protected void toggleShowAllMatches() {
    showAllMatches = !showAllMatches;
    refresh(getFilterText());
  }

  void refresh(String filter) {

    //an empty filter indicates a new query, meaning we need to clear caches
    if (filter.length() == 0) {
      for (OmniProposalProvider provider : providers) {
        provider.reset();
      }
    }

    int numItems = computeNumberOfItems();

    // perfect match, to be selected in the table if not null
    OmniElement perfectMatch = (OmniElement) elementMap.get(filter);

    List[] entries = computeMatchingEntries(filter, perfectMatch, numItems);

    int selectionIndex = refreshTable(perfectMatch, entries);

    if (table.getItemCount() > 0) {
      table.setSelection(selectionIndex);
    } else if (filter.length() == 0) {
      {
        TableItem item = new TableItem(table, SWT.NONE);
        item.setText(0, OmniBoxMessages.OmniBox_Providers);
        item.setForeground(0, grayColor);
      }
      for (int i = 0; i < providers.length; i++) {
        OmniProposalProvider provider = providers[i];
        TableItem item = new TableItem(table, SWT.NONE);
        item.setText(1, provider.getName());
        item.setForeground(1, grayColor);
      }
    }

    if (filter.length() == 0) {
      setInfoText(OmniBoxMessages.OmniBox_StartTypingToFindMatches);
    } else {
      TriggerSequence[] sequences = getInvokingCommandKeySequences();
      if (sequences != null && sequences.length != 0) {
        if (showAllMatches) {
          setInfoText(NLS.bind(OmniBoxMessages.OmniBox_PressKeyToShowInitialMatches,
              sequences[0].format()));
        } else {
          setInfoText(NLS.bind(OmniBoxMessages.OmniBox_PressKeyToShowAllMatches,
              sequences[0].format()));
        }
      } else {
        setInfoText(""); //$NON-NLS-1$
      }
    }
  }

  void setFilterControl(Text filterControl) {
    this.filterControl = filterControl;

  }

  private void addPreviousPick(String text, Object element) {
    // previousPicksList:
    // Remove element from previousPicksList so there are no duplicates
    // If list is max size, remove last(oldest) element
    // Remove entries for removed element from elementMap and textMap
    // Add element to front of previousPicksList
    previousPicksList.remove(element);
    if (previousPicksList.size() == MAXIMUM_NUMBER_OF_ELEMENTS) {
      Object removedElement = previousPicksList.removeLast();
      ArrayList<String> removedList = textMap.remove(removedElement);
      for (int i = 0; i < removedList.size(); i++) {
        elementMap.remove(removedList.get(i));
      }
    }
    previousPicksList.addFirst(element);

    // textMap:
    // Get list of strings for element from textMap
    // Create new list for element if there isn't one and put
    // element->textList in textMap
    // Remove rememberedText from list
    // If list is max size, remove first(oldest) string
    // Remove text from elementMap
    // Add rememberedText to list of strings for element in textMap
    ArrayList<String> textList = textMap.get(element);
    if (textList == null) {
      textList = new ArrayList<String>();
      textMap.put(element, textList);
    }

    textList.remove(text);
    if (textList.size() == MAXIMUM_NUMBER_OF_TEXT_ENTRIES_PER_ELEMENT) {
      Object removedText = textList.remove(0);
      elementMap.remove(removedText);
    }

    if (text.length() > 0) {
      textList.add(text);

      // elementMap:
      // Put rememberedText->element in elementMap
      // If it replaced a different element update textMap and
      // PreviousPicksList
      Object replacedElement = elementMap.put(text, element);
      if (replacedElement != null && !replacedElement.equals(element)) {
        textList = textMap.get(replacedElement);
        if (textList != null) {
          textList.remove(text);
          if (textList.isEmpty()) {
            textMap.remove(replacedElement);
            previousPicksList.remove(replacedElement);
          }
        }
      }
    }
  }

  private List[] computeMatchingEntries(String filter, OmniElement perfectMatch, int maxCount) {

    // collect matches in an array of lists
    List[] entries = new ArrayList[providers.length];
    int[] indexPerProvider = new int[providers.length];
    int countPerProvider = Math.min(maxCount / 4, INITIAL_COUNT_PER_PROVIDER);
    int countTotal = 0;
    boolean perfectMatchAdded = true;
    if (perfectMatch != null) {
      // reserve one entry for the perfect match
      maxCount--;
      perfectMatchAdded = false;
    }
    boolean done;
    do {
      // will be set to false if we find a provider with remaining
      // elements
      done = true;
      for (int i = 0; i < providers.length && (showAllMatches || countTotal < maxCount); i++) {
        if (entries[i] == null) {
          entries[i] = new ArrayList<OmniEntry>();
          indexPerProvider[i] = 0;
        }
        int count = 0;
        OmniProposalProvider provider = providers[i];
        if (filter.length() > 0 || provider instanceof PreviousPicksProvider || showAllMatches) {
          OmniElement[] elements = provider.getElementsSorted(filter);
          int j = indexPerProvider[i];
          while (j < elements.length
              && (showAllMatches || (count < countPerProvider && countTotal < maxCount))) {
            OmniElement element = elements[j];
            OmniEntry entry;
            if (filter.length() == 0) {
              if (i == 0 || showAllMatches) {
                entry = new OmniEntry(element, provider, new int[0][0], new int[0][0]);
              } else {
                entry = null;
              }
            } else {
              entry = element.match(filter, provider);
            }
            if (entry != null) {
              entries[i].add(entry);
              count++;
              countTotal++;
              if (i == 0 && entry.element == perfectMatch) {
                perfectMatchAdded = true;
                maxCount = MAX_COUNT_TOTAL;
              }
            }
            j++;
          }
          indexPerProvider[i] = j;
          if (j < elements.length) {
            done = false;
          }
        }
      }
      // from now on, add one element per provider
      countPerProvider = 1;
    } while ((showAllMatches || countTotal < maxCount) && !done);
    if (!perfectMatchAdded) {
      OmniEntry entry = perfectMatch.match(filter, providers[0]);
      if (entry != null) {
        if (entries[0] == null) {
          entries[0] = new ArrayList();
          indexPerProvider[0] = 0;
        }
        entries[0].add(entry);
      }
    }
    return entries;
  }

  private int computeNumberOfItems() {
    Rectangle rect = table.getClientArea();
    int itemHeight = table.getItemHeight();
    int headerHeight = table.getHeaderHeight();
    return (rect.height - headerHeight + itemHeight - 1) / (itemHeight + table.getGridLineWidth());
  }

  private OmniProposalProvider[] createProviders() {
    return new OmniProposalProvider[] {
        new PreviousPicksProvider(), new TypeProvider(getProgressMonitor()),
        new FileProvider(getProgressMonitor()), new EditorProvider(),
//        new ViewProvider(),
//        new CommandProvider(), new ActionProvider(),
//        new PreferenceProvider(),
//        new PropertiesProvider()
    };
  }

  private String getFilterText() {
    return filterControl.getText().toLowerCase();
  }

  private KeyAdapter getKeyAdapter() {
    if (keyAdapter == null) {
      keyAdapter = new KeyAdapter() {
        @Override
        public void keyPressed(KeyEvent e) {
          int accelerator = SWTKeySupport.convertEventToUnmodifiedAccelerator(e);
          KeySequence keySequence = KeySequence.getInstance(SWTKeySupport.convertAcceleratorToKeyStroke(accelerator));
          TriggerSequence[] sequences = getInvokingCommandKeySequences();
          if (sequences == null) {
            return;
          }
          for (int i = 0; i < sequences.length; i++) {
            if (sequences[i].equals(keySequence)) {
              e.doit = false;
              toggleShowAllMatches();
              return;
            }
          }
        }
      };
    }
    return keyAdapter;
  }

  private IProgressMonitor getProgressMonitor() {
    //TODO (pquitslund): get a proper progress monitor
    return new NullProgressMonitor();
  }

  private void handleSelection() {
    OmniElement selectedElement = null;
    String text = getFilterText();
    if (table.getSelectionCount() == 1) {
      OmniEntry entry = (OmniEntry) table.getSelection()[0].getData();
      selectedElement = entry == null ? null : entry.element;
    }
    close();
    if (selectedElement != null) {
      handleElementSelected(text, selectedElement);
    }
  }

  private boolean isHeader(TableItem item) {
    Object data = item.getData();
    if (data instanceof OmniEntry) {
      return (((OmniEntry) data).element instanceof HeaderElement);
    }
    return false;
  }

  @SuppressWarnings("rawtypes")
  private int refreshTable(OmniElement perfectMatch, List[] entries) {

    //TODO (pquitslund): clearing to force a complete redraw; prime for future optimization
    //if (table.getItemCount() > entries.length && table.getItemCount() - entries.length > 20) {
    //  table.removeAll();
    //}
    table.removeAll();

    TableItem[] items = table.getItems();
    int selectionIndex = -1;
    int index = 0;
    for (int i = 0; i < providers.length; i++) {
      if (entries[i] != null) {

        Iterator iterator = entries[i].iterator();
        TableItem item;

        //create headers for non-empty categories
        if (iterator.hasNext()) {
          item = new TableItem(table, SWT.NONE);
          item.setData(new OmniEntry(new HeaderElement(providers[i]), providers[i], new int[0][0],
              new int[0][0]));
        }
        //create entries
        for (Iterator it = entries[i].iterator(); it.hasNext();) {
          OmniEntry entry = (OmniEntry) it.next();
          if (!it.hasNext()) {
            entry.lastInCategory = true;
          }
          if (index < items.length) {
            item = items[index];
            table.clear(index);
          } else {
            item = new TableItem(table, SWT.NONE);
          }
          if (perfectMatch == entry.element && selectionIndex == -1) {
            selectionIndex = index;
          }
          item.setData(entry);
          item.setText(0, entry.provider.getName());
          item.setText(1, entry.element.getLabel());
          if (Util.isWpf()) {
            item.setImage(1, entry.getImage(entry.element, resourceManager));
          }
          index++;
        }
      }
    }
    if (index < items.length) {
      table.remove(index, items.length - 1);
    }
    if (selectionIndex == -1) {
      selectionIndex = 0;
    }
    return selectionIndex;
  }

  private void restoreDialog() {
    IDialogSettings dialogSettings = getDialogSettings();
    if (dialogSettings != null) {
      String[] orderedElements = dialogSettings.getArray(ORDERED_ELEMENTS);
      String[] orderedProviders = dialogSettings.getArray(ORDERED_PROVIDERS);
      String[] textEntries = dialogSettings.getArray(TEXT_ENTRIES);
      String[] textArray = dialogSettings.getArray(TEXT_ARRAY);
      elementMap = new HashMap<String, Object>();
      textMap = new HashMap<Object, ArrayList<String>>();
      previousPicksList = new LinkedList<Object>();
      if (orderedElements != null && orderedProviders != null && textEntries != null
          && textArray != null) {
        int arrayIndex = 0;
        for (int i = 0; i < orderedElements.length; i++) {
          OmniProposalProvider omniElementProvider = providerMap.get(orderedProviders[i]);
          int numTexts = Integer.parseInt(textEntries[i]);
          if (omniElementProvider != null) {
            OmniElement omniElement = omniElementProvider.getElementForId(orderedElements[i]);
            if (omniElement != null) {
              ArrayList<String> arrayList = new ArrayList<String>();
              for (int j = arrayIndex; j < arrayIndex + numTexts; j++) {
                String text = textArray[j];
                if (text.length() > 0) {
                  arrayList.add(text);
                  elementMap.put(text, omniElement);
                }
              }
              textMap.put(omniElement, arrayList);
              previousPicksList.add(omniElement);
            }
          }
          arrayIndex += numTexts;
        }
      }
    }
  }

  private void setFilterFocus() {
//    filterText.setFocus();
  }

  private void storeDialog(IDialogSettings dialogSettings) {
    String[] orderedElements = new String[previousPicksList.size()];
    String[] orderedProviders = new String[previousPicksList.size()];
    String[] textEntries = new String[previousPicksList.size()];
    ArrayList arrayList = new ArrayList();
    for (int i = 0; i < orderedElements.length; i++) {
      OmniElement quickAccessElement = (OmniElement) previousPicksList.get(i);
      ArrayList elementText = textMap.get(quickAccessElement);
      Assert.isNotNull(elementText);
      orderedElements[i] = quickAccessElement.getId();
      orderedProviders[i] = quickAccessElement.getProvider().getId();
      arrayList.addAll(elementText);
      textEntries[i] = elementText.size() + ""; //$NON-NLS-1$
    }
    String[] textArray = (String[]) arrayList.toArray(new String[arrayList.size()]);
    dialogSettings.put(ORDERED_ELEMENTS, orderedElements);
    dialogSettings.put(ORDERED_PROVIDERS, orderedProviders);
    dialogSettings.put(TEXT_ENTRIES, textEntries);
    dialogSettings.put(TEXT_ARRAY, textArray);
  }

}
