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

#ifndef MPACT_SIM_CODELABS_OTHER_RISCV_REGISTER_H_
#define MPACT_SIM_CODELABS_OTHER_RISCV_REGISTER_H_

#include <any>
#include <string>
#include <vector>

#include "mpact/sim/generic/data_buffer.h"
#include "mpact/sim/generic/operand_interface.h"
#include "mpact/sim/generic/register.h"
#include "mpact/sim/generic/state_item.h"

// File contains shorthand type definitions for RiscV32G registers.

namespace mpact {
namespace sim {
namespace riscv {

class RiscVState;

// The value type of the register must be an unsigned integer type.
using RV32Register = generic::Register<uint32_t>;
using RV64Register = generic::Register<uint64_t>;

using RVXRegister = RV32Register;

}  // namespace riscv
}  // namespace sim
}  // namespace mpact

#endif // MPACT_SIM_CODELABS_OTHER_RISCV_REGISTER_H_
