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

#ifndef MPACT_SIM_CODELABS_FULL_DECODER_RISCV32_DECODER_H_
#define MPACT_SIM_CODELABS_FULL_DECODER_RISCV32_DECODER_H_

#include <memory>

#include "mpact/sim/generic/arch_state.h"
#include "mpact/sim/generic/decoder_interface.h"
#include "mpact/sim/generic/instruction.h"
#include "mpact/sim/generic/program_error.h"
#include "mpact/sim/util/memory/memory_interface.h"
#include "riscv/riscv_state.h"
#include "riscv_full_decoder/riscv32i_encoding.h"
#include "riscv_isa_decoder/solution/riscv32i_decoder.h"

namespace mpact {
namespace sim {
namespace codelab {

using riscv::RiscVState;

// Exercise 1 - step 1. Fill in the factory class definition.
class RiscV32IsaFactory;

// Exercise 1 - step 2. Fill in the decoder class definition.
class RiscV32Decoder;

}  // namespace codelab
}  // namespace sim
}  // namespace mpact

#endif  // MPACT_SIM_CODELABS_FULL_DECODER_RISCV32_DECODER_H_
