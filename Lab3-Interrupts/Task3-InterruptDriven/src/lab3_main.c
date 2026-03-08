#include <stdint.h>
#include "lab3_mmio.h"
#include "lab3_trap.h"
#include "lab3_timer.h"
#include "lab3_stats.h"

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
    uint32_t timeout = 100000;
    while ((mmio_read32(REG_UART_LSR) & UART_LSR_THRE) == 0 && timeout--) {
        if (timeout == 0) return;
    }
    mmio_write8(REG_UART_THR, (uint8_t)c);
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

void uart_put_hex32(uint32_t val) {
    const char *hex = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 7; i >= 0; i--) {
        uart_putc(hex[(val >> (i * 4)) & 0xF]);
    }
}

void uart_put_dec(uint64_t n) {
    if (n == 0) {
        uart_putc('0');
        return;
    }
    
    char buf[32];
    int len = 0;
    while (n > 0) {
        buf[len++] = (n % 10) + '0';
        n /= 10;
    }
    
    while (len > 0) {
        uart_putc(buf[--len]);
    }
}

void uart_put_dec_aligned(uint64_t n, int width) {
    char buf[32];
    int len = 0;
    
    if (n == 0) {
        buf[len++] = '0';
    } else {
        uint64_t tmp = n;
        while (tmp > 0) {
            buf[len++] = (tmp % 10) + '0';
            tmp /= 10;
        }
    }
    
    int spaces = width - len;
    while (spaces > 0) {
        uart_putc(' ');
        spaces--;
    }
    
    for (int i = len - 1; i >= 0; i--) {
        uart_putc(buf[i]);
    }
}

// ==========================================
// 主程序
// ==========================================

int main(void) {
    // 初始化计时器（必须第一步！）
    lab3_timer_init();

    // 初始化 GPIO LED
    uint32_t val = mmio_read32(REG_GPIO_PADDIR);
    val |= MASK_LEDS;
    mmio_write32(REG_GPIO_PADDIR, val);

    // 初始化中断系统（包括统计）
    lab3_trap_init();

    uart_puts("\r\n");
    uart_puts("╔════════════════════════════════════════════════╗\r\n");
    uart_puts("║   Lab3 Task3: Interrupt vs Polling Analysis    ║\r\n");
    uart_puts("║         CPU Freq: 16 MHz (HummingBird)         ║\r\n");
    uart_puts("╚════════════════════════════════════════════════╝\r\n");
    uart_puts("\r\n[Commands]\r\n");
    uart_puts("  't' - Trigger Software Interrupt (MSI)\r\n");
    uart_puts("  's' - Show Statistics\r\n");
    uart_puts("  'r' - Reset Statistics\r\n");
    uart_puts("  'a' - Stress Test (10 MSI)\r\n");
    uart_puts("  'q' - Quick MSI Burst (100x, test latency)\r\n");
    uart_puts("\r\n");

    uint32_t last_msi_count = 0;

    while (1) {
        int event_handled = 0;

        // === 1. 处理 UART 接收 ===
        if (g_uart_rx_flag == 1) {
            event_handled = 1;
            lab3_stats_cpu_active();

            char c = g_uart_rx_char;
            g_uart_rx_flag = 0;
            
            if (c != '\r' && c != '\n') {
                uart_putc(c);
            }
            
            if (c == 't' || c == 'T') {
                uart_puts(" [MSI trigger]\r\n");
                lab3_trigger_msi();
            } 
            else if (c == 'r' || c == 'R') {
                lab3_stats_reset();
                uart_puts(" [Reset]\r\n");
            } 
            else if (c == 's' || c == 'S') {
                uart_puts("\r\n");
                lab3_stats_display();
            }
            else if (c == 'a' || c == 'A') {
                uart_puts(" [Stress test (10 MSI)...]\r\n");
                for (int i = 0; i < 10; i++) {
                    lab3_trigger_msi();
                    lab3_delay_us(100);  // 延迟 100us
                }
                uart_puts("[Stress test done]\r\n");
            }
            else if (c == 'q' || c == 'Q') {
                uart_puts(" [Quick burst test (100 MSI)...]\r\n");
                for (int i = 0; i < 100; i++) {
                    lab3_trigger_msi();
                    // 无延迟，尽可能快地触发
                }
                uart_puts("[Burst test done]\r\n");
            }
            else if (c == '\r' || c == '\n') {
                uart_puts("\r\n");
            }
        } else {
            event_handled = 0;
            lab3_stats_cpu_idle();
        }

        // === 2. 处理软件中断 (MSI) ===
        if (g_msi_count != last_msi_count) {
            event_handled = 1;
            lab3_stats_cpu_active();
            uart_puts("[MSI] Count = ");
            uart_put_dec(g_msi_count);
            uart_puts("\r\n");
            last_msi_count = g_msi_count;
        }
    }

    return 0;
}