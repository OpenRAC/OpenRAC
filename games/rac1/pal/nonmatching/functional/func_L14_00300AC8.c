/* func_L14_00300AC8 -- src/overlays/shared/vendor_002B2A28.c (functional C for the port, not a match)
 * Update of the Kalebo III floating mine (class 933, levels 14 and 16). State 0: hover height unset;
 * a placed mine (no carrier drone) sits 0.7 above the ground and rolls its three spin rates, then
 * goes to 1; a carried one goes to 1 once its drone is dead. State 1: on level 16 it springs to its
 * hover height (a dropped mine is deleted unless Ratchet grinds); a hit (not from class 0x283) or
 * Ratchet's touch blows it up (the touch with a damage sphere); a placed mine then hides (state 2),
 * a dropped one is deleted. State 2: back to 1 once out of view. Every state but the blow-up then
 * pulses the glow and spins the mine.
 * Adapted with ReRAC's kalebo_mine.rs (ISC), which documents the US twin. equiv: FUNCTIONAL. */
typedef unsigned int Q_300AC8 __attribute__((mode(TI), aligned(16)));

typedef struct {
    float f0;
    short h4;
    unsigned char pad6[0x10 - 6];
    float f10;
    unsigned char pad14[0x3A - 0x14];
    unsigned char b3A;
    unsigned char b3B;
    unsigned char pad3C[4];
} Hit_300AC8;
typedef struct {
    unsigned char pad0[0xA6];
    short oclass;
} Cls_300AC8;
typedef struct {
    unsigned char pad0[0x20];
    Cls_300AC8 *attacker;
} HitMsg_300AC8;
typedef struct {
    void *carrier;
    float height;
    float vel;
    float phase;
    float spin[3];
} Pv_300AC8;
typedef struct {
    unsigned char pad0[0x10];
    float x, y, z;
    unsigned char pad1C[4];
    unsigned char state;
    unsigned char pad21[3];
    char *cls;
    unsigned char pad28[0x30 - 0x28];
    unsigned char updateDist;
    unsigned char pad31[3];
    unsigned short mode;
    unsigned char pad36[0x40 - 0x36];
    float rot[3];
    unsigned char pad4C[0x78 - 0x4C];
    Pv_300AC8 *pv;
    unsigned char pad7C[0x90 - 0x7C];
    unsigned int glow;
    int coll;
    unsigned char pad98[0xA4 - 0x98];
    unsigned char hitSlot;
} Moby_300AC8;

extern float D_L14_001620D0_300AC8 SDATA(D_L14_001620D0);
extern int D_0015EE84_300AC8 __asm__("D_0015EE84") MACRO_ADDR;
extern float D_0015EE64_300AC8 __asm__("D_0015EE64") MACRO_ADDR;
extern unsigned int D_L14_0016009C_300AC8 __asm__("D_L14_0016009C") MACRO_ADDR;
typedef struct {
    unsigned char pad0[0x23C];
    void *contact;
    unsigned char pad240[0x208C - 0x240];
    int group;
} Hero_300AC8;
extern Hero_300AC8 G_300AC8 __asm__("D_0013F450");

extern float func_00214358_300AC8(void *, int, float) __asm__("func_00214358");
extern float func_002140F8_300AC8(float, float) __asm__("func_002140F8");
extern void func_L00_0025C918_300AC8(float *, float *, float, float, float, float) __asm__("func_L00_0025C918");
extern HitMsg_300AC8 *func_L00_0025B478_300AC8(void *, int, int) __asm__("func_L00_0025B478");
extern void func_001F99D8_300AC8(void *, int) __asm__("func_001F99D8");
extern int func_L00_0025B4D0_300AC8(void *, void *, void *, int, int *, float *, int, int) __asm__("func_L00_0025B4D0");
extern void func_001F9BC0_300AC8(void *) __asm__("func_001F9BC0");
extern void func_0022ED80_300AC8(int, int, void *) __asm__("func_0022ED80");
extern void func_L00_0025F4A8_300AC8(void *, void *, void *, int, int, int, int, int, int, int, int,
                                     float, float, float, float, float, float, float) __asm__("func_L00_0025F4A8");
extern void func_0020D678_300AC8(void *) __asm__("func_0020D678");
extern int func_L00_00200290_300AC8(void *, float) __asm__("func_L00_00200290");
extern float func_001FA748_300AC8(float, float) __asm__("func_001FA748");
extern float func_001F9FA8_300AC8(float) __asm__("func_001F9FA8");
extern int func_001FA898_300AC8(float) __asm__("func_001FA898");

void func_L14_00300AC8(Moby_300AC8 *m) {
    union {
        Hit_300AC8 rec;
        Q_300AC8 v;
    } u;
    float dir[4];
    int out;
    Pv_300AC8 *pv = m->pv;
    HitMsg_300AC8 *hit;
    float s;
    int b, g;

    switch (m->state) {
    case 0:
        pv->height = -1.0f;
        if (pv->carrier == 0) {
            m->z = func_00214358_300AC8(&m->x, 0, 0.5f) + 0.7f;
            pv->spin[0] = func_002140F8_300AC8(0.010471975f, 0.02094395f);
            pv->spin[1] = func_002140F8_300AC8(0.006981317f, 0.013962634f);
            pv->spin[2] = func_002140F8_300AC8(0.0034906585f, 0.006981317f);
            m->state = 1;
        } else if (((signed char *)pv->carrier)[0x20] < 0) {
            m->state = 1;
        }
        break;
    case 1:
        if (D_0015EE84_300AC8 == 0x10) {
            if (pv->height < 0.0f) {
                pv->height = func_00214358_300AC8(&m->x, 0, 0.5f) + 0.7f;
            }
            func_L00_0025C918_300AC8(&m->z, &pv->vel, pv->height, D_0015EE64_300AC8 * 0.03f,
                                     D_0015EE64_300AC8 * 0.3f, 0.0f);
            if (D_L14_0016009C_300AC8 < (unsigned int)m && G_300AC8.group != 0xF) {
                func_0020D678_300AC8(m);
                return;
            }
        }
        hit = func_L00_0025B478_300AC8(m, 0x230000, 0);
        func_001F99D8_300AC8(&u.rec, 0x40);
        u.rec.f0 = 1.0f;
        u.rec.b3B = 0xFF;
        u.rec.b3A = 7;
        u.rec.h4 = 1;
        u.rec.f10 = 0.5f;
        func_L00_0025B4D0_300AC8(m, hit, &u.rec, 0, &out, 0, 0, 4);
        if (out < 2 || hit == 0 || hit->attacker->oclass == 0x283) {
            if (G_300AC8.contact != m) {
                break;
            }
        }
        func_001F9BC0_300AC8(dir);
        func_0022ED80_300AC8(0, 0, m);
        if (G_300AC8.contact == m) {
            func_L00_0025F4A8_300AC8(m, dir, &m->x, 10, 20, 60, -1, 1, 1, -1, 0,
                                     3.0f, 1.0f, 3.0f, 5.0f, 9.0f, 1.0f, 15.0f);
        } else {
            func_L00_0025F4A8_300AC8(m, dir, &m->x, 10, 20, 60, -1, 1, 1, -1, 0,
                                     0.0f, 0.0f, 3.0f, 5.0f, 9.0f, 1.0f, 15.0f);
        }
        if ((unsigned int)m < D_L14_0016009C_300AC8) {
            m->updateDist = 0xFF;
            m->state = 2;
            m->mode |= 0x41;
            m->hitSlot = 0xFF;
            m->coll = 0;
            return;
        }
        func_0020D678_300AC8(m);
        return;
    case 2:
        qcopy(&u.v, &m->x);
        ((float *)&u.v)[3] = 3.0f;
        if (func_L00_00200290_300AC8(&u.v, 64.0f) == -1) {
            m->state = 1;
            m->mode &= 0xFFBE;
            m->coll = *(int *)(m->cls + 0x10);
        }
        break;
    }

    s = func_001FA748_300AC8(pv->phase, D_L14_001620D0_300AC8);
    pv->phase = s;
    s = func_001F9FA8_300AC8(s);
    b = func_001FA898_300AC8(s * 48.0f) + 0xCF;
    g = func_001FA898_300AC8(s * 32.0f) + 0x30;
    m->glow = (g << 16) | 0xFF000000 | (g << 8) | b;
    m->rot[0] = func_001FA748_300AC8(m->rot[0], pv->spin[0]);
    m->rot[1] = func_001FA748_300AC8(m->rot[1], pv->spin[1]);
    m->rot[2] = func_001FA748_300AC8(m->rot[2], pv->spin[2]);
}
