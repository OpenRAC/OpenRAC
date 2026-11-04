/* Tier: sol-high (v1022), USA v1.01, -O2 -G8 -ffunction-sections.
 * Best check.py: 1184/1184 bytes, 0/27 places; first mismatch +0x70.
 * Six instructions differ: the first timer address uses v0 instead of v1
 * (+0x68/+0x6C/+0x70), and reset-chain destination setup differs at
 * +0x160/+0x168/+0x16C (address temporary and output-pointer scheduling).
 * All other instructions agree apart from relocation addends. Full symbol
 * binding and linked validation remain pending because this does not match.
 * Incomplete sda timer arrays prevent high-half hoisting and recover the
 * reset branch's 160-byte constant delay slot. Two-dimensional reset indexing
 * removes mtlo/madd. Forward indices recover all jitter-loop setup order;
 * the previous chain uses flat indexing to preserve addition operand order.
 * Pointer-local and struct-chain forms tied this source byte for byte.
 * -fno-gcse grew an earlier 1192-byte draft to 1228; -mno-split-addresses
 * changed broad allocation; -g was unchanged. Eighteen checker invocations
 * (one parse error) plus two diagnostic compiles; stopped on the final tie.
 * Next: compare a matched caller's timer-base and reset-address lifetimes;
 * no barriers, pinned registers or volatile accesses were used.
 * Required declarations are retained below; compile as a -G8 unit.
 */
#include "common.h"

typedef float Vector[4] __attribute__((aligned(16)));
extern int func_00282978(void *);
extern float func_002A7878(float, float);
extern float func_002A78D8(float, float);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern void func_L00_0031AA28(void *, void *, void *, void *, void *, float);
extern short D_L00_001B2D40[] __attribute__((sda));
extern short D_L00_001B2D48[] __attribute__((sda));
extern short D_L00_001B2D50[] __attribute__((sda));
extern float D_0018BE30[4];
extern float D_L00_001B100C __attribute__((sda));
extern float D_L00_001B1004 __attribute__((sda));
extern Vector D_L00_002563A0[][10];
extern float D_L00_00256390[][4];

void func_L00_0040B398(char *object)
{
    Vector p, delta, out, diffs[10];
    char *state = *(char **)(object + 0x68);
    int g, i, j, k, l, n;
    float (*base)[4] = (float (*)[4])D_L00_002563A0;

    for (g = 0; g < 4; g++) {
        if (func_00282978(&D_L00_001B2D40[g])) {
            D_L00_001B2D48[g] = 0;
            D_L00_001B2D40[g] = D_L00_001B2D50[g];
            qcopy(p, state + 0x30);
            p[0] += func_002A7878(-0.5f, 0.5f);
            p[1] += func_002A7878(-0.5f, 0.5f);
            p[2] += func_002A7878(-0.5f, 0.5f);
            func_00282CF0(delta, p, D_0018BE30);
            func_00282F20(delta, delta, D_L00_001B100C);
            func_00282CC0(delta, p, delta);
            for (i = 0; i < 10; i++) {
                float t = i * 0.1f;
                if (i == 9)
                    t = 1.0f;
                func_L00_0031AA28(out, D_0018BE30, p, state + 0x40, delta, t);
                qcopy(D_L00_002563A0[g][i], out);
            }
        } else {
            for (j = 1; j < 10; j++) {
                func_00282CF0(diffs[j], ((Vector (*)[10])base)[g][j], D_L00_00256390[g * 10 + j]);
                diffs[j][0] += func_002A78D8(0.0f, 0.05f);
                diffs[j][1] += func_002A78D8(0.0f, 0.05f);
                diffs[j][2] += func_002A78D8(0.0f, 0.05f);
            }
            qcopy(base[g * 10], D_0018BE30);
            for (k = 1; k < 10; k++)
                func_00282CC0(((Vector (*)[10])base)[g][k], D_L00_00256390[g * 10 + k], diffs[k]);
        }
        for (l = 2; l < 9; l++) {
            qcopy(p, base[l]);
            p[0] += func_002A78D8(0.0f, D_L00_001B1004);
            p[1] += func_002A78D8(0.0f, D_L00_001B1004);
            p[2] += func_002A78D8(0.0f, D_L00_001B1004);
            qcopy(base[l + 10], p);
        }
        for (n = 2; n < 9; n++) {
            qcopy(p, base[n]);
            p[0] += func_002A78D8(0.0f, D_L00_001B1004);
            p[1] += func_002A78D8(0.0f, D_L00_001B1004);
            p[2] += func_002A78D8(0.0f, D_L00_001B1004);
            qcopy(base[n + 20], p);
        }
        for (i = 2; i < 9; i++) {
            qcopy(p, base[i]);
            p[0] += func_002A78D8(0.0f, D_L00_001B1004);
            p[1] += func_002A78D8(0.0f, D_L00_001B1004);
            p[2] += func_002A78D8(0.0f, D_L00_001B1004);
            qcopy(base[i + 30], p);
        }
    }
}
