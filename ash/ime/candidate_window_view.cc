// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/ime/candidate_window_view.h"

#include <string>

#include "ash/ime/candidate_view.h"
#include "ash/ime/candidate_window_constants.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/screen.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/corewm/window_animations.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"

namespace ash {
namespace ime {

namespace {

class CandidateWindowBorder : public views::BubbleBorder {
 public:
  explicit CandidateWindowBorder(gfx::NativeView parent)
      : views::BubbleBorder(views::BubbleBorder::TOP_CENTER,
                            views::BubbleBorder::NO_SHADOW,
                            SK_ColorTRANSPARENT),
        parent_(parent),
        offset_(0) {
    set_paint_arrow(views::BubbleBorder::PAINT_NONE);
  }
  virtual ~CandidateWindowBorder() {}

  void set_offset(int offset) { offset_ = offset; }

 private:
  // Overridden from views::BubbleBorder:
  virtual gfx::Rect GetBounds(const gfx::Rect& anchor_rect,
                              const gfx::Size& content_size) const OVERRIDE {
    gfx::Rect bounds(content_size);
    bounds.set_origin(gfx::Point(
        anchor_rect.x() - offset_,
        is_arrow_on_top(arrow()) ?
        anchor_rect.bottom() : anchor_rect.y() - content_size.height()));

    // It cannot use the normal logic of arrow offset for horizontal offscreen,
    // because the arrow must be in the content's edge. But CandidateWindow has
    // to be visible even when |anchor_rect| is out of the screen.
    gfx::Rect work_area = gfx::Screen::GetNativeScreen()->
        GetDisplayNearestWindow(parent_).work_area();
    if (bounds.right() > work_area.right())
      bounds.set_x(work_area.right() - bounds.width());
    if (bounds.x() < work_area.x())
      bounds.set_x(work_area.x());

    return bounds;
  }

  virtual gfx::Insets GetInsets() const OVERRIDE {
    return gfx::Insets();
  }

  gfx::NativeView parent_;
  int offset_;

  DISALLOW_COPY_AND_ASSIGN(CandidateWindowBorder);
};

// Computes the page index. For instance, if the page size is 9, and the
// cursor is pointing to 13th candidate, the page index will be 1 (2nd
// page, as the index is zero-origin). Returns -1 on error.
int ComputePageIndex(const ui::CandidateWindow& candidate_window) {
  if (candidate_window.page_size() > 0)
    return candidate_window.cursor_position() / candidate_window.page_size();
  return -1;
}

}  // namespace

class InformationTextArea : public views::View {
 public:
  // InformationTextArea's border is drawn as a separator, it should appear
  // at either top or bottom.
  enum BorderPosition {
    TOP,
    BOTTOM
  };

  // Specify the alignment and initialize the control.
  InformationTextArea(gfx::HorizontalAlignment align, int min_width)
      : min_width_(min_width) {
    label_ = new views::Label;
    label_->SetHorizontalAlignment(align);
    label_->SetBorder(views::Border::CreateEmptyBorder(2, 2, 2, 4));

    SetLayoutManager(new views::FillLayout());
    AddChildView(label_);
    set_background(views::Background::CreateSolidBackground(
        color_utils::AlphaBlend(SK_ColorBLACK,
                                GetNativeTheme()->GetSystemColor(
                                    ui::NativeTheme::kColorId_WindowBackground),
                                0x10)));
  }

  // Sets the text alignment.
  void SetAlignment(gfx::HorizontalAlignment alignment) {
    label_->SetHorizontalAlignment(alignment);
  }

  // Sets the displayed text.
  void SetText(const base::string16& text) {
    label_->SetText(text);
  }

  // Sets the border thickness for top/bottom.
  void SetBorderFromPosition(BorderPosition position) {
    SetBorder(views::Border::CreateSolidSidedBorder(
        (position == TOP) ? 1 : 0,
        0,
        (position == BOTTOM) ? 1 : 0,
        0,
        GetNativeTheme()->GetSystemColor(
            ui::NativeTheme::kColorId_MenuBorderColor)));
  }

 protected:
  virtual gfx::Size GetPreferredSize() OVERRIDE {
    gfx::Size size = views::View::GetPreferredSize();
    size.SetToMax(gfx::Size(min_width_, 0));
    return size;
  }

 private:
  views::Label* label_;
  int min_width_;

  DISALLOW_COPY_AND_ASSIGN(InformationTextArea);
};

CandidateWindowView::CandidateWindowView(gfx::NativeView parent)
    : selected_candidate_index_in_page_(-1),
      should_show_at_composition_head_(false),
      should_show_upper_side_(false),
      was_candidate_window_open_(false) {
  set_parent_window(parent);
  set_margins(gfx::Insets());

  // Set the background and the border of the view.
  ui::NativeTheme* theme = GetNativeTheme();
  set_background(
      views::Background::CreateSolidBackground(theme->GetSystemColor(
          ui::NativeTheme::kColorId_WindowBackground)));
  SetBorder(views::Border::CreateSolidBorder(
      1, theme->GetSystemColor(ui::NativeTheme::kColorId_MenuBorderColor)));

  SetLayoutManager(new views::BoxLayout(views::BoxLayout::kVertical, 0, 0, 0));
  auxiliary_text_ = new InformationTextArea(gfx::ALIGN_RIGHT, 0);
  preedit_ = new InformationTextArea(gfx::ALIGN_LEFT, kMinPreeditAreaWidth);
  candidate_area_ = new views::View;
  auxiliary_text_->SetVisible(false);
  preedit_->SetVisible(false);
  candidate_area_->SetVisible(false);
  preedit_->SetBorderFromPosition(InformationTextArea::BOTTOM);
  if (candidate_window_.orientation() == ui::CandidateWindow::VERTICAL) {
    AddChildView(preedit_);
    AddChildView(candidate_area_);
    AddChildView(auxiliary_text_);
    auxiliary_text_->SetBorderFromPosition(InformationTextArea::TOP);
    candidate_area_->SetLayoutManager(new views::BoxLayout(
        views::BoxLayout::kVertical, 0, 0, 0));
  } else {
    AddChildView(preedit_);
    AddChildView(auxiliary_text_);
    AddChildView(candidate_area_);
    auxiliary_text_->SetAlignment(gfx::ALIGN_LEFT);
    auxiliary_text_->SetBorderFromPosition(InformationTextArea::BOTTOM);
    candidate_area_->SetLayoutManager(new views::BoxLayout(
        views::BoxLayout::kHorizontal, 0, 0, 0));
  }
}

CandidateWindowView::~CandidateWindowView() {
}

views::Widget* CandidateWindowView::InitWidget() {
  views::Widget* widget = BubbleDelegateView::CreateBubble(this);

  views::corewm::SetWindowVisibilityAnimationType(
      widget->GetNativeView(),
      views::corewm::WINDOW_VISIBILITY_ANIMATION_TYPE_FADE);

  GetBubbleFrameView()->SetBubbleBorder(scoped_ptr<views::BubbleBorder>(
      new CandidateWindowBorder(parent_window())));
  return widget;
}

void CandidateWindowView::UpdateVisibility() {
  if (candidate_area_->visible() || auxiliary_text_->visible() ||
      preedit_->visible()) {
    SizeToContents();
  } else {
    GetWidget()->Close();
  }
}

void CandidateWindowView::HideLookupTable() {
  candidate_area_->SetVisible(false);
  auxiliary_text_->SetVisible(false);
  UpdateVisibility();
}

void CandidateWindowView::HidePreeditText() {
  preedit_->SetVisible(false);
  UpdateVisibility();
}

void CandidateWindowView::ShowPreeditText() {
  preedit_->SetVisible(true);
  UpdateVisibility();
}

void CandidateWindowView::UpdatePreeditText(const base::string16& text) {
  preedit_->SetText(text);
}

void CandidateWindowView::ShowLookupTable() {
  candidate_area_->SetVisible(true);
  auxiliary_text_->SetVisible(candidate_window_.is_auxiliary_text_visible());
  UpdateVisibility();
}

void CandidateWindowView::UpdateCandidates(
    const ui::CandidateWindow& new_candidate_window) {
  // Updating the candidate views is expensive. We'll skip this if possible.
  if (!candidate_window_.IsEqual(new_candidate_window)) {
    if (candidate_window_.orientation() != new_candidate_window.orientation()) {
      // If the new layout is vertical, the aux text should appear at the
      // bottom. If horizontal, it should appear between preedit and candidates.
      if (new_candidate_window.orientation() == ui::CandidateWindow::VERTICAL) {
        ReorderChildView(auxiliary_text_, -1);
        auxiliary_text_->SetAlignment(gfx::ALIGN_RIGHT);
        auxiliary_text_->SetBorderFromPosition(InformationTextArea::TOP);
        candidate_area_->SetLayoutManager(new views::BoxLayout(
            views::BoxLayout::kVertical, 0, 0, 0));
      } else {
        ReorderChildView(auxiliary_text_, 1);
        auxiliary_text_->SetAlignment(gfx::ALIGN_LEFT);
        auxiliary_text_->SetBorderFromPosition(InformationTextArea::BOTTOM);
        candidate_area_->SetLayoutManager(new views::BoxLayout(
            views::BoxLayout::kHorizontal, 0, 0, 0));
      }
    }

    // Initialize candidate views if necessary.
    MaybeInitializeCandidateViews(new_candidate_window);

    should_show_at_composition_head_
        = new_candidate_window.show_window_at_composition();
    // Compute the index of the current page.
    const int current_page_index = ComputePageIndex(new_candidate_window);
    if (current_page_index < 0)
      return;

    // Update the candidates in the current page.
    const size_t start_from =
        current_page_index * new_candidate_window.page_size();

    int max_shortcut_width = 0;
    int max_candidate_width = 0;
    for (size_t i = 0; i < candidate_views_.size(); ++i) {
      const size_t index_in_page = i;
      const size_t candidate_index = start_from + index_in_page;
      CandidateView* candidate_view = candidate_views_[index_in_page];
      // Set the candidate text.
      if (candidate_index < new_candidate_window.candidates().size()) {
        const ui::CandidateWindow::Entry& entry =
            new_candidate_window.candidates()[candidate_index];
        candidate_view->SetEntry(entry);
        candidate_view->SetState(views::Button::STATE_NORMAL);
        candidate_view->SetInfolistIcon(!entry.description_title.empty());
      } else {
        // Disable the empty row.
        candidate_view->SetEntry(ui::CandidateWindow::Entry());
        candidate_view->SetState(views::Button::STATE_DISABLED);
        candidate_view->SetInfolistIcon(false);
      }
      if (new_candidate_window.orientation() == ui::CandidateWindow::VERTICAL) {
        int shortcut_width = 0;
        int candidate_width = 0;
        candidate_views_[i]->GetPreferredWidths(
            &shortcut_width, &candidate_width);
        max_shortcut_width = std::max(max_shortcut_width, shortcut_width);
        max_candidate_width = std::max(max_candidate_width, candidate_width);
      }
    }
    if (new_candidate_window.orientation() == ui::CandidateWindow::VERTICAL) {
      for (size_t i = 0; i < candidate_views_.size(); ++i)
        candidate_views_[i]->SetWidths(max_shortcut_width, max_candidate_width);
    }

    CandidateWindowBorder* border = static_cast<CandidateWindowBorder*>(
        GetBubbleFrameView()->bubble_border());
    if (new_candidate_window.orientation() == ui::CandidateWindow::VERTICAL)
      border->set_offset(max_shortcut_width);
    else
      border->set_offset(0);
  }
  // Update the current candidate window. We'll use candidate_window_ from here.
  // Note that SelectCandidateAt() uses candidate_window_.
  candidate_window_.CopyFrom(new_candidate_window);

  // Select the current candidate in the page.
  if (candidate_window_.is_cursor_visible()) {
    if (candidate_window_.page_size()) {
      const int current_candidate_in_page =
          candidate_window_.cursor_position() % candidate_window_.page_size();
      SelectCandidateAt(current_candidate_in_page);
    }
  } else {
    // Unselect the currently selected candidate.
    if (0 <= selected_candidate_index_in_page_ &&
        static_cast<size_t>(selected_candidate_index_in_page_) <
        candidate_views_.size()) {
      candidate_views_[selected_candidate_index_in_page_]->SetState(
          views::Button::STATE_NORMAL);
      selected_candidate_index_in_page_ = -1;
    }
  }

  // Updates auxiliary text
  auxiliary_text_->SetVisible(candidate_window_.is_auxiliary_text_visible());
  auxiliary_text_->SetText(base::UTF8ToUTF16(
      candidate_window_.auxiliary_text()));
}

void CandidateWindowView::SetCursorBounds(const gfx::Rect& cursor_bounds,
                                          const gfx::Rect& composition_head) {
  if (candidate_window_.show_window_at_composition())
    SetAnchorRect(composition_head);
  else
    SetAnchorRect(cursor_bounds);
}

void CandidateWindowView::MaybeInitializeCandidateViews(
    const ui::CandidateWindow& candidate_window) {
  const ui::CandidateWindow::Orientation orientation =
      candidate_window.orientation();
  const size_t page_size = candidate_window.page_size();

  // Reset all candidate_views_ when orientation changes.
  if (orientation != candidate_window_.orientation())
    STLDeleteElements(&candidate_views_);

  while (page_size < candidate_views_.size()) {
    delete candidate_views_.back();
    candidate_views_.pop_back();
  }
  while (page_size > candidate_views_.size()) {
    CandidateView* new_candidate = new CandidateView(this, orientation);
    candidate_area_->AddChildView(new_candidate);
    candidate_views_.push_back(new_candidate);
  }
}

void CandidateWindowView::SelectCandidateAt(int index_in_page) {
  const int current_page_index = ComputePageIndex(candidate_window_);
  if (current_page_index < 0) {
    return;
  }

  const int cursor_absolute_index =
      candidate_window_.page_size() * current_page_index + index_in_page;
  // Ignore click on out of range views.
  if (cursor_absolute_index < 0 ||
      candidate_window_.candidates().size() <=
      static_cast<size_t>(cursor_absolute_index)) {
    return;
  }

  // Remember the currently selected candidate index in the current page.
  selected_candidate_index_in_page_ = index_in_page;

  // Select the candidate specified by index_in_page.
  candidate_views_[index_in_page]->SetState(views::Button::STATE_PRESSED);

  // Update the cursor indexes in the model.
  candidate_window_.set_cursor_position(cursor_absolute_index);
}

void CandidateWindowView::ButtonPressed(views::Button* sender,
                                        const ui::Event& event) {
  for (size_t i = 0; i < candidate_views_.size(); ++i) {
    if (sender == candidate_views_[i]) {
      FOR_EACH_OBSERVER(Observer, observers_, OnCandidateCommitted(i));
      return;
    }
  }
}

}  // namespace ime
}  // namespace ash
