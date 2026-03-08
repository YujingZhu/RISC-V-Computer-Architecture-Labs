/*
 * lab3_mmio.h
 *
 *  Created on: 2026年1月9日
 */

#ifndef APPLICATION_LAB3_MMIO_H_
#define APPLICATION_LAB3_MMIO_H_


#include <stdint.h>

// ==========================================
// 1. 外设基地址定义
// ==========================================
#define LAB_GPIOA_BASE      0x10012000u
#define LAB_UART0_BASE      0x10013000u

// ==========================================
// 2. 寄存器偏移
// ==========================================
// GPIO
#define LAB_GPIO_PADDIR_OFS 0x00u
#define LAB_GPIO_PADIN_OFS  0x04u
#define LAB_GPIO_PADOUT_OFS 0x08u
#define LAB_GPIO_IOFCFG_OFS 0x1Cu

// UART (16550)
#define LAB_UART_RBR_THR_OFS 0x00u
#define LAB_UART_LSR_OFS     0x14u

// ==========================================
// 3. 完整寄存器地址
// ==========================================
#define REG_GPIO_PADDIR (LAB_GPIOA_BASE + LAB_GPIO_PADDIR_OFS)
#define REG_GPIO_PADIN  (LAB_GPIOA_BASE + LAB_GPIO_PADIN_OFS)
#define REG_GPIO_PADOUT (LAB_GPIOA_BASE + LAB_GPIO_PADOUT_OFS)
#define REG_GPIO_IOFCFG (LAB_GPIOA_BASE + LAB_GPIO_IOFCFG_OFS)

#define REG_UART_RBR    (LAB_UART0_BASE + LAB_UART_RBR_THR_OFS)
#define REG_UART_THR    (LAB_UART0_BASE + LAB_UART_RBR_THR_OFS)
#define REG_UART_LSR    (LAB_UART0_BASE + LAB_UART_LSR_OFS)

// ==========================================
// 4. 位掩码定义
// ==========================================
// UART LSR
#define UART_LSR_DR     (1u << 0)  // 接收数据就绪
#define UART_LSR_THRE   (1u << 5)  // 发送缓冲区空

// GPIO
#define MASK_LEDS       (0x3Fu << 20) // LED0-LED5
#define MASK_SWITCHES   (0x3Fu << 26) // SW0-SW5

// ==========================================
// 5. MMIO 访问封装
// ==========================================
static inline uint32_t mmio_read32(uintptr_t addr) {
    return *(volatile uint32_t *)addr;
}

static inline void mmio_write32(uintptr_t addr, uint32_t val) {
    *(volatile uint32_t *)addr = val;
}

#endif /* APPLICATION_LAB3_MMIO_H_ */