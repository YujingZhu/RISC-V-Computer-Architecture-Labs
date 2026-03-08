#ifndef LAB3_STATS_H
#define LAB3_STATS_H

#include <stdint.h>

// ==========================================
// ISR 性能统计
// ==========================================
typedef struct {
    uint64_t total_cycles;      // ISR 总执行周期
    uint32_t call_count;        // ISR 调用次数
    uint64_t min_cycles;        // 最小执行周期
    uint64_t max_cycles;        // 最大执行周期
} isr_perf_t;

// ==========================================
// 中断延迟统计
// ==========================================
typedef struct {
    uint64_t total_cycles;      // 总延迟周期
    uint32_t count;             // 采样次数
    uint64_t min_cycles;        // 最小延迟
    uint64_t max_cycles;        // 最大延迟
} interrupt_latency_t;

// ==========================================
// UART RX 统计
// ==========================================
typedef struct {
    uint32_t total_chars;       // 接收字符总数
    uint32_t isr_calls;         // UART ISR 调用次数
    uint32_t fifo_drains;       // FIFO 清空次数（处理过数据）
    uint64_t total_cycles;      // 总处理周期
    uint64_t min_cycles;        // 单次最小周期
    uint64_t max_cycles;        // 单次最大周期
} uart_rx_stats_t;

// ==========================================
// CPU 利用率统计（轮询 vs 中断）
// ==========================================
typedef struct {
    uint64_t cycles_total;      // 总周期
    uint64_t cycles_active;     // 活跃周期（处理事件）
    uint64_t cycles_idle;       // 空闲周期
    uint32_t event_count;       // 事件总数
} cpu_util_t;

// ==========================================
// 综合统计结构体（修正版）
// ==========================================
typedef struct {
    isr_perf_t isr_perf;                // ISR 性能
    interrupt_latency_t latency;        // 中断延迟
    uart_rx_stats_t uart_rx;            // UART RX
    cpu_util_t cpu_util;                // CPU 利用率
    
    uint64_t timestamp_start;           // 统计开始时间戳
    uint32_t msi_events;                // MSI 事件计数（添加）
    uint32_t uart_rx_events;            // UART RX 事件计数（添加）
    uint32_t total_events;              // 总事件计数
} lab3_stats_t;

// ==========================================
// 全局统计对象
// ==========================================
extern lab3_stats_t g_stats;

// ==========================================
// 统计接口
// ==========================================

/**
 * @brief 初始化统计系统
 */
void lab3_stats_init(void);

/**
 * @brief 记录 ISR 进入时间戳
 * @return 进入时的周期数
 */
uint64_t lab3_stats_isr_entry(void);

/**
 * @brief 记录 ISR 退出，计算执行时间
 * @param entry_cycle ISR 进入时的周期数
 */
void lab3_stats_isr_exit(uint64_t entry_cycle);

/**
 * @brief 记录中断延迟
 * @param latency_cycles 延迟的周期数
 */
void lab3_stats_record_latency(uint64_t latency_cycles);

/**
 * @brief 记录 UART RX ISR 处理
 * @param char_count 本次读取的字符数
 */
void lab3_stats_uart_rx_event(uint32_t char_count);

/**
 * @brief 记录 CPU 活跃周期
 */
void lab3_stats_cpu_active(void);

/**
 * @brief 记录 CPU 空闲周期
 */
void lab3_stats_cpu_idle(void);

/**
 * @brief 显示所有统计数据
 */
void lab3_stats_display(void);

/**
 * @brief 重置所有统计数据
 */
void lab3_stats_reset(void);

#endif // LAB3_STATS_H