
---

# Lab 3 Task 1: Polling Mode (轮询模式) 实验记录

## 1. MMIO 映射表 (Memory Map)

> 
> **修正说明**：根据指导文档要求，补充了代码中使用的宏定义名称 (代码宏名) 。
> 
> 

### 1.1 UART0 寄存器映射

| Peripheral | Base | Reg | Offset | R/W | 用途 | 代码宏名 |
| --- | --- | --- | --- | --- | --- | --- |
| **UART0** | 0x1001 3000 | **RBR** | 0x00 | R | RX 数据读取 (接收缓冲) | `REG_UART_RBR` |
| **UART0** | 0x1001 3000 | **THR** | 0x00 | W | TX 数据发送 (发送保持) | `REG_UART_THR` |
| **UART0** | 0x1001 3000 | **LSR** | 0x14 | R | 线路状态 (Bit0=DR, Bit5=THRE) | `REG_UART_LSR` |
| **UART0** | 0x1001 3000 | **IER** | 0x04 | R/W | 中断使能 (Task3使用) | `REG_UART_IER` |
| **UART0** | 0x1001 3000 | **IIR** | 0x08 | R | 中断识别 (Task3使用) | `REG_UART_IIR` |

### 1.2 GPIOA 寄存器映射

| Peripheral | Base | Reg | Offset | R/W | 用途 | 代码宏名 |
| --- | --- | --- | --- | --- | --- | --- |
| **GPIOA** | 0x1001 2000 | **PADDIR** | 0x00 | R/W | LED 输出/按键输入方向 | `REG_GPIO_PADDIR` |
| **GPIOA** | 0x1001 2000 | **PADIN** | 0x04 | R | 读取开关输入电平 | `REG_GPIO_PADIN` |
| **GPIOA** | 0x1001 2000 | **PADOUT** | 0x08 | R/W | 控制 LED 输出电平 | `REG_GPIO_PADOUT` |
| **GPIOA** | 0x1001 2000 | **IOFCFG** | 0x1C | R/W | IO 禁止复用功能配置 | `REG_GPIO_IOFCFG` |

---

## 2. 实验数据记录 (Experimental Data)

根据实验运行日志统计的关键性能指标 。

| 指标 (Metric) | 测量值 (Value) | 说明 / 计算依据 |
| --- | --- | --- |
| **轮询周期 (Tpoll)** | **56 cycles** | 实测值。CPU 执行一次主循环（含两次 MMIO 读取）所需时钟周期。 |
| **平均响应延迟** | **28 cycles** | 计算值。公式： (假设事件随机到达)。 |
| **最大响应延迟** | **56 cycles** | 计算值。公式： (最坏情况需等待一整圈)。 |
| **CPU 忙等比例** | **99%** | 实测值。公式：。 |
| **有效事件处理验证** | **通过** | 31个输入字符产生31次有效循环增量，验证无丢包。 |

---

## 附录 A：完整源代码 (Source Code)

### A.1 头文件 `lab3_mmio.h`

```c
/*
 * lab3_mmio.h
 * Created on: 2026年1月9日
 * Description: MMIO 寄存器地址定义与访问宏
 */

#ifndef APPLICATION_LAB3_MMIO_H_
#define APPLICATION_LAB3_MMIO_H_

#include <stdint.h>

// ==========================================
[cite_start]// 1. 外设基地址定义 [cite: 137, 139]
// ==========================================
#define LAB_GPIOA_BASE      0x10012000u
#define LAB_UART0_BASE      0x10013000u

// ==========================================
// 2. 寄存器偏移
// ==========================================
// GPIO
#define LAB_GPIO_PADDIR_OFS 0x00u
#define LAB_GPIO_PADIN_OFS  0x04u
#define LAB_GPIO_PADOUT_OFS 0x08u
#define LAB_GPIO_IOFCFG_OFS 0x1Cu

// UART (16550)
#define LAB_UART_RBR_THR_OFS 0x00u
#define LAB_UART_LSR_OFS     0x14u
// Task 3 预留
#define LAB_UART_IER_OFS     0x04u
#define LAB_UART_IIR_OFS     0x08u

// ==========================================
// 3. 完整寄存器地址 (代码宏名)
// ==========================================
#define REG_GPIO_PADDIR (LAB_GPIOA_BASE + LAB_GPIO_PADDIR_OFS)
#define REG_GPIO_PADIN  (LAB_GPIOA_BASE + LAB_GPIO_PADIN_OFS)
#define REG_GPIO_PADOUT (LAB_GPIOA_BASE + LAB_GPIO_PADOUT_OFS)
#define REG_GPIO_IOFCFG (LAB_GPIOA_BASE + LAB_GPIO_IOFCFG_OFS)

#define REG_UART_RBR    (LAB_UART0_BASE + LAB_UART_RBR_THR_OFS)
#define REG_UART_THR    (LAB_UART0_BASE + LAB_UART_RBR_THR_OFS)
#define REG_UART_LSR    (LAB_UART0_BASE + LAB_UART_LSR_OFS)
#define REG_UART_IER    (LAB_UART0_BASE + LAB_UART_IER_OFS)
#define REG_UART_IIR    (LAB_UART0_BASE + LAB_UART_IIR_OFS)

// ==========================================
// 4. 位掩码定义
// ==========================================
// UART LSR
[cite_start]#define UART_LSR_DR     (1u << 0)  // 接收数据就绪 [cite: 187]
[cite_start]#define UART_LSR_THRE   (1u << 5)  // 发送缓冲区空 [cite: 188]

// GPIO
#define MASK_LEDS       (0x3Fu << 20) // LED0-LED5
#define MASK_SWITCHES   (0x3Fu << 26) // SW0-SW5

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

```

### A.2 主程序 `main.c`

```c
/*
 * main.c
 * Description: Lab3 Task 1 - Polling Mode Implementation
 */
#include <stdio.h>
#include <stdint.h>
#include "lab3_mmio.h"

// ==========================================
// 全局统计变量
// ==========================================
volatile uint64_t g_cycles_total = 0; // 总循环次数
volatile uint64_t g_cycles_idle  = 0; // 空闲(无事可做)次数
volatile uint64_t g_last_tpoll   = 0; // 最近一次轮询周期的 Cycle 数

// ==========================================
// 辅助函数
// ==========================================

// 读取 RISC-V 内部 cycle 计数器 (64位)
static inline uint64_t read_cycles() {
    uint64_t high, low, tmp;
    __asm__ volatile(
        "1:\n"
        "csrr %0, mcycleh\n"
        "csrr %1, mcycle\n"
        "csrr %2, mcycleh\n"
        "bne %0, %2, 1b\n"
        : "=r"(high), "=r"(low), "=r"(tmp)
    );
    return (high << 32) | low;
}

// GPIO 初始化
void gpio_init() {
    uint32_t val;
    // 1. 切到 GPIO 模式
    val = mmio_read32(REG_GPIO_IOFCFG);
    val &= ~(MASK_LEDS | MASK_SWITCHES);
    mmio_write32(REG_GPIO_IOFCFG, val);

    // 2. 设置方向: LED 输出, SW 输入
    val = mmio_read32(REG_GPIO_PADDIR);
    val |= MASK_LEDS;
    val &= ~MASK_SWITCHES;
    mmio_write32(REG_GPIO_PADDIR, val);

    // 3. 初始熄灭所有灯
    val = mmio_read32(REG_GPIO_PADOUT);
    val &= ~MASK_LEDS;
    mmio_write32(REG_GPIO_PADOUT, val);
}

// 轮询式发送单个字符
void uart_putc_poll(char c) {
    while ((mmio_read32(REG_UART_LSR) & UART_LSR_THRE) == 0) {
        // Busy wait
    }
    mmio_write32(REG_UART_THR, c);
}

// 轮询式发送字符串
void uart_puts_poll(const char *s) {
    while (*s) {
        uart_putc_poll(*s++);
    }
}

// ==========================================
// 主函数
// ==========================================
int main() {
    gpio_init();

    uart_puts_poll("Lab3 Task1: Polling Mode (Edge Detection) Ready.\r\n");
    uart_puts_poll("1. Toggle Switches to verify GPIO Polling.\r\n");
    uart_puts_poll("2. Type characters to verify UART Polling.\r\n");
    uart_puts_poll("3. Press 's' to print Statistics (Tpoll/Ratio).\r\n");

    uint32_t last_sw_val = 0;
    uint64_t loop_start_time = read_cycles();

    while (1) {
        // [统计] 测量轮询周期 (Tpoll)
        uint64_t current_time = read_cycles();
        if (current_time > loop_start_time) {
            g_last_tpoll = current_time - loop_start_time;
        }
        loop_start_time = current_time;

        g_cycles_total++;
        int event_handled = 0;

        // 1. 轮询 UART
        uint32_t lsr = mmio_read32(REG_UART_LSR);
        if (lsr & UART_LSR_DR) {
            event_handled = 1;
            char c = (char)mmio_read32(REG_UART_RBR);

            // 统计命令
            if (c == 's' || c == 'S') {
                char buf[128];
                uint64_t ratio_percent = 0;
                if (g_cycles_total > 0) {
                    ratio_percent = (g_cycles_idle * 100) / g_cycles_total;
                }
                sprintf(buf, "\r\n[Stats]\r\n"
                             "  Total Loops: %lu\r\n"
                             "  Idle Loops:  %lu\r\n"
                             "  Busy Ratio:  %lu%%\r\n"
                             "  Last Tpoll:  %lu cycles\r\n", 
                        (unsigned long)g_cycles_total, 
                        (unsigned long)g_cycles_idle, 
                        (unsigned long)ratio_percent,
                        (unsigned long)g_last_tpoll);
                uart_puts_poll(buf);
            }

            // 回显 + LED翻转
            uart_putc_poll(c);
            if (c == '\r') uart_putc_poll('\n');
            uint32_t out = mmio_read32(REG_GPIO_PADOUT);
            out ^= (1u << 20);
            mmio_write32(REG_GPIO_PADOUT, out);
        }

        // 2. 轮询 GPIO (边沿检测)
        uint32_t in_val = mmio_read32(REG_GPIO_PADIN);
        uint32_t current_sw_val = (in_val & MASK_SWITCHES) >> 6;

        if (current_sw_val != last_sw_val) {
            event_handled = 1;
            uint32_t out_val = mmio_read32(REG_GPIO_PADOUT);
            uint32_t mask_leds_1_to_5 = (0x3E << 20);
            out_val &= ~mask_leds_1_to_5;
            out_val |= (current_sw_val & mask_leds_1_to_5);
            mmio_write32(REG_GPIO_PADOUT, out_val);
            last_sw_val = current_sw_val;
        }

        // 3. 忙等统计
        if (event_handled == 0) {
            g_cycles_idle++;
        }
    }
    return 0;
}

```

---

## 附录 B：测试数据记录 (Test Logs)

以下为 Task 1 实际运行时的串口终端输出截图转录。

### B.1 空闲状态测试

* **操作**: 系统启动后静置，不输入字符，不拨动开关。
* **日志**:

```text
Lab3 Task1: Polling Mode (Edge Detection) Ready.
1. Toggle Switches to verify GPIO Polling.
2. Type characters to verify UART Polling.
3. Press 's' to print Statistics (Tpoll/Ratio).

[Stats]
  Total Loops: 761585
  Idle Loops:  761583
  Busy Ratio:  99%
  Last Tpoll:  56 cycles

```

### B.2 压力与计数逻辑测试

* **操作**: 输入长字符串 `saaaaaaaaaaaaaaaaaaaaaaaaaaaaaa` (1个s + 30个a)，验证计数器增量。
* **输入前日志**:

```text
[Stats]
  Total Loops: 4422850
  Idle Loops:  4422622
  Busy Ratio:  99%
  Last Tpoll:  56 cycles

```

* **输入后日志**:

```text
saaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
[Stats]
  Total Loops: 4958981
  Idle Loops:  4958722
  Busy Ratio:  99%
  Last Tpoll:  56 cycles

```

* **数据分析**:
* 增量计算: (4,958,981 - 4,958,722) - (4,422,850 - 4,422,622) = 259 - 228 = **31**.
* 实际输入: 1 ('s') + 30 ('a') = **31**.
* 结论: 事件捕获逻辑准确。