// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_TRAY_BUBBLE_VIEW_H_
#define ASH_SYSTEM_TRAY_TRAY_BUBBLE_VIEW_H_

#include "ash/ash_export.h"
#include "ui/views/bubble/bubble_delegate.h"

// Specialized bubble view for bubbles associated with a tray icon (e.g. the
// Ash status area). Mostly this handles custom anchor location and arrow and
// border rendering. This also has its own delegate for handling mouse events
// and other implementation specific details.

namespace ui {
class LocatedEvent;
}

namespace views {
class View;
class Widget;
}

// TODO(stevenjb): Move this out of message_center namespace once in views.
namespace message_center {

namespace internal {
class TrayBubbleBorder;
class TrayBubbleBackground;
}

class ASH_EXPORT TrayBubbleView : public views::BubbleDelegateView {
 public:
  enum AnchorType {
    ANCHOR_TYPE_TRAY,
    ANCHOR_TYPE_BUBBLE
  };

  enum AnchorAlignment {
    ANCHOR_ALIGNMENT_BOTTOM,
    ANCHOR_ALIGNMENT_LEFT,
    ANCHOR_ALIGNMENT_RIGHT
  };

  class ASH_EXPORT Delegate {
   public:
    typedef TrayBubbleView::AnchorType AnchorType;
    typedef TrayBubbleView::AnchorAlignment AnchorAlignment;

    Delegate() {}
    virtual ~Delegate() {}

    // Called when the view is destroyed. Any pointers to the view should be
    // cleared when this gets called.
    virtual void BubbleViewDestroyed() = 0;

    // Called when the mouse enters/exits the view.
    virtual void OnMouseEnteredView() = 0;
    virtual void OnMouseExitedView() = 0;

    // Called from GetAccessibleState(); should return the appropriate
    // accessible name for the bubble.
    virtual string16 GetAccessibleNameForBubble() = 0;

    // Passes responsibility for BubbleDelegateView::GetAnchorRect to the
    // delegate.
    virtual gfx::Rect GetAnchorRect(views::Widget* anchor_widget,
                                    AnchorType anchor_type,
                                    AnchorAlignment anchor_alignment) = 0;

    // Called when a bubble wants to hide/destroy itself (e.g. last visible
    // child view was closed).
    virtual void HideBubble(const TrayBubbleView* bubble_view) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  struct InitParams {
    static const int kArrowDefaultOffset;

    InitParams(AnchorType anchor_type,
               AnchorAlignment anchor_alignment,
               int bubble_width);
    AnchorType anchor_type;
    AnchorAlignment anchor_alignment;
    int bubble_width;
    int max_height;
    bool can_activate;
    bool close_on_deactivate;
    SkColor top_color;
    SkColor arrow_color;
    views::BubbleBorder::ArrowLocation arrow_location;
    int arrow_offset;
    views::BubbleBorder::Shadow shadow;
  };

  // Constructs and returns a TrayBubbleView. init_params may be modified.
  static TrayBubbleView* Create(aura::Window* parent_window,
                                views::View* anchor,
                                Delegate* delegate,
                                InitParams* init_params);

  virtual ~TrayBubbleView();

  // Sets up animations, and show the bubble. Must occur after CreateBubble()
  // is called.
  void InitializeAndShowBubble();

  // Called whenever the bubble size or location may have changed.
  void UpdateBubble();

  // Sets the maximum bubble height and resizes the bubble.
  void SetMaxHeight(int height);

  // Returns the border insets. Called by TrayEventFilter.
  void GetBorderInsets(gfx::Insets* insets) const;

  // Called when the delegate is destroyed.
  void reset_delegate() { delegate_ = NULL; }

  Delegate* delegate() { return delegate_; }

  void set_gesture_dragging(bool dragging) { is_gesture_dragging_ = dragging; }
  bool is_gesture_dragging() const { return is_gesture_dragging_; }

  // Overridden from views::WidgetDelegate.
  virtual bool CanActivate() const OVERRIDE;
  virtual views::NonClientFrameView* CreateNonClientFrameView(
      views::Widget* widget) OVERRIDE;
  virtual bool WidgetHasHitTestMask() const OVERRIDE;
  virtual void GetWidgetHitTestMask(gfx::Path* mask) const OVERRIDE;

  // Overridden from views::BubbleDelegateView.
  virtual gfx::Rect GetAnchorRect() OVERRIDE;

  // Overridden from views::View.
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual void OnMouseEntered(const ui::MouseEvent& event) OVERRIDE;
  virtual void OnMouseExited(const ui::MouseEvent& event) OVERRIDE;
  virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;

 protected:
  TrayBubbleView(aura::Window* parent_window,
                 views::View* anchor,
                 Delegate* delegate,
                 const InitParams& init_params);

  // Overridden from views::BubbleDelegateView.
  virtual void Init() OVERRIDE;

  // Overridden from views::View.
  virtual void ChildPreferredSizeChanged(View* child) OVERRIDE;
  virtual void ViewHierarchyChanged(bool is_add,
                                    views::View* parent,
                                    views::View* child) OVERRIDE;

 private:
  InitParams params_;
  Delegate* delegate_;
  internal::TrayBubbleBorder* bubble_border_;
  internal::TrayBubbleBackground* bubble_background_;
  bool is_gesture_dragging_;

  DISALLOW_COPY_AND_ASSIGN(TrayBubbleView);
};

}  // namespace message_center

#endif  // ASH_SYSTEM_TRAY_TRAY_BUBBLE_VIEW_H_
