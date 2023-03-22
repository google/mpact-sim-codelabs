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

#include "other/rv32i_top.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>

#include "absl/functional/bind_front.h"
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "mpact/sim/generic/data_buffer.h"
#include "riscv/riscv32_htif_semihost.h"
#include "riscv/riscv_breakpoint.h"

namespace mpact {
namespace sim {
namespace codelab {

using generic::DataBuffer;
using riscv::RiscVXlen;

constexpr char kRiscV32Name[] = "RiscV";
constexpr char kRegisterAliases[32][6] = {
    "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
    "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
    "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

RV32ITop::RV32ITop(std::string name)
    : Component(name),
      counter_num_instructions_("num_instructions", 0) {
  // Using a single flat memory for this core.
  memory_ = new util::FlatDemandMemory(0);
  // Creat the simulation state.
  state_ = new RiscVState(kRiscV32Name, RiscVXlen::RV32, memory_);
  pc_ = state_->GetRegister<RV32Register>(RiscVState::kPcName).first;
  // Set up the decoder and decode cache.
  rv32_decoder_ = new RiscV32Decoder(state_, memory_);
  rv32_decode_cache_ =
      generic::DecodeCache::Create({16 * 1024, 2}, rv32_decoder_);
  // Register instruction opcode counters.
  for (int i = 0; i < static_cast<int>(OpcodeEnum::kPastMaxValue); i++) {
    counter_opcode_[i].Initialize(absl::StrCat("num_", kOpcodeNames[i]), 0);
    CHECK_OK(AddCounter(&counter_opcode_[i]));
  }
  // Register instruction counter.
  CHECK_OK(AddCounter(&counter_num_instructions_))
      << "Failed to register counter";
  rv_bp_manager_ = new RiscVBreakpointManager(
      memory_,
      absl::bind_front(&generic::DecodeCache::Invalidate, rv32_decode_cache_));
  // Set the software breakpoint callback.
  state_->AddEbreakHandler([this](const Instruction *inst) -> bool {
    // If there is a breakpoint, handle it and return true to signal that
    // the ebreak has been handled. Otherwise return false.
    if ((inst != nullptr) && (HasBreakpoint(inst->address()))) {
      RequestHalt(HaltReason::kSoftwareBreakpoint, inst);
      return true;
    }
    return false;
  });
  // Make sure the architectural and abi register aliases are added.
  for (int i = 0; i < 32; i++) {
    std::string reg_name = absl::StrCat(RiscVState::kXregPrefix, i);
    (void)state_->AddRegister<RV32Register>(reg_name);
    (void)state_->AddRegisterAlias<RV32Register>(reg_name, kRegisterAliases[i]);
  }
}

RV32ITop::~RV32ITop() {
  // If the simulator is still running, request a halt (set halted_ to true),
  // and wait until the simulator finishes before continuing the destructor.
  if (run_status_ == RunStatus::kRunning) {
    run_halted_->WaitForNotification();
    delete run_halted_;
    run_halted_ = nullptr;
  }

  delete rv32_semihost_;
  delete rv_bp_manager_;
  delete rv32_decode_cache_;
  delete rv32_decoder_;
  delete state_;
  delete watcher_;
  delete memory_;
}

absl::Status RV32ITop::Halt() {
  // If it is already halted, just return.
  if (run_status_ == RunStatus::kHalted) {
    return absl::OkStatus();
  }
  // If it is not running, then there's an error.
  if (run_status_ != RunStatus::kRunning) {
    return absl::FailedPreconditionError("RV32ITop::Halt: Core is not running");
  }
  halt_reason_ = HaltReason::kUserRequest;
  halted_ = true;
  return absl::OkStatus();
}

absl::StatusOr<int> RV32ITop::Step(int num) {
  if (num <= 0) {
    return absl::InvalidArgumentError("Step count must be > 0");
  }
  // If the simulator is running, return with an error.
  if (run_status_ != RunStatus::kHalted) {
    return absl::FailedPreconditionError("RV32ITop::Step: Core must be halted");
  }
  run_status_ = RunStatus::kSingleStep;
  int count = 0;
  halted_ = false;

  // First check to see if the previous halt was due to a breakpoint. If so,
  // need to step over the breakpoint.
  if (halt_reason_ == HaltReason::kSoftwareBreakpoint) {
    auto bp_pc = previous_pc_;
    // Disable the breakpoint. Status will show error if there is no breakpoint.
    auto status = rv_bp_manager_->DisableBreakpoint(bp_pc);
    // Execute the real instruction.
    auto prev_inst = rv32_decode_cache_->GetDecodedInstruction(bp_pc);
    prev_inst->Execute(nullptr);
    counter_opcode_[prev_inst->opcode()].Increment(1);
    counter_num_instructions_.Increment(1);
    count++;
    // Re-enable the breakpoint.
    if (status.ok()) {
      status = rv_bp_manager_->EnableBreakpoint(bp_pc);
      if (!status.ok()) return status;
    }
    // No longer stopped at breakpoint, so update halt reason.
    halt_reason_ = HaltReason::kNone;
  }

  // Step the simulator forward until the number of steps have been achieved, or
  // there is a halt request.

  DataBuffer *pc_db = pc_->data_buffer();
  uint32_t next_pc = pc_db->Get<uint32_t>(0);
  uint32_t pc;
  while (count < num) {
    pc = next_pc;
    auto *inst = rv32_decode_cache_->GetDecodedInstruction(pc);
    inst->Execute(nullptr);
    count++;
    next_pc += inst->size();
    DataBuffer *tmp_db = pc_->data_buffer();
    if (pc_db != tmp_db) {
      // PC has been updated by an instruction.
      pc_db = tmp_db;
      next_pc = pc_db->Get<uint32_t>(0);
    }
    counter_opcode_[inst->opcode()].Increment(1);
    counter_num_instructions_.Increment(1);
    if (halted_) break;
  }
  previous_pc_ = pc;
  // Update the pc register, now that it can be read.
  pc_db->Set<uint32_t>(0, next_pc);
  // If there is no halt request, there is no specific halt reason.
  if (!halted_) {
    halt_reason_ = HaltReason::kNone;
  }
  run_status_ = RunStatus::kHalted;
  return count;
}

absl::Status RV32ITop::Run() {
  // Verify that the core isn't running already.
  if (run_status_ == RunStatus::kRunning) {
    return absl::FailedPreconditionError(
        "RV32ITop::Run: core is already running");
  }

  // First check to see if the previous halt was due to a breakpoint. If so,
  // need to step over the breakpoint.
  if (halt_reason_ == HaltReason::kSoftwareBreakpoint) {
    auto bp_pc = previous_pc_;
    // Disable the breakpoint.
    auto status = rv_bp_manager_->DisableBreakpoint(bp_pc);
    // Execute the real instruction.
    auto prev_inst = rv32_decode_cache_->GetDecodedInstruction(bp_pc);
    prev_inst->Execute(nullptr);
    counter_opcode_[prev_inst->opcode()].Increment(1);
    counter_num_instructions_.Increment(1);
    // Re-enable the breakpoint.
    if (status.ok()) {
      status = rv_bp_manager_->EnableBreakpoint(bp_pc);
      if (!status.ok()) return status;
    }
    // No longer stopped at breakpoint, so update halt reason.
    halt_reason_ = HaltReason::kNone;
  }

  run_status_ = RunStatus::kRunning;
  halted_ = false;

  // The simulator is now run in a separate thread so as to allow a user
  // interface to continue operating. Allocate a new run_halted_ Notification
  // object, as they are single use only.
  run_halted_ = new absl::Notification();

  // The thread is detached so it executes without having to be joined.
  std::thread([this]() {
    DataBuffer *pc_db = pc_->data_buffer();
    uint32_t next_pc = pc_db->Get<uint32_t>(0);
    uint32_t pc;
    while (!halted_) {
      pc = next_pc;
      auto *inst = rv32_decode_cache_->GetDecodedInstruction(pc);
      inst->Execute(nullptr);
      next_pc += inst->size();
      DataBuffer *tmp_db = pc_->data_buffer();
      if (pc_db != tmp_db) {
        // PC has been updated by an instruction.
        pc_db = tmp_db;
        next_pc = pc_db->Get<uint32_t>(0);
      }
      counter_opcode_[inst->opcode()].Increment(1);
      counter_num_instructions_.Increment(1);
    }
    previous_pc_ = pc;
    // Update the pc register, now that it can be read.
    pc_db->Set<uint32_t>(0, next_pc);
    run_status_ = RunStatus::kHalted;
    // Notify that the run has completed.
    run_halted_->Notify();
  }).detach();
  return absl::OkStatus();
}

absl::Status RV32ITop::Wait() {
  // If the simulator isn't running, then just return.
  if (run_status_ != RunStatus::kRunning) {
    delete run_halted_;
    run_halted_ = nullptr;
    return absl::OkStatus();
  }

  // Wait for the simulator to finish - i.e., notification on run_halted_
  run_halted_->WaitForNotification();
  // Now delete the notification object - it is single use only.
  delete run_halted_;
  run_halted_ = nullptr;
  return absl::OkStatus();
}

absl::StatusOr<RV32ITop::RunStatus> RV32ITop::GetRunStatus() {
  return run_status_;
}

absl::StatusOr<RV32ITop::HaltReason> RV32ITop::GetLastHaltReason() {
  return halt_reason_;
}

absl::StatusOr<uint64_t> RV32ITop::ReadRegister(const std::string &name) {
  // The registers aren't protected by a mutex, so let's not read them while
  // the simulator is running.
  if (run_status_ != RunStatus::kHalted) {
    return absl::FailedPreconditionError("ReadRegister: Core must be halted");
  }
  auto iter = state_->registers()->find(name);
  // Was the register found?
  if (iter == state_->registers()->end())
    return absl::NotFoundError(absl::StrCat("Register '", name, "' not found"));

  // If requesting PC and we're stopped at a software breakpoint, the next
  // instruction to be executed is at the address of the software breakpoint, so
  // return that address.
  if ((name == "pc") && (halt_reason_ == HaltReason::kSoftwareBreakpoint)) {
    return previous_pc_;
  }

  auto *db = (iter->second)->data_buffer();
  uint64_t value;
  switch (db->size<uint8_t>()) {
    case 1:
      value = static_cast<uint64_t>(db->Get<uint8_t>(0));
      return value;
    case 2:
      value = static_cast<uint64_t>(db->Get<uint16_t>(0));
      return value;
    case 4:
      value = static_cast<uint64_t>(db->Get<uint32_t>(0));
      return value;
    case 8:
      value = static_cast<uint64_t>(db->Get<uint64_t>(0));
      return value;
    default:
      return absl::InternalError("Register size is not 1, 2, 4, or 8 bytes");
  }
}

absl::Status RV32ITop::WriteRegister(const std::string &name, uint64_t value) {
  // The registers aren't protected by a mutex, so let's not read them while
  // the simulator is running.
  if (run_status_ != RunStatus::kHalted) {
    return absl::FailedPreconditionError("WriteRegister: Core must be halted");
  }
  auto iter = state_->registers()->find(name);
  // Was the register found?
  if (iter == state_->registers()->end())
    return absl::NotFoundError(absl::StrCat("Register '", name, "' not found"));

  // If stopped at a software breakpoing and the pc is changed, change the
  // halt reason, since the next instruction won't be were we stopped.
  if ((name == "pc") && (halt_reason_ == HaltReason::kSoftwareBreakpoint)) {
    halt_reason_ = HaltReason::kNone;
  }

  auto *db = (iter->second)->data_buffer();
  switch (db->size<uint8_t>()) {
    case 1:
      db->Set<uint8_t>(0, static_cast<uint8_t>(value));
      break;
    case 2:
      db->Set<uint16_t>(0, static_cast<uint16_t>(value));
      break;
    case 4:
      db->Set<uint32_t>(0, static_cast<uint32_t>(value));
      break;
    case 8:
      db->Set<uint64_t>(0, static_cast<uint64_t>(value));
      break;
    default:
      return absl::InternalError("Register size is not 1, 2, 4, or 8 bytes");
  }
  return absl::OkStatus();
}

absl::StatusOr<size_t> RV32ITop::ReadMemory(uint64_t address, void *buffer,
                                            size_t length) {
  if (run_status_ != RunStatus::kHalted) {
    return absl::FailedPreconditionError("ReadMemory: Core must be halted");
  }
  auto *db = db_factory_.Allocate(length);
  // Load bypassing any watch points/semihosting.
  state_->memory()->Load(address, db, nullptr, nullptr);
  std::memcpy(buffer, db->raw_ptr(), length);
  db->DecRef();
  return length;
}

absl::StatusOr<size_t> RV32ITop::WriteMemory(uint64_t address,
                                             const void *buffer,
                                             size_t length) {
  if (run_status_ != RunStatus::kHalted) {
    return absl::FailedPreconditionError("WriteMemory: Core must be halted");
  }
  auto *db = db_factory_.Allocate(length);
  std::memcpy(db->raw_ptr(), buffer, length);
  // Store bypassing any watch points/semihosting.
  state_->memory()->Store(address, db);
  db->DecRef();
  return length;
}

bool RV32ITop::HasBreakpoint(uint64_t address) {
  return rv_bp_manager_->HasBreakpoint(address);
}

absl::Status RV32ITop::SetSwBreakpoint(uint64_t address) {
  // Don't try if the simulator is running.
  if (run_status_ != RunStatus::kHalted) {
    return absl::FailedPreconditionError(
        "SetSwBreakpoint: Core must be halted");
  }
  // If there is no breakpoint manager, return an error.
  if (rv_bp_manager_ == nullptr) {
    return absl::InternalError("Breakpoints are not enabled");
  }
  // Try setting the breakpoint.
  return rv_bp_manager_->SetBreakpoint(address);
}

absl::Status RV32ITop::ClearSwBreakpoint(uint64_t address) {
  // Don't try if the simulator is running.
  if (run_status_ != RunStatus::kHalted) {
    return absl::FailedPreconditionError(
        "ClearSwBreakpoing: Core must be halted");
  }
  if (rv_bp_manager_ == nullptr) {
    return absl::InternalError("Breakpoints are not enabled");
  }
  return rv_bp_manager_->ClearBreakpoint(address);
}

absl::Status RV32ITop::ClearAllSwBreakpoints() {
  // Don't try if the simulator is running.
  if (run_status_ != RunStatus::kHalted) {
    return absl::FailedPreconditionError(
        "ClearAllSwBreakpoints: Core must be halted");
  }
  if (rv_bp_manager_ == nullptr) {
    return absl::InternalError("Breakpoints are not enabled");
  }
  rv_bp_manager_->ClearAllBreakpoints();
  return absl::OkStatus();
}

absl::StatusOr<Instruction *> RV32ITop::GetInstruction(uint64_t address) {
  auto inst = rv32_decode_cache_->GetDecodedInstruction(address);
  return inst;
}

absl::StatusOr<std::string> RV32ITop::GetDisassembly(uint64_t address) {
  // Don't try if the simulator is running.
  if (run_status_ != RunStatus::kHalted) {
    return absl::FailedPreconditionError("GetDissasembly: Core must be halted");
  }

  Instruction *inst;
  // If requesting the disassembly for an instruction at a breakpoint, return
  // that of the original instruction instead.
  if (rv_bp_manager_->IsBreakpoint(address)) {
    auto bp_pc = address;
    // Disable the breakpoint.
    auto status = rv_bp_manager_->DisableBreakpoint(bp_pc);
    if (!status.ok()) return status;
    // Get the real instruction.
    inst = rv32_decode_cache_->GetDecodedInstruction(bp_pc);
    // Re-enable the breakpoint.
    status = rv_bp_manager_->EnableBreakpoint(bp_pc);
    if (!status.ok()) return status;
  } else {
    // If not at the breakpoint, or requesting a different instruction,
    inst = rv32_decode_cache_->GetDecodedInstruction(address);
  }
  return inst != nullptr ? inst->AsString() : "Invalid instruction";
}

absl::Status RV32ITop::SetUpSemiHosting(const SemiHostAddresses &magic) {
  // Don't try if the simulator is running.
  if (run_status_ != RunStatus::kHalted) {
    return absl::FailedPreconditionError(
        "SetupSemihosting: Core must be halted");
  }
  watcher_ = new util::MemoryWatcher(memory_);
  rv32_semihost_ = new RiscV32HtifSemiHost(
      watcher_, memory_, magic,
      [this]() { RequestHalt(HaltReason::kSemihostHaltRequest, nullptr); },
      [this](std::string) {
        RequestHalt(HaltReason::kSemihostHaltRequest, nullptr);
      });
  state_->set_memory(watcher_);
  return absl::OkStatus();
}

void RV32ITop::RequestHalt(HaltReason halt_reason, const Instruction *inst) {
  // First set the halt_reason_, then the half flag.
  halt_reason_ = halt_reason;
  halted_ = true;
}

}  // namespace codelab
}  // namespace sim
}  // namespace mpact
