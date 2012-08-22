// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_ZOOM_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_ZOOM_VIEW_H_

#include "base/basictypes.h"
#include "chrome/browser/ui/toolbar/toolbar_model.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "ui/views/controls/image_view.h"

namespace views {
class KeyEvent;
class MouseEvent;
}

class ZoomController;

// View for the zoom icon in the Omnibox.
class ZoomView : public views::ImageView {
 public:
  // Constructor for ZoomView. Clicking on the ZoomView shows a ZoomBubbleView,
  // which requires the current TabContents. Because the current TabContents
  // changes as the user switches tabs, it cannot be provided in the
  // constructor.  Instead, a LocationBarView::Delegate is passed here so that
  // it can be queried for the current TabContents as needed.
  ZoomView(ToolbarModel* toolbar_model,
           LocationBarView::Delegate* location_bar_delegate);
  virtual ~ZoomView();

  // Updates the image and its tooltip appropriately, hiding or showing the icon
  // as needed.
  void Update(ZoomController* zoom_controller);

 private:
  // views::ImageView:
  virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;
  virtual bool GetTooltipText(const gfx::Point& p,
                              string16* tooltip) const OVERRIDE;
  virtual bool OnMousePressed(const ui::MouseEvent& event) OVERRIDE;
  virtual void OnMouseReleased(const ui::MouseEvent& event) OVERRIDE;
  virtual bool OnKeyPressed(const ui::KeyEvent& event) OVERRIDE;

  // Toolbar model used to test whether location bar input is in progress.
  ToolbarModel* toolbar_model_;

  // The delegate used to get the currently visible TabContents.
  LocationBarView::Delegate* location_bar_delegate_;

  DISALLOW_COPY_AND_ASSIGN(ZoomView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_ZOOM_VIEW_H_
