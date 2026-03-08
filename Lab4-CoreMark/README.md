# Lab4 - CoreMark Performance Analysis

## Objective
Benchmark and optimize the HBirdv2 E203 RISC-V processor using CoreMark.

## Phases

### Phase A - Baseline Setup
- Configured CoreMark with `ITERATIONS = 4000` (runtime > 10s)
- **Result**: CoreMark Score = 36.436, CoreMark/MHz = 2.277

### Phase B - Performance Instrumentation
- Added inline assembly to read `mcycle` and `minstret` CSRs
- **Result**: IPC = 0.522, CPI = 1.915

### Phase C - Statistical Validation
- 5 independent runs with identical results (std dev = 0)
- Deterministic execution confirmed (bare-metal on FPGA, no OS jitter)
- Bottleneck analysis: Load-Use hazards > Control hazards > MUL/DIV latency

### Phase D - RTL Optimization
- Modified `e203_exu.v` for pipeline optimization
- Target: Data forwarding (bypass) from LSU to EXU to reduce Load-Use stalls

## Key Results

| Metric | Value |
|--------|-------|
| CoreMark Score | 36.436 |
| CoreMark/MHz | 2.277 |
| Total Cycles | 1,756,951,756 |
| Total Instructions | 917,399,219 |
| IPC | 0.522 |
| CPI | 1.915 |

## File Structure
```
src/                    # CoreMark benchmark (EEMBC)
├── core_main.c         # Main benchmark driver (instrumented)
├── core_list_join.c    # Linked list operations
├── core_matrix.c       # Matrix operations
├── core_state.c        # State machine operations
├── core_util.c         # Utility functions
├── core_portme.c       # Platform porting layer
├── core_portme.h       # Platform config (ITERATIONS=4000)
└── coremark.h          # CoreMark definitions

rtl/                    # Verilog RTL modifications
├── config.v            # Processor configuration
└── e203_exu.v          # Execution unit (pipeline optimization)
```

## Detailed Documentation
- [Phase A - Baseline](../docs/Lab4-PhaseA-Baseline.md)
- [Phase B - Measurement](../docs/Lab4-PhaseB-Measurement.md)
- [Phase C - Analysis](../docs/Lab4-PhaseC-Analysis.md)
- [Phase D - Optimization](../docs/Lab4-PhaseD-Optimization.md)
