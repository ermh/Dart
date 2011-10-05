// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

typedef bool EnabledFunction();
typedef Function CallbackFunction(Element elmt, int value);

class ElementFunc {
  Element _element;
  EnabledFunction _func;
  
  Element get element() {
    return _element;
  }

  EnabledFunction get func() {
    return _func;
  }

  ElementFunc(this._element, this._func) { }
}

class ContextMenu {
  Element _contextMenu;
  List<ElementFunc> _enableFunctions;
  Spreadsheet _spreadsheet;
  SpreadsheetPresenter _spreadsheetPresenter;

  ContextMenu(this._spreadsheetPresenter) {
    Document doc = _spreadsheetPresenter.window.document;
    _contextMenu = doc.createElement("ul");
    _contextMenu.style.setProperty("left","0px");
    _contextMenu.style.setProperty("top", "0px");
    _contextMenu.classes.add("contextMenu");
    _contextMenu.classes.add("fadeOut");
    _spreadsheetPresenter.spreadsheetElement.nodes.add(_contextMenu);
    _spreadsheet = _spreadsheetPresenter.spreadsheet;
  }

  Element addMenuItem(String label, int value,
      EnabledFunction enabled, CallbackFunction callback) {
    Element item = _spreadsheetPresenter.window.document.createElement("li");
    item.classes.add("contextMenuItem");
    item.classes.add("contextMenuItem-enabled");
    item.innerHTML = label;
    item.on.click.add((Event e) {
      _spreadsheetPresenter.popdownMenu(_contextMenu, value, callback);
    });
    _contextMenu.nodes.add(item);
    if (_enableFunctions == null) {
      _enableFunctions = new List<ElementFunc>();
    }
    _enableFunctions.add(new ElementFunc(item, enabled));
    return item;
  }

  // Display the popup at the given coordinates
  void show(int x, int y) {
    // Enable or disable the menu items
    _enableFunctions.forEach((ElementFunc elementFunc) {
      _enableMenuItem(elementFunc.element, (elementFunc.func)());
    });
    _spreadsheetPresenter.popupMenu(_contextMenu, x, y);
  }

  // Enable or disable a particular menu item
  void _enableMenuItem(Element item, bool enabled) {
    if (enabled) {
      item.classes.remove("contextMenuItem-disabled");
      item.classes.add("contextMenuItem-enabled");
    } else {
      item.classes.remove("contextMenuItem-enabled");
      item.classes.add("contextMenuItem-disabled");
    }
  }
}
