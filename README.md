# SoundCortex
A firmware that makes your LPC810 work as a historical sound chip

## What's this?
Firmware that makes your LPC810 work like a PSG, AY-3-8910. But all registers can be accessible via I2C.

## Schematic
[![Schematic](https://raw.githubusercontent.com/toyoshim/SoundCortex/master/schem.png "Schematic")](https://upverter.com/toyoshim/564092a49959599c/I2CBridge/)

This contains RC LPF that consists from 680Ω register and 0.01uF capasitor. Cut-off frequency of the filter is about 23.4kHz.
You might be able to connect it to mixer without them, but please do it at your own risk. If you ain't lucky enough, your equipments will be damaged.

## How to build
This repository includes prebiult binaries under Release/ directory, but also you can build it by yourself with the LPCXpresso project files included in this repository too.
You need to import CMSIS_CORE_LPX8xx and lpc800_driver_lib from <lpcxpresso>/Examples/Legacy/NXP/LPC800/LPC8xx_Libraries.zip to use these libraries.

If you are using MCUXpresso, select 'Import...' from the popup menu, and import this project by 'General > Existing Projects into Workspace'. Dependent libraries can be imported as a project archive, via 'Import project(s) from file system...' menu. Library should exist in <mcuxpresso>/ide/Examples/... .

## How to use
You can access to the chip through I2C or SPI bus.

If you build it with I2C support enabled, the slave address for PSG is 0x50, and one for SCC is 0x51. You can write two bytes data to write to internal registers for each chip emulation with each address. The first byte is the register address, and the second byte is data to write. Internal register map for PSG is compatible with AY-3-8910, and one for SCC is compatible with lower 8-bit address of memory mapped SCC+ cart.

If you build it with SPI support enabled, you can send 16-bit data in MSG first over MOSI of SPI mode 0. The most significant 8-bit is assumed as a register address, and the reset 8-bit is assumed as data to write. Address 0xff is specially handled as a page setting register, i.e. sending 0xff50 maps internal PSG register into the 8-bit address space, and 0xff51 does one of SCC.

To use this chip from PC, you may need something that allows your PC to send I2C transactions.
You may be interested in [I2CBridge](https://github.com/toyoshim/I2CBridge) that converts USART serial to I2C.

To use from Raspberry Pi, you can just use built-in I2C. [Here](https://youtu.be/buaCriXYXNY) is a demo movie that controls the chip from Raspberry Pi.

## Pin assign
|pin|  function  | |pin|  function   |
|---|------------|-|---|-------------|
| 1 |SCL (SCK *1)| | 8 |SDA (MOSI *1)|
| 2 |Sound Out   | | 7 |GND          |
| 3 |SWCLK       | | 6 |V3.3         |
| 4 |SWDIO       | | 5 |(SSEL *1)    |

(*1) is description for SPI enabled built

You may need to hold the pin 1 high on PON, since it is originally assigned to /RESET and the reset function is disabled after start up.
