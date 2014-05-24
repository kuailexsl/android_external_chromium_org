# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'wm',
      'type': '<(component)',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../skia/skia.gyp:skia',
        '../aura/aura.gyp:aura',
        '../compositor/compositor.gyp:compositor',
        '../events/events.gyp:events',
        '../events/events.gyp:events_base',
        '../gfx/gfx.gyp:gfx_geometry',
        '../gfx/gfx.gyp:gfx',
        '../resources/ui_resources.gyp:ui_resources',
        '../base/ui_base.gyp:ui_base',
      ],
      'defines': [
        'WM_IMPLEMENTATION',
      ],
      'sources': [
        'core/nested_accelerator_dispatcher_linux.cc',
        'core/nested_accelerator_dispatcher_win.cc',
        'core/nested_accelerator_dispatcher.cc',
        'core/nested_accelerator_dispatcher.h',
        'core/nested_accelerator_delegate.h',
        'core/nested_accelerator_controller.cc',
        'core/nested_accelerator_controller.h',
        'core/base_focus_rules.h',
        'core/base_focus_rules.cc',
        'core/base_focus_rules.h',
        'core/capture_controller.cc',
        'core/capture_controller.h',
        'core/compound_event_filter.cc',
        'core/compound_event_filter.h',
        'core/cursor_manager.cc',
        'core/cursor_manager.h',
        'core/default_activation_client.cc',
        'core/default_activation_client.h',
        'core/easy_resize_window_targeter.cc',
        'core/easy_resize_window_targeter.h',
        'core/focus_controller.cc',
        'core/focus_controller.h',
        'core/focus_rules.h',
        'core/image_grid.cc',
        'core/image_grid.h',
        'core/input_method_event_filter.cc',
        'core/input_method_event_filter.h',
        'core/masked_window_targeter.cc',
        'core/masked_window_targeter.h',
        'core/native_cursor_manager.h',
        'core/native_cursor_manager_delegate.h',
        'core/shadow.cc',
        'core/shadow.h',
        'core/shadow_controller.cc',
        'core/shadow_controller.h',
        'core/shadow_types.cc',
        'core/shadow_types.h',
        'core/transient_window_controller.cc',
        'core/transient_window_controller.h',
        'core/transient_window_manager.cc',
        'core/transient_window_manager.h',
        'core/transient_window_observer.h',
        'core/transient_window_stacking_client.cc',
        'core/transient_window_stacking_client.h',
        'core/user_activity_detector.cc',
        'core/user_activity_detector.h',
        'core/user_activity_observer.h',
        'core/visibility_controller.cc',
        'core/visibility_controller.h',
        'core/window_animations.cc',
        'core/window_animations.h',
        'core/window_modality_controller.cc',
        'core/window_modality_controller.h',
        'core/window_util.cc',
        'core/window_util.h',
        'core/wm_core_switches.cc',
        'core/wm_core_switches.h',
        'core/wm_state.cc',
        'core/wm_state.h',
        'public/window_types.h',
        'wm_export.h',
      ],
    },
    {
      'target_name': 'wm_test_support',
      'type': 'static_library',
      'dependencies': [
        '../../skia/skia.gyp:skia',
        '../aura/aura.gyp:aura',
        '../events/events.gyp:events',
        '../events/events.gyp:events_base',
      ],
      'sources': [
        'test/wm_test_helper.cc',
        'test/wm_test_helper.h',
      ],
    },
    {
      'target_name': 'wm_unittests',
      'type': 'executable',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:test_support_base',
        '../../skia/skia.gyp:skia',
        '../../testing/gtest.gyp:gtest',
        '../aura/aura.gyp:aura',
        '../aura/aura.gyp:aura_test_support',
        '../compositor/compositor.gyp:compositor',
        '../events/events.gyp:events',
        '../events/events.gyp:events_base',
        '../gfx/gfx.gyp:gfx_geometry',
        '../gfx/gfx.gyp:gfx',
        '../base/ui_base.gyp:ui_base',
        'wm',
        'wm_test_support',
      ],
      'sources': [
        'test/run_all_unittests.cc',
        'core/compound_event_filter_unittest.cc',
        'core/cursor_manager_unittest.cc',
        'core/focus_controller_unittest.cc',
        'core/input_method_event_filter_unittest.cc',
        'core/image_grid_unittest.cc',
        'core/nested_accelerator_controller_unittest.cc',
        'core/shadow_controller_unittest.cc',
        'core/transient_window_manager_unittest.cc',
        'core/transient_window_stacking_client_unittest.cc',
        'core/user_activity_detector_unittest.cc',
        'core/visibility_controller_unittest.cc',
        'core/window_animations_unittest.cc',
        'core/window_util_unittest.cc',
      ],
    },
  ],
}
