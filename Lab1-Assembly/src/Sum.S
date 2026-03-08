.section .text
    .global sum_to_n

# -----------------------------------------------------------
# 函数原型: uint32_t sum_to_n(uint32_t n);
# 功能: 计算 1 + 2 + ... + n 的和
# 输入: a0 = n
# 输出: a0 = result
# -----------------------------------------------------------
sum_to_n:
    # 初始化
    li t0, 0        # t0 用作累加器 (sum = 0)
    li t1, 1        # t1 用作计数器 (i = 1)

loop_start:
    # 循环判断: 如果 i (t1) > n (a0)，则跳转到循环结束
    bgt t1, a0, loop_end

    # 累加: sum = sum + i
    add t0, t0, t1

    # 计数器递增: i++
    addi t1, t1, 1

    # 跳转回循环开始
    j loop_start

loop_end:
    # 将结果放入返回值寄存器 a0
    mv a0, t0

    # 返回调用者 (本质是 jalr x0, ra, 0)
    ret
