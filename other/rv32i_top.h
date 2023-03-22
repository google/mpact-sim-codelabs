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

#ifndef MPACT_SIM_CODELABS_OTHER_RV32I_TOP_H_
#define MPACT_SIM_CODELABS_OTHER_RV32I_TOP_H_

#include <string>

#include "absl/status/status.h"
#include "absl/synchronization/notification.h"
#include "mpact/sim/generic/component.h"
#include "mpact/sim/generic/core_debug_interface.h"
#include "mpact/sim/generic/decode_cache.h"
#include "mpact/sim/util/memory/flat_demand_memory.h"
#include "mpact/sim/util/memory/memory_interface.h"
#include "mpact/sim/util/memory/memory_watcher.h"
#include "riscv/riscv32_htif_semihost.h"
#include "riscv/riscv_breakpoint.h"
#include "riscv/riscv_register.h"
#include "riscv/riscv_state.h"
#include "riscv_full_decoder/solution/riscv32_decoder.h"
#include "riscv_isa_decoder/solution/riscv32i_enums.h"

namespace mpact {
namespace sim {
namespace codelab {

using riscv::RiscV32HtifSemiHost;
using riscv::RiscVBreakpointManager;
using riscv::RiscVState;
using riscv::RV32Register;

// Top level class for the RiscV32G simulator. This is the main interface for
// interacting and controlling execution of programs running on the simulator.
// This class brings together the decoder, the architecture state, and control.
class RV32ITop : public generic::Component, public generic::CoreDebugInterface {
 public:
  using RunStatus = generic::CoreDebugInterface::RunStatus;
  using HaltReason = generic::CoreDebugInterface::HaltReason;
  using SemiHostAddresses = RiscV32HtifSemiHost::SemiHostAddresses;

  explicit RV32ITop(std::string name);
  ~RV32ITop() override;

  // Methods inherited from CoreDebugInterface.
  absl::Status Halt() override;
  absl::StatusOr<int> Step(int num) override;
  absl::Status Run() override;
  absl::Status Wait() override;

  absl::StatusOr<RunStatus> GetRunStatus() override;
  absl::StatusOr<HaltReason> GetLastHaltReason() override;

  absl::StatusOr<uint64_t> ReadRegister(const std::string &name) override;
  absl::Status WriteRegister(const std::string &name, uint64_t value) override;

  // Read and Write memory methods bypass any semihosting.
  absl::StatusOr<size_t> ReadMemory(uint64_t address, void *buf,
                                    size_t length) override;
  absl::StatusOr<size_t> WriteMemory(uint64_t address, const void *buf,
                                     size_t length) override;

  bool HasBreakpoint(uint64_t address) override;
  absl::Status SetSwBreakpoint(uint64_t address) override;
  absl::Status ClearSwBreakpoint(uint64_t address) override;
  absl::Status ClearAllSwBreakpoints() override;

  absl::StatusOr<Instruction *>GetInstruction(uint64_t address) override;

  absl::StatusOr<std::string> GetDisassembly(uint64_t address) override;

  // Set up semihosting with the given magic addresses.
  absl::Status SetUpSemiHosting(const SemiHostAddresses &magic);

  // Accessors.
  RiscVState *state() const { return state_; }
  util::MemoryInterface *memory() const { return memory_; }

 private:
  // Called when a halt is requested.
  void RequestHalt(HaltReason halt_reason, const Instruction *inst);

  uint32_t previous_pc_;
  // The DB factory is used to manage data buffers for memory read/writes.
  generic::DataBufferFactory db_factory_;
  // Current status and last halt reasons.
  RunStatus run_status_ = RunStatus::kHalted;
  HaltReason halt_reason_ = HaltReason::kNone;
  // Halting flag. This is set to true when execution must halt.
  bool halted_ = false;
  absl::Notification *run_halted_ = nullptr;
  // The local RiscV32 state.
  RiscVState *state_;
  // Semihosting class.
  RiscV32HtifSemiHost *rv32_semihost_ = nullptr;
  // Breakpoint manager.
  RiscVBreakpointManager *rv_bp_manager_ = nullptr;
  // The pc register instance.
  RV32Register *pc_;
  // RiscV32 decoder instance.
  RiscV32Decoder *rv32_decoder_ = nullptr;
  // Decode cache, memory and memory watcher.
  generic::DecodeCache *rv32_decode_cache_ = nullptr;
  util::FlatDemandMemory *memory_ = nullptr;
  util::MemoryWatcher *watcher_ = nullptr;
  // Counter for the number of instructions simulated.
  generic::SimpleCounter<uint64_t>
      counter_opcode_[static_cast<int>(OpcodeEnum::kPastMaxValue)];
  generic::SimpleCounter<uint64_t> counter_num_instructions_;
};

}  // namespace codelab
}  // namespace sim
}  // namespace mpact

#endif  // MPACT_SIM_CODELABS_OTHER_RV32I_TOP_H_
