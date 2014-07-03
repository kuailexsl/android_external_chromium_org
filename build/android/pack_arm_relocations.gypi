# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file is meant to be included into an action to provide a rule that
# packs ARM relative relocations in native libraries.
#
# To use this, create a gyp target with the following form:
#  {
#    'action_name': 'pack_arm_relocations',
#    'actions': [
#      'variables': {
#        'enable_packing': 'pack relocations if 1, plain file copy if 0'
#        'exclude_packing_list': 'names of libraries explicitly not packed',
#        'ordered_libraries_file': 'file generated by write_ordered_libraries'
#        'input_paths': 'files to be added to the list of inputs'
#        'stamp': 'file to touch when the action is complete'
#        'stripped_libraries_dir': 'directory holding stripped libraries',
#        'packed_libraries_dir': 'directory holding packed libraries',
#      'includes': [ '../../build/android/pack_arm_relocations.gypi' ],
#    ],
#  },
#

{
  'variables': {
    'input_paths': [],
  },
  'inputs': [
    '<(DEPTH)/build/android/gyp/util/build_utils.py',
    '<(DEPTH)/build/android/gyp/pack_arm_relocations.py',
    '<(ordered_libraries_file)',
    '>@(input_paths)',
  ],
  'outputs': [
    '<(stamp)',
  ],
  'conditions': [
    ['enable_packing == 1', {
      'message': 'Packing ARM relative relocations for <(_target_name)',
      'dependencies': [
        '<(DEPTH)/tools/relocation_packer/relocation_packer.gyp:relocation_packer#host',
      ],
      'inputs': [
        '<(PRODUCT_DIR)/relocation_packer',
      ],
      'action': [
        'python', '<(DEPTH)/build/android/gyp/pack_arm_relocations.py',
        '--enable-packing=1',
        '--exclude-packing-list=<@(exclude_packing_list)',
        '--android-pack-relocations=<(PRODUCT_DIR)/relocation_packer',
        '--android-objcopy=<(android_objcopy)',
        '--stripped-libraries-dir=<(stripped_libraries_dir)',
        '--packed-libraries-dir=<(packed_libraries_dir)',
        '--libraries-file=<(ordered_libraries_file)',
        '--stamp=<(stamp)',
      ],
    }, {
      'message': 'Copying libraries (no relocation packing) for <(_target_name)',
      'action': [
        'python', '<(DEPTH)/build/android/gyp/pack_arm_relocations.py',
        '--enable-packing=0',
        '--stripped-libraries-dir=<(stripped_libraries_dir)',
        '--packed-libraries-dir=<(packed_libraries_dir)',
        '--libraries-file=<(ordered_libraries_file)',
        '--stamp=<(stamp)',
      ],
    }],
    ['component == "shared_library"', {
      # Add a fake output to force the build to always re-run this step. This
      # is required because the real inputs are not known at gyp-time and
      # changing base.so may not trigger changes to dependent libraries.
      'outputs': [ '<(stamp).fake' ]
    }],
  ],
}
