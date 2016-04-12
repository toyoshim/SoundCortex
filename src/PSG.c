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
#include "PSG.h"

// Constant variables to improve readability.
enum {
  CLK_MSX =  3579545UL,
  CLK_4MHZ = 4000000UL,
  FOUT = 46875UL,  // Sampling rate
};

const uint16_t vt[32] = {
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09, 0x0b, 0x0d, 0x10,
    0x13, 0x17, 0x1b, 0x20, 0x26, 0x2d, 0x36, 0x40,
    0x4c, 0x5a, 0x6b, 0x80, 0x98, 0xb4, 0xd6, 0xff
};

typedef struct {
  uint16_t tp;
  uint16_t ml;
} Channel;

typedef struct {
  uint32_t limit;
  uint32_t count;
  uint32_t on;
  uint32_t out;
  uint32_t tone;
  uint32_t noise;
} Synth;

typedef struct {
  uint32_t np;
  uint32_t limit;
  uint32_t count;
  uint32_t seed;
} Noise;

struct {
  uint32_t step;
  Synth synth[3];
  Noise noise;

  Channel channel[3];
} PSGWork;

void PSGInit() {
  PSGWork.step = CLK_MSX;
  for (int i = 0; i < 3; ++i) {
    PSGWork.synth[i].limit = 0;
    PSGWork.synth[i].count = 0;
    PSGWork.synth[i].on = 0;
    PSGWork.synth[i].out = 0;
    PSGWork.synth[i].tone = 1;
    PSGWork.synth[i].noise = 1;
  }
  PSGWork.noise.limit = 0;
  PSGWork.noise.count = 0;
  PSGWork.noise.seed = 0xffff;
}

bool PSGWrite(uint8_t reg, uint8_t value) {
  switch (reg) {
  case 0x00:  // TP[7:0] for Ch.A
    PSGWork.channel[0].tp = (PSGWork.channel[0].tp & 0x0f00) | value;
    PSGWork.synth[0].limit = (uint32_t)PSGWork.channel[0].tp * 16 * FOUT;
    break;
  case 0x01:  // TP[11:8] for Ch.A
    PSGWork.channel[0].tp = (PSGWork.channel[0].tp & 0x00ff) | ((uint16_t)(value & 0x0f) << 8);
    PSGWork.synth[0].limit = (uint32_t)PSGWork.channel[0].tp * 16 * FOUT;
    break;
  case 0x02:  // TP[7:0] for Ch.B
    PSGWork.channel[1].tp = (PSGWork.channel[1].tp & 0x0f00) | value;
    PSGWork.synth[1].limit = (uint32_t)PSGWork.channel[1].tp * 16 * FOUT;
    break;
  case 0x03:  // TP[11:8] for Ch.B
    PSGWork.channel[1].tp = (PSGWork.channel[1].tp & 0x00ff) | ((uint16_t)(value & 0x0f) << 8);
    PSGWork.synth[1].limit = (uint32_t)PSGWork.channel[1].tp * 16 * FOUT;
    break;
  case 0x04:  // TP[7:0] for Ch.C
    PSGWork.channel[2].tp = (PSGWork.channel[2].tp & 0x0f00) | value;
    PSGWork.synth[2].limit = (uint32_t)PSGWork.channel[2].tp * 16 * FOUT;
    break;
  case 0x05:  // TP[11:8] for Ch.C
    PSGWork.channel[2].tp = (PSGWork.channel[2].tp & 0x00ff) | ((uint16_t)(value & 0x0f) << 8);
    PSGWork.synth[2].limit = (uint32_t)PSGWork.channel[2].tp * 16 * FOUT;
    break;
  case 0x06:  // NP[4:0]
    PSGWork.noise.np = value & 0x1f;
    PSGWork.noise.limit = (uint32_t)PSGWork.noise.np * 2 * 16 * FOUT;
    break;
  case 0x07:  // MIXER
    PSGWork.synth[0].tone = !!(value & (1 << 0));
    PSGWork.synth[1].tone = !!(value & (1 << 1));
    PSGWork.synth[2].tone = !!(value & (1 << 2));
    PSGWork.synth[0].noise = !!(value & (1 << 3));
    PSGWork.synth[1].noise = !!(value & (1 << 4));
    PSGWork.synth[2].noise = !!(value & (1 << 5));
    break;
  case 0x08:  // M/L[3:0] for Ch.A
    PSGWork.channel[0].ml = value & 0x1f;
    PSGWork.synth[0].out = vt[1 + ((PSGWork.channel[0].ml & 0x0f) << 1)];
    break;
  case 0x09:  // M/L[3:0] for Ch.B
    PSGWork.channel[1].ml = value & 0x1f;
    PSGWork.synth[1].out = vt[1 + ((PSGWork.channel[1].ml & 0x0f) << 1)];
    break;
  case 0x0a:  // M/L[3:0] for Ch.C
    PSGWork.channel[2].ml = value & 0x1f;
    PSGWork.synth[2].out = vt[1 + ((PSGWork.channel[2].ml & 0x0f) << 1)];
    break;
  case 0x0b:  // EP[7:0]
  case 0x0c:  // EP[15:8]
  case 0x0d:  // CONT/ATT/ALT/HOLD
    // TODO: support envelope.
    break;
  case 0x0e:
  case 0x0f:
    break;
  case 0xff: // Virtual Clock
    if (value == 0)
      PSGWork.step = CLK_MSX;
    else
      PSGWork.step = CLK_4MHZ;
    break;
  default:
    return false;
  }
  return true;
}

bool PSGRead(uint8_t reg, uint8_t* value) {
  switch (reg) {
  case 0xfe:  // minor version
    *value = 1;
    break;
  case 0xff:  // major version
    *value = 1;
    break;
  default:
    return false;
  }
  return true;
}
