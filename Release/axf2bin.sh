#!/bin/sh

PATH=/cygdrive/d/nxp/LPCXpresso_8.2.0_647/lpcxpresso/tools/bin:$PATH

APP=SoundCortex
arm-none-eabi-objcopy -O binary $APP.axf $APP.bin
arm-none-eabi-objcopy -O ihex $APP.axf $APP.hex
