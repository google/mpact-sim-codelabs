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

#include "riscv_full_decoder/riscv32i_encoding.h"

#include <string>

#include "riscv_bin_decoder/solution/riscv32i_bin_decoder.h"
#include "mpact/sim/generic/devnull_operand.h"
#include "mpact/sim/generic/immediate_operand.h"
#include "mpact/sim/generic/literal_operand.h"
#include "mpact/sim/generic/operand_interface.h"
#include "other/riscv_simple_state.h"
#include "other/riscv_register.h"

namespace mpact {
namespace sim {
namespace codelab {

using riscv::RiscVState;
using riscv::RV32Register;

using generic::DevNullOperand;
using generic::ImmediateOperand;
using generic::IntLiteralOperand;

// Exercise 2.

}  // namespace codelab
}  // namespace sim
}  // namespace mpact
