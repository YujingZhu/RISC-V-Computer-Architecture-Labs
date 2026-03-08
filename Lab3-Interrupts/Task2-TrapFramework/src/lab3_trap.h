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

#endif