/* NON_MATCHING func_L01_00303A20 -- src/overlays/shared/vendor_002F7700.c
 * Best so far: BYTES 8/416 (98.1% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L01_00303A20 (416 B)
 *   Best: d1.c BYTES 8/416. Only `li $s6,-1` / `daddu $s0,$s1` (e = d) swap places in the loop preheader.
 *   Levers: do-while with `(int)e < (int)(d + 20)` (retail's signed slt on pointers; an index loop keeps i
 *   because its scc compare blocks biv elimination), `goto fail` from the level check (retail threads the
 *   jump straight to the zero store), t+0x10 in a local, `t->18 > e->lo` operand order.
 *   Tried: all orders of `+= 1` / tp / e = d (r*), e init at declaration.
 */
#include "common.h"

extern char D_0013E633[];
extern char *D_L01_00160064 MACRO_ADDR;
extern char *D_L01_001B0C30[];
extern short D_L01_0015F40C;
extern void func_L01_00303810(char *);
extern int func_L00_0025A778(void *, void *, int);
extern int func_00215570(void *, int);
extern void func_001F49B0(void (*)(char *), char *);
extern int func_001F9850(int);
extern void func_L00_002B6E10(void *, void *);

typedef struct { int zone; float lo; float hi; int vol; } Zone_303a20;

/* Counts how long the type-0xAC moby has stood clear of the 20 zones; after 90 ticks hands it on. */
void func_L01_00303A20(char *m) {
    Zone_303a20 *d = *(Zone_303a20 **)(m + 0x78);
    char *t = 0;
    Zone_303a20 *e;
    char *tp;
    ((unsigned char *)m)[0x30] = 0xFF;
    if (*(int *)(D_0013E633 + 0x2EA1) != 0x1D) goto fail;
    {
        char *o;
        for (o = D_L01_00160064; o != 0; o = *(char **)(o + 0x28)) {
            if (*(short *)(o + 0xA6) == 0xAC) {
                t = o;
                break;
            }
        }
    }
    if (t == 0 || *(short *)(t + 0xA6) != 0xAC) {
    fail:
        *(int *)((char *)d + 0x140) = 0;
        return;
    }
    *(int *)((char *)d + 0x140) += 1;
    tp = t + 0x10;
    e = d;
    do {
        if (e->zone != -1 && *(float *)(t + 0x18) > e->lo && *(float *)(t + 0x18) < e->hi) {
            char *z = D_L01_001B0C30[e->zone];
            if (func_L00_0025A778(tp, z + 0x10, *(int *)z)) {
                *(int *)((char *)d + 0x140) = 0;
                break;
            }
        }
        if (func_00215570(tp, e->vol)) {
            *(int *)((char *)d + 0x140) = 0;
            break;
        }
        e++;
    } while ((int)e < (int)(d + 20));
    if (*(int *)((char *)d + 0x140) > 0) {
        if (*(int *)&D_L01_0015F40C != 0) {
            func_001F49B0(func_L01_00303810, m);
        }
        if (func_001F9850(0x5A) < *(int *)((char *)d + 0x140)) {
            func_L00_002B6E10(t, *(void **)(t + 0x78));
        }
    }
}
