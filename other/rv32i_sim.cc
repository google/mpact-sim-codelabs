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

#include <signal.h>

#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/log.h"
#include "absl/log/check.h"
#include "absl/strings/str_cat.h"
#include "mpact/sim/generic/data_buffer.h"
#include "mpact/sim/proto/component_data.pb.h"
#include "mpact/sim/util/memory/memory_watcher.h"
#include "mpact/sim/util/program_loader/elf_program_loader.h"
#include "other/rv32i_top.h"
#include "riscv/debug_command_shell.h"
#include "riscv/riscv32_htif_semihost.h"
#include "src/google/protobuf/text_format.h"

using ::mpact::sim::proto::ComponentData;
using ::mpact::sim::riscv::RiscV32HtifSemiHost;
using AddressRange = mpact::sim::util::MemoryWatcher::AddressRange;

// Flags for specifying interactive mode.
ABSL_FLAG(bool, i, false, "Interactive mode");
ABSL_FLAG(bool, interactive, false, "Interactive mode");
// Flag for destination directory of proto file.
ABSL_FLAG(std::string, output_dir, "", "Output directory");

// Static pointer to the top instance. Used by the control-C handler.
static mpact::sim::codelab::RV32ITop *top = nullptr;

// Control-c handler to interrupt any running simulation.
static void sim_sigint_handler(int arg) {
  if (top != nullptr) {
    (void)top->Halt();
    return;
  } else {
    exit(-1);
  }
}

// Helper function to get the magic semihosting addresses from the loader.
static bool GetMagicAddresses(mpact::sim::util::ElfProgramLoader *loader,
                              RiscV32HtifSemiHost::SemiHostAddresses *magic) {
  auto result = loader->GetSymbol("tohost_ready");
  if (!result.ok()) return false;
  magic->tohost_ready = result.value().first;

  result = loader->GetSymbol("tohost");
  if (!result.ok()) return false;
  magic->tohost = result.value().first;

  result = loader->GetSymbol("fromhost_ready");
  if (!result.ok()) return false;
  magic->fromhost_ready = result.value().first;

  result = loader->GetSymbol("fromhost");
  if (!result.ok()) return false;
  magic->fromhost = result.value().first;

  return true;
}

int main(int argc, char **argv) {
  auto arg_vec = absl::ParseCommandLine(argc, argv);

  if (arg_vec.size() > 2) {
    std::cerr << "Only a single input file allowed" << std::endl;
    return -1;
  }
  std::string full_file_name = arg_vec[1];
  std::string file_name =
      full_file_name.substr(full_file_name.find_last_of('/') + 1);
  std::string file_basename = file_name.substr(0, file_name.find_first_of('.'));

  mpact::sim::codelab::RV32ITop rv32i_top("RV32I");

  // Set up control-c handling.
  top = &rv32i_top;
  struct sigaction sa;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask, SIGINT);
  sa.sa_handler = &sim_sigint_handler;
  sigaction(SIGINT, &sa, nullptr);

  // Load the elf segments into memory.
  mpact::sim::util::ElfProgramLoader elf_loader(rv32i_top.memory());
  auto load_result = elf_loader.LoadProgram(full_file_name);
  if (!load_result.ok()) {
    std::cerr << "Error while loading '" << full_file_name
              << "': " << load_result.status().message();
  }

  // Initialize the PC to the entry point.
  uint32_t entry_point = load_result.value();
  auto pc_write = rv32i_top.WriteRegister("pc", entry_point);
  if (!pc_write.ok()) {
    std::cerr << "Error writing to pc: " << pc_write.message();
  }

  // Set up semihosting.
  RiscV32HtifSemiHost::SemiHostAddresses magic_addresses;
  if (GetMagicAddresses(&elf_loader, &magic_addresses)) {
    auto status = rv32i_top.SetUpSemiHosting(magic_addresses);
    if (!status.ok()) {
      std::cerr << "Failed to set up semihosting\n";
      exit(-1);
    }
  }

  // Determine if this is being run interactively or as a batch job.
  bool interactive = absl::GetFlag(FLAGS_i) || absl::GetFlag(FLAGS_interactive);
  if (interactive) {
    mpact::sim::riscv::DebugCommandShell cmd_shell({{&rv32i_top, &elf_loader}});
    cmd_shell.Run(std::cin, std::cout);
  } else {
    std::cerr << "Starting simulation\n";

    auto run_status = rv32i_top.Run();
    if (!run_status.ok()) {
      std::cerr << run_status.message() << std::endl;
    }

    auto wait_status = rv32i_top.Wait();
    if (!wait_status.ok()) {
      std::cerr << wait_status.message() << std::endl;
    }

    std::cerr << "Simulation done\n";
  }

  // Export counters.
  auto component_proto = std::make_unique<ComponentData>();
  CHECK_OK(rv32i_top.Export(component_proto.get())) << "Failed to export proto";
  std::string proto_file_name;
  if (FLAGS_output_dir.CurrentValue().empty()) {
    proto_file_name = "./" + file_basename + ".proto";
  } else {
    proto_file_name =
        FLAGS_output_dir.CurrentValue() + "/" + file_basename + ".proto";
  }
  std::fstream proto_file(proto_file_name.c_str(), std::ios_base::out);
  std::string serialized;
  if (!proto_file.good() || !google::protobuf::TextFormat::PrintToString(
                                *component_proto.get(), &serialized)) {
    LOG(ERROR) << "Failed to write proto to file";
  } else {
    proto_file << serialized;
    proto_file.close();
  }

}
