/* NON_MATCHING func_L00_00236BF8 -- src/overlays/shared/hud_00235960.c
 * Best so far: SIZE ours 504 / retail 492, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Fills 100 16-byte hud particle records at D_L00_0017E760 (x, y from func_002140F8 ranges, two drift floats as 
 *   Best try p4.c (struct array, hi = lo + span before the calls): same instructions but 508 vs 492 bytes: ours ke
 *   Flat float[] indexing (p6) gives 460 bytes (everything reduced to one giv); 2D/struct/pointer forms all give 5
 */
extern float func_002140F8(float, float);
extern float D_L00_0017E760[][4];
extern short D_L00_0015F830;
extern short D_L00_0015F834;
extern short D_L00_0015F838;
extern short D_L00_0015F83C;

/* fills 100 hud particle records with randomised positions and drift */
void func_L00_00236BF8(HudElem *e) {
    int i;
    float v, lo, span;
    int n;

    func_L00_00236710(e);
    for (i = 0; i < 100; i++) {
        v = i < 10 ? *(float *)&D_L00_0015F830 : i < 20 ? *(float *)&D_L00_0015F834 : *(float *)&D_L00_0015F838;
        n = *(int *)&D_L00_0015F83C;
        span = (232.0f - (v + v)) / (float)n;
        lo = (v - 116.0f) + (float)(i % n) * span;
        D_L00_0017E760[i][0] = func_002140F8(v - 17.0f, 17.0f - v);
        D_L00_0017E760[i][1] = func_002140F8(lo, lo + span);
        D_L00_0017E760[i][2] = func_002140F8(-0.4f, 0.4f);
        D_L00_0017E760[i][3] = func_002140F8(-0.6f, 0.6f);
        D_L00_0017E760[i][2] += func_002140F8(-0.4f, 0.4f);
        D_L00_0017E760[i][3] += func_002140F8(-0.6f, 0.6f);
        D_L00_0017E760[i][2] += func_002140F8(-0.4f, 0.4f);
        D_L00_0017E760[i][3] += func_002140F8(-0.6f, 0.6f);
    }
}
