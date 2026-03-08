# Lab3 - Interrupt Handling on RISC-V

## Objective
Progressively implement and analyze interrupt mechanisms: from polling to full interrupt-driven I/O.

## Tasks

### Task1 - Polling Mode
Simple polling loop with UART echo and GPIO edge detection.
- Measures polling cycle time via `mcycle`/`mcycleh` CSRs
- **Performance**: T_poll = 56 cycles, avg response = 28 cycles, CPU busy = 99%

### Task2 - Trap Framework & MSI
Software interrupt (MSI) handling with custom trap entry.
- Assembly trap entry (`lab3_trap_entry.S`): 16-register context save/restore + `mret`
- CSR configuration: `mtvec`, `mie`, `mstatus`
- MSI trigger via CLINT MSIP register at `0x02000000`

### Task3 - Full Interrupt System
Complete interrupt-driven I/O with PLIC and performance analytics.
- UART RX interrupt via PLIC
- ISR performance: 93-178 cycles execution, 59-60 cycles latency (~3.7 us)
- CPU idle rate: 99%+ (vs 99% busy in polling mode)
- Interactive commands: `t`=trigger MSI, `s`=stats, `r`=reset, `a`=stress test

## File Structure
```
Task1-Polling/src/
├── main.c           # Polling loop with UART echo
└── lab3_mmio.h      # MMIO register definitions

Task2-TrapFramework/src/
├── main.c           # MSI trigger & test
├── lab3_trap.c      # Trap init & handler (C)
├── lab3_trap.h      # Trap function declarations
├── lab3_trap_entry.S # Assembly trap entry point
└── lab3_mmio.h

Task3-InterruptDriven/src/
├── lab3_main.c      # Main loop with interactive commands
├── lab3_trap.c      # Trap handlers (MSI + UART RX)
├── lab3_trap.h
├── lab3_trap.S       # Assembly trap entry (64-bit save)
├── lab3_trap_entry.S
├── lab3_stats.c     # Performance statistics tracking
├── lab3_stats.h
├── lab3_timer.c     # Timer utilities
├── lab3_timer.h
└── lab3_mmio.h
```

## Detailed Documentation
- [Task1 - Polling Mode](../docs/Lab3-Task1-Polling.md)
- [Task2 - Trap Framework](../docs/Lab3-Task2-TrapFramework.md)
- [Task3 - Interrupt-Driven](../docs/Lab3-Task3-InterruptDriven.md)
