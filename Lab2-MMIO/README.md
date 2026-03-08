# Lab2 - MMIO & GPIO Driver

## Objective
Implement bare-metal GPIO and UART drivers using Memory-Mapped I/O on the HBirdv2 E203 SoC.

## MMIO Register Map

| Register | Address | Description |
|----------|---------|-------------|
| GPIOA_PADDIR | 0x10012000 | Pin direction (1=output) |
| GPIOA_PADIN | 0x10012004 | Input value |
| GPIOA_PADOUT | 0x10012008 | Output value |
| GPIOA_IOFCFG | 0x1001201C | IO function config |
| UART0_THR/RBR | 0x10013000 | TX/RX data register |
| UART0_LSR | 0x10013014 | Line status register |

## GPIO Pin Mapping
- **LED0-LED5**: GPIOA bit[20:25] (active low)
- **SW0-SW5**: GPIOA bit[26:31] (active high)

## Features
1. **GPIO 3-step initialization**: Clear IOFCFG → Set PADDIR → Init PADOUT
2. **LED running light**: 3-round sweep using Read-Modify-Write protection
3. **Switch-LED linkage**: Real-time SW→LED mapping with change detection
4. **UART interaction**: Status register read and TX/RX demo

## Detailed Documentation
See [docs/Lab2-MMIO.md](../docs/Lab2-MMIO.md) for full implementation notes.
