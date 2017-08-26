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

#include <stdbool.h>
#include <stdint.h>

#include "PSG.h"

static uint16_t note_param[128] = {
  0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff,

  0xd5d, 0xc9c, 0xbe7, 0xb3c, 0xa9b, 0xa02, 0x973, 0x8eb, 0x86b, 0x7f2, 0x780, 0x714,
  0x6af, 0x64e, 0x5f4, 0x59e, 0x54e, 0x501, 0x4ba, 0x476, 0x436, 0x3f9, 0x3c0, 0x38a,
  0x357, 0x327, 0x2fa, 0x2cf, 0x2a7, 0x281, 0x25d, 0x23b, 0x21b, 0x1fd, 0x1e0, 0x1c5,
  0x1ac, 0x194, 0x17d, 0x168, 0x153, 0x140, 0x12e, 0x11d, 0x10d, 0x0fe, 0x0f0, 0x0e3,
  0x0d6, 0x0ca, 0x0be, 0x0b4, 0x0aa, 0x0a0, 0x097, 0x08f, 0x087, 0x07f, 0x078, 0x071,
  0x06b, 0x065, 0x05f, 0x05a, 0x055, 0x050, 0x04c, 0x047, 0x043, 0x040, 0x03c, 0x039,
  0x035, 0x032, 0x030, 0x02d, 0x02a, 0x028, 0x026, 0x024, 0x022, 0x020, 0x01e, 0x01c,
  0x01b, 0x019, 0x018, 0x016, 0x015, 0x014, 0x013, 0x012, 0x011, 0x010, 0x00f, 0x00e,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

struct {
  const uint8_t* start;
  const uint8_t* cur;
  const uint8_t* end;
  uint32_t tempo;
  uint32_t tick_us;
  uint32_t tick;
  uint16_t division;
} MIDIWork;

static void MIDINoteOff(uint8_t ch, uint8_t note, uint8_t velocity) {
  if (ch > 2)
    return;
  PSGWrite(8 + ch, 0);
}

static void MIDINoteOn(uint8_t ch, uint8_t note, uint8_t velocity) {
  if (ch > 2)
    return;
  PSGWrite(ch * 2 + 0, note_param[note] & 0xff);
  PSGWrite(ch * 2 + 1, note_param[note] >> 8);
  PSGWrite(8 + ch, velocity >> 3);
}

static uint32_t MIDIDeltaTime() {
  uint32_t delta = 0;
  do {
    delta = (delta << 7) | (*MIDIWork.cur & 0x7f);
  } while ((*MIDIWork.cur++ & 0x80) != 0);
  return delta;
}

bool MIDIInit(const uint8_t* data) {
  if (data[0] != 'M' || data[1] != 'T' || data[2] != 'h' || data[3] != 'd')
    return false;  // invalid magic
  if (data[4] != 0 || data[5] != 0 || data[6] != 0 || data[7] != 6)
    return false;  // invalid size
  if (data[8] != 0 || data[9] != 0 || data[10] != 0 || data[11] != 1)
    return false;  // ! format 0
  MIDIWork.division = (data[12] << 8) | data[13];
  if (data[14] != 'M' || data[15] != 'T' || data[16] != 'r' || data[17] != 'k')
    return false;  // invalid magic
  uint32_t size =
      (data[18] << 24) | (data[19] << 16) | (data[20] << 8) | data[21];
  MIDIWork.start = MIDIWork.cur = &data[22];
  MIDIWork.end = &data[22 + size - 1];
  MIDIWork.tick_us = 0;
  MIDIWork.tick = 0;
  MIDIWork.tempo = 1000000;
  MIDIWork.tick_us = MIDIWork.tempo / MIDIWork.division;

  PSGWrite(7, 0x38);
  return true;
}

bool MIDIUpdate(uint16_t tick_us, bool repeat, uint16_t gap) {
  while (tick_us) {
    if (MIDIWork.tick == 0)
      MIDIWork.tick = MIDIDeltaTime() * MIDIWork.tick_us;
    if (MIDIWork.tick > tick_us) {
      MIDIWork.tick -= tick_us;
      return true;
    }
    tick_us -= MIDIWork.tick;
    MIDIWork.tick = 0;
    uint8_t status = *MIDIWork.cur++;
    switch (status & 0xf0) {
      case 0x80:
        MIDINoteOff(status & 0x0f, MIDIWork.cur[0], MIDIWork.cur[1]);
        MIDIWork.cur += 2;
        break;
      case 0x90:
        MIDINoteOn(status & 0x0f, MIDIWork.cur[0], MIDIWork.cur[1]);
        MIDIWork.cur += 2;
        break;
      case 0xf0:
        if (status == 0xff) {
          uint8_t type = *MIDIWork.cur++;
          uint8_t size = *MIDIWork.cur++;
          if (type == 0x2f && size == 0) {
            if (!repeat)
              return false;
            MIDIWork.cur = MIDIWork.start;
            MIDIWork.tick = (MIDIDeltaTime() + gap) * MIDIWork.tick_us;
          } else if (type == 0x51 && size == 3) {
            MIDIWork.tempo = (MIDIWork.cur[0] << 16) | (MIDIWork.cur[1] << 8) |
                (MIDIWork.cur[2]);
            MIDIWork.tick_us = MIDIWork.tempo / MIDIWork.division;
            MIDIWork.cur += 3;
          } else {
            MIDIWork.cur += size;
          }
          break;
        }
        // no break
      default:
        // not impl.
        // printf("$%02x\n", status);
        return false;
    }
  }
  return true;
}
