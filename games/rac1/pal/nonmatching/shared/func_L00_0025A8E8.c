/* NON_MATCHING func_L00_0025A8E8 -- src/overlays/shared/mobyutil_00258BC8.c
 * Best so far: SIZE ours 316 / retail 308, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Builds a 16-byte heading vector (from moby+0xC0 or sin/cos of moby+0x48), a stack descriptor via func_L00_0025
 *   Best: p2 (61 bytes differ of 308). Differences: saved-register assignment of the int params (retail s3..s6 = a
 *   Typing c,d as unsigned char fixes the saved regs but adds andi; char changes size. Would need an unknown wordi
 */
#include "common.h"
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_0025A8C0(char *arg, int a, int b, void *src, float scale);
extern void func_L00_001F2BE8(int, int, float, void *, void *);

/* build a stack effect descriptor from a moby's heading and hand it to the collision sphere test */
void func_L00_0025A8E8(char *m, int p1, int b, char c, char d, int e, float f, float g, float h) {
    float v[4];
    char s[0x30];
    if (*(unsigned short *)(m + 0x34) & 0x100) {
        qcopy(v, m + 0xC0);
    } else {
        v[0] = func_001F9F90(*(float *)(m + 0x48));
        v[1] = func_001F9FA8(*(float *)(m + 0x48));
        v[2] = 0;
    }
    func_001F9C30(v, v, h);
    v[2] = 1.0f;
    v[3] = 5627.9248f;
    func_L00_0025A8C0(s + 0, (int)m, b, v, g);
    s[0x18] = c;
    s[0x19] = d;
    *(unsigned short *)(s + 0x1A) = *(unsigned short *)(m + 0xA6);
    func_L00_001F2BE8(p1, e, f, m, s);
}
