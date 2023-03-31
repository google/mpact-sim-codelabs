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

# riscv_full_decoder codelab solution project

package(
    default_applicable_licenses = ["//:license"],
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "riscv32i_decoder",
    srcs = [
        "riscv32_decoder.cc",
        "riscv32i_encoding.cc",
    ],
    hdrs = [
        "riscv32_decoder.h",
        "riscv32i_encoding.h",
    ],
    deps = [
        "//other:riscv_simple_state",
        "//riscv_bin_decoder/solution:riscv32i_bin_fmt",
        "//riscv_isa_decoder/solution:riscv32i_isa",
        "//riscv_semantic_functions/solution:riscv32i",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/functional:bind_front",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@mpact-sim//mpact/sim/generic:arch_state",
        "@mpact-sim//mpact/sim/generic:core",
        "@mpact-sim//mpact/sim/generic:instruction",
        "@mpact-sim//mpact/sim/generic:program_error",
        "@mpact-sim//mpact/sim/util/memory",
    ],
)
