/* NON_MATCHING func_L08_002DB468 -- src/overlays/l08_batalia/vendor_002B9438.c
 * Best so far: BYTES 6/720 (99.2% of the bytes match), checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   mini16 a02: detects player crossing a plane, handles impact and emits particles. Best p3.c BYTES 6/720, actual
 *   Stopped budget 8/8 and three distinct p3/p6/p7 identical. Only +12c/+130 differ: stack velocity pointer and co
 */
extern void func_L02_002A52B0(void *, float);
extern void func_001F49B0(void *, void *);
extern void func_00216270(void);
extern int func_L00_0028EB98(void *, int);
extern void func_L00_0028EBF0(int);
extern char *func_L00_002D9340(void *, float);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_L00_00258BC8(int, int);
extern int func_002140B0(int);
extern void func_L00_002703E8(void *, void *, int, int);
extern void func_L00_00258DB0(float *, float, float);
extern void func_001F9BD8(void *, void *, void *);
extern unsigned char *func_L00_00272770(void *, void *, void *, float, float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float D_L08_001D7320[];
extern int D_0015EE84_r __asm__("D_0015EE84") MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_L08_0015F660[] MACRO_ADDR;
extern short D_L08_00161A00;
typedef struct { char pad0[0x80]; float pos[4]; char pad90[0x78]; float previous_z; char pad10c[0x1f74]; char *moby; char pad2084[0x22e]; short handle; } Player08;
extern Player08 D_0013F450;

/* detects the player crossing a height plane and emits impact particles */
void func_L08_002DB468(char *m) {
    float pos[4], v[4];
    int i;
    Player08 *player;
    if (*(int *)&D_L08_00161A00 == 0) {
        *(int *)&D_L08_00161A00 = 1;
        func_L02_002A52B0(D_L08_001D7320, 0.6666667f);
    }
    func_001F49B0(func_00216270, m);
    if (D_0015EE84_r != 8) return;
    player = &D_0013F450;
    if (!(player->pos[2] < D_L08_001D7320[2])) return;
    if (!(D_L08_001D7320[2] <= player->pos[2] - player->previous_z)) return;
    if (func_L00_0028EB98(player->moby, player->handle)) {
        func_L00_0028EBF0(player->handle);
        player->handle = -1;
    }
    qcopy(pos, player->pos);
    pos[2] = D_L08_001D7320[2];
    {
        char *p = func_L00_002D9340(pos, 3.0f);
        if (p) p[0x23] = 0x70;
    }
    i = 15;
    do {
        float a = func_00214158();
        float r = func_002140F8(D_0015EE6C * 0.0f, D_0015EE6C * 3.0f);
        int life, kind;
        v[0] = func_001F9F90(a) * r;
        v[1] = func_001F9FA8(a) * r;
        v[2] = func_002140F8(D_0015EE6C * 3.0f, D_0015EE6C * 6.5f);
        life = func_L00_00258BC8(90, 120);
        kind = func_002140B0(2);
        func_L00_002703E8(pos, v, kind, life);
        i--;
    } while (i >= 0);
    for (i = 0; i < 16; i++) {
        float speed;
        float side;
        unsigned char *p;
        func_L00_00258DB0(v, 1.0f, 1.0f);
        func_001F9BD8(v, v, pos);
        v[2] = D_L08_001D7320[2] + 0.05f;
        speed = func_002140F8(0.7f, 1.0f);
        side = -2.0f;
        if (i == 0) side = 2.0f;
        p = func_L00_00272770(v, D_L08_0015F660, D_L08_001D7320 + 2, speed, side);
        if (p) *(short *)(p + 0xA) = func_001FA898_r(func_001F9878(func_002140F8(30.0f, 60.0f)));
    }
}
