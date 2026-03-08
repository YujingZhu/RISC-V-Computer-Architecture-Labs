#include "lab3_timer.h"

// ==========================================
// CPU 频率（Hz）
// ==========================================
#define CPU_FREQ_HZ  16000000UL  // 16 MHz for HBirdv2

// ==========================================
// CSR 访问宏
// ==========================================
#define read_csr(reg) \
    ({ unsigned long __tmp; \
       __asm__ volatile ("csrr %0, " #reg : "=r"(__tmp)); \
       __tmp; })

// ==========================================
// 实现
// ==========================================

void lab3_timer_init(void) {
    // 当前实现中无需特殊初始化
    // mcycle 寄存器由硬件自动计数
}

uint64_t lab3_get_cycle(void) {
    // 读取 mcycle 寄存器
    return read_csr(mcycle);
}

uint64_t lab3_get_elapsed_us(uint64_t start_cycle) {
    uint64_t end_cycle = lab3_get_cycle();
    uint64_t diff_cycles = end_cycle - start_cycle;
    
    // 转换为微秒：cycles / (freq_hz / 1e6) = cycles * 1e6 / freq_hz
    return (diff_cycles * 1000000UL) / CPU_FREQ_HZ;
}

uint64_t lab3_get_elapsed_ms(uint64_t start_cycle) {
    uint64_t elapsed_us = lab3_get_elapsed_us(start_cycle);
    return elapsed_us / 1000UL;
}

void lab3_delay_us(uint64_t us) {
    uint64_t start = lab3_get_cycle();
    uint64_t target = start + (us * CPU_FREQ_HZ) / 1000000UL;
    
    while (lab3_get_cycle() < target) {
        // 忙轮询延迟
    }
}

void lab3_delay_ms(uint64_t ms) {
    lab3_delay_us(ms * 1000UL);
}