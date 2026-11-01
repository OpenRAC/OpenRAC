#include "include_asm.h"
typedef int TI128 __attribute__((mode(TI)));

typedef struct {
    unsigned char pad0[4];
    unsigned char flag;
    unsigned char pad1[0x74 - 5];
    float scale;
    unsigned char pad2[0xE0 - 0x78];
} Entry262570b;

extern void func_00282D30(void *, void *, float);
extern int func_00277F48(float, int, int, int, int);
extern void func_002A9850(void *, void *, int *, int, int, int, int, int, float, float, float);
extern unsigned char D_00139568[];
extern Entry262570b D_L00_00262570[];
extern int D_L00_001C3280[];

void func_L00_0040AFB8(char *a0, void *a1, int a2)
{
    float head[4];
    float vec[4];
    float scale;
    int count;

    func_00282D30(head, a1, -3.0f);
    scale = (D_L00_00262570 + D_00139568[0x2C])->scale;
    count = func_00277F48(D_L00_00262570[D_00139568[0x1B]].flag != 0 ? 3.0f : 4.0f, (int)(a0 + 0x10), 0x10, (int)a0, 0);
    *(TI128 *)vec = *(TI128 *)(a0 + 0x10);
    func_002A9850(a0, vec, D_L00_001C3280, count, a2, 0x810000, 3, 1, scale, 1.0f, 1.0f);
}
