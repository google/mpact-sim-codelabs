# Copyright 2023 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Build file for the bin_decoder part of the RiscV instruction decoder codelab.

load("@mpact-sim//mpact/sim/decoder:mpact_sim_isa.bzl", "mpact_bin_fmt_decoder")

package(
    default_applicable_licenses = ["//:license"],
    default_visibility = ["//visibility:public"],
)

mpact_bin_fmt_decoder(
    name = "riscv32i_bin_fmt",
    src = "riscv32i.bin_fmt",
    decoder_name = "RiscV32I",
    includes = [],
    deps = [
        "//riscv_isa_decoder/solution:riscv32i_isa",
    ],
)
