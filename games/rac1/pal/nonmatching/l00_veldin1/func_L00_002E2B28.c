/* NON_MATCHING func_L00_002E2B28 -- src/overlays/l00_veldin1/vendor_002DB278.c
 * Best so far: BYTES 158/1068 (85.2% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby troll (Lombyte FUN_L00_002e1678): takes damage, knockback/death state, then tracks a target moby. B
 *   Needed for size: Lombyte's TrollMoby/TrollVars structs (real struct stores let the float-global loads CSE), `t
 *   Differences: retail schedules the gp-rel loads (D_L00_00161CA4..B0, read as short + float cast) and D_0015EE70
 */
extern u8 D_0013F450[];
extern short D_L00_00160098_E2B28 __asm__("D_L00_00160098");
typedef u32 u128_E2B28 __attribute__((mode(TI), aligned(16)));
extern f32 D_0015EE6C MACRO_ADDR;
extern f32 D_0015EE70 MACRO_ADDR;
extern void func_L00_002E2B28_E2B28() __asm__("func_L00_002E2B28");
extern f32 func_001F9D48(void *, void *);
extern f32 func_L00_001FF860(f32, f32);
typedef union {
    u128_E2B28 q;
    f32 f[4];
} TrollVec;
typedef struct TrollMoby {
    u8 pad0[0x10];
    TrollVec pos;
    u8 state;
    u8 pad21[0x13];
    u16 flags;
    u8 pad36[0x42];
    struct TrollVars *vars;
    u8 pad7C[0x18];
    s32 unk94;
    u8 pad98[0xC];
    u8 unkA4;
} TrollMoby;
typedef struct TrollVars {
    u8 pad0[0x20];
    f32 health;
    u8 pad24[2];
    s16 unk26;
    u8 pad28[0x10];
    s32 unk38;
    u8 pad3C[0x24];
    u8 unk60[7];
    u8 unk67;
    u8 pad68[8];
    u8 unk70[0x10];
    f32 unk80;
    f32 unk84;
    f32 unk88;
    f32 unk8C;
    s32 unk90;
    s32 unk94;
    f32 unk98;
    u8 pad9C[0x11];
    u8 unkAD;
    u8 padAE[0xE];
    f32 unkBC;
    f32 unkC0;
    f32 unkC4;
    u8 padC8[8];
    TrollVec target;
    u8 padE0[0x30];
    TrollMoby *targetMoby;
    s32 unk114;
    u8 pad118[0x9C];
    s32 unk1B4;
    u8 pad1B8[4];
    s32 unk1BC;
    u8 pad1C0[0x1A];
    s16 unk1DA;
} TrollVars;
typedef struct {
    s32 unk0;
    u8 pad4[0xC];
    u8 data[1];
} TrollAnim;
typedef struct {
    u8 pad0[0x10];
    TrollVec pos;
} TrollColl;
extern s16 func_001F9850_E2B28(s32) __asm__("func_001F9850");
extern int func_001F9938(void *);
extern f32 func_001F9D48_E2B28b(void *, void *) __asm__("func_001F9D48");
extern float func_L00_001FF860_E2B28b(float, float) __asm__("func_L00_001FF860");
extern void func_L00_002584A8(TrollMoby *, s32, s32);
extern int func_L00_00258BC8(int, int);
extern TrollColl *func_L00_0025B478(TrollMoby *, s32, s32);
extern void func_L00_0025B4D0(TrollMoby *, TrollColl *, void *, s32, s32 *, f32 *, s32, s32);
extern void func_L00_0025BBA0(TrollVec *, f32 *, f32 *, f32 *);
extern void func_L00_0025D5B0(TrollMoby *, void *, s32, s32, s32, f32);
extern void func_L00_0025E4B0(TrollMoby *, void *);
extern void func_L00_0025E590(TrollMoby *, void *);
extern s32 func_L00_00260D30(TrollMoby *, f32, void *);
extern void func_L00_00260FB0(TrollMoby *, void *, s32, s32, void *, s32, f32);
extern void func_L00_002E34F0_E2B28(TrollMoby *) __asm__("func_L00_002E34F0");
extern f32 D_0015EE6C_E2B28b __asm__("D_0015EE6C") MACRO_ADDR;
extern f32 D_0015EE70_E2B28b __asm__("D_0015EE70") MACRO_ADDR;
extern short D_L00_00161CA0;
extern short D_L00_00161CA4;
extern short D_L00_00161CA8;
extern short D_L00_00161CAC;
extern short D_L00_00161CB0;
extern TrollAnim * D_L00_001B0830_E2B28[] __asm__("D_L00_001B0830");

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/overlays/l00/gameplay_vendor_002e0988.c, FUN_L00_002e1678. */
void func_L00_002E2B28_E2B28(TrollMoby *m)
{
    TrollVars *vars;
    void *anim;
    TrollColl *coll;
    TrollAnim *a;
    TrollMoby *t;
    TrollVec v;
    u8 *g;
    s32 hit;
    f32 dmg;
    f32 angle;
    f32 unused;
    f32 range;

    vars = m->vars;
    if (m->state == 0) {
        return;
    }
    if (vars->unk38 != 0) {
        vars->unk38 = 0;
        vars->unk1DA = func_001F9850_E2B28(func_L00_00258BC8(0xB4, 0x12C));
    }
    func_001F9938(&vars->unk1DA);
    anim = vars->unk60;
    dmg = 0.0f;
    coll = func_L00_0025B478(m, 0x330000, 0);
    func_L00_0025B4D0(m, coll, &vars->health, 0, &hit, &dmg, 0, 4);
    if (hit != 1 && m->state != 8 && dmg != 0.0f) {
        vars->health -= dmg;
        vars->unk90 = 0x200;
        vars->unk80 = (*(f32 *)&D_L00_00161CA4) * D_0015EE70_E2B28b;
        vars->unk84 = (*(f32 *)&D_L00_00161CA8) * D_0015EE70_E2B28b;
        vars->unk88 = (*(f32 *)&D_L00_00161CAC) * D_0015EE6C_E2B28b;
        vars->unk8C = (*(f32 *)&D_L00_00161CB0) * D_0015EE6C_E2B28b;
        vars->unkAD = 0;
        vars->unk94 = 9;
        vars->unkBC = D_0015EE6C_E2B28b + D_0015EE6C_E2B28b;
        vars->unk98 = 0.5f;
        if (vars->health <= 0.0f) {
            vars->unk94 = 0x29;
            m->flags &= ~0x1000;
            v.q = coll->pos.q;
            func_L00_0025BBA0(&v, &angle, &vars->unk88, &vars->unk8C);
            func_L00_0025D5B0(m, vars->unk70, 0xB, 1, 0, angle);
            vars->unkC0 = 7.0f;
            vars->unkC4 = 16.0f;
            m->state = 8;
            m->unk94 = 0;
            vars->unk67 = 0x78;
            func_L00_0025E4B0(m, anim);
            func_L00_002584A8(m, 0, -1);
        } else {
            vars->unk88 = (*(f32 *)&D_L00_00161CAC) * D_0015EE6C_E2B28b * 0.35f;
            vars->unk8C = (*(f32 *)&D_L00_00161CB0) * D_0015EE6C_E2B28b * 0.5f;
            v.q = coll->pos.q;
            func_L00_0025BBA0(&v, &unused, &vars->unk88, &vars->unk8C);
            func_L00_0025D5B0(m, vars->unk70, 8, 1, 0, func_L00_001FF860_E2B28b(coll->pos.f[0], coll->pos.f[1]));
            vars->unkC4 = vars->unkC0 = -1.0f;
            m->state = 7;
            vars->unk67 = 0xFA;
            vars->unk26 = func_001F9850_E2B28(0x3C);
            func_L00_0025E4B0(m, anim);
        }
        func_L00_002E34F0_E2B28(m);
    }
    m->unkA4 = 0xFF;
    func_L00_0025E590(m, anim);
    if (m->state == 1) {
        a = D_L00_001B0830_E2B28[vars->unk1B4];
        func_L00_00260FB0(m, &vars->target, 0, 0, a->data, a->unk0, 64.0f);
    } else if (m->state != 5) {
        range = (*(f32 *)&D_L00_00161CA0);
        if (vars->unk1DA != 0) {
            range = 37.0f;
        }
        if (func_L00_00260D30(m, range, &vars->target) != 2 && range < func_001F9D48_E2B28b(&m->pos, &vars->target)) {
            vars->unk114 = 2;
        }
    }
    if (vars->targetMoby == 0) {
        g = D_0013F450;
        vars->targetMoby = *(TrollMoby **)(g + 0x2080);
        qcopy(&vars->target, g + 0x80);
    }
    if (vars->unk1BC >= 0) {
        t = (TrollMoby *)((*(u8 * *)&D_L00_00160098_E2B28) + (vars->unk1BC << 8));
        if (t == 0 || t->state == 0xFE || t->state == 0xFD) {
            vars->unk1BC = -1;
        } else {
            range = func_001F9D48_E2B28b(&m->pos, &t->pos);
            if (range < func_001F9D48_E2B28b(&m->pos, &vars->targetMoby->pos)) {
                vars->targetMoby = t;
                qcopy(&vars->target, &t->pos);
                vars->unk114 = 1;
            }
        }
    }
}
