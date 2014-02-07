// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_NATIVE_THEME_NATIVE_THEME_AURAWIN_H_
#define UI_NATIVE_THEME_NATIVE_THEME_AURAWIN_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ui/native_theme/native_theme_aura.h"

namespace gfx {
class NineImagePainter;
}

namespace ui {

// Aura implementation of native theme support.
class NATIVE_THEME_EXPORT NativeThemeAuraWin : public NativeThemeAura {
 public:
  static NativeThemeAuraWin* instance();

 private:
  NativeThemeAuraWin();
  virtual ~NativeThemeAuraWin();

  // Overridden from NativeThemeBase:
  virtual gfx::Size GetPartSize(Part part,
                                State state,
                                const ExtraParams& extra) const OVERRIDE;
  virtual void Paint(SkCanvas* canvas,
                     Part part,
                     State state,
                     const gfx::Rect& rect,
                     const ExtraParams& extra) const OVERRIDE;
  virtual void PaintArrowButton(SkCanvas* gc,
                                const gfx::Rect& rect,
                                Part direction,
                                State state) const OVERRIDE;
  virtual void PaintScrollbarTrack(
      SkCanvas* sk_canvas,
      Part part,
      State state,
      const ScrollbarTrackExtraParams& extra_params,
      const gfx::Rect& rect) const OVERRIDE;
  virtual void PaintScrollbarThumb(SkCanvas* sk_canvas,
                                   Part part,
                                   State state,
                                   const gfx::Rect& rect) const OVERRIDE;
  virtual void PaintScrollbarCorner(SkCanvas* canvas,
                                    State state,
                                    const gfx::Rect& rect) const OVERRIDE;

  // Returns the NineImagePainter used to paint the specified state, creating if
  // necessary. If no image is provided for the specified state the normal state
  // images are used.
  gfx::NineImagePainter* GetOrCreatePainter(
      const int image_ids[kMaxState][9],
      State state,
      scoped_ptr<gfx::NineImagePainter> painters[kMaxState]) const;

  // Paints |painter| into the canvas using |rect|.
  void PaintPainter(gfx::NineImagePainter* painter,
                    SkCanvas* sk_canvas,
                    const gfx::Rect& rect) const;

  mutable scoped_ptr<gfx::NineImagePainter> scrollbar_track_painter_;

  mutable scoped_ptr<gfx::NineImagePainter>
      scrollbar_thumb_painters_[kMaxState];

  mutable scoped_ptr<gfx::NineImagePainter>
      scrollbar_arrow_button_painters_[kMaxState];

  DISALLOW_COPY_AND_ASSIGN(NativeThemeAuraWin);
};

}  // namespace ui

#endif  // UI_NATIVE_THEME_NATIVE_THEME_AURAWIN_H_
