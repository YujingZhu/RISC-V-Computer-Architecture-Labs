# RISC-V Computer Architecture Labs

A series of hands-on lab experiments exploring RISC-V processor architecture on the **Nuclei DDR200T FPGA** evaluation board with the **HBirdv2 E203** (RV32IMAC) SoC.

## Hardware Platform

| Component | Specification |
|-----------|--------------|
| FPGA | Xilinx XC7A200T (DDR200T Board) |
| Processor | HBirdv2 E203 (RV32IMAC) |
| Clock | 16 MHz |
| UART | 115200 baud, 8N1 |
| IDE | Nuclei Studio (Eclipse-based) |
| Toolchain | GCC RISC-V |

## Lab Overview

### [Lab1 - Assembly Programming](Lab1-Assembly/)
Introduction to RISC-V assembly and C/Assembly mixed programming. Implements a `sum_to_n` function in RISC-V assembly called from C.

### [Lab2 - MMIO & GPIO Driver](Lab2-MMIO/)
Memory-Mapped I/O programming with bare-metal GPIO and UART drivers.
- GPIO initialization (IOFCFG, PADDIR, PADOUT)
- LED running light (3-round sweep with RMW protection)
- Switch-to-LED linkage with real-time polling
- UART status register access

### [Lab3 - Interrupt Handling](Lab3-Interrupts/)
Progressive exploration of interrupt mechanisms on RISC-V:

| Task | Mode | Description |
|------|------|-------------|
| **Task1** | Polling | Edge detection via GPIO polling, UART echo, cycle-accurate performance measurement |
| **Task2** | Trap Framework | MSI (Machine Software Interrupt) with assembly trap entry, CSR configuration (mtvec/mie/mstatus) |
| **Task3** | Interrupt-Driven | Full interrupt system with PLIC, UART RX interrupt, performance statistics, CPU utilization tracking |

Key metrics from Task1 polling mode:
- Poll cycle: 56 CPU cycles
- Average response latency: 28 cycles
- CPU busy ratio: 99%

### [Lab4 - CoreMark Performance Analysis](Lab4-CoreMark/)
Performance benchmarking and RTL optimization of the E203 processor pipeline.

| Phase | Focus | Key Result |
|-------|-------|------------|
| **A** | Baseline CoreMark | Score: 36.436 (2.277 CM/MHz) |
| **B** | Instrumentation | IPC = 0.522, CPI = 1.915 |
| **C** | Statistical Analysis | 5-run deterministic validation (std dev = 0) |
| **D** | RTL Optimization | Pipeline hazard analysis, Verilog modifications to `e203_exu.v` |

## Project Structure

```
.
├── Lab1-Assembly/
│   └── src/
│       ├── main.c              # C entry point
│       └── Sum.S               # RISC-V assembly (sum_to_n)
├── Lab2-MMIO/
│   └── src/
│       ├── main.c              # GPIO & UART driver
│       └── lab2_mmio.h         # MMIO register definitions
├── Lab3-Interrupts/
│   ├── Task1-Polling/
│   │   └── src/                # Polling mode implementation
│   ├── Task2-TrapFramework/
│   │   └── src/                # MSI trap handler with asm entry
│   └── Task3-InterruptDriven/
│       └── src/                # Full interrupt system with stats
├── Lab4-CoreMark/
│   ├── src/                    # CoreMark benchmark (EEMBC)
│   └── rtl/                    # Verilog RTL modifications
│       ├── config.v            # Processor configuration
│       └── e203_exu.v          # Execution unit (pipeline opt)
├── docs/                       # Detailed lab documentation
├── reports/                    # Experiment reports & guides
│   ├── Lab1~4-实验报告.pdf      # Final experiment reports (4)
│   ├── guides/                 # Lab guidance documents (4)
│   ├── diagrams/               # Architecture diagrams & screenshots
│   │   ├── Lab1/               # 框架图, 实验截图, 报告截图
│   │   ├── Lab2/               # 架构图, GPIO/UART 截图
│   │   ├── Lab3/               # 架构图, Task1~3 截图, 代码截图
│   │   └── Lab4/               # 数据通路, 流水线, 性能对比图
│   └── drafts/                 # Report drafts (v1.0, v2.0)
└── references/                 # Reference documents
    ├── MCS参考官方文档流程.pdf
    └── RISC-V源开源项目仿真.docx
```

## Build & Run

### Prerequisites
- [Nuclei Studio IDE](https://nucleisys.com/download.php)
- RISC-V GCC Toolchain (bundled with Nuclei Studio)
- Nuclei DDR200T FPGA Board + JTAG debugger

### Steps
1. Import the project into Nuclei Studio
2. Select the target memory layout (ILM recommended for debugging)
3. Build the project (GCC RISC-V cross-compiler)
4. Flash via JTAG and connect UART terminal (115200 8N1)

### SDK Dependency
This project depends on [HBird SDK](https://github.com/riscv-mcu/hbird-sdk) for:
- NMSIS Core headers (CSR access, interrupt management)
- SoC/Board support package (startup, linker scripts, drivers)

## Technologies

- **ISA**: RISC-V RV32IMAC
- **Languages**: C, RISC-V Assembly, Verilog
- **Concepts**: MMIO, GPIO, UART, Interrupt handling (PLIC/CLINT), Trap mechanism, Pipeline hazards, CoreMark benchmarking

## License

This project is for educational purposes. CoreMark is a benchmark by [EEMBC](https://www.eembc.org/coremark/).
