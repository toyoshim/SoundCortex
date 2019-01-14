// Copyright 2019, Takashi Toyoshima <toyoshim@gmail.com>
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
#include "IOEXTSlave.h"

#include <NXP/crp.h>

// Disable ISP by Code Read Protection because UART-ISP pin is assigned to
// PIN0_12 that may be activated by chance via floating IOEXT values on PON.
// CRP feature should be activated by linker script to handle the special
// .crp section.
__CRP unsigned int CRP_WORD = CRP_NO_ISP;

// Constant variables to improve readability.
enum {
  CLK_GPIO = (1 << 6),
  CLK_IOCON = (1 << 18),

  PIO_MODE_INACTIVE = (0 << 3),
  PIO_OPENDRAIN = (1 << 10),
  PIO_TRUE_OPENDRAIN = (1 << 8),

  PIO_nW = (1 << 0),
  PIO_nIOE = (1 << 1),
  PIO_nREADY = (1 << 5),
  PIO_nDATA = (1 << 6),
  PIO_D0 = (1 << 8),
  PIO_D1 = (1 << 9),
  PIO_D2 = (1 << 10),
  PIO_D3 = (1 << 11),
  PIO_D4 = (1 << 12),
  PIO_D5 = (1 << 13),
  PIO_D6 = (1 << 14),
  PIO_D7 = (1 << 15),
  PIO_DATA =
      (PIO_D0 | PIO_D1 | PIO_D2 | PIO_D3 | PIO_D4 | PIO_D5 | PIO_D6 | PIO_D7)
};

void PININT0_IRQHandler(void) {
  NVIC_DisableIRQ(PININT0_IRQn);

  uint32_t io = LPC_GPIO_PORT->PIN0;
  uint8_t port = (io >> 8) & 0xff;
  if ((io & PIO_nDATA) && IOEXTSlaveAccess(port)) {
    do {
      LPC_GPIO_PORT->CLR0 = PIO_nREADY;
      while (!(LPC_GPIO_PORT->PIN0 & PIO_nIOE));
      LPC_GPIO_PORT->SET0 = PIO_nREADY;
      while (LPC_GPIO_PORT->PIN0 & PIO_nIOE);
    } while (LPC_GPIO_PORT->PIN0 & PIO_nDATA);
    if (io & PIO_nW) {
      LPC_GPIO_PORT->DIR0 |= PIO_DATA;
      LPC_GPIO_PORT->PIN0 &= ~(0xff << 8);
      uint8_t data;
      IOEXTSlaveRead(port, &data);
      LPC_GPIO_PORT->CLR0 = 0xff << 8;
      LPC_GPIO_PORT->SET0 = data << 8;
    } else {
      uint8_t data = LPC_GPIO_PORT->PIN0 >> 8;
      IOEXTSlaveWrite(port, data);
    }
    LPC_GPIO_PORT->CLR0 = PIO_nREADY;
    while (!(LPC_GPIO_PORT->PIN0 & PIO_nIOE));
    LPC_GPIO_PORT->DIR0 &= ~PIO_DATA;
    LPC_GPIO_PORT->SET0 = PIO_nREADY;
  }

  LPC_PIN_INT[0].FALL = (1 << 0);  // Reset detected falling edge flag
  NVIC_ClearPendingIRQ(PININT0_IRQn);
  NVIC_EnableIRQ(PININT0_IRQn);
}

void IOEXTSlaveInit() {
  LPC_SYSCON->SYSAHBCLKCTRL |=  CLK_GPIO | CLK_IOCON;
  LPC_SYSCON->PINTSEL[0] = 1;  // Select PIO0_1 for PINT0.
  LPC_PIN_INT[0].ISEL = 0;  // Edge sensitive
  LPC_PIN_INT[0].IENF = 1;  // Enables falling-edge interrupt
  NVIC->IP[6] |= (3 << 6);  // Set PININT0 interrupt priority to the lowest

  LPC_IOCON->PIO0_0 = PIO_MODE_INACTIVE;  // /W
  LPC_IOCON->PIO0_1 = PIO_MODE_INACTIVE;  // /IOE
  LPC_IOCON->PIO0_5 = PIO_OPENDRAIN | PIO_MODE_INACTIVE;  // /READY
  LPC_IOCON->PIO0_6 = PIO_MODE_INACTIVE;  // /DATA (Use PIN0_7 instead to be 5V tolerant)
  LPC_IOCON->PIO0_8 = PIO_OPENDRAIN | PIO_MODE_INACTIVE;  // D0
  LPC_IOCON->PIO0_9 = PIO_OPENDRAIN | PIO_MODE_INACTIVE;  // D1
  LPC_IOCON->PIO0_10 = PIO_TRUE_OPENDRAIN;  // D2
  LPC_IOCON->PIO0_11 = PIO_TRUE_OPENDRAIN;  // D3
  LPC_IOCON->PIO0_12 = PIO_OPENDRAIN | PIO_MODE_INACTIVE;  // D4
  LPC_IOCON->PIO0_13 = PIO_OPENDRAIN | PIO_MODE_INACTIVE;  // D5
  LPC_IOCON->PIO0_14 = PIO_OPENDRAIN | PIO_MODE_INACTIVE;  // D6
  LPC_IOCON->PIO0_15 = PIO_OPENDRAIN | PIO_MODE_INACTIVE;  // D7

  LPC_GPIO_PORT->DIR0 &= ~(PIO_nW | PIO_nIOE | PIO_nDATA | PIO_DATA);
  LPC_GPIO_PORT->PIN0 |= PIO_nREADY;
  LPC_GPIO_PORT->DIR0 |= PIO_nREADY;

  NVIC_EnableIRQ(PININT0_IRQn);
}

__attribute__ ((weak)) bool IOEXTSlaveAccess(uint8_t port) { return false; }
__attribute__ ((weak)) bool IOEXTSlaveWrite(uint8_t port, uint8_t data) { return false; }
__attribute__ ((weak)) bool IOEXTSlaveRead(uint8_t port, uint8_t* data) { return false; }
