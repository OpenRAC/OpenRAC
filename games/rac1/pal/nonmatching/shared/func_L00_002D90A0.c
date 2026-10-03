/* NON_MATCHING func_L00_002D90A0 -- src/overlays/shared/vendor_002D1168.c
 * Best so far: SIZE ours 352 / retail 356, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Initialises slot i of a set of per-particle arrays (3 random floats, 2 ranges from the moby's data, zeros, sho
 *   Best is p9.c (88/356 bytes differ): size and front half match; the tail differs in allocation only (ours puts 
 *   Globals 161448/161444 declared as arrays (x[0]) to get explicit lui+load. Store-order permutations of 5A40/63A
 */
#include "common.h"
extern float func_L00_00258C80(float lo, float hi);
extern float func_002140F8(float, float);
extern float D_L00_001CBAE0[][4];
extern float D_L00_001D05E0[];
extern float D_L00_001D18A0[];
extern float D_L00_001D34C0[];
extern float D_L00_001D4780[];
extern short D_L00_001D5A40[];
extern short D_L00_001D63A0[];
extern char D_L00_001D3010[];
extern short D_L00_001D6D00[];
extern unsigned short D_L00_00161448 MACRO_ADDR;
extern int D_L00_00161444 MACRO_ADDR;

/* initialises particle slot i of a moby's effect arrays */
void func_L00_002D90A0(char *moby, int i) {
    float *d = *(float **)(moby + 0x78);
    char *c = (char *)d;
    float a, *p;
    a = func_L00_00258C80(0.0f, 1.0f);
    p = D_L00_001CBAE0[i];
    p[0] = a;
    p[1] = func_L00_00258C80(0.0f, 1.0f);
    p[2] = func_L00_00258C80(0.0f, 1.0f);
    D_L00_001D05E0[i] = func_002140F8(d[4], d[5]);
    D_L00_001D18A0[i] = func_002140F8(d[6], d[7]);
    D_L00_001D34C0[i] = 0.0f;
    D_L00_001D4780[i] = func_002140F8(d[2], d[3]);
    D_L00_001D5A40[i] = D_L00_00161448;
    D_L00_001D63A0[i] = 0;
    D_L00_001D3010[i] = 0;
    D_L00_001D6D00[i] = (i - *(short *)c + 1) * D_L00_00161444;
}
