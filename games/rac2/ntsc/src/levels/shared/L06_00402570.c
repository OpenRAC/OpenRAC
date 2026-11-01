#include "common.h"

typedef struct { char pad[0x228]; float f228, f22C; } WaterFog;
typedef struct { char pad0[0x348]; short f348; char pad34A[0x1F4A]; int currentState; } WaterHero;
typedef struct { char pad[0x140]; float x, y, z; } WaterCamera;
typedef struct {
    int color0, color1, color2, color3, color4;
    float f14, range;
    int texture;
    int f20;
    float f24, reach, f2C;
} WaterState;

extern WaterFog D_L06_001BDD40;
extern WaterHero D_00189E20;
extern WaterCamera D_L06_001B9700;
extern int D_L06_001A8A48 MACRO_ADDR;
extern int D_001A79F0 __attribute__((sda));
extern float D_L06_001B3290[3][4] __attribute__((sda));
extern float D_L06_001B9A70[][4];
extern float D_L06_001B3280 MACRO_ADDR;
extern float D_L06_001B0060[] SDATA(D_L06_001B0060);
extern float D_L06_001B3270[][2] MACRO_ADDR;
extern float D_L06_0023E5A0[][3], D_L06_0023E9A0[][3];
extern int D_L06_0023E8A0[], D_L06_0023ECA0[];
extern float D_L06_0023EDA0[][2], D_L06_0023EFA0[][2];

extern void func_002FC560(int, unsigned long);
extern unsigned long func_0027C878(int);
extern void func_00280328(void);
extern void func_00282DD8(void *, void *, void *);
extern void func_00283098(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern float func_00283CE0(int);
extern int func_00283D38(int, int, float);
extern void func_00282D30(void *, void *, float);
extern void func_00281810(int, void *, void *, void *, int);
extern float func_00282E20(void *);
extern void func_00282A28(void);

/* Draws the level's water sheet: a fading edge strip and two scrolling layers of grid cells
 * around the camera. RAC1's draw_pokitaru_ocean is its counterpart. */
void func_L06_00402570(unsigned char *owner)
{
    float a[4], b[4], x[4], y[4], corner[4][4], dx[4], dy[4];
    WaterState *state;
    int k;
    float saved22C;
    float saved228;
    int i, count, color;
    int c0, c1;
    float range, step, zstep, xx, yy, t;

    saved228 = D_L06_001BDD40.f228;
    state = *(WaterState **)(owner + 0x68);
    saved22C = D_L06_001BDD40.f22C;
    if (D_00189E20.currentState != 28 && !D_L06_001A8A48) {
        D_L06_001BDD40.f228 = 255.0f;
        D_L06_001BDD40.f22C = 249.9f;
    }
    func_002FC560(0x42, 0x3200000044UL);
    func_002FC560(8, 0);
    func_002FC560(0x14, 0xFF9000000260UL);
    if (state->color0 < 0)
        func_002FC560(0x47, 0x5310B);
    else
        func_002FC560(0x47, 0x513F1);
    func_002FC560(6, func_0027C878(22));
    func_00280328();
    if (D_001A79F0 == 13 || (D_001A79F0 == 16 && D_00189E20.f348 == 1)) {
        qcopy(D_L06_001B3290[1], D_L06_001B9A70[1]);
        D_L06_001B3290[2][0] = 0;
        D_L06_001B3290[2][1] = 0;
        D_L06_001B3290[2][2] = 1;
        D_L06_001B3290[2][3] = 0;
        func_00282DD8(D_L06_001B3290[0], D_L06_001B3290[2], D_L06_001B3290[1]);
        x[0] = 1;
        x[1] = 0;
        x[2] = 0;
        x[3] = 1;
        func_00283098(x, x, D_L06_001B3290);
        y[0] = 0;
        y[1] = 1;
        y[2] = 0;
        y[3] = 1;
        func_00283098(y, y, D_L06_001B3290);
    } else {
        x[0] = 1;
        x[1] = 0;
        x[2] = 0;
        x[3] = 1;
        func_00283098(x, x, D_L06_001B9A70);
        y[0] = 0;
        y[1] = 1;
        y[2] = 0;
        y[3] = 1;
        func_00283098(y, y, D_L06_001B9A70);
    }
    if (D_001A79F0 != 16) {
        x[2] = 0;
        func_00282F20(x, x, 1);
        y[2] = 0;
        func_00282F20(y, y, 1);
    } else if (D_00189E20.f348 == 1) {
        x[0] = 0;
        x[1] = 1;
        x[2] = 0;
        x[3] = 1;
        y[0] = -0.79706f;
        y[1] = 0;
        y[2] = -0.60389f;
        y[3] = 1.00405f;
    }
    if (D_001A79F0 != 16) {
        func_00282F20(a, x, -75);
        func_00282F20(b, y, -87);
        func_00282CC0(corner[0], a, b);
        func_00282F20(b, y, 87);
        func_00282CC0(corner[2], a, b);
        func_00282F20(a, x, state->reach);
        func_00282F20(b, y, -750);
        func_00282CC0(corner[1], a, b);
        func_00282F20(b, y, 750);
        func_00282CC0(corner[3], a, b);
    } else {
        func_00282F20(a, x, -150);
        func_00282F20(b, y, -175);
        func_00282CC0(corner[0], a, b);
        func_00282F20(b, y, 175);
        func_00282CC0(corner[2], a, b);
        func_00282F20(a, x, state->reach);
        func_00282F20(b, y, -1500);
        func_00282CC0(corner[1], a, b);
        func_00282F20(b, y, 1500);
        func_00282CC0(corner[3], a, b);
    }
    for (i = 0; i < 4; i++) {
        float *p = corner[i];
        func_00282CC0(p, p, &D_L06_001B9700.x);
        p[2] = D_L06_001B3280;
    }
    func_00282CF0(dx, corner[1], corner[0]);
    func_00282CF0(dy, corner[3], corner[2]);
    t = 10;
    for (i = 0; i < 12; i += 2) {
        float f = func_00283CE0(i) * 0.1f;
        color = func_00283D38(state->color0, state->color1, func_00283CE0(i) / t);
        func_00282D30(a, dx, f);
        D_L06_0023E8A0[i] = color;
        D_L06_0023EDA0[i][0] = 0;
        D_L06_0023EDA0[i][1] = 0;
        func_00282CC0(a, a, corner[0]);
        D_L06_0023E5A0[i][0] = a[0];
        D_L06_0023E5A0[i][1] = a[1];
        D_L06_0023E5A0[i][2] = a[2];
        func_00282D30(a, dy, f);
        D_L06_0023E8A0[i + 1] = color;
        D_L06_0023EDA0[i + 1][0] = 0;
        D_L06_0023EDA0[i + 1][1] = 0;
        func_00282CC0(a, a, corner[2]);
        D_L06_0023E5A0[i + 1][0] = a[0];
        D_L06_0023E5A0[i + 1][1] = a[1];
        D_L06_0023E5A0[i + 1][2] = a[2];
    }
    func_00281810(12, D_L06_0023E5A0, D_L06_0023E8A0, D_L06_0023EDA0, 1);
    func_002FC560(0x42, 0x8000000048UL);
    func_002FC560(8, 0);
    func_002FC560(0x14, 0xFF9000000260UL);
    func_002FC560(0x47, 0x513F1);
    c0 = state->color3;
    c1 = state->color4;
    range = state->range;
    if (D_00189E20.currentState == 28 || D_L06_001A8A48) {
        if (range > 32)
            range = 32;
    }

    for (k = 0; k < 2; k++) {
        int id = k + 63;
        func_002FC560(6, func_0027C878(state->texture + id));
        for (yy = range * -2; yy < range * 2; yy += range) {
            count = 0;
            step = D_L06_001B9700.z - D_L06_001B3280;
            if (step < 10)
                step = 10;
            zstep = step * range * 0.00002f;
            if (zstep < 0.125f)
                zstep = 0.125f;
            for (xx = range * -2; xx < range * 2; xx += range) {
                D_L06_0023EFA0[count][0] = xx / (range * 2) * D_L06_001B0060[k] + D_L06_001B3270[k][0];
                D_L06_0023EFA0[count][1] = yy / (range * 2) * D_L06_001B0060[k] + D_L06_001B3270[k][1];
                D_L06_0023E9A0[count][0] = xx + D_L06_001B9700.x + 1;
                D_L06_0023E9A0[count][1] = yy + D_L06_001B9700.y + 1;
                D_L06_0023E9A0[count][2] = D_L06_001B3280 + zstep;
                i = count + 1;
                D_L06_0023EFA0[i][0] = xx / (range * 2) * D_L06_001B0060[k] + D_L06_001B3270[k][0];
                D_L06_0023EFA0[i][1] = (yy + range) / (range * 2) * D_L06_001B0060[k] + D_L06_001B3270[k][1];
                D_L06_0023E9A0[i][0] = xx + D_L06_001B9700.x + 1;
                D_L06_0023E9A0[i][1] = (yy + range) + D_L06_001B9700.y + 1;
                D_L06_0023E9A0[i][2] = D_L06_001B3280 + zstep;
                i = count + 2;
                D_L06_0023EFA0[i][0] = (xx + range) / (range * 2) * D_L06_001B0060[k] + D_L06_001B3270[k][0];
                D_L06_0023EFA0[i][1] = yy / (range * 2) * D_L06_001B0060[k] + D_L06_001B3270[k][1];
                D_L06_0023E9A0[i][0] = (xx + range) + D_L06_001B9700.x + 1;
                D_L06_0023E9A0[i][1] = yy + D_L06_001B9700.y + 1;
                D_L06_0023E9A0[i][2] = D_L06_001B3280 + zstep;
                i = count + 3;
                D_L06_0023EFA0[i][0] = (xx + range) / (range * 2) * D_L06_001B0060[k] + D_L06_001B3270[k][0];
                D_L06_0023EFA0[i][1] = (yy + range) / (range * 2) * D_L06_001B0060[k] + D_L06_001B3270[k][1];
                D_L06_0023E9A0[i][0] = (xx + range) + D_L06_001B9700.x + 1;
                D_L06_0023E9A0[i][1] = (yy + range) + D_L06_001B9700.y + 1;
                D_L06_0023E9A0[i][2] = D_L06_001B3280 + zstep;
                for (i = count; i < count + 4; i++) {
                    a[0] = D_L06_0023E9A0[i][0] - D_L06_001B9700.x;
                    a[1] = D_L06_0023E9A0[i][1] - D_L06_001B9700.y;
                    a[2] = D_L06_0023E9A0[i][2] - D_L06_001B9700.z;
                    a[3] = 1;
                    t = func_00282E20(a) / (range * 2);
                    if (t > 1)
                        t = 1;
                    D_L06_0023ECA0[i] = func_00283D38(c0, c1, t);
                }
                count += 4;
                if (count >= 64)
                    func_00282A28();
            }
            func_00281810(count, D_L06_0023E9A0, D_L06_0023ECA0, D_L06_0023EFA0, 1);
        }
    }
    D_L06_001BDD40.f228 = saved228;
    D_L06_001BDD40.f22C = saved22C;
    func_002FC560(0x47, 0x5360B);
}
