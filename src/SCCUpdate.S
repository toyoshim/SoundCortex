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
#define rMask r3
#define rTableOffset r4
#define rTmp1 r5
#define rTmp2 r6
#define rTmp3 r7

#define iStep 0
#define iSynth 4

#define iSynthLimit 0
#define iSynthCount 4
#define iSynthOffset 8
#define iSynthVol 12
#define iSynthTone 16
#define iSynthWaveTable 20
#define iSynthSize 52

.macro UpdateTone
  ldr  rTmp1, [rWork, #(iSynth + iSynthCount)]
  add  rTmp1, rTmp1, rStep
  str  rTmp1, [rWork, #(iSynth + iSynthCount)]
  ldr  rTmp2, [rWork, #(iSynth + iSynthLimit)]
  subs rTmp3, rTmp1, rTmp2
  ldr  rTmp1, [rWork, #(iSynth + iSynthOffset)]
  bhi  1f
  str  rTmp3, [rWork, #(iSynth + iSynthCount)]
  adds rTmp1, rTmp1, #1
  ands rTmp1, rTmp1, rMask
  str  rTmp1, [rWork, #(iSynth + iSynthOffset)]
1:
  ldr  rTmp2, [rWork, #(iSynth + iSynthTone)]
  mvns rTmp2, rTmp2
  beq  1f
  add  rTmp1, rTmp1, rWork
  ldrsb rTmp1, [rTmp1, rTableOffset]
  ldr  rTmp2, [rWork, #(iSynth + iSynthVol)]
  muls rTmp1, rTmp1, rTmp2
  add  rOut, rOut, rTmp1
1:
.endm

  .extern SCCWork

  .text
  .global SCCUpdate
  .type SCCUpdate, %function
SCCUpdate:
  push {r4-r7, lr}
  ldr  rWork, =#SCCWork
  ldr  rStep, [rWork, #iStep]
  movs rOut,  #0
  movs rMask, #0x1f
  movs rTableOffset, #(iSynth + iSynthWaveTable)

  UpdateTone
  adds rWork, rWork, #iSynthSize
  UpdateTone
  adds rWork, rWork, #iSynthSize
  UpdateTone
  adds rWork, rWork, #iSynthSize
  UpdateTone
  adds rWork, rWork, #iSynthSize
  UpdateTone

  asrs rOut, rOut, #4
  pop  {r4-r7, pc}
  .size SCCUpdate, . - SCCUpdate
