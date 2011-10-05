# Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
# for details. All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.

# A set of variables needed to build some of the Chrome based subparts of the
# Dart project (e.g. V8). This is in no way a complete list of variables being
# defined by Chrome, but just the minimally needed subset.
{
  'variables': {
    'library': 'static_library',
    'component': 'static_library',
    'host_arch': 'ia32',
    'target_arch': 'ia32',
    'v8_location': '<(DEPTH)/third_party/v8',
  },
  'conditions': [
    [ 'OS=="linux"', {
      'target_defaults': {
        'ldflags': [ '-pthread', ],
      },
    }],
  ],
  'includes': [
    'xcode.gypi',
    'configurations.gypi',
    'source_filter.gypi',
  ],
}
