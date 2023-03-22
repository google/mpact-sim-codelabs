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

#ifndef MPACT_SIM_CODELABS_SEMANTIC_FUNCTIONS_SOLUTION_RV32I_INSTRUCTIONS_H_
#define MPACT_SIM_CODELABS_SEMANTIC_FUNCTIONS_SOLUTION_RV32I_INSTRUCTIONS_H_

#include "mpact/sim/generic/instruction.h"

// This file contains the declarations of the instruction semantic functions
// for the RiscV 32i instructions.

namespace mpact {
namespace sim {
namespace codelab {

using ::mpact::sim::generic::Instruction;

//
// All semantic functions must have the signature void(Instruction *).
//

// For the following, source operand 0 refers to the register specified in rs1,
// and source operand 1 refers to either the register specified in rs2, or the
// immediate. Destination operand 0 refers to the register specified in rd.

// Semantic functions for Exercise 2.
void RV32IAdd(Instruction *instruction);
void RV32IAnd(Instruction *instruction);
void RV32IOr(Instruction *instruction);
void RV32ISll(Instruction *instruction);
void RV32ISltu(Instruction *instruction);
void RV32ISub(Instruction *instruction);
void RV32ISra(Instruction *instruction);
void RV32ISrl(Instruction *instruction);
void RV32IXor(Instruction *instruction);
// End semantic functions for Exercise 2.

// For the following, source operand 0 refers to the 20-bit immediate value,
// already shifted left by 12 to form a 32-bit immediate.

// Semantic functions for Exercise 3.
void RV32ILui(Instruction *instruction);
void RV32IAuipc(Instruction *instruction);
// End semantic functions for Exercise 3.

// For the following branch instructions. Source operand 0 refers to the
// register specified by rs1, source operand 2 refers to the register specified
// by rs2, and source operand 3 refers to the immediate offset. Destination
// operand 0 refers to the pc destination operand.

// Semantic functions for Exercise 4 - branches.
void RV32IBeq(Instruction *instruction);
void RV32IBge(Instruction *instruction);
void RV32IBgeu(Instruction *instruction);
void RV32IBlt(Instruction *instruction);
void RV32IBltu(Instruction *instruction);
void RV32IBne(Instruction *instruction);
// End semantic functions for Exercise 4 - branches.

// Source operand 0 contains the immediate value. Destination operand 0 refers
// to the pc destination operand, wheras destination operand 1 refers to the
// link register specified in rd.

// Semantic function for Exercise 4 - jal.
void RV32IJal(Instruction *instruction);
// End semantic function for Exercise 4 - jal.

// Source operand 0 refers to the base registers specified by rs1, source
// operand 1 contains the immediate value. Destination operand 0 refers to the
// pc destination operand, wheras destination operand 1 refers to the
// link register specified in rd.

// Semantic function for Exercise 4 - jalr.
void RV32IJalr(Instruction *instruction);
// End semantic functions for Exercise 4 - jalr.

// For each store instruction semantic function, source operand 0 is the base
// register, source operand 1 is the offset, while source operand 2 is the value
// to be stored referred to by rs2.

// Semantic functions for Exercise 5.
void RV32ISb(Instruction *instruction);
void RV32ISh(Instruction *instruction);
void RV32ISw(Instruction *instruction);
// End semantic functions for Exercise 5.

// Each of the load instructions are modeled by a pair of semantic instruction
// functions. The "main" function computes the effective address and initiates
// the load, the "child" function processes the load result and writes it back
// to the destination register.
// For the "main" semantic function, source operand 0 is the base register,
// source operand 1 the offset. Destination operand 0 is the register specified
// by rd. The "child" semantic function will get a copy of the destination
// operand.

// Semantic functions for Exercise 6.
void RV32ILb(Instruction *instruction);
void RV32ILbChild(Instruction *instruction);
void RV32ILbu(Instruction *instruction);
void RV32ILbuChild(Instruction *instruction);
void RV32ILh(Instruction *instruction);
void RV32ILhChild(Instruction *instruction);
void RV32ILhu(Instruction *instruction);
void RV32ILhuChild(Instruction *instruction);
void RV32ILw(Instruction *instruction);
void RV32ILwChild(Instruction *instruction);
// End semantic functions for Exercise 6.

// Exercises End.

// The Fence instruction takes a single source operand (index 0) which consists
// of an immediate value containing the right justified concatenation of the FM,
// predecessor, and successor bit fields of the instruction.
void RV32IFence(Instruction *instruction);

// Software breakpoint instruction.
void RV32IEbreak(Instruction *instruction);

void RV32IllegalInstruction(Instruction *inst);

}  // namespace codelab
}  // namespace sim
}  // namespace mpact

#endif  // MPACT_SIM_CODELABS_SEMANTIC_FUNCTIONS_SOLUTION_RV32I_INSTRUCTIONS_H_
