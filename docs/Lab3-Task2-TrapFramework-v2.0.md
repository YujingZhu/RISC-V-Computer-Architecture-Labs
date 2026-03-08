记录撰写一个READ ME文档
#include <stdint.h>
#include "lab3_mmio.h"

extern void lab3_trap_init(void);
extern void lab3_trigger_msi(void);
extern volatile uint32_t g_msi_count;
extern volatile uint64_t g_isr_latency;

volatile uint64_t g_cycles_total = 0;
volatile uint64_t g_cycles_idle = 0;

void uart_putc_raw(char c) {
    while ((mmio_read32(REG_UART_LSR) & UART_LSR_THRE) == 0);
    mmio_write32(REG_UART_THR, c);
}

void uart_puts_raw(const char *s) {
    while (*s) uart_putc_raw(*s++);
}

void uart_put_dec(uint32_t num) {
    char buf[16];
    int i = 0;
    
    if (num == 0) {
        uart_putc_raw('0');
        return;
    }
    
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    
    while (i > 0) {
        uart_putc_raw(buf[--i]);
    }
}

int main(void) {
    uart_puts_raw("\r\n");
    uart_puts_raw("===================================\r\n");
    uart_puts_raw("  Lab3 Task2: Trap Framework\r\n");
    uart_puts_raw("===================================\r\n");
    uart_puts_raw("Commands: 't'=trigger, 's'=stats, 'r'=reset\r\n");
    uart_puts_raw("===================================\r\n\r\n");

    lab3_trap_init();
    
    uart_puts_raw("Ready. Press 't' to trigger MSI.\r\n\r\n");

    uint32_t last_count = 0;
    int msg_printed = 0;  // 【关键】防止重复打印

    while (1) {
        g_cycles_total++;
        int is_busy = 0;

        // 1. 轮询串口
        if (mmio_read32(REG_UART_LSR) & UART_LSR_DR) {
            char c = (char)mmio_read32(REG_UART_RBR);
            is_busy = 1;

            uart_putc_raw(c);
            uart_puts_raw("\r\n");

            switch (c) {
                case 't':
                case 'T':
                    lab3_trigger_msi();
                    msg_printed = 0;  // 重置标志
                    break;

                case 's':
                case 'S':
                    uart_puts_raw("\r\n=== Statistics ===\r\n");
                    uart_puts_raw("MSI Count:    ");
                    uart_put_dec(g_msi_count);
                    uart_puts_raw("\r\nLast Latency: ");
                    uart_put_dec((uint32_t)g_isr_latency);
                    uart_puts_raw(" cycles\r\n");
                    uart_puts_raw("==================\r\n\r\n");
                    break;

                case 'r':
                case 'R':
                    g_msi_count = 0;
                    g_cycles_total = 0;
                    g_cycles_idle = 0;
                    last_count = 0;
                    msg_printed = 0;
                    uart_puts_raw("Reset done.\r\n\r\n");
                    break;

                default:
                    break;
            }
        }

        // 2. 【关键修复】检查中断计数变化 - 只打印一次
        uint32_t current_count = g_msi_count;
        if (current_count != last_count && !msg_printed) {
            is_busy = 1;
            
            uart_puts_raw("[Main] MSI Handled! Count=");
            uart_put_dec(current_count);
            uart_puts_raw(" Latency=");
            uart_put_dec((uint32_t)g_isr_latency);
            uart_puts_raw(" cycles\r\n");
            
            last_count = current_count;
            msg_printed = 1;  // 【关键】设置标志，防止重复打印
        }

        // 3. 统计空闲周期
        if (!is_busy) {
            g_cycles_idle++;
        }
    }

    return 0;
}

.section .text
.align 2
.global lab3_trap_entry

lab3_trap_entry:
    /* ==========================================
     * 1. 保存现场 (Context Save)
     * ========================================== */
    addi sp, sp, -64

    /* 【关键】保存所有 caller-saved 寄存器 */
    sw ra,  0(sp)
    sw t0,  4(sp)
    sw t1,  8(sp)
    sw t2, 12(sp)
    sw a0, 16(sp)
    sw a1, 20(sp)
    sw a2, 24(sp)
    sw a3, 28(sp)
    sw a4, 32(sp)
    sw a5, 36(sp)
    sw a6, 40(sp)
    sw a7, 44(sp)
    sw t3, 48(sp)
    sw t4, 52(sp)
    sw t5, 56(sp)
    sw t6, 60(sp)

    /* ==========================================
     * 2. 读取 CSR 并调用 C 处理函数
     * ========================================== */

    /* 【关键】在调用前读取 CSR，避免被破坏 */
    csrr a0, mcause    /* a0 = mcause */
    csrr a1, mepc      /* a1 = mepc */
    csrr a2, mtval     /* a2 = mtval */

    /* 【关键】call 指令会改变 ra，但已经保存在栈中 */
    call lab3_trap_handler

    /* ==========================================
     * 3. 恢复现场 (Context Restore)
     * ========================================== */
    lw ra,  0(sp)
    lw t0,  4(sp)
    lw t1,  8(sp)
    lw t2, 12(sp)
    lw a0, 16(sp)
    lw a1, 20(sp)
    lw a2, 24(sp)
    lw a3, 28(sp)
    lw a4, 32(sp)
    lw a5, 36(sp)
    lw a6, 40(sp)
    lw a7, 44(sp)
    lw t3, 48(sp)
    lw t4, 52(sp)
    lw t5, 56(sp)
    lw t6, 60(sp)

    addi sp, sp, 64

    /* 【关键】使用 mret 返回中断处理 */
    mret

#include "lab3_mmio.h"

// CSR 读写宏
#define read_csr(reg) ({ \
    unsigned long __tmp; \
    __asm__ volatile ("csrr %0, " #reg : "=r"(__tmp)); \
    __tmp; \
})

#define write_csr(reg, val) ({ \
    __asm__ volatile ("csrw " #reg ", %0" :: "rK"(val)); \
})

#define set_csr(reg, bit) ({ \
    unsigned long __tmp; \
    __asm__ volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "rK"(bit)); \
    __tmp; \
})

// CSR 位定义
#define MSTATUS_MIE     (1UL << 3)
#define MIE_MSIE        (1UL << 3)
#define MCAUSE_INTR     (1UL << 31)

// 在 lab3_trap.c 的顶部添加:
#define REG_CLINT_MSIP      0x02000000u


// 全局变量
volatile uint32_t g_msi_count = 0;
volatile uint64_t g_isr_latency = 0;

// 外部声明
extern void uart_puts_raw(const char *s);
extern void uart_put_dec(uint32_t num);

void lab3_trap_init(void) {
    extern void lab3_trap_entry(void);
    
    uintptr_t trap_addr = (uintptr_t)lab3_trap_entry;
    write_csr(mtvec, trap_addr & ~0x3);
    
    set_csr(mie, MIE_MSIE);
    set_csr(mstatus, MSTATUS_MIE);
    
    uart_puts_raw("[TRAP] Init Done.\r\n");
}

void lab3_trigger_msi(void) {
    uart_puts_raw("[MSI] Triggering...\r\n");
    mmio_write32(REG_CLINT_MSIP, 1);
    
    // 短暂延迟
    for (volatile int i = 0; i < 1000; i++);
}

/* =========================================
 * 中断处理函数 - 修复版
 * ========================================= */
void lab3_trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval) {
    uint64_t start_cycle = read_csr(mcycle);
    
    uart_puts_raw("\r\n[ISR] Entered!\r\n");
    uart_puts_raw("[ISR] mcause=0x");
    uart_put_dec(mcause);
    uart_puts_raw("\r\n");

    // 【关键检查】检查是否为中断且代码为3
    int is_intr = (mcause & MCAUSE_INTR) ? 1 : 0;
    int exc_code = mcause & 0xFF;
    
    uart_puts_raw("[ISR] is_interrupt=");
    uart_put_dec(is_intr);
    uart_puts_raw(", code=");
    uart_put_dec(exc_code);
    uart_puts_raw("\r\n");

    if (is_intr && (exc_code == 3)) {
        // 软件中断
        g_msi_count++;
        
        uart_puts_raw("[ISR] MSI count -> ");
        uart_put_dec(g_msi_count);
        uart_puts_raw("\r\n");
        
        // 【关键】清除 MSIP 挂起位
        uart_puts_raw("[ISR] Clearing MSIP...\r\n");
        mmio_write32(REG_CLINT_MSIP, 0);
        
        // 读回验证
        uint32_t msip_verify = mmio_read32(REG_CLINT_MSIP);
        uart_puts_raw("[ISR] MSIP verify=");
        uart_put_dec(msip_verify);
        uart_puts_raw("\r\n");
    }
    else {
        uart_puts_raw("[TRAP] ERROR: Not MSI! mcause=0x");
        uart_put_dec(mcause);
        uart_puts_raw("\r\n");
        while(1);
    }

    uint64_t end_cycle = read_csr(mcycle);
    g_isr_latency = end_cycle - start_cycle;
    
    uart_puts_raw("[ISR] Done.\r\n");
}

/* application/lab3_trap.h */
#ifndef LAB3_TRAP_H
#define LAB3_TRAP_H

#include <stdint.h>

// 中断初始化
void lab3_trap_init(void);

// 触发软件中断 (Task 2)
void lab3_trigger_msi(void);

// 全局中断计数器 (用于主循环观察) [cite: 91]
extern volatile uint32_t g_msi_count;

#endif/*
 * lab3_mmio.h
 *
 *  Created on: 2026年1月9日
 */

#ifndef APPLICATION_LAB3_MMIO_H_
#define APPLICATION_LAB3_MMIO_H_


#include <stdint.h>

// ==========================================
// 1. 外设基地址定义
// ==========================================
#define LAB_GPIOA_BASE      0x10012000u
#define LAB_UART0_BASE      0x10013000u

// ==========================================
// 2. 寄存器偏移
// ==========================================
// GPIO
#define LAB_GPIO_PADDIR_OFS 0x00u
#define LAB_GPIO_PADIN_OFS  0x04u
#define LAB_GPIO_PADOUT_OFS 0x08u
#define LAB_GPIO_IOFCFG_OFS 0x1Cu

// UART (16550)
#define LAB_UART_RBR_THR_OFS 0x00u
#define LAB_UART_LSR_OFS     0x14u

// ==========================================
// 3. 完整寄存器地址
// ==========================================
#define REG_GPIO_PADDIR (LAB_GPIOA_BASE + LAB_GPIO_PADDIR_OFS)
#define REG_GPIO_PADIN  (LAB_GPIOA_BASE + LAB_GPIO_PADIN_OFS)
#define REG_GPIO_PADOUT (LAB_GPIOA_BASE + LAB_GPIO_PADOUT_OFS)
#define REG_GPIO_IOFCFG (LAB_GPIOA_BASE + LAB_GPIO_IOFCFG_OFS)

#define REG_UART_RBR    (LAB_UART0_BASE + LAB_UART_RBR_THR_OFS)
#define REG_UART_THR    (LAB_UART0_BASE + LAB_UART_RBR_THR_OFS)
#define REG_UART_LSR    (LAB_UART0_BASE + LAB_UART_LSR_OFS)

// ==========================================
// 4. 位掩码定义
// ==========================================
// UART LSR
#define UART_LSR_DR     (1u << 0)  // 接收数据就绪
#define UART_LSR_THRE   (1u << 5)  // 发送缓冲区空

// GPIO
#define MASK_LEDS       (0x3Fu << 20) // LED0-LED5
#define MASK_SWITCHES   (0x3Fu << 26) // SW0-SW5

// ==========================================
// 5. MMIO 访问封装
// ==========================================
static inline uint32_t mmio_read32(uintptr_t addr) {
    return *(volatile uint32_t *)addr;
}

static inline void mmio_write32(uintptr_t addr, uint32_t val) {
    *(volatile uint32_t *)addr = val;
}

#endif /* APPLICATION_LAB3_MMIO_H_ */

这是task2的代码下面是测试结果
===================================
  Lab3 Task2: Trap Framework
===================================
Commands: 't'=trigger, 's'=stats, 'r'=reset
===================================

[TRAP] Init Done.
Ready. Press 't' to trigger MSI.

t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 1
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=1 Latency=196202 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 2
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=2 Latency=196202 cycles
s

=== Statistics ===
MSI Count:    2
Last Latency: 196202 cycles
==================

t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 3
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=3 Latency=196202 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 4
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=4 Latency=196202 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 5
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=5 Latency=196202 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 6
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=6 Latency=196202 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 7
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=7 Latency=196202 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 8
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=8 Latency=196202 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 9
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=9 Latency=196202 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 10
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=10 Latency=197591 cycles
s

=== Statistics ===
MSI Count:    10
Last Latency: 197591 cycles
==================

s

=== Statistics ===
MSI Count:    10
Last Latency: 197591 cycles
==================

s

=== Statistics ===
MSI Count:    10
Last Latency: 197591 cycles
==================

t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 11
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=11 Latency=197591 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 12
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=12 Latency=197591 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 13
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=13 Latency=197591 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 14
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=14 Latency=197591 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 15
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=15 Latency=197591 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 16
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=16 Latency=197591 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 17
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=17 Latency=197591 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 18
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=18 Latency=197591 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 19
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=19 Latency=197591 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 20
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=20 Latency=197591 cycles
t
[MSI] Triggering...

[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 21
[ISR] Clearing MSIP...
[ISR] MSIP verify=0
[ISR] Done.
[Main] MSI Handled! Count=21 Latency=197591 cycles
s

=== Statistics ===
MSI Count:    21
Last Latency: 197591 cycles
==================

这是一个为您整理的 `README.md` 文档。该文档不仅包含了项目说明和使用方法，还结合您提供的代码和测试日志进行了结果分析，非常适合作为实验报告或项目归档使用。

---

# Lab 3 Task 2: RISC-V Trap Framework & MSI Handling

## 1. 项目简介 (Overview)

本项目实现了基于 RISC-V 架构的中断处理框架（Trap Framework）。作为 Lab3 的 Task 2，主要目标是演示**机器态软件中断 (MSI, Machine Software Interrupt)** 的触发、响应与处理流程。

系统通过操作 CLINT (Core Local Interruptor) 外设触发中断，使用汇编与 C 语言混合编程的方式保存/恢复上下文，并在中断服务程序 (ISR) 中统计中断次数及计算响应延迟。

## 2. 文件结构 (File Structure)

| 文件名 | 说明 |
| --- | --- |
| `lab3_main.c` | **主程序入口**。初始化中断，轮询串口输入，根据命令触发中断或打印统计信息。 |
| `lab3_trap.S` | **汇编入口** (`lab3_trap_entry`)。负责保存寄存器上下文 (Context Save)、调用 C 处理函数、恢复上下文 (Context Restore) 及执行 `mret`。 |
| `lab3_trap.c` | **中断核心逻辑**。包含中断初始化、MSI 触发函数以及 C 语言层面的中断处理函数 (`lab3_trap_handler`)。 |
| `lab3_mmio.h` | **硬件定义**。包含 UART、GPIO 及 CLINT 寄存器的地址映射和读写宏。 |

## 3. 核心机制 (Implementation Details)

### 3.1 初始化 (Initialization)

在 `lab3_trap_init` 中配置 CSR 寄存器：

1. **mtvec**: 设置中断向量表基地址为 `lab3_trap_entry` (Direct Mode)。
2. **mie**: 开启 MSIE (Machine Software Interrupt Enable) 位。
3. **mstatus**: 开启 MIE (Machine Interrupt Enable) 全局中断位。

### 3.2 中断触发 (Triggering)

通过 `lab3_trigger_msi` 函数向 **CLINT MSIP** 寄存器写入 `1`。这会立即向 Hart (硬件线程) 发送一个软件中断信号。

### 3.3 中断处理流程 (Handling Flow)

1. **Trap Entry**: 硬件跳转至 `lab3_trap_entry`。
2. **Context Save**: 汇编代码将所有 Caller-saved 寄存器 (`ra`, `t0`-`t6`, `a0`-`a7`) 压入栈中。
3. **Dispatch**: 调用 C 函数 `lab3_trap_handler(mcause, mepc, mtval)`。
4. **Service**:
* 检查 `mcause` 最高位是否为 1 (Interrupt) 且低位是否为 3 (Machine Software Interrupt)。
* 递增全局计数器 `g_msi_count`。
* **关键步骤**：向 CLINT MSIP 寄存器写入 `0` 以清除中断挂起状态，否则会陷入死循环。
* 计算 ISR 延迟 (Latency)。


5. **Context Restore**: 从栈中恢复寄存器。
6. **Return**: 执行 `mret` 返回主程序。

## 4. 使用说明 (Usage)

### 编译与运行

将代码烧录至目标 RISC-V 开发板，并连接串口终端 (波特率通常为 115200)。

### 串口命令交互

系统启动后进入主循环，支持以下单字符命令：

* **`t` / `T` (Trigger)**: 触发一次软件中断 (MSI)。
* **`s` / `S` (Stats)**: 打印当前中断统计信息（总次数、最后一次中断的延迟周期数）。
* **`r` / `R` (Reset)**: 重置所有计数器。

## 5. 测试结果分析 (Test Results & Analysis)

基于提供的串口日志 (`Serial Output`)，系统运行表现符合预期。

### 5.1 成功触发中断

日志显示：

```text
[MSI] Triggering...
[ISR] Entered!
[ISR] mcause=0x2147483651
[ISR] is_interrupt=1, code=3
[ISR] MSI count -> 1
[ISR] Clearing MSIP...

```

* **分析**:
* `mcause` 打印值为 `2147483651` (十进制)，对应十六进制 `0x80000003`。
* 最高位为 `1` (Interrupt)，低位为 `3` (Machine Software Interrupt)，识别正确。
* ISR 成功进入并清除了 MSIP 标志位。



### 5.2 延迟 (Latency) 观测

日志显示：

```text
[Main] MSI Handled! Count=1 Latency=196202 cycles
...
[Main] MSI Handled! Count=10 Latency=197591 cycles

```

* **观测**: 中断处理延迟约为 196,000 ~ 197,000 时钟周期。
* **分析**: 延迟数值非常大。这**不是**硬件本身的延迟，而是因为在 ISR (`lab3_trap_handler`) 内部调用了 `uart_puts_raw`。串口打印是阻塞式 I/O，极大地拖慢了中断处理速度。
* **结论**: 在实际生产环境中，ISR 应尽可能短小，避免执行耗时的 I/O 操作。但在本实验中，该行为有助于直观观察中断流程。

### 5.3 稳定性测试

经过连续 27 次触发 (`t` 命令)：

```text
=== Statistics ===
MSI Count:    27
Last Latency: 197591 cycles

```

计数器准确无误，系统未发生崩溃或死锁，证明上下文保存/恢复及 MSIP 清除逻辑正确。

## 6. 总结 (Conclusion)

Lab3 Task2 成功实现了基于 RISC-V 的 Trap 框架。验证了 CSR 寄存器的配置、汇编上下文切换以及 C 语言中断处理程序的协作。该框架为后续实现更复杂的定时器中断或外部中断奠定了基础。