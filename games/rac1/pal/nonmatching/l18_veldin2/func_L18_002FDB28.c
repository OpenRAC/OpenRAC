/* NON_MATCHING func_L18_002FDB28 -- src/overlays/l18_veldin2/vendor_002F9D48.c
 * Best so far: SIZE ours 368 / retail 372, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Looks up a list of mob ids (D_L18_001AC540[i], 15-bit ids, bit 15 ends the list) for a moby of class 0x772 in 
 *   Best p3.c (31 diff words, size 368 vs 372) / p8.c (goto-style loop, allocation closer to retail). Everything e
 *   Missing one insn: retail copies the lhu result into another register (`daddu $a3,$v0,$0`) before `andi`, uses 
 */
#include "common.h"
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_L00_0025BC48(float *a, float *b, float *out, float speed, float g);
extern float func_L00_00258C80(float lo, float hi);
extern unsigned short *D_L18_001AC540[];
extern char *D_L18_00160058 MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;

/* find the moby in list i in state 8, reset it toward p and launch it */
int func_L18_002FDB28(int i, float *p, float *q, float t) {
    unsigned short *list = D_L18_001AC540[i];
    if (list != 0) {
        char *tbl = D_L18_00160058;
        short v;
        unsigned short id;
        char *m;
    again:
        {
            id = *list;
            v = (short)id;
            m = tbl + ((id & 0x7FFF) << 8);
            if (*(short *)(m + 0xA6) == 0x772 && (unsigned char)m[0x20] == 8) {
                char *data;
                int val;
                float *vec;
                *(unsigned short *)(m + 0x34) &= 0xFFBE;
                val = *(int *)(*(char **)(m + 0x24) + 0x10);
                *(unsigned short *)(m + 0x34) |= 0x1000;
                data = *(char **)(m + 0x78);
                *(int *)(m + 0x94) = val;
                qcopy(m + 0x10, p);
                m[0x20] = 1;
                *(float *)(data + 0x1F4) = 1.0f;
                vec = (float *)(data + 0x1D0);
                func_001F9BF0(vec, q, p);
                *(int *)(data + 0x1D8) = 0;
                func_L00_001FF4B0(vec, vec, t);
                *(float *)(data + 0x1D8) = func_L00_0025BC48(p, q, 0, t, -(D_0015EE70 * 10.0f));
                *(float *)(data + 0x1EC) = func_L00_00258C80(15.0f, 45.0f) * 0.017453292f;
                return 1;
            }
            list++;
        }
        if (v >= 0) goto again;
    }
    return 0;
}
