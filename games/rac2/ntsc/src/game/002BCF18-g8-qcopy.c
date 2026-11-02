/* func_002BCF18 in its own unit: small data at -G8 and lq/sq quadword copies (-frac2-qcopy -frac2-qcopy-barrier). */
#include "include_asm.h"

typedef int TI128 __attribute__((mode(TI)));

extern float D_001B168C;
/* Read through lui/lw with a separate address register: not small data, so an
   incomplete array indexed [0]. */
extern unsigned char *D_001F2580[];
extern int func_002A77E0(int);
extern char *func_002B9030(int);
extern int func_00283CF0(float);
extern char func_001163B0(void);

char *func_002BCF18(char *a0, char *a1, int a2, int a3, int t0, int t1, float f12)
{
    char *p;

    if (t0 == 0)
        return 0;
    if (D_001B168C > 0.9f && func_002A77E0(3) == 0)
        return 0;
    if (D_001B168C > 0.95f && func_002A77E0(2) == 0)
        return 0;
    if (D_001B168C > 1.0f && func_002A77E0(1) == 0)
        return 0;
    p = func_002B9030(0x10);
    if (p != 0) {
        char *q;
        unsigned char *tab;
        int t;

        *(TI128 *)(p + 0x10) = *(TI128 *)a0;
        *(int *)(p + 4) = a2;
        t = func_00283CF0(4.0f) + 0x40;
        p[1] = 0;
        *(float *)(p + 0xC) = f12;
        p[3] = 0x44;
        p[9] = t;
        p[8] = func_001163B0();
        q = p + 0x20;
        tab = D_001F2580[0];
        p[2] = *((t1 != 3) ? tab + t1 : tab);
        *(short *)(p + 0xA) = t0;
        *(short *)(q + 0x18) = t0;
        *(float *)(q + 0x1C) = f12;
        *(int *)(q + 0x10) = a2;
        *(int *)(q + 0x14) = a3;
        *(short *)(q + 0x1A) = t1;
        *(TI128 *)q = *(TI128 *)a1;
        *(float *)(q + 0xC) = *(float *)(a0 + 8) - 0.5f;
    }
    return p;
}
