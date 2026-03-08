#include "lab3_stats.h"
#include "lab3_timer.h"
#include "lab3_mmio.h"

// ==========================================
// 全局统计对象
// ==========================================
lab3_stats_t g_stats = {
    .isr_perf = {
        .total_cycles = 0,
        .call_count = 0,
        .min_cycles = UINT64_MAX,
        .max_cycles = 0,
    },
    .latency = {
        .total_cycles = 0,
        .count = 0,
        .min_cycles = UINT64_MAX,
        .max_cycles = 0,
    },
    .uart_rx = {
        .total_chars = 0,
        .isr_calls = 0,
        .fifo_drains = 0,
        .total_cycles = 0,
        .min_cycles = UINT64_MAX,
        .max_cycles = 0,
    },
    .cpu_util = {
        .cycles_total = 0,
        .cycles_active = 0,
        .cycles_idle = 0,
        .event_count = 0,
    },
    .timestamp_start = 0,
    .msi_events = 0,
    .uart_rx_events = 0,
    .total_events = 0,
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
    g_stats.timestamp_start = lab3_get_cycle();
    g_stats.isr_perf.min_cycles = UINT64_MAX;
    g_stats.latency.min_cycles = UINT64_MAX;
    g_stats.uart_rx.min_cycles = UINT64_MAX;
}

uint64_t lab3_stats_isr_entry(void) {
    return lab3_get_cycle();
}

void lab3_stats_isr_exit(uint64_t entry_cycle) {
    uint64_t exit_cycle = lab3_get_cycle();
    uint64_t duration = exit_cycle - entry_cycle;

    g_stats.isr_perf.total_cycles += duration;
    g_stats.isr_perf.call_count++;

    if (duration < g_stats.isr_perf.min_cycles) {
        g_stats.isr_perf.min_cycles = duration;
    }
    if (duration > g_stats.isr_perf.max_cycles) {
        g_stats.isr_perf.max_cycles = duration;
    }
}

void lab3_stats_record_latency(uint64_t latency_cycles) {
    g_stats.latency.total_cycles += latency_cycles;
    g_stats.latency.count++;

    if (latency_cycles < g_stats.latency.min_cycles) {
        g_stats.latency.min_cycles = latency_cycles;
    }
    if (latency_cycles > g_stats.latency.max_cycles) {
        g_stats.latency.max_cycles = latency_cycles;
    }
}

void lab3_stats_uart_rx_event(uint32_t char_count) {
    if (char_count == 0) return;

    g_stats.uart_rx.total_chars += char_count;
    g_stats.uart_rx.isr_calls++;
    g_stats.uart_rx.fifo_drains++;
    
    // 此处暂假设每次处理的周期固定，实际应在 ISR 中精确测量
    // 这里用一个简单的估计：每字符约 50 cycles
    uint64_t estimated_cycles = char_count * 50;
    g_stats.uart_rx.total_cycles += estimated_cycles;

    if (estimated_cycles < g_stats.uart_rx.min_cycles) {
        g_stats.uart_rx.min_cycles = estimated_cycles;
    }
    if (estimated_cycles > g_stats.uart_rx.max_cycles) {
        g_stats.uart_rx.max_cycles = estimated_cycles;
    }
}

void lab3_stats_cpu_active(void) {
    g_stats.cpu_util.cycles_active++;
    g_stats.cpu_util.cycles_total++;
}

void lab3_stats_cpu_idle(void) {
    g_stats.cpu_util.cycles_idle++;
    g_stats.cpu_util.cycles_total++;
}

void lab3_stats_reset(void) {
    g_stats.isr_perf.total_cycles = 0;
    g_stats.isr_perf.call_count = 0;
    g_stats.isr_perf.min_cycles = UINT64_MAX;
    g_stats.isr_perf.max_cycles = 0;

    g_stats.latency.total_cycles = 0;
    g_stats.latency.count = 0;
    g_stats.latency.min_cycles = UINT64_MAX;
    g_stats.latency.max_cycles = 0;

    g_stats.uart_rx.total_chars = 0;
    g_stats.uart_rx.isr_calls = 0;
    g_stats.uart_rx.fifo_drains = 0;
    g_stats.uart_rx.total_cycles = 0;
    g_stats.uart_rx.min_cycles = UINT64_MAX;
    g_stats.uart_rx.max_cycles = 0;

    g_stats.cpu_util.cycles_total = 0;
    g_stats.cpu_util.cycles_active = 0;
    g_stats.cpu_util.cycles_idle = 0;
    g_stats.cpu_util.event_count = 0;

    g_stats.timestamp_start = lab3_get_cycle();
    g_stats.msi_events = 0;
    g_stats.uart_rx_events = 0;
    g_stats.total_events = 0;
}

void lab3_stats_display(void) {
    uint64_t elapsed_us = lab3_get_elapsed_us(g_stats.timestamp_start);
    uint64_t elapsed_ms = elapsed_us / 1000;

    uart_puts("╔════════════════════════════════════════════════════════════╗\r\n");
    uart_puts("║            Lab3 Interrupt Statistics Report                ║\r\n");
    uart_puts("╠════════════════════════════════════════════════════════════╣\r\n");

    // === 事件统计 ===
    uart_puts("║ [Event Summary]\r\n");
    uart_puts("║   Total Events .......... ");
    uart_put_dec_aligned(g_stats.total_events, 10);
    uart_puts("\r\n");

    uart_puts("║   MSI Events ............ ");
    uart_put_dec_aligned(g_stats.msi_events, 10);
    uart_puts("\r\n");

    uart_puts("║   UART RX Events ........ ");
    uart_put_dec_aligned(g_stats.uart_rx_events, 10);
    uart_puts("\r\n");

    uart_puts("║\r\n");

    // === ISR 性能 ===
    uart_puts("║ [ISR Performance]\r\n");
    uart_puts("║   Total ISR Calls ....... ");
    uart_put_dec_aligned(g_stats.isr_perf.call_count, 10);
    uart_puts("\r\n");

    if (g_stats.isr_perf.call_count > 0) {
        uart_puts("║   Total ISR Cycles ...... ");
        uart_put_dec_aligned(g_stats.isr_perf.total_cycles, 10);
        uart_puts("\r\n");

        uint64_t avg_isr_cycles = g_stats.isr_perf.total_cycles / g_stats.isr_perf.call_count;
        uint64_t avg_isr_us = (avg_isr_cycles * 1000) / 16;  // 16 MHz

        uart_puts("║   Min ISR Cycles ........ ");
        uart_put_dec_aligned(g_stats.isr_perf.min_cycles, 10);
        uart_puts(" (");
        uart_put_dec((g_stats.isr_perf.min_cycles * 1000) / 16);
        uart_puts(" us)\r\n");

        uart_puts("║   Avg ISR Cycles ........ ");
        uart_put_dec_aligned(avg_isr_cycles, 10);
        uart_puts(" (");
        uart_put_dec(avg_isr_us);
        uart_puts(" us)\r\n");

        uart_puts("║   Max ISR Cycles ........ ");
        uart_put_dec_aligned(g_stats.isr_perf.max_cycles, 10);
        uart_puts(" (");
        uart_put_dec((g_stats.isr_perf.max_cycles * 1000) / 16);
        uart_puts(" us)\r\n");
    }

    uart_puts("║\r\n");

    // === 中断延迟 ===
    uart_puts("║ [Interrupt Latency]\r\n");
    if (g_stats.latency.count > 0) {
        uint64_t avg_latency = g_stats.latency.total_cycles / g_stats.latency.count;
        uint64_t avg_latency_us = (avg_latency * 1000) / 16;

        uart_puts("║   Latency Samples ....... ");
        uart_put_dec_aligned(g_stats.latency.count, 10);
        uart_puts("\r\n");

        uart_puts("║   Min Latency ........... ");
        uart_put_dec_aligned(g_stats.latency.min_cycles, 10);
        uart_puts(" cycles (");
        uart_put_dec((g_stats.latency.min_cycles * 1000) / 16);
        uart_puts(" us)\r\n");

        uart_puts("║   Avg Latency ........... ");
        uart_put_dec_aligned(avg_latency, 10);
        uart_puts(" cycles (");
        uart_put_dec(avg_latency_us);
        uart_puts(" us)\r\n");

        uart_puts("║   Max Latency ........... ");
        uart_put_dec_aligned(g_stats.latency.max_cycles, 10);
        uart_puts(" cycles (");
        uart_put_dec((g_stats.latency.max_cycles * 1000) / 16);
        uart_puts(" us)\r\n");
    } else {
        uart_puts("║   (No latency data recorded)\r\n");
    }

    uart_puts("║\r\n");

    // === UART RX 统计 ===
    uart_puts("║ [UART RX Statistics]\r\n");
    uart_puts("║   Total Chars Received .. ");
    uart_put_dec_aligned(g_stats.uart_rx.total_chars, 10);
    uart_puts("\r\n");

    uart_puts("║   UART ISR Calls ........ ");
    uart_put_dec_aligned(g_stats.uart_rx.isr_calls, 10);
    uart_puts("\r\n");

    uart_puts("║   FIFO Drains ........... ");
    uart_put_dec_aligned(g_stats.uart_rx.fifo_drains, 10);
    uart_puts("\r\n");

    if (g_stats.uart_rx.isr_calls > 0) {
        uint32_t avg_chars_per_isr = g_stats.uart_rx.total_chars / g_stats.uart_rx.isr_calls;
        uart_puts("║   Avg Chars/ISR ......... ");
        uart_put_dec_aligned(avg_chars_per_isr, 10);
        uart_puts("\r\n");
    }

    uart_puts("║\r\n");

    // === CPU 利用率 ===
    uart_puts("║ [CPU Utilization]\r\n");
    uart_puts("║   Total Cycles .......... ");
    uart_put_dec_aligned(g_stats.cpu_util.cycles_total, 12);
    uart_puts("\r\n");

    uart_puts("║   Active Cycles ......... ");
    uart_put_dec_aligned(g_stats.cpu_util.cycles_active, 12);
    uart_puts("\r\n");

    uart_puts("║   Idle Cycles ........... ");
    uart_put_dec_aligned(g_stats.cpu_util.cycles_idle, 12);
    uart_puts("\r\n");

    if (g_stats.cpu_util.cycles_total > 0) {
        uint64_t busy_rate = (g_stats.cpu_util.cycles_active * 100) / g_stats.cpu_util.cycles_total;
        uint64_t idle_rate = (g_stats.cpu_util.cycles_idle * 100) / g_stats.cpu_util.cycles_total;

        uart_puts("║   Busy Rate ............. ");
        uart_put_dec(busy_rate);
        uart_puts("%\r\n");

        uart_puts("║   Idle Rate ............. ");
        uart_put_dec(idle_rate);
        uart_puts("%\r\n");
    }

    uart_puts("║\r\n");

    // === 时间统计 ===
    uart_puts("║ [Timing]\r\n");
    uart_puts("║   Elapsed Time .......... ");
    uart_put_dec_aligned(elapsed_ms, 8);
    uart_puts(" ms (");
    uart_put_dec(elapsed_us);
    uart_puts(" us)\r\n");

    uart_puts("╚════════════════════════════════════════════════════════════╝\r\n");
    uart_puts("\r\n");
}