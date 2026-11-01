#include "include_asm.h"
#include "common.h"

extern void ref_L22_003A4FF8_00(void);
extern int func_L22_003A2DF8(int, float);

typedef struct Rec_003A2D58 {
    int count;
    unsigned char pad04[0x18];
    float angle;
} Rec_003A2D58;

typedef union F32Bits {
    float f;
    unsigned int u;
} F32Bits;

extern Rec_003A2D58 *D_L22_001FE940[];

INCLUDE_ASM("asm/levels", func_L22_0039D708);

INCLUDE_ASM("asm/levels", func_L22_003A15D0);


/* func_L22_003A1ED8 is defined in L22_003A1ED8.c. */

INCLUDE_ASM("asm/levels", func_L22_003A2160);

typedef int Quad_003A2780 __attribute__((mode(TI)));
extern float func_00282E48(void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern int func_002761F8(void *, void *, int, int, int);
extern void func_L22_003A2988(void *, float);

/* Walks from FROM to TO in steps of 8 units, each point dropped by the placement helper, and
 * returns 1 when no segment of the walk hits anything. Points over 50 units apart fail. */
int func_L22_003A2780(char *obj, Quad_003A2780 *from, Quad_003A2780 *to) {
    struct { Quad_003A2780 end, step, previous, next, cursor, scaled; } v;
    char *state = *(char **)(obj + 0x68);
    int last;
    float dist;

    if (func_00282E48(from, to) > 50.0f) return 0;
    qcopy(&v.end, to);
    func_L22_003A2988(&v.end, *(float *)(state + 0x114) + *(float *)(state + 0x110));
    func_00282CF0(&v.step, &v.end, from);
    func_00282F20(&v.step, &v.step, 8.0f);
    qcopy(&v.cursor, from);
    qcopy(&v.previous, from);
    func_L22_003A2988(&v.previous, *(float *)(state + 0x114) + *(float *)(state + 0x110));
    last = 0;
    do {
        dist = func_00282E48(&v.cursor, &v.end);
        if (dist < 8.0f) {
            if (dist < 0.0f) return 1;
            func_00282F20(&v.scaled, &v.step, dist);
            last = 1;
            func_00282CC0(&v.next, &v.cursor, &v.scaled);
            func_L22_003A2988(&v.next, *(float *)(state + 0x114) + *(float *)(state + 0x110));
        } else {
            func_00282CC0(&v.next, &v.cursor, &v.step);
            func_L22_003A2988(&v.next, *(float *)(state + 0x114) + *(float *)(state + 0x110));
        }
        if (func_002761F8(&v.previous, &v.next, 0x12, 0, 0)) return 0;
        qcopy(&v.previous, &v.next);
        func_00282CC0(&v.cursor, &v.cursor, &v.step);
    } while (!last);
    return 1;
}

/* func_L22_003A2988 is defined in L22_003A2988-qcopy-barrier.c. */

/* func_L22_003A2A28 is defined in L22_003A2A28-qcopy-barrier.c. */

extern int func_L22_003A48F8(int);

/* Collects the set bit positions of VALUE, keeping those a null FLAG lets
 * through and, otherwise, those the shared table scan accepts. */
int func_L22_003A2CB8(float value, int unused, int *out, int flag) {
    int pos = 0;
    int count = 0;
    unsigned int mask;
    int *o;
    F32Bits bits;
    bits.f = value;
    mask = bits.u;
    o = out;
    while (pos < 32) {
        if (mask & 1) {
            if (flag == 0 || func_L22_003A48F8(pos) != 0) {
                *o = pos;
                o++;
                count++;
            }
        }
        pos++;
        mask >>= 1;
    }
    return count;
}

int func_L22_003A2D58(char *a0, int a1, int a2) {
    char *base = *(char **)(a0 + 0x68) + 0x2A0;
    int *slot = (int *)(base + a1 * 4);
    Rec_003A2D58 *rec;
    int n;
    rec = D_L22_001FE940[*slot];
    if (func_L22_003A2DF8(a2, rec->angle) != 0) return 0;
    rec = D_L22_001FE940[*slot];
    n = rec->count - 1;
    rec = (Rec_003A2D58 *)((char *)rec + n * 16);
    func_L22_003A2DF8(a2, rec->angle);
    return n;
}

/* Returns bit BIT of VALUE's raw pattern. */
int func_L22_003A2DF8(int bit, float value) {
    F32Bits bits;
    bits.f = value;
    return (bits.u >> bit) & 1;
}

void func_L22_003A2E10(char *a0, unsigned short a1) {
    char *p = *(char **)(a0 + 0x68);
    unsigned short old = *(unsigned short *)(p + 0x354);
    int i = (short)*(short *)(p + 0x35E);
    char *w;
    i = (i + 1) % 3;
    *(short *)(p + 0x35E) = (short)i;
    w = p + i * 2;
    *(unsigned short *)(w + 0x358) = old;
    *(unsigned short *)(p + 0x354) = a1;
}

INCLUDE_ASM("asm/levels", func_L22_003A2E50);



typedef struct PathState_003A36A0 {
    char pad[0x2A0];
    int ids[32];
    char pad320[0x20];
    int other_id;
} PathState_003A36A0;
typedef struct PathRec_003A36A0 {
    int count;
    int pad[3];
    Quad_003A2780 pts[1];
} PathRec_003A36A0;
extern Quad_003A2780 D_L22_001C00E0;
extern void func_L22_003A3CA0(char *);
extern int func_002A77E0(int);
extern void *func_00115484(void *, int, unsigned int);
extern void func_L22_003A3B50(void *, int);
extern void func_002AFFD8(void *, void *, int);
extern void func_00282D30(void *, void *, float);

/* Picks seven different points at random from the 32 paths in ST->ids and spawns an object at
 * each one, skipping five of the paths, then probes the points of the other_id path, spawning
 * once per hit, up to 27 times. CUMULATIVE[i] is the number of segments before path i. The two
 * `% 3` results are unused: retail keeps only their zero-divisor guards (divisor 3, `beql`,
 * `break 0,7`), so the dividend is a reconstruction. */
void func_L22_003A36A0(char *obj) {
    struct {
        int cumulative[32];
        int selected[7];
        Quad_003A2780 point, low, normal;
    } v;
    PathState_003A36A0 *st = *(PathState_003A36A0 **)(obj + 0x68);
    PathRec_003A36A0 *rec;
    int total = 0, i, j, next, unused, chosen, duplicate, n, spawned;
    Quad_003A2780 *pt;

    func_L22_003A3CA0(obj);
    func_002A77E0(3);
    func_00115484(v.selected, -1, 28);
    for (i = 0; i < 32; i++) {
        v.cumulative[i] = total;
        total += D_L22_001FE940[st->ids[i]]->count - 1;
    }
    for (i = 0; i < 7; i++) {
        duplicate = 1;
        do {
            chosen = func_002A77E0(total);
            for (j = 0; j < 7; j++) {
                if (chosen == v.selected[j]) {
                    duplicate = 1;
                    break;
                }
                if (j == 6) duplicate = 0;
            }
        } while (duplicate == 1);
        v.selected[i] = chosen;
        for (j = 0; j < 32; j = next) {
            if (j == 31) {
                qcopy_nc(&v.point, (char *)((chosen - v.cumulative[31]) * 16 + (int)D_L22_001FE940[st->ids[31]]) + 16);
                break;
            }
            next = j + 1;
            if (chosen >= v.cumulative[j] && chosen < v.cumulative[next]) {
                qcopy_nc(&v.point, (char *)((chosen - v.cumulative[j]) * 16 + (int)D_L22_001FE940[st->ids[j]]) + 16);
                break;
            }
        }
        if (j != 10 && j != 27 && j != 28 && j != 31 && j != 15) {
            unused = i % 3;
            func_L22_003A3B50(&v.point, 0);
        }
    }
    rec = (PathRec_003A36A0 *)D_L22_001FE940[st->other_id];
    n = rec->count;
    for (i = 0, spawned = 0; i < n && spawned < 27; i++) {
        unused = i % 3;
        pt = &rec->pts[i];
        func_002AFFD8(pt, &v.normal, 0);
        func_00282D30(&v.point, &v.normal, 6.0f);
        func_00282D30(&v.low, &v.normal, -3.0f);
        func_00282CC0(&v.point, &v.point, pt);
        func_00282CC0(&v.low, &v.low, pt);
        if (func_002761F8(&v.low, &v.point, 0x12, 0, 0)) {
            func_L22_003A3B50(&D_L22_001C00E0, 1);
            spawned++;
        }
    }
}

/* func_L22_003A3B50 is defined in L22_0039D708-g8.c. */

/* func_L22_003A3D50 is defined in L22_003A3D50-g8.c. */

/* func_L22_003A4128 is defined in L22_003A4128.c. */

/* func_L22_003A42E8 uses the profile in L22_003A42E8-g8.c. */


typedef struct Ctl_003A47E8 {
    int f0;
    int f4;
    int f8;
} Ctl_003A47E8;

extern Ctl_003A47E8 *D_L22_001B2AB4[1] __attribute__((sda));
extern void func_00316B30(void *, int, void *);
extern void func_0039CDC0(void *, void *, int, int, int);

/* Runs the two optional child passes while the controller's counter lasts. */
void func_L22_003A47E8(char *a0, int flags, int a2, int a3, int a4) {
    char *s0 = *(char **)(a0 + 0x68);
    char buf[16];
    if (D_L22_001B2AB4[0]->f8 == 0) return;
    if (flags & 1) {
        func_00316B30(*(void **)(s0 + 0x440), 3, buf);
        func_0039CDC0(*(void **)(s0 + 0x440), buf, a2, a3,
                      a4 != 0 || D_L22_001B2AB4[0]->f0 != 0);
    }
    if (flags & 2) {
        func_00316B30(*(void **)(s0 + 0x440), 4, buf);
        func_0039CDC0(*(void **)(s0 + 0x440), buf, a2, a3,
                      a4 != 0 || D_L22_001B2AB4[0]->f0 != 0);
    }
    D_L22_001B2AB4[0]->f8 = D_L22_001B2AB4[0]->f8 - 1;
}

/* func_L22_003A4938 is defined in L22_0039D708-scan.c. */

extern float D_L22_001B2AA4 __attribute__((sda));
extern char *D_0018C0B0;
extern void func_002F3AB0(void *, void *, void *);
extern void func_002F3D60(void *, void *, float);
extern void func_002F3AF0(void *, void *, float);
extern void func_002F3A80(void *, void *, void *);
extern void func_L22_003A2B90(void *, void *, void *, int);
extern void func_L22_003A2988(void *, float);

/* Builds the sub-object's three local matrices around its fixed offset and
 * hands the result to the placement helper. */
void func_L22_003A4EE8(char *a0) {
    char b0[16];
    char b1[16];
    char b2[16];
    char *o = *(char **)(a0 + 0x68);
    func_002F3AB0(b0, a0 + 0x10, o + 0x3F0);
    func_002F3D60(b0, b0, D_L22_001B2AA4 * 4.0f);
    func_002F3AF0(b1, D_0018C0B0 + 0xE0, -D_L22_001B2AA4 * 4.0f);
    func_002F3A80(b2, b0, b1);
    func_002F3AF0(b2, b2, 0.5f);
    func_002F3A80(b2, b2, o + 0x3F0);
    func_L22_003A2B90(a0, o + 0x3B0, b2, 1);
    func_L22_003A2988(o + 0x3B0, 0.0f);
    *(char **)(o + 0x3AC) = o + 0x3B0;
}

int func_L22_003A4FF8(void) {
    ref_L22_003A4FF8_00();
    return 1;
}
