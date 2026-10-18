#pragma once
// Original synthetic register context for timing-model tests. Production ABI
// compatibility is checked separately by a fresh generated runtime replay.
#include <stdint.h>
typedef struct recomp_context {
 uint64_t r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31;
 uint32_t status_reg;
 float sf0,sf1,sf2;
 double df0,df1,df2;
} recomp_context;
