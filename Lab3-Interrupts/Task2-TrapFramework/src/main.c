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
    uart_puts_raw("  Lab3 Task2: Trap Framework\r\n");
    uart_puts_raw("===================================\r\n");
    uart_puts_raw("Commands: 't'=trigger, 's'=stats, 'r'=reset\r\n");
    uart_puts_raw("===================================\r\n\r\n");

    lab3_trap_init();
    
    uart_puts_raw("Ready. Press 't' to trigger MSI.\r\n\r\n");

    uint32_t last_count = 0;
    int msg_printed = 0;  // 【关键】防止重复打印

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
                    msg_printed = 0;  // 重置标志
                    break;

                case 's':
                case 'S':
                    uart_puts_raw("\r\n=== Statistics ===\r\n");
                    uart_puts_raw("MSI Count:    ");
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
            msg_printed = 1;  // 【关键】设置标志，防止重复打印
        }

        // 3. 统计空闲周期
        if (!is_busy) {
            g_cycles_idle++;
        }
    }

    return 0;
}