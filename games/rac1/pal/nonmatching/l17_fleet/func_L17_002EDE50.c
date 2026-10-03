/* NON_MATCHING func_L17_002EDE50 -- src/overlays/l17_fleet/vendor_002AA068.c
 * Best so far: BYTES 58/3252 (98.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   prints `.extern D,4` (lui) unless an `extern short` ref to the same symbol is parsed earlier in the file, whic
 *   never have. Needs tooling (e.g. treat a MACRO_ADDR float alias of a gp `extern short` symbol as small).
 *   (2) table 0x668-0x6f0: regs of A/C/F/J/K/L/P and two sched2 swaps (Q before N, PS before MS/NS, stores follow)
 *   Diagnostics: sched1's order (run23) already has N before Q like retail; the swap comes from sched2 under our r
 *   choice, and the final registers come from a sched2-aware local allocation (runs 6/7: with -fschedule-insns2 on
 *   alloc differs from plain 2.95), so they could not be steered further by statement order.
 *   - run25 p24: BYTES 65. p20 with 2110 read into a local at the top: worse (load position does matter for J).
 *   - run26 p25: BYTES 61. p20 with [0][1] inline instead of the x01 local: close but worse. Stopping with p20.
 */
extern void func_L00_0028EBF0(int);
extern int func_0022ED80(int, int, int);
void func_L17_002EFA20(char *moby, float *a, float *b);
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);
extern float func_L00_0025CCF0(char *, char *, int, float, float, float, float);
extern float func_001FA790(float, float);
extern float func_001F9CB8(void *);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001F9D48(void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_L13_002BB2F8(int, int, float, float, float, float, float);
extern float func_001F9B88(float);
extern void func_00215C00(void *, float, float, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001FA1F8(void *, void *);
extern void func_001F9EC0(void *, void *, void *);
extern void func_L00_002EBE88(void *);
extern float func_001FA748(float, float);
extern void func_L00_002EBEE0(void *);
extern float func_001F9FA8(float);
extern float func_001F9F90(float);
extern void func_001F3140(void);
extern int func_L00_001F10E0(float, void *, int, void *);
extern void func_L00_001FF610(void *, void *, void *);
extern float func_001F9CE8(void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_0025F4A8_alt(void *, void *, void *, float, float, int, int, int, float, float, float, float, int, float, int, int, int, int) __asm__("func_L00_0025F4A8");
extern void func_001F49B0(void *, void *);
extern void func_L17_002ED498();
extern float D_0015EE6C MACRO_ADDR;
extern char *D_L17_0016016C MACRO_ADDR;
extern char (*D_L17_0016016C_t)[128] __asm__("D_L17_0016016C") MACRO_ADDR;
extern unsigned char D_0013A5E0[];
extern unsigned char D_0013E633[];
extern float D_L17_0016D470;
extern char D_L17_001747C0[];
extern short D_L17_00162120;
extern short D_L17_00162078;
extern short D_L17_0016207C;
extern short D_L17_00162080;
extern short D_L17_00162084;
extern short D_L17_00162088;
extern short D_L17_0016208C;
extern short D_L17_00162090;
extern short D_L17_00162094;
extern short D_L17_00162098;
extern short D_L17_0016209C;
extern short D_L17_001620A4;
extern short D_L17_001620A8;
extern short D_L17_001620AC;
extern short D_L17_001620B0;
extern short D_L17_001620B4;
extern short D_L17_001620B8;
extern short D_L17_001620BC;
extern short D_L17_001620C0;
extern short D_L17_001620C4;
extern short D_L17_001620C8;
extern short D_L17_001620CC;
extern short D_L17_001620DC;
extern short D_L17_001620E0;
extern short D_L17_0016210C;
extern short D_L17_00162110;
extern short D_L17_0016211C;
extern short D_L17_0016212C;
extern short D_L17_00162130;
extern short D_L17_00162134;
extern short D_L17_00162138;
extern short D_L17_0016213C;
extern short D_L17_00162140;
extern short D_L17_00162144;
extern short D_L17_00162148;
extern short D_L17_0016214C;
extern short D_L17_00162150;
extern short D_L17_00162154;
extern short D_L17_00162170;
extern short D_L17_00162174;
extern short D_L17_00162178;
extern short D_L17_0016217C;
extern short D_L17_00162180;
extern short D_L17_00162184;
extern short D_L17_00162188;
extern short D_L17_0016218C;
extern short D_L17_00162190;
extern short D_L17_00162194;
extern short D_L17_00162198;
extern short D_L17_0016219C;
extern short D_L17_001621A0;
extern short D_L17_001621A4;
extern short D_L17_001621A8;
extern short D_L17_001621AC;
extern short D_L17_001621B0;
extern short D_L17_001621B4;
extern short D_L17_001621B8;
extern short D_L17_001621BC;
extern short D_L17_001621C0;
extern short D_L17_001621C4;
extern short D_L17_001621C8;
extern short D_L17_001621CC;
extern short D_L17_001621D0;
extern short D_L17_001621D4;
extern short D_L17_001621DC;
extern short D_L17_001621E0;
extern short D_L17_001621E4;
extern short D_L17_001621E8;
extern short D_L17_001621FC;
extern short D_L17_00162270;

typedef float W4[4] __attribute__((aligned(16)));

typedef struct {
    char pad00[0x10];
    W4 pos;
    unsigned char state;
    char pad21[0x13];
    unsigned short flags;
    char pad36[0xA];
    float r40;
    float r44;
    float r48;
    char pad4C[0x70];
    unsigned char bc;
} Moby;

typedef struct {
    char pad00[0x10];
    W4 v10;
    float f20, f24, f28, f2C;
    float f30, f34, f38, f3C;
    char pad40[0x20];
    unsigned char sel;
    unsigned char flip;
    short s62;
    float speed;
    short count;
    short s6A;
    float f6C;
    float f70;
    char pad74[0x1C];
    float f90, f94, f98, f9C;
    float fA0, fA4, fA8, fAC;
    float fB0, fB4, fB8, fBC;
    float fC0, fC4, fC8, fCC;
    float fD0, fD4, fD8;
    int iDC;
    char padE0[0x2C];
    int i10C;
    int i110;
} Obj;

extern float D_L17_00162084_f __asm__("D_L17_00162084") MACRO_ADDR;
extern float D_L17_00162088_f __asm__("D_L17_00162088") MACRO_ADDR;
extern float D_L17_001620A4_f __asm__("D_L17_001620A4") MACRO_ADDR;
extern float D_L17_001620A8_f __asm__("D_L17_001620A8") MACRO_ADDR;
extern float D_L17_00162120_f __asm__("D_L17_00162120") MACRO_ADDR;

/* Flies the player's ship: throttle and stick input, steering toward the target, camera, and collision response. */
void func_L17_002EDE50(Moby *m, Obj *o) {
    float dt = D_L17_00162120_f;
    W4 old;
    W4 vel = { 0.0f, 0.0f, D_0015EE6C * 8.0f * dt, 0.0f };
    W4 dir;
    W4 side;
    W4 up = { 0.0f, 0.0f, 1.0f, 0.0f };
    W4 ang;
    W4 target;
    W4 diff;
    float a;
    float b;
    float ka;
    float kb;
    float lenA;
    float lenB;
    float d;
    float e1, e2;
    float x, y;
    float h, tn;
    float ratio;
    float t;
    float k;
    float c0, c1, c2;
    char *pad;
    char *g;
    char *e;
    char *p;
    int idx;
    int hc;

    m->flags &= 0xFEFF;
    if (m->bc == 0) {
        if (*(int *)&D_L17_00162080 < o->count) {
            pad = (char *)D_0013A5E0 + 0x2460;
            if (*(int *)(pad + 0x1B4) & 0x40) {
                idx = o->i10C;
                if (idx != -1) {
                    e = (char *)D_0013E633 + 0x1D + idx * 0x70;
                    if (*(char **)(e + 0x88) == (char *)m && *(unsigned char *)(e + 0x74) != 0) {
                        func_L00_0028EBF0(idx);
                    }
                }
                o->i10C = -1;
                o->i10C = func_0022ED80(1, 4, (int)m);
            } else if (*(int *)(pad + 0x1B8) & 0x40) {
                idx = o->i10C;
                if (idx != -1) {
                    e = (char *)D_0013E633 + 0x1D + idx * 0x70;
                    if (*(char **)(e + 0x88) == (char *)m && *(unsigned char *)(e + 0x74) != 0) {
                        func_L00_0028EBF0(idx);
                    }
                }
                o->i10C = -1;
            }
            pad = (char *)D_0013A5E0 + 0x2460;
            if (*(int *)(pad + 0x1B0) & 0x40) {
                float f = o->speed + *(float *)&D_L17_0016208C * D_L17_00162120_f;
                float max = *(float *)&D_L17_00162088 * D_L17_00162120_f;
                o->speed = f;
                if (max < f) {
                    o->speed = max;
                }
            } else {
                float hi = *(float *)&D_L17_00162084 * D_L17_00162120_f;
                float step = *(float *)&D_L17_0016207C * D_L17_00162120_f;
                float cur = o->speed;
                if (cur < hi - step) {
                    o->speed = cur + step;
                } else if (hi < cur) {
                    o->speed = cur + (hi - cur) * *(float *)&D_L17_00162090;
                } else {
                    o->speed = hi;
                }
            }
        } else {
            o->speed += *(float *)&D_L17_00162078 * dt;
            o->count += 1;
        }
    } else {
        o->speed = 0.0001f;
    }

    if (*(int *)&D_L17_00162094 < o->count) {
        g = (char *)D_0013E633 + 0xE1D;
        x = *(float *)(g + 0x1D20);
        y = *(float *)(g + 0x1D24);
        ka = *(float *)&D_L17_00162134;
        kb = *(float *)&D_L17_00162144;
        a = x;
        b = y;
        if (*(int *)(D_0013A5E0 + 0x2600) & 0x200) {
            a = x / *(float *)&D_L17_001620A8 * *(float *)&D_L17_001620B0;
            b = y / *(float *)&D_L17_001620A4 * *(float *)&D_L17_001620AC;
            ka = *(float *)&D_L17_00162138;
            kb = *(float *)&D_L17_00162148;
        }
        func_L17_002EFA20((char *)m, &a, &b);
        func_L00_0025C918(&o->f6C, &o->f90, a, *(float *)&D_L17_0016212C, *(float *)&D_L17_00162130, ka);
        func_L00_0025C918(&o->f70, &o->f94, b, *(float *)&D_L17_0016213C, *(float *)&D_L17_00162140, kb);
        func_L00_0025CCF0((char *)&o->f30, (char *)&o->f98, 0, o->f6C * *(float *)&D_L17_0016210C, *(float *)&D_L17_0016214C, *(float *)&D_L17_00162150, *(float *)&D_L17_00162154);
        o->f34 = func_001FA790(o->f34, o->f70 * D_L17_001620A4_f);
        o->f38 = func_001FA790(o->f38, o->f6C * D_L17_001620A8_f);
        if (o->f34 > 1.3962634f) {
            o->f34 = 1.3962634f;
        } else if (o->f34 < -1.3962634f) {
            o->f34 = -1.3962634f;
        }
        if (m->r44 > 1.3962634f) {
            m->r44 = 1.3962634f;
        } else if (m->r44 < -1.3962634f) {
            m->r44 = -1.3962634f;
        }
    }

    lenA = 310.0f;
    lenB = 500.0f;
    if (o->i110 != -1) {
        int off = o->i110 << 7;
        char *base = D_L17_0016016C;
        qcopy(target, ((char (*)[128])base)[o->i110] + 0x30);
        lenA = func_001F9CB8(base + off + 0x20);
        func_001F9BD8(diff, (o->i110 << 7) + D_L17_0016016C, (o->i110 << 7) + D_L17_0016016C + 0x10);
        lenB = func_001F9CB8(diff);
    } else {
        qcopy(target, &D_L17_00162270);
    }

    d = func_001F9D48(m->pos, &D_L17_00162270);
    o->f38 = func_L13_002BB2F8(1, 5, d, o->f38, func_L00_001FF860(target[0] - m->pos[0], target[1] - m->pos[1]), 50.0f, lenB);
    m->r48 = func_L13_002BB2F8(1, 5, d, m->r48, func_L00_001FF860(target[0] - m->pos[0], target[1] - m->pos[1]), 50.0f, lenB);
    e1 = func_001F9B88(target[2] - m->pos[2]);
    o->f34 = func_L13_002BB2F8(1, 5, e1, o->f34, -func_L00_001FF860(func_001F9D48(m->pos, target), target[2] - m->pos[2]), 50.0f, lenA);
    e2 = func_001F9B88(target[2] - m->pos[2]);
    m->r44 = func_L13_002BB2F8(1, 5, e2, m->r44, -func_L00_001FF860(func_001F9D48(m->pos, target), target[2] - m->pos[2]), 50.0f, lenA);
    func_00215C00(dir, o->speed, o->f38, -o->f34);
    func_L00_001FF4B0(o->v10, dir, 1.0f);
    qcopy(old, m->pos);
    if (m->bc == 0) {
        func_001F9BD8(m->pos, m->pos, dir);
    }
    {
        float lo = D_L17_00162084_f;
        float sp = o->speed - lo;
        float range = D_L17_00162088_f - lo;
        if (sp < 0.0f) {
            ratio = 0.0f / range;
        } else {
            ratio = sp / range;
        }
    }
    {
        float sc = *(float *)&D_L17_0016211C;
        float x02 = (*(float *)&D_L17_001620B8 + o->f70 * *(float *)&D_L17_001621BC) * sc;
        float x10 = *(float *)&D_L17_001620BC * sc;
        float x01 = o->f6C * *(float *)&D_L17_00162110 * sc;
        W4 tbl[2] = {
            { (*(float *)&D_L17_001620B4 - o->f70 * *(float *)&D_L17_001621B8 - ratio * *(float *)&D_L17_001621C0) * sc,
              x01,
              x02 },
            { x10,
              0.0f,
              *(float *)&D_L17_001620C0 * sc }
        };
        float mat[12];
        W4 w;
        W4 r;
        c0 = *(float *)&D_L17_00162194;
        c1 = *(float *)&D_L17_00162198;
        c2 = *(float *)&D_L17_0016219C;
        o->fC4 = ratio;
        func_L00_0025C918(&o->fB4, &o->fC8, tbl[o->sel][0], c0, c1, c2);
        func_L00_0025C918(&o->fB8, &o->fCC, tbl[o->sel][1], *(float *)&D_L17_001621A0, *(float *)&D_L17_001621A4, *(float *)&D_L17_001621A8);
        func_L00_0025C918(&o->fBC, &o->fD0, tbl[o->sel][2], *(float *)&D_L17_001621AC, *(float *)&D_L17_001621B0, *(float *)&D_L17_001621B4);
        diff[0] = o->fB4;
        diff[1] = o->fB8;
        diff[2] = o->fBC;
        diff[3] = 0.0f;
        func_001FA1F8(mat, (char *)&o->f30);
        func_001F9EC0(target, diff, mat);
        func_001F9BD8(w, m->pos, target);
        func_L00_002EBE88(w);
        r[0] = func_001FA748(o->f30 * *(float *)&D_L17_001620C8, o->f6C * *(float *)&D_L17_001620C4);
        r[1] = func_001FA790(func_001FA748(o->f34, *(float *)&D_L17_001620CC), o->f70 * D_L17_001620A4_f);
        r[2] = o->f38;
        func_L00_0025CCF0((char *)&o->f20, (char *)&o->fA0, 0, r[0], *(float *)&D_L17_00162170, *(float *)&D_L17_00162174, *(float *)&D_L17_00162178);
        func_L00_0025CCF0((char *)&o->f24, (char *)&o->fA4, 0, r[1], *(float *)&D_L17_0016217C, *(float *)&D_L17_00162180, *(float *)&D_L17_00162184);
        func_L00_0025CCF0((char *)&o->f28, (char *)&o->fA8, 0, r[2], *(float *)&D_L17_00162188, *(float *)&D_L17_0016218C, *(float *)&D_L17_00162190);
    }
    ang[0] = o->f20;
    ang[2] = o->f28;
    ang[1] = o->f24;
    func_L00_002EBEE0(ang);
    func_L00_0025C918(&o->fB0, &o->fC0, *(float *)&D_L17_001621C4 + (*(float *)&D_L17_001621C8 - *(float *)&D_L17_001621C4) * ratio, *(float *)&D_L17_001621CC, *(float *)&D_L17_001621D0, *(float *)&D_L17_001621D4);
    h = o->fB0 * 0.5f;
    tn = func_001F9FA8(h) / func_001F9F90(h);
    D_L17_0016D470 = tn;
    func_001F3140();
    m->r40 = func_001FA748(o->f30, o->f6C * *(float *)&D_L17_001621E0);
    m->r44 = func_001FA748(o->f34, -o->f70 * *(float *)&D_L17_001621E4);
    m->r48 = func_001FA748(o->f38, -o->f6C * *(float *)&D_L17_001621E8);
    if (func_L00_001F10E0(1.2f, m->pos, 0, m)) {
        p = D_L17_001747C0;
        if (*(char **)(p + 0x18) == 0 || (hc = *(short *)(*(char **)(p + 0x18) + 0xA6), hc != 0 && hc != 0x192 && hc != 0x4C2 && hc != 0x4C3 && hc != 0x52 && hc != 0x53)) {
            qcopy(m->pos, p + 0x30);
            func_L00_001FF610(dir, dir, p + 0x40);
            o->f38 = func_001FA748(o->f38, func_001FA790(func_L00_001FF860(dir[0], dir[1]), o->f38) * *(float *)&D_L17_00162098);
            t = func_001FA748(o->f34, func_001FA790(-func_L00_001FF860(func_001F9CE8(dir), dir[2]), o->f34) * *(float *)&D_L17_0016209C);
            k = o->speed * *(float *)&D_L17_001620DC;
            o->f34 = t;
            o->speed = k;
            if (*(char **)(p + 0x18) != 0 && *(short *)(*(char **)(p + 0x18) + 0xA6) != 0x15B) {
                if (o->iDC == 0) {
                    g = (char *)D_0013E633 + 0xE1D;
                    *(float *)(g + 0x15FC) = *(float *)(g + 0x15FC) - *(float *)&D_L17_001620E0;
                    if (*(float *)(g + 0x15FC) < 0.0f) {
                        func_001F9BF0(side, p + 0x50, p + 0x60);
                        func_L00_001FF4B0(side, side, 1.0f);
                        func_L00_001FF610(vel, dir, p + 0x40);
                        func_L00_001FF4B0(vel, vel, D_0015EE6C + D_0015EE6C);
                        func_L00_001FF4B0(up, p + 0x40, 1.0f);
                        func_L00_0025F4A8_alt(m, vel, 0, 8.0f, 100.0f, 0x1E, 0xA, 0x18, 16.0f, 8.0f, 9.0f, 1.0f, -1, 50.0f, 1, 1, -1, 0);
                        m->state = 7;
                    }
                    o->iDC = *(int *)&D_L17_001621DC;
                }
            } else {
                *(int *)(D_0013E633 + 0x2419) = 0;
                func_L00_0025F4A8_alt(m, vel, 0, 8.0f, 100.0f, 0x1E, 0xA, 0x18, 16.0f, 8.0f, 9.0f, 1.0f, -1, 50.0f, 1, 1, -1, 0);
                o->s62 = D_L17_001621FC;
                m->state = 7;
                m->flags |= 0x41;
            }
        }
    }
    func_001F49B0(func_L17_002ED498, m);
    func_L17_002EDC40((char *)m, (char *)o);
    func_L17_002EDAF0((char *)m, (char *)o, func_001FA748(o->f28, -o->f6C / 7.0f),
                      -func_001FA748(o->f24, -(o->f70 / 7.0f) - 0.1f));
}
