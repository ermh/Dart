// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/compiler_stats.h"

#include "vm/flags.h"
#include "vm/timer.h"


namespace dart {

DEFINE_FLAG(bool, compiler_stats, false, "Compiler stat counters.");

// Bytes allocated for generated code.
intptr_t CompilerStats::code_allocated = 0;

// Total number of characters in source.
intptr_t CompilerStats::src_length = 0;

// Cumulative runtime of parser.
Timer CompilerStats::parser_timer(true, "parser timer");

// Cumulative runtime of scanner.
Timer CompilerStats::scanner_timer(true, "scanner timer");

intptr_t CompilerStats::num_tokens_total = 0;
intptr_t CompilerStats::num_tokens_consumed = 0;
intptr_t CompilerStats::num_token_checks = 0;
intptr_t CompilerStats::num_tokens_rewind = 0;
intptr_t CompilerStats::num_tokens_lookahead = 0;

void CompilerStats::Print() {
  if (!FLAG_compiler_stats) {
    return;
  }
  OS::Print("==== Compiler Stats ====\n");
  OS::Print("Number of tokens:   %ld\n", num_tokens_total);
  OS::Print("Tokens consumed:    %ld  (%.2f times number of tokens)\n",
            num_tokens_consumed,
            (1.0 * num_tokens_consumed) / num_tokens_total);
  OS::Print("Tokens checked:     %ld  (%.2f times tokens consumed)\n",
            num_token_checks, (1.0 * num_token_checks) / num_tokens_consumed);
  OS::Print("Token rewind:       %ld  (%ld%% of tokens checked)\n",
            num_tokens_rewind, (100 * num_tokens_rewind) / num_token_checks);
  OS::Print("Token lookahead:    %ld  (%ld%% of tokens checked)\n",
            num_tokens_lookahead,
            (100 * num_tokens_lookahead) / num_token_checks);
  OS::Print("Source length:      %ld characters\n", src_length);
  intptr_t scan_usecs = scanner_timer.TotalElapsedTime();
  OS::Print("Scanner time:       %ld msecs\n",
            scan_usecs / 1000);
  intptr_t parse_usecs = parser_timer.TotalElapsedTime();
  OS::Print("Compilation time:   %ld msecs\n",
            parse_usecs / 1000);
  OS::Print("Compilation speed:  %ld tokens per msec\n",
            1000 * num_tokens_total / parse_usecs);
  OS::Print("Code size:          %ld KB\n",
            code_allocated / 1024);
  OS::Print("Code density:       %ld tokens per KB\n",
            num_tokens_total * 1024 / code_allocated);
}

}  // namespace dart

