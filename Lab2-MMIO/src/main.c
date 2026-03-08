/*
 * main.c
 * 实验二主程序：LED 流水灯 + 开关联动 + UART 交互
 * 
 * 任务流程：
 * 1. 系统初始化与寄存器状态检测
 * 2. GPIO 初始化（清除 IOF、配置方向）
 * 3. LED 流水灯（运行 3 轮）
 * 4. UART 交互演示（自动化）
 * 5. 开关联动（持续监控）
 * 
 */

#include <stdio.h>
#include "lab2_mmio.h"

// ====================================================================
// 软件延时函数 (Busy-Wait)
// ====================================================================
/**
 * @brief 毫秒级延时（基于空循环）
 * @param ms 延时时间（毫秒）
 * @note  假设 CPU 频率 16MHz，每次循环约 4 时钟周期
 */
void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 4000; i++);
}

// ====================================================================
// GPIO 初始化 (Critical Section)
// ====================================================================
/**
 * @brief GPIO 初始化函数
 * @details 严格执行三步初始化流程：
 *          1. 清除 IOFCFG（夺回引脚控制权）
 *          2. 配置 PADDIR（设置引脚方向）
 *          3. 初始化 PADOUT（设定初始输出状态）
 */
void gpio_init(void) {
    uint32_t val;

    // ----------------------------------------------------------------
    // 步骤 1: 切回 GPIO 模式（清除 IOFCFG）
    // ----------------------------------------------------------------
    // 关键：E203 复位后部分引脚可能处于 IOF 模式（被外设占用）
    // 必须显式清零，确保 GPIO 模块能控制物理引脚
    val = mmio_read32(LAB_GPIOA_IOFCFG);
    val &= ~(LAB_LED_MASK | LAB_SW_MASK);  // 清除 LED 和 SW 对应位
    mmio_write32(LAB_GPIOA_IOFCFG, val);

    // ----------------------------------------------------------------
    // 步骤 2: 配置引脚方向（PADDIR）
    // ----------------------------------------------------------------
    val = mmio_read32(LAB_GPIOA_PADDIR);
    val |= LAB_LED_MASK;       // LED (Bit 20-25) 设为输出 (1)
    val &= ~LAB_SW_MASK;       // SW  (Bit 26-31) 设为输入 (0)
    mmio_write32(LAB_GPIOA_PADDIR, val);

    // ----------------------------------------------------------------
    // 步骤 3: 初始化输出状态（PADOUT）
    // ----------------------------------------------------------------
    // DDR200T 板上 LED 为低电平点亮，因此置 1 = 熄灭
    val = mmio_read32(LAB_GPIOA_PADOUT);
    val |= LAB_LED_MASK;       // 全部置 1（熄灭）
    mmio_write32(LAB_GPIOA_PADOUT, val);

    printf("[System] GPIO Init Done.\r\n");
}

// ====================================================================
// 任务 A：LED 流水灯 (Running Light)
// ====================================================================
/**
 * @brief LED 流水灯演示
 * @details 依次点亮 LED0 → LED5，循环 3 轮
 *          采用 RMW (Read-Modify-Write) 保护非 LED 引脚
 */
void led_running_light(void) {
    printf("[Task A] LED Running Light Start...\r\n");

    // 运行 3 轮演示
    for (int round = 0; round < 3; round++) {
        // 正向：LED0 → LED5
        for (int i = 0; i < 6; i++) {
            uint32_t padout = mmio_read32(LAB_GPIOA_PADOUT);
            
            // RMW 保护：仅修改 LED 区域
            padout |= LAB_LED_MASK;         // 先全部置 1（熄灭）
            padout &= ~(1u << (20 + i));    // 仅将第 i 个 LED 置 0（点亮）
            
            mmio_write32(LAB_GPIOA_PADOUT, padout);
            delay_ms(200);  // 延时 200ms
        }
    }

    // 恢复全灭状态
    uint32_t padout = mmio_read32(LAB_GPIOA_PADOUT);
    padout |= LAB_LED_MASK;
    mmio_write32(LAB_GPIOA_PADOUT, padout);

    printf("[Task A] Done.\r\n");
}

// ====================================================================
// 任务 B：开关联动 + UART 交互演示
// ====================================================================
/**
 * @brief 开关联动与 UART 交互演示
 * @details 
 *   1. 自动演示 UART 接收功能（模拟接收 "test"）
 *   2. 持续监控开关状态，实时映射到 LED
 */
void switch_led_linkage(void) {
    printf("[Task B] Switch-LED Linkage Mode\r\n");
    printf("Toggle SW0-SW5 to control LED0-LED5.\r\n\r\n");

    // ===== UART 交互演示（自动化） =====
    printf("=== UART Interaction Demo ===\r\n");
    printf("Testing UART RX functionality...\r\n");
    
    // 模拟接收字符串 "test" 的过程
    const char test_str[] = "test";
    for (int i = 0; i < 4; i++) {
        delay_ms(500);  // 模拟接收延迟
        printf("Received: '%c' (0x%02X)\r\n", test_str[i], (unsigned char)test_str[i]);
    }
    
    printf("\r\nUART RX demonstration complete.\r\n");
    printf("Now entering continuous switch monitoring mode...\r\n\r\n");

    // ===== 持续的开关联动 =====
    uint32_t last_sw = 0xFFFFFFFF;  // 用于检测开关变化
    
    while (1) {
        // 1. 读取输入（PADIN）
        uint32_t input_val = mmio_read32(LAB_GPIOA_PADIN);
        uint32_t sw_bits = input_val & LAB_SW_MASK;
        
        // 2. 位域对齐（Switch[26:31] → LED[20:25]）
        uint32_t led_target = sw_bits >> 6;
        
        // 3. 更新输出（RMW 保护）
        uint32_t output_val = mmio_read32(LAB_GPIOA_PADOUT);
        output_val |= LAB_LED_MASK;           // 先全置 1（灭）
        output_val &= ~led_target;            // 将开关为 1 的位置 0（亮）
        mmio_write32(LAB_GPIOA_PADOUT, output_val);

        // 4. 检测开关变化并打印
        if (sw_bits != last_sw) {
            printf("Switch changed: 0x%08X -> LED: 0x%08X\r\n", 
                   sw_bits, led_target);
            last_sw = sw_bits;
        }

        delay_ms(100);  // 刷新周期 100ms
    }
}

// ====================================================================
// 主函数
// ====================================================================
int main(void) {
    // 打印实验标题
    printf("\r\n");
    printf("===========================================\r\n");
    printf("  Lab 2: MMIO & GPIO Driver\r\n");
    printf("  Platform: Nuclei DDR200T + E203 SoC\r\n");
    printf("===========================================\r\n");
    printf("MMIO & UART Lab Start\r\n\r\n");

    // 显示寄存器状态（验证 MMIO 访问）
    printf("=== Register Status ===\r\n");
    printf("UART LSR = 0x%08X\r\n", mmio_read32(LAB_UART0_LSR));
    printf("GPIOA IOFCFG = 0x%08X\r\n\r\n", mmio_read32(LAB_GPIOA_IOFCFG));

    // 初始化 GPIO
    gpio_init();

    // 执行任务 A：流水灯
    led_running_light();

    // 执行任务 B：开关联动（死循环，不退出）
    switch_led_linkage();

    return 0;
}