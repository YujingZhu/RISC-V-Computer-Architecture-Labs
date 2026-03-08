#include <stdint.h>
// 我们不再依赖 <stdio.h> 的 sprintf，彻底杜绝库函数 bug
#include "lab3_mmio.h"

// ==========================================
// 全局统计变量
// ==========================================
volatile uint64_t g_cycles_total_time = 0; 
volatile uint64_t g_cycles_idle_time  = 0; 
volatile uint64_t g_last_tpoll        = 0; 

// ==========================================
// 辅助函数
// ==========================================

// 读取 RISC-V 64位 Cycle 计数器
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
    val = mmio_read32(REG_GPIO_IOFCFG);
    val &= ~(MASK_LEDS | MASK_SWITCHES);
    mmio_write32(REG_GPIO_IOFCFG, val);

    val = mmio_read32(REG_GPIO_PADDIR);
    val |= MASK_LEDS;
    val &= ~MASK_SWITCHES;
    mmio_write32(REG_GPIO_PADDIR, val);

    val = mmio_read32(REG_GPIO_PADOUT);
    val &= ~MASK_LEDS;
    mmio_write32(REG_GPIO_PADOUT, val);
}

// 轮询式发送单个字符
void uart_putc_poll(char c) {
    while ((mmio_read32(REG_UART_LSR) & UART_LSR_THRE) == 0) {
        // 等待
    }
    mmio_write32(REG_UART_THR, c);
}

// 轮询式发送字符串
void uart_puts_poll(const char *s) {
    while (*s) {
        uart_putc_poll(*s++);
    }
}

// [关键新增] 手动打印十进制整数，绕过 buggy 的 sprintf
void uart_print_dec(uint32_t val) {
    char buffer[12]; // 32位整数最大只有10位
    int i = 0;
    
    if (val == 0) {
        uart_putc_poll('0');
        return;
    }
    
    // 提取每一位
    while (val > 0) {
        buffer[i++] = (val % 10) + '0';
        val /= 10;
    }
    
    // 倒序输出
    while (i > 0) {
        uart_putc_poll(buffer[--i]);
    }
}

// ==========================================
// 主函数
// ==========================================
int main() {
    gpio_init();

    uart_puts_poll("\r\n=== Lab3 Task1: Polling (No Lib) ===\r\n");
    uart_puts_poll("1. Toggle Switches -> Verify LED.\r\n");
    uart_puts_poll("2. Type chars -> Verify Echo.\r\n");
    uart_puts_poll("3. Press 's' -> View Stats.\r\n");

    uint32_t last_sw_val = 0;
    uint64_t start_time = read_cycles();

    while (1) {
        // 1. 时间测量
        uint64_t current_time = read_cycles();
        uint64_t delta = current_time - start_time; 
        start_time = current_time;

        g_last_tpoll = delta;
        g_cycles_total_time += delta;

        // 2. 轮询逻辑
        int event_handled = 0;

        // UART
        uint32_t lsr = mmio_read32(REG_UART_LSR);
        if (lsr & UART_LSR_DR) {
            event_handled = 1; 
            char c = (char)mmio_read32(REG_UART_RBR);

            if (c == 's' || c == 'S') {
                // === 手动分行打印统计数据 ===
                
                // 1. 计算 Ratio (64位运算，转32位结果)
                uint32_t ratio = 0;
                if (g_cycles_total_time > 0) {
                    ratio = (uint32_t)((g_cycles_idle_time * 100) / g_cycles_total_time);
                }
                
                // 2. 抓取快照并转换为32位 (丢弃高32位，足够显示几十秒的数据)
                uint32_t total_32 = (uint32_t)g_cycles_total_time;
                uint32_t idle_32  = (uint32_t)g_cycles_idle_time;
                uint32_t tpoll_32 = (uint32_t)g_last_tpoll;

                uart_puts_poll("\r\n[Stats]\r\n");
                
                uart_puts_poll("  Total: ");
                uart_print_dec(total_32);
                uart_puts_poll("\r\n");

                uart_puts_poll("  Idle:  ");
                uart_print_dec(idle_32);
                uart_puts_poll("\r\n");

                uart_puts_poll("  Ratio: ");
                uart_print_dec(ratio);
                uart_puts_poll("%\r\n"); // 加上百分号

                uart_puts_poll("  Tpoll: ");
                uart_print_dec(tpoll_32);
                uart_puts_poll("\r\n");

                // 清零
                g_cycles_total_time = 0;
                g_cycles_idle_time = 0;
            } else {
                uart_putc_poll(c);
                if (c == '\r') uart_putc_poll('\n');
                
                uint32_t out = mmio_read32(REG_GPIO_PADOUT);
                out ^= (1u << 20); 
                mmio_write32(REG_GPIO_PADOUT, out);
            }
        }

        // GPIO
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

        if (event_handled == 0) {
            g_cycles_idle_time += delta;
        }
    }
    return 0;
}