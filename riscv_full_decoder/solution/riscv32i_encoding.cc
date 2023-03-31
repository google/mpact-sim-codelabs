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

#include "riscv_full_decoder/solution/riscv32i_encoding.h"

#include <string>

#include "mpact/sim/generic/devnull_operand.h"
#include "mpact/sim/generic/immediate_operand.h"
#include "mpact/sim/generic/literal_operand.h"
#include "mpact/sim/generic/operand_interface.h"
#include "other/riscv_register.h"
#include "other/riscv_simple_state.h"
#include "riscv_bin_decoder/solution/riscv32i_bin_decoder.h"

namespace mpact {
namespace sim {
namespace codelab {

using riscv::RiscVState;
using riscv::RV32Register;

using generic::DevNullOperand;
using generic::ImmediateOperand;
using generic::IntLiteralOperand;

// Generic helper functions to create register operands.
template <typename RegType>
inline DestinationOperandInterface *GetRegisterDestinationOp(
    RiscVState *state, const std::string &name, int latency) {
  auto *reg = state->GetRegister<RegType>(name).first;
  return reg->CreateDestinationOperand(latency);
}

template <typename RegType>
inline DestinationOperandInterface *GetRegisterDestinationOp(
    RiscVState *state, const std::string &name, int latency,
    const std::string &op_name) {
  auto *reg = state->GetRegister<RegType>(name).first;
  return reg->CreateDestinationOperand(latency, op_name);
}

template <typename RegType>
inline SourceOperandInterface *GetRegisterSourceOp(RiscVState *state,
                                                   const std::string &name) {
  auto [reg_ptr, unused] = state->GetRegister<RegType>(name);
  auto *op = reg_ptr->CreateSourceOperand();
  return op;
}

template <typename RegType>
inline SourceOperandInterface *GetRegisterSourceOp(RiscVState *state,
                                                   const std::string &name,
                                                   const std::string &op_name) {
  auto [reg_ptr, unused] = state->GetRegister<RegType>(name);
  auto *op = reg_ptr->CreateSourceOperand(op_name);
  return op;
}

RiscV32IEncoding::RiscV32IEncoding(RiscVState *state) : state_(state) {
  InitializeSourceOperandGetters();
  InitializeDestinationOperandGetters();
}

// Parse the instruction word to determine the opcode.
void RiscV32IEncoding::ParseInstruction(uint32_t inst_word) {
  inst_word_ = inst_word;
  opcode_ = mpact::sim::codelab::DecodeRiscVInst32(inst_word_);
}

DestinationOperandInterface *RiscV32IEncoding::GetDestination(
    SlotEnum, int, OpcodeEnum, DestOpEnum dest_op, int, int latency) {
  return dest_op_getters_[static_cast<int>(dest_op)](latency);
}

SourceOperandInterface *RiscV32IEncoding::GetSource(SlotEnum, int, OpcodeEnum,
                                                    SourceOpEnum source_op,
                                                    int) {
  return source_op_getters_[static_cast<int>(source_op)]();
}

void RiscV32IEncoding::InitializeDestinationOperandGetters() {
  // Destination operand getters.
  dest_op_getters_[static_cast<int>(DestOpEnum::kNone)] = [](int latency) {
    return nullptr;
  };
  dest_op_getters_[static_cast<int>(DestOpEnum::kCsr)] = [this](int latency) {
    return GetRegisterDestinationOp<RV32Register>(state_, "CSR", latency);
  };
  dest_op_getters_[static_cast<int>(DestOpEnum::kNextPc)] =
      [this](int latency) {
        return GetRegisterDestinationOp<RV32Register>(
            state_, RiscVState::kPcName, latency);
      };
  dest_op_getters_[static_cast<int>(DestOpEnum::kRd)] =
      [this](int latency) -> DestinationOperandInterface * {
    int num = inst32_format::ExtractRd(inst_word_);
    if (num == 0) return new DevNullOperand<uint32_t>(state_, {1});
    return GetRegisterDestinationOp<RV32Register>(
        state_, absl::StrCat(RiscVState::kXregPrefix, num), latency,
        xreg_alias_[num]);
  };
}

void RiscV32IEncoding::InitializeSourceOperandGetters() {
  // Source operand getters.

  source_op_getters_[static_cast<int>(SourceOpEnum::kNone)] = []() {
    return nullptr;
  };

  // Register operands.
  source_op_getters_[static_cast<int>(SourceOpEnum::kCsr)] = [this]() {
    return GetRegisterSourceOp<RV32Register>(state_, "CSR");
  };
  source_op_getters_[static_cast<int>(SourceOpEnum::kRs1)] =
      [this]() -> SourceOperandInterface * {
    int num = inst32_format::ExtractRs1(inst_word_);
    if (num == 0) return new IntLiteralOperand<0>({1}, xreg_alias_[0]);
    return GetRegisterSourceOp<RV32Register>(
        state_, absl::StrCat(RiscVState::kXregPrefix, num), xreg_alias_[num]);
  };
  source_op_getters_[static_cast<int>(SourceOpEnum::kRs2)] =
      [this]() -> SourceOperandInterface * {
    int num = inst32_format::ExtractRs2(inst_word_);
    if (num == 0) return new IntLiteralOperand<0>({1}, xreg_alias_[0]);
    return GetRegisterSourceOp<RV32Register>(
        state_, absl::StrCat(RiscVState::kXregPrefix, num), xreg_alias_[num]);
  };

  // Immediates.
  source_op_getters_[static_cast<int>(SourceOpEnum::kBimm12)] = [this]() {
    return new ImmediateOperand<int32_t>(
        inst32_format::ExtractBImm(inst_word_));
  };
  source_op_getters_[static_cast<int>(SourceOpEnum::kImm12)] = [this]() {
    return new ImmediateOperand<int32_t>(
        inst32_format::ExtractImm12(inst_word_));
  };
  source_op_getters_[static_cast<int>(SourceOpEnum::kUimm5)] = [this]() {
    return new ImmediateOperand<uint32_t>(
        inst32_format::ExtractUimm5(inst_word_));
  };
  source_op_getters_[static_cast<int>(SourceOpEnum::kJimm20)] = [this]() {
    return new ImmediateOperand<int32_t>(
        inst32_format::ExtractJImm(inst_word_));
  };
  source_op_getters_[static_cast<int>(SourceOpEnum::kSimm12)] = [this]() {
    return new ImmediateOperand<int32_t>(
        inst32_format::ExtractSImm(inst_word_));
  };
  source_op_getters_[static_cast<int>(SourceOpEnum::kUimm20)] = [this]() {
    return new ImmediateOperand<int32_t>(
        inst32_format::ExtractUimm32(inst_word_));
  };
}

}  // namespace codelab
}  // namespace sim
}  // namespace mpact
