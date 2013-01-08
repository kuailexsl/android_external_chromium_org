// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "cc/picture_pile.h"
#include "cc/picture_pile_impl.h"

namespace {
// Maximum number of pictures that can overlap before we collapse them into
// a larger one.
const int kMaxOverlapping = 2;
// Maximum percentage area of the base picture another picture in the pile
// can be.  If higher, we destroy the pile and recreate from scratch.
const float kResetThreshold = 0.7f;
}

namespace cc {

PicturePile::PicturePile()
    : min_contents_scale_(0),
      buffer_pixels_(0) {
  SetMinContentsScale(1);
}

PicturePile::~PicturePile() {
}

void PicturePile::Resize(gfx::Size size) {
  if (size_ == size)
    return;

  pile_.clear();
  size_ = size;
}

void PicturePile::SetMinContentsScale(float min_contents_scale) {
  DCHECK(min_contents_scale);
  if (min_contents_scale_ == min_contents_scale)
    return;

  pile_.clear();
  min_contents_scale_ = min_contents_scale;

  // Picture contents are played back scaled. When the final contents scale is
  // less than 1 (i.e. low res), then multiple recorded pixels will be used
  // to raster one final pixel.  To avoid splitting a final pixel across
  // pictures (which would result in incorrect rasterization due to blending), a
  // buffer margin is added so that any picture can be snapped to integral
  // final pixels.
  //
  // For example, if a 1/4 contents scale is used, then that would be 3 buffer
  // pixels, since that's the minimum number of pixels to add so that resulting
  // content can be snapped to a four pixel aligned grid.
  buffer_pixels_ = static_cast<int>(ceil(1 / min_contents_scale_) - 1);
  buffer_pixels_ = std::max(0, buffer_pixels_);
}

void PicturePile::Update(
    ContentLayerClient* painter,
    const Region& invalidation,
    RenderingStats& stats) {
  if (pile_.empty()) {
    ResetPile(painter, stats);
    return;
  }

  for (Region::Iterator i(invalidation); i.has_rect(); i.next())
    InvalidateRect(i.rect());

  for (Pile::iterator i = pile_.begin(); i != pile_.end(); ++i) {
    if (!(*i)->HasRecording())
      (*i)->Record(painter, stats);
  }
}

class FullyContainedPredicate {
public:
  FullyContainedPredicate(gfx::Rect rect) : layer_rect_(rect) { }
  bool operator()(const scoped_refptr<Picture>& picture) {
    return layer_rect_.Contains(picture->LayerRect());
  }
  gfx::Rect layer_rect_;
};

void PicturePile::InvalidateRect(gfx::Rect invalidation) {
  if (invalidation.IsEmpty())
    return;

  // Inflate all recordings from invalidations with a margin so that when
  // scaled down to at least min_contents_scale, any final pixel touched by an
  // invalidation can be fully rasterized by this picture.
  invalidation.Inset(
      -buffer_pixels_,
      -buffer_pixels_,
      -buffer_pixels_,
      -buffer_pixels_);
  invalidation.Intersect(gfx::Rect(size_));

  std::vector<Pile::iterator> overlaps;
  for (Pile::iterator i = pile_.begin(); i != pile_.end(); ++i) {
    if ((*i)->LayerRect().Contains(invalidation) && !(*i)->HasRecording())
      return;
    if ((*i)->LayerRect().Intersects(invalidation) && i != pile_.begin())
      overlaps.push_back(i);
  }

  gfx::Rect picture_rect = invalidation;
  if (overlaps.size() >= kMaxOverlapping) {
    for (size_t j = 0; j < overlaps.size(); j++)
      picture_rect = gfx::UnionRects(picture_rect, (*overlaps[j])->LayerRect());
  }
  if (picture_rect.size().GetArea() / static_cast<float>(size_.GetArea()) >
      kResetThreshold)
    picture_rect = gfx::Rect(size_);

  FullyContainedPredicate pred(picture_rect);
  pile_.erase(std::remove_if(pile_.begin(), pile_.end(), pred), pile_.end());

  pile_.push_back(Picture::Create(picture_rect));
}


void PicturePile::ResetPile(ContentLayerClient* painter,
                            RenderingStats& stats) {
  pile_.clear();

  scoped_refptr<Picture> base_picture = Picture::Create(gfx::Rect(size_));
  base_picture->Record(painter, stats);
  pile_.push_back(base_picture);
}

void PicturePile::PushPropertiesTo(PicturePileImpl* other) {
  other->pile_ = pile_;
  other->min_contents_scale_ = min_contents_scale_;
  // Remove all old clones.
  other->clones_.clear();
}

}  // namespace cc
