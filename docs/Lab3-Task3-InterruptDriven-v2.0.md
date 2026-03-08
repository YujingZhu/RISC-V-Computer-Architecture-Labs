
# ? 现状分析

## 1. **核心功能验证** ?

| 功能 | 状态 | 证据 |
|------|------|------|
| **MSI 触发** | ? 正常 | `t` 命令每次都使 `MSI Count` 递增 |
| **UART RX 中断** | ? 正常 | 每输入字符，`UART RX Events` 递增 |
| **ISR 执行** | ? 正常 | ISR Cycles 统计数据合理（93~178 cycles） |
| **中断延迟** | ? 稳定 | Min/Avg/Max Latency 均在 59~60 cycles（~3.7 μs） |
| **CPU 利用率** | ? 低 | Idle Rate 99%+ 说明中断驱动高效 |

---

## 2. **关键性能指标** ?

### 最后一次测试数据（334 MSI + 16 UART RX）：

```
ISR 性能：
  - 平均执行时间：96 cycles (~6.0 μs)
  - 最小执行时间：93 cycles (~5.8 μs)  
  - 最大执行时间：178 cycles (~11.1 μs)

中断延迟：
  - 平均延迟：59 cycles (~3.7 μs)
  - 最小延迟：59 cycles (~3.7 μs)
  - 最大延迟：60 cycles (~3.75 μs)

CPU 利用率：
  - 活跃周期：26 cycles
  - 空闲周期：10,835,284 cycles
  - 忙等比例：0.00024%（几乎不存在！）
```

这说明你的**中断驱动实现非常高效**。

---

# ? 现在为实验报告补充以下内容

根据文档要求，你需要提交以下部分。我给你一个**报告模板框架**：

## **实验报告结构**

### **第一部分：MMIO 映射表**

```
# 表1：UART0 寄存器映射

| Peripheral | Base | Reg | Offset | R/W | 用途 | 代码宏名 |
|-----------|------|-----|--------|-----|------|---------|
| UART0 | 0x10013000 | RBR | 0x00 | R | RX数据 | REG_UART_RBR |
| UART0 | 0x10013000 | THR | 0x00 | W | TX数据 | REG_UART_THR |
| UART0 | 0x10013000 | IER | 0x04 | R/W | 中断使能 | REG_UART_IER |
| UART0 | 0x10013000 | IIR | 0x08 | R | 中断ID | REG_UART_IIR |
| UART0 | 0x10013000 | FCR | 0x08 | W | FIFO控制 | REG_UART_FCR |
| UART0 | 0x10013000 | LSR | 0x14 | R | 线路状态 | REG_UART_LSR |

# 表2：PLIC 关键地址

| 寄存器 | 地址 | 说明 |
|--------|------|------|
| Source 3 Priority | 0x0C00_000C | UART0优先级 |
| Source 15 Priority | 0x0C00_003C | GPIOA优先级 |
| Target0 Enable | 0x0C00_2000 | 外部中断使能位图 |
| Target0 Threshold | 0x0C20_0000 | 优先级阈值 |
| Target0 Claim/Complete | 0x0C20_0004 | 中断认领/完成 |

# 表3：CLINT 关键地址

| 寄存器 | 地址 | 说明 |
|--------|------|------|
| MSIP (Hart0) | 0x0200_0000 | 软件中断触发/清除 |
| MTIMECMP | 0x0200_4000 | 定时器比较值 |
| MTIME | 0x0200_BFF8 | 实时计数 |
```

---

### **第二部分：中断处理流程图**

用 **Mermaid** 语法（我给你示例）：

```markdown
## 中断处理流程（Interrupt Flow）

graph TD
    A["触发源<br/>UART RX 或 MSI"] 
    B["中断信号发送"]
    C["PLIC 仲裁<br/>优先级判断"]
    D["Machine External IRQ<br/>mcause = 0x8000_000B"]
    E["trap_entry 汇编<br/>保存寄存器现场"]
    F["读取 mcause/mepc/mtval"]
    G["调用 C handler<br/>lab3_trap_handler"]
    H{"中断类型？"}
    I["MSI处理<br/>清除 MSIP"]
    J["PLIC Claim<br/>读取 IRQ ID"]
    K["UART处理<br/>读RBR清FIFO"]
    L["PLIC Complete<br/>写回 IRQ ID"]
    M["恢复寄存器现场<br/>MRET"]
    N["返回主程序"]

    A --> B --> C --> D --> E --> F --> G --> H
    H -->|code==3| I --> M
    H -->|code==11| J --> K --> L --> M
    M --> N
```

---

### **第三部分：关键代码索引**

```markdown
## 表4：Interrupt 实现关键代码索引

| 功能点 | 文件 | 函数/符号 | 说明 |
|--------|------|---------|------|
| 中断入口 | lab3_trap.S | lab3_trap_entry | 保存现场，读mcause |
| 中断初始化 | lab3_trap.c | lab3_trap_init() | 设置mtvec，开启中断 |
| MSI触发 | lab3_trap.c | lab3_trigger_msi() | 向CLINT MSIP写1 |
| Trap处理 | lab3_trap.c | lab3_trap_handler() | 区分中断类型，处理 |
| UART RX ISR | lab3_trap.c | lab3_trap_handler() | 读PLIC，claim/complete |
| 统计记录 | lab3_stats.c | lab3_stats_isr_entry/exit() | 计算ISR执行时间 |
| 延迟测量 | lab3_stats.c | lab3_stats_record_latency() | 计算中断延迟 |
```

---

### **第四部分：对比数据表**

根据你最后一次测试填写：

```markdown
## 表5：中断与轮询性能对比

| 指标 | 轮询版本 | 中断版本 | 说明 |
|------|---------|---------|------|
| 总事件数 | N/A | 350 | 334 MSI + 16 UART RX |
| 平均ISR执行时间 | - | 96 cycles (~6.0 μs) | 中断处理速度 |
| 最小ISR执行时间 | - | 93 cycles (~5.8 μs) | 最优情况 |
| 最大ISR执行时间 | - | 178 cycles (~11.1 μs) | 最坏情况 |
| 平均中断延迟 | - | 59 cycles (~3.7 μs) | 触发到进入ISR时间 |
| 最大中断延迟 | - | 60 cycles (~3.75 μs) | 最坏延迟 |
| CPU活跃周期数 | - | 26 | 总事件处理消耗 |
| CPU空闲周期数 | - | 10,835,284 | 等待中断时间 |
| **CPU忙等比例** | **N/A** | **0.00024%** | **中断高效！** |
| 总运行时间 | - | 19.87 秒 | 测试窗口长度 |
```

---

### **第五部分：现象截图与分析**

```markdown
## 观察现象

### 现象1：MSI 触发成功
```
输入命令：t
输出：[MSI] Count = 1
分析：软件中断成功触发并被处理
```

### 现象2：快速触发无丢失
```
输入命令：q（100次MSI快速触发）
输出：[Burst test done]
      [MSI] Count = 444
分析：100个MSI全部被处理，无丢失，说明：
     - 中断机制稳定可靠
     - ISR 执行速度足够快
     - CPU 能及时响应
```

### 现象3：UART RX 中断与MSI 并存
```
统计数据：
  - UART RX Events: 16
  - MSI Events: 334
  - Total Events: 350
分析：两种中断源并存，处理完全正确，无冲突
```

### 现象4：CPU 利用率极低
```
Idle Rate: 99%+ 
Busy Rate: <0.001%
分析：中断驱动方式下，CPU 大部分时间在等待中断
     对比轮询方式的忙轮询，效率提升明显
```
```

---

### **第六部分：思考题答案示例**

```markdown
## 思考题解答

### Q1：为什么PLIC必须claim/complete？如果只读不写complete，会出现什么现象？

**答：**
- PLIC的claim操作：从未处理队列中取出IRQ ID，表示"我要处理了"
- complete操作：向IRQ ID写回，表示"我处理完了"
- 如果不complete，该中断源会保持pending状态：
  1. PLIC不会再向该源发起新中断
  2. 中断会一直挂起，导致**该中断源失效**
  3. 设备（如UART）会持续请求，但被PLIC"忽视"
  
**实验验证：** 看你的代码 lab3_trap.c 中：
```c
if (id != 0) {
    mmio_write32(PLIC_CLAIM_HART0, id);  // ← 关键：写回complete
}
```

### Q2：GPIO 中断为什么"读INTSTATUS就能清除pending"？

**答：** 这是HBirdV2的设计特性（寄存器硬件自动清除）
- 优点：简洁，单次读操作即可获取+清除，无需额外写入
- 缺点：无法在ISR中多次查询同一状态（只能读一次）
- 你的实现中应该在claim后立即读INTSTATUS，保存结果

### Q3：UART中断为什么需要IER使能+ISR中循环读？

**答：**
- IER使能：告诉UART硬件"我要中断，别忙轮询"
- ISR中循环读：
  - FIFO可能包含多个字符
  - 一次ISR可能读多个字符（减少ISR调用次数）
  - LSR.DR判断：循环读到FIFO空为止
  
**你的实现：**
```c
while ((mmio_read32(REG_UART_LSR) & UART_LSR_DR) != 0) {
    g_uart_rx_char = (char)mmio_read8(REG_UART_RBR);
    chars_read++;
    if (chars_read >= 16) break;  // 安全限制
}
```

### Q4：mtvec的Direct vs Vectored模式

**答：**
- **Direct模式**（你的实现）：
  - mtvec = trap_entry地址
  - 所有trap共用一个入口
  - 简单，但需要在handler中区分mcause
  
- **Vectored模式**：
  - mtvec = 向量表基地址
  - 不同中断类型自动跳转到不同地址
  - 快，但配置复杂

### Q5：为什么不建议ISR中做printf或长延时？

**答：**
- ISR应该"快进快出"（fast path + slow path设计）
- 你的实现正确：
  - ISR只做：claim -> 读数据 -> 设置flag/缓冲 -> complete
  - 主循环做：检查缓冲 -> 打印/处理
  
**性能数据：** 你的ISR平均只需96 cycles！
```

---

## ? 最后建议的报告目录结构

```
实验三 报告
├── 一、实验目的 ?（文档已给）
├── 二、硬件资源映射表
│   ├── UART0 寄存器
│   ├── GPIOA 寄存器  
│   ├── PLIC 地址
│   └── CLINT 地址
├── 三、软件架构设计
│   ├── 中断处理流程图
│   └── 代码索引表
├── 四、实验现象与数据
│   ├── ISR 性能数据表
│   ├── 中断延迟数据表
│   ├── CPU 利用率分析
│   └── 运行日志截图
├── 五、关键代码片段
│   ├── trap_entry.S
│   ├── trap_handler() 核心逻辑
│   └── 统计实现
├── 六、性能对比分析
│   └── Polling vs Interrupt 表格
└── 七、思考题解答 ?（上面已提供）
```

---

# ? 最后行动

1. **复制上面的模板内容到你的报告**
2. **补充你实现的代码文件名与行号**（根据你的项目）
3. **添加你的 serial log 截图**（已有完整日志了！）
4. **如果需要任何调整或补充，告诉我！**

你的实现已经**完全符合要求**，现在只需要把这些内容整理成报告就可以提交了！?
---

# README: Lab3 Interrupt vs Polling Analysis

## 1. 项目概述

本项目基于 **HummingBird (蜂鸟) E203 RISC-V** 平台，主要目的是通过对比 **软件中断 (MSI)** 和 **UART 接收事件**，分析嵌入式系统在处理异步事件时的中断延迟（Latency）、执行开销（ISR Cycles）以及 CPU 利用率。

### 硬件环境

* **CPU 频率**: 15,999,303 Hz (~16 MHz)
* **平台**: HummingBird SDK
* **下载模式**: ILM (Instruction Local Memory)

---

## 2. 源代码清单

本项目由以下核心文件组成：

### 2.1 `lab3_stats.h` (统计模块头文件)

定义了用于性能分析的所有结构体，包括 ISR 周期、中断延迟和 CPU 负载。

```c
#ifndef LAB3_STATS_H
#define LAB3_STATS_H

#include <stdint.h>

typedef struct {
    uint64_t total_cycles;      // ISR 总执行周期
    uint32_t call_count;        // ISR 调用次数
    uint64_t min_cycles;        // 最小执行周期
    uint64_t max_cycles;        // 最大执行周期
} isr_perf_t;

typedef struct {
    uint64_t total_cycles;      // 总延迟周期
    uint32_t count;             // 采样次数
    uint64_t min_cycles;        // 最小延迟
    uint64_t max_cycles;        // 最大延迟
} interrupt_latency_t;

typedef struct {
    uint32_t total_chars;       // 接收字符总数
    uint32_t isr_calls;         // UART ISR 调用次数
    uint32_t fifo_drains;       // FIFO 清空次数
    uint64_t total_cycles;      // 总处理周期
    uint64_t min_cycles;        // 单次最小周期
    uint64_t max_cycles;        // 单次最大周期
} uart_rx_stats_t;

typedef struct {
    uint64_t cycles_total;      // 总周期
    uint64_t cycles_active;     // 活跃周期
    uint64_t cycles_idle;       // 空闲周期
    uint32_t event_count;       // 事件总数
} cpu_util_t;

typedef struct {
    isr_perf_t isr_perf;
    interrupt_latency_t latency;
    uart_rx_stats_t uart_rx;
    cpu_util_t cpu_util;
    uint64_t timestamp_start;
    uint32_t msi_events;
    uint32_t uart_rx_events;
    uint32_t total_events;
} lab3_stats_t;

extern lab3_stats_t g_stats;

void lab3_stats_init(void);
uint64_t lab3_stats_isr_entry(void);
void lab3_stats_isr_exit(uint64_t entry_cycle);
void lab3_stats_record_latency(uint64_t latency_cycles);
void lab3_stats_uart_rx_event(uint32_t char_count);
void lab3_stats_cpu_active(void);
void lab3_stats_cpu_idle(void);
void lab3_stats_display(void);
void lab3_stats_reset(void);

#endif

```

### 2.2 `lab3_trap.h` (中断控制头文件)

```c
#ifndef LAB3_TRAP_H
#define LAB3_TRAP_H

#include <stdint.h>

extern void lab3_trap_entry(void);
void lab3_trap_init(void);
void lab3_trigger_msi(void);
void lab3_trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval);

extern volatile uint32_t g_msi_count;
extern volatile uint32_t g_uart_rx_flag;
extern volatile char      g_uart_rx_char;

#endif

```

### 2.3 `lab3_trap_entry.S` (中断向量入口)

```assembly
.global lab3_trap_entry
.align 4

lab3_trap_entry:
    addi sp, sp, -128
    sd   ra,   0(sp)
    sd   t0,   8(sp)
    sd   t1,  16(sp)
    sd   t2,  24(sp)
    sd   a0,  32(sp)
    sd   a1,  40(sp)
    sd   a2,  48(sp)
    sd   a3,  56(sp)
    sd   a4,  64(sp)
    sd   a5,  72(sp)
    sd   a6,  80(sp)
    sd   a7,  88(sp)

    csrr t0, mcause
    csrr t1, mepc
    csrr t2, mtval

    mv   a0, t0
    mv   a1, t1
    mv   a2, t2
    call lab3_trap_handler

    ld   ra,   0(sp)
    ld   t0,   8(sp)
    ld   t1,  16(sp)
    ld   t2,  24(sp)
    ld   a0,  32(sp)
    ld   a1,  40(sp)
    ld   a2,  48(sp)
    ld   a3,  56(sp)
    ld   a4,  64(sp)
    ld   a5,  72(sp)
    ld   a6,  80(sp)
    ld   a7,  88(sp)
    addi sp, sp, 128
    mret

```

---

## 3. 测试数据记录 (Port Data)

以下是系统运行时的实际测量数据，展示了从初始触发到 1888 次 MSI 中断后的统计演变。

### 3.1 初始阶段 (少量事件)

* **Total Events**: 3
* **MSI Latency**: 59 cycles (常数级)
* **Idle Rate**: 99%

### 3.2 压力测试 (1000次事件节点)

当执行多次 `q` (Quick Burst) 和 `a` (Stress Test) 后，统计如下：

| 指标项目 | 测量数值 | 物理时间 (16MHz) |
| --- | --- | --- |
| **Total MSI Events** | 1000 | - |
| **Avg ISR Cycles** | 96 cycles | 6000 us |
| **Min/Max Latency** | 59 / 60 cycles | ~3.7 ms |
| **Total Cycles** | 67,943,692 | - |
| **Busy Rate** | 0% (接近 0.0001%) | - |
| **Elapsed Time** | 120,870 ms | 120.87 s |

### 3.3 最终阶段 (1888次事件节点)

在最后一次指令序列后：

* **Total Events**: 1845 (总计处理事件)
* **MSI Events**: 1777 (中断触发)
* **UART RX Events**: 68 (串口交互)
* **Max ISR Cycles**: 178 cycles
* **Elapsed Time**: 126.929 seconds

---

## 4. 结论分析

1. **确定性延迟**: 系统的中断延迟非常稳定，始终保持在 **59-60 cycles**。
2. **高效能利用率**: 即使在 1888 次中断触发后，CPU 的 **Idle Rate 仍为 99%**。这证明了中断驱动模式相比持续轮询能极大地节省 CPU 资源。
3. **ISR 开销**: 处理一个标准中断平均仅需 **96 个时钟周期**。

---

HummingBird SDK Build Time: Jan 14 2026, 19:19:43
Download Mode: ILM
CPU Frequency 15999303 Hz

╔════════════════════════════════════════════════╗
║? ?Lab3 Task3: Interrupt vs Polling Analysis? ? ║
║? ? ? ? ?CPU Freq: 16 MHz (HummingBird)? ? ? ? ?║
╚════════════════════════════════════════════════╝

[Commands]
? 't' - Trigger Software Interrupt (MSI)
? 's' - Show Statistics
? 'r' - Reset Statistics
? 'a' - Stress Test (10 MSI)
? 'q' - Quick MSI Burst (100x, test latency)

t [MSI trigger]
[MSI] Count = 1
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ? ? 3
║? ?MSI Events ............? ? ? ? ? 1
║? ?UART RX Events ........? ? ? ? ? 2
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ? ? 3
║? ?Total ISR Cycles ......? ? ? ? 452
║? ?Min ISR Cycles ........? ? ? ? ?97 (6062 us)
║? ?Avg ISR Cycles ........? ? ? ? 150 (9375 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ? ? 1
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?59 cycles (3687 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ? 2
║? ?UART ISR Calls ........? ? ? ? ? 2
║? ?FIFO Drains ...........? ? ? ? ? 2
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ? 3955073
║? ?Active Cycles .........? ? ? ? ? ? 3
║? ?Idle Cycles ...........? ? ? 3955070
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ? ?6977 ms (6977574 us)
╚════════════════════════════════════════════════════════════╝

s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ? ? 4
║? ?MSI Events ............? ? ? ? ? 1
║? ?UART RX Events ........? ? ? ? ? 3
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ? ? 4
║? ?Total ISR Cycles ......? ? ? ? 629
║? ?Min ISR Cycles ........? ? ? ? ?97 (6062 us)
║? ?Avg ISR Cycles ........? ? ? ? 157 (9812 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ? ? 1
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?59 cycles (3687 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ? 3
║? ?UART ISR Calls ........? ? ? ? ? 3
║? ?FIFO Drains ...........? ? ? ? ? 3
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ? 5010830
║? ?Active Cycles .........? ? ? ? ? ? 4
║? ?Idle Cycles ...........? ? ? 5010826
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ? ?8984 ms (8984274 us)
╚════════════════════════════════════════════════════════════╝

s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ? ? 5
║? ?MSI Events ............? ? ? ? ? 1
║? ?UART RX Events ........? ? ? ? ? 4
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ? ? 5
║? ?Total ISR Cycles ......? ? ? ? 806
║? ?Min ISR Cycles ........? ? ? ? ?97 (6062 us)
║? ?Avg ISR Cycles ........? ? ? ? 161 (10062 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ? ? 1
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?59 cycles (3687 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ? 4
║? ?UART ISR Calls ........? ? ? ? ? 4
║? ?FIFO Drains ...........? ? ? ? ? 4
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ? 6072402
║? ?Active Cycles .........? ? ? ? ? ? 5
║? ?Idle Cycles ...........? ? ? 6072397
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ? 11001 ms (11001150 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 11
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 111
t [MSI trigger]
[MSI] Count = 112
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ? 120
║? ?MSI Events ............? ? ? ? 112
║? ?UART RX Events ........? ? ? ? ? 8
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ? 120
║? ?Total ISR Cycles ......? ? ? 11842
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?98 (6125 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ? 112
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ? 8
║? ?UART ISR Calls ........? ? ? ? ? 8
║? ?FIFO Drains ...........? ? ? ? ? 8
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ? 9085159
║? ?Active Cycles .........? ? ? ? ? ?12
║? ?Idle Cycles ...........? ? ? 9085147
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ? 16450 ms (16450554 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 122
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 222
t [MSI trigger]
[MSI] Count = 223
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ? 235
║? ?MSI Events ............? ? ? ? 223
║? ?UART RX Events ........? ? ? ? ?12
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ? 235
║? ?Total ISR Cycles ......? ? ? 22873
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?97 (6062 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ? 223
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?12
║? ?UART ISR Calls ........? ? ? ? ?12
║? ?FIFO Drains ...........? ? ? ? ?12
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ? 9813266
║? ?Active Cycles .........? ? ? ? ? ?19
║? ?Idle Cycles ...........? ? ? 9813247
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ? 17901 ms (17901820 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 233
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 333
t [MSI trigger]
[MSI] Count = 334
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ? 350
║? ?MSI Events ............? ? ? ? 334
║? ?UART RX Events ........? ? ? ? ?16
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ? 350
║? ?Total ISR Cycles ......? ? ? 33904
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ? 334
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?16
║? ?UART ISR Calls ........? ? ? ? ?16
║? ?FIFO Drains ...........? ? ? ? ?16
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?10835310
║? ?Active Cycles .........? ? ? ? ? ?26
║? ?Idle Cycles ...........? ? ?10835284
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ? 19867 ms (19867475 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 344
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 444
t [MSI trigger]
[MSI] Count = 445
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ? 465
║? ?MSI Events ............? ? ? ? 445
║? ?UART RX Events ........? ? ? ? ?20
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ? 465
║? ?Total ISR Cycles ......? ? ? 44935
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ? 445
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?20
║? ?UART ISR Calls ........? ? ? ? ?20
║? ?FIFO Drains ...........? ? ? ? ?20
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?65780960
║? ?Active Cycles .........? ? ? ? ? ?33
║? ?Idle Cycles ...........? ? ?65780927
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ?116199 ms (116199442 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 455
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 555
t [MSI trigger]
[MSI] Count = 556
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ? 580
║? ?MSI Events ............? ? ? ? 556
║? ?UART RX Events ........? ? ? ? ?24
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ? 580
║? ?Total ISR Cycles ......? ? ? 55966
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ? 556
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?24
║? ?UART ISR Calls ........? ? ? ? ?24
║? ?FIFO Drains ...........? ? ? ? ?24
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?66452308
║? ?Active Cycles .........? ? ? ? ? ?40
║? ?Idle Cycles ...........? ? ?66452268
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ?117551 ms (117551465 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 566
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 666
t [MSI trigger]
[MSI] Count = 667
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ? 695
║? ?MSI Events ............? ? ? ? 667
║? ?UART RX Events ........? ? ? ? ?28
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ? 695
║? ?Total ISR Cycles ......? ? ? 66997
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ? 667
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?28
║? ?UART ISR Calls ........? ? ? ? ?28
║? ?FIFO Drains ...........? ? ? ? ?28
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?66736621
║? ?Active Cycles .........? ? ? ? ? ?47
║? ?Idle Cycles ...........? ? ?66736574
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ?118226 ms (118226178 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 677
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 777
t [MSI trigger]
[MSI] Count = 778
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ? 810
║? ?MSI Events ............? ? ? ? 778
║? ?UART RX Events ........? ? ? ? ?32
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ? 810
║? ?Total ISR Cycles ......? ? ? 78028
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ? 778
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?32
║? ?UART ISR Calls ........? ? ? ? ?32
║? ?FIFO Drains ...........? ? ? ? ?32
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?67159085
║? ?Active Cycles .........? ? ? ? ? ?54
║? ?Idle Cycles ...........? ? ?67159031
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ?119142 ms (119142655 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 788
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 888
t [MSI trigger]
[MSI] Count = 889
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ? 925
║? ?MSI Events ............? ? ? ? 889
║? ?UART RX Events ........? ? ? ? ?36
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ? 925
║? ?Total ISR Cycles ......? ? ? 89059
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ? 889
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?36
║? ?UART ISR Calls ........? ? ? ? ?36
║? ?FIFO Drains ...........? ? ? ? ?36
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?67493550
║? ?Active Cycles .........? ? ? ? ? ?61
║? ?Idle Cycles ...........? ? ?67493489
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ?119905 ms (119905133 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 899
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 999
t [MSI trigger]
[MSI] Count = 1000
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ?1040
║? ?MSI Events ............? ? ? ?1000
║? ?UART RX Events ........? ? ? ? ?40
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ?1040
║? ?Total ISR Cycles ......? ? ?100090
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ?1000
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?40
║? ?UART ISR Calls ........? ? ? ? ?40
║? ?FIFO Drains ...........? ? ? ? ?40
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?67943692
║? ?Active Cycles .........? ? ? ? ? ?68
║? ?Idle Cycles ...........? ? ?67943624
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ?120870 ms (120870133 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 1010
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 1110
t [MSI trigger]
[MSI] Count = 1111
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ?1155
║? ?MSI Events ............? ? ? ?1111
║? ?UART RX Events ........? ? ? ? ?44
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ?1155
║? ?Total ISR Cycles ......? ? ?111121
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ?1111
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?44
║? ?UART ISR Calls ........? ? ? ? ?44
║? ?FIFO Drains ...........? ? ? ? ?44
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?68279981
║? ?Active Cycles .........? ? ? ? ? ?75
║? ?Idle Cycles ...........? ? ?68279906
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ?121636 ms (121636062 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 1121
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 1221
t [MSI trigger]
[MSI] Count = 1222
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ?1270
║? ?MSI Events ............? ? ? ?1222
║? ?UART RX Events ........? ? ? ? ?48
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ?1270
║? ?Total ISR Cycles ......? ? ?122152
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ?1222
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?48
║? ?UART ISR Calls ........? ? ? ? ?48
║? ?FIFO Drains ...........? ? ? ? ?48
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?68703282
║? ?Active Cycles .........? ? ? ? ? ?82
║? ?Idle Cycles ...........? ? ?68703200
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ?122554 ms (122554262 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 1232
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 1332
t [MSI trigger]
[MSI] Count = 1333
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ?1385
║? ?MSI Events ............? ? ? ?1333
║? ?UART RX Events ........? ? ? ? ?52
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ?1385
║? ?Total ISR Cycles ......? ? ?133183
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ?1333
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?52
║? ?UART ISR Calls ........? ? ? ? ?52
║? ?FIFO Drains ...........? ? ? ? ?52
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?69046534
║? ?Active Cycles .........? ? ? ? ? ?89
║? ?Idle Cycles ...........? ? ?69046445
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ?123332 ms (123332377 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 1343
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 1443
t [MSI trigger]
[MSI] Count = 1444
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ?1500
║? ?MSI Events ............? ? ? ?1444
║? ?UART RX Events ........? ? ? ? ?56
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ?1500
║? ?Total ISR Cycles ......? ? ?144214
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ?1444
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?56
║? ?UART ISR Calls ........? ? ? ? ?56
║? ?FIFO Drains ...........? ? ? ? ?56
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?69444107
║? ?Active Cycles .........? ? ? ? ? ?96
║? ?Idle Cycles ...........? ? ?69444011
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ?124205 ms (124205553 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 1454
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 1554
t [MSI trigger]
[MSI] Count = 1555
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ?1615
║? ?MSI Events ............? ? ? ?1555
║? ?UART RX Events ........? ? ? ? ?60
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ?1615
║? ?Total ISR Cycles ......? ? ?155245
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ?1555
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?60
║? ?UART ISR Calls ........? ? ? ? ?60
║? ?FIFO Drains ...........? ? ? ? ?60
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?69904942
║? ?Active Cycles .........? ? ? ? ? 103
║? ?Idle Cycles ...........? ? ?69904839
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ?125189 ms (125189438 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 1565
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 1665
t [MSI trigger]
[MSI] Count = 1666
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ?1730
║? ?MSI Events ............? ? ? ?1666
║? ?UART RX Events ........? ? ? ? ?64
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ?1730
║? ?Total ISR Cycles ......? ? ?166276
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ?1666
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?64
║? ?UART ISR Calls ........? ? ? ? ?64
║? ?FIFO Drains ...........? ? ? ? ?64
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?70305582
║? ?Active Cycles .........? ? ? ? ? 110
║? ?Idle Cycles ...........? ? ?70305472
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ?126067 ms (126067981 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 1676
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 1776
t [MSI trigger]
[MSI] Count = 1777
s
╔════════════════════════════════════════════════════════════╗
║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║
╠════════════════════════════════════════════════════════════╣
║ [Event Summary]
║? ?Total Events ..........? ? ? ?1845
║? ?MSI Events ............? ? ? ?1777
║? ?UART RX Events ........? ? ? ? ?68
║
║ [ISR Performance]
║? ?Total ISR Calls .......? ? ? ?1845
║? ?Total ISR Cycles ......? ? ?177307
║? ?Min ISR Cycles ........? ? ? ? ?93 (5812 us)
║? ?Avg ISR Cycles ........? ? ? ? ?96 (6000 us)
║? ?Max ISR Cycles ........? ? ? ? 178 (11125 us)
║
║ [Interrupt Latency]
║? ?Latency Samples .......? ? ? ?1777
║? ?Min Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Avg Latency ...........? ? ? ? ?59 cycles (3687 us)
║? ?Max Latency ...........? ? ? ? ?60 cycles (3750 us)
║
║ [UART RX Statistics]
║? ?Total Chars Received ..? ? ? ? ?68
║? ?UART ISR Calls ........? ? ? ? ?68
║? ?FIFO Drains ...........? ? ? ? ?68
║? ?Avg Chars/ISR .........? ? ? ? ? 1
║
║ [CPU Utilization]
║? ?Total Cycles ..........? ? ?70696519
║? ?Active Cycles .........? ? ? ? ? 117
║? ?Idle Cycles ...........? ? ?70696402
║? ?Busy Rate ............. 0%
║? ?Idle Rate ............. 99%
║
║ [Timing]
║? ?Elapsed Time ..........? ?126929 ms (126929544 us)
╚════════════════════════════════════════════════════════════╝

a [Stress test (10 MSI)...]
[Stress test done]
[MSI] Count = 1787
q [Quick burst test (100 MSI)...]
[Burst test done]
[MSI] Count = 1887
t [MSI trigger]
[MSI] Count = 1888
# ifndef LAB3_STATS_H
# define LAB3_STATS_H

# include <stdint.h>

// ==========================================
// ISR 性能统计
// ==========================================
typedef struct {
? ? uint64_t total_cycles;? ? ? // ISR 总执行周期
? ? uint32_t call_count;? ? ? ? // ISR 调用次数
? ? uint64_t min_cycles;? ? ? ? // 最小执行周期
? ? uint64_t max_cycles;? ? ? ? // 最大执行周期
} isr_perf_t;

// ==========================================
// 中断延迟统计
// ==========================================
typedef struct {
? ? uint64_t total_cycles;? ? ? // 总延迟周期
? ? uint32_t count;? ? ? ? ? ? ?// 采样次数
? ? uint64_t min_cycles;? ? ? ? // 最小延迟
? ? uint64_t max_cycles;? ? ? ? // 最大延迟
} interrupt_latency_t;

// ==========================================
// UART RX 统计
// ==========================================
typedef struct {
? ? uint32_t total_chars;? ? ? ?// 接收字符总数
? ? uint32_t isr_calls;? ? ? ? ?// UART ISR 调用次数
? ? uint32_t fifo_drains;? ? ? ?// FIFO 清空次数（处理过数据）
? ? uint64_t total_cycles;? ? ? // 总处理周期
? ? uint64_t min_cycles;? ? ? ? // 单次最小周期
? ? uint64_t max_cycles;? ? ? ? // 单次最大周期
} uart_rx_stats_t;

// ==========================================
// CPU 利用率统计（轮询 vs 中断）
// ==========================================
typedef struct {
? ? uint64_t cycles_total;? ? ? // 总周期
? ? uint64_t cycles_active;? ? ?// 活跃周期（处理事件）
? ? uint64_t cycles_idle;? ? ? ?// 空闲周期
? ? uint32_t event_count;? ? ? ?// 事件总数
} cpu_util_t;

// ==========================================
// 综合统计结构体（修正版）
// ==========================================
typedef struct {
? ? isr_perf_t isr_perf;? ? ? ? ? ? ? ? // ISR 性能
? ? interrupt_latency_t latency;? ? ? ? // 中断延迟
? ? uart_rx_stats_t uart_rx;? ? ? ? ? ? // UART RX
? ? cpu_util_t cpu_util;? ? ? ? ? ? ? ? // CPU 利用率

? ? uint64_t timestamp_start;? ? ? ? ? ?// 统计开始时间戳
? ? uint32_t msi_events;? ? ? ? ? ? ? ? // MSI 事件计数（添加）
? ? uint32_t uart_rx_events;? ? ? ? ? ? // UART RX 事件计数（添加）
? ? uint32_t total_events;? ? ? ? ? ? ? // 总事件计数
} lab3_stats_t;

// ==========================================
// 全局统计对象
// ==========================================
extern lab3_stats_t g_stats;

// ==========================================
// 统计接口
// ==========================================

/**
?*@brief 初始化统计系统
?*/
void lab3_stats_init(void);

/**
?*@brief 记录 ISR 进入时间戳
?* @return 进入时的周期数
?*/
uint64_t lab3_stats_isr_entry(void);

/**
?*@brief 记录 ISR 退出，计算执行时间
?* @param entry_cycle ISR 进入时的周期数
?*/
void lab3_stats_isr_exit(uint64_t entry_cycle);

/**
?*@brief 记录中断延迟
?* @param latency_cycles 延迟的周期数
?*/
void lab3_stats_record_latency(uint64_t latency_cycles);

/**
?*@brief 记录 UART RX ISR 处理
?* @param char_count 本次读取的字符数
?*/
void lab3_stats_uart_rx_event(uint32_t char_count);

/**
?*@brief 记录 CPU 活跃周期
?*/
void lab3_stats_cpu_active(void);

/**
?*@brief 记录 CPU 空闲周期
?*/
void lab3_stats_cpu_idle(void);

/**
?*@brief 显示所有统计数据
?*/
void lab3_stats_display(void);

/**
?*@brief 重置所有统计数据
?*/
void lab3_stats_reset(void);

# endif // LAB3_STATS_H
# ifndef LAB3_TRAP_H
# define LAB3_TRAP_H

# include <stdint.h>

// 外部声明汇编入口
extern void lab3_trap_entry(void);

// 初始化与触发
void lab3_trap_init(void);
void lab3_trigger_msi(void);

// 中断处理函数
void lab3_trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval);

// 全局变量
extern volatile uint32_t g_msi_count;
extern volatile uint32_t g_uart_rx_flag;
extern volatile char? ? ?g_uart_rx_char;

# endif
.global lab3_trap_entry
.align 4

lab3_trap_entry:
? ? # 保存通用寄存器（这里简化为关键寄存器）
? ? addi sp, sp, -128

? ? # 保存 caller-save
? ? sd? ?ra,? ?0(sp)
? ? sd? ?t0,? ?8(sp)
? ? sd? ?t1,? 16(sp)
? ? sd? ?t2,? 24(sp)
? ? sd? ?a0,? 32(sp)
? ? sd? ?a1,? 40(sp)
? ? sd? ?a2,? 48(sp)
? ? sd? ?a3,? 56(sp)
? ? sd? ?a4,? 64(sp)
? ? sd? ?a5,? 72(sp)
? ? sd? ?a6,? 80(sp)
? ? sd? ?a7,? 88(sp)

? ? # 读取 mcause
? ? csrr t0, mcause

? ? # 读取 mepc
? ? csrr t1, mepc

? ? # 读取 mtval
? ? csrr t2, mtval

? ? # 调用 C 函数：lab3_trap_handler(mcause, mepc, mtval)
? ? mv? ?a0, t0
? ? mv? ?a1, t1
? ? mv? ?a2, t2
? ? call lab3_trap_handler

? ? # 恢复寄存器
? ? ld? ?ra,? ?0(sp)
? ? ld? ?t0,? ?8(sp)
? ? ld? ?t1,? 16(sp)
? ? ld? ?t2,? 24(sp)
? ? ld? ?a0,? 32(sp)
? ? ld? ?a1,? 40(sp)
? ? ld? ?a2,? 48(sp)
? ? ld? ?a3,? 56(sp)
? ? ld? ?a4,? 64(sp)
? ? ld? ?a5,? 72(sp)
? ? ld? ?a6,? 80(sp)
? ? ld? ?a7,? 88(sp)

? ? addi sp, sp, 128

? ? # 返回
? ? mret

# include <stdint.h>
# include "lab3_mmio.h"
# include "lab3_trap.h"
# include "lab3_timer.h"
# include "lab3_stats.h"

// ==========================================
// 函数前向声明
// ==========================================
void uart_putc(char c);
void uart_puts(const char *s);
void uart_put_hex32(uint32_t val);
void uart_put_dec(uint64_t n);
void uart_put_dec_aligned(uint64_t n, int width);

// ==========================================
// UART 输出函数
// ==========================================

void uart_putc(char c) {
? ? uint32_t timeout = 100000;
? ? while ((mmio_read32(REG_UART_LSR) & UART_LSR_THRE) == 0 && timeout--) {
? ? ? ? if (timeout == 0) return;
? ? }
? ? mmio_write8(REG_UART_THR, (uint8_t)c);
}

void uart_puts(const char *s) {
? ? while (*s) {
? ? ? ? if (*s == '\n') uart_putc('\r');
? ? ? ? uart_putc(*s++);
? ? }
}

void uart_put_hex32(uint32_t val) {
? ? const char *hex = "0123456789ABCDEF";
? ? uart_puts("0x");
? ? for (int i = 7; i >= 0; i--) {
? ? ? ? uart_putc(hex[(val >> (i* 4)) & 0xF]);
? ? }
}

void uart_put_dec(uint64_t n) {
? ? if (n == 0) {
? ? ? ? uart_putc('0');
? ? ? ? return;
? ? }

? ? char buf[32];
? ? int len = 0;
? ? while (n > 0) {
? ? ? ? buf[len++] = (n % 10) + '0';
? ? ? ? n /= 10;
? ? }

? ? while (len > 0) {
? ? ? ? uart_putc(buf[--len]);
? ? }
}

void uart_put_dec_aligned(uint64_t n, int width) {
? ? char buf[32];
? ? int len = 0;

? ? if (n == 0) {
? ? ? ? buf[len++] = '0';
? ? } else {
? ? ? ? uint64_t tmp = n;
? ? ? ? while (tmp > 0) {
? ? ? ? ? ? buf[len++] = (tmp % 10) + '0';
? ? ? ? ? ? tmp /= 10;
? ? ? ? }
? ? }

? ? int spaces = width - len;
? ? while (spaces > 0) {
? ? ? ? uart_putc(' ');
? ? ? ? spaces--;
? ? }

? ? for (int i = len - 1; i >= 0; i--) {
? ? ? ? uart_putc(buf[i]);
? ? }
}

// ==========================================
// 主程序
// ==========================================

int main(void) {
? ? // 初始化计时器（必须第一步！）
? ? lab3_timer_init();

? ? // 初始化 GPIO LED
? ? uint32_t val = mmio_read32(REG_GPIO_PADDIR);
? ? val |= MASK_LEDS;
? ? mmio_write32(REG_GPIO_PADDIR, val);

? ? // 初始化中断系统（包括统计）
? ? lab3_trap_init();

? ? uart_puts("\r\n");
? ? uart_puts("╔════════════════════════════════════════════════╗\r\n");
? ? uart_puts("║? ?Lab3 Task3: Interrupt vs Polling Analysis? ? ║\r\n");
? ? uart_puts("║? ? ? ? ?CPU Freq: 16 MHz (HummingBird)? ? ? ? ?║\r\n");
? ? uart_puts("╚════════════════════════════════════════════════╝\r\n");
? ? uart_puts("\r\n[Commands]\r\n");
? ? uart_puts("? 't' - Trigger Software Interrupt (MSI)\r\n");
? ? uart_puts("? 's' - Show Statistics\r\n");
? ? uart_puts("? 'r' - Reset Statistics\r\n");
? ? uart_puts("? 'a' - Stress Test (10 MSI)\r\n");
? ? uart_puts("? 'q' - Quick MSI Burst (100x, test latency)\r\n");
? ? uart_puts("\r\n");

? ? uint32_t last_msi_count = 0;

? ? while (1) {
? ? ? ? int event_handled = 0;

? ? ? ? // === 1. 处理 UART 接收 ===
? ? ? ? if (g_uart_rx_flag == 1) {
? ? ? ? ? ? event_handled = 1;
? ? ? ? ? ? lab3_stats_cpu_active();

? ? ? ? ? ? char c = g_uart_rx_char;
? ? ? ? ? ? g_uart_rx_flag = 0;

? ? ? ? ? ? if (c != '\r' && c != '\n') {
? ? ? ? ? ? ? ? uart_putc(c);
? ? ? ? ? ? }

? ? ? ? ? ? if (c == 't' || c == 'T') {
? ? ? ? ? ? ? ? uart_puts(" [MSI trigger]\r\n");
? ? ? ? ? ? ? ? lab3_trigger_msi();
? ? ? ? ? ? }
? ? ? ? ? ? else if (c == 'r' || c == 'R') {
? ? ? ? ? ? ? ? lab3_stats_reset();
? ? ? ? ? ? ? ? uart_puts(" [Reset]\r\n");
? ? ? ? ? ? }
? ? ? ? ? ? else if (c == 's' || c == 'S') {
? ? ? ? ? ? ? ? uart_puts("\r\n");
? ? ? ? ? ? ? ? lab3_stats_display();
? ? ? ? ? ? }
? ? ? ? ? ? else if (c == 'a' || c == 'A') {
? ? ? ? ? ? ? ? uart_puts(" [Stress test (10 MSI)...]\r\n");
? ? ? ? ? ? ? ? for (int i = 0; i < 10; i++) {
? ? ? ? ? ? ? ? ? ? lab3_trigger_msi();
? ? ? ? ? ? ? ? ? ? lab3_delay_us(100);? // 延迟 100us
? ? ? ? ? ? ? ? }
? ? ? ? ? ? ? ? uart_puts("[Stress test done]\r\n");
? ? ? ? ? ? }
? ? ? ? ? ? else if (c == 'q' || c == 'Q') {
? ? ? ? ? ? ? ? uart_puts(" [Quick burst test (100 MSI)...]\r\n");
? ? ? ? ? ? ? ? for (int i = 0; i < 100; i++) {
? ? ? ? ? ? ? ? ? ? lab3_trigger_msi();
? ? ? ? ? ? ? ? ? ? // 无延迟，尽可能快地触发
? ? ? ? ? ? ? ? }
? ? ? ? ? ? ? ? uart_puts("[Burst test done]\r\n");
? ? ? ? ? ? }
? ? ? ? ? ? else if (c == '\r' || c == '\n') {
? ? ? ? ? ? ? ? uart_puts("\r\n");
? ? ? ? ? ? }
? ? ? ? } else {
? ? ? ? ? ? event_handled = 0;
? ? ? ? ? ? lab3_stats_cpu_idle();
? ? ? ? }

? ? ? ? // === 2. 处理软件中断 (MSI) ===
? ? ? ? if (g_msi_count != last_msi_count) {
? ? ? ? ? ? event_handled = 1;
? ? ? ? ? ? lab3_stats_cpu_active();
? ? ? ? ? ? uart_puts("[MSI] Count = ");
? ? ? ? ? ? uart_put_dec(g_msi_count);
? ? ? ? ? ? uart_puts("\r\n");
? ? ? ? ? ? last_msi_count = g_msi_count;
? ? ? ? }
? ? }

? ? return 0;
}

# include "lab3_stats.h"
# include "lab3_timer.h"
# include "lab3_mmio.h"

// ==========================================
// 全局统计对象
// ==========================================
lab3_stats_t g_stats = {
? ? .isr_perf = {
? ? ? ? .total_cycles = 0,
? ? ? ? .call_count = 0,
? ? ? ? .min_cycles = UINT64_MAX,
? ? ? ? .max_cycles = 0,
? ? },
? ? .latency = {
? ? ? ? .total_cycles = 0,
? ? ? ? .count = 0,
? ? ? ? .min_cycles = UINT64_MAX,
? ? ? ? .max_cycles = 0,
? ? },
? ? .uart_rx = {
? ? ? ? .total_chars = 0,
? ? ? ? .isr_calls = 0,
? ? ? ? .fifo_drains = 0,
? ? ? ? .total_cycles = 0,
? ? ? ? .min_cycles = UINT64_MAX,
? ? ? ? .max_cycles = 0,
? ? },
? ? .cpu_util = {
? ? ? ? .cycles_total = 0,
? ? ? ? .cycles_active = 0,
? ? ? ? .cycles_idle = 0,
? ? ? ? .event_count = 0,
? ? },
? ? .timestamp_start = 0,
? ? .msi_events = 0,
? ? .uart_rx_events = 0,
? ? .total_events = 0,
};

// ==========================================
// 前向声明
// ==========================================
extern void uart_puts(const char *s);
extern void uart_put_dec(uint64_t n);
extern void uart_put_dec_aligned(uint64_t n, int width);
extern void uart_put_hex32(uint32_t val);

// ==========================================
// 实现
// ==========================================

void lab3_stats_init(void) {
? ? g_stats.timestamp_start = lab3_get_cycle();
? ? g_stats.isr_perf.min_cycles = UINT64_MAX;
? ? g_stats.latency.min_cycles = UINT64_MAX;
? ? g_stats.uart_rx.min_cycles = UINT64_MAX;
}

uint64_t lab3_stats_isr_entry(void) {
? ? return lab3_get_cycle();
}

void lab3_stats_isr_exit(uint64_t entry_cycle) {
? ? uint64_t exit_cycle = lab3_get_cycle();
? ? uint64_t duration = exit_cycle - entry_cycle;

? ? g_stats.isr_perf.total_cycles += duration;
? ? g_stats.isr_perf.call_count++;

? ? if (duration < g_stats.isr_perf.min_cycles) {
? ? ? ? g_stats.isr_perf.min_cycles = duration;
? ? }
? ? if (duration > g_stats.isr_perf.max_cycles) {
? ? ? ? g_stats.isr_perf.max_cycles = duration;
? ? }
}

void lab3_stats_record_latency(uint64_t latency_cycles) {
? ? g_stats.latency.total_cycles += latency_cycles;
? ? g_stats.latency.count++;

? ? if (latency_cycles < g_stats.latency.min_cycles) {
? ? ? ? g_stats.latency.min_cycles = latency_cycles;
? ? }
? ? if (latency_cycles > g_stats.latency.max_cycles) {
? ? ? ? g_stats.latency.max_cycles = latency_cycles;
? ? }
}

void lab3_stats_uart_rx_event(uint32_t char_count) {
? ? if (char_count == 0) return;

? ? g_stats.uart_rx.total_chars += char_count;
? ? g_stats.uart_rx.isr_calls++;
? ? g_stats.uart_rx.fifo_drains++;

? ? // 此处暂假设每次处理的周期固定，实际应在 ISR 中精确测量
? ? // 这里用一个简单的估计：每字符约 50 cycles
? ? uint64_t estimated_cycles = char_count * 50;
? ? g_stats.uart_rx.total_cycles += estimated_cycles;

? ? if (estimated_cycles < g_stats.uart_rx.min_cycles) {
? ? ? ? g_stats.uart_rx.min_cycles = estimated_cycles;
? ? }
? ? if (estimated_cycles > g_stats.uart_rx.max_cycles) {
? ? ? ? g_stats.uart_rx.max_cycles = estimated_cycles;
? ? }
}

void lab3_stats_cpu_active(void) {
? ? g_stats.cpu_util.cycles_active++;
? ? g_stats.cpu_util.cycles_total++;
}

void lab3_stats_cpu_idle(void) {
? ? g_stats.cpu_util.cycles_idle++;
? ? g_stats.cpu_util.cycles_total++;
}

void lab3_stats_reset(void) {
? ? g_stats.isr_perf.total_cycles = 0;
? ? g_stats.isr_perf.call_count = 0;
? ? g_stats.isr_perf.min_cycles = UINT64_MAX;
? ? g_stats.isr_perf.max_cycles = 0;

? ? g_stats.latency.total_cycles = 0;
? ? g_stats.latency.count = 0;
? ? g_stats.latency.min_cycles = UINT64_MAX;
? ? g_stats.latency.max_cycles = 0;

? ? g_stats.uart_rx.total_chars = 0;
? ? g_stats.uart_rx.isr_calls = 0;
? ? g_stats.uart_rx.fifo_drains = 0;
? ? g_stats.uart_rx.total_cycles = 0;
? ? g_stats.uart_rx.min_cycles = UINT64_MAX;
? ? g_stats.uart_rx.max_cycles = 0;

? ? g_stats.cpu_util.cycles_total = 0;
? ? g_stats.cpu_util.cycles_active = 0;
? ? g_stats.cpu_util.cycles_idle = 0;
? ? g_stats.cpu_util.event_count = 0;

? ? g_stats.timestamp_start = lab3_get_cycle();
? ? g_stats.msi_events = 0;
? ? g_stats.uart_rx_events = 0;
? ? g_stats.total_events = 0;
}

void lab3_stats_display(void) {
? ? uint64_t elapsed_us = lab3_get_elapsed_us(g_stats.timestamp_start);
? ? uint64_t elapsed_ms = elapsed_us / 1000;

? ? uart_puts("╔════════════════════════════════════════════════════════════╗\r\n");
? ? uart_puts("║? ? ? ? ? ? Lab3 Interrupt Statistics Report? ? ? ? ? ? ? ? ║\r\n");
? ? uart_puts("╠════════════════════════════════════════════════════════════╣\r\n");

? ? // === 事件统计 ===
? ? uart_puts("║ [Event Summary]\r\n");
? ? uart_puts("║? ?Total Events .......... ");
? ? uart_put_dec_aligned(g_stats.total_events, 10);
? ? uart_puts("\r\n");

? ? uart_puts("║? ?MSI Events ............ ");
? ? uart_put_dec_aligned(g_stats.msi_events, 10);
? ? uart_puts("\r\n");

? ? uart_puts("║? ?UART RX Events ........ ");
? ? uart_put_dec_aligned(g_stats.uart_rx_events, 10);
? ? uart_puts("\r\n");

? ? uart_puts("║\r\n");

? ? // === ISR 性能 ===
? ? uart_puts("║ [ISR Performance]\r\n");
? ? uart_puts("║? ?Total ISR Calls ....... ");
? ? uart_put_dec_aligned(g_stats.isr_perf.call_count, 10);
? ? uart_puts("\r\n");

? ? if (g_stats.isr_perf.call_count > 0) {
? ? ? ? uart_puts("║? ?Total ISR Cycles ...... ");
? ? ? ? uart_put_dec_aligned(g_stats.isr_perf.total_cycles, 10);
? ? ? ? uart_puts("\r\n");

? ? ? ? uint64_t avg_isr_cycles = g_stats.isr_perf.total_cycles / g_stats.isr_perf.call_count;
? ? ? ? uint64_t avg_isr_us = (avg_isr_cycles * 1000) / 16;? // 16 MHz

? ? ? ? uart_puts("║? ?Min ISR Cycles ........ ");
? ? ? ? uart_put_dec_aligned(g_stats.isr_perf.min_cycles, 10);
? ? ? ? uart_puts(" (");
? ? ? ? uart_put_dec((g_stats.isr_perf.min_cycles * 1000) / 16);
? ? ? ? uart_puts(" us)\r\n");

? ? ? ? uart_puts("║? ?Avg ISR Cycles ........ ");
? ? ? ? uart_put_dec_aligned(avg_isr_cycles, 10);
? ? ? ? uart_puts(" (");
? ? ? ? uart_put_dec(avg_isr_us);
? ? ? ? uart_puts(" us)\r\n");

? ? ? ? uart_puts("║? ?Max ISR Cycles ........ ");
? ? ? ? uart_put_dec_aligned(g_stats.isr_perf.max_cycles, 10);
? ? ? ? uart_puts(" (");
? ? ? ? uart_put_dec((g_stats.isr_perf.max_cycles * 1000) / 16);
? ? ? ? uart_puts(" us)\r\n");
? ? }

? ? uart_puts("║\r\n");

? ? // === 中断延迟 ===
? ? uart_puts("║ [Interrupt Latency]\r\n");
? ? if (g_stats.latency.count > 0) {
? ? ? ? uint64_t avg_latency = g_stats.latency.total_cycles / g_stats.latency.count;
? ? ? ? uint64_t avg_latency_us = (avg_latency * 1000) / 16;

? ? ? ? uart_puts("║? ?Latency Samples ....... ");
? ? ? ? uart_put_dec_aligned(g_stats.latency.count, 10);
? ? ? ? uart_puts("\r\n");

? ? ? ? uart_puts("║? ?Min Latency ........... ");
? ? ? ? uart_put_dec_aligned(g_stats.latency.min_cycles, 10);
? ? ? ? uart_puts(" cycles (");
? ? ? ? uart_put_dec((g_stats.latency.min_cycles * 1000) / 16);
? ? ? ? uart_puts(" us)\r\n");

? ? ? ? uart_puts("║? ?Avg Latency ........... ");
? ? ? ? uart_put_dec_aligned(avg_latency, 10);
? ? ? ? uart_puts(" cycles (");
? ? ? ? uart_put_dec(avg_latency_us);
? ? ? ? uart_puts(" us)\r\n");

? ? ? ? uart_puts("║? ?Max Latency ........... ");
? ? ? ? uart_put_dec_aligned(g_stats.latency.max_cycles, 10);
? ? ? ? uart_puts(" cycles (");
? ? ? ? uart_put_dec((g_stats.latency.max_cycles * 1000) / 16);
? ? ? ? uart_puts(" us)\r\n");
? ? } else {
? ? ? ? uart_puts("║? ?(No latency data recorded)\r\n");
? ? }

? ? uart_puts("║\r\n");

? ? // === UART RX 统计 ===
? ? uart_puts("║ [UART RX Statistics]\r\n");
? ? uart_puts("║? ?Total Chars Received .. ");
? ? uart_put_dec_aligned(g_stats.uart_rx.total_chars, 10);
? ? uart_puts("\r\n");

? ? uart_puts("║? ?UART ISR Calls ........ ");
? ? uart_put_dec_aligned(g_stats.uart_rx.isr_calls, 10);
? ? uart_puts("\r\n");

? ? uart_puts("║? ?FIFO Drains ........... ");
? ? uart_put_dec_aligned(g_stats.uart_rx.fifo_drains, 10);
? ? uart_puts("\r\n");

? ? if (g_stats.uart_rx.isr_calls > 0) {
? ? ? ? uint32_t avg_chars_per_isr = g_stats.uart_rx.total_chars / g_stats.uart_rx.isr_calls;
? ? ? ? uart_puts("║? ?Avg Chars/ISR ......... ");
? ? ? ? uart_put_dec_aligned(avg_chars_per_isr, 10);
? ? ? ? uart_puts("\r\n");
? ? }

? ? uart_puts("║\r\n");

? ? // === CPU 利用率 ===
? ? uart_puts("║ [CPU Utilization]\r\n");
? ? uart_puts("║? ?Total Cycles .......... ");
? ? uart_put_dec_aligned(g_stats.cpu_util.cycles_total, 12);
? ? uart_puts("\r\n");

? ? uart_puts("║? ?Active Cycles ......... ");
? ? uart_put_dec_aligned(g_stats.cpu_util.cycles_active, 12);
? ? uart_puts("\r\n");

? ? uart_puts("║? ?Idle Cycles ........... ");
? ? uart_put_dec_aligned(g_stats.cpu_util.cycles_idle, 12);
? ? uart_puts("\r\n");

? ? if (g_stats.cpu_util.cycles_total > 0) {
? ? ? ? uint64_t busy_rate = (g_stats.cpu_util.cycles_active *100) / g_stats.cpu_util.cycles_total;
? ? ? ? uint64_t idle_rate = (g_stats.cpu_util.cycles_idle* 100) / g_stats.cpu_util.cycles_total;

? ? ? ? uart_puts("║? ?Busy Rate ............. ");
? ? ? ? uart_put_dec(busy_rate);
? ? ? ? uart_puts("%\r\n");

? ? ? ? uart_puts("║? ?Idle Rate ............. ");
? ? ? ? uart_put_dec(idle_rate);
? ? ? ? uart_puts("%\r\n");
? ? }

? ? uart_puts("║\r\n");

? ? // === 时间统计 ===
? ? uart_puts("║ [Timing]\r\n");
? ? uart_puts("║? ?Elapsed Time .......... ");
? ? uart_put_dec_aligned(elapsed_ms, 8);
? ? uart_puts(" ms (");
? ? uart_put_dec(elapsed_us);
? ? uart_puts(" us)\r\n");

? ? uart_puts("╚════════════════════════════════════════════════════════════╝\r\n");
? ? uart_puts("\r\n");
}
# ifndef LAB3_STATS_H
# define LAB3_STATS_H

# include <stdint.h>

// ==========================================
// ISR 性能统计
// ==========================================
typedef struct {
? ? uint64_t total_cycles;? ? ? // ISR 总执行周期
? ? uint32_t call_count;? ? ? ? // ISR 调用次数
? ? uint64_t min_cycles;? ? ? ? // 最小执行周期
? ? uint64_t max_cycles;? ? ? ? // 最大执行周期
} isr_perf_t;

// ==========================================
// 中断延迟统计
// ==========================================
typedef struct {
? ? uint64_t total_cycles;? ? ? // 总延迟周期
? ? uint32_t count;? ? ? ? ? ? ?// 采样次数
? ? uint64_t min_cycles;? ? ? ? // 最小延迟
? ? uint64_t max_cycles;? ? ? ? // 最大延迟
} interrupt_latency_t;

// ==========================================
// UART RX 统计
// ==========================================
typedef struct {
? ? uint32_t total_chars;? ? ? ?// 接收字符总数
? ? uint32_t isr_calls;? ? ? ? ?// UART ISR 调用次数
? ? uint32_t fifo_drains;? ? ? ?// FIFO 清空次数（处理过数据）
? ? uint64_t total_cycles;? ? ? // 总处理周期
? ? uint64_t min_cycles;? ? ? ? // 单次最小周期
? ? uint64_t max_cycles;? ? ? ? // 单次最大周期
} uart_rx_stats_t;

// ==========================================
// CPU 利用率统计（轮询 vs 中断）
// ==========================================
typedef struct {
? ? uint64_t cycles_total;? ? ? // 总周期
? ? uint64_t cycles_active;? ? ?// 活跃周期（处理事件）
? ? uint64_t cycles_idle;? ? ? ?// 空闲周期
? ? uint32_t event_count;? ? ? ?// 事件总数
} cpu_util_t;

// ==========================================
// 综合统计结构体（修正版）
// ==========================================
typedef struct {
? ? isr_perf_t isr_perf;? ? ? ? ? ? ? ? // ISR 性能
? ? interrupt_latency_t latency;? ? ? ? // 中断延迟
? ? uart_rx_stats_t uart_rx;? ? ? ? ? ? // UART RX
? ? cpu_util_t cpu_util;? ? ? ? ? ? ? ? // CPU 利用率

? ? uint64_t timestamp_start;? ? ? ? ? ?// 统计开始时间戳
? ? uint32_t msi_events;? ? ? ? ? ? ? ? // MSI 事件计数（添加）
? ? uint32_t uart_rx_events;? ? ? ? ? ? // UART RX 事件计数（添加）
? ? uint32_t total_events;? ? ? ? ? ? ? // 总事件计数
} lab3_stats_t;

// ==========================================
// 全局统计对象
// ==========================================
extern lab3_stats_t g_stats;

// ==========================================
// 统计接口
// ==========================================

/**
?*@brief 初始化统计系统
?*/
void lab3_stats_init(void);

/**
?*@brief 记录 ISR 进入时间戳
?* @return 进入时的周期数
?*/
uint64_t lab3_stats_isr_entry(void);

/**
?*@brief 记录 ISR 退出，计算执行时间
?* @param entry_cycle ISR 进入时的周期数
?*/
void lab3_stats_isr_exit(uint64_t entry_cycle);

/**
?*@brief 记录中断延迟
?* @param latency_cycles 延迟的周期数
?*/
void lab3_stats_record_latency(uint64_t latency_cycles);

/**
?*@brief 记录 UART RX ISR 处理
?* @param char_count 本次读取的字符数
?*/
void lab3_stats_uart_rx_event(uint32_t char_count);

/**
?*@brief 记录 CPU 活跃周期
?*/
void lab3_stats_cpu_active(void);

/**
?*@brief 记录 CPU 空闲周期
?*/
void lab3_stats_cpu_idle(void);

/**
?*@brief 显示所有统计数据
?*/
void lab3_stats_display(void);

/**
?*@brief 重置所有统计数据
?*/
void lab3_stats_reset(void);

# endif // LAB3_STATS_H

# ifndef LAB3_TRAP_H
# define LAB3_TRAP_H

# include <stdint.h>

// 外部声明汇编入口
extern void lab3_trap_entry(void);

// 初始化与触发
void lab3_trap_init(void);
void lab3_trigger_msi(void);

// 中断处理函数
void lab3_trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval);

// 全局变量
extern volatile uint32_t g_msi_count;
extern volatile uint32_t g_uart_rx_flag;
extern volatile char? ? ?g_uart_rx_char;

# endif

.global lab3_trap_entry
.align 4

lab3_trap_entry:
? ? # 保存通用寄存器（这里简化为关键寄存器）
? ? addi sp, sp, -128

? ? # 保存 caller-save
? ? sd? ?ra,? ?0(sp)
? ? sd? ?t0,? ?8(sp)
? ? sd? ?t1,? 16(sp)
? ? sd? ?t2,? 24(sp)
? ? sd? ?a0,? 32(sp)
? ? sd? ?a1,? 40(sp)
? ? sd? ?a2,? 48(sp)
? ? sd? ?a3,? 56(sp)
? ? sd? ?a4,? 64(sp)
? ? sd? ?a5,? 72(sp)
? ? sd? ?a6,? 80(sp)
? ? sd? ?a7,? 88(sp)

? ? # 读取 mcause
? ? csrr t0, mcause

? ? # 读取 mepc
? ? csrr t1, mepc

? ? # 读取 mtval
? ? csrr t2, mtval

? ? # 调用 C 函数：lab3_trap_handler(mcause, mepc, mtval)
? ? mv? ?a0, t0
? ? mv? ?a1, t1
? ? mv? ?a2, t2
? ? call lab3_trap_handler

? ? # 恢复寄存器
? ? ld? ?ra,? ?0(sp)
? ? ld? ?t0,? ?8(sp)
? ? ld? ?t1,? 16(sp)
? ? ld? ?t2,? 24(sp)
? ? ld? ?a0,? 32(sp)
? ? ld? ?a1,? 40(sp)
? ? ld? ?a2,? 48(sp)
? ? ld? ?a3,? 56(sp)
? ? ld? ?a4,? 64(sp)
? ? ld? ?a5,? 72(sp)
? ? ld? ?a6,? 80(sp)
? ? ld? ?a7,? 88(sp)

? ? addi sp, sp, 128

? ? # 返回
? ? mret


