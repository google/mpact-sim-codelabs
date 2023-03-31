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

#ifndef MPACT_SIM_CODELABS_FULL_DECODER_RISCV32I_ENCODING_H_
#define MPACT_SIM_CODELABS_FULL_DECODER_RISCV32I_ENCODING_H_

#include <functional>
#include <string>

#include "absl/functional/any_invocable.h"
#include "mpact/sim/generic/operand_interface.h"
#include "other/riscv_simple_state.h"
#include "riscv_isa_decoder/solution/riscv32i_decoder.h"
#include "riscv_isa_decoder/solution/riscv32i_enums.h"

namespace mpact {
namespace sim {
namespace codelab {

using ::mpact::sim::generic::DestinationOperandInterface;
using ::mpact::sim::generic::PredicateOperandInterface;
using ::mpact::sim::generic::SourceOperandInterface;

// Exercise 2.

}  // namespace codelab
}  // namespace sim
}  // namespace mpact

#endif  // MPACT_SIM_CODELABS_FULL_DECODER_RISCV32I_ENCODING_H_
