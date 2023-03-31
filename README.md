# MPACT-Sim Codelabs

This repoistory contains the codelab exercises for the
[MPACT-Sim codelabs](https://developers.google.com/mpact-sim).
These codelabs provide a tutorials on how to get started using MPACT-Sim to
create instruction set simulators.

The codelabs guide you to write most of the required code to build an
instruction set simulator for the RiscV 32I instruction set (integer
instructions only). 

There are four directories that contain code and build targets for the coding
exercises :

*   `riscv_isa_decoder` <br />
    This directory contains the skeleton and solution for writing the
    description file for the encoding independent instruction decoder.

*   `riscv_bin_decoder` <br />
    This directory contains the skeleton and solution for writing the
    description file for the binary decoder.

*   `riscv_semantic_functions` <br />
    This directory contains the skeleton and solution for writing the
    semantic functions that implement the instructions in the codelab.

*   `riscv_full_decoder` <br />
    This directory contains the skeleton and solution for writing the full
    instruction decoder that integrates the decoders that were generated in
    prior exercises.

Additionally, there is a directory `other`, that contains support code that
is not part of the exercises, but allows a finished simulator to be built
and executed. A sample "Hellow World" executable is also provided.

