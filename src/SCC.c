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
#include "SCC.h"

// Constant variables to improve readability.
enum {
  CLK_MSX =  3579545UL,
  CLK_4MHZ = 4000000UL,
  FOUT = 46875UL,  // Sampling rate
};

typedef struct {
  uint32_t tp;
  uint32_t ml;
} Channel;

typedef struct {
  uint32_t limit;
  uint32_t count;
  uint32_t offset;
  uint32_t vol;
  uint32_t tone;
  uint8_t wt[32];
} Synth;

struct {
  uint32_t step;
  Synth synth[5];

  Channel channel[5];
} SCCWork;

void SCCInit() {
  SCCWork.step = CLK_MSX;
  for (int i = 0; i < 5; ++i) {
    SCCWork.synth[i].limit = 0;
    SCCWork.synth[i].count = 0;
    SCCWork.synth[i].offset = 0;
    SCCWork.synth[i].vol = 0;
    SCCWork.synth[i].tone = 1;
    for (int j = 0; j < 32; ++j)
      SCCWork.synth[i].wt[j] = 0;
  }
}

void SCCWrite(uint8_t reg, uint8_t value) {
  // Register map is compatible with SCC+.
  if (reg <= 0x9f) {
    int ch = reg >> 5;
    int offset = reg & 0x1f;
    SCCWork.synth[ch].wt[offset] = value;
  } else if (reg <= 0xa9) {
    int ch = (reg - 0xa0) >> 1;
    if (reg & 0)
      SCCWork.channel[ch].tp = (SCCWork.channel[ch].tp & 0x00ff) | ((uint16_t)(value & 0x0f) << 8);
    else
      SCCWork.channel[ch].tp = (SCCWork.channel[ch].tp & 0x0f00) | value;
    SCCWork.synth[ch].limit = (uint32_t)SCCWork.channel[ch].tp * FOUT;
  } else if (reg <= 0xae) {
    int ch = reg - 0xaa;
    SCCWork.channel[ch].ml = value & 0x0f;
    SCCWork.synth[ch].vol = SCCWork.channel[ch].ml;
  } else if (reg == 0xaf) {
    SCCWork.synth[0].tone = !(value & (1 << 0));
    SCCWork.synth[1].tone = !(value & (1 << 1));
    SCCWork.synth[2].tone = !(value & (1 << 2));
    SCCWork.synth[3].tone = !(value & (1 << 3));
    SCCWork.synth[4].tone = !(value & (1 << 4));
  }
  // TODO: mode register.
}
