# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'ui_resources',
      'type': 'none',
      'variables': {
        'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/ui/ui_resources',
      },
      'actions': [
        {
          'action_name': 'ui_resources',
          'variables': {
            'grit_grd_file': 'resources/ui_resources.grd',
          },
          'includes': [ '../build/grit_action.gypi' ],
        },
      ],
      'includes': [ '../build/grit_target.gypi' ],
    },
    {
      'target_name': 'ui_resources_standard',
      'type': 'none',
      'variables': {
        'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/ui/ui_resources_standard',
      },
      'actions': [
        {
          'action_name': 'ui_resources_standard',
          'variables': {
            'grit_grd_file': 'resources/ui_resources_standard.grd',
          },
          'includes': [ '../build/grit_action.gypi' ],
        },
      ],
      # gfx_resources.pak is used by DumpRenderTree.
      # TODO(oshima): Update DumpRenderTree.gyp to use new pak file and
      # remove this.
      'copies': [ {
          'destination': '<(SHARED_INTERMEDIATE_DIR)/ui/gfx/',
          'files' : [
             '<(SHARED_INTERMEDIATE_DIR)/ui/ui_resources_standard/gfx_resources.pak',
          ],
        },
      ],
      'includes': [ '../build/grit_target.gypi' ],
    },
  ],
}
