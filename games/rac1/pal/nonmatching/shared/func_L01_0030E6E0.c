/* NON_MATCHING func_L01_0030E6E0 -- src/overlays/shared/vendor_002F7700.c
 * Best so far: BYTES 116/744 (84.4% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char *D_L01_001AC180[];
extern char D_L01_00174340[];
extern float *func_L00_0025D390_t(void *) __asm__("func_L00_0025D390");
extern float func_001F9D10(void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern float func_001F9B50(float);
extern int func_L00_001EFFF0_t(void *, void *, int, void *, void *) __asm__("func_L00_001EFFF0");

/* Picks the best lock-on target for moby: a living class-5 enemy within range whose direction from `from`
 * is inside the yaw/pitch cone (or the wider near cone when close), scored by angle error and distance,
 * with a clear line of sight (or the blocker is the same kind of enemy). */
void *func_L01_0030E6E0(void *moby, void *from, void *ang, float yawMax, float pitchMax, float range, float near,
                        float nearYaw, float nearPitch) {
    float tp[4];
    char *best = 0;
    float bestScore = 1.0e9f;
    char **lp;
    char *pl = D_0013E633 + 0xE1D;
    char *t;
    for (lp = D_L01_001AC180; (t = *lp) != 0; lp++) {
        float *hp;
        int cls;
        float dist, score, p2;
        if (*(short *)(t + 0x32) == 0) continue;
        hp = func_L00_0025D390_t(t);
        if (t == 0 || *(char **)(t + 0x24) == 0) continue;
        cls = *(short *)(*(char **)(t + 0x24) + 0x46);
        if (cls != 5 || hp == 0 || !(0.0f < *hp)) continue;
        dist = func_001F9D10(from, t + 0x10);
        if (!(dist < range)) continue;
        qcopy(tp, t + 0x10);
        tp[2] += 0.4f;
        score = func_001FA850(((float *)ang)[2], func_L00_001FF860(tp[0] - ((float *)from)[0], tp[1] - ((float *)from)[1]));
        score = score * score;
        if (!(score < yawMax * yawMax)) {
            if (!(dist < near) || !(score < nearYaw * nearYaw)) continue;
        }
        p2 = func_001FA850(((float *)ang)[1], func_L00_001FF860(dist, tp[2] - ((float *)from)[2]));
        p2 = p2 * p2;
        if (!(p2 < pitchMax * pitchMax)) {
            if (!(dist < near) || !(p2 < nearPitch * nearPitch)) continue;
        }
        score *= p2;
        score *= func_001F9B50(func_001F9D10(from, tp));
        if (!(score < bestScore)) continue;
        if (func_L00_001EFFF0_t(from, tp, 0, moby, 0)) {
            char *hs = D_L01_00174340;
            char *hit = *(char **)(hs + 0x18);
            if (hit == 0 || hit == *(char **)(pl + 0x2080) || hit == 0 || *(char **)(hit + 0x24) == 0
                || *(short *)(*(char **)(hit + 0x24) + 0x46) != cls) {
                continue;
            }
        }
        bestScore = score;
        best = t;
    }
    return best;
}
