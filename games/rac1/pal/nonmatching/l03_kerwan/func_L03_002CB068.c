/* NON_MATCHING func_L03_002CB068 -- src/overlays/l03_kerwan/vendor_00293720.c
 * Best so far: SIZE ours 532 / retail 536, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Picks best point index from circular list by heading/distance (like func_L08_002DBD88). Best p3.c: size 532 vs
 */
extern void func_L00_00260D30(char *, float *, float);
extern void func_L00_0025EFC0(void *, void *, void *, void *, void *, int, float, float, float);
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern float func_001F9D10(void *, void *);
typedef int u128_2CB068 __attribute__((mode(TI)));

/* Walks the circular list of points and keeps the one best placed by heading and distance. */
void func_L03_002CB068(char *moby) {
    float v0[20], v50[4], v60[4], dist;
    float *p60;
    int cur;
    char *d = *(char **)(moby + 0x78);
    char *q = d + 0x120;
    int best = 0;
    float bestA = 0.0f, bestD = bestA;
    func_L00_00260D30(moby, v0, *(float *)(d + 0x270));
    cur = *(int *)q;
    func_L00_0025EFC0(*(void **)(q + 0x10), moby + 0x10, v50, &cur, &dist, 0, 999.0f, 5.0f, 0.0f);
    *(int *)(d + 0x120) = 0;
    p60 = v60;
    do {
        if (*(int *)q != cur) {
            float ang, a;
            *(u128_2CB068 *)p60 = *(u128_2CB068 *)(*(char **)(q + 0x10) + *(int *)q * 16 + 0x10);
            a = func_L00_001FF860(v60[0] - *(float *)(moby + 0x10), v60[1] - *(float *)(moby + 0x14));
            ang = func_001FA850(a, func_L00_001FF860(v0[0] - *(float *)(moby + 0x10), v0[1] - *(float *)(moby + 0x14)));
            dist = func_001F9D10(v0, p60);
            if (ang < 1.5707964f) {
                if (bestA < ang) {
                    best = *(int *)q;
                    bestD = dist;
                    bestA = ang;
                }
            } else if ((dist >= 16.0f && (dist < bestD || bestD < 16.0f)) || (dist < 16.0f && bestD < dist)) {
                best = *(int *)q;
                bestD = dist;
                bestA = ang;
            }
        }
        {
            int *tab = *(int **)(q + 0x10);
            *(int *)q = (*(int *)q + *tab + *(char *)(q + 4)) % *tab;
        }
    } while (*(int *)q != 0);
    *(int *)q = best;
}
