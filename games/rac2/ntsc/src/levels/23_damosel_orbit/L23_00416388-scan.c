#include "include_asm.h"
#include "common.h"

/* Small-data unit for func_L23_00419970: the five-slot table at $gp+0x2668
 * (20 bytes, hence the assembler's -G32) and the current slot right after it,
 * bounded by the count at 0x001B38C0, read through the absolute macro form. */

extern int D_L23_001B166C __attribute__((sda));
extern int D_L23_001B1658 __attribute__((sda));
extern int D_L23_001B38C0 MACRO_ADDR;
extern int *D_L23_001FF300[];
extern void func_0032A1C8(void *, void *, int);
extern void func_002F3D60(void *, void *, float);
extern void func_002F3AB0(void *, void *, void *);
extern void func_002F3BB0(void *, void *, void *);
extern void func_002F3AF0(void *, void *, float);
extern void func_002F3A80(void *, void *, void *);
extern int func_L23_00418BC8(void *, void *, void *);

typedef struct ScanSub_00419970 {
    char pad[0x230];
    int record_ids[0];
} ScanSub_00419970;

/* Reading the slot through an accessor matters: its inlined block stays at
 * the top of the rotated while loop, so cse1 does not merge the loop's table
 * base with the pre-test's and retail's separate loop copy is kept. */
static inline int SlotAt(int i) {
    return (&D_L23_001B1658)[i];
}

/* Advances the current slot round-robin to the next one not yet spent (-1),
 * giving up after a full turn. Two point pairs are built from the first and
 * last points of the slot's record, and the slot is marked spent once
 * func_L23_00418BC8 accepts both pairs. */
void func_L23_00419970(char *obj) {
    float a[4], b[4], c[4], d[4], e[4], f[4], g[4], h[4], i[4], j[4];
    char *s;
    int old = D_L23_001B166C;
    char *rec;
    s = *(char **)(obj + 0x68);
    while (SlotAt(D_L23_001B166C = (D_L23_001B166C + 1) % D_L23_001B38C0) == -1) {
        if (old == D_L23_001B166C) return;
    }
    rec = (char *)D_L23_001FF300[((ScanSub_00419970 *)s)->record_ids[(&D_L23_001B1658)[D_L23_001B166C]]];
    qcopy_nc(a, rec + 16);
    qcopy_nc(c, rec + *(int *)rec * 16);
    qcopy_nc(b, rec + 16);
    qcopy_nc(d, rec + *(int *)rec * 16);
    func_0032A1C8(a, h, 1);
    func_0032A1C8(c, i, 1);
    func_002F3D60(h, h, 1.0f);
    func_002F3D60(i, i, 1.0f);
    func_002F3AB0(g, c, a);
    func_002F3BB0(e, g, h);
    func_002F3D60(e, e, -1.5f);
    func_002F3D60(f, e, -1.5f);
    func_002F3AF0(j, h, *(float *)(s + 0x54) + *(float *)(s + 0x50));
    func_002F3A80(j, j, f);
    func_002F3A80(a, a, j);
    func_002F3AF0(j, i, *(float *)(s + 0x54) + *(float *)(s + 0x50));
    func_002F3A80(j, j, f);
    func_002F3A80(c, c, j);
    func_002F3AF0(j, h, *(float *)(s + 0x54) + *(float *)(s + 0x50));
    func_002F3A80(j, j, e);
    func_002F3A80(b, b, j);
    func_002F3AF0(j, i, *(float *)(s + 0x54) + *(float *)(s + 0x50));
    func_002F3A80(j, j, e);
    func_002F3A80(d, d, j);
    if (func_L23_00418BC8(obj, a, c) && func_L23_00418BC8(obj, b, d))
        (&D_L23_001B1658)[D_L23_001B166C] = -1;
}
