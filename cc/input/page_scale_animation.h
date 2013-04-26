// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_INPUT_PAGE_SCALE_ANIMATION_H_
#define CC_INPUT_PAGE_SCALE_ANIMATION_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "ui/gfx/size.h"
#include "ui/gfx/vector2d_f.h"

namespace cc {
class TimingFunction;

// A small helper class that does the math for zoom animations, primarily for
// double-tap zoom. Initialize it with starting and ending scroll/page scale
// positions and an animation length time, then call ...AtTime() at every frame
// to obtain the current interpolated position. The supplied timing function
// is used to ease the animation.
//
// All sizes and vectors in this class's public methods are in the root scroll
// layer's coordinate space.
class PageScaleAnimation {
 public:
  // Construct with the state at the beginning of the animation.
  static scoped_ptr<PageScaleAnimation> Create(
      gfx::Vector2dF start_scroll_offset,
      float start_page_scale_factor,
      gfx::SizeF viewport_size,
      gfx::SizeF root_layer_size,
      double start_time,
      scoped_ptr<TimingFunction> timing_function);

  ~PageScaleAnimation();

  // The following methods initialize the animation. Call one of them
  // immediately after construction to set the final scroll and page scale.

  // Zoom while explicitly specifying the top-left scroll position.
  void ZoomTo(gfx::Vector2dF target_scroll_offset,
              float target_page_scale_factor,
              double duration);

  // Zoom based on a specified anchor. The animator will attempt to keep it
  // at the same position on the physical display throughout the animation,
  // unless the edges of the root layer are hit. The anchor is specified
  // as an offset from the content layer.
  void ZoomWithAnchor(gfx::Vector2dF anchor,
                      float target_page_scale_factor,
                      double duration);

  // Call these functions while the animation is in progress to output the
  // current state.
  gfx::Vector2dF ScrollOffsetAtTime(double time) const;
  float PageScaleFactorAtTime(double time) const;
  bool IsAnimationCompleteAtTime(double time) const;

  // The following methods return state which is invariant throughout the
  // course of the animation.
  double start_time() const { return start_time_; }
  double duration() const { return duration_; }
  double end_time() const { return start_time_ + duration_; }
  gfx::Vector2dF target_scroll_offset() const { return target_scroll_offset_; }
  float target_page_scale_factor() const { return target_page_scale_factor_; }

 protected:
  PageScaleAnimation(gfx::Vector2dF start_scroll_offset,
                     float start_page_scale_factor,
                     gfx::SizeF viewport_size,
                     gfx::SizeF root_layer_size,
                     double start_time,
                     scoped_ptr<TimingFunction> timing_function);

 private:
  void ClampTargetScrollOffset();
  void InferTargetScrollOffsetFromStartAnchor();
  void InferTargetAnchorFromScrollOffsets();

  gfx::SizeF StartViewportSize() const;
  gfx::SizeF TargetViewportSize() const;
  float InterpAtTime(double time) const;
  gfx::SizeF ViewportSizeAt(float interp) const;
  gfx::Vector2dF ScrollOffsetAt(float interp) const;
  gfx::Vector2dF AnchorAt(float interp) const;
  gfx::Vector2dF ViewportRelativeAnchorAt(float interp) const;
  float PageScaleFactorAt(float interp) const;

  float start_page_scale_factor_;
  float target_page_scale_factor_;
  gfx::Vector2dF start_scroll_offset_;
  gfx::Vector2dF target_scroll_offset_;

  gfx::Vector2dF start_anchor_;
  gfx::Vector2dF target_anchor_;

  gfx::SizeF viewport_size_;
  gfx::SizeF root_layer_size_;

  double start_time_;
  double duration_;

  scoped_ptr<TimingFunction> timing_function_;

  DISALLOW_COPY_AND_ASSIGN(PageScaleAnimation);
};

}  // namespace cc

#endif  // CC_INPUT_PAGE_SCALE_ANIMATION_H_
