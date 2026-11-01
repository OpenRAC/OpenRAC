#include "common.h"
typedef int TI128 __attribute__((mode(TI)));

extern void func_L00_002D7940(char *p);
extern void func_L00_002D7958(char *dst, char *src, f32 x);
extern void func_L00_00427C78(char *p);

extern char D_00189EA0[];

/* Installs two aligned control vectors, raises the first Z component and
 * copies it into the saved state before clearing and refreshing the blend. */
void func_L00_004284B0(char *a0)
{
    char *s1;
    char *s0;
    char *s2 = a0;
    {
        char *initial = *(char **)(a0 + 0x70);
        char *block = initial + 0xB0;
        qcopy(block, D_00189EA0);
        *(f32 *)(block + 8) += 2.0f;
        qcopy(initial + 0xC0, D_00189EA0 + 0x10);
        s1 = *(char **)(a0 + 0x70);
        qcopy(s1 + 0x80, block);
    }
    s0 = s1 + 0x14;
    func_L00_002D7940(s0);
    *(f32 *)(s1 + 0x1C) = 4.64f;
    func_L00_002D7958(s2 + 0x50, s0, 0.0f);
    func_L00_00427C78(s2);
}
