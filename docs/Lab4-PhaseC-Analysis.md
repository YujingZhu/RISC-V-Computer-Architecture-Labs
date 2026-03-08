以下是为您整理好的 **实验三：基准运行与性能数据分析 (Phase C)** 的 README 文档。

---

# 实验三：基准运行与性能数据分析 (README)

## 1. 实验目的

本阶段实验旨在对 HBirdv2 E203 处理器进行系统性的性能评估。通过多次独立运行 CoreMark 基准测试，结合实验二搭建的性能计数器框架，统计分析处理器的 CPI (Cycles Per Instruction)、IPC (Instructions Per Cycle) 及稳定性。核心目标是**识别流水线瓶颈**，为后续的微结构优化（Phase D）提供理论依据和对比基线。

## 2. 实验环境与配置

* **硬件**: Nuclei DDR200T FPGA Evaluation Kit
* **核心**: HBirdv2 E203 (默认配置)
* **主频**: ~16.00 MHz
* **编译器优化**: `-O2 -funroll-all-loops -finline-limit=600 ...` (全循环展开与内联)
* **测试规模**: CoreMark Iterations = 4000
* **采样方法**: 独立运行 5 次，记录 `mcycle` 与 `minstret`

## 3. 实验数据统计

对 5 次独立运行的日志数据进行汇总与统计分析。

### 3.1 原始数据记录表

| 运行次序 (Run #) | Total Cycles (`mcycle`) | Retired Insts (`minstret`) | IPC | CPI | CoreMark/MHz |
| --- | --- | --- | --- | --- | --- |
| Run 1 | 1,756,951,756 | 917,399,219 | 0.522 | 1.915 | 2.277 |
| Run 2 | 1,756,951,756 | 917,399,219 | 0.522 | 1.915 | 2.277 |
| Run 3 | 1,756,951,756 | 917,399,219 | 0.522 | 1.915 | 2.277 |
| Run 4 | 1,756,951,756 | 917,399,219 | 0.522 | 1.915 | 2.277 |
| Run 5 | 1,756,951,756 | 917,399,219 | 0.522 | 1.915 | 2.277 |

### 3.2 统计分析结果

由于 5 次运行结果在周期级完全一致，表明实验环境具有极高的确定性（Deterministic）。

| 统计指标 | 均值 (Mean) | 标准差 (Std Dev, ) | 结果说明 |
| --- | --- | --- | --- |
| **CPI** | **1.915144** | **0.000** | 性能极其稳定，无抖动 |
| **IPC** | **0.522154** | **0.000** | 同上 |
| **CoreMark Score** | **36.426** | **0.000** | Iterations/Sec |
| **CM/MHz** | **2.277** | **0.000** | 归一化得分 |

## 4. 性能瓶颈深入分析 (Bottleneck Analysis)

### 4.1 CPI 现状分析

当前测得的 **CPI  1.915**，意味着处理器平均需要近 2 个时钟周期才能退休一条指令。对于标量 RISC-V 处理器，理想 CPI 应接近 1.0。当前近 50% 的性能损失（1.0 vs 1.9）主要源于流水线停顿。

### 4.2 瓶颈归因推断 (Root Cause)

结合 CoreMark 代码特征（大量指针追踪、矩阵运算）与 E203 的两级流水线结构，主要瓶颈如下：

1. **Load-Use 冒险 (Load-Use Hazards) - [主要矛盾]**
* **现象**：CoreMark 中包含大量的链表遍历操作（如 `ptr = ptr->next`）。
* **原理**：在 E203 架构中，LSU（访存单元）加载数据需要延迟。如果紧接着的指令需要使用该数据（RAW 依赖），流水线必须停顿（Stall）以等待数据从存储器返回。
* **数据佐证**：CPI 远大于 1，且未达到 3（通常分支预测失败代价更高），暗示存在大量短周期的 Load-Use 停顿。


2. **控制冒险 (Control Hazards)**
* **现象**：CoreMark 包含复杂的状态机和循环。
* **原理**：E203 采用静态分支预测（或简单的 Not-Taken 策略）。一旦预测失败，需要冲刷流水线，造成周期浪费。
* **影响**：虽然编译器使用了 `-funroll-all-loops`（循环展开）来减少分支指令的数量，但状态判断逻辑依然存在，贡献了部分 CPI 损失。


3. **结构/长延迟指令**
* 乘除法指令（MUL/DIV）在 E203 中不是单周期的，会阻塞流水线，导致 IPC 下降。



## 5. 结论与优化方向

1. **基线确立**：我们成功确立了 **CPI = 1.915** 为待优化的基线（Baseline）。任何后续优化必须使 CPI 低于此值。
2. **系统稳定性**：5 次运行的标准差为 0，证明了实验平台的可靠性，无需担心随机噪声干扰优化结果的验证。
3. **优化策略 (Phase D)**：
* 鉴于 Load-Use 是两级流水线的主要痛点，下一阶段将优先尝试 **数据旁路 (Data Forwarding)** 优化。
* **目标**：通过在 RTL 中建立从 LSU 到 EXU 的快速通路，减少 Load 指令后的等待周期，从而降低 CPI。



---

Start to run coremark for 4000 iterations

==========================================================
[Lab4 Perf Data] Cycles (mcycle) : 1756951756
[Lab4 Perf Data] Insts  (minstret): 917399219
[Lab4 Perf Data] IPC              : 0.522154
[Lab4 Perf Data] CPI              : 1.915144
==========================================================

2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 1756951730
Total time (secs): 109.812986
Iterations/Sec   : 36.425564
Iterations       : 4000
Compiler version : GCC14.2.1 20240816
Compiler flags   : -O2 -funroll-all-loops -finline-limit=600 -ftree-dominator-opts -fno-if-conversion2 -fselective-scheduling -fno-code-hoisting -fno-common -funroll-loops -finline-functions -falign-functions=4 -falign-jumps=4 -falign-loops=4
Memory location  : STACK
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x65c5
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 36.425564 / GCC14.2.1 20240816 -O2 -funroll-all-loops -finline-limit=600 -ftree-dominator-opts -fno-if-conversion2 -fselective-scheduling -fno-code-hoisting -fno-common -funroll-loops -finline-functions -falign-functions=4 -falign-jumps=4 -falign-loops=4 / STACK


Print Personal Added Addtional Info to Easy Visual Analysis

     (Iterations is: 4000
     (total_ticks is: 1756951730
 (*) Assume the core running at 1 MHz
     So the CoreMark/MHz can be caculated by: 
     (Iterations*1000000/total_ticks) = 2.276670 CoreMark/MHz

HummingBird SDK Build Time: Jan 10 2026, 10:17:42
Download Mode: ILM
CPU Frequency 15999631 Hz
Start to run coremark for 4000 iterations

==========================================================
[Lab4 Perf Data] Cycles (mcycle) : 1756951756
[Lab4 Perf Data] Insts  (minstret): 917399219
[Lab4 Perf Data] IPC              : 0.522154
[Lab4 Perf Data] CPI              : 1.915144
==========================================================

2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 1756951730
Total time (secs): 109.812986
Iterations/Sec   : 36.425564
Iterations       : 4000
Compiler version : GCC14.2.1 20240816
Compiler flags   : -O2 -funroll-all-loops -finline-limit=600 -ftree-dominator-opts -fno-if-conversion2 -fselective-scheduling -fno-code-hoisting -fno-common -funroll-loops -finline-functions -falign-functions=4 -falign-jumps=4 -falign-loops=4
Memory location  : STACK
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x65c5
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 36.425564 / GCC14.2.1 20240816 -O2 -funroll-all-loops -finline-limit=600 -ftree-dominator-opts -fno-if-conversion2 -fselective-scheduling -fno-code-hoisting -fno-common -funroll-loops -finline-functions -falign-functions=4 -falign-jumps=4 -falign-loops=4 / STACK


Print Personal Added Addtional Info to Easy Visual Analysis

     (Iterations is: 4000
     (total_ticks is: 1756951730
 (*) Assume the core running at 1 MHz
     So the CoreMark/MHz can be caculated by: 
     (Iterations*1000000/total_ticks) = 2.276670 CoreMark/MHz

HummingBird SDK Build Time: Jan 10 2026, 10:17:42
Download Mode: ILM
CPU Frequency 15999631 Hz
Start to run coremark for 4000 iterations

==========================================================
[Lab4 Perf Data] Cycles (mcycle) : 1756951756
[Lab4 Perf Data] Insts  (minstret): 917399219
[Lab4 Perf Data] IPC              : 0.522154
[Lab4 Perf Data] CPI              : 1.915144
==========================================================

2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 1756951730
Total time (secs): 109.812986
Iterations/Sec   : 36.425564
Iterations       : 4000
Compiler version : GCC14.2.1 20240816
Compiler flags   : -O2 -funroll-all-loops -finline-limit=600 -ftree-dominator-opts -fno-if-conversion2 -fselective-scheduling -fno-code-hoisting -fno-common -funroll-loops -finline-functions -falign-functions=4 -falign-jumps=4 -falign-loops=4
Memory location  : STACK
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x65c5
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 36.425564 / GCC14.2.1 20240816 -O2 -funroll-all-loops -finline-limit=600 -ftree-dominator-opts -fno-if-conversion2 -fselective-scheduling -fno-code-hoisting -fno-common -funroll-loops -finline-functions -falign-functions=4 -falign-jumps=4 -falign-loops=4 / STACK


Print Personal Added Addtional Info to Easy Visual Analysis

     (Iterations is: 4000
     (total_ticks is: 1756951730
 (*) Assume the core running at 1 MHz
     So the CoreMark/MHz can be caculated by: 
     (Iterations*1000000/total_ticks) = 2.276670 CoreMark/MHz

HummingBird SDK Build Time: Jan 10 2026, 10:17:42
Download Mode: ILM
CPU Frequency 15999631 Hz
Start to run coremark for 4000 iterations

==========================================================
[Lab4 Perf Data] Cycles (mcycle) : 1756951756
[Lab4 Perf Data] Insts  (minstret): 917399219
[Lab4 Perf Data] IPC              : 0.522154
[Lab4 Perf Data] CPI              : 1.915144
==========================================================

2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 1756951730
Total time (secs): 109.812986
Iterations/Sec   : 36.425564
Iterations       : 4000
Compiler version : GCC14.2.1 20240816
Compiler flags   : -O2 -funroll-all-loops -finline-limit=600 -ftree-dominator-opts -fno-if-conversion2 -fselective-scheduling -fno-code-hoisting -fno-common -funroll-loops -finline-functions -falign-functions=4 -falign-jumps=4 -falign-loops=4
Memory location  : STACK
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x65c5
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 36.425564 / GCC14.2.1 20240816 -O2 -funroll-all-loops -finline-limit=600 -ftree-dominator-opts -fno-if-conversion2 -fselective-scheduling -fno-code-hoisting -fno-common -funroll-loops -finline-functions -falign-functions=4 -falign-jumps=4 -falign-loops=4 / STACK


Print Personal Added Addtional Info to Easy Visual Analysis

     (Iterations is: 4000
     (total_ticks is: 1756951730
 (*) Assume the core running at 1 MHz
     So the CoreMark/MHz can be caculated by: 
     (Iterations*1000000/total_ticks) = 2.276670 CoreMark/MHz

HummingBird SDK Build Time: Jan 10 2026, 10:17:42
Download Mode: ILM
CPU Frequency 15999631 Hz
Start to run coremark for 4000 iterations

==========================================================
[Lab4 Perf Data] Cycles (mcycle) : 1756951756
[Lab4 Perf Data] Insts  (minstret): 917399219
[Lab4 Perf Data] IPC              : 0.522154
[Lab4 Perf Data] CPI              : 1.915144
==========================================================

2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 1756951730
Total time (secs): 109.812986
Iterations/Sec   : 36.425564
Iterations       : 4000
Compiler version : GCC14.2.1 20240816
Compiler flags   : -O2 -funroll-all-loops -finline-limit=600 -ftree-dominator-opts -fno-if-conversion2 -fselective-scheduling -fno-code-hoisting -fno-common -funroll-loops -finline-functions -falign-functions=4 -falign-jumps=4 -falign-loops=4
Memory location  : STACK
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x65c5
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 36.425564 / GCC14.2.1 20240816 -O2 -funroll-all-loops -finline-limit=600 -ftree-dominator-opts -fno-if-conversion2 -fselective-scheduling -fno-code-hoisting -fno-common -funroll-loops -finline-functions -falign-functions=4 -falign-jumps=4 -falign-loops=4 / STACK


Print Personal Added Addtional Info to Easy Visual Analysis

     (Iterations is: 4000
     (total_ticks is: 1756951730
 (*) Assume the core running at 1 MHz
     So the CoreMark/MHz can be caculated by: 
     (Iterations*1000000/total_ticks) = 2.276670 CoreMark/MHz
