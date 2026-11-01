/* GPT-6.1 Sol high, 28 October 2026; USA v1.01: 360/360 bytes, 0/27 places.
 * Profile: -O2 -G0 -ffunction-sections -frac2-zero-ti-por
 *          -fno-strength-reduce -fno-gcse. Thirteen check.py runs.
 * A named record stride, a separate literal float-row stride, explicit next index,
 * local scaled-vector pointer and incomplete sda array fix the former saved-register,
 * duplicate-multiply and zero/load scheduling differences. Only +0x94/+0x98 differ:
 * retail multiplies into v1 and forms base+40 in v0; this draft swaps those temps.
 * Three scoped-pointer/offset wordings reach the same two-instruction tie.
 * Turning off rerun CSE or CSE jump following did not recover the independent stride;
 * widening the index multiply emitted unwanted full-width multiplication machinery.
 * Reopen with a justified inner-row address lifetime or type model. Required declarations
 * are included below. This is C-reachable code, not handwritten evidence.
 */
#include "include_asm.h"
typedef int TI128 __attribute__((mode(TI)));
extern char *func_L00_0032A088(int, int);
extern int func_00282978(void *);
extern float func_00283B98(float, float);
extern void func_002AFFD8(void *, void *, int);
extern void func_00282D30(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern int D_L00_001A8FF0[] __attribute__((sda));

int func_L00_003A87D8(char *object)
{
    char local[32];
    char *base = func_L00_0032A088(1483, *(short *)(*(char **)(object + 104) + 26));
    char *entry;
    int i, j, k;
    int stride = 112;
    int active = 0;
    for (i = 0; i < 80; i = j) {
        entry = base + i * stride;
        j = i + 1;
        if (*(short *)(entry + 18) != 0) {
            if (func_00282978(entry + 16) != 0) {
                *(short *)(entry + 18) = 0;
            } else {
                char *scaled;
                float *p;
                ++active;
                *(float *)(entry + 20) = func_00283B98(*(float *)(entry + 20), *(float *)(entry + 24));
                p = (float *)(base + 40);
                p += i * 28;
                scaled = local + 16;
                for (k = 8; k >= 0; --k, ++p)
                    *p += p[9];
                *(TI128 *)local = 0;
                *(float *)(local + 8) = 1.0f;
                *(float *)(local + 12) = 1.0f;
                if (D_L00_001A8FF0[0] != 0)
                    func_002AFFD8(base + i * stride, local, 1);
                entry = base + i * stride;
                func_00282D30(scaled, local, *(float *)(entry + 36));
                func_00282CC0(entry, entry, scaled);
            }
        }
    }
    return active;
}
