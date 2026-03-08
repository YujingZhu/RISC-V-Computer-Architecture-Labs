#ifndef APPLICATION_LAB3_MMIO_H_
#define APPLICATION_LAB3_MMIO_H_

#include <stdint.h>

// ==========================================
// 1. 外设基地址
// ==========================================
#define LAB_GPIOA_BASE      0x10012000u
#define LAB_UART0_BASE      0x10013000u
#define LAB_PLIC_BASE       0x0C000000u
#define LAB_CLINT_BASE      0x02000000u

// ==========================================
// 2. 寄存器偏移与地址
// ==========================================

// --- GPIO ---
#define REG_GPIO_PADDIR     (LAB_GPIOA_BASE + 0x00u)
#define REG_GPIO_PADIN      (LAB_GPIOA_BASE + 0x04u)
#define REG_GPIO_PADOUT     (LAB_GPIOA_BASE + 0x08u)
#define REG_GPIO_IOFCFG     (LAB_GPIOA_BASE + 0x0Cu)

// --- UART0 (16550) ---
#define REG_UART_RBR        (LAB_UART0_BASE + 0x00u)
#define REG_UART_THR        (LAB_UART0_BASE + 0x00u)
#define REG_UART_IER        (LAB_UART0_BASE + 0x04u)
#define REG_UART_IIR        (LAB_UART0_BASE + 0x08u)
#define REG_UART_FCR        (LAB_UART0_BASE + 0x08u)
#define REG_UART_LCR        (LAB_UART0_BASE + 0x0Cu)
#define REG_UART_MCR        (LAB_UART0_BASE + 0x10u)
#define REG_UART_LSR        (LAB_UART0_BASE + 0x14u)

// --- PLIC (正确的地址计算) ---
// Priority: PLIC_BASE + 0x0 + 4 * SourceID
// Source ID 3 (UART0) -> Priority = 0x0C000000 + 0x0 + 4*3 = 0x0C00000C
#define PLIC_PRIORITY_BASE  (LAB_PLIC_BASE + 0x000000u)
#define PLIC_PRIORITY_UART0 (PLIC_PRIORITY_BASE + 4 * 3)

// Enable: PLIC_BASE + 0x2000
#define PLIC_ENABLE_HART0   (LAB_PLIC_BASE + 0x002000u)

// Threshold: PLIC_BASE + 0x200000
#define PLIC_THRESHOLD_HART0 (LAB_PLIC_BASE + 0x200000u)

// Claim/Complete: PLIC_BASE + 0x200004
#define PLIC_CLAIM_HART0    (LAB_PLIC_BASE + 0x200004u)

// --- CLINT ---
#define REG_CLINT_MSIP      (LAB_CLINT_BASE + 0x0000u)

// ==========================================
// 3. 位掩码
// ==========================================
#define UART_LSR_DR         (1u << 0)
#define UART_LSR_THRE       (1u << 5)
#define UART_IER_RX         (1u << 0)

#define MASK_LEDS           (0x3Fu << 20)

// ==========================================
// 4. MMIO 访问
// ==========================================
static inline uint32_t mmio_read32(uintptr_t addr) {
    return *(volatile uint32_t *)addr;
}

static inline void mmio_write32(uintptr_t addr, uint32_t val) {
    *(volatile uint32_t *)addr = val;
}

static inline uint8_t mmio_read8(uintptr_t addr) {
    return *(volatile uint8_t *)addr;
}

static inline void mmio_write8(uintptr_t addr, uint8_t val) {
    *(volatile uint8_t *)addr = val;
}

#endif