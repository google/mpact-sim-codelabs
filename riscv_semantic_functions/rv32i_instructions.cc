// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "riscv_semantic_functions/rv32i_instructions.h"

#include <functional>
#include <iostream>

#include "mpact/sim/generic/arch_state.h"
#include "mpact/sim/generic/data_buffer.h"
#include "mpact/sim/generic/instruction_helpers.h"
#include "riscv/riscv_state.h"

namespace mpact {
namespace sim {
namespace codelab {

using generic::DataBuffer;
using riscv::RiscVState;

void RV32IllegalInstruction(Instruction *inst) {
  inst->state()
      ->program_error_controller()
      ->GetProgramError(generic::ProgramErrorController::kInternalErrorName)
      ->Raise(
          absl::StrCat("Illegal instruction at ", absl::Hex(inst->address())));
  std::cerr << absl::StrCat("Illegal instruction at 0x",
                            absl::Hex(inst->address()), "\n");
}

// The following instruction semantic functions implement basic alu operations.
// They are used for both register-register and register-immediate versions of
// the corresponding instructions.

// Semantic functions for Exercise 2.
void RV32IAdd(Instruction *instruction) {}

void RV32IAnd(Instruction *instruction) {}

void RV32IOr(Instruction *instruction) {}

void RV32ISll(Instruction *instruction) {}

void RV32ISltu(Instruction *instruction) {}

void RV32ISra(Instruction *instruction) {}

void RV32ISrl(Instruction *instruction) {}

void RV32ISub(Instruction *instruction) {}

void RV32IXor(Instruction *instruction) {}
// End semantic functions for exercise 2.

// Semantic functions for Exercise 3.
// Load upper immediate. It is assumed that the decoder already shifted the
// immediate.
void RV32ILui(Instruction *instruction) {}

// Add upper immediate to PC (for PC relative addressing). It is assumed that
// the decoder already shifted the immediate.
void RV32IAuipc(Instruction *instruction) {}
// End semantic functions for Exercise 3.

// Semantic functions for Exercise 4.
// Branch instructions.
void RV32IBeq(Instruction *instruction) {}

void RV32IBge(Instruction *instruction) {}

void RV32IBgeu(Instruction *instruction) {}

void RV32IBlt(Instruction *instruction) {}

void RV32IBltu(Instruction *instruction) {}

void RV32IBne(Instruction *instruction) {}

// Jal instruction.
void RV32IJal(Instruction *instruction) {}

// Jalr instruction.
void RV32IJalr(Instruction *instruction) {}
// End of semantic functions for Exercise 4.

// Semantic functions for Exercise 5.
// Store instructions.
void RV32ISw(Instruction *instruction) {}

void RV32ISh(Instruction *instruction) {}

void RV32ISb(Instruction *instruction) {}
// End of semantic functions for Exercise 5.

// Semantic functions for Exercise 6.
// Load instructions.
void RV32ILw(Instruction *instruction) {}

void RV32ILwChild(Instruction *instruction) {}

void RV32ILh(Instruction *instruction) {}

void RV32ILhChild(Instruction *instruction) {}

void RV32ILhu(Instruction *instruction) {}

void RV32ILhuChild(Instruction *instruction) {}

void RV32ILb(Instruction *instruction) {}

void RV32ILbChild(Instruction *instruction) {}

void RV32ILbu(Instruction *instruction) {}

void RV32ILbuChild(Instruction *instruction) {}
// End of semantic functions for Exercise 6.

// Fence.
void RV32IFence(Instruction *instruction) {
  uint32_t bits = instruction->Source(0)->AsUint32(0);
  int fm = (bits >> 8) & 0xf;
  int predecessor = (bits >> 4) & 0xf;
  int successor = bits & 0xf;
  auto *state = static_cast<RiscVState *>(instruction->state());
  state->Fence(instruction, fm, predecessor, successor);
}

// Ebreak - software breakpoint instruction.
void RV32IEbreak(Instruction *instruction) {
  auto *state = static_cast<RiscVState *>(instruction->state());
  state->EBreak(instruction);
}

}  // namespace codelab
}  // namespace sim
}  // namespace mpact
