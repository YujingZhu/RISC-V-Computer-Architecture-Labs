.section .text
.align 2
.global lab3_trap_entry

lab3_trap_entry:
    /* ==========================================
     * 1. 保存现场 (Context Save)
     * ========================================== */
    addi sp, sp, -64

    /* 【关键】保存所有 caller-saved 寄存器 */
    sw ra,  0(sp)
    sw t0,  4(sp)
    sw t1,  8(sp)
    sw t2, 12(sp)
    sw a0, 16(sp)
    sw a1, 20(sp)
    sw a2, 24(sp)
    sw a3, 28(sp)
    sw a4, 32(sp)
    sw a5, 36(sp)
    sw a6, 40(sp)
    sw a7, 44(sp)
    sw t3, 48(sp)
    sw t4, 52(sp)
    sw t5, 56(sp)
    sw t6, 60(sp)

    /* ==========================================
     * 2. 读取 CSR 并调用 C 处理函数
     * ========================================== */

    /* 【关键】在调用前读取 CSR，避免被破坏 */
    csrr a0, mcause    /* a0 = mcause */
    csrr a1, mepc      /* a1 = mepc */
    csrr a2, mtval     /* a2 = mtval */

    /* 【关键】call 指令会改变 ra，但已经保存在栈中 */
    call lab3_trap_handler

    /* ==========================================
     * 3. 恢复现场 (Context Restore)
     * ========================================== */
    lw ra,  0(sp)
    lw t0,  4(sp)
    lw t1,  8(sp)
    lw t2, 12(sp)
    lw a0, 16(sp)
    lw a1, 20(sp)
    lw a2, 24(sp)
    lw a3, 28(sp)
    lw a4, 32(sp)
    lw a5, 36(sp)
    lw a6, 40(sp)
    lw a7, 44(sp)
    lw t3, 48(sp)
    lw t4, 52(sp)
    lw t5, 56(sp)
    lw t6, 60(sp)

    addi sp, sp, 64

    /* 【关键】使用 mret 返回中断处理 */
    mret
