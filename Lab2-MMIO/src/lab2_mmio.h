#ifndef APPLICATION_LAB2_MMIO_H_
#define APPLICATION_LAB2_MMIO_H_

#include <stdint.h>

// ==================================================
// Hardware Base Addresses
// ==================================================
#define GPIOA_BASE            0x10012000u
#define UART0_BASE            0x10013000u

// ==================================================
// GPIO Register Offsets
// ==================================================
#define GPIO_PADDIR_OFS       0x00u   // Direction register
#define GPIO_PADIN_OFS        0x04u   // Input register
#define GPIO_PADOUT_OFS       0x08u   // Output register
#define GPIO_IOFCFG_OFS       0x1Cu   // IO function config register

// ==================================================
// GPIOA Full Addresses
// ==================================================
#define LAB_GPIOA_PADDIR      (GPIOA_BASE + GPIO_PADDIR_OFS)
#define LAB_GPIOA_PADIN       (GPIOA_BASE + GPIO_PADIN_OFS)
#define LAB_GPIOA_PADOUT      (GPIOA_BASE + GPIO_PADOUT_OFS)
#define LAB_GPIOA_IOFCFG      (GPIOA_BASE + GPIO_IOFCFG_OFS)

// ==================================================
// UART0 Registers
// ==================================================
#define LAB_UART0_THR         (UART0_BASE + 0x00u)   // TX holding register
#define LAB_UART0_RBR         (UART0_BASE + 0x00u)   // RX buffer register
#define LAB_UART0_LSR         (UART0_BASE + 0x14u)   // Line status register

#define LAB_UART_LSR_DR       (1u << 0)   // RX data ready
#define LAB_UART_LSR_THRE     (1u << 5)   // TX holding register empty

// ==================================================
// LED & Switch GPIO Bitmasks
// ==================================================
// LED0-LED5: GPIOA bit[20:25]
#define LAB_LED_MASK          0x03F00000u
#define LED0_BIT              (1u << 20)
#define LED1_BIT              (1u << 21)
#define LED2_BIT              (1u << 22)
#define LED3_BIT              (1u << 23)
#define LED4_BIT              (1u << 24)
#define LED5_BIT              (1u << 25)

// SW0-SW5: GPIOA bit[26:31]
#define LAB_SW_MASK           0xFC000000u

// ==================================================
// MMIO Access Functions
// ==================================================
static inline uint32_t mmio_read32(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}

static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}

#endif /* APPLICATION_LAB2_MMIO_H_ */
