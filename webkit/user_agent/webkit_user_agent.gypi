# Copyright 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'conditions': [
      ['inside_chromium_build==0', {
        'webkit_src_dir': '../../../../..',
      },{
        'webkit_src_dir': '../../third_party/WebKit',
      }],
    ],
  },
  'targets': [
    {
      'target_name': 'user_agent',
      'type': '<(component)',
      'defines': [
        'WEBKIT_USER_AGENT_IMPLEMENTATION',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
      ],
      'sources': [
        'user_agent.cc',
        'user_agent.h',
        'user_agent_util.cc',
        'user_agent_util_ios.mm',
        'user_agent_util.h',
        'webkit_user_agent_export.h',
      ],
      'conditions': [
        ['OS == "ios"', {
          # iOS has different user-agent construction utilities, since the
          # version strings is not derived from webkit_version, and follows
          # a different format.
          'sources!': [
            'user_agent_util.cc',
          ],
        }, {  # OS != "ios"
          'dependencies': [
            'webkit_version',
            '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
          ],
        }],
      ],
    },
  ],
  'conditions': [
    ['OS != "ios"', {
      'targets': [
        {
          'target_name': 'webkit_version',
          'type': 'none',
          'actions': [
            {
              'action_name': 'webkit_version',
              'inputs': [
                '<(script)',
                '<(webkit_src_dir)<(version_file)',
                '../../build/util/lastchange.py',  # Used by the script.
              ],
              'outputs': [
                '<(SHARED_INTERMEDIATE_DIR)/webkit_version.h',
              ],
              'action': ['python', '<(script)', '<(webkit_src_dir)',
                         '<(version_file)', '<(SHARED_INTERMEDIATE_DIR)'],
              'variables': {
                'script': '../build/webkit_version.py',
                # version_file is a relative path from |webkit_src_dir| to
                # the version file.  But gyp will eat the variable unless
                # it looks like an absolute path, so write it like one and
                # then use it carefully above.
                'version_file': '/Source/WebCore/Configurations/Version.xcconfig',
              },
            },
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              '<(SHARED_INTERMEDIATE_DIR)',
            ],
          },
          # Dependents may rely on files generated by this target or one of its
          # own hard dependencies.
          'hard_dependency': 1,
        },
      ],
    }],
  ],
}
