// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

/**
 * An object for holding cell locations.
 *
 * This class is immutable: instances cannot be changed after creation.
 */
class CellLocation {
  final RowCol _rowCol;
  final Spreadsheet _spreadsheet;

  int get col() {
    return _rowCol.col;
  }

  int get row() {
    return _rowCol.row;
  }

  RowCol get rowCol() {
    return _rowCol;
  }

  Spreadsheet get spreadsheet() {
    return _spreadsheet;
  }


  CellLocation(this._spreadsheet, this._rowCol) { }

  bool operator ==(CellLocation other) {
    if (!(other is CellLocation)) {
      return false;
    }
    return _spreadsheet === other._spreadsheet && other._rowCol == _rowCol;
  }

  CellLocation operator +(RowCol rowCol) {
    return new CellLocation(_spreadsheet, _rowCol + rowCol);
  }

  // Convenience method for 'spreadsheet.getCell(rowCol)'
  Cell getCell() {
    return _spreadsheet.getCell(_rowCol);
  }

  int hashCode() {
    return (_spreadsheet.hashCode() * 31) ^ _rowCol.hashCode();
  }

  bool isValidCell() {
    return _rowCol.isValidCell();
  }

  // Convenience method for 'spreadsheet.markDirty(rowCol)'
  void markDirty() {
    _spreadsheet.markDirty(_rowCol);
  }

  // Convenience method for 'spreadsheet.setCellContentFromString(rowCol, content)'
  Cell setCellFromContentString(String content) {
    return _spreadsheet.setCellFromContentString(_rowCol, content);
  }

  String toString() {
    // TODO: Sheets should have names, and we should use them here.
    // But we probably want a flag, because when printing strings
    // we would normally only add the spreadsheet ID if the formula or
    // whatever we are printing is in a different spreadsheet.
    return _rowCol.toString();
  }
}
