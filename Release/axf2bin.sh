#!/bin/sh

APP=SoundCortex
arm-none-eabi-objcopy -O binary $APP.axf $APP.bin
arm-none-eabi-objcopy -O ihex $APP.axf $APP.hex
