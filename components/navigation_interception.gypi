# Copyright (c) 2012 The Chromium Authors. All rights reserved.
#
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'conditions': [
    ['OS != "ios"', {
      'targets': [
        {
          'target_name': 'navigation_interception',
          'type': 'static_library',
          'defines!': ['CONTENT_IMPLEMENTATION'],
          'dependencies': [
            '../base/base.gyp:base',
            '../content/content.gyp:content_browser',
            '../content/content.gyp:content_common',
            '../net/net.gyp:net',
          ],
          'include_dirs': [
            '..',
            '../skia/config',
            '<(SHARED_INTERMEDIATE_DIR)/navigation_interception',

          ],
          'sources': [
            'navigation_interception/intercept_navigation_resource_throttle.cc',
            'navigation_interception/intercept_navigation_resource_throttle.h',
          ],
          'conditions': [
            ['OS=="android"', {
              'dependencies': [
                  'navigation_interception_jni_headers',
              ],
              'sources': [
                'navigation_interception/component_jni_registrar.cc',
                'navigation_interception/component_jni_registrar.h',
                'navigation_interception/intercept_navigation_delegate.cc',
                'navigation_interception/intercept_navigation_delegate.h',
              ],
            }],
          ],
        },
      ],
      'conditions': [
        ['OS=="android"', {
          'targets': [
            {
              'target_name': 'navigation_interception_java',
              'type': 'none',
              'dependencies': [
                '../base/base.gyp:base',
              ],
              'variables': {
                'package_name': 'navigation_interception',
                'java_in_dir': 'navigation_interception/android/java',
              },
              'includes': [ '../build/java.gypi' ],
            },
            {
              'target_name': 'navigation_interception_jni_headers',
              'type': 'none',
              'sources': [
                'navigation_interception/android/java/src/org/chromium/content/components/navigation_interception/InterceptNavigationDelegate.java',
              ],
              'variables': {
                'jni_gen_dir': 'navigation_interception',
              },
              'includes': [ '../build/jni_generator.gypi' ],
            },
          ],
        }],
      ],
    }],
  ],
}
