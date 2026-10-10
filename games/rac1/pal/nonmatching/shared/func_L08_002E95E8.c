/* NON_MATCHING func_L08_002E95E8 -- src/overlays/shared/vendor_002D3DF8.c
 * Best so far: SIZE ours 1392 / retail 1396, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Stopped at run 6 of 10 with p4.c at 1392 bytes against retail 1396 (-4). Rocket update: kill checks on pos, a 
 *   Left: retail stores d+0x10 once at a shared label reached by FE/FD and the 0x1B3 mismatch; in ours the flag fo
 *   Unblock: a form of the state test that keeps the 0x1B3 compare a direct branch while sharing the store.
 */
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern float func_00214158(void);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern float func_001F9F90(float);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_001F9FA8(float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_00219780(void *, void *, void *, int, int, int, int, int, int);
extern float func_001FA748(float, float);
extern float func_001F9CB8(void *);
extern float func_001F9D10_1edff8(void *, void *) __asm__("func_001F9D10");
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_002607A8(void *, float);
extern int func_001F9908_i(void *) __asm__("func_001F9908");
extern void func_0020D678(void *);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, int, int, float, float, float, float, int, float, int, int, int, int);
extern float D_0015EE6C MACRO_ADDR;
extern char D_L08_001746E0[];
extern short D_L08_00161D6C;
extern short D_L08_00161D74;
extern short D_L08_00161D78;
extern short D_L08_00161D7C;
extern short D_L08_00161D90;
extern short D_L08_00161D98;
extern short D_L08_00161D9C;
extern short D_L08_00161DA0;
extern short D_L08_00161DAC;
extern short D_L08_00161DB0;

// Rocket (moby class 458) update: homes the rocket on its target, emits the trail, kills it out of range.
void func_L08_002E95E8(char *moby) {
    char *d;
    char *pos;
    char *p2;
    char *o;
    char *ob;
    float v20[4];
    float v30[4];
    float v40[4];
    float v50[4];
    float v60[4];
    int blk[6];
    float thr;
    float f20;
    float f21;
    float f22;
    float f1g;
    float f0g;
    float fx;
    float fy;
    float fz;
    float t;
    float t18;
    int k;
    int ra;
    int rb;
    int rc;
    int r;

    d = *(char **)(moby + 0x78);
    pos = moby + 0x10;
    qcopy(v20, pos);
    func_001F9BD8(pos, pos, d);
    fx = *(float *)(moby + 0x10);
    fy = *(float *)(moby + 0x14);
    fz = *(float *)(moby + 0x18);
    if (fx < 10.0f || fy < 10.0f || fz < 10.0f || 500.0f < fx || 500.0f < fy || 500.0f < fz) {
        func_0020D678(moby);
        return;
    }
    thr = *(float *)&D_L08_00161D6C;
    func_001F9C30(v40, d, -1.0f / thr);
    qcopy(v30, pos);
    p2 = (char *)v30;
    k = 0;
    if (0.0f < thr) {
        f22 = 1.2f;
        do {
            func_001F9BD8(p2, p2, v40);
            f21 = func_002140F8(0.0f, *(float *)&D_L08_00161D90);
            f20 = func_001F9F90(f21) * D_0015EE6C;
            func_L00_001FF4B0(v50, moby + 0xE0, f20 * func_001F9F90(f21));
            func_L00_001FF4B0(v60, moby + 0xD0, f20 * func_001F9FA8(f21));
            func_001F9BD8(v50, v50, v60);
            qcopy(v60, v60);
            fx = *(float *)&D_L08_00161DB0;
            fy = *(float *)&D_L08_00161DAC;
            k++;
            v60[3] = fx;
            v50[3] = fy;
            fx = (float)*(int *)&D_L08_00161D98;
            ra = func_001FA898_r(func_001F9878(func_002140F8(fx, fx * f22)));
            fy = (float)*(int *)&D_L08_00161D9C;
            rb = func_001FA898_r(func_001F9878(func_002140F8(fy, fy * f22)));
            fz = (float)*(int *)&D_L08_00161DA0;
            rc = func_001FA898_r(func_001F9878(func_002140F8(fz, fz * 1.5f)));
            func_00219780(p2, v50, v60, *(int *)&D_L08_00161D78, *(int *)&D_L08_00161D7C, ra, rb, rc, -1);
        } while ((float)k < thr);
    }
    fx = *(float *)&D_L08_00161D74;
    fy = (float)*(int *)(d + 0x1C);
    t = fx * 0.017453292f;
    t = t * D_0015EE6C;
    t = t * fy;
    *(float *)(moby + 0x40) = func_001FA748(*(float *)(moby + 0x40), t);
    o = *(char **)(d + 0x10);
    if (o != 0) {
        int off;
        off = 0;
        if (*(unsigned char *)(o + 0x20) == 0xFE) {
            off = 1;
        } else if (*(unsigned char *)(o + 0x20) == 0xFD) {
            off = 1;
        } else if (*(short *)(o + 0xA6) != 0x1B3) {
            off = 1;
        }
        if (off) {
            *(int *)(d + 0x10) = 0;
        } else {
            ob = *(char **)(o + 0x78);
            f20 = func_001F9CB8(d);
            fx = func_001F9D10_1edff8(pos, o + 0x10);
            func_001F9C30(v30, ob + 0x20, fx / f20);
            func_001F9BD8(v30, v30, o + 0x10);
            func_001F9BF0(v40, v30, pos);
            func_L00_001FF4B0(v40, v40, f20);
            func_001F9BF0(v50, v40, d);
            func_L00_002607A8(v50, D_0015EE6C * 3.7f);
            func_001F9BD8(d, d, v50);
            func_L00_001FF4B0(d, d, f20);
        }
    }
    f20 = 2.0f;
    blk[0] = (int)moby;
    blk[1] = 0x50001;
    *(float *)&blk[3] = f20;
    blk[4] = 1;
    qcopy(v30, d);
    r = func_001F9908_i(d + 0x18);
    if (r != 0) {
        func_0020D678(moby);
        return;
    }
    r = func_L00_001EFFF0(v20, pos, 0, (int)moby, (int)v30);
    if (r == 0) {
        return;
    }
    qcopy(pos, D_L08_001746E0);
    if (15.0f < *(float *)(moby + 0x10) && 15.0f < *(float *)(moby + 0x14) && 15.0f < *(float *)(moby + 0x18)) {
        t18 = 24.0f;
        t = *(float *)(moby + 0x10) - f20;
        if (t < t18) t18 = t;
        t = *(float *)(moby + 0x14) - f20;
        if (t < t18) t18 = t;
        t = *(float *)(moby + 0x18) - f20;
        if (t < t18) t18 = t;
        func_L00_0025F4A8(moby, d, pos, 3.0f, 3.0f, 0, 10, 20, 5.0f, 3.0f, 9.0f, 1.0f, -1, t18, 1, 1, -1, 0);
    }
    func_0020D678(moby);
}
