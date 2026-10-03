/* NON_MATCHING func_L11_002D3970 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: SIZE ours 548 / retail 552, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Turret aim update for level 11: if state is in the table set, copy player pos, compute yaw/pitch with clamps, 
 *   Best p2.c (544 vs 552; p3.c is 548 but diverges earlier). Left: order of vec/tmp quad copies (retail hoists ad
 */
typedef int u128 __attribute__((mode(TI)));
extern void func_001F9BF0(void *, void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001FA790(float, float);
extern float func_001F9CE8(void *);
extern void func_L00_00263950(char *, char *, int, float, float);
extern unsigned char D_0015EEB0[] MACRO_ADDR;
extern float D_0015EE64 MACRO_ADDR;
extern char D_0013E633[];

// Updates the heli-pack style aim: steers the moby's turret angles toward the player.
void func_L11_002D3970(void *mm) {
    char *m = mm;
    float vec[4];
    float tmp[4];
    float v2[4];
    char *d = *(char **)(m + 0x78);
    int ok;
    float *tp = tmp;
    float a, b, c;
    float k1 = 0.02f;
    float k2 = 0.3f;
    switch ((unsigned char)m[0x20]) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 11:
    case 12:
    case 14:
    case 16:
        *(u128 *)vec = *(u128 *)(D_0013E633 + 0xE9D);
        ok = 1;
        break;
    default:
        ok = 0;
        break;
    }
    if (ok) {
        *(u128 *)tp = *(u128 *)(m + 0x10);
        tp[2] = tp[2] + 1.0f;
        func_001F9BF0(v2, vec, tp);
        a = func_001FA790(func_L00_001FF860(v2[0], v2[1]), *(float *)(m + 0x48));
        b = func_L00_001FF860(func_001F9CE8(v2), v2[2]);
        c = -b;
        if (a > 1.2201f) {
            a = 1.2201f;
        } else if (a < -1.2201f) {
            a = -1.2201f;
        }
        if (c > 0.2619f) {
            c = 0.2619f;
        } else if (c < -0.5236f) {
            c = -0.5236f;
        }
        *(float *)(d + 0x1C4) = c;
        *(float *)(d + 0x1C8) = a * 0.7f;
        *(float *)(d + 0x248) = a * 0.3f;
    }
    if (D_0015EEB0[0] != 0) {
        *(float *)(d + 0x1D0) = 2.75f;
    }
    func_L00_00263950(m, d + 0x160, 0, k1 * D_0015EE64, k2 * D_0015EE64);
    func_L00_00263950(m, d + 0x1E0, 1, k1 * D_0015EE64, k2 * D_0015EE64);
}
