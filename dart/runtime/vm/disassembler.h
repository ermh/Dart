// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef VM_DISASSEMBLER_H_
#define VM_DISASSEMBLER_H_

#include "vm/allocation.h"
#include "vm/assembler.h"
#include "vm/globals.h"

namespace dart {

// Froward declaration.
class MemoryRegion;

// Disassembly formatter interface, which consumes the
// disassembled instructions in any desired form.
class DisassemblyFormatter {
 public:
  DisassemblyFormatter() { }
  virtual ~DisassemblyFormatter() { }

  // Consume the decoded instruction at the given pc.
  virtual void ConsumeInstruction(char* hex_buffer,
                                  intptr_t hex_size,
                                  char* human_buffer,
                                  intptr_t human_size,
                                  uword pc) = 0;
};


// Basic disassembly formatter that outputs the disassembled instruction
// to stdout.
class DisassembleToStdout : public DisassemblyFormatter {
 public:
  DisassembleToStdout() : DisassemblyFormatter() { }
  ~DisassembleToStdout() { }

  virtual void ConsumeInstruction(char* hex_buffer,
                                  intptr_t hex_size,
                                  char* human_buffer,
                                  intptr_t human_size,
                                  uword pc);

 private:
  DISALLOW_ALLOCATION()
  DISALLOW_COPY_AND_ASSIGN(DisassembleToStdout);
};


// Disassemble instructions.
class Disassembler : public AllStatic {
 public:
  // Disassemble instructions between start and end.
  // (The assumption is that start is at a valid instruction).
  static void Disassemble(uword start,
                          uword end,
                          DisassemblyFormatter* formatter);
  static void Disassemble(uword start, uword end) {
    DisassembleToStdout stdout_formatter;
    Disassemble(start, end, &stdout_formatter);
  }

  // Disassemble instructions in a memory region.
  static void DisassembleMemoryRegionRange(const MemoryRegion& instructions,
                                           uword from,
                                           uword to,
                                           DisassemblyFormatter* formatter);
  static void DisassembleMemoryRegion(const MemoryRegion& instructions,
                                      DisassemblyFormatter* formatter) {
    uword start = instructions.start();
    uword end = instructions.end();
    Disassemble(start, end, formatter);
  }
  static void DisassembleMemoryRegion(const MemoryRegion& instructions) {
    uword start = instructions.start();
    uword end = instructions.end();
    Disassemble(start, end);
  }

  // Decodes one instruction.
  // Writes a hexadecimal representation into the hex_buffer and a
  // human-readable representation into the human_buffer.
  // Returns the length of the decoded instruction in bytes.
  static int DecodeInstruction(char* hex_buffer, intptr_t hex_size,
                               char* human_buffer, intptr_t human_size,
                               uword pc);

  // Returns the name for the given register. For instance the Register EAX
  // returns the string "eax".
  static const char* RegisterName(Register reg);

 private:
  static const int kHexadecimalBufferSize = 32;
  static const int kUserReadableBufferSize = 256;
};

}  // namespace dart

#endif  // VM_DISASSEMBLER_H_
