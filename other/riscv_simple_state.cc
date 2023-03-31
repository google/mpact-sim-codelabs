// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "other/riscv_simple_state.h"

#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "mpact/sim/generic/type_helpers.h"
#include "mpact/sim/util/memory/flat_demand_memory.h"
#include "other/riscv_register.h"

namespace mpact {
namespace sim {
namespace riscv {

RiscVState::RiscVState(absl::string_view id, RiscVXlen xlen,
                       util::MemoryInterface *memory,
                       util::AtomicMemoryOpInterface *atomic_memory)
    : ArchState(id),
      xlen_(xlen),
      memory_(memory),
      atomic_memory_(atomic_memory) {
  if (memory_ == nullptr) {
    memory_ = owned_memory_ = new util::FlatDemandMemory(0);
  }

  DataBuffer *db = nullptr;
  switch (xlen_) {
    case RiscVXlen::RV32: {
      auto *pc32 = GetRegister<RV32Register>(kPcName).first;
      pc_src_operand_ = pc32->CreateSourceOperand();
      pc_dst_operand_ = pc32->CreateDestinationOperand(0);
      pc_ = pc32;
      db = db_factory()->Allocate<RV32Register::ValueType>(1);
      db->Set<uint32_t>(0, 0);
      break;
    }
    default:
      LOG(ERROR) << "Unsupported xlen";
      return;
  }

  set_pc_operand(pc_src_operand_);
  pc_->SetDataBuffer(db);
  db->DecRef();
}

RiscVState::~RiscVState() {
  delete pc_src_operand_;
  delete pc_dst_operand_;
  delete owned_memory_;
}

void RiscVState::LoadMemory(const Instruction *inst, uint64_t address,
                            DataBuffer *db, Instruction *child_inst,
                            ReferenceCount *context) {
  memory_->Load(address, db, child_inst, context);
}

void RiscVState::LoadMemory(const Instruction *inst, DataBuffer *address_db,
                            DataBuffer *mask_db, int el_size, DataBuffer *db,
                            Instruction *child_inst, ReferenceCount *context) {
  memory_->Load(address_db, mask_db, el_size, db, child_inst, context);
}

void RiscVState::StoreMemory(const Instruction *inst, uint64_t address,
                             DataBuffer *db) {
  memory_->Store(address, db);
}

void RiscVState::StoreMemory(const Instruction *inst, DataBuffer *address_db,
                             DataBuffer *mask_db, int el_size, DataBuffer *db) {
  memory_->Store(address_db, mask_db, el_size, db);
}

void RiscVState::Fence(const Instruction *inst, int fm, int predecessor,
                       int successor) {
  // TODO: Add fence operation once operations have non-zero latency.
}

void RiscVState::FenceI(const Instruction *inst) {
  // TODO: Add instruction fence operation when needed.
}

void RiscVState::ECall(const Instruction *inst) {
  if (on_ecall_ != nullptr) {
    auto res = on_ecall_(inst);
    if (res) return;
  }
  std::string where = (inst != nullptr)
                          ? absl::StrCat(absl::Hex(inst->address()))
                          : "unknown location";

  LOG(ERROR) << "ECall called without handler at address: " << where;
  LOG(ERROR) << "Treating as nop";
}

void RiscVState::EBreak(const Instruction *inst) {
  for (auto &handler : on_ebreak_) {
    bool res = handler(inst);
    if (res) return;
  }
  std::string where = (inst != nullptr)
                          ? absl::StrCat(absl::Hex(inst->address()))
                          : "unknown location";

  LOG(ERROR) << "EBreak called without handler at address: " << where;
  LOG(ERROR) << "Treating as nop";
}

void RiscVState::WFI(const Instruction *inst) {
  if (on_wfi_ != nullptr) {
    bool res = on_wfi_(inst);
    if (res) return;
  }

  std::string where = (inst != nullptr)
                          ? absl::StrCat(absl::Hex(inst->address()))
                          : "unknown location";

  LOG(INFO) << "No handler for wfi: treating as nop: " << where;
}

}  // namespace riscv
}  // namespace sim
}  // namespace mpact
