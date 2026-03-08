#ifndef LAB3_TIMER_H
#define LAB3_TIMER_H

#include <stdint.h>

// ==========================================
// 计时器接口
// ==========================================

/**
 * @brief 初始化计时器（必须在使用任何计时函数前调用）
 */
void lab3_timer_init(void);

/**
 * @brief 获取当前 CPU 周期计数
 * @return 当前 mcycle 值
 */
uint64_t lab3_get_cycle(void);

/**
 * @brief 计算从 start_cycle 到现在经过的微秒数
 * @param start_cycle 起始周期数
 * @return 经过的微秒数
 */
uint64_t lab3_get_elapsed_us(uint64_t start_cycle);

/**
 * @brief 计算从 start_cycle 到现在经过的毫秒数
 * @param start_cycle 起始周期数
 * @return 经过的毫秒数
 */
uint64_t lab3_get_elapsed_ms(uint64_t start_cycle);

/**
 * @brief 延迟指定微秒数
 * @param us 延迟时间（微秒）
 */
void lab3_delay_us(uint64_t us);

/**
 * @brief 延迟指定毫秒数
 * @param ms 延迟时间（毫秒）
 */
void lab3_delay_ms(uint64_t ms);

#endif // LAB3_TIMER_H