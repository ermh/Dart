# Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
# for details. All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.

{
  'variables': {
    'common_gcc_warning_flags': [
      '-Wall',
      '-Wextra', # Also known as -W.
      '-Wno-unused-parameter',
      # TODO(v8-team): Fix V8 build.
      #'-Wold-style-cast',
    ],

    # Default value.  This may be overridden in a containing project gyp.
    'target_arch%': 'ia32',
  },
  'conditions': [
    [ 'OS=="linux"', { 'includes': [ 'configurations_make.gypi', ], } ],
    [ 'OS=="mac"', { 'includes': [ 'configurations_xcode.gypi', ], } ],
    [ 'OS=="win"', { 'includes': [ 'configurations_msvs.gypi', ], } ],
  ],
  'target_defaults': {
    'default_configuration': 'Debug_ia32',
    'configurations': {
      'Dart_Base': {
        'abstract': 1,
      },

      'Dart_ia32_Base': {
        'abstract': 1,
      },

      'Dart_x64_Base': {
        'abstract': 1,
      },

      'Dart_simarm_Base': {
        'abstract': 1,
        'defines': [
          'TARGET_ARCH_ARM',
        ]
      },

      'Dart_arm_Base': {
        'abstract': 1,
        'defines': [
          'TARGET_ARCH_ARM',
        ],
      },

      'Dart_Debug': {
        'abstract': 1,
      },

      'Dart_Release': {
        'abstract': 1,
        'defines': [
          'NDEBUG',
        ],
      },

      'Debug_ia32': {
        'inherit_from': ['Dart_Base', 'Dart_ia32_Base', 'Dart_Debug'],
      },

      'Release_ia32': {
        'inherit_from': ['Dart_Base', 'Dart_ia32_Base', 'Dart_Release'],
      },

      'Debug_x64': {
        'inherit_from': ['Dart_Base', 'Dart_x64_Base', 'Dart_Debug'],
      },

      'Release_x64': {
        'inherit_from': ['Dart_Base', 'Dart_x64_Base', 'Dart_Release'],
      },

      'Debug_simarm': {
        # Should not inherit from Dart_Debug because Dart_simarm_Base defines
        # the optimization level to be -O3, as the simulator runs too slow
        # otherwise.
        'inherit_from': ['Dart_Base', 'Dart_simarm_Base'],
        'defines': [
          'DEBUG',
        ],
      },

      'Release_simarm': {
        # Should not inherit from Dart_Release (see Debug_simarm).
        'inherit_from': ['Dart_Base', 'Dart_simarm_Base'],
        'defines': [
          'NDEBUG',
        ],
      },

      'Debug_arm': {
        'inherit_from': ['Dart_Base', 'Dart_arm_Base', 'Dart_Debug'],
      },

      'Release_arm': {
        'inherit_from': ['Dart_Base', 'Dart_arm_Base', 'Dart_Release'],
      },

      'Debug_dartc': {
        # If we build any native code (e.g. V8), then we should just use the
        # release version.
        'inherit_from': ['Release_ia32'],
      },

      'Release_dartc': {
        'inherit_from': ['Release_ia32'],
      },

      # These targets assume that target_arch is passed in explicitly
      # by the containing project (e.g., chromium).
      'Debug': {
        'inherit_from': ['Debug_<(target_arch)']
      },

      'Release': {
        'inherit_from': ['Release_<(target_arch)']
      },
    },
  },
}
