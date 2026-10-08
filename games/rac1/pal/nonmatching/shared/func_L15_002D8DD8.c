/* NON_MATCHING func_L15_002D8DD8 -- src/overlays/shared/vendor_002D7C00.c
 * Best so far: BYTES 23/452 (94.9% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Mover-init: when D_L15_00161C78 is set, clears it, seeds vector tables (19 slots, stride 16/4) from moby data 
 *   Best: p9.c, 29 words differ of 113, size exact. Left: short-loop counter lands in $s1 not $v1 (retail keeps it
 *   Idioms found: gp ints declared `extern char X;` and read as *(int*)&X; non-gp shorts/D60 as `extern T X[n] MAC
 *   q30 w07: p13.c best (BYTES 23/452): second loop gets its own counter j (fixes the s1 counter). Left: retail ho
 */
#include "common.h"
extern char D_L15_00161C78;
extern char D_L15_00161C7C;
extern char D_L15_00161C94;
extern char D_L15_00161C9C;
extern char D_L15_00161CB0;
extern char D_L15_00161CE0;
extern char D_L15_00161D0C;
extern char D_L15_00161D64;
extern int D_L15_00161D60[2] MACRO_ADDR;
extern char D_L15_001D3A20[];
extern char D_L15_001D3A10[];
extern char D_L15_001D3CA0[];
extern char D_L15_001D3CF0[];
extern char D_L15_001D3E30[];
extern short D_L15_00161D46[];
extern short D_L15_00161D50[4] MACRO_ADDR;
extern short D_L15_00161D52[4] MACRO_ADDR;
extern short D_L15_00161D54[4] MACRO_ADDR;
extern short D_L15_00161D56[4] MACRO_ADDR;
extern short D_L15_00161D58[4] MACRO_ADDR;
extern short D_L15_00161D5A[4] MACRO_ADDR;
extern short D_L15_00161D5C[4] MACRO_ADDR;
extern short D_L15_00161D5E[4] MACRO_ADDR;
extern int func_001F9850(int);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BC0(void *);

/* One-shot initialisation of the level's vector tables when triggered. */
void func_L15_002D8DD8(char *moby) {
    char *data = *(char **)(moby + 0x78);
    if (*(int *)&D_L15_00161C78) {
        float v[4];
        float w[4];
        int i;
        int j;
        char *src = data + 0xF0;
        *(int *)&D_L15_00161C78 = 0;
        *(int *)&D_L15_00161D64 = 0;
        D_L15_00161D60[0] = func_001F9850(*(int *)&D_L15_00161D0C);
        *(int *)&D_L15_00161CE0 = 0x7F2020;
        func_001F9BF0(v, data + 0x100, src);
        func_L00_001FF4B0(w, v, *(float *)&D_L15_00161C7C / 20.0f);
        qcopy(D_L15_001D3A20, src);
        for (i = 1; i < 20; i++) {
            func_001F9BD8(D_L15_001D3A20 + i * 0x10, D_L15_001D3A10 + i * 0x10, w);
            *(int *)(D_L15_001D3CA0 + i * 4) = 0;
            func_001F9BC0(D_L15_001D3CF0 + i * 0x10);
            *(int *)(D_L15_001D3E30 + i * 4) = 0;
        }
        for (j = 3; j >= 0; j--)
            D_L15_00161D46[j - 3] = -1;
        D_L15_00161D5C[0] = 4;
        D_L15_00161D5A[0] = 2;
        D_L15_00161D52[0] = 4;
        D_L15_00161D58[0] = 4;
        D_L15_00161D50[0] = 8;
        D_L15_00161D5E[0] = 2;
        D_L15_00161D56[0] = 4;
        D_L15_00161D54[0] = 8;
        *(int *)&D_L15_00161C94 = 0;
        *(int *)&D_L15_00161C9C = 0;
        *(int *)&D_L15_00161CB0 = 0;
    }
}
