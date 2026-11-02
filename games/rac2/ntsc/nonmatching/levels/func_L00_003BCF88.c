/* sol-high, USA v1.01, default game compiler with
 * -O2 -G0 -ffunction-sections -g -frac2-zero-ti-barrier:
 * 1412/1412 bytes, 0/27 placements, first difference +0x50.
 * Fourteen instructions differ only in f22/f23/f24 roles and save order:
 * retail UV step / reverse / midpoint = f22 / f23 / f24;
 * draft = f24 / f22 / f23. Other instructions agree after relocation.
 * Use in a separate unit with the flags above; all declarations are below.
 * Requalified the earlier sol-low draft using the current worker compiler.
 * Scalar initialization at first use and register storage tied the baseline.
 * Literal UV step: 1416 bytes; all three constants named: 1408 bytes;
 * separate named loop reverse/midpoint: 1396 bytes; step array: 1424 bytes.
 * Allocator evidence: step pseudo 91 has 7 refs, life 490, 38 calls;
 * loop reverse 188 has 5 refs, life 274, 21 calls; midpoint 259 has
 * 5 refs, life 276, 21 calls. Late step initialization retains these stats.
 * Next useful experiment needs different real expression/lifetime evidence;
 * do not repeat cosmetic scalar declarations. Per-function refs handle the
 * two level-dependent helpers, rather than assuming their L00 identities.
 */
#include "common.h"

extern float func_002A7878(float, float);
extern void func_00282DD8(void *, void *, void *);
extern void func_002830C0(void *, void *, void *);
extern float func_00283CE0(int);
extern int func_00283D38(int, int, float);
extern void func_00282D08(void *, void *, void *, float);
extern char D_L00_001B8DC0[];
typedef struct {
    float corner[4][4];
    int color[4];
    float uv[8];
    unsigned long long tags[4];
} RibbonPacket;

typedef int Rac2TI128 __attribute__((mode(TI)));
typedef struct { float x, y, z, w; } Vec16_Fc936841d;
extern long long func_002E67A0_2(int);
extern void ref_L00_003BCF88_00(Vec16_Fc936841d *, int);
extern void ref_L00_003BCF88_01(char *);
extern void func_802EE138_1(float *, float *, char *);
extern void func_802EE3D0_1(float *, float *, float);
extern void func_002EE178_1(char *, char *, float);
extern void func_802EAC30_1(char *, float *, int);
void func_L00_003BCF88(int unused, int width, Vec16_Fc936841d *points, int count)
{
    RibbonPacket a, b;
    float matrix[4][4] __attribute__((aligned(16)));
    float inner[4] __attribute__((aligned(16)));
    float outer[4] __attribute__((aligned(16)));
    int i;
    int outer_color = 0x400072FF;
    unsigned long long texture_state = 0xFF9000000260ULL;
    float step = 0.33f;
    float t, u, v;
    a.tags[1] = func_002E67A0_2(43);
    a.tags[2] = texture_state;
    a.tags[3] = ((unsigned long long)(unsigned int)width << 32) | 0x68;
    a.tags[0] = 0;
    a.color[0] = a.color[1] = 0x7F0072FF;
    b.tags[1] = func_002E67A0_2(43);
    b.tags[2] = texture_state;
    b.tags[0] = 0;
    b.tags[3] = ((unsigned long long)(unsigned int)(width / 3) << 32) | 0x68;
    b.color[0] = b.color[1] = outer_color;
    *(Rac2TI128 *)inner = 0;
    *(Rac2TI128 *)outer = 0;
    inner[3] = 1.0f;
    outer[3] = 1.0f;
    inner[2] = func_002A7878(0.005f, 0.085f) * 0.25f;
    outer[2] = func_002A7878(0.1f, 0.25f) * 0.25f;
    ref_L00_003BCF88_00(points, count);
    ref_L00_003BCF88_01((char *)&a);
    ref_L00_003BCF88_01((char *)&b);
    a.uv[0] = 0; a.uv[1] = 0; a.uv[2] = 0; a.uv[3] = 1;
    a.uv[4] = 1; a.uv[5] = 0; a.uv[6] = 1; a.uv[7] = 1;
    b.uv[0] = 0; b.uv[1] = 0; b.uv[2] = 0; b.uv[3] = 1;
    b.uv[4] = step; b.uv[5] = 0; b.uv[6] = step; b.uv[7] = 1;
    func_802EE138_1(matrix[0], (float *)&points[1], (char *)points);
    func_802EE3D0_1(matrix[0], matrix[0], 1.0f);
    func_802EE138_1(matrix[1], (float *)D_L00_001B8DC0, (char *)points);
    func_00282DD8(matrix[2], matrix[1], matrix[0]);
    func_802EE3D0_1(matrix[2], matrix[2], 1.0f);
    func_00282DD8(matrix[1], matrix[0], matrix[2]);
    qcopy(matrix[3], points);
    qcopy(a.corner[0], inner);
    func_002EE178_1((char *)a.corner[1], (char *)a.corner[0], -1.0f);
    func_002830C0(a.corner[0], a.corner[0], matrix[0]);
    func_002830C0(a.corner[1], a.corner[1], matrix[0]);
    qcopy(b.corner[0], outer);
    func_002EE178_1((char *)b.corner[1], (char *)b.corner[0], -1.0f);
    func_002830C0(b.corner[0], b.corner[0], matrix[0]);
    func_002830C0(b.corner[1], b.corner[1], matrix[0]);
    for (i = 1; i < count - 1; i++) {
        qcopy(a.corner[2], inner);
        func_002EE178_1((char *)a.corner[3], (char *)a.corner[2], -1.0f);
        qcopy(b.corner[2], outer);
        func_002EE178_1((char *)b.corner[3], (char *)b.corner[2], -1.0f);
        func_802EE138_1(matrix[0], (float *)&points[i + 1], (char *)&points[i]);
        func_802EE3D0_1(matrix[0], matrix[0], 1.0f);
        func_802EE138_1(matrix[1], (float *)D_L00_001B8DC0, (char *)&points[i]);
        func_00282DD8(matrix[2], matrix[1], matrix[0]);
        func_802EE3D0_1(matrix[2], matrix[2], 1.0f);
        func_00282DD8(matrix[1], matrix[0], matrix[2]);
        qcopy(matrix[3], &points[i]);
        func_002830C0(a.corner[2], a.corner[2], matrix[0]);
        func_002830C0(a.corner[3], a.corner[3], matrix[0]);
        func_002830C0(b.corner[2], b.corner[2], matrix[0]);
        func_002830C0(b.corner[3], b.corner[3], matrix[0]);
        t = func_00283CE0(i);
        u = func_00283CE0(count);
        t = t / (u - 2.0f);
        a.color[2] = a.color[3] = func_00283D38(0x7F0072FF, 0x7FDDB973, t);
        b.color[2] = b.color[3] = func_00283D38(0x7F0072FF, 0x40FF0000, t);
        if (i == count - 2) {
            func_00282D08(a.corner[2], a.corner[2], a.corner[3], 0.5f);
            func_00282D08(b.corner[2], b.corner[2], b.corner[3], 0.5f);
            qcopy(a.corner[3], a.corner[2]);
            qcopy(b.corner[3], b.corner[2]);
        }
        func_802EAC30_1((char *)&a, 0, 0);
        func_802EAC30_1((char *)&b, 0, 0);
        u = func_00283CE0(i) * step;
        v = u + step;
        if (u >= 1.0f) { u -= 1.0f; v -= 1.0f; }
        a.color[0] = a.color[1] = a.color[2];
        b.color[0] = b.color[1] = b.color[2];
        b.uv[2] = b.uv[0] = u;
        b.uv[6] = b.uv[4] = v;
        qcopy(a.corner[0], a.corner[2]);
        qcopy(a.corner[1], a.corner[3]);
        qcopy(b.corner[0], b.corner[2]);
        qcopy(b.corner[1], b.corner[3]);
    }
}
