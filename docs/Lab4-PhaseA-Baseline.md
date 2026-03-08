---

# 实验一：基线环境搭建与 CoreMark 跑通验证 (README)

## 1. 实验目的

本阶段旨在 DDR200T FPGA 平台上搭建 HBirdv2 E203 SoC 的基线测试环境，成功编译并运行 CoreMark 基准测试程序。通过调整迭代次数（Iterations），确保测试运行时间满足官方要求的 ≥10 秒，验证系统功能的正确性与稳定性，并获取基线性能数据（CoreMark Score 与 CoreMark/MHz）。

## 2. 实验环境

* **硬件平台**: Nuclei DDR200T FPGA Evaluation Kit (Xilinx XC7A200T)
* **处理器核**: HBirdv2 E203 (RISC-V RV32IMAC)
* **开发工具**: Nuclei Studio IDE / Nuclei RISC-V GCC Toolchain
* **SDK**: HBird SDK (hbird-sdk)
* **调试器**: HBird Debugger (JTAG)
* **串口设置**: 波特率 115200

## 3. 关键代码修改说明

为了满足 CoreMark 运行时间大于 10 秒的要求，并解决 IDE 工程配置可能存在的宏定义冲突，对 `application/core_portme.h` 文件进行了修改。

### 修改文件：`application/core_portme.h`

**修改内容**：
使用 `#undef` 强制取消可能存在的 `ITERATIONS` 宏定义（如 IDE 默认的 500），并将其重新定义为 **4000**。

**完整修改代码片段**：

```c
#ifndef CORE_PORTME_H
#define CORE_PORTME_H

#include "hbird_sdk_soc.h"

// ---------------------------------------------------------------------------
// 编译器优化参数 (保持默认)
// ---------------------------------------------------------------------------
#ifndef FLAGS_STR
    #define FLAGS_STR "-O2 -funroll-all-loops -finline-limit=600 -ftree-dominator-opts -fno-if-conversion2 -fselective-scheduling -fno-code-hoisting -fno-common -funroll-loops -finline-functions -falign-functions=4 -falign-jumps=4 -falign-loops=4"
#endif

// ---------------------------------------------------------------------------
// 性能运行模式设置
// ---------------------------------------------------------------------------
#ifndef PERFORMANCE_RUN
#define PERFORMANCE_RUN 1
#endif

// ---------------------------------------------------------------------------
// 【关键修改】强制设置迭代次数
// ---------------------------------------------------------------------------
// 删除 #ifndef 和 #endif，使用 #undef 强制覆盖 IDE 或 Makefile 中的默认设置
// 必须设置为 4000 以确保在 E203 (16MHz) 上运行时间 > 10秒
#undef ITERATIONS
#define ITERATIONS 4000

// ---------------------------------------------------------------------------
// CoreMark 标准移植定义 (未修改)
// ---------------------------------------------------------------------------
#ifndef FESDK_CORE_PORTME_H
#define FESDK_CORE_PORTME_H

#include <stdint.h>
#include <stddef.h>

#define HAS_FLOAT 1
#define HAS_TIME_H 1
#define USE_CLOCK 1
#define HAS_STDIO 1
#define HAS_PRINTF 1
#define SEED_METHOD SEED_VOLATILE
#define CORE_TICKS uint64_t
#define ee_u8 uint8_t
#define ee_u16 uint16_t

#if defined(__riscv_xlen) && (__riscv_xlen == 64)
#define ee_u32 int32_t
#else
#define ee_u32 uint32_t
#endif
#define ee_s16 int16_t
#define ee_s32 int32_t
#define ee_ptr_int uintptr_t
#define ee_size_t size_t
#define COMPILER_FLAGS FLAGS_STR

#define align_mem(x) (void *)(((ee_ptr_int)(x) + sizeof(ee_u32) - 1) & -sizeof(ee_u32))

#ifdef __GNUC__
# define COMPILER_VERSION "GCC"__VERSION__
#else
# error
#endif

#define MEM_METHOD MEM_STACK
#define MEM_LOCATION "STACK"

#define MAIN_HAS_NOARGC 0
#define MAIN_HAS_NORETURN 0

#define MULTITHREAD 1
#define USE_PTHREAD 0
#define USE_FORK 0
#define USE_SOCKET 0

#define default_num_contexts MULTITHREAD

typedef int core_portable;
static void portable_init(core_portable *p, int *argc, char *argv[]) {}
static void portable_fini(core_portable *p) {}

#if !defined(PROFILE_RUN) && !defined(PERFORMANCE_RUN) && !defined(VALIDATION_RUN)
#if (TOTAL_DATA_SIZE==1200)
#define PROFILE_RUN 1
#elif (TOTAL_DATA_SIZE==2000)
#define PERFORMANCE_RUN 1
#else
#define VALIDATION_RUN 1
#endif
#endif

#endif

#endif

```

## 4. 实验结果数据

以下数据采集自串口输出，验证了修改的有效性及系统基线性能。

### 4.1 运行日志摘要

```text
HummingBird SDK Build Time: Jan 10 2026, 10:17:42
Download Mode: ILM
CPU Frequency 16002252 Hz
Start to run coremark for 4000 iterations
2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 1756728445
Total time (secs): 109.781468
Iterations/Sec   : 36.436022
Iterations       : 4000
Compiler version : GCC14.2.1 20240816
Compiler flags   : -O2 -funroll-all-loops -finline-limit=600 -ftree-dominator-opts -fno-if-conversion2 -fselective-scheduling -fno-code-hoisting -fno-common -funroll-loops -finline-functions -falign-functions=4 -falign-jumps=4 -falign-loops=4
Memory location  : STACK
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x65c5
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 36.436022 / GCC14.2.1 20240816 -O2 ... / STACK

```

### 4.2 性能指标统计

| 指标 | 实验值 | 说明 |
| --- | --- | --- |
| **Total Time** | **109.78 s** | 满足 CoreMark 规则要求的 > 10s，结果有效。 |
| **Iterations** | **4000** | 修改后的迭代次数已生效（原默认值为 500）。 |
| **CoreMark Score** | **36.436** | (Iterations / Total time) |
| **CPU Frequency** | **16.00 MHz** | 实际运行频率。 |
| **CoreMark/MHz** | **2.277** | 归一化性能指标，符合 E203 预期范围。 |
| **Validation** | **PASSED** | CRC 校验通过 (Correct operation validated)。 |

## 5. 端口信息 (FPGA 约束与连接)

本实验基于 Nuclei DDR200T 开发板，涉及的主要硬件接口如下：

* **UART 接口 (用于 printf 输出实验结果)**:
* **TX (FPGA 侧)**: `C4` (MCU_UART1_TX) -> 连接至板载 MCU 虚拟串口
* **RX (FPGA 侧)**: `C5` (MCU_UART1_RX) -> 连接至板载 MCU 虚拟串口
* **波特率**: 115200


* **时钟与复位**:
* **系统时钟 (CLK_IN)**: `R4` (50MHz板载晶振) -> 经 MMCM 分频产生 16MHz core clock
* **系统复位 (rst_n)**: `T6` (板载按键/MCU控制)


* **JTAG 调试接口**:
* 连接至 MCU JTAG 协议转换模块，用于程序下载与 GDB 调试。



## 6. 结论

实验一修改代码成功生效，CoreMark 运行时间达到 109 秒，远超标准要求的 10 秒，且 CRC 校验通过。获得的 2.277 CoreMark/MHz 分数可作为后续微结构优化的 Baseline（基线）数据进行对比。