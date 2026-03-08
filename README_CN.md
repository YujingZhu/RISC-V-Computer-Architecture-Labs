<p align="center">
  <h1 align="center">RISC-V 计算机体系结构实验</h1>
  <p align="center">
    一套渐进式裸机实验 —— 从汇编基础到 FPGA 上的 RTL 级流水线优化，覆盖 RISC-V 体系结构核心知识。
  </p>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/开发板-Nuclei_DDR200T_(Artix--7)-blue?style=flat-square" alt="Board" />
  <img src="https://img.shields.io/badge/SoC-HBirdv2_E203-green?style=flat-square" alt="SoC" />
  <img src="https://img.shields.io/badge/指令集-RV32IMAC-orange?style=flat-square" alt="ISA" />
  <img src="https://img.shields.io/badge/工具链-RISC--V_GCC-red?style=flat-square" alt="Toolchain" />
  <img src="https://img.shields.io/badge/IDE-Nuclei_Studio-purple?style=flat-square" alt="IDE" />
  <img src="https://img.shields.io/badge/HDL-Verilog-yellow?style=flat-square" alt="HDL" />
  <img src="https://img.shields.io/badge/CoreMark-36.436_(2.277_CM%2FMHz)-brightgreen?style=flat-square" alt="CoreMark" />
</p>

<p align="center">
  <a href="README.md"><strong>[ English README ]</strong></a>
</p>

---

## 项目亮点

- **全栈深度 (Full-Stack Depth)**：从 C/汇编混合编程到 Verilog RTL 修改，在真实 FPGA 上完成，无仿真器。
- **量化分析 (Quantitative Rigor)**：所有设计决策均有 `mcycle`/`minstret` CSR 周期级精确测量支撑。
- **渐进式复杂度 (Progressive Complexity)**：四个实验形成完整知识弧线 —— 汇编 → MMIO 驱动 → 中断系统 → 流水线优化。
- **RTL 级优化 (RTL-Level Optimization)**：在 `e203_exu.v` 中添加数据前递（Data Forwarding）旁路逻辑，消除 Load-Use 停顿（Stall）。

---

## 硬件平台

| 组件 | 规格 |
|------|------|
| **FPGA** | Xilinx XC7A200T（Nuclei DDR200T 开发板） |
| **处理器** | HBirdv2 E203 — 二级流水线，RV32IMAC |
| **主频** | 16 MHz |
| **存储** | ITCM 64 KB / DTCM 16 KB |
| **外设** | GPIO（6 LED + 6 拨码开关）、UART 115200-8N1 |
| **调试** | JTAG（OpenOCD） |

---

## 实验总览与关键指标

| 实验 | 主题 | 核心技术 | 关键指标 |
|------|------|----------|----------|
| **Lab 1** | C/汇编互调 | RISC-V 调用规约、寄存器级调试 | `sum_to_n` 经 `a0` 验证 |
| **Lab 2** | 裸机 MMIO | GPIO 三步初始化、RMW 安全写、UART 轮询 | 6 位拨码开关↔LED 联动 |
| **Lab 3** | 中断层次体系 | 轮询 → MSI 陷阱 → PLIC 中断驱动 | 延迟: 28 → 59 周期; CPU 空闲率: 1% → 99% |
| **Lab 4** | 流水线优化 | CoreMark 插桩、RTL 数据前递 | **CPI = 1.915，分数 = 36.436** |

---

## Lab 1 — 汇编程序设计

**目标**：用 RISC-V 汇编实现 `sum_to_n(n) = 1 + 2 + ... + n`，从 C 函数调用。

**核心实现** (`Sum.S`)：
```asm
sum_to_n:
    li   t0, 0          # 累加器 = 0
    li   t1, 1          # 计数器 = 1
loop:
    add  t0, t0, t1     # 累加器 += 计数器
    addi t1, t1, 1      # 计数器++
    bgt  a0, t1, loop   # 若 n > 计数器，继续循环
    add  a0, t0, x0     # 返回值存入 a0
    ret
```

> **调用规约 (Calling Convention)**：参数通过 `a0` 传入，返回值通过 `a0` 传出。被调用函数仅使用临时寄存器（`t0`–`t1`），无需栈帧。

---

## Lab 2 — MMIO 与 GPIO/UART 驱动

**目标**：通过直接内存映射 I/O（Memory-Mapped I/O）编写裸机 GPIO 和 UART 驱动。

### MMIO 寄存器映射表

| 寄存器 | 地址 | 功能 |
|--------|------|------|
| `GPIOA_PADDIR` | `0x10012000` | 引脚方向（1 = 输出） |
| `GPIOA_PADIN`  | `0x10012004` | 输入值读取 |
| `GPIOA_PADOUT` | `0x10012008` | 输出值写入 |
| `GPIOA_IOFCFG` | `0x1001201C` | I/O 功能配置 |
| `UART0_THR/RBR`| `0x10013000` | 发送保持 / 接收缓冲 |
| `UART0_LSR`    | `0x10013014` | 线路状态（TX 空 / RX 就绪） |

### GPIO 引脚映射

```
Bit:   31  30  29  28  27  26  25  24  23  22  21  20
功能:  SW5 SW4 SW3 SW2 SW1 SW0 LED5 LED4 LED3 LED2 LED1 LED0
```

### 关键的三步 GPIO 初始化流程

```c
// 1. 清除 IOF — 释放引脚的硬件功能复用
REG32(GPIOA_IOFCFG) &= ~(LED_MASK | SW_MASK);
// 2. 设置方向 — LED 为输出，拨码开关为输入
REG32(GPIOA_PADDIR) |= LED_MASK;
REG32(GPIOA_PADDIR) &= ~SW_MASK;
// 3. 初始化输出 — 所有 LED 关闭（低电平有效）
REG32(GPIOA_PADOUT) |= LED_MASK;
```

> **设计模式**：所有 GPIO 写操作采用"读-修改-写"（Read-Modify-Write, RMW）模式，保护共享 MMIO 寄存器上相邻引脚的状态不被破坏。

---

## Lab 3 — 中断层次体系：从轮询到中断驱动

本实验通过三个递进任务构建完整的中断系统，量化**响应延迟 (Response Latency)** 与 **CPU 利用率 (CPU Utilization)** 之间的根本权衡。

### Task 1 — 轮询模式 (Polling Mode)

紧密循环 GPIO 轮询，通过 `mcycle` CSR 进行周期精确测量。

**实测性能**：

| 指标 | 数值 |
|------|------|
| 轮询循环体 | **56 周期** |
| 平均响应延迟 | **28 周期**（= 56/2） |
| CPU 占用率（用于轮询） | **99%** |
| 有效计算能力 | **~1%** |

### Task 2 — 陷阱框架 (Trap Framework, MSI)

汇编级陷阱入口 (`lab3_trap_entry.S`)，在 64 字节栈帧上保存 16 个调用者保存寄存器：

```
┌─────────────────────────────────────┐
│         陷阱入口执行序列            │
├─────────────────────────────────────┤
│ 1. addi sp, sp, -64                │  ← 分配 64B 栈帧
│ 2. sw ra/t0–t6/a0–a7 → [sp]       │  ← 保存 16 个寄存器
│ 3. csrr a0, mcause                 │  ← 读取陷阱原因
│ 4. csrr a1, mepc                   │  ← 读取异常 PC
│ 5. csrr a2, mtval                  │  ← 读取陷阱值
│ 6. call lab3_trap_handler          │  ← 分发到 C 处理函数
│ 7. lw ra/t0–t6/a0–a7 ← [sp]      │  ← 恢复 16 个寄存器
│ 8. addi sp, sp, 64                 │  ← 释放栈帧
│ 9. mret                            │  ← 从陷阱返回
└─────────────────────────────────────┘
```

**CSR 配置链**：
```
mtvec   ← &lab3_trap_entry     // 陷阱向量基地址
mie     |= MIE_MSIE            // 使能机器软件中断 (Machine Software Interrupt)
mstatus |= MSTATUS_MIE         // 全局中断使能
```

### Task 3 — 完整中断驱动系统 (PLIC + UART RX)

**实测性能**：

| 指标 | 数值 |
|------|------|
| ISR 执行时间 | **93–178 周期** |
| 中断延迟（硬件 + 入口） | **59–60 周期（~3.7 μs @ 16 MHz）** |
| CPU 空闲率 | **>99%** |

### 延迟模型：轮询 vs. 中断驱动

**轮询模式 (Polling)**：

$$L_{\text{poll}} = \frac{T_{\text{loop}}}{2} = \frac{56}{2} = 28 \text{ 周期}$$

$$\eta_{\text{CPU}} = \frac{T_{\text{useful}}}{T_{\text{total}}} \approx 1\%$$

**中断驱动模式 (Interrupt-Driven)**：

$$L_{\text{int}} = T_{\text{hw\_latch}} + T_{\text{context\_save}} + T_{\text{dispatch}} \approx 59 \text{ 周期}$$

$$\eta_{\text{CPU}} = 1 - \frac{N_{\text{events}} \cdot T_{\text{ISR}}}{T_{\text{total}}} > 99\%$$

### 对比总结

| 维度 | 轮询 (Polling) | 中断驱动 (Interrupt) | 优胜方 |
|------|----------------|----------------------|--------|
| 响应延迟 | **28 周期 (1.75 μs)** | 59 周期 (3.69 μs) | 轮询 |
| CPU 利用率 | 1% 有效 | **>99% 有效** | 中断 |
| 可扩展性 | 随设备数 O(n) 增长 | **均摊 O(1)** | 中断 |
| 功耗效率 | 全速持续运行 | **事件驱动，可 WFI 休眠** | 中断 |
| 代码复杂度 | 简单循环 | ISR + CSR + 上下文保存 | 轮询 |

> **核心结论**：轮询在原始延迟上胜出（28 vs. 59 周期），但根本上不可扩展。中断驱动模型释放了 **98%+ 的 CPU 周期**用于计算 —— 在多任务场景下带来 50 倍吞吐量增益，代价仅为 2.1 倍的延迟增加。

---

## Lab 4 — CoreMark 与流水线优化

### 阶段 A — 基线测试

| 参数 | 数值 |
|------|------|
| CoreMark 分数 | **36.436** |
| CoreMark/MHz | **2.277** |
| 迭代次数 | 4000 |
| 运行时间 | >10 s |

### 阶段 B — 性能插桩

通过内联汇编读取 `mcycle`/`minstret` 性能计数器（CSR）：

| 计数器 | 数值 |
|--------|------|
| 总周期数 (Total Cycles) | 1,756,951,756 |
| 总指令数 (Total Instructions) | 917,399,219 |
| **CPI（每指令周期数）** | **1.915** |
| **IPC（每周期指令数）** | **0.522** |

$$\text{CPI} = \frac{\text{Cycles}}{\text{Instructions}} = \frac{1{,}756{,}951{,}756}{917{,}399{,}219} = 1.915$$

> **分析**：CPI = 1.915 表明平均每条指令消耗近 2 个周期 —— 二级流水线的理想值为 CPI = 1.0。0.915 周期/指令的差距主要由流水线停顿（Pipeline Stall）导致。

### 阶段 C — 瓶颈识别

5 次独立运行确认裸机 FPGA 上执行完全确定性（标准差 σ = 0）。停顿来源按影响排序：

| 相关性类型 (Hazard Type) | 机制 | 估计影响 |
|--------------------------|------|----------|
| **Load-Use（RAW 数据相关）** | `lw rd, ...` 后紧跟使用 `rd` 的 ALU 指令 — 1 周期停顿 | **主要因素**（~60%） |
| **控制相关 (Control Hazard)** | 分支目标在 EXE 阶段才确定 — 1 周期冲刷 | 次要因素（~25%） |
| **长延迟 ALU** | MUL/DIV 多周期执行 → 流水线停顿 | 第三因素（~15%） |

### 阶段 D — RTL 优化：`e203_exu.v` 中的数据前递

核心优化：**旁路寄存器堆（Register File）**，将加载存储单元（LSU）写回数据直接前递到派发阶段（Dispatch Stage），消除 Load-Use 停顿。

#### 前递逻辑 (Verilog)

```verilog
// ─── Load-Use 数据前递 (e203_exu.v, 第 309-335 行) ───

// 1. 前递使能：写回有效 ∧ 目标寄存器匹配源寄存器 ∧ 目标 ≠ x0 ∧ 整数寄存器
wire fwd_rs1_en = longp_wbck_o_valid
                & (longp_wbck_o_rdidx == i_rs1idx)
                & (|longp_wbck_o_rdidx)
                & (~longp_wbck_o_rdfpu);

wire fwd_rs2_en = longp_wbck_o_valid
                & (longp_wbck_o_rdidx == i_rs2idx)
                & (|longp_wbck_o_rdidx)
                & (~longp_wbck_o_rdfpu);

// 2. 多路选择器：选择前递数据或寄存器堆数据
wire [`E203_XLEN-1:0] disp_fwd_rs1 = fwd_rs1_en ? longp_wbck_o_wdat[`E203_XLEN-1:0] : rf_rs1;
wire [`E203_XLEN-1:0] disp_fwd_rs2 = fwd_rs2_en ? longp_wbck_o_wdat[`E203_XLEN-1:0] : rf_rs2;

// 3. 当前递解决了依赖关系时，抑制停顿信号
wire disp_no_stall_rs1 = oitfrd_match_disprs1 & (~fwd_rs1_en);
wire disp_no_stall_rs2 = oitfrd_match_disprs2 & (~fwd_rs2_en);
```

#### 流水线时序图：前递前 vs. 前递后

```
无前递（Load-Use 停顿）:
  周期:    1       2       3       4       5
  lw x5   [IF]    [EX]    [WB]
  add x6  -----   [IF]   <停顿>   [EX]    [WB]
                          ↑ 停顿：x5 尚未写回寄存器堆

有前递（旁路）:
  周期:    1       2       3       4
  lw x5   [IF]    [EX]    [WB]
  add x6  -----   [IF]  ──[EX]──  [WB]
                          ↑ x5 从 WB 前递到 EX
```

#### 前递条件（布尔逻辑）

$$\text{fwd\_en} = V_{\text{wb}} \wedge (R_{\text{dst}} = R_{\text{src}}) \wedge (R_{\text{dst}} \neq \texttt{x0}) \wedge \neg F_{\text{fpu}}$$

其中：
- $V_{\text{wb}}$：写回阶段有有效数据
- $R_{\text{dst}} = R_{\text{src}}$：目标寄存器与源操作数匹配
- $R_{\text{dst}} \neq \texttt{x0}$：排除硬连线零寄存器
- $\neg F_{\text{fpu}}$：整数寄存器（非浮点）

> **结果**：前递路径将 Load-Use 停顿从强制 1 周期惩罚转变为零周期旁路，直接推动 CPI 向理论最小值 1.0 逼近。

### 编译器优化标志

```
-O2 -funroll-all-loops -finline-limit=600
-ftree-dominator-opts -fno-if-conversion2
-finline-functions -falign-functions=4
-falign-jumps=4 -falign-loops=4
```

---

## 项目结构

```
RISC-V-Computer-Architecture-Labs/
├── Lab1-Assembly/                          # Lab 1: C/汇编互调
│   └── src/
│       ├── main.c                          #   C 入口
│       └── Sum.S                           #   RISC-V 汇编 (sum_to_n)
├── Lab2-MMIO/                              # Lab 2: 裸机 MMIO 驱动
│   └── src/
│       ├── main.c                          #   GPIO & UART 驱动
│       └── lab2_mmio.h                     #   寄存器映射定义
├── Lab3-Interrupts/                        # Lab 3: 中断系统
│   ├── Task1-Polling/src/                  #   T1: GPIO 轮询 + 周期计数
│   ├── Task2-TrapFramework/src/            #   T2: MSI 陷阱 + 汇编入口
│   └── Task3-InterruptDriven/src/          #   T3: PLIC + UART RX + 性能统计
│       ├── lab3_main.c                     #     交互命令界面
│       ├── lab3_trap.c / .h                #     C 陷阱处理函数
│       ├── lab3_trap_entry.S               #     汇编陷阱入口
│       ├── lab3_stats.c / .h               #     性能统计模块
│       └── lab3_timer.c / .h               #     定时器工具
├── Lab4-CoreMark/                          # Lab 4: CoreMark 与 RTL 优化
│   ├── src/                                #   EEMBC CoreMark 测试套件
│   │   ├── core_main.c                     #     基准驱动
│   │   ├── core_list_join.c                #     链表负载
│   │   ├── core_matrix.c                   #     矩阵负载
│   │   ├── core_state.c                    #     状态机负载
│   │   └── core_portme.c / .h              #     平台移植层
│   └── rtl/                                #   Verilog RTL 修改
│       ├── e203_exu.v                      #     ★ 执行单元（前递逻辑）
│       └── config.v                        #     处理器配置
├── docs/                                   # 各实验详细文档（8 个文件）
├── reports/
│   ├── Lab1~4-实验报告.pdf                  # 最终实验报告
│   ├── guides/                             # 实验指导文档
│   ├── diagrams/                           # 架构图与截图
│   └── drafts/                             # 报告草稿
├── references/                             # 参考资料
├── README.md                               # English version（英文版）
└── README_CN.md                            # 本文件（中文版）
```

---

## 构建与运行

### 环境准备

- [Nuclei Studio IDE](https://nucleisys.com/download.php)（基于 Eclipse）
- RISC-V GCC 工具链（Nuclei Studio 内置）
- Nuclei DDR200T FPGA 开发板 + JTAG 调试器
- [HBird SDK](https://github.com/riscv-mcu/hbird-sdk) — NMSIS Core 头文件与 SoC 支持包

### 构建步骤

```bash
# 1. 将项目导入 Nuclei Studio
# 2. 选择目标存储布局：ILM（推荐调试用）或 Flash
# 3. 构建 → GCC RISC-V 交叉编译
# 4. 通过 JTAG 烧写
# 5. 打开串口终端：波特率 115200，8N1
```

Lab 4 RTL 修改额外步骤：
1. 用修改后的 `e203_exu.v` 替换 HBirdv2 SoC RTL 源码中的对应文件
2. 在 `config.v` 中使能 `E203_CFG_SUPPORT_LSU_FWD` 宏
3. 重新综合比特流并烧写 FPGA

---

## 技术栈与核心概念

| 类别 | 详情 |
|------|------|
| **指令集** | RISC-V RV32IMAC（整数、乘法、原子操作、压缩指令） |
| **编程语言** | C、RISC-V 汇编（GAS 语法）、Verilog HDL |
| **硬件接口** | MMIO、GPIO、UART、JTAG |
| **中断系统** | CLINT（MSI/MTI）、PLIC（外部中断）、基于 CSR 的陷阱处理 |
| **流水线** | 二级流水线（IF/EX+WB）、Load-Use 数据相关、数据前递旁路 |
| **性能基准** | EEMBC CoreMark、`mcycle`/`minstret` CSR 插桩 |

---

## 许可证

本项目仅供教育用途。CoreMark 是 [EEMBC](https://www.eembc.org/coremark/) 的注册商标。
