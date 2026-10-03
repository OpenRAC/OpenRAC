/* NON_MATCHING func_L00_002761C0 -- src/overlays/shared/partupd_00272158.c
 * Best so far: SIZE ours 500 / retail 492, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002761C0 (PartType73Update): particle update that kills itself out of bounds (0..512) or below the gr
 *   Only difference: SIZE 500 vs 492. `mfc1 $2,$f1` followed directly by `bnel $17,$2` / `beq $19,$2` (the `ix != 
 *   Would unblock: pipeline fix so the assembler adds no nop between mfc1 and a branch reading its result (mult af
 */
typedef struct { float f[4]; } V73 __attribute__((aligned(16)));
typedef struct { char pad0[0x10]; V73 pos; V73 vel; } O73;
extern float func_L00_002644E0(void *);
extern void func_L00_00264570(void *);
extern char D_L00_00160310[];
extern float D_L00_00160318 MACRO_ADDR;
extern float D_L00_00160320 MACRO_ADDR;

/* Part type 73 update: a falling particle that adds its velocity, dies out of bounds or on hitting the ground, and accelerates downward. Adapted from Lombyte (MIT) for PAL: overlays/shared/rendering_002712b8.c, FUN_L00_00275320. */
void func_L00_002761C0(O73 *o) {
    V73 tmp;
    V73 t2;
    V73 t3;
    V73 *vel = &o->vel;
    float h;
    int ix;
    int iy;

    h = vel->f[3];
    if (o->pos.f[0] < 0.0f || o->pos.f[1] < 0.0f || 512.0f < o->pos.f[0] || 512.0f < o->pos.f[1]) {
        func_L00_002688A8(o);
        return;
    }
    if (o->pos.f[2] < h) {
        tmp = o->pos;
        func_L00_00264570(&tmp);
        func_L00_002688A8(o);
        return;
    }
    tmp = o->vel;
    ix = o->pos.f[0];
    iy = o->pos.f[1];
    tmp.f[3] = 0.0f;
    func_001F9BD8(&o->pos, &o->pos, &tmp);
    func_001F9BD8(&o->pos, &o->pos, D_L00_00160310);
    if ((int)o->pos.f[0] != ix || (int)o->pos.f[1] != iy) {
        t2 = o->pos;
        h = func_L00_002644E0(&t2);
        if (h < D_L00_00160320) {
            h = D_L00_00160320;
        } else {
            t3 = o->pos;
            h = func_L00_002644E0(&t3);
        }
        if (o->pos.f[2] < h) {
            t2 = o->pos;
            func_L00_00264570(&t2);
            func_L00_002688A8(o);
            return;
        }
    }
    tmp.f[0] = 0.0f;
    tmp.f[1] = 0.0f;
    tmp.f[3] = 0.0f;
    tmp.f[2] = -(D_0015EE70 * 0.5f);
    func_001F9BD8(vel, &tmp, vel);
    vel->f[3] = h + D_L00_00160318;
}
