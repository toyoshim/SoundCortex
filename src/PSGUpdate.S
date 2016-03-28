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
  .syntax unified
  .cpu cortex-m0
  .align 2
  .thumb
  .thumb_func

#define rOut r0
#define rWork r1
#define rStep r2
#define rNoise r3
#define rTmp1 r4
#define rTmp2 r5
#define rTmp3 r6

#define iStep 0
#define iSynth 4

#define iSynthLimit 0
#define iSynthCount 4
#define iSynthOn 8
#define iSynthOut 12
#define iSynthTone 16
#define iSynthNoise 20
#define iSynthSize 24

#define iSynth0 iSynth
#define iSynth1 (iSynth0 + iSynthSize)
#define iSynth2 (iSynth1 + iSynthSize)

#define iNoise (iSynth2 + iSynthSize)
#define iNoiseLimit 4
#define iNoiseCount 8
#define iNoiseSeed 12

.macro UpdateTone base
  ldr  rTmp1, [rWork, #(\base + iSynthCount)]
  add  rTmp1, rTmp1, rStep
  str  rTmp1, [rWork, #(\base + iSynthCount)]
  ldr  rTmp2, [rWork, #(\base + iSynthLimit)]
  subs rTmp3, rTmp1, rTmp2
  ldr  rTmp1, [rWork, #(\base + iSynthOn)]
  bhi  1f
  str  rTmp3, [rWork, #(\base + iSynthCount)]
  mvns rTmp1, rTmp1
  str  rTmp1, [rWork, #(\base + iSynthOn)]
1:
  ldr  rTmp2, [rWork, #(\base + iSynthTone)]
  orrs rTmp1, rTmp1, rTmp2
  beq  1f
  ldr  rTmp1, [rWork, #(\base + iSynthNoise)]
  orrs rTmp1, rTmp1, rNoise
  bne  2f
1:
  ldr  rTmp1, [rWork, #(\base + iSynthOut)]
  add  rOut, rOut, rTmp1
2:
.endm

  .extern PSGWork

  .text
  .global PSGUpdate
  .type PSGUpdate, %function
PSGUpdate:
  push {r4-r6, lr}
  ldr  rWork, =#PSGWork
  ldr  rStep, [rWork, #iStep]
  movs rOut,  #0

  ldr  rTmp1, [rWork, #(iNoise + iNoiseCount)]
  add  rTmp1, rTmp1, rStep
  str  rTmp1, [rWork, #(iNoise + iNoiseCount)]
  ldr  rTmp2, [rWork, #(iNoise + iNoiseLimit)]
  subs rTmp3, rTmp1, rTmp2
  ldr  rTmp1, [rWork, #(iNoise + iNoiseSeed)]
  bhi  1f
  str  rTmp3, [rWork, #(iNoise + iNoiseCount)]
  movs rTmp2, #9
  ands rTmp2, rTmp2, rTmp1
  lsrs rTmp3, rTmp2, #3
  eors rTmp2, rTmp2, rTmp3
  lsls rTmp2, rTmp2, #15
  lsrs rTmp1, rTmp1, #1
  orrs rTmp1, rTmp1, rTmp2
  uxth rTmp1, rTmp1
  str  rTmp1, [rWork, #(iNoise + iNoiseSeed)]
1:
  movs rNoise, #1
  ands rNoise, rNoise, rTmp1

  UpdateTone iSynth0
  UpdateTone iSynth1
  UpdateTone iSynth2

  pop  {r4-r6, pc}
  .size PSGUpdate, . - PSGUpdate
