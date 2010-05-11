// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gtk/tabs/tab_gtk.h"

#include <gdk/gdkkeysyms.h>

#include "app/gtk_dnd_util.h"
#include "app/l10n_util.h"
#include "app/menus/accelerator_gtk.h"
#include "app/resource_bundle.h"
#include "base/keyboard_codes_posix.h"
#include "base/singleton.h"
#include "base/utf_string_conversions.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/browser/gtk/accelerators_gtk.h"
#include "chrome/browser/gtk/menu_gtk.h"
#include "chrome/browser/tab_menu_model.h"
#include "gfx/path.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"

namespace {

// Returns the width of the title for the current font, in pixels.
int GetTitleWidth(gfx::Font* font, std::wstring title) {
  DCHECK(font);
  if (title.empty())
    return 0;

  return font->GetStringWidth(title);
}

}  // namespace

class TabGtk::ContextMenuController : public menus::SimpleMenuModel::Delegate {
 public:
  explicit ContextMenuController(TabGtk* tab)
      : tab_(tab),
        model_(this, tab->delegate()->IsTabPinned(tab)) {
    menu_.reset(new MenuGtk(NULL, &model_));
  }

  virtual ~ContextMenuController() {}

  void RunMenu() {
    menu_->PopupAsContext(gtk_get_current_event_time());
  }

  void Cancel() {
    tab_ = NULL;
    menu_->Cancel();
  }

 private:
  // Overridden from menus::SimpleMenuModel::Delegate:
  virtual bool IsCommandIdChecked(int command_id) const {
    return false;
  }
  virtual bool IsCommandIdEnabled(int command_id) const {
    return tab_ && tab_->delegate()->IsCommandEnabledForTab(
        static_cast<TabStripModel::ContextMenuCommand>(command_id),
        tab_);
  }
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      menus::Accelerator* accelerator) {
    int browser_command = 0;
    switch (command_id) {
      case TabStripModel::CommandNewTab:
        browser_command = IDC_NEW_TAB;
        break;
      case TabStripModel::CommandReload:
        browser_command = IDC_RELOAD;
        break;
      case TabStripModel::CommandCloseTab:
        browser_command = IDC_CLOSE_TAB;
        break;
      case TabStripModel::CommandRestoreTab:
        browser_command = IDC_RESTORE_TAB;
        break;
      case TabStripModel::CommandBookmarkAllTabs:
        browser_command = IDC_BOOKMARK_ALL_TABS;
        break;
      default:
        return false;
    }

    const menus::AcceleratorGtk* accelerator_gtk =
        Singleton<AcceleratorsGtk>()->GetPrimaryAcceleratorForCommand(
            browser_command);
    if (accelerator_gtk)
      *accelerator = *accelerator_gtk;
    return !!accelerator_gtk;
  }
  virtual void ExecuteCommand(int command_id) {
    if (!tab_)
      return;
    tab_->delegate()->ExecuteCommandForTab(
        static_cast<TabStripModel::ContextMenuCommand>(command_id), tab_);
  }

  // The context menu.
  scoped_ptr<MenuGtk> menu_;

  // The Tab the context menu was brought up for. Set to NULL when the menu
  // is canceled.
  TabGtk* tab_;

  // The model.
  TabMenuModel model_;

  DISALLOW_COPY_AND_ASSIGN(ContextMenuController);
};

class TabGtk::TabGtkObserverHelper {
 public:
  explicit TabGtkObserverHelper(TabGtk* tab)
      : tab_(tab) {
    MessageLoopForUI::current()->AddObserver(tab_);
  }

  ~TabGtkObserverHelper() {
    MessageLoopForUI::current()->RemoveObserver(tab_);
  }

 private:
  TabGtk* tab_;

  DISALLOW_COPY_AND_ASSIGN(TabGtkObserverHelper);
};

///////////////////////////////////////////////////////////////////////////////
// TabGtk, public:

TabGtk::TabGtk(TabDelegate* delegate)
    : TabRendererGtk(delegate->GetThemeProvider()),
      delegate_(delegate),
      closing_(false),
      last_mouse_down_(NULL),
      drag_widget_(NULL),
      title_width_(0),
      ALLOW_THIS_IN_INITIALIZER_LIST(destroy_factory_(this)),
      ALLOW_THIS_IN_INITIALIZER_LIST(drag_end_factory_(this)) {
  event_box_ = gtk_event_box_new();
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(event_box_), FALSE);
  g_signal_connect(event_box_, "button-press-event",
                   G_CALLBACK(OnButtonPressEvent), this);
  g_signal_connect(event_box_, "button-release-event",
                   G_CALLBACK(OnButtonReleaseEvent), this);
  g_signal_connect(event_box_, "enter-notify-event",
                   G_CALLBACK(OnEnterNotifyEvent), this);
  g_signal_connect(event_box_, "leave-notify-event",
                   G_CALLBACK(OnLeaveNotifyEvent), this);
  gtk_widget_add_events(event_box_,
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
        GDK_LEAVE_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
  gtk_container_add(GTK_CONTAINER(event_box_), TabRendererGtk::widget());
  gtk_widget_show_all(event_box_);
}

TabGtk::~TabGtk() {
  if (drag_widget_) {
    // Shadow the drag grab so the grab terminates. We could do this using any
    // widget, |drag_widget_| is just convenient.
    gtk_grab_add(drag_widget_);
    gtk_grab_remove(drag_widget_);
    DestroyDragWidget();
  }

  if (menu_controller_.get()) {
    // The menu is showing. Close the menu.
    menu_controller_->Cancel();

    // Invoke this so that we hide the highlight.
    ContextMenuClosed();
  }
}

// static
gboolean TabGtk::OnButtonPressEvent(GtkWidget* widget, GdkEventButton* event,
                                    TabGtk* tab) {
  // Every button press ensures either a button-release-event or a drag-fail
  // signal for |widget|.
  if (event->button == 1 && event->type == GDK_BUTTON_PRESS) {
    // Store whether or not we were selected just now... we only want to be
    // able to drag foreground tabs, so we don't start dragging the tab if
    // it was in the background.
    bool just_selected = !tab->IsSelected();
    if (just_selected) {
      tab->delegate_->SelectTab(tab);
    }

    // Hook into the message loop to handle dragging.
    tab->observer_.reset(new TabGtkObserverHelper(tab));

    // Store the button press event, used to initiate a drag.
    tab->last_mouse_down_ = gdk_event_copy(reinterpret_cast<GdkEvent*>(event));
  } else if (event->button == 3) {
    // Only show the context menu if the left mouse button isn't down (i.e.,
    // the user might want to drag instead).
    if (!tab->last_mouse_down_)
      tab->ShowContextMenu();
  }

  return TRUE;
}

// static
gboolean TabGtk::OnButtonReleaseEvent(GtkWidget* widget, GdkEventButton* event,
                                      TabGtk* tab) {
  if (event->button == 1) {
    tab->observer_.reset();

    if (tab->last_mouse_down_) {
      gdk_event_free(tab->last_mouse_down_);
      tab->last_mouse_down_ = NULL;
    }
  }

  // Middle mouse up means close the tab, but only if the mouse is over it
  // (like a button).
  if (event->button == 2 &&
      event->x >= 0 && event->y >= 0 &&
      event->x < widget->allocation.width &&
      event->y < widget->allocation.height) {
    // If the user is currently holding the left mouse button down but hasn't
    // moved the mouse yet, a drag hasn't started yet.  In that case, clean up
    // some state before closing the tab to avoid a crash.  Once the drag has
    // started, we don't get the middle mouse click here.
    if (tab->last_mouse_down_) {
      DCHECK(!tab->drag_widget_);
      tab->observer_.reset();
      gdk_event_free(tab->last_mouse_down_);
      tab->last_mouse_down_ = NULL;
    }
    tab->delegate_->CloseTab(tab);
  }

  return TRUE;
}

// static
gboolean TabGtk::OnDragFailed(GtkWidget* widget, GdkDragContext* context,
                              GtkDragResult result,
                              TabGtk* tab) {
  bool canceled = (result == GTK_DRAG_RESULT_USER_CANCELLED);
  tab->EndDrag(canceled);
  return TRUE;
}

// static
gboolean TabGtk::OnDragButtonReleased(GtkWidget* widget, GdkEventButton* button,
                                      TabGtk* tab) {
  // We always get this event when gtk is releasing the grab and ending the
  // drag.  However, if the user ended the drag with space or enter, we don't
  // get a follow up event to tell us the drag has finished (either a
  // drag-failed or a drag-end).  So we post a task to manually end the drag.
  // If GTK+ does send the drag-failed or drag-end event, we cancel the task.
  MessageLoop::current()->PostTask(FROM_HERE,
      tab->drag_end_factory_.NewRunnableMethod(&TabGtk::EndDrag, false));
  return TRUE;
}

// static
void TabGtk::OnDragBegin(GtkWidget* widget, GdkDragContext* context,
                         TabGtk* tab) {
  GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);
  gtk_drag_set_icon_pixbuf(context, pixbuf, 0, 0);
  g_object_unref(pixbuf);
}

///////////////////////////////////////////////////////////////////////////////
// TabGtk, MessageLoop::Observer implementation:

void TabGtk::WillProcessEvent(GdkEvent* event) {
  // Nothing to do.
}

void TabGtk::DidProcessEvent(GdkEvent* event) {
  if (!(event->type == GDK_MOTION_NOTIFY || event->type == GDK_LEAVE_NOTIFY ||
        event->type == GDK_ENTER_NOTIFY)) {
    return;
  }

  if (drag_widget_) {
    delegate_->ContinueDrag(NULL);
    return;
  }

  gint old_x = static_cast<gint>(last_mouse_down_->button.x_root);
  gint old_y = static_cast<gint>(last_mouse_down_->button.y_root);
  gdouble new_x;
  gdouble new_y;
  gdk_event_get_root_coords(event, &new_x, &new_y);

  if (gtk_drag_check_threshold(widget(), old_x, old_y,
      static_cast<gint>(new_x), static_cast<gint>(new_y))) {
    StartDragging(gfx::Point(
        static_cast<int>(last_mouse_down_->button.x),
        static_cast<int>(last_mouse_down_->button.y)));
  }
}

///////////////////////////////////////////////////////////////////////////////
// TabGtk, TabRendererGtk overrides:

bool TabGtk::IsSelected() const {
  return delegate_->IsTabSelected(this);
}

bool TabGtk::IsVisible() const {
  return GTK_WIDGET_FLAGS(event_box_) & GTK_VISIBLE;
}

void TabGtk::SetVisible(bool visible) const {
  if (visible) {
    gtk_widget_show(event_box_);
  } else {
    gtk_widget_hide(event_box_);
  }
}

void TabGtk::CloseButtonClicked() {
  delegate_->CloseTab(this);
}

void TabGtk::UpdateData(TabContents* contents,
                        bool phantom,
                        bool loading_only) {
  TabRendererGtk::UpdateData(contents, phantom, loading_only);
  // Cache the title width so we don't recalculate it every time the tab is
  // resized.
  title_width_ = GetTitleWidth(title_font(), GetTitle());
  UpdateTooltipState();
}

void TabGtk::SetBounds(const gfx::Rect& bounds) {
  TabRendererGtk::SetBounds(bounds);
  UpdateTooltipState();
}

///////////////////////////////////////////////////////////////////////////////
// TabGtk, private:

void TabGtk::ShowContextMenu() {
  menu_controller_.reset(new ContextMenuController(this));

  menu_controller_->RunMenu();
}

void TabGtk::ContextMenuClosed() {
  delegate()->StopAllHighlighting();
  menu_controller_.reset();
}

void TabGtk::UpdateTooltipState() {
  // Only show the tooltip if the title is truncated.
  if (title_width_ > title_bounds().width()) {
    gtk_widget_set_tooltip_text(widget(), WideToUTF8(GetTitle()).c_str());
  } else {
    gtk_widget_set_has_tooltip(widget(), FALSE);
  }
}

void TabGtk::CreateDragWidget() {
  DCHECK(!drag_widget_);
  drag_widget_ = gtk_invisible_new();
  g_signal_connect(drag_widget_, "drag-failed",
                   G_CALLBACK(OnDragFailed), this);
  g_signal_connect(drag_widget_, "button-release-event",
                   G_CALLBACK(OnDragButtonReleased), this);
  g_signal_connect_after(drag_widget_, "drag-begin",
                         G_CALLBACK(OnDragBegin), this);
}

void TabGtk::DestroyDragWidget() {
  if (drag_widget_) {
    gtk_widget_destroy(drag_widget_);
    drag_widget_ = NULL;
  }
}

void TabGtk::StartDragging(gfx::Point drag_offset) {
  CreateDragWidget();

  GtkTargetList* list = gtk_dnd_util::GetTargetListFromCodeMask(
      gtk_dnd_util::CHROME_TAB);
  gtk_drag_begin(drag_widget_, list, GDK_ACTION_MOVE,
                 1,  // Drags are always initiated by the left button.
                 last_mouse_down_);

  delegate_->MaybeStartDrag(this, drag_offset);
}

void TabGtk::EndDrag(bool canceled) {
  // Make sure we only run EndDrag once by canceling any tasks that want
  // to call EndDrag.
  drag_end_factory_.RevokeAll();

  // We must let gtk clean up after we handle the drag operation, otherwise
  // there will be outstanding references to the drag widget when we try to
  // destroy it.
  MessageLoop::current()->PostTask(FROM_HERE,
      destroy_factory_.NewRunnableMethod(&TabGtk::DestroyDragWidget));

  if (last_mouse_down_) {
    gdk_event_free(last_mouse_down_);
    last_mouse_down_ = NULL;
  }

  // Notify the drag helper that we're done with any potential drag operations.
  // Clean up the drag helper, which is re-created on the next mouse press.
  delegate_->EndDrag(canceled);

  observer_.reset();
}
