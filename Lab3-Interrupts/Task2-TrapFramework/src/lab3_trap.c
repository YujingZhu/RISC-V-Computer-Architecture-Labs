#include "lab3_mmio.h"

// CSR 读写宏
#define read_csr(reg) ({ \
    unsigned long __tmp; \
    __asm__ volatile ("csrr %0, " #reg : "=r"(__tmp)); \
    __tmp; \
})

#define write_csr(reg, val) ({ \
    __asm__ volatile ("csrw " #reg ", %0" :: "rK"(val)); \
})

#define set_csr(reg, bit) ({ \
    unsigned long __tmp; \
    __asm__ volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "rK"(bit)); \
    __tmp; \
})

// CSR 位定义
#define MSTATUS_MIE     (1UL << 3)
#define MIE_MSIE        (1UL << 3)
#define MCAUSE_INTR     (1UL << 31)

// 在 lab3_trap.c 的顶部添加:
#define REG_CLINT_MSIP      0x02000000u


// 全局变量
volatile uint32_t g_msi_count = 0;
volatile uint64_t g_isr_latency = 0;

// 外部声明
extern void uart_puts_raw(const char *s);
extern void uart_put_dec(uint32_t num);

void lab3_trap_init(void) {
    extern void lab3_trap_entry(void);
    
    uintptr_t trap_addr = (uintptr_t)lab3_trap_entry;
    write_csr(mtvec, trap_addr & ~0x3);
    
    set_csr(mie, MIE_MSIE);
    set_csr(mstatus, MSTATUS_MIE);
    
    uart_puts_raw("[TRAP] Init Done.\r\n");
}

void lab3_trigger_msi(void) {
    uart_puts_raw("[MSI] Triggering...\r\n");
    mmio_write32(REG_CLINT_MSIP, 1);
    
    // 短暂延迟
    for (volatile int i = 0; i < 1000; i++);
}

/* =========================================
 * 中断处理函数 - 修复版
 * ========================================= */
void lab3_trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval) {
    uint64_t start_cycle = read_csr(mcycle);
    
    uart_puts_raw("\r\n[ISR] Entered!\r\n");
    uart_puts_raw("[ISR] mcause=0x");
    uart_put_dec(mcause);
    uart_puts_raw("\r\n");

    // 【关键检查】检查是否为中断且代码为3
    int is_intr = (mcause & MCAUSE_INTR) ? 1 : 0;
    int exc_code = mcause & 0xFF;
    
    uart_puts_raw("[ISR] is_interrupt=");
    uart_put_dec(is_intr);
    uart_puts_raw(", code=");
    uart_put_dec(exc_code);
    uart_puts_raw("\r\n");

    if (is_intr && (exc_code == 3)) {
        // 软件中断
        g_msi_count++;
        
        uart_puts_raw("[ISR] MSI count -> ");
        uart_put_dec(g_msi_count);
        uart_puts_raw("\r\n");
        
        // 【关键】清除 MSIP 挂起位
        uart_puts_raw("[ISR] Clearing MSIP...\r\n");
        mmio_write32(REG_CLINT_MSIP, 0);
        
        // 读回验证
        uint32_t msip_verify = mmio_read32(REG_CLINT_MSIP);
        uart_puts_raw("[ISR] MSIP verify=");
        uart_put_dec(msip_verify);
        uart_puts_raw("\r\n");
    }
    else {
        uart_puts_raw("[TRAP] ERROR: Not MSI! mcause=0x");
        uart_put_dec(mcause);
        uart_puts_raw("\r\n");
        while(1);
    }

    uint64_t end_cycle = read_csr(mcycle);
    g_isr_latency = end_cycle - start_cycle;
    
    uart_puts_raw("[ISR] Done.\r\n");
}