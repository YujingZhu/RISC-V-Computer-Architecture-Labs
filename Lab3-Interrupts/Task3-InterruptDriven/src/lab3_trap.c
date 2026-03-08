#include <stdint.h>
#include "lab3_trap.h"
#include "lab3_mmio.h"
#include "lab3_timer.h"
#include "lab3_stats.h"

// ==========================================
// CSR 宏
// ==========================================
#define read_csr(reg) \
    ({ unsigned long __tmp; \
       __asm__ volatile ("csrr %0, " #reg : "=r"(__tmp)); \
       __tmp; })

#define write_csr(reg, val) \
    ({ __asm__ volatile ("csrw " #reg ", %0" :: "r"(val)); })

#define set_csr(reg, bit) \
    ({ unsigned long __tmp; \
       __asm__ volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit)); \
       __tmp; })

#define clear_csr(reg, bit) \
    ({ unsigned long __tmp; \
       __asm__ volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit)); \
       __tmp; })

// ==========================================
// 寄存器位定义
// ==========================================
#define MSTATUS_MIE         (1UL << 3)
#define MIE_MSIE            (1UL << 3)
#define MIE_MEIE            (1UL << 11)

// ==========================================
// 全局变量
// ==========================================
volatile uint32_t g_msi_count = 0;
volatile uint32_t g_uart_rx_flag = 0;
volatile char     g_uart_rx_char = 0;

// 用于记录 MSI 触发时间（用于延迟测量）
static volatile uint64_t g_msi_trigger_cycle = 0;

// ==========================================
// Trap 初始化
// ==========================================
void lab3_trap_init(void) {
    // 1. 初始化统计系统
    lab3_stats_init();

    // 2. 设置 mtvec (trap vector)
    write_csr(mtvec, ((uintptr_t)&lab3_trap_entry & ~0x3UL));

    // 3. 配置 PLIC
    mmio_write32(PLIC_PRIORITY_UART0, 1);
    
    // 使能 UART0 中断源 (ID=3)
    uint32_t en = mmio_read32(PLIC_ENABLE_HART0);
    en |= (1u << 3);
    mmio_write32(PLIC_ENABLE_HART0, en);
    
    // 阈值设为 0
    mmio_write32(PLIC_THRESHOLD_HART0, 0);

    // 4. 配置 UART
    mmio_write32(REG_UART_FCR, 0x00u);  // 禁用 FIFO
    mmio_write32(REG_UART_IER, UART_IER_RX);  // 使能 RX 中断

    // 5. 使能 CPU 中断
    set_csr(mie, MIE_MSIE | MIE_MEIE);
    set_csr(mstatus, MSTATUS_MIE);
}

// ==========================================
// 触发软件中断（记录触发时间）
// ==========================================
void lab3_trigger_msi(void) {
    // 【关键】记录触发时刻，用于计算延迟
    g_msi_trigger_cycle = lab3_get_cycle();
    mmio_write32(REG_CLINT_MSIP, 1);
}

// ==========================================
// Trap Handler (C 级)
// ==========================================
void lab3_trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval) {
    // 【关键】ISR 入口时刻
    uint64_t isr_entry_cycle = lab3_stats_isr_entry();

    uint32_t is_intr = (mcause >> 31) & 1;
    uint32_t code    = mcause & 0x7FFFFFFFUL;

    if (is_intr) {
        // --- 软件中断 (MSI) ---
        if (code == 3) {
            g_stats.msi_events++;
            g_stats.total_events++;

            // 【关键】计算延迟
            if (g_msi_trigger_cycle > 0) {
                uint64_t latency = isr_entry_cycle - g_msi_trigger_cycle;
                lab3_stats_record_latency(latency);
            }

            g_msi_count++;
            mmio_write32(REG_CLINT_MSIP, 0);  // 清除 MSIP
        }
        // --- 外部中断 (PLIC) ---
        else if (code == 11) {
            // Claim
            uint32_t id = mmio_read32(PLIC_CLAIM_HART0);
            
            // 处理 UART0 (ID=3)
            if (id == 3) {
                // 【关键】计算 UART RX 处理时间
                uint64_t uart_isr_start = lab3_get_cycle();
                uint32_t chars_read = 0;

                // 读取所有可用数据
                while ((mmio_read32(REG_UART_LSR) & UART_LSR_DR) != 0) {
                    g_uart_rx_char = (char)mmio_read8(REG_UART_RBR);
                    g_uart_rx_flag = 1;
                    chars_read++;
                    
                    // 安全起见，限制读取次数
                    if (chars_read >= 16) break;
                }

                uint64_t uart_isr_end = lab3_get_cycle();
                if (chars_read > 0) {
                    lab3_stats_uart_rx_event(chars_read);
                }

                g_stats.uart_rx_events++;      // 【修正】这里现在有定义
                g_stats.total_events++;
            }
            
            // Complete
            if (id != 0) {
                mmio_write32(PLIC_CLAIM_HART0, id);
            }
        }
    }

    // 【关键】ISR 退出时刻
    lab3_stats_isr_exit(isr_entry_cycle);
}