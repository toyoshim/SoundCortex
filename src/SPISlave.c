// Copyright 2017, Takashi Toyoshima <toyoshim@gmail.com>
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
#include "SPISlave.h"

// Constant variables to improve readability.
enum {
  CLK_SWM = (1 << 7),
  CLK_SPI0 = (1 << 11),
  CLK_IOCON = (1 << 18),

  RESET_SPI0_N = (1 << 0),

  SPI0_TXDATCTL_LEN = (0xf << 24),  // 16-bits
  SPI0_RXDAT_RXSSEL_N = (1 << 16),

  PIO0_0 = 0,  // PIN 8 - MOSI
  PIO0_1 = 1,  // PIN 5 - SSEL
  PIO0_5 = 5,  // PIN 1 - SCK

  PIO_MODE_INACTIVE = (0 << 3),
  PIO_MODE_PULL_DOWN = (1 << 3),
  PIO_OPENDRAIN = (1 << 10),

  PINASSIGN3_SPI0_SCK_IO_MASK = 0xff000000UL,
  PINASSIGN3_SPI0_SCK_IO_BIT = 24,
  PINASSIGN4_SPI0_MOSI_IO_MASK = 0x000000ffUL,
  PINASSIGN4_SPI0_MOSI_IO_BIT = 0,
  PINASSIGN4_SPI0_SSEL_IO_MASK = 0x00ff0000UL,
  PINASSIGN4_SPI0_SSEL_IO_BIT = 16,

  SPI0_CFG_ENABLE = (1 << 0),
  SPI0_CFG_SLAVE = (0 << 2),
  SPI0_CFG_MSBF = (0 << 3),
  SPI0_CFG_MODE3 = (3 << 4),  // CPOL = 1, CPHA = 1

  SPI0_STAT_TXRDY = (1 << 1),
  SPI0_INT_RXRDY = (1 << 0),
};

void SPISlaveInit() {
  LPC_SYSCON->SYSAHBCLKCTRL |= CLK_SWM | CLK_SPI0 | CLK_IOCON;

  LPC_SYSCON->PRESETCTRL &= ~RESET_SPI0_N;
  LPC_SYSCON->PRESETCTRL |= RESET_SPI0_N;

  LPC_SWM->PINASSIGN3 = (LPC_SWM->PINASSIGN3 & ~PINASSIGN3_SPI0_SCK_IO_MASK) | (PIO0_5 << PINASSIGN3_SPI0_SCK_IO_BIT);
  LPC_SWM->PINASSIGN4 = (LPC_SWM->PINASSIGN4 & ~PINASSIGN4_SPI0_MOSI_IO_MASK) | (PIO0_0 << PINASSIGN4_SPI0_MOSI_IO_BIT);
  LPC_SWM->PINASSIGN4 = (LPC_SWM->PINASSIGN4 & ~PINASSIGN4_SPI0_SSEL_IO_MASK) | (PIO0_1 << PINASSIGN4_SPI0_SSEL_IO_BIT);

  LPC_IOCON->PIO0_0 = PIO_OPENDRAIN | PIO_MODE_INACTIVE;
  LPC_IOCON->PIO0_1 = PIO_OPENDRAIN | PIO_MODE_PULL_DOWN;  // Selected by default (on open)
  LPC_IOCON->PIO0_5 = PIO_OPENDRAIN | PIO_MODE_INACTIVE;

  LPC_SPI0->CFG = SPI0_CFG_ENABLE | SPI0_CFG_SLAVE | SPI0_CFG_MSBF | SPI0_CFG_MODE3;
  while (~LPC_SPI0->STAT & SPI0_STAT_TXRDY);
  LPC_SPI0->TXDATCTL = SPI0_TXDATCTL_LEN;  // This also manages RX data length.

  LPC_SPI0->INTENSET = SPI0_INT_RXRDY;
  NVIC_EnableIRQ(SPI0_IRQn);
}

void SPI0_IRQHandler(void) {
  if (LPC_SPI0->INTSTAT & SPI0_INT_RXRDY) {
    uint32_t data = LPC_SPI0->RXDAT;
    if ((data & SPI0_RXDAT_RXSSEL_N) == 0)
      SPISlaveWrite16(data);
  }
}

__attribute__ ((weak)) void SPISlaveWrite16(uint16_t data) {}
