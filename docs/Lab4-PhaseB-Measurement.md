以下为您生成的实验二完整的 `README.md` 文件内容。这份文档详细记录了性能测量框架的搭建过程、关键代码修改以及对采集到的基线数据的深入分析。

---

# 实验二：性能测量框架搭建与指标采集 (README)

## 1. 实验目的

本阶段实验旨在 CoreMark 基准测试的基础上，构建处理器微结构性能测量框架。通过在代码中植入 RISC-V 专用汇编指令，读取硬件性能计数器（CSR），采集 CPU 的**执行周期数 (Cycles)** 和 **退休指令数 (Instructions Retired)**。利用这些数据计算 **CPI (Cycles Per Instruction)** 和 **IPC (Instructions Per Cycle)**，从而定量分析 HBirdv2 E203 处理器的流水线效率及潜在瓶颈。

## 2. 实验环境

* **硬件平台**: Nuclei DDR200T FPGA Evaluation Kit
* **处理器核**: HBirdv2 E203 (RISC-V RV32IMAC 2-Stage Pipeline)
* **SDK 版本**: HummingBird SDK (Build Jan 10 2026)
* **编译器**: GCC 14.2.1 (Optimization Level: -O2)

## 3. 关键代码修改说明

为了获取精确的性能数据，修改了 `application/core_main.c` 文件。利用 RISC-V 特权架构定义的机器模式计数器寄存器 `mcycle` (Machine Cycle Counter) 和 `minstret` (Machine Instructions Retired Counter) 进行埋点采样。

### 修改文件：`application/core_main.c`

**修改原理**：
在 CoreMark 核心计时函数 `start_time()` 之前记录计数器初值，在 `stop_time()` 之后记录终值，两者相减即为 CoreMark 运行期间的精确开销。

**完整修改代码片段**：

**1. 变量定义 (在 `main` 函数开头)**

```c
/* --- [Lab4 Modification Start] 1. Define Performance Counter Variables --- */
unsigned long start_cyc, end_cyc;
unsigned long start_inst, end_inst;
/* --- [Lab4 Modification End] --- */

```

**2. 采样开始 (在 `start_time()` 之前)**

```c
/* --- [Lab4 Modification Start] 2. Read Counters Before Execution --- */
// 使用内联汇编读取 CSR 寄存器
__asm__ volatile("csrr %0, mcycle" : "=r"(start_cyc));   // 读取当前周期数
__asm__ volatile("csrr %0, minstret" : "=r"(start_inst)); // 读取当前已执行指令数
/* --- [Lab4 Modification End] --- */

start_time(); // CoreMark 原生计时开始

```

**3. 采样结束与计算 (在 `stop_time()` 之后)**

```c
stop_time(); // CoreMark 原生计时结束

/* --- [Lab4 Modification Start] 3. Read Counters After Execution & Print --- */
// 再次读取计数器
__asm__ volatile("csrr %0, mcycle" : "=r"(end_cyc));
__asm__ volatile("csrr %0, minstret" : "=r"(end_inst));

// 计算差值 (Delta)
unsigned long delta_cyc = end_cyc - start_cyc;
unsigned long delta_inst = end_inst - start_inst;

// 计算微结构指标
// IPC = Instructions / Cycles
float ipc = (float)delta_inst / delta_cyc;
// CPI = Cycles / Instructions
float cpi = (float)delta_cyc / delta_inst;

// 打印结果到串口
ee_printf("\n==========================================================\n");
ee_printf("[Lab4 Perf Data] Cycles (mcycle) : %lu\n", delta_cyc);
ee_printf("[Lab4 Perf Data] Insts  (minstret): %lu\n", delta_inst);
ee_printf("[Lab4 Perf Data] IPC              : %f\n", ipc);
ee_printf("[Lab4 Perf Data] CPI              : %f\n", cpi);
ee_printf("==========================================================\n\n");
/* --- [Lab4 Modification End] --- */

```

## 4. 实验结果数据与分析

以下数据采集自 DDR200T FPGA 平台实际运行结果（基于 4000 次迭代）。

### 4.1 测量数据汇总

| 性能指标 | 测量值 | 单位 | 说明 |
| --- | --- | --- | --- |
| **Total Cycles (mcycle)** | **1,756,951,756** | Cycles | 运行 CoreMark 消耗的总时钟周期数 |
| **Total Instructions (minstret)** | **917,399,219** | Insts | 实际退休（执行完成）的指令总数 |
| **Execution Time** | **109.813** | Sec | 总运行时间 |
| **CoreMark Score** | **36.426** | Iter/Sec | 综合跑分 |

### 4.2 微结构效率分析 (Baseline)

基于上述测量数据计算得出的核心效率指标：

* **IPC (Instructions Per Cycle) = 0.522**
* **分析**: 理想情况下单发射流水线 IPC 接近 1.0。当前 IPC 约为 0.52，意味着 CPU 平均每经过 2 个周期才能完成 1 条指令。


* **CPI (Cycles Per Instruction) = 1.915**
* **分析**: CPI 接近 2.0，表明存在大量的**流水线停顿 (Pipeline Stalls)**。
* **瓶颈推断**: E203 采用 2 级流水线（取指/执行）。
1. **Load-Use Hazard**: 加载指令（LW/LB）的数据需要从存储器返回，如果紧接的指令立即使用该数据，流水线必须停顿等待。
2. **Control Hazard**: 分支跳转指令（Branch/Jump）若跳转发生，需要冲刷流水线，造成周期浪费。
3. **Memory Latency**: ILM/DLM 或外部存储器的访问延迟。





### 4.3 串口输出日志备份

```text
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
...
Correct operation validated. See readme.txt for run and reporting rules.

```

## 5. 测量方法原理

本实验利用 RISC-V 架构定义的性能计数器 CSR 实现非侵入式测量：

1. **`csrr rd, mcycle`**: 读取自 CPU 上电以来经过的时钟周期数。该计数器反映了真实的时间流逝（以时钟为单位），包含了所有的流水线停顿、缓存缺失等待等开销。
2. **`csrr rd, minstret`**: 读取自 CPU 上电以来成功退休（Retire）的指令数。该计数器不包含推测执行错误或被冲刷掉的指令，反映了程序的有效工作量。

通过在测试负载前后读取差值，能够精确剥离出测试代码本身的硬件行为特征，避免了操作系统或调试器干扰。

## 6. 结论

实验二成功搭建了微结构性能测量框架，并获取了 E203 处理器的 Baseline 性能指纹。**CPI = 1.915** 的结果明确指出了当前架构存在显著的流水线效率瓶颈。这为后续实验（流水线优化与数据旁路设计）提供了量化的优化目标——即通过硬件改进降低 CPI，使其向 1.0 逼近。