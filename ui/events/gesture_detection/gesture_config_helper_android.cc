// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/gesture_detection/gesture_config_helper.h"

#include "ui/gfx/android/view_configuration.h"
#include "ui/gfx/screen.h"

using gfx::ViewConfiguration;

namespace ui {
namespace {
// TODO(jdduke): Adopt GestureConfiguration on Android, crbug/339203.

// This was the minimum tap/press size used on Android before the new gesture
// detection pipeline.
const float kMinGestureBoundsLengthDips = 24.f;

// This value is somewhat arbitrary, but provides a reasonable maximum
// approximating a large thumb depression.
const float kMaxGestureBoundsLengthDips = kMinGestureBoundsLengthDips * 4.f;

GestureDetector::Config DefaultGestureDetectorConfig(
    const gfx::Display& display) {
  GestureDetector::Config config;

  config.longpress_timeout = base::TimeDelta::FromMilliseconds(
      ViewConfiguration::GetLongPressTimeoutInMs());
  config.showpress_timeout =
      base::TimeDelta::FromMilliseconds(ViewConfiguration::GetTapTimeoutInMs());
  config.double_tap_timeout = base::TimeDelta::FromMilliseconds(
      ViewConfiguration::GetDoubleTapTimeoutInMs());

  const float px_to_dp = 1.f / display.device_scale_factor();
  config.touch_slop =
      ViewConfiguration::GetTouchSlopInPixels() * px_to_dp;
  config.double_tap_slop =
      ViewConfiguration::GetDoubleTapSlopInPixels() * px_to_dp;
  config.minimum_fling_velocity =
      ViewConfiguration::GetMinimumFlingVelocityInPixelsPerSecond() * px_to_dp;
  config.maximum_fling_velocity =
      ViewConfiguration::GetMaximumFlingVelocityInPixelsPerSecond() * px_to_dp;

  return config;
}

ScaleGestureDetector::Config DefaultScaleGestureDetectorConfig(
    const gfx::Display& display) {
  ScaleGestureDetector::Config config;

  config.gesture_detector_config = DefaultGestureDetectorConfig(display);
  config.quick_scale_enabled = true;

  const float px_to_dp = 1.f / display.device_scale_factor();
  config.min_scaling_touch_major =
      ViewConfiguration::GetMinScalingTouchMajorInPixels() * px_to_dp;
  config.use_touch_major_in_span =
      ViewConfiguration::ShouldUseTouchMajorInScalingSpan();
  config.min_scaling_span =
      ViewConfiguration::GetMinScalingSpanInPixels() * px_to_dp;
  // As the |min_scaling_span| platform constant assumes that touch major values
  // are used when computing the span, subtract off a reasonable touch major
  // value for the case where the touch major values are not used.
  if (!config.use_touch_major_in_span) {
    config.min_scaling_span = std::max(
        0.f, config.min_scaling_span - 2.f * config.min_scaling_touch_major);
  }

  return config;
}

}  // namespace

GestureProvider::Config DefaultGestureProviderConfig() {
  GestureProvider::Config config;
  config.display = gfx::Screen::GetNativeScreen()->GetPrimaryDisplay();
  config.gesture_detector_config = DefaultGestureDetectorConfig(config.display);
  config.scale_gesture_detector_config =
      DefaultScaleGestureDetectorConfig(config.display);
  config.gesture_begin_end_types_enabled = false;
  config.min_gesture_bounds_length = kMinGestureBoundsLengthDips;
  config.max_gesture_bounds_length = kMaxGestureBoundsLengthDips;
  return config;
}

}  // namespace ui
