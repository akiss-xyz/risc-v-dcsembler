#!/usr/bin/env bash
set -euxo pipefail # Exit on command failure and print all commands as they are executed.

riscv64-elf-gcc -march=rv32i -mabi=ilp32 -nostdlib -fno-builtin "$1" -o "$1.risv64-elf-gcc.out"
riscv64-elf-objdump -d "$1.risv64-elf-gcc.out"
