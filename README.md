# SoundCortex
A firmware that makes your LPC810/812 work as a historical sound chip

## What's this?
Firmware that makes your LPC810/812 work like a PSG, AY-3-8910. But all registers can be accessible via I2C/SPI.
LPC812 version can support IOEXT protocol for my [CP/Mega32U2](https://github.com/toyoshim/cp-mega88).

## Schematic
[![Schematic](https://raw.githubusercontent.com/toyoshim/SoundCortex/master/schem.png "Schematic")](https://upverter.com/toyoshim/564092a49959599c/I2CBridge/)

This contains RC LPF that consists from 68Ω register and 0.1uF capasitor. Cut-off frequency of the filter is about 23.4kHz.
You might be able to connect it to mixer without them, but please do it at your own risk. If you ain't lucky enough, your equipments will be damaged.

## How to build
This repository includes prebiult binaries under Release/ directory, but also you can build it by yourself with the LPCXpresso project files included in this repository too.
You need to import CMSIS_CORE_LPX8xx and lpc800_driver_lib from <lpcxpresso>/Examples/Legacy/NXP/LPC800/LPC8xx_Libraries.zip to use these libraries.
For MCUXpresso, you may need CMSIS_DSPLIB_CM0 instead of lpc800_driver_lib. That can be imported from <mcuxpresso>/Examples/CMSIS_DSPLIB/CMSIS_DSPLIB_Latest.zip.

If you are using MCUXpresso, select 'Import...' from the popup menu, and import this project by 'General > Existing Projects into Workspace'. Dependent libraries can be imported as a project archive, via 'Import project(s) from file system...' menu. Library should exist in <mcuxpresso>/ide/Examples/... .

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

## Pin assign

### LPC810
|pin|  function  | |pin|  function   |
|---|------------|-|---|-------------|
| 1 |SCL (SCK *1)| | 8 |SDA (MOSI *1)|
| 2 |Sound Out   | | 7 |GND          |
| 3 |SWCLK       | | 6 |V3.3         |
| 4 |SWDIO       | | 5 |(SSEL *1)    |

(*1) is description for SPI enabled built

You may need to hold the pin 1 high on PON, since it is originally assigned to /RESET and the reset function is disabled after start up.

### LPC812 (SO20)
#### for IOEXT
|pin|  function  | |pin|  function   |
|---|------------|-|---|-------------|
| 1 |-           | |20 |D6           |
| 2 |D5          | |19 |/W           |
| 3 |D4          | |18 |/DATA        |
| 4 |/READY      | |17 |-            |
| 5 |Sound Out   | |16 |GND          |
| 6 |SWCLK       | |15 |V3.3         |
| 7 |SWDIO       | |14 |D0           |
| 8 |D3          | |13 |D1           |
| 9 |D2          | |12 |/IOE         |
|10 |-           | |11 |D7           |

#### for I2C/SPI
|pin|  function  | |pin|  function   |
|---|------------|-|---|-------------|
| 1 |-           | |20 |-            |
| 2 |-           | |19 |SDA (MOSI *1)|
| 3 |-           | |18 |-            |
| 4 |SCL (SCK *1)| |17 |-            |
| 5 |Sound Out   | |16 |GND          |
| 6 |SWCLK       | |15 |V3.3         |
| 7 |SWDIO       | |14 |-            |
| 8 |-           | |13 |-            |
| 9 |-           | |12 |(SSEL *1)    |
|10 |-           | |11 |-            |

(*1) is description for SPI enabled built

You may need to hold the pin 4 high on PON, since it is originally assigned to /RESET and the reset function is disabled after start up.
Note that ISP is disabled by CRP for the IOEXT build.
