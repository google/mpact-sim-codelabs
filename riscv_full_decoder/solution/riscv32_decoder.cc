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

#include "riscv_full_decoder/solution/riscv32_decoder.h"

namespace mpact {
namespace sim {
namespace codelab {

using ::mpact::sim::riscv::RiscVState;
using ::mpact::sim::util::MemoryInterface;

RiscV32Decoder::RiscV32Decoder(RiscVState *state, MemoryInterface *memory)
    : state_(state), memory_(memory) {
  // Allocate the isa factory class, the top level isa decoder instance, and
  // the encoding parser.
  riscv_isa_factory_ = new RiscV32IsaFactory();
  riscv_isa_ = new RiscV32IInstructionSet(state, riscv_isa_factory_);
  riscv_encoding_ = new RiscV32IEncoding(state);
  // Need a data buffer to load instructions from memory. Allocate a single
  // buffer that can be reused for each instruction word.
  inst_db_ = state_->db_factory()->Allocate<uint32_t>(1);
}

RiscV32Decoder::~RiscV32Decoder() {
  inst_db_->DecRef();
  delete riscv_isa_;
  delete riscv_isa_factory_;
  delete riscv_encoding_;
}

generic::Instruction *RiscV32Decoder::DecodeInstruction(uint64_t address) {
  // Read the instruction word from memory and parse it in the encoding parser.
  memory_->Load(address, inst_db_, nullptr, nullptr);
  uint32_t iword = inst_db_->Get<uint32_t>(0);
  riscv_encoding_->ParseInstruction(iword);

  // Call the isa decoder to obtain a new instruction object for the instruction
  // word that was parsed above.
  auto *instruction = riscv_isa_->Decode(address, riscv_encoding_);
  return instruction;
}

}  // namespace codelab
}  // namespace sim
}  // namespace mpact
