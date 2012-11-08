// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PICTURE_PILE_H_
#define CC_PICTURE_PILE_H_

#include "base/basictypes.h"
#include "cc/cc_export.h"
#include "cc/picture.h"
#include "cc/scoped_ptr_vector.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"

namespace cc {

class CC_EXPORT PicturePile {
public:
  PicturePile();
  ~PicturePile();

  // Mark a portion of the PicturePile as invalid and needing to be re-recorded
  // the next time update is called.
  void invalidate(gfx::Rect);

  // Resize the PicturePile, invalidating / dropping recorded pictures as necessary.
  void resize(gfx::Size);

  // Rerecord parts of the picture that are invalid.
  void update(ContentLayerClient* painter);

  // Update other with a shallow copy of this (main => compositor thread commit)
  void pushPropertiesTo(PicturePile& other);

  // Clone a paint-safe version of this picture (with cloned PicturePileRecords)
  scoped_ptr<PicturePile> cloneForDrawing(gfx::Rect rect);

  // Raster a subrect of this PicturePile into the given canvas.
  // It's only safe to call paint on a cloned version.
  void paint(SkCanvas* canvas, gfx::Rect rect);

protected:
  std::vector<scoped_refptr<Picture> > pile_;
  gfx::Size size_;

  DISALLOW_COPY_AND_ASSIGN(PicturePile);
};

}  // namespace cc

#endif  // CC_PICTURE_PILE_H_
