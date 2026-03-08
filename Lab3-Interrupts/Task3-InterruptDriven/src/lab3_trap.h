#ifndef LAB3_TRAP_H
#define LAB3_TRAP_H

#include <stdint.h>

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
extern volatile char     g_uart_rx_char;

#endif