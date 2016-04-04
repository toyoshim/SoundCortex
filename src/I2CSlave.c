// Copyright 2016, Takashi Toyoshima <toyoshim@gmail.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of the authors nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#include "I2CSlave.h"

// Constant variables to improve readability.
enum {
  CLK_I2C = (1 << 5),
  CLK_IOCON = (1 << 18),

  RESET_I2C_N = (1 << 6),

  ADR_ENABLE = 0,
  ADR_DISABLE = 1,

  PIN_0 = 0,
  PIN_5 = 5,

  PIO_MODE_INACTIVE = (0 << 3),
  PIO_MODE_PULL_DOWN = (1 << 3),
  PIO_MODE_PULL_UP = (2 << 3),
  PIO_OPENDRAIN = (1 << 10),

  PINASSIGN7_I2C_SDA_IO_MASK = 0xff000000UL,
  PINASSIGN7_I2C_SDA_IO_BIT = 24,
  PINASSIGN8_I2C_SCL_IO_MASK = 0x000000ffUL,
  PINASSIGN8_I2C_SCL_IO_BIT = 0,

  I2C_CFG_SLVEN = (1 << 1),

  I2C_INT_SLVPENDING = (1 << 8),
  I2C_INT_SLVDESEL = (1 << 15),

  I2C_STAT_SLVSTATE_MASK = (3 << 9),
  I2C_STAT_SLVSTATE_ADDR = (0 << 9),
  I2C_STAT_SLVSTATE_WRITE = (1 << 9),
  I2C_STAT_SLVSTATE_READ = (2 << 9),
  I2C_STAT_SLVIDX_BIT = 12,
  I2C_STAT_SLVIDX_MASK = (3 << 12),

  I2C_SLVCTL_SLVCONTINUE = (1 << 0),
  I2C_SLVCTL_SLVNACK = (1 << 1)
};

void I2CSlaveInit(uint8_t address1, uint8_t address2) {
  LPC_SYSCON->SYSAHBCLKCTRL |= CLK_I2C | CLK_IOCON;

  LPC_SYSCON->PRESETCTRL &= ~RESET_I2C_N;
  LPC_SYSCON->PRESETCTRL |= RESET_I2C_N;

  LPC_I2C->DIV = 0;  // Try the fastest mode as possible.
  //LPC_I2C->DIV = 30 - 1;  // 12MHz = 100kHz * x4 sampling * 30
  LPC_I2C->SLVADR1 = (address1 << 1) | ADR_ENABLE;
  if (address2)
    LPC_I2C->SLVADR2 = (address2 << 1) | ADR_ENABLE;

  LPC_SWM->PINASSIGN7 = (LPC_SWM->PINASSIGN7 & ~PINASSIGN7_I2C_SDA_IO_MASK) | (PIN_0 << PINASSIGN7_I2C_SDA_IO_BIT);
  LPC_SWM->PINASSIGN8 = (LPC_SWM->PINASSIGN8 & ~PINASSIGN8_I2C_SCL_IO_MASK) | (PIN_5 << PINASSIGN8_I2C_SCL_IO_BIT);

  LPC_IOCON->PIO0_0 = PIO_OPENDRAIN | PIO_MODE_INACTIVE;
  LPC_IOCON->PIO0_5 = PIO_OPENDRAIN | PIO_MODE_INACTIVE;

  LPC_I2C->CFG = I2C_CFG_SLVEN;

  LPC_I2C->INTENSET = I2C_INT_SLVPENDING | I2C_INT_SLVDESEL;
  NVIC_EnableIRQ(I2C_IRQn);
}

void I2C_IRQHandler(void) {
  if (LPC_I2C->INTSTAT & I2C_INT_SLVDESEL) {
    I2CSlaveStop();
    LPC_I2C->STAT = I2C_INT_SLVDESEL;
  } else if (LPC_I2C->INTSTAT & I2C_INT_SLVPENDING) {
    switch (LPC_I2C->STAT & I2C_STAT_SLVSTATE_MASK) {
    case I2C_STAT_SLVSTATE_ADDR: {
      int index = (LPC_I2C->STAT & I2C_STAT_SLVIDX_MASK) >> I2C_STAT_SLVIDX_BIT;
      I2CSlaveStart((index == 1) ? (LPC_I2C->SLVADR1 >> 1) :
                    (index == 2) ? (LPC_I2C->SLVADR2 >> 1) : 0);
      LPC_I2C->SLVCTL = I2C_SLVCTL_SLVCONTINUE;
      break; }
    case I2C_STAT_SLVSTATE_WRITE:
      if (I2CSlaveWrite(LPC_I2C->SLVDAT))
        LPC_I2C->SLVCTL = I2C_SLVCTL_SLVCONTINUE;
      else
        LPC_I2C->SLVCTL = I2C_SLVCTL_SLVNACK | I2C_SLVCTL_SLVCONTINUE;
      break;
    case I2C_STAT_SLVSTATE_READ: {
      uint8_t data;
      if (I2CSlaveRead(&data)) {
        LPC_I2C->SLVDAT = data;
        LPC_I2C->SLVCTL = I2C_SLVCTL_SLVCONTINUE;
      } else {
        LPC_I2C->SLVCTL = I2C_SLVCTL_SLVNACK | I2C_SLVCTL_SLVCONTINUE;
      }
      break; }
    default:
      LPC_I2C->SLVCTL = I2C_SLVCTL_SLVNACK | I2C_SLVCTL_SLVCONTINUE;
    }
  } else {
    LPC_I2C->SLVCTL = I2C_SLVCTL_SLVNACK | I2C_SLVCTL_SLVCONTINUE;
  }
}

__attribute__ ((weak)) void I2CSlaveStart(uint8_t addr) {}
__attribute__ ((weak)) void I2CSlaveStop() {}
__attribute__ ((weak)) bool I2CSlaveWrite(uint8_t data) { return true; }
__attribute__ ((weak)) bool I2CSlaveRead(uint8_t* data) { return false; }
