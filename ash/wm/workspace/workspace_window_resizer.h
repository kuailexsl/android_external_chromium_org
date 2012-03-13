// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WORKSPACE_WINDOW_RESIZER_H_
#define ASH_WM_WORKSPACE_WINDOW_RESIZER_H_
#pragma once

#include <vector>

#include "ash/wm/window_resizer.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"

namespace ash {
namespace internal {

class PhantomWindowController;
class RootWindowEventFilter;

// WindowResizer implementation for workspaces. This enforces that windows are
// not allowed to vertically move or resize outside of the work area. As windows
// are moved outside the work area they are shrunk. We remember the height of
// the window before it was moved so that if the window is again moved up we
// attempt to restore the old height.
class ASH_EXPORT WorkspaceWindowResizer : public WindowResizer {
 public:
  // Used when the window is dragged against the edge of the screen.
  enum EdgeType {
    LEFT_EDGE,
    RIGHT_EDGE
  };

  // When dragging an attached window this is the min size we'll make sure is
  // visibile. In the vertical direction we take the max of this and that from
  // the delegate.
  static const int kMinOnscreenSize;

  virtual ~WorkspaceWindowResizer();

  // Returns the bounds for a window along the specified edge.
  static gfx::Rect GetBoundsForWindowAlongEdge(
      aura::Window* window,
      EdgeType edge,
      int grid_size);

  static WorkspaceWindowResizer* Create(
      aura::Window* window,
      const gfx::Point& location,
      int window_component,
      int grid_size,
      const std::vector<aura::Window*>& attached_windows);

  // Returns true if the drag will result in changing the window in anyway.
  bool is_resizable() const { return details_.is_resizable; }

  const gfx::Point& initial_location_in_parent() const {
    return details_.initial_location_in_parent;
  }

  // Overridden from WindowResizer:
  virtual void Drag(const gfx::Point& location) OVERRIDE;
  virtual void CompleteDrag() OVERRIDE;
  virtual void RevertDrag() OVERRIDE;

 private:
  WorkspaceWindowResizer(const Details& details,
                         const std::vector<aura::Window*>& attached_windows);

 private:
  // Location of the phanton window.
  enum PhantomType {
    TYPE_LEFT_EDGE,
    TYPE_RIGHT_EDGE,
    TYPE_DESTINATION,
    TYPE_NONE
  };

  // Type and bounds of the phantom window.
  struct PhantomPlacement {
    PhantomPlacement();
    ~PhantomPlacement();

    PhantomType type;
    gfx::Rect bounds;
  };

  // Returns the final bounds to place the window at. This differs from
  // the current if there is a grid.
  gfx::Rect GetFinalBounds() const;

  // Lays out the attached windows. |bounds| is the bounds of the main window.
  void LayoutAttachedWindowsHorizontally(const gfx::Rect& bounds);
  void LayoutAttachedWindowsVertically(const gfx::Rect& bounds);

  // Adjusts the bounds to enforce that windows are vertically contained in the
  // work area.
  void AdjustBoundsForMainWindow(gfx::Rect* bounds) const;
  void AdjustBoundsForWindow(const gfx::Rect& work_area,
                             aura::Window* window,
                             gfx::Rect* bounds) const;

  // Clears the cached width/height of the window being dragged.
  void ClearCachedHeights();
  void ClearCachedWidths();

  // Returns true if the window touches the bottom/right edge of the work area.
  bool TouchesBottomOfScreen() const;
  bool TouchesRightSideOfScreen() const;

  // Returns a coordinate along the primary axis. Used to share code for
  // left/right multi window resize and top/bottom resize.
  int PrimaryAxisSize(const gfx::Size& size) const;
  int PrimaryAxisCoordinate(int x, int y) const;

  // Updates the bounds of the phantom window.
  void UpdatePhantomWindow(const gfx::Point& location);

  // Updates |phantom_placement_| when type is one of TYPE_LEFT_EDGE or
  // TYPE_RIGHT_EDGE.
  void UpdatePhantomWindowBoundsAlongEdge(
      const PhantomPlacement& last_placement);

  // Returns a PhantomPlacement for the specified point. TYPE_NONE is used if
  // the location doesn't have a valid phantom location.
  PhantomPlacement GetPhantomPlacement(const gfx::Point& location);

  aura::Window* window() const { return details_.window; }

  const Details details_;

  // True if the window size (height) should be constrained.
  const bool constrain_size_;

  const std::vector<aura::Window*> attached_windows_;

  // Set to true once Drag() is invoked and the bounds of the window change.
  bool did_move_or_resize_;

  internal::RootWindowEventFilter* root_filter_;

  // The initial size of each of the windows in |attached_windows_| along the
  // primary axis.
  std::vector<int> initial_size_;

  // The min size of each of the windows in |attached_windows_| along the
  // primary axis.
  std::vector<int> min_size_;

  // The amount each of the windows in |attached_windows_| can be compressed.
  // This is a fraction of the amount a window can be compressed over the total
  // space the windows can be compressed.
  std::vector<float> compress_fraction_;

  // Sum of sizes in |min_size_|.
  int total_min_;

  // Sum of the sizes in |initial_size_|.
  int total_initial_size_;

  // Gives a previews of where the the window will end up. Only used if there
  // is a grid and the caption is being dragged.
  scoped_ptr<PhantomWindowController> phantom_window_controller_;

  // Last PhantomPlacement.
  PhantomPlacement phantom_placement_;

  // Number of mouse moves since the last bounds change. Only used for phantom
  // placement to track when the mouse is moved while pushed against the edge of
  // the screen.
  int num_mouse_moves_since_bounds_change_;

  DISALLOW_COPY_AND_ASSIGN(WorkspaceWindowResizer);
};

}  // namespace internal
}  // namespace ash

#endif  // ASH_WM_WORKSPACE_WINDOW_RESIZER_H_
