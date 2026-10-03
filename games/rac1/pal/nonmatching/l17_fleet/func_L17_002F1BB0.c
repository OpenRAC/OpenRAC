/* NON_MATCHING func_L17_002F1BB0 -- src/overlays/l17_fleet/vendor_002F1558.c
 * Best so far: SIZE ours 368 / retail 364, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Retail has the load first and the pitch store before the 0.02/0.3 constants; no source form found that gets bo
 *   [l17n2 t06] p13 (D loaded into s first, both stores, a/b inline in the call) 412/364, p14 (stores then inline 
 *   Still the same wall as before: retail wants the D load first in the join block and the pitch store second, bef
 *   [l17n3 u03] p15 (yaw store right after yaw clamp, D first, pitch store): 142 bytes diff, worse. p17 (const D, 
 *   p19 (stores via float *pp = data+0x294, pp[0]/pp[1], D first): 380, worse. (p18 was a compile error.) Same wal
 *   the pitch store second (so pitch stays $f0); every form with the load first lets sched1 sink the store to the 
 *   [l17n4 v08] 3 runs: p20 (p6 with a/b inline-D, between stores) 46/364; p21 (p7 with const D) 368/364, store si
 *   Same wall as before: retail wants D load first in the join, pitch store second (pitch stays $f0), before the 0
 */
extern char D_0013E633[];
extern const float D_0015EE64 MACRO_ADDR;
extern void func_001F9BF0(void *, void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001FA790(float, float);
extern float func_001F9CE8(void *);
extern void func_L00_00263950(char *, char *, int, float, float);

/* Aims the turret data at the player: yaw and pitch clamped, then feeds the smoothing call. */
void func_L17_002F1BB0(char *moby) {
    char *data = *(char **)(moby + 0x78);
    float tmp[4];
    float v[4];
    float yaw;
    float pitch;
    float s;
    float a;
    float b;
    qcopy(tmp, moby + 0x10);
    tmp[2] += 0.5f;
    func_001F9BF0(v, D_0013E633 + 0xEED, tmp);
    yaw = func_001FA790(func_L00_001FF860(v[0], v[1]), *(float *)(moby + 0x48));
    pitch = -func_L00_001FF860(func_001F9CE8(v), v[2]);
    if (yaw > 1.0471976f) yaw = 1.0471976f;
    else if (yaw < -1.0471976f) yaw = -1.0471976f;
    if (pitch > 0.5235988f) pitch = 0.5235988f;
    else if (pitch < -0.5235988f) pitch = -0.5235988f;
    s = D_0015EE64;
    b = 0.3f * s;
    a = 0.02f * s;
    *(float *)(data + 0x294) = pitch;
    *(float *)(data + 0x298) = yaw;
    func_L00_00263950(moby, data + 0x230, 0, a, b);
}
