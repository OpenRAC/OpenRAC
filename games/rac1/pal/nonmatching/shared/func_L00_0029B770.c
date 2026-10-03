/* NON_MATCHING func_L00_0029B770 -- src/overlays/shared/update_0029B6A0.c
 * Best so far: SIZE ours 676 / retail 684, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Builds the vendor item list (item array at D_L00_001CA7C0+0xD0, stride 0x14, count at +0x210) from the owned-i
 *   Remaining difference: register allocation only. Retail copies the argument ($a0) to $15 at entry because $4 is
 *   Would need a source form that changes pseudo priority so the arg pseudo loses $4; not found in 9 runs.
 */
#include "common.h"
#include "include_asm.h"

extern int D_L00_001CA7C0[];
extern unsigned char D_0015EED0[] MACRO_ADDR;
extern unsigned char D_0013D50F[] NOT_SDA;
typedef struct { char pad[8]; unsigned short v; char pad2[14]; } Ent18;
typedef struct { int v; int pad[4]; } Ent14;
typedef struct { int a, b, c, d, e; } Item;
extern Ent14 D_L00_001C2488[];
extern Ent18 D_L00_001C43B0[];

// build the vendor's list of purchasable items from the owned-item bytes and the level's item table
void func_L00_0029B770(int skip) {
    int n;
    int i;
    int j;
    int k;
    for (n = 0; n < 12 && D_0015EED0[n] != 0xFF; n++) {
    }
    D_L00_001CA7C0[0x84] = 0;
    for (i = 0; i < n; i++) {
        int e = D_0015EED0[i];
        int idx;
        int v;
        if (e == 0xFF) continue;
        idx = e & 0x3F;
        if (e & 0x40) {
            Ent18 *e18 = &D_L00_001C43B0[idx];
            if (e18->v == 0) continue;
            v = D_L00_001C2488[idx].v;
            (D_L00_001CA7C0 + 0x34)[D_L00_001CA7C0[0x84] * 5] = idx;
            (D_L00_001CA7C0 + 0x35)[D_L00_001CA7C0[0x84] * 5] = 1;
        } else {
            if (skip) continue;
            v = D_L00_001C2488[idx].v;
            (D_L00_001CA7C0 + 0x34)[D_L00_001CA7C0[0x84] * 5] = idx;
            (D_L00_001CA7C0 + 0x35)[D_L00_001CA7C0[0x84] * 5] = 0;
        }
        (D_L00_001CA7C0 + 0x36)[D_L00_001CA7C0[0x84] * 5] = 0;
        (D_L00_001CA7C0 + 0x38)[D_L00_001CA7C0[0x84] * 5] = v;
        D_L00_001CA7C0[0x84] = D_L00_001CA7C0[0x84] + 1;
    }
    for (k = 0; k < 0x25; k++) {
        int v;
        if (D_0013D50F[0xB9 + k] == 0) continue;
        {
            Ent18 *e18 = &D_L00_001C43B0[k];
            if (e18->v == 0) continue;
        }
        i = 0;
        if (D_L00_001CA7C0[0x84] > 0) {
            for (i = 0; i < D_L00_001CA7C0[0x84] && (D_L00_001CA7C0 + 0x34)[i * 5] != k; i++) {
            }
            if (i < D_L00_001CA7C0[0x84]) continue;
        }
        v = D_L00_001C2488[k].v;
        (D_L00_001CA7C0 + 0x34)[D_L00_001CA7C0[0x84] * 5] = k;
        (D_L00_001CA7C0 + 0x35)[D_L00_001CA7C0[0x84] * 5] = 1;
        (D_L00_001CA7C0 + 0x36)[D_L00_001CA7C0[0x84] * 5] = 0;
        (D_L00_001CA7C0 + 0x38)[D_L00_001CA7C0[0x84] * 5] = v;
        D_L00_001CA7C0[0x84] = D_L00_001CA7C0[0x84] + 1;
    }
}
