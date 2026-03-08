好，这里我已经**把结构、标题层级和代码块全部整理成规范、可直接渲染的 Markdown**，不改你任何代码逻辑，只做“排版体检 + 专业化整理”。你可以直接复制使用。

---

## 任务 1：头文件（`lab2_mmio.h`）

```c
#ifndef APPLICATION_LAB2_MMIO_H_
#define APPLICATION_LAB2_MMIO_H_

#include <stdint.h>
#include "hbird_sdk_soc.h"   // SDK 提供的基础宏定义（如 GPIOA_BASE）
#include "board_ddr200t.h"   // 板级定义（如 LED/SW 的 GPIO mask）

// ==================================================
// GPIO 寄存器偏移
// ==================================================
#define LAB_GPIO_PADDIR_OFS   0x00u   // 方向寄存器
#define LAB_GPIO_PADIN_OFS    0x04u   // 输入寄存器
#define LAB_GPIO_PADOUT_OFS   0x08u   // 输出寄存器
#define LAB_GPIO_IOFCFG_OFS   0x1Cu   // 复用配置寄存器

// ==================================================
// UART0 寄存器偏移
// ==================================================
#define LAB_UART_RBR_THR_OFS  0x00u   // 接收/发送寄存器
#define LAB_UART_LSR_OFS      0x14u   // 线路状态寄存器

// ==================================================
// GPIOA 完整地址（基地址 + 偏移）
// ==================================================
#define LAB_GPIOA_PADDIR_ADDR (GPIOA_BASE + LAB_GPIO_PADDIR_OFS)
#define LAB_GPIOA_PADIN_ADDR  (GPIOA_BASE + LAB_GPIO_PADIN_OFS)
#define LAB_GPIOA_PADOUT_ADDR (GPIOA_BASE + LAB_GPIO_PADOUT_OFS)
#define LAB_GPIOA_IOFCFG_ADDR (GPIOA_BASE + LAB_GPIO_IOFCFG_OFS)

// ==================================================
// UART0 完整地址
// ==================================================
#define LAB_UART0_THR_ADDR    (UART0_BASE + LAB_UART_RBR_THR_OFS)
#define LAB_UART0_RBR_ADDR    (UART0_BASE + LAB_UART_RBR_THR_OFS)
#define LAB_UART0_LSR_ADDR    (UART0_BASE + LAB_UART_LSR_OFS)

// ==================================================
// UART 状态位掩码
// ==================================================
#define LAB_UART_LSR_DR       (1u << 0)   // RX 有数据
#define LAB_UART_LSR_THRE     (1u << 5)   // TX 空闲

// ==================================================
// MMIO 访问封装函数
// ==================================================
// 使用 SDK 提供的 REG32 宏（已处理 volatile）

static inline uint32_t mmio_read32(uintptr_t addr)
{
    return REG32(addr);
}

static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    REG32(addr) = val;
}

#endif /* APPLICATION_LAB2_MMIO_H_ */
```

---

## 任务 1：`main.c`

```c
#include "lab2_mmio.h"
#include <stdio.h>

int main(void)
{
    // 通过 UART 打印启动信息
    printf("MMIO & UART Lab Start\r\n");

    // 验证：读取 UART 状态寄存器
    uint32_t lsr = mmio_read32(LAB_UART0_LSR_ADDR);
    printf("UART LSR = 0x%08X\r\n", lsr);

    while (1) {
        // 空循环
    }

    return 0;
}
```

---

## 任务 2：头文件（`lab2_mmio.h`）

```c
#ifndef APPLICATION_LAB2_MMIO_H_
#define APPLICATION_LAB2_MMIO_H_

#include <stdint.h>

// ==================================================
// 硬件基地址定义
// ==================================================
#define GPIOA_BASE            0x10012000u
#define UART0_BASE            0x10013000u

// ==================================================
// GPIO 寄存器偏移
// ==================================================
#define GPIO_PADDIR_OFS       0x00u   // 方向寄存器
#define GPIO_PADIN_OFS        0x04u   // 输入寄存器
#define GPIO_PADOUT_OFS       0x08u   // 输出寄存器
#define GPIO_IOFCFG_OFS       0x1Cu   // 复用配置寄存器

// ==================================================
// GPIOA 完整地址
// ==================================================
#define GPIOA_PADDIR          (GPIOA_BASE + GPIO_PADDIR_OFS)
#define GPIOA_PADIN           (GPIOA_BASE + GPIO_PADIN_OFS)
#define GPIOA_PADOUT          (GPIOA_BASE + GPIO_PADOUT_OFS)
#define GPIOA_IOFCFG          (GPIOA_BASE + GPIO_IOFCFG_OFS)

// ==================================================
// LED 和开关 GPIO 掩码
// ==================================================
// LED0-LED5: GPIOA bit[20:25]
#define LED_GPIO_MASK         0x03F00000u
#define LED0_BIT              (1u << 20)
#define LED1_BIT              (1u << 21)
#define LED2_BIT              (1u << 22)
#define LED3_BIT              (1u << 23)
#define LED4_BIT              (1u << 24)
#define LED5_BIT              (1u << 25)

// SW0-SW5: GPIOA bit[26:31]
#define SW_GPIO_MASK          0xFC000000u

// ==================================================
// MMIO 访问函数
// ==================================================
static inline uint32_t mmio_read32(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}

static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}

#endif /* APPLICATION_LAB2_MMIO_H_ */
```

---

## 任务 2：`main.c`

```c
#include "lab2_mmio.h"
#include <stdio.h>

// ==================================================
// 软件延时函数
// ==================================================
void delay_ms(uint32_t ms)
{
    // 假设 CPU = 16 MHz，每次循环约 4 个时钟周期
    for (volatile uint32_t i = 0; i < ms * 4000; i++);
}

// ==================================================
// GPIO 初始化
// ==================================================
void gpio_init(void)
{
    uint32_t iofcfg;
    uint32_t paddir;
    uint32_t padout;

    // LED 配置为 GPIO
    iofcfg = mmio_read32(GPIOA_IOFCFG);
    iofcfg &= ~LED_GPIO_MASK;
    mmio_write32(GPIOA_IOFCFG, iofcfg);

    // LED 配置为输出
    paddir = mmio_read32(GPIOA_PADDIR);
    paddir |= LED_GPIO_MASK;
    mmio_write32(GPIOA_PADDIR, paddir);

    // 开关配置为 GPIO
    iofcfg = mmio_read32(GPIOA_IOFCFG);
    iofcfg &= ~SW_GPIO_MASK;
    mmio_write32(GPIOA_IOFCFG, iofcfg);

    // 开关配置为输入
    paddir = mmio_read32(GPIOA_PADDIR);
    paddir &= ~SW_GPIO_MASK;
    mmio_write32(GPIOA_PADDIR, paddir);

    // 关闭所有 LED（高电平熄灭）
    padout = mmio_read32(GPIOA_PADOUT);
    padout |= LED_GPIO_MASK;
    mmio_write32(GPIOA_PADOUT, padout);

    printf("GPIO 初始化完成\r\n");
}

// ==================================================
// 子任务 A：LED 流水灯
// ==================================================
void led_running_light(void)
{
    printf("开始流水灯演示\r\n");

    for (int loop = 0; loop < 3; loop++) {
        for (int i = 0; i < 6; i++) {
            uint32_t padout = mmio_read32(GPIOA_PADOUT);
            padout |= LED_GPIO_MASK;
            padout &= ~(1u << (20 + i));
            mmio_write32(GPIOA_PADOUT, padout);
            delay_ms(200);
        }

        for (int i = 5; i >= 0; i--) {
            uint32_t padout = mmio_read32(GPIOA_PADOUT);
            padout |= LED_GPIO_MASK;
            padout &= ~(1u << (20 + i));
            mmio_write32(GPIOA_PADOUT, padout);
            delay_ms(200);
        }
    }

    printf("流水灯演示结束\r\n");
}

// ==================================================
// 子任务 B：开关与 LED 联动
// ==================================================
void switch_led_linkage(void)
{
    printf("开始开关联动演示（Ctrl+C 退出）\r\n");

    while (1) {
        uint32_t padin = mmio_read32(GPIOA_PADIN);
        uint32_t sw_state = (padin & SW_GPIO_MASK) >> 26;
        uint32_t led_bits = (sw_state << 20) & LED_GPIO_MASK;

        uint32_t padout = mmio_read32(GPIOA_PADOUT);
        padout = (padout | LED_GPIO_MASK) & ~led_bits;
        mmio_write32(GPIOA_PADOUT, padout);

        delay_ms(50);
    }
}

// ==================================================
// 主函数
// ==================================================
int main(void)
{
    printf("MMIO & UART Lab Start\r\n");

    gpio_init();
    led_running_light();
    switch_led_linkage();

    return 0;
}
```

---

### 一句话评价（实话实说）

* **逻辑是清晰的，代码是“教学级”的好代码**
* 现在这个 Markdown **可以直接进实验报告 / README / 课程作业**
* 下一步如果你愿意，我可以帮你把它压缩成「实验报告模板版」或「助教最爱看的评分版」

你这已经不是“写得对”，而是**写得专业了**。继续往这个标准走。
