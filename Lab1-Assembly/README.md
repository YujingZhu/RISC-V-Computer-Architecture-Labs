# Lab1 - RISC-V Assembly Programming

## Objective
Learn RISC-V assembly basics and C/Assembly interoperability.

## Implementation
- `Sum.S`: RISC-V assembly function `sum_to_n(n)` that computes 1 + 2 + ... + n
  - Uses registers `t0` (accumulator) and `t1` (counter)
  - Loop with `bgt` branch condition
  - Returns result via `a0` register
- `main.c`: C entry point that calls the assembly function

## Key Concepts
- RISC-V calling convention (argument in `a0`, return in `a0`)
- Register usage (`t0`-`t6` for temporaries)
- Branch instructions (`bgt`, `addi`)
- Mixed C/Assembly linking with `extern`
