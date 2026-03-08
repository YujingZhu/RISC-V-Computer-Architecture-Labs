#include <stdio.h>
#include <stdint.h>

extern uint32_t sum_to_n(uint32_t n);  // 声明汇编函数

int main() {
    uint32_t result = sum_to_n(10);
    return 0;
}
