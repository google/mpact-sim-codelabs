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

#ifndef MPACT_SIM_CODELABS_RISCV_FULL_DECODER_SOLUTION_RISCV32_DECODER_H_
#define MPACT_SIM_CODELABS_RISCV_FULL_DECODER_SOLUTION_RISCV32_DECODER_H_

#include <memory>

#include "mpact/sim/generic/arch_state.h"
#include "mpact/sim/generic/decoder_interface.h"
#include "mpact/sim/generic/instruction.h"
#include "mpact/sim/util/memory/memory_interface.h"
#include "other/riscv_simple_state.h"
#include "riscv_full_decoder/solution/riscv32i_encoding.h"
#include "riscv_isa_decoder/solution/riscv32i_decoder.h"

namespace mpact {
namespace sim {
namespace codelab {

// This is the factory class needed by the generated decoder. It is responsible
// for creating the decoder for each slot instance. Since the riscv architecture
// only has a single slot, it's a pretty simple class.
class RiscV32IsaFactory : public RiscV32IInstructionSetFactory {
 public:
  std::unique_ptr<Riscv32Slot> CreateRiscv32Slot(ArchState *state) override {
    return std::make_unique<Riscv32Slot>(state);
  }
};

// This class implements the generic DecoderInterface and provides a bridge
// to the (isa specific) generated decoder classes.
class RiscV32Decoder : public generic::DecoderInterface {
 public:
  using SlotEnum = SlotEnum;
  using OpcodeEnum = OpcodeEnum;

  RiscV32Decoder(riscv::RiscVState *state, util::MemoryInterface *memory);
  RiscV32Decoder() = delete;
  ~RiscV32Decoder() override;

  // This will always return a valid instruction that can be executed. In the
  // case of a decode error, the semantic function in the instruction object
  // instance will raise an internal simulator error when executed.
  generic::Instruction *DecodeInstruction(uint64_t address) override;

 private:
  riscv::RiscVState *state_;
  util::MemoryInterface *memory_;
  RiscV32IsaFactory *riscv_isa_factory_;
  RiscV32IEncoding *riscv_encoding_;
  RiscV32IInstructionSet *riscv_isa_;
  generic::DataBuffer *inst_db_;
};

}  // namespace codelab
}  // namespace sim
}  // namespace mpact

#endif  // MPACT_SIM_CODELABS_RISCV_FULL_DECODER_SOLUTION_RISCV32_DECODER_H_
