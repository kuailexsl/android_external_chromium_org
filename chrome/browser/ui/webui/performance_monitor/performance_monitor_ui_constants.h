// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_PERFORMANCE_MONITOR_PERFORMANCE_MONITOR_UI_CONSTANTS_H_
#define CHROME_BROWSER_UI_WEBUI_PERFORMANCE_MONITOR_PERFORMANCE_MONITOR_UI_CONSTANTS_H_

namespace performance_monitor {

enum EventCategory {
  EVENT_CATEGORY_EXTENSIONS,
  EVENT_CATEGORY_CHROME,
  EVENT_CATEGORY_EXCEPTIONS,
  EVENT_CATEGORY_NUMBER_OF_CATEGORIES
};

enum MetricCategory {
  METRIC_CATEGORY_CPU,
  METRIC_CATEGORY_MEMORY,
  METRIC_CATEGORY_TIMING,
  METRIC_CATEGORY_NETWORK,
  METRIC_CATEGORY_NUMBER_OF_CATEGORIES
};

enum Unit {
  UNIT_BYTES,
  UNIT_KILOBYTES,
  UNIT_MEGABYTES,
  UNIT_GIGABYTES,
  UNIT_TERABYTES,
  UNIT_MICROSECONDS,
  UNIT_MILLISECONDS,
  UNIT_SECONDS,
  UNIT_MINUTES,
  UNIT_HOURS,
  UNIT_DAYS,
  UNIT_WEEKS,
  UNIT_MONTHS,
  UNIT_YEARS,
  UNIT_PERCENT,
  UNIT_UNDEFINED
};

}  // namespace performance_monitor

#endif  // CHROME_BROWSER_UI_WEBUI_PERFORMANCE_MONITOR_PERFORMANCE_MONITOR_UI_CONSTANTS_H_
