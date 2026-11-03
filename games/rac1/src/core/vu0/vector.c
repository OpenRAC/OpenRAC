#include "common.h"
#include "callees.h"
#include "structs/misc.h"

/*
 * core_text object 0x1252A0-0x125630. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony's libvu0, libvu0.o: the sceVu0 matrix and vector routines, VU0
 * macro-mode inline assembly (_sceVu0ecossin and sceVu0RotMatrixZ/X/Y
 * among them; the linker dead-stripped what the game does not call).
 * Built with Sony's 2.9-ee (Makefile.sn, EE29_CORE), like the prebuilt
 * libvu0.a.
 */

LINKER_REMNANT("asm/remnants/core_text", func_001252A0);
void sceVu0Normalize(void *dst, void *src) {
    __asm__ __volatile__("lqc2 $vf4,0x0(%1)\n\t"
                         "vmul.xyz $vf5,$vf4,$vf4\n\t"
                         "vaddy.x $vf5,$vf5,$vf5y\n\t"
                         "vaddz.x $vf5,$vf5,$vf5z\n\t"
                         ".word 0x4A0503BD\n\t"  /* vsqrt Q,$vf5x */
                         ".word 0x4A0003BF\n\t"  /* vwaitq */
                         ".word 0x4B000160\n\t"  /* vaddq.x $vf5,$vf0,Q */
                         "vnop\n\tvnop\n\t"
                         ".word 0x4A6503BC\n\t"  /* vdiv Q,$vf0w,$vf5x */
                         "vsub.xyzw $vf6,$vf0,$vf0\n\t"
                         ".word 0x4A0003BF\n\t"  /* vwaitq */
                         ".word 0x4BC0219C\n\t"  /* vmulq.xyz $vf6,$vf4,Q */
                         "sqc2 $vf6,0x0(%0)"
                         :
                         : "r"(dst), "r"(src)
                         : "memory");
}

LINKER_REMNANT("asm/remnants/core_text", func_00125300);
void sceVu0ScaleVector(float *dst, float *src, float s) {
    __asm__ __volatile__("lqc2 $vf4,0x0(%1)\n\t"
                         "mfc1 $8,%2\n\t"
                         "qmtc2 $8,$vf5\n\t"
                         "vmulx.xyzw $vf6,$vf4,$vf5x\n\t"
                         "sqc2 $vf6,0x0(%0)"
                         :
                         : "r"(dst), "r"(src), "f"(s)
                         : "$8", "memory");
}

LINKER_REMNANT("asm/remnants/core_text", func_00125340);

void sceVu0UnitMatrix(float *m) {
    __asm__ __volatile__("vsub.xyzw $vf4,$vf0,$vf0\n\t"
                         "vadd.w $vf4,$vf4,$vf0\n\t"
                         "vmr32.xyzw $vf5,$vf4\n\t"
                         "vmr32.xyzw $vf6,$vf5\n\t"
                         "vmr32.xyzw $vf7,$vf6\n\t"
                         "sqc2 $vf4,0x30(%0)\n\t"
                         "sqc2 $vf5,0x20(%0)\n\t"
                         "sqc2 $vf6,0x10(%0)\n\t"
                         "sqc2 $vf7,0x0(%0)"
                         :
                         : "r"(m)
                         : "memory");
}

ASM_FUNC("asm/handwritten/core_text", func_00125380);

ASM_FUNC("asm/handwritten/core_text", func_001253F8);

ASM_FUNC("asm/handwritten/core_text", func_001254A0);

ASM_FUNC("asm/handwritten/core_text", func_00125548);

LINKER_REMNANT("asm/remnants/core_text", func_001255F0);
