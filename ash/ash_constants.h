// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ASH_CONSTANTS_H_
#define ASH_ASH_CONSTANTS_H_

#include "ash/ash_export.h"
#include "ui/aura/window.h"
#include "ui/base/ui_base_types.h"

namespace ash {

// The window is a constrained window and lives therefore entirely within
// another aura window.
ASH_EXPORT extern const aura::WindowProperty<bool>* const
    kConstrainedWindowKey;

// In the window corners, the resize areas don't actually expand bigger, but the
// 16 px at the end of each edge triggers diagonal resizing.
ASH_EXPORT extern const int kResizeAreaCornerSize;

// Ash windows do not have a traditional visible window frame. Window content
// extends to the edge of the window. We consider a small region outside the
// window bounds and an even smaller region overlapping the window to be the
// "non-client" area and use it for resizing.
ASH_EXPORT extern const int kResizeOutsideBoundsSize;
ASH_EXPORT extern const int kResizeOutsideBoundsScaleForTouch;
ASH_EXPORT extern const int kResizeInsideBoundsSize;

} // namespace ash

#endif  // ASH_ASH_CONSTANTS_H_
