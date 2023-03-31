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

#ifndef MPACT_SIM_CODELABS_OTHER_RISCV_SIMPLE_STATE_H_
#define MPACT_SIM_CODELABS_OTHER_RISCV_SIMPLE_STATE_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "absl/functional/any_invocable.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "mpact/sim/generic/arch_state.h"
#include "mpact/sim/generic/data_buffer.h"
#include "mpact/sim/generic/instruction.h"
#include "mpact/sim/generic/operand_interface.h"
#include "mpact/sim/generic/ref_count.h"
#include "mpact/sim/generic/type_helpers.h"
#include "mpact/sim/util/memory/flat_demand_memory.h"
#include "mpact/sim/util/memory/memory_interface.h"
#include "other/riscv_register.h"

namespace mpact {
namespace sim {
namespace riscv {

using ArchState = ::mpact::sim::generic::ArchState;
using DataBuffer = ::mpact::sim::generic::DataBuffer;
using Instruction = ::mpact::sim::generic::Instruction;
using ReferenceCount = ::mpact::sim::generic::ReferenceCount;

// A simple load context class for convenience.
struct LoadContext : public generic::ReferenceCount {
  explicit LoadContext(DataBuffer *vdb) : value_db(vdb) {}
  ~LoadContext() override {
    if (value_db != nullptr) value_db->DecRef();
  }

  // Override the base class method so that the data buffer can be DecRef'ed
  // when the context object is recycled.
  void OnRefCountIsZero() override {
    if (value_db != nullptr) value_db->DecRef();
    value_db = nullptr;
    // Call the base class method.
    generic::ReferenceCount::OnRefCountIsZero();
  }
  // Data buffers for the value loaded from memory (byte, half, word, etc.).
  DataBuffer *value_db = nullptr;
};

// Supported values of Xlen.
enum class RiscVXlen : uint64_t {
  RV32 = 0b01,
  RVUnknown = 4,
};

// Class that extends ArchState with RiscV specific methods. These methods
// implement RiscV specific memory operations, memory/IO fencing, system
// calls and software breakpoints.
class RiscVState : public ArchState {
 public:
  static constexpr char kXregPrefix[] = "x";
  static constexpr char kVregPrefix[] = "v";
  static constexpr char kNextPcName[] = "next_pc";
  static constexpr char kPcName[] = "pc";

  RiscVState(absl::string_view id, RiscVXlen xlen,
             util::MemoryInterface *memory,
             util::AtomicMemoryOpInterface *atomic_memory);
  RiscVState(absl::string_view id, RiscVXlen xlen,
             util::MemoryInterface *memory)
      : RiscVState(id, xlen, memory, nullptr) {}
  RiscVState(absl::string_view id, RiscVXlen xlen)
      : RiscVState(id, xlen, nullptr, nullptr) {}
  ~RiscVState() override;

  // Deleted Constructors and operators.
  RiscVState(const RiscVState &) = delete;
  RiscVState(RiscVState &&) = delete;
  RiscVState &operator=(const RiscVState &) = delete;
  RiscVState &operator=(RiscVState &&) = delete;

  // Return a pair consisting of pointer to the named register and a bool that
  // is true if the register had to be created, and false if it was found
  // in the register map (or if nullptr is returned).
  template <typename RegisterType>
  std::pair<RegisterType *, bool> GetRegister(absl::string_view name) {
    // If the register already exists, return a pointer to the register.
    auto ptr = registers()->find(std::string(name));
    if (ptr != registers()->end())
      return std::make_pair(static_cast<RegisterType *>(ptr->second), false);
    // Create a new register and return a pointer to the object.
    return std::make_pair(AddRegister<RegisterType>(name), true);
  }

  // Add register alias.
  template <typename RegisterType>
  absl::Status AddRegisterAlias(absl::string_view current_name,
                                absl::string_view new_name) {
    auto ptr = registers()->find(std::string(current_name));
    if (ptr == registers()->end()) {
      return absl::NotFoundError(
          absl::StrCat("Register '", current_name, "' does not exist."));
    }
    AddRegister(new_name, ptr->second);
    return absl::OkStatus();
  }

  // Methods called by instruction semantic functions to load from memory.
  void LoadMemory(const Instruction *inst, uint64_t address, DataBuffer *db,
                  Instruction *child_inst, ReferenceCount *context);
  void LoadMemory(const Instruction *inst, DataBuffer *address_db,
                  DataBuffer *mask_db, int el_size, DataBuffer *db,
                  Instruction *child_inst, ReferenceCount *context);
  // Methods called by instruction semantic functions to store to memory.
  void StoreMemory(const Instruction *inst, uint64_t address, DataBuffer *db);
  void StoreMemory(const Instruction *inst, DataBuffer *address_db,
                   DataBuffer *mask_db, int el_size, DataBuffer *db);
  // Called by the fence instruction semantic function to signal a fence
  // operation.
  void Fence(const Instruction *inst, int fm, int predecessor, int successor);
  // Synchronize instruction and data streams.
  void FenceI(const Instruction *inst);
  // System call.
  void ECall(const Instruction *inst);
  // Breakpoint.
  void EBreak(const Instruction *inst);
  // WFI
  void WFI(const Instruction *inst);
  // Trap.
  void Trap(bool is_interrupt, uint64_t trap_value, uint64_t exception_code,
            uint64_t epc, const Instruction *inst);
  // Add ebreak handler.
  void AddEbreakHandler(absl::AnyInvocable<bool(const Instruction *)> handler) {
    on_ebreak_.emplace_back(std::move(handler));
  }

  // Accessors.
  void set_memory(util::MemoryInterface *memory) { memory_ = memory; }
  util::MemoryInterface *memory() const { return memory_; }
  util::AtomicMemoryOpInterface *atomic_memory() const {
    return atomic_memory_;
  }

  // Setters for handlers for ecall, and trap. The handler returns true
  // if the instruction/event was handled, and false otherwise.

  void set_on_ecall(absl::AnyInvocable<bool(const Instruction *)> callback) {
    on_ecall_ = std::move(callback);
  }

  void set_on_wfi(absl::AnyInvocable<bool(const Instruction *)> callback) {
    on_wfi_ = std::move(callback);
  }

  void set_on_trap(
      absl::AnyInvocable<bool(bool /*is_interrupt*/, uint64_t /*trap_value*/,
                              uint64_t /*exception_code*/, uint64_t /*epc*/,
                              const Instruction *)>
          callback) {
    on_trap_ = std::move(callback);
  }

  int flen() const { return flen_; }
  RiscVXlen xlen() const { return xlen_; }

  // Getters for select CSRs.

 private:
  RiscVXlen xlen_;
  // Program counter register.
  generic::RegisterBase *pc_;
  // Operands used to access pc values generically. Note, the pc value may read
  // as the address of the next instruction during execution of an instruction,
  // so the address of the instruction executing should be used instead.
  generic::SourceOperandInterface *pc_src_operand_ = nullptr;
  generic::DestinationOperandInterface *pc_dst_operand_ = nullptr;
  int flen_ = 0;
  util::FlatDemandMemory *owned_memory_ = nullptr;
  util::MemoryInterface *memory_ = nullptr;
  util::AtomicMemoryOpInterface *atomic_memory_ = nullptr;
  std::vector<absl::AnyInvocable<bool(const Instruction *)>> on_ebreak_;
  absl::AnyInvocable<bool(const Instruction *)> on_ecall_;
  absl::AnyInvocable<bool(bool, uint64_t, uint64_t, uint64_t,
                          const Instruction *)>
      on_trap_;
  absl::AnyInvocable<bool(const Instruction *)> on_wfi_;
};

}  // namespace riscv
}  // namespace sim
}  // namespace mpact

#endif // MPACT_SIM_CODELABS_OHTER_RISCV_SIMPLE_STATE_H_
