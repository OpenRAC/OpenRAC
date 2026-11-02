#include "include_asm.h"

/* 28 October 2026 current-master candidate: 120 bytes vs 124, 0/28 USA v1.01.
   The fifth-argument register-copy shape now matches with a dedicated 64-bit prototype.
   The remaining difference is the shared address/load ordering for the nearby short and
   float data. Earlier 116-byte measurement follows as history.

   A temporary explicit-statement-order probe kept the initial call first, then loaded the
   short, called func_00283CE0, and loaded the float for the calculation. It remained 120/124:
   the scheduler put the short load in the helper-call delay slot, unlike retail's empty slot,
   and the address materializations/registers still differed. Isolated-unit
   probes with `-fno-schedule-insns` and, separately, `-fno-schedule-insns2`
   also remain 120/124 and 0/28. Neither moves the short load out of the call
   delay slot; the second flag also changes stack-save and float-constant order.
   No strict match.

   The retail body:

     addiu $sp,$sp,-0x10 / addiu $a1,$zero,0x44 / sd $ra / jal func_002FC560 /
     addiu $a0,$zero,0x42 / lui $a0,0x1B / lh $a0,0x2312($a0) / jal func_00283CE0 /
     nop / lui $at,0x1B / lwc1 $f1,0x2314($at) / lui $at,0x3F80 / mtc1 $at,$f12 /
     mul.s $f0,$f0,$f1 / lui $at,0x437F / mtc1 $at,$f2 / sub.s $f12,$f12,$f0 /
     jal func_00283CF0 / mul.s $f12,$f12,$f2 / lui $v1,0x1A / sll $v0,$v0,24 /
     addiu $v1,$v1,0x6480 / daddu $t0,$v0,$zero / lh $a3,0x150($v1) / daddu $a0,$zero /
     lh $a1,0x152($v1) / jal func_0027DE50 / daddu $a2,$zero / ld $ra / jr $ra

   Current blocker: the compiler shares a high-half address materialization for the
   adjacent short and float data, and schedules the short load in the call delay slot.
   Retail emits separate address materializations and leaves that delay slot empty,
   loading the float before the 1.0f constant. The 64-bit fifth-argument prototype now
   reproduces the retail `sll` plus `daddu` sequence. Keep this function in its own source
   unit so its callee prototype can differ from func_002FFFE0.

   The current 120-byte candidate was checked with a 64-bit fifth argument; it still
   differs at the short/float address sequence and is not a match.

   The two data cells sit at a different address in the boot (0x1B2312 and 0x1B2314) and
   in the levels (0x1B2672 and 0x1B2674), so they are named without an address. */

extern short ref_L00_002FFCF8_00;

extern float ref_L00_002FFCF8_01;

extern float func_00283CE0(int);

extern int func_00283CF0(float);

extern short D_001A6480[];

extern void func_002FC560(int, long long);

extern void func_0027DE50(int, int, int, int, long long);

void func_002FFCF8(void)
{
    int v;
    func_002FC560(0x42, 0x44);
    v = func_00283CF0((1.0f - func_00283CE0(ref_L00_002FFCF8_00) * ref_L00_002FFCF8_01) * 255.0f);
    func_0027DE50(0, D_001A6480[0xA9], 0, D_001A6480[0xA8], (long long)(v << 24));
}
