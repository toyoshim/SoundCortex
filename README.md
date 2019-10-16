# SoundCortex
A firmware that makes your microcomputer work as a historical sound chip.
See [SoundCortexLPC](https://github.com/toyoshim/SoundCortexLPC) for LPC810/812.

## How to use
You can access to the chip through I2C, SPI, or IOEXT bus.

If you build it with I2C support enabled, the slave address for PSG is 0x50, and one for SCC is 0x51. You can write two bytes data to write to internal registers for each chip emulation with each address. The first byte is the register address, and the second byte is data to write. Internal register map for PSG is compatible with AY-3-8910, and one for SCC is compatible with lower 8-bit address of memory mapped SCC+ cart.

If you build it with SPI support enabled, you can send 16-bit data in MSG first over MOSI of SPI mode 0. The most significant 8-bit is assumed as a register address, and the reset 8-bit is assumed as data to write. Address 0xff is specially handled as a page setting register, i.e. sending 0xff50 maps internal PSG register into the 8-bit address space, and 0xff51 does one of SCC.

If you build it with IOEXT support enabled, IO ports are assigned as
 - 0xA0: PSG address latch
 - 0xA1: PSG data read/write
 - 0xA2: SCC address latch
 - 0xA3: SCC data read/write

To use this chip from PC, you may need something that allows your PC to send I2C transactions.
You may be interested in [I2CBridge](https://github.com/toyoshim/I2CBridge) that converts USART serial to I2C.

To use from Raspberry Pi, you can just use built-in I2C. [Here](https://youtu.be/buaCriXYXNY) is a demo movie that controls the chip from Raspberry Pi.
