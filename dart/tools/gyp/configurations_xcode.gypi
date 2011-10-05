# Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
# for details. All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.

{
  'target_defaults': {
    'configurations': {
      'Dart_Base': {
        'xcode_settings': {
          # To switch to the LLVM based backend change the two lines below.
          #'GCC_VERSION': 'com.apple.compilers.llvmgcc42',
          'GCC_VERSION': '4.2',
          'GCC_C_LANGUAGE_STANDARD': 'ansi',
          'GCC_ENABLE_CPP_EXCEPTIONS': 'NO', # -fno-exceptions
          'GCC_ENABLE_CPP_RTTI': 'NO', # -fno-rtti
          'GCC_DEBUGGING_SYMBOLS': 'default', # -g
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'YES', # Do not strip symbols
          'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES', # -fvisibility=hidden
          'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES', # -fvisibility-inlines-hidden
          'GCC_WARN_NON_VIRTUAL_DESTRUCTOR': 'YES', # -Wnon-virtual-dtor
          # TODO(v8-team): Fix V8 build.
          #'GCC_WARN_HIDDEN_VIRTUAL_FUNCTIONS': 'YES', # -Woverloaded-virtual
          'GCC_TREAT_WARNINGS_AS_ERRORS': 'YES', # -Werror
          'WARNING_CFLAGS': [
            '<@(common_gcc_warning_flags)',
            '-Wtrigraphs', # Disable Xcode default.
          ],

          # Generate PIC code as Chrome is switching to this.
          'GCC_DYNAMIC_NO_PIC': 'NO',

          # When searching for header files, do not search
          # subdirectories. Without this option, vm/assert.h conflicts
          # with the system header assert.h. Apple also recommend
          # setting this to NO.
          'ALWAYS_SEARCH_USER_PATHS': 'NO',

          # Attempt to remove compiler options that Xcode adds by default.
          'GCC_CW_ASM_SYNTAX': 'NO', # Remove -fasm-blocks.

          'GCC_ENABLE_PASCAL_STRINGS': 'NO',
          'GCC_ENABLE_TRIGRAPHS': 'NO',
          'PREBINDING': 'NO',
        },
      },

      'Dart_ia32_Base': {
        'xcode_settings': {
          'ARCHS': 'i386',
        },
      },

      'Dart_x64_Base': {
        'xcode_settings': {
          'ARCHS': 'x86_64',
        },
      },

      'Dart_simarm_Base': {
        'xcode_settings': {
          'ARCHS': 'i386',
          'GCC_OPTIMIZATION_LEVEL': '3',
        },
      },

      'Dart_Debug': {
        'xcode_settings': {
          'GCC_OPTIMIZATION_LEVEL': '0',
        },
      },

      'Dart_Release': {
        'xcode_settings': {
          'GCC_OPTIMIZATION_LEVEL': '3',
        },
      },
    },
  },
}
