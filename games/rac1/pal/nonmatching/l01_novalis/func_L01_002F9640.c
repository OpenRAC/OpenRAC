/* NON_MATCHING func_L01_002F9640 -- src/overlays/l01_novalis/vendor_002BA898.c
 * Best so far: BYTES 8/708 (98.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L01_002F9640 (FloatingPushableUpdate): state 0 stores the start position into the vars and goes to state 
 *   Lombyte's port (p3: player base D_0013E633+0xE1D assigned inside the radius test, the player position as D_001
 *   Same bytes with char-offset spelling of both pointers (p4), a local for the vel pointer (p5) and a local for t
 */
typedef struct { float x, y, z, w; } HV __attribute__((aligned(16)));
typedef struct {
    float home_x;
    float home_y;
    float pad8[2];
    float vel_x;
    float vel_y;
} HoverVars;
typedef struct {
    char pad0[0x10];
    HV pos;
    unsigned char state;
    char pad21[0x57];
    HoverVars *pvars;
} HoverMoby;

extern char D_0013E633[] NOT_SDA;
extern short D_L01_00161C80;
extern short D_L01_00161C84;
extern short D_L01_00161C88;
extern short D_L01_00161C8C;
extern short D_L01_00161C90;
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float func_L01_002F95A0(float target, float k, float a, float b, float *cur, float *vel);
extern float func_001F9D48(void *, void *);
extern float func_001F9B88(float);
extern float func_L00_001FF860(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_L00_002594C8(void *, void *, void *, int, float, float, float, float);
extern float func_001F9D10(void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_00214440(void *, int);

/* Update a floating pushable object: springs toward its home, is pushed out by the player, follows the ground. Adapted from Lombyte (MIT) for PAL: overlays/l01/unclassified_002b96e0.c, FUN_L01_002f8268. */
void func_L01_002F9640(HoverMoby *m) {
    HoverVars *v;
    char *pl;
    HV old;
    HV tgt;
    HV step;
    HV d;
    float ang;
    float dz;
    float lim;
    float k;

    old = m->pos;
    v = m->pvars;
    tgt = old;
    if (v == 0) {
        return;
    }
    switch (m->state) {
    case 0:
        *(HV *)v = old;
        m->state = 1;
        break;
    case 1:
        func_L01_002F95A0(v->home_x, *(float *)&D_L01_00161C80 * D_0015EE70, *(float *)&D_L01_00161C84, 3.0f, &m->pos.x, &v->vel_x);
        k = *(float *)&D_L01_00161C80 * D_0015EE70;
        func_L01_002F95A0(v->home_y, k, *(float *)&D_L01_00161C84, 3.0f, &m->pos.y, &v->vel_y);
        if (func_001F9D48(&m->pos, D_0013E633 + 0xE9D) < *(float *)&D_L01_00161C88) {
            pl = D_0013E633 + 0xE1D;
            if (func_001F9B88(m->pos.z - *(float *)(pl + 0x88)) < 0.7f) {
                ang = func_L00_001FF860(m->pos.x - *(float *)(pl + 0x80), m->pos.y - *(float *)(pl + 0x84));
                m->pos.x = func_001F9F90(ang) * *(float *)&D_L01_00161C88;
                m->pos.y = func_001F9FA8(ang) * *(float *)&D_L01_00161C88;
                m->pos.x += *(float *)(pl + 0x80);
                m->pos.y += *(float *)(pl + 0x84);
            }
        }
        func_L00_002594C8(m, &tgt, &m->pos, 3, 0.1f, *(float *)&D_L01_00161C90, 600.0f, 1.5707964f);
        m->pos.x = tgt.x;
        m->pos.y = tgt.y;
        m->pos.z = old.z;
        if (func_001F9D10(&old, &m->pos) > *(float *)&D_L01_00161C8C * D_0015EE6C) {
            func_001F9BF0(&d, &m->pos, &old);
            step = d;
            func_L00_001FF4B0(&step, &step, *(float *)&D_L01_00161C8C * D_0015EE6C);
            func_001F9BD8(&d, &old, &step);
            m->pos = d;
        }
        dz = func_00214440(&m->pos, 0) - m->pos.z;
        lim = D_0015EE6C * 3.0f;
        if (dz > lim) {
            dz = lim;
        } else if (dz < -lim) {
            dz = -lim;
        }
        m->pos.z += dz;
        break;
    }
}
