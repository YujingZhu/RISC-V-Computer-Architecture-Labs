.global lab3_trap_entry
.align 4

lab3_trap_entry:
    # 保存通用寄存器（这里简化为关键寄存器）
    addi sp, sp, -128

    # 保存 caller-save
    sd   ra,   0(sp)
    sd   t0,   8(sp)
    sd   t1,  16(sp)
    sd   t2,  24(sp)
    sd   a0,  32(sp)
    sd   a1,  40(sp)
    sd   a2,  48(sp)
    sd   a3,  56(sp)
    sd   a4,  64(sp)
    sd   a5,  72(sp)
    sd   a6,  80(sp)
    sd   a7,  88(sp)

    # 读取 mcause
    csrr t0, mcause

    # 读取 mepc
    csrr t1, mepc

    # 读取 mtval
    csrr t2, mtval

    # 调用 C 函数：lab3_trap_handler(mcause, mepc, mtval)
    mv   a0, t0
    mv   a1, t1
    mv   a2, t2
    call lab3_trap_handler

    # 恢复寄存器
    ld   ra,   0(sp)
    ld   t0,   8(sp)
    ld   t1,  16(sp)
    ld   t2,  24(sp)
    ld   a0,  32(sp)
    ld   a1,  40(sp)
    ld   a2,  48(sp)
    ld   a3,  56(sp)
    ld   a4,  64(sp)
    ld   a5,  72(sp)
    ld   a6,  80(sp)
    ld   a7,  88(sp)

    addi sp, sp, 128

    # 返回
    mret
