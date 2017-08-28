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
#include "LPC8xx.h"

#include <stdbool.h>

#include "SCTimer.h"

//#define BUILD_I2C
#define BUILD_SPI

#define BUILD_PSG
#define BUILD_SCC

//#define BUILD_MIDI

#if defined(BUILD_I2C)
#  include "I2CSlave.h"
#endif

#if defined(BUILD_PSG)
#  include "PSG.h"
#endif

#if defined(BUILD_SCC)
#  include "SCC.h"
#endif

#if defined(BUILD_SPI)
#  include "SPISlave.h"
#endif

#if defined(BUILD_MIDI)
#  include "MIDI.h"
#  include "SMF.h"
#endif

// Constant variables to improve readability.
enum {
  SYSPLL_PD = (1 << 7),
  SEL_IRC = 0,
  SEL_PLLOUT = 3,
  CLK_ENA_X = 0,
  CLK_ENA = 1,
  MSEL_X4 = 3,
  PSEL_P1 = 1,
  PLL_LOCK = 1,

  CLK_GPIO = (1 << 6),
  CLK_SWM = (1 << 7),
  CLK_IOCON = (1 << 18),

  DIR_INPUT = 0,
  DIR_OUTPUT = 1,

  PIO0_4 = 4,  // PIN 2

  PIO_MODE_INACTIVE = (0 << 3),

  DISABLE_RESET = (1 << 6),

  PINASSIGN6_CTOUT_0_O_MASK = 0xff000000UL,
  PINASSIGN6_CTOUT_0_O_BIT = 24,

  SET = 1,

  PSG_ADDRESS = 0x50,
  SCC_ADDRESS = 0x51,
  PWM_10BIT = 1023  // Sampling rate = 48MHz / 1024 = 46.875kHz
};

// Use an empty function instead of linking with CMSIS_CORE_LPC8xx library.
void SystemInit() {}

uint16_t SCTimerPWMUpdate() {
#if defined(BUILD_MIDI)
  MIDIUpdate(21, true, 120);  // 21.3usec
#endif
  // TODO: Use signed signals for both.
#if defined(BUILD_PSG) && !defined(BUILD_SCC)
  return PSGUpdate();
#elif !defined(BUILD_PSG) && defined(BUILD_SCC)
  return 320 + (SCCUpdate() >> 1);
#elif defined(BUILD_PSG) && defined(BUILD_SCC)
  return 160 + (PSGUpdate() >> 1) + (SCCUpdate() >> 2);
#else
  return 0;
#endif
}

#if defined(BUILD_I2C)
// I2C Slave handling code.
static uint8_t i2c_addr = 0;
static uint8_t i2c_data_index = 0;
static uint8_t i2c_data_addr = 0;

void I2CSlaveStart(uint8_t addr) {
  i2c_addr = addr;
  i2c_data_index = 0;
}

bool I2CSlaveWrite(uint8_t data) {
  if (i2c_data_index == 0) {
    i2c_data_addr = data;
  } else if (i2c_data_index == 1) {
#  if defined(BUILD_PSG) && !defined(BUILD_SCC)
    return PSGWrite(i2c_data_addr, data);
#  elif !defined(BUILD_PSG) && defined(BUILD_SCC)
    return SCCWrite(i2c_data_addr, data);
#  elif defined(BUILD_PSG) && defined(BUILD_SCC)
    if (i2c_addr == PSG_ADDRESS)
      return PSGWrite(i2c_data_addr, data);
    return SCCWrite(i2c_data_addr, data);
#  else
    return false;
#  endif
  } else {
    return false;
  }
  i2c_data_index++;
  return true;
}

bool I2CSlaveRead(uint8_t* data) {
#  if defined(BUILD_PSG) && !defined(BUILD_SCC)
  return PSGRead(i2c_data_addr, data);
#  elif !defined(BUILD_PSG) && defined(BUILD_SCC)
  return SCCRead(i2c_data_addr, data);
#  elif defined(BUILD_PSG) && defined(BUILD_SCC)
  if (i2c_addr == PSG_ADDRESS)
    return PSGRead(i2c_data_addr, data);
  return SCCRead(i2c_data_addr, data);
#  else
  return false;
#  endif
}
#endif

#if defined(BUILD_SPI)
static uint8_t spi_chip_select = PSG_ADDRESS;

void SPISlaveWrite16(uint16_t data) {
  if ((data >> 8) == 0xff)
    spi_chip_select = data;
#if defined(BUILD_PSG)
  else if (spi_chip_select == PSG_ADDRESS)
    PSGWrite(data >> 8, data);
#endif
#if defined(BUILD_SCC)
  else if (spi_chip_select == SCC_ADDRESS)
    SCCWrite(data >> 8, data);
#endif
}
#endif

void SlaveInit(uint8_t address1, uint8_t address2) {
#if defined(BUILD_I2C)
  I2CSlaveInit(address1, address2);
#endif
#if defined(BUILD_SPI)
  SPISlaveInit();
#endif
}

// Initialization and main loop.
int main() {
  // Main Clock = 12MHz(Internal RC) x 4 = 48MHz
  LPC_SYSCON->PDRUNCFG &= ~SYSPLL_PD;
  LPC_SYSCON->SYSPLLCLKSEL = SEL_IRC;
  LPC_SYSCON->SYSPLLCLKUEN = CLK_ENA_X;
  LPC_SYSCON->SYSPLLCLKUEN = CLK_ENA;
  LPC_SYSCON->MAINCLKSEL = SEL_PLLOUT;
  LPC_SYSCON->MAINCLKUEN = CLK_ENA_X;
  LPC_SYSCON->MAINCLKUEN = CLK_ENA;
  LPC_SYSCON->SYSPLLCTRL = MSEL_X4 | PSEL_P1;
  while (!(LPC_SYSCON->SYSPLLSTAT & PLL_LOCK));

  LPC_SYSCON->SYSAHBCLKCTRL |= CLK_GPIO | CLK_SWM | CLK_IOCON;
  NVIC->IP[2] |= (0 << 14) | (1 << 6);

  LPC_SWM->PINENABLE0 |= DISABLE_RESET;

  LPC_IOCON->PIO0_4 = PIO_MODE_INACTIVE;
  LPC_SWM->PINASSIGN6 = (LPC_SWM->PINASSIGN6 & ~PINASSIGN6_CTOUT_0_O_MASK) | (PIO0_4 << PINASSIGN6_CTOUT_0_O_BIT);

#if defined(BUILD_PSG) && !defined(BUILD_SCC)
  PSGInit();
  SlaveInit(PSG_ADDRESS, 0);
#elif !defined(BUILD_PSG) && defined(BUILD_SCC)
  SCCInit();
  SlaveInit(SCC_ADDRESS, 0);
#elif defined(BUILD_PSG) && defined(BUILD_SCC)
  PSGInit();
  SCCInit();
  SlaveInit(PSG_ADDRESS, SCC_ADDRESS);
#else
#endif
#if defined(BUILD_MIDI)
  MIDIInit(SMF);
#endif
  SCTimerPWMInit(PWM_10BIT);

  for (;;);
  return 0;
}
