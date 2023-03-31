/*
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MPACT_SIM_CODELABS_RISCV_FULL_DECODER_SOLUTION_RISCV32I_ENCODING_H_
#define MPACT_SIM_CODELABS_RISCV_FULL_DECODER_SOLUTION_RISCV32I_ENCODING_H_

#include <functional>
#include <string>

#include "absl/functional/any_invocable.h"
#include "mpact/sim/generic/operand_interface.h"
#include "other/riscv_simple_state.h"
#include "riscv_isa_decoder/solution/riscv32i_decoder.h"
#include "riscv_isa_decoder/solution/riscv32i_enums.h"

namespace mpact {
namespace sim {
namespace codelab {

using ::mpact::sim::generic::DestinationOperandInterface;
using ::mpact::sim::generic::PredicateOperandInterface;
using ::mpact::sim::generic::SourceOperandInterface;

// This class provides the interface between the generated instruction decoder
// framework (which is agnostic of the actual bit representation of
// instructions) and the instruction representation. This class provides methods
// to return the opcode, source operands, and destination operands for
// instructions according to the operand fields in the encoding.
class RiscV32IEncoding : public RiscV32IEncodingBase {
 public:
  explicit RiscV32IEncoding(riscv::RiscVState *state);
  ~RiscV32IEncoding() override = default;

  // Parses an instruction and determines the opcode.
  void ParseInstruction(uint32_t inst_word);

  // RiscV32 has a single slot type and single entry, so the following methods
  // ignore the SlotEnum parameter.

  // Returns the opcode in the current instruction representation.
  OpcodeEnum GetOpcode(SlotEnum, int) override { return opcode_; }

  // There is no predicate, so return nullptr.
  PredicateOperandInterface *GetPredicate(SlotEnum, int, OpcodeEnum,
                                          PredOpEnum) override {
    return nullptr;
  }
  // The following method returns a source operand that corresponds to the
  // particular operand field.
  SourceOperandInterface *GetSource(SlotEnum, int, OpcodeEnum, SourceOpEnum op,
                                    int) override;

  // The following method returns a destination operand that corresponds to the
  // particular operand field.
  DestinationOperandInterface *GetDestination(SlotEnum, int, OpcodeEnum,
                                              DestOpEnum op, int,
                                              int latency) override;
  // This method returns latency for any destination operand for which the
  // latency specifier in the .isa file is '*'. Since there are none, just
  // return 0.
  int GetLatency(SlotEnum, int, OpcodeEnum, DestOpEnum, int) override {
    return 0;
  }

  ResourceOperandInterface *GetSimpleResourceOperand(SlotEnum, int, OpcodeEnum,
                                                     SimpleResourceVector &,
                                                     int) override {
    return nullptr;
  }

  ResourceOperandInterface *GetComplexResourceOperand(SlotEnum, int, OpcodeEnum,
                                                      ComplexResourceEnum, int,
                                                      int) override {
    return nullptr;
  }

 private:
  // These two methods initialize the source and destination operand getter
  // arrays.
  void InitializeSourceOperandGetters();
  void InitializeDestinationOperandGetters();

  riscv::RiscVState *state_;
  uint32_t inst_word_;
  OpcodeEnum opcode_;

  absl::AnyInvocable<SourceOperandInterface *()>
      source_op_getters_[static_cast<int>(SourceOpEnum::kPastMaxValue)];
  absl::AnyInvocable<DestinationOperandInterface *(int)>
      dest_op_getters_[static_cast<int>(DestOpEnum::kPastMaxValue)];

  const std::string xreg_alias_[32] = {
      "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
      "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
      "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};
};

}  // namespace codelab
}  // namespace sim
}  // namespace mpact

#endif  // MPACT_SIM_CODELABS_RISCV_FULL_DECODER_SOLUTION_RISCV32I_ENCODING_H_
