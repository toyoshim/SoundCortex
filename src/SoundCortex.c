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
#include <stdbool.h>

#include "BuildConfig.h"
#include "SoundCortex.h"

uint16_t SoundCortexUpdate() {
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

#if defined(BUILD_IOEXT)
static uint8_t psg_address = 0;
static uint8_t scc_address = 0;

bool IOEXTSlaveAccess(uint8_t port) {
  switch (port) {
#if defined(BUILD_PSG)
  case PSG_ADDRESS_PORT:
  case PSG_DATA_PORT:
    break;
#endif
#if defined(BUILD_SCC)
  case SCC_ADDRESS_PORT:
  case SCC_DATA_PORT:
    break;
#endif
  default:
    return false;
  }
  return true;
}

bool IOEXTSlaveWrite(uint8_t port, uint8_t data) {
  switch (port) {
#if defined(BUILD_PSG)
  case PSG_ADDRESS_PORT:
    psg_address = data;
    break;
  case PSG_DATA_PORT:
    PSGWrite(psg_address, data);
    break;
#endif
#if defined(BUILD_SCC)
  case SCC_ADDRESS_PORT:
    scc_address = data;
    break;
  case SCC_DATA_PORT:
    SCCWrite(scc_address, data);
    break;
#endif
  default:
    return false;
  }
  return true;
}

bool IOEXTSlaveRead(uint8_t port, uint8_t* data) {
  switch (port) {
#if defined(BUILD_PSG)
  case PSG_ADDRESS_PORT:
    *data = psg_address;
    break;
  case PSG_DATA_PORT:
    PSGRead(psg_address, data);
    break;
#endif
#if defined(BUILD_SCC)
  case SCC_ADDRESS_PORT:
    *data = scc_address;
    break;
  case SCC_DATA_PORT:
    SCCRead(scc_address, data);
    break;
#endif
  default:
    return false;
  }
  return true;
}
#endif

void SlaveInit(uint8_t address1, uint8_t address2) {
#if defined(BUILD_I2C)
  I2CSlaveInit(address1, address2);
#endif
#if defined(BUILD_SPI)
  SPISlaveInit();
#endif
#if defined(BUILD_IOEXT)
  IOEXTSlaveInit();
#endif
}

void SoundCortexInit(uint32_t sample_rate) {
#if defined(BUILD_PSG) && !defined(BUILD_SCC)
  PSGInit(sample_rate);
  SlaveInit(PSG_ADDRESS, 0);
#elif !defined(BUILD_PSG) && defined(BUILD_SCC)
  SCCInit(sample_rate);
  SlaveInit(SCC_ADDRESS, 0);
#elif defined(BUILD_PSG) && defined(BUILD_SCC)
  PSGInit(sample_rate);
  SCCInit(sample_rate);
  SlaveInit(PSG_ADDRESS, SCC_ADDRESS);
#else
#endif
#if defined(BUILD_MIDI)
  MIDIInit(SMF);
#endif
}
