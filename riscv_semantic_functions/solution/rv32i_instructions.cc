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

#include "riscv_semantic_functions/solution/rv32i_instructions.h"

#include <cstdint>
#include <functional>
#include <iostream>

#include "mpact/sim/generic/arch_state.h"
#include "mpact/sim/generic/data_buffer.h"
#include "mpact/sim/generic/instruction_helpers.h"
#include "other/riscv_simple_state.h"

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
void RV32IAdd(Instruction *instruction) {
  generic::BinaryOp<uint32_t>(instruction,
                              [](uint32_t a, uint32_t b) { return a + b; });
}

void RV32IAnd(Instruction *instruction) {
  generic::BinaryOp<uint32_t>(instruction,
                              [](uint32_t a, uint32_t b) { return a & b; });
}

void RV32IOr(Instruction *instruction) {
  generic::BinaryOp<uint32_t>(instruction,
                              [](uint32_t a, uint32_t b) { return a | b; });
}

void RV32ISll(Instruction *instruction) {
  generic::BinaryOp<uint32_t>(
      instruction, [](uint32_t a, uint32_t b) { return a << (b & 0x1f); });
}

void RV32ISltu(Instruction *instruction) {
  generic::BinaryOp<uint32_t>(
      instruction, [](uint32_t a, uint32_t b) { return (a < b) ? 1 : 0; });
}

void RV32ISra(Instruction *instruction) {
  generic::BinaryOp<uint32_t, int32_t, uint32_t>(
      instruction, [](int32_t a, uint32_t b) { return a >> (b & 0x1f); });
}

void RV32ISrl(Instruction *instruction) {
  generic::BinaryOp<uint32_t>(
      instruction, [](uint32_t a, uint32_t b) { return a >> (b & 0x1f); });
}

void RV32ISub(Instruction *instruction) {
  generic::BinaryOp<uint32_t>(instruction,
                              [](uint32_t a, uint32_t b) { return a - b; });
}

void RV32IXor(Instruction *instruction) {
  generic::BinaryOp<uint32_t>(instruction,
                              [](uint32_t a, uint32_t b) { return a ^ b; });
}
// End semantic functions for exercise 2.

// Semantic functions for Exercise 3.
// Load upper immediate. It is assumed that the decoder already shifted the
// immediate.
void RV32ILui(Instruction *instruction) {
  generic::UnaryOp<uint32_t>(instruction, [](uint32_t a) { return a; });
}

// Add upper immediate to PC (for PC relative addressing). It is assumed that
// the decoder already shifted the immediate.
void RV32IAuipc(Instruction *instruction) {
  generic::UnaryOp<uint32_t>(instruction, [instruction](uint32_t a) {
    return a + instruction->address();
  });
}
// End semantic functions for Exercise 3.

// Semantic functions for Exercise 4.
// Branch instructions.

template <typename OperandType>
static inline void BranchConditional(
    Instruction *instruction,
    std::function<bool(OperandType, OperandType)> cond) {
  OperandType a = generic::GetInstructionSource<OperandType>(instruction, 0);
  OperandType b = generic::GetInstructionSource<OperandType>(instruction, 1);
  if (cond(a, b)) {
    uint32_t offset = generic::GetInstructionSource<uint32_t>(instruction, 2);
    uint32_t target = offset + instruction->address();
    DataBuffer *db = instruction->Destination(0)->AllocateDataBuffer();
    db->Set<uint32_t>(0, target);
    db->Submit();
  }
}

void RV32IBeq(Instruction *instruction) {
  BranchConditional<uint32_t>(instruction,
                              [](uint32_t a, uint32_t b) { return a == b; });
}

void RV32IBge(Instruction *instruction) {
  BranchConditional<int32_t>(instruction,
                             [](int32_t a, int32_t b) { return a >= b; });
}

void RV32IBgeu(Instruction *instruction) {
  BranchConditional<uint32_t>(instruction,
                              [](uint32_t a, uint32_t b) { return a >= b; });
}

void RV32IBlt(Instruction *instruction) {
  BranchConditional<int32_t>(instruction,
                             [](int32_t a, int32_t b) { return a < b; });
}

void RV32IBltu(Instruction *instruction) {
  BranchConditional<uint32_t>(instruction,
                              [](uint32_t a, uint32_t b) { return a < b; });
}

void RV32IBne(Instruction *instruction) {
  BranchConditional<uint32_t>(instruction,
                              [](uint32_t a, uint32_t b) { return a != b; });
}

// Jal instruction.
void RV32IJal(Instruction *instruction) {
  uint32_t offset = instruction->Source(0)->AsUint32(0);
  uint32_t target = offset + instruction->address();
  uint32_t return_address = instruction->address() + instruction->size();
  auto *db = instruction->Destination(0)->AllocateDataBuffer();
  db->Set<uint32_t>(0, target);
  db->Submit();
  db = instruction->Destination(1)->AllocateDataBuffer();
  db->Set<uint32_t>(0, return_address);
  db->Submit();
}

// Jalr instruction.
void RV32IJalr(Instruction *instruction) {
  uint32_t reg_base = instruction->Source(0)->AsUint32(0);
  uint32_t offset = instruction->Source(1)->AsUint32(0);
  uint32_t target = offset + reg_base;
  uint32_t return_address = instruction->address() + instruction->size();
  auto *db = instruction->Destination(0)->AllocateDataBuffer();
  db->Set<uint32_t>(0, target);
  db->Submit();
  db = instruction->Destination(1)->AllocateDataBuffer();
  db->Set<uint32_t>(0, return_address);
  db->Submit();
}
// End of semantic functions for Exercise 4.

// Semantic functions for Exercise 5.
// Store instructions.

template <typename ValueType>
inline void StoreValue(Instruction *instruction) {
  auto base = generic::GetInstructionSource<uint32_t>(instruction, 0);
  auto offset = generic::GetInstructionSource<uint32_t>(instruction, 1);
  uint32_t address = base + offset;
  auto value = generic::GetInstructionSource<ValueType>(instruction, 2);
  auto *state = static_cast<RiscVState *>(instruction->state());
  auto *db = state->db_factory()->Allocate(sizeof(ValueType));
  db->Set<ValueType>(0, value);
  state->StoreMemory(instruction, address, db);
  db->DecRef();
}

void RV32ISw(Instruction *instruction) { StoreValue<uint32_t>(instruction); }

void RV32ISh(Instruction *instruction) { StoreValue<uint16_t>(instruction); }

void RV32ISb(Instruction *instruction) { StoreValue<uint8_t>(instruction); }

// End of semantic functions for Exercise 5.

// Semantic functions for Exercise 6.
// Load instructions.
template <typename ValueType>
inline void LoadValue(Instruction *instruction) {
  auto base = generic::GetInstructionSource<uint32_t>(instruction, 0);
  auto offset = generic::GetInstructionSource<uint32_t>(instruction, 1);
  uint32_t address = base + offset;
  auto *state = static_cast<RiscVState *>(instruction->state());
  auto *db = state->db_factory()->Allocate(sizeof(ValueType));
  db->set_latency(0);
  auto *context = new riscv::LoadContext(db);
  state->LoadMemory(instruction, address, db, instruction->child(), context);
  context->DecRef();
}

template <typename ValueType>
inline void LoadValueChild(Instruction *instruction) {
  auto *context = static_cast<riscv::LoadContext *>(instruction->context());
  uint32_t value = static_cast<uint32_t>(context->value_db->Get<ValueType>(0));
  auto *db = instruction->Destination(0)->AllocateDataBuffer();
  db->Set<uint32_t>(0, value);
  db->Submit();
}

void RV32ILw(Instruction *instruction) { LoadValue<uint32_t>(instruction); }

void RV32ILwChild(Instruction *instruction) {
  LoadValueChild<uint32_t>(instruction);
}

void RV32ILh(Instruction *instruction) { LoadValue<int16_t>(instruction); }

void RV32ILhChild(Instruction *instruction) {
  LoadValueChild<int16_t>(instruction);
}

void RV32ILhu(Instruction *instruction) { LoadValue<uint16_t>(instruction); }

void RV32ILhuChild(Instruction *instruction) {
  LoadValueChild<uint16_t>(instruction);
}

void RV32ILb(Instruction *instruction) { LoadValue<int8_t>(instruction); }

void RV32ILbChild(Instruction *instruction) {
  LoadValueChild<int8_t>(instruction);
}

void RV32ILbu(Instruction *instruction) { LoadValue<uint8_t>(instruction); }

void RV32ILbuChild(Instruction *instruction) {
  LoadValueChild<uint8_t>(instruction);
}
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
