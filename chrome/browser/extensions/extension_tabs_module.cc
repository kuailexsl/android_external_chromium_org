// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_tabs_module.h"

#include "base/string_util.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/extensions/extension_function_dispatcher.h"
#include "chrome/browser/renderer_host/render_view_host_delegate.h"
#include "chrome/browser/tab_contents/navigation_entry.h"

// TODO(port): Port these files.
#if defined(OS_WIN)
#include "chrome/browser/window_sizer.h"
#else
#include "chrome/common/temp_scaffolding_stubs.h"
#endif

namespace {
// keys
const wchar_t* kIdKey = L"id";
const wchar_t* kIndexKey = L"index";
const wchar_t* kWindowIdKey = L"windowId";
const wchar_t* kUrlKey = L"url";
const wchar_t* kTitleKey = L"title";
const wchar_t* kSelectedKey = L"selected";
const wchar_t* kFocusedKey = L"focused";
const wchar_t* kFavIconUrlKey = L"favIconUrl";
const wchar_t* kLeftKey = L"left";
const wchar_t* kTopKey = L"top";
const wchar_t* kWidthKey = L"width";
const wchar_t* kHeightKey = L"height";
const wchar_t* kTabsKey = L"tabs";
}

// Forward declare static helper functions defined below.
static DictionaryValue* CreateWindowValue(Browser* browser, bool populate_tabs);
static ListValue* CreateTabList(Browser* browser);
static Browser* GetBrowserInProfileWithId(Profile* profile,
                                          const int window_id);

// ExtensionTabUtil
int ExtensionTabUtil::GetWindowId(const Browser* browser) {
  return browser->session_id().id();
}

int ExtensionTabUtil::GetTabId(const TabContents* tab_contents) {
  return tab_contents->controller().session_id().id();
}

int ExtensionTabUtil::GetWindowIdOfTab(const TabContents* tab_contents) {
  return tab_contents->controller().window_id().id();
}

DictionaryValue* ExtensionTabUtil::CreateTabValue(
    const TabContents* contents) {
  // Find the tab strip and index of this guy.
  for (BrowserList::const_iterator it = BrowserList::begin();
      it != BrowserList::end(); ++it) {
    TabStripModel* tab_strip = (*it)->tabstrip_model();
    int tab_index = tab_strip->GetIndexOfTabContents(contents);
    if (tab_index != -1) {
      return ExtensionTabUtil::CreateTabValue(contents, tab_strip, tab_index);
    }
  }

  // Couldn't find it.  This can happen if the tab is being dragged.
  return ExtensionTabUtil::CreateTabValue(contents, NULL, -1);
}

DictionaryValue* ExtensionTabUtil::CreateTabValue(
    const TabContents* contents, TabStripModel* tab_strip, int tab_index) {
  DictionaryValue* result = new DictionaryValue();
  result->SetInteger(kIdKey, ExtensionTabUtil::GetTabId(contents));
  result->SetInteger(kIndexKey, tab_index);
  result->SetInteger(kWindowIdKey,
                     ExtensionTabUtil::GetWindowIdOfTab(contents));
  result->SetString(kUrlKey, contents->GetURL().spec());
  result->SetString(kTitleKey, UTF16ToWide(contents->GetTitle()));
  result->SetBoolean(kSelectedKey,
                     tab_strip && tab_index == tab_strip->selected_index());

  NavigationEntry* entry = contents->controller().GetActiveEntry();
  if (entry) {
    if (entry->favicon().is_valid())
      result->SetString(kFavIconUrlKey, entry->favicon().url().spec());
  }

  return result;
}

bool ExtensionTabUtil::GetTabById(int tab_id, Profile* profile,
                                  Browser** browser,
                                  TabStripModel** tab_strip,
                                  TabContents** contents,
                                  int* tab_index) {
  Browser* target_browser;
  TabStripModel* target_tab_strip;
  TabContents* target_contents;
  for (BrowserList::const_iterator iter = BrowserList::begin();
       iter != BrowserList::end(); ++iter) {
    target_browser = *iter;
    if (target_browser->profile() == profile) {
      target_tab_strip = target_browser->tabstrip_model();
      for (int i = 0; i < target_tab_strip->count(); ++i) {
        target_contents = target_tab_strip->GetTabContentsAt(i);
        if (target_contents->controller().session_id().id() == tab_id) {
          if (browser)
            *browser = target_browser;
          if (tab_strip)
            *tab_strip = target_tab_strip;
          if (contents)
            *contents = target_contents;
          if (tab_index)
            *tab_index = i;
          return true;
        }
      }
    }
  }
  return false;
}

static bool GetWindowFunctionHelper(Browser* browser, Profile* profile,
                            scoped_ptr<Value>* result) {
  // TODO(rafaelw): need "not found" error message.
  if (browser == NULL || browser->profile() != profile)
    return false;

  result->reset(CreateWindowValue(browser, false));

  return true;
}

// Windows ---------------------------------------------------------------------

bool GetWindowFunction::RunImpl() {
  int window_id;
  EXTENSION_FUNCTION_VALIDATE(args_->GetAsInteger(&window_id));

  Browser* target = GetBrowserInProfileWithId(profile(), window_id);
  return GetWindowFunctionHelper(target, profile(), &result_);
}

bool GetCurrentWindowFunction::RunImpl() {
  return GetWindowFunctionHelper(dispatcher_->browser(), profile(), &result_);
}

bool GetFocusedWindowFunction::RunImpl() {
  return GetWindowFunctionHelper(BrowserList::GetLastActive(), profile(),
      &result_);
}

bool GetAllWindowsFunction::RunImpl() {
  bool populate_tabs = false;
  if (!args_->IsType(Value::TYPE_NULL)) {
    EXTENSION_FUNCTION_VALIDATE(args_->GetAsBoolean(&populate_tabs));
  }

  result_.reset(new ListValue());
  for (BrowserList::const_iterator browser = BrowserList::begin();
    browser != BrowserList::end(); ++browser) {
      // Only examine browsers in the current profile.
      if ((*browser)->profile() == profile()) {
        static_cast<ListValue*>(result_.get())->
          Append(CreateWindowValue(*browser, populate_tabs));
      }
  }

  return true;
}

bool CreateWindowFunction::RunImpl() {
  scoped_ptr<GURL> url(new GURL());

  // Look for optional url.
  if (!args_->IsType(Value::TYPE_NULL)) {
    EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_DICTIONARY));
    const DictionaryValue *args = static_cast<const DictionaryValue*>(args_);
    std::string url_input;
    if (args->HasKey(kUrlKey)) {
      EXTENSION_FUNCTION_VALIDATE(args->GetString(kUrlKey, &url_input));
      url.reset(new GURL(url_input));
      if (!url->is_valid()) {
        // TODO(rafaelw): need error message/callback here
        return false;
      }
    }
  }

  // Try to get the browser associated with view that this call came from, so
  // its position can be set relative to its browser window.
  Browser* browser = dispatcher_->browser();
  if (browser == NULL)
    browser = BrowserList::GetLastActiveWithProfile(dispatcher_->profile());

  // Try to position the new browser relative its originating browser window.
  gfx::Rect empty_bounds;
  gfx::Rect bounds;
  bool maximized;
  // The call offsets the bounds by kWindowTilePixels (defined in WindowSizer to
  // be 10).
  WindowSizer::GetBrowserWindowBounds(std::wstring(), empty_bounds, browser,
      &bounds, &maximized);

  // Any part of the bounds can optionally be set by the caller.
  if (args_->IsType(Value::TYPE_DICTIONARY)) {
    const DictionaryValue *args = static_cast<const DictionaryValue*>(args_);
    int bounds_val;
    if (args->HasKey(kLeftKey)) {
      EXTENSION_FUNCTION_VALIDATE(args->GetInteger(kLeftKey, &bounds_val));
      bounds.set_x(bounds_val);
    }

    if (args->HasKey(kTopKey)) {
      EXTENSION_FUNCTION_VALIDATE(args->GetInteger(kTopKey, &bounds_val));
      bounds.set_y(bounds_val);
    }

    if (args->HasKey(kWidthKey)) {
      EXTENSION_FUNCTION_VALIDATE(args->GetInteger(kWidthKey, &bounds_val));
      bounds.set_width(bounds_val);
    }

    if (args->HasKey(kHeightKey)) {
      EXTENSION_FUNCTION_VALIDATE(args->GetInteger(kHeightKey, &bounds_val));
      bounds.set_height(bounds_val);
    }
  }

  Browser* new_window = Browser::Create(dispatcher_->profile());
  new_window->AddTabWithURL(*(url.get()), GURL(), PageTransition::LINK, true,
                            -1, false, NULL);

  new_window->window()->SetBounds(bounds);
  new_window->window()->Show();

  // TODO(rafaelw): support |focused|, |zIndex|

  result_.reset(CreateWindowValue(new_window, false));

  return true;
}

bool RemoveWindowFunction::RunImpl() {
  int window_id;
  EXTENSION_FUNCTION_VALIDATE(args_->GetAsInteger(&window_id));

  Browser* target = NULL;
  for (BrowserList::const_iterator browser = BrowserList::begin();
      browser != BrowserList::end(); ++browser) {
    // Only examine browsers in the current profile.
    if ((*browser)->profile() == profile()) {
      if (ExtensionTabUtil::GetWindowId(*browser) == window_id) {
        target = *browser;
        break;
      }
    }
  }

  if (target == NULL) {
    // TODO(rafaelw): need error message.
    return false;
  }

  target->CloseWindow();

  return true;
}

// Tabs ---------------------------------------------------------------------

bool GetSelectedTabFunction::RunImpl() {
  int window_id;
  Browser* browser;
  // windowId defaults to "current" window.
  if (!args_->IsType(Value::TYPE_NULL)) {
    EXTENSION_FUNCTION_VALIDATE(args_->GetAsInteger(&window_id));
    browser = GetBrowserInProfileWithId(profile(), window_id);
  } else {
    browser = dispatcher_->browser();
  }

  if (!browser)
    // TODO(rafaelw): return a "no 'current' browser" error.
    return false;

  TabStripModel* tab_strip = browser->tabstrip_model();
  result_.reset(ExtensionTabUtil::CreateTabValue(
      tab_strip->GetSelectedTabContents(),
      tab_strip,
      tab_strip->selected_index()));
  return true;
}

bool GetAllTabsInWindowFunction::RunImpl() {
  int window_id;
  Browser* browser;
  // windowId defaults to "current" window.
  if (!args_->IsType(Value::TYPE_NULL)) {
    EXTENSION_FUNCTION_VALIDATE(args_->GetAsInteger(&window_id));
    browser = GetBrowserInProfileWithId(profile(), window_id);
  } else {
    browser = dispatcher_->browser();
  }

  if (!browser)
    // TODO(rafaelw): return a "no 'current' browser" error.
    return false;

  result_.reset(CreateTabList(browser));

  return true;
}

bool CreateTabFunction::RunImpl() {
  EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_DICTIONARY));
  const DictionaryValue* args = static_cast<const DictionaryValue*>(args_);

  int window_id;
  Browser* browser;
  // windowId defaults to "current" window.
  if (args->HasKey(kWindowIdKey)) {
    EXTENSION_FUNCTION_VALIDATE(args->GetInteger(kWindowIdKey, &window_id));
    browser = GetBrowserInProfileWithId(profile(), window_id);
  } else {
    browser = dispatcher_->browser();
  }

  if (!browser)
    // TODO(rafaelw): return a "no 'current' browser" error.
    return false;

  TabStripModel* tab_strip = browser->tabstrip_model();

  // TODO(rafaelw): handle setting remaining tab properties:
  // -title
  // -favIconUrl

  std::string url_string;
  scoped_ptr<GURL> url(new GURL());
  if (args->HasKey(kUrlKey)) {
    EXTENSION_FUNCTION_VALIDATE(args->GetString(kUrlKey, &url_string));
    url.reset(new GURL(url_string));
    // TODO(rafaelw): return an "invalid url" error.
    if (!url->is_valid())
      return false;
  }

  // Default to foreground for the new tab. The presence of 'selected' property
  // will override this default.
  bool selected = true;
  if (args->HasKey(kSelectedKey))
    EXTENSION_FUNCTION_VALIDATE(args->GetBoolean(kSelectedKey, &selected));

  // If index is specified, honor the value, but keep it bound to
  // 0 <= index <= tab_strip->count()
  int index = -1;
  if (args->HasKey(kIndexKey))
    EXTENSION_FUNCTION_VALIDATE(args->GetInteger(kIndexKey, &index));

  if (index < 0) {
    // Default insert behavior
    index = -1;
  }
  if (index > tab_strip->count()) {
    index = tab_strip->count();
  }

  TabContents* contents = browser->AddTabWithURL(*(url.get()), GURL(),
      PageTransition::LINK, selected, index, true, NULL);
  index = tab_strip->GetIndexOfTabContents(contents);

  // Return data about the newly created tab.
  if (has_callback())
    result_.reset(ExtensionTabUtil::CreateTabValue(contents, tab_strip, index));

  return true;
}

bool GetTabFunction::RunImpl() {
  int tab_id;
  EXTENSION_FUNCTION_VALIDATE(args_->GetAsInteger(&tab_id));

  TabStripModel* tab_strip = NULL;
  TabContents* contents = NULL;
  int tab_index = -1;
  // TODO(rafaelw): return "tab_id not found" error.
  if (!ExtensionTabUtil::GetTabById(tab_id, profile(), NULL, &tab_strip,
                                    &contents, &tab_index))
    return false;

  result_.reset(ExtensionTabUtil::CreateTabValue(contents, tab_strip,
      tab_index));
  return true;
}

bool UpdateTabFunction::RunImpl() {
  int tab_id;
  EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_LIST));
  const ListValue* args = static_cast<const ListValue*>(args_);
  EXTENSION_FUNCTION_VALIDATE(args->GetInteger(0, &tab_id));
  DictionaryValue* update_props;
  EXTENSION_FUNCTION_VALIDATE(args->GetDictionary(1, &update_props));

  TabStripModel* tab_strip = NULL;
  TabContents* contents = NULL;
  int tab_index = -1;
  // TODO(rafaelw): return "tab_id not found" error.
  if (!ExtensionTabUtil::GetTabById(tab_id, profile(), NULL, &tab_strip,
                                    &contents, &tab_index))
    return false;

  NavigationController& controller = contents->controller();

  // TODO(rafaelw): handle setting remaining tab properties:
  // -title
  // -favIconUrl

  // Navigate the tab to a new location if the url different.
  std::string url;
  if (update_props->HasKey(kUrlKey)) {
    EXTENSION_FUNCTION_VALIDATE(update_props->GetString(kUrlKey, &url));
    GURL new_gurl(url);

    // TODO(rafaelw): return "invalid url" here.
    if (!new_gurl.is_valid())
      return false;

    controller.LoadURL(new_gurl, GURL(), PageTransition::LINK);
  }

  bool selected = false;
  // TODO(rafaelw): Setting |selected| from js doesn't make much sense.
  // Move tab selection management up to window.
  if (update_props->HasKey(kSelectedKey)) {
    EXTENSION_FUNCTION_VALIDATE(update_props->GetBoolean(kSelectedKey,
                                                         &selected));
    if (selected && tab_strip->selected_index() != tab_index) {
      tab_strip->SelectTabContentsAt(tab_index, false);
    }
  }

  return true;
}

bool MoveTabFunction::RunImpl() {
  int tab_id;
  EXTENSION_FUNCTION_VALIDATE(args_->IsType(Value::TYPE_LIST));
  const ListValue* args = static_cast<const ListValue*>(args_);
  EXTENSION_FUNCTION_VALIDATE(args->GetInteger(0, &tab_id));
  DictionaryValue* update_props;
  EXTENSION_FUNCTION_VALIDATE(args->GetDictionary(1, &update_props));

  int new_index;
  EXTENSION_FUNCTION_VALIDATE(update_props->GetInteger(kIndexKey, &new_index));
  EXTENSION_FUNCTION_VALIDATE(new_index >= 0);

  Browser* source_browser = NULL;
  TabStripModel* source_tab_strip = NULL;
  int tab_index = -1;
  // TODO(rafaelw): return "tab_id not found" error.
  if (!ExtensionTabUtil::GetTabById(tab_id, profile(), &source_browser,
                                    &source_tab_strip, NULL,
                                    &tab_index))
    return false;

  if (update_props->HasKey(kWindowIdKey)) {
    int window_id;
    EXTENSION_FUNCTION_VALIDATE(update_props->GetInteger(kWindowIdKey,
        &window_id));
    Browser* target_browser = GetBrowserInProfileWithId(profile(), window_id);
    // TODO(rafaelw): return "window_id not found" error.
    if (!target_browser) 
      return false;

    // If windowId is different from the current window, move between windows.
    if (ExtensionTabUtil::GetWindowId(target_browser) !=
        ExtensionTabUtil::GetWindowId(source_browser)) {
      TabStripModel* target_tab_strip = target_browser->tabstrip_model();
      TabContents *contents = source_tab_strip->DetachTabContentsAt(tab_index);
      // TODO(rafaelw): return a "tab not found" error.
      if (!contents)
        return false;
      
      // Clamp move location to the last position.
      // This is ">" because it can append to a new index position.
      if (new_index > target_tab_strip->count())
        new_index = target_tab_strip->count();

      target_tab_strip->InsertTabContentsAt(new_index, contents,
          false, true);

      return true;
    } 
  }
 
  // Perform a simple within-window move.
  // Clamp move location to the last position.
  // This is ">=" because the move must be to an existing location.
  if (new_index >= source_tab_strip->count())
    new_index = source_tab_strip->count() - 1;
    
  if (new_index != tab_index)
    source_tab_strip->MoveTabContentsAt(tab_index, new_index, false);

  return true;
}


bool RemoveTabFunction::RunImpl() {
  // TODO(rafaelw): This should have a callback, but it can't because it could
  // close it's own tab.
  int tab_id;
  EXTENSION_FUNCTION_VALIDATE(args_->GetAsInteger(&tab_id));

  Browser* browser = NULL;
  TabContents* contents = NULL;
  // TODO(rafaelw): return "tab_id not found" error.
  if (!ExtensionTabUtil::GetTabById(tab_id, profile(), &browser, NULL,
                                    &contents, NULL))
    return false;

  browser->CloseTabContents(contents);
  return true;
}

// static helpers
// if |populate| is true, each window gets a list property |tabs| which contains
// fully populated tab objects.
static DictionaryValue* CreateWindowValue(Browser* browser,
                                          bool populate_tabs) {
  DictionaryValue* result = new DictionaryValue();
  result->SetInteger(kIdKey, ExtensionTabUtil::GetWindowId(browser));
  result->SetBoolean(kFocusedKey, browser->window()->IsActive());
  gfx::Rect bounds = browser->window()->GetNormalBounds();

  // TODO(rafaelw): zIndex ?
  result->SetInteger(kLeftKey, bounds.x());
  result->SetInteger(kTopKey, bounds.y());
  result->SetInteger(kWidthKey, bounds.width());
  result->SetInteger(kHeightKey, bounds.height());

  if (populate_tabs) {
    result->Set(kTabsKey, CreateTabList(browser));
  }

  return result;
}

static ListValue* CreateTabList(Browser* browser) {
  ListValue* tab_list = new ListValue();
  TabStripModel* tab_strip = browser->tabstrip_model();
  for (int i = 0; i < tab_strip->count(); ++i) {
    tab_list->Append(ExtensionTabUtil::CreateTabValue(
        tab_strip->GetTabContentsAt(i), tab_strip, i));
  }

  return tab_list;
}

static Browser* GetBrowserInProfileWithId(Profile* profile,
                                          const int window_id) {
  for (BrowserList::const_iterator browser = BrowserList::begin();
      browser != BrowserList::end(); ++browser) {
    if ((*browser)->profile() == profile &&
        ExtensionTabUtil::GetWindowId(*browser) == window_id)
      return *browser;
  }
  return NULL;
}
