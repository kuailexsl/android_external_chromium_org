// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

////////////////////////////////////////////////////////////////////////////////
// DirectoryTreeUtil

/**
 * Utility methods. They are intended for use only in this file.
 */
var DirectoryTreeUtil = {};

/**
 * Updates sub-elements of {@code parentElement} reading {@code DirectoryEntry}
 * with calling {@code iterator}.
 *
 * @param {string} changedDirectryPath The path of the changed directory.
 * @param {DirectoryItem|DirectoryTree} currentDirectoryItem An item to be
 *     started traversal from.
 */
DirectoryTreeUtil.updateChangedDirectoryItem = function(
    changedDirectryPath, currentDirectoryItem) {
  if (changedDirectryPath === currentDirectoryItem.entry.fullPath) {
    currentDirectoryItem.updateSubDirectories(false /* recursive */);
    return;
  }

  for (var i = 0; i < currentDirectoryItem.items.length; i++) {
    var item = currentDirectoryItem.items[i];
    if (PathUtil.isParentPath(item.entry.fullPath, changedDirectryPath)) {
      DirectoryTreeUtil.updateChangedDirectoryItem(changedDirectryPath, item);
      break;
    }
  }
};

/**
 * Updates sub-elements of {@code parentElement} reading {@code DirectoryEntry}
 * with calling {@code iterator}.
 *
 * @param {DirectoryItem|DirectoryTree} parentElement Parent element of newly
 *     created items.
 * @param {function(number): DirectoryEntry} iterator Function which returns
 *     the n-th Entry in the directory.
 * @param {DirectoryTree} tree Current directory tree, which contains this item.
 * @param {boolean} recursive True if the all visible sub-directories are
 *     updated recursively including left arrows. If false, the update walks
 *     only immediate child directories without arrows.
 */
DirectoryTreeUtil.updateSubElementsFromList = function(
    parentElement, iterator, tree, recursive) {
  var index = 0;
  while (iterator(index)) {
    var currentEntry = iterator(index);
    var currentElement = parentElement.items[index];

    if (index >= parentElement.items.length) {
      var item = new DirectoryItem(currentEntry, parentElement, tree);
      parentElement.add(item);
      index++;
    } else if (currentEntry.fullPath == currentElement.fullPath) {
      if (recursive && parentElement.expanded)
        currentElement.updateSubDirectories(true /* recursive */);

      index++;
    } else if (currentEntry.fullPath < currentElement.fullPath) {
      var item = new DirectoryItem(currentEntry, parentElement, tree);
      parentElement.addAt(item, index);
      index++;
    } else if (currentEntry.fullPath > currentElement.fullPath) {
      parentElement.remove(currentElement);
    }
  }

  var removedChild;
  while (removedChild = parentElement.items[index]) {
    parentElement.remove(removedChild);
  }

  if (index == 0) {
    parentElement.hasChildren = false;
    parentElement.expanded = false;
  } else {
    parentElement.hasChildren = true;
  }
};

/**
 * Finds a parent directory of the {@code entry} from the {@code items}, and
 * invokes the DirectoryItem.selectByEntry() of the found directory.
 *
 * @param {Array.<DirectoryItem>} items Items to be searched.
 * @param {DirectoryEntry|Object} entry The entry to be searched for. Can be
 *     a fake.
 * @return {boolean} True if the parent item is found.
 */
DirectoryTreeUtil.searchAndSelectByEntry = function(items, entry) {
  for (var i = 0; i < items.length; i++) {
    var item = items[i];
    if (util.isParentEntry(item.entry, entry)) {
      item.selectByEntry(entry);
      return true;
    }
  }
  return false;
};

/**
 * Modifies a list of the directory entries to match the new UI sepc.
 *
 * TODO(yoshiki): remove this after the old UI is removed.
 *
 * @param {Array.<DirectoryEntry>} entries The list of entty.
 * @return {Array.<DirectoryEntries>} Modified entries.
 */
DirectoryTreeUtil.addAndRemoveDriveSpecialDirs = function(entries) {
  var modifiedEntries = [];
  for (var i in entries) {
    // Removes '/drive/other'.
    var entry = entries[i];
    if (entry.fullPath ==
        (RootDirectory.DRIVE + '/' + DriveSubRootDirectory.OTHER)) {
      continue;
    }

    // Changes the label of '/drive/root' to 'My Drive'.
    if (entry.fullPath == DirectoryModel.fakeDriveEntry_.fullPath) {
      entry.label = str('DRIVE_MY_DRIVE_LABEL');
    }

    modifiedEntries.push(entry);
  }

  // Adds the special directories.
  var specialDirs = DirectoryModel.FAKE_DRIVE_SPECIAL_SEARCH_ENTRIES;
  for (var i in specialDirs) {
    var dir = specialDirs[i];
    dir['label'] = PathUtil.getRootLabel(dir.fullPath);
    modifiedEntries.push(dir);
  }
  return modifiedEntries;
};

/**
 * Retrieves the file list with the latest information.
 *
 * @param {DirectoryTree|DirectoryItem} item Parent to be reloaded.
 * @param {DirectoryModel} dm The directory model.
 * @param {function(Array.<Entry>)} successCallback Callback on success.
 * @param {function()=} opt_errorCallback Callback on failure.
 */
DirectoryTreeUtil.updateSubDirectories = function(
    item, dm, successCallback, opt_errorCallback) {
  // Tries to retrieve new entry if the cached entry is dummy.
  if (util.isFakeDirectoryEntry(item.entry)) {
    // Fake Drive root.
    dm.resolveDirectory(
        item.fullPath,
        function(entry) {
          item.dirEntry_ = entry;

          // If the retrieved entry is dummy again, returns with an error.
          if (util.isFakeDirectoryEntry(entry)) {
            if (opt_errorCallback)
              opt_errorCallback();
            return;
          }

          DirectoryTreeUtil.updateSubDirectories(
              item, dm, successCallback, opt_errorCallback);
        },
        opt_errorCallback || function() {});
    return;
  }

  var reader = item.entry.createReader();
  var entries = [];
  var readEntry = function() {
    reader.readEntries(function(results) {
      if (!results.length) {
        if (item.entry.fullPath == RootDirectory.DRIVE)
          successCallback(
              DirectoryTreeUtil.addAndRemoveDriveSpecialDirs(entries));
        else
          successCallback(
              DirectoryTreeUtil.sortEntries(item.fileFilter_, entries));
        return;
      }

      for (var i = 0; i < results.length; i++) {
        var entry = results[i];
        if (entry.isDirectory)
          entries.push(entry);
      }
      readEntry();
    });
  };
  readEntry();
};

/**
 * Sorts a list of entries.
 *
 * @param {FileFilter} fileFilter The file filter.
 * @param {Array.<Entries>} entries Entries to be sorted.
 * @return {Array.<Entries>} Sorted entries.
 */
DirectoryTreeUtil.sortEntries = function(fileFilter, entries) {
  entries.sort(function(a, b) {
    return (a.name.toLowerCase() > b.name.toLowerCase()) ? 1 : -1;
  });
  return entries.filter(fileFilter.filter.bind(fileFilter));
};

/**
 * Checks if tre tree should be hidden on the given directory.
 *
 * @param {string} path Path to be checked.
 * @return {boolean} True if the tree should NOT be visible on the given
 *     directory. Otherwise, false.
 */
DirectoryTreeUtil.shouldHideTree = function(path) {
  return !PathUtil.isDriveBasedPath(path);
};

////////////////////////////////////////////////////////////////////////////////
// DirectoryItem

/**
 * A directory in the tree. Each element represents one directory.
 *
 * @param {DirectoryEntry} dirEntry DirectoryEntry of this item.
 * @param {DirectoryItem|DirectoryTree} parentDirItem Parent of this item.
 * @param {DirectoryTree} tree Current tree, which contains this item.
 * @extends {cr.ui.TreeItem}
 * @constructor
 */
function DirectoryItem(dirEntry, parentDirItem, tree) {
  var item = cr.doc.createElement('div');
  DirectoryItem.decorate(item, dirEntry, parentDirItem, tree);
  return item;
}

/**
 * @param {HTMLElement} el Element to be DirectoryItem.
 * @param {DirectoryEntry} dirEntry DirectoryEntry of this item.
 * @param {DirectoryItem|DirectoryTree} parentDirItem Parent of this item.
 * @param {DirectoryTree} tree Current tree, which contains this item.
 */
DirectoryItem.decorate =
    function(el, dirEntry, parentDirItem, tree) {
  el.__proto__ = DirectoryItem.prototype;
  (/** @type {DirectoryItem} */ el).decorate(
      dirEntry, parentDirItem, tree);
};

DirectoryItem.prototype = {
  __proto__: cr.ui.TreeItem.prototype,

  /**
   * The DirectoryEntry corresponding to this DirectoryItem. This may be
   * a dummy DirectoryEntry.
   * @type {DirectoryEntry|Object}
   */
  get entry() {
    return this.dirEntry_;
  },

  /**
   * The element containing the label text and the icon.
   * @type {!HTMLElement}
   * @override
   */
  get labelElement() {
    return this.firstElementChild.querySelector('.label');
  }
};

/**
 * @param {DirectoryEntry} dirEntry DirectoryEntry of this item.
 * @param {DirectoryItem|DirectoryTree} parentDirItem Parent of this item.
 * @param {DirectoryTree} tree Current tree, which contains this item.
 */
DirectoryItem.prototype.decorate = function(
    dirEntry, parentDirItem, tree) {
  var path = dirEntry.fullPath;
  var label;
  label = dirEntry.label ? dirEntry.label : dirEntry.name;

  this.className = 'tree-item';
  this.innerHTML =
      '<div class="tree-row">' +
      ' <span class="expand-icon"></span>' +
      ' <span class="icon"></span>' +
      ' <span class="label"></span>' +
      ' <div class="root-eject"></div>' +
      '</div>' +
      '<div class="tree-children"></div>';
  this.setAttribute('role', 'treeitem');

  this.parentTree_ = tree;
  this.directoryModel_ = tree.directoryModel;
  this.parent_ = parentDirItem;
  this.label = label;
  this.fullPath = path;
  this.dirEntry_ = dirEntry;
  this.fileFilter_ = this.directoryModel_.getFileFilter();

  // Sets hasChildren=false tentatively. This will be overridden after
  // scanning sub-directories in DirectoryTreeUtil.updateSubElementsFromList.
  this.hasChildren = false;

  this.addEventListener('expand', this.onExpand_.bind(this), false);
  var volumeManager = VolumeManager.getInstance();
  var icon = this.querySelector('.icon');
  icon.classList.add('volume-icon');
  var iconType = PathUtil.getRootType(path);
  if (iconType && PathUtil.isRootPath(path))
    icon.setAttribute('volume-type-icon', iconType);
  else
    icon.setAttribute('file-type-icon', 'folder');

  var eject = this.querySelector('.root-eject');
  eject.hidden = !PathUtil.isUnmountableByUser(path);
  eject.addEventListener('click',
      function(event) {
        event.stopPropagation();
        if (!PathUtil.isUnmountableByUser(path))
          return;

        volumeManager.unmount(path, function() {}, function() {});
      }.bind(this));

  if (this.parentTree_.contextMenuForSubitems)
    this.setContextMenu(this.parentTree_.contextMenuForSubitems);
  // Adds handler for future change.
  this.parentTree_.addEventListener(
      'contextMenuForSubitemsChange',
      function(e) { this.setContextMenu(e.newValue); }.bind(this));

  if (parentDirItem.expanded)
    this.updateSubDirectories(false /* recursive */);
};

/**
 * Overrides WebKit's scrollIntoViewIfNeeded, which doesn't work well with
 * a complex layout. This call is not necessary, so we are ignoring it.
 *
 * @param {boolean} unused Unused.
 * @override
 */
DirectoryItem.prototype.scrollIntoViewIfNeeded = function(unused) {
};

/**
 * Removes the child node, but without selecting the parent item, to avoid
 * unintended changing of directories. Removing is done externally, and other
 * code will navigate to another directory.
 *
 * @param {!cr.ui.TreeItem} child The tree item child to remove.
 * @override
 */
DirectoryItem.prototype.remove = function(child) {
  this.lastElementChild.removeChild(child);
  if (this.items.length == 0)
    this.hasChildren = false;
};

/**
 * Invoked when the item is being expanded.
 * @param {!UIEvent} e Event.
 * @private
 **/
DirectoryItem.prototype.onExpand_ = function(e) {
  this.updateSubDirectories(
      true /* recursive */,
      function() {},
      function() {
        this.expanded = false;
      }.bind(this));

  e.stopPropagation();
};

/**
 * Retrieves the latest subdirectories and update them on the tree.
 * @param {boolean} recursive True if the update is recursively.
 * @param {function()=} opt_successCallback Callback called on success.
 * @param {function()=} opt_errorCallback Callback called on error.
 */
DirectoryItem.prototype.updateSubDirectories = function(
    recursive, opt_successCallback, opt_errorCallback) {
  DirectoryTreeUtil.updateSubDirectories(
      this,
      this.directoryModel_,
      function(entries) {
        this.entries_ = entries;
        this.redrawSubDirectoryList_(recursive);
        opt_successCallback && opt_successCallback();
      }.bind(this),
      opt_errorCallback);
};

/**
 * Redraw subitems with the latest information. The items are sorted in
 * alphabetical order, case insensitive.
 * @param {boolean} recursive True if the update is recursively.
 * @private
 */
DirectoryItem.prototype.redrawSubDirectoryList_ = function(recursive) {
  DirectoryTreeUtil.updateSubElementsFromList(
      this,
      function(i) { return this.entries_[i]; }.bind(this),
      this.parentTree_,
      recursive);
};

/**
 * Select the item corresponding to the given {@code entry}.
 * @param {DirectoryEntry|Object} entry The entry to be selected. Can be a fake.
 */
DirectoryItem.prototype.selectByEntry = function(entry) {
  if (util.isSameEntry(entry, this.entry)) {
    this.selected = true;
    return;
  }

  if (DirectoryTreeUtil.searchAndSelectByEntry(this.items, entry))
    return;

  // If the path doesn't exist, updates sub directories and tryes again.
  this.updateSubDirectories(
      false /* recursive */,
      DirectoryTreeUtil.searchAndSelectByEntry.bind(null, this.items, entry));
};

/**
 * Executes the assigned action as a drop target.
 */
DirectoryItem.prototype.doDropTargetAction = function() {
  this.expanded = true;
};

/**
 * Executes the assigned action. DirectoryItem performs changeDirectory.
 */
DirectoryItem.prototype.doAction = function() {
  if (this.fullPath != this.directoryModel_.getCurrentDirPath())
    this.directoryModel_.changeDirectory(this.fullPath);
};

/**
 * Sets the context menu for directory tree.
 * @param {cr.ui.Menu} menu Menu to be set.
 */
DirectoryItem.prototype.setContextMenu = function(menu) {
  if (this.entry && PathUtil.isEligibleForFolderShortcut(this.entry.fullPath))
    cr.ui.contextMenuHandler.setContextMenu(this, menu);
};

////////////////////////////////////////////////////////////////////////////////
// DirectoryTree

/**
 * Tree of directories on the sidebar. This element is also the root of items,
 * in other words, this is the parent of the top-level items.
 *
 * @constructor
 * @extends {cr.ui.Tree}
 */
function DirectoryTree() {}

/**
 * Decorates an element.
 * @param {HTMLElement} el Element to be DirectoryTree.
 * @param {DirectoryModel} directoryModel Current DirectoryModel.
 */
DirectoryTree.decorate = function(el, directoryModel) {
  el.__proto__ = DirectoryTree.prototype;
  (/** @type {DirectoryTree} */ el).decorate(directoryModel);
};

DirectoryTree.prototype = {
  __proto__: cr.ui.Tree.prototype,

  // DirectoryTree is always expanded.
  get expanded() { return true; },
  /**
   * @param {boolean} value Not used.
   */
  set expanded(value) {},

  /**
   * The DirectoryEntry corresponding to this DirectoryItem. This may be
   * a dummy DirectoryEntry.
   * @type {DirectoryEntry|Object}
   * @override
   **/
  get entry() {
      return this.dirEntry_;
  },

  get directoryModel() {
    return this.directoryModel_;
  }
};

cr.defineProperty(DirectoryTree, 'contextMenuForSubitems', cr.PropertyKind.JS);

/**
 * Decorates an element.
 * @param {DirectoryModel} directoryModel Current DirectoryModel.
 */
DirectoryTree.prototype.decorate = function(directoryModel) {
  cr.ui.Tree.prototype.decorate.call(this);

  this.directoryModel_ = directoryModel;
  this.entries_ = [];

  this.fileFilter_ = this.directoryModel_.getFileFilter();
  this.fileFilter_.addEventListener('changed',
                                    this.onFilterChanged_.bind(this));
  /**
   * The path of the root directory.
   * @type {string}
   */
  this.fullPath = '/';
  this.dirEntry_ = null;

  /**
   * The currently selected entry.
   * For special search entries, this could be a fake DirectoryEntry.
   * @type {DirectoryEntry|Object}
   */
  this.currentEntry_ = null;

  this.directoryModel_.addEventListener('directory-changed',
      this.onCurrentDirectoryChanged_.bind(this));

  // Add a handler for directory change.
  this.addEventListener('change', function() {
    if (this.selectedItem &&
        (!this.currentEntry_ ||
         !util.isSameEntry(this.currentEntry_, this.selectedItem.entry))) {
      this.currentEntry_ = this.selectedItem.entry;
      this.selectedItem.doAction();
      return;
    }
  }.bind(this));

  this.privateOnDirectoryChangedBound_ =
      this.onDirectoryContentChanged_.bind(this);
  chrome.fileBrowserPrivate.onDirectoryChanged.addListener(
      this.privateOnDirectoryChangedBound_);

  this.scrollBar_ = MainPanelScrollBar();
  this.scrollBar_.initialize(this.parentNode, this);
};

/**
 * Select the item corresponding to the given entry.
 * @param {DirectoryEntry|Object} entry The directory entry to be selected. Can
 *     be a fake.
 */
DirectoryTree.prototype.selectByEntry = function(entry) {
  if ((this.entry && util.isSameEntry(this.entry, entry)) ||
      (this.currentEntry_ && util.isSameEntry(this.currentEntry_, entry)))
    return;

  this.currentEntry_ = entry;
  if (DirectoryTreeUtil.shouldHideTree(entry.fullPath))
    return;

  this.selectByEntryInternal_(entry);
};

/**
 * Select the item corresponding to the given entry. This method is used
 * internally.
 * @param {DirectoryEntry|Object} entry The directory entry to be selected.
 * @private
 */
DirectoryTree.prototype.selectByEntryInternal_ = function(entry) {
  var rootDirPath = PathUtil.getRootPath(entry.fullPath);

  if (PathUtil.isSpecialSearchRoot(rootDirPath) ||
      PathUtil.getRootType(rootDirPath) == RootType.DRIVE) {
    rootDirPath = RootDirectory.DRIVE;
  }

  var onError = function() {
    this.clearTree_(true);
  }.bind(this);

  if (this.fullPath != rootDirPath || !this.dirEntry_) {
    // The old root is not the one for the new entry. So, first we try to set
    // the root, then retry.
    this.fullPath = rootDirPath;

    this.directoryModel_.resolveDirectory(
        rootDirPath,
        function(rootDirEntry) {
          if (this.fullPath != rootDirPath)
            return;

          this.dirEntry_ = rootDirEntry;
          this.selectByEntryInternal_(entry);
        }.bind(this),
        onError);
    return;
  }

  if (this.selectedItem && util.isSameEntry(entry, this.selectedItem.entry))
    return;

  if (DirectoryTreeUtil.searchAndSelectByEntry(this.items, entry))
    return;

  this.selectedItem = null;
  this.updateSubDirectories(
      false /* recursive */,
      function() {
        if (!DirectoryTreeUtil.searchAndSelectByEntry(
            this.items, this.currentEntry_))
          this.selectedItem = null;
      }.bind(this),
      onError);
};

/**
 * Retrieves the latest subdirectories and update them on the tree.
 * @param {boolean} recursive True if the update is recursively.
 * @param {function()=} opt_successCallback Callback called on success.
 * @param {function()=} opt_errorCallback Callback called on error.
 */
DirectoryTree.prototype.updateSubDirectories = function(
    recursive, opt_successCallback, opt_errorCallback) {
  if (!this.currentEntry_)
    return;

  DirectoryTreeUtil.updateSubDirectories(
      this,
      this.directoryModel_,
      function(entries) {
        this.entries_ = entries;
        this.redraw(recursive);
        if (opt_successCallback)
          opt_successCallback();
      }.bind(this),
      opt_errorCallback);
};

/**
 * Redraw the list.
 * @param {boolean} recursive True if the update is recursively. False if the
 *     only root items are updated.
 */
DirectoryTree.prototype.redraw = function(recursive) {
  DirectoryTreeUtil.updateSubElementsFromList(
      this,
      function(i) { return this.entries_[i]; }.bind(this),
      this,
      recursive);
};

/**
 * Invoked when the filter is changed.
 * @private
 */
DirectoryTree.prototype.onFilterChanged_ = function() {
  // Returns immediately, if the tree is hidden.
  if (!this.currentEntry_ ||
      DirectoryTreeUtil.shouldHideTree(this.currentEntry_.fullPath))
    return;

  this.redraw(true /* recursive */);
};

/**
 * Invoked when a directory is changed.
 * @param {!UIEvent} event Event.
 * @private
 */
DirectoryTree.prototype.onDirectoryContentChanged_ = function(event) {
  // Returns immediately, if the tree is hidden.
  if (!this.currentEntry_ ||
      DirectoryTreeUtil.shouldHideTree(this.currentEntry_.fullPath))
    return;

  if (event.eventType == 'changed') {
    var path = util.extractFilePath(event.directoryUrl);
    DirectoryTreeUtil.updateChangedDirectoryItem(path, this);
  }
};

/**
 * Invoked when the current directory is changed.
 * @param {!UIEvent} event Event.
 * @private
 */
DirectoryTree.prototype.onCurrentDirectoryChanged_ = function(event) {
  this.selectByEntry(event.newDirEntry);
};

/**
 * Returns the path of the selected item.
 * @return {string} The current path.
 */
DirectoryTree.prototype.getCurrentPath = function() {
  return this.selectedItem ? this.selectedItem.fullPath : null;
};

/**
 * Clears the tree.
 * @param {boolean} redraw Redraw the tree if true.
 * @private
 */
DirectoryTree.prototype.clearTree_ = function(redraw) {
  this.dirEntry_ = null;
  this.fullPath = '';
  this.selectedItem = null;
  this.entries_ = [];

  if (redraw)
    this.redraw(false);
};

/**
 * Sets the margin height for the transparent preview panel at the bottom.
 * @param {number} margin Margin to be set in px.
 */
DirectoryTree.prototype.setBottomMarginForPanel = function(margin) {
  this.style.paddingBottom = margin + 'px';
  this.scrollBar_.setBottomMarginForPanel(margin);
};

/**
 * Updates the UI after the layout has changed.
 */
DirectoryTree.prototype.relayout = function() {
  cr.dispatchSimpleEvent(this, 'relayout');
};
