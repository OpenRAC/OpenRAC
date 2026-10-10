/* NON_MATCHING func_L08_002EAB30 -- src/overlays/l08_batalia/vendor_002E0258.c
 * Best so far: SIZE ours 692 / retail 700, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Batalia moby-slot scan: walks 17 slots (data+0x60, 16 bytes each), tests each with func_L00_0025B478, scores a
 *   Best candidate p1.c (structure right, 0xFF needs an unsigned char store) is 660 vs 700 bytes: gcc cross-jumps 
 *   Retail keeps the two release bodies separate (pointer s17 in one, s19 in the other), so the two copies differ 
 */
#include "common.h"
extern void func_L00_00260108(void *, void *, int, float, float);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_002140F8(float, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float D_0015EE6C MACRO_ADDR;

/* Picks which of the 17 slots' mobys were reached, advances a counter and releases one moby near the end. */
void func_L08_002EAB30(char *moby) {
    char *data = *(char **)(moby + 0x78);
    int i, cnt = 0, idx = 0, res = -1;
    for (i = 0; i < 17; i++) {
        char **e = (char **)(data + 0x60 + i * 16);
        if (*e != 0 && func_L00_0025B478(*e, 0x10000, 0)) {
            if (i < 3) {
                if (cnt <= 0) { cnt = 1; res = 1; }
            } else if (i < 9) {
                if (cnt < 2) { cnt = 2; res = 1; }
            } else if (cnt < 4) {
                cnt += 4; idx = i; res = 2;
            }
            ((unsigned char *)*e)[0xA4] = 0xFF;
        }
    }
    *(short *)(data + 0x17C) += cnt;
    if (*(short *)(data + 0x17C) >= 12) {
        *(short *)(data + 0x17C) %= 12;
        if (idx != 0) {
            char **e = (char **)(data + 0x60 + idx * 16);
            char *m = *e;
            char *mdata = *(char **)(m + 0x78);
            func_L00_00260108(moby, m + 0x10, -1, 2.0f, 13.0f);
            func_001F9BF0(mdata, *e + 0x10, *(char **)(data + 0x64 + idx * 16) + 0x10);
            func_L00_001FF4B0(mdata, mdata, D_0015EE6C * 10.0f);
            (*e)[0x20] = 1;
            *e = 0;
            return;
        } else {
            int r = func_001FA898_r(func_002140F8(0.0f, 23.0f));
            for (i = 0; i < 8; i++) {
                int k = (i + r) % 8 + 9;
                char **e = (char **)(data + 0x60 + k * 16);
                if (*e != 0) {
                    char *m = *e;
                    char *mdata = *(char **)(m + 0x78);
                    func_L00_00260108(moby, m + 0x10, -1, 2.0f, 13.0f);
                    func_001F9BF0(mdata, *e + 0x10, *(char **)(data + 0x64 + k * 16) + 0x10);
                    func_L00_001FF4B0(mdata, mdata, D_0015EE6C * 10.0f);
                    (*e)[0x20] = 1;
                    *e = 0;
                    return;
                }
            }
            moby[0x20] = 0x63;
        }
    }
    if (res >= 0) func_0022ED80(res, 0, (int)moby);
}
