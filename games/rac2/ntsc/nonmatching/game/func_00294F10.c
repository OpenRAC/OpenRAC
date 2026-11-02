/* GPT-6.1 Sol (high effort), task v0859, USA v1.01.
 * Best: 1444/1452 bytes, 0/28 places. Profile: -O2 -G0 -ffunction-sections -g.
 * Separate pointers into the same endpoint/coefficient storage reproduce the
 * second loop's twelve-pointer topology; MACRO_ADDR preserves absolute fields,
 * and SDATA binds the two measured GP bases without independent scalar aliases.
 * Scoped loop indices recover the retail countdown registers. Reordering the
 * first loop's real stores selects its +0x15C destination induction base.
 * Separate division source lines plus -g recover all four pre-division nop pairs.
 * Remaining: coefficient FPR allocation/load scheduling, resource addressing,
 * allocation lifetimes (six saved GPRs versus seven), and mask/tail scheduling.
 * Forward flat arrays alone: 1300 bytes; field pointers: 1380; MACRO_ADDR: 1412;
 * -g: 1444. Typed resource-directory control: 1420; disabling sched1: 1440,
 * neither was closer. Next: qualify coefficient load/evaluation ordering and
 * the resource-directory owner before further allocator tuning.
 * All declarations needed by this standalone draft are included. Not linked.
 */
#include "common.h"
extern int func_00294EC8(int);
extern int D_001C4FA0[];
extern char D_0014B540_1[];
extern int D_001A79F0_3 __attribute__((sda));
typedef struct { float x0, y0, x1, y1, u0, v0, u1, v1; } UiMapEndpoints;
typedef struct { float x_bias, x_scale, y_bias, y_scale; } UiMapCoefficients;
extern int D_00256028[][2];
extern float D_00255C30[];
extern float D_00255ED0[];
extern float D_001A93D0[] MACRO_ADDR;
extern float D_001A93D0_gp[] SDATA(D_001A93D0);
extern float D_001A9428[] MACRO_ADDR;
extern float D_001A9428_gp[] SDATA(D_001A9428);
extern unsigned char D_001A7AFD;
extern char *D_001BAED0;
extern char D_001A95A0[], D_001A95B8[];
extern unsigned char D_0018D278[];
extern void func_00295C50(int);
extern int func_002B7C80(char *, int, int);
extern int func_002EED58(int, int, int, int);
extern int func_0029D4D0(char *, char *);
/* Retail retains the allocator's source-file and line slots, although its
   release implementation consumes only the byte count. */
extern char *func_0028B2B8(int, int, const char *, int);
extern void func_00282B50(void *, void *, int);
extern void func_00297730(char *, unsigned char *, int);
extern void func_00297400(u8 *, u8 *, short *);

void func_00294F10(void)
{
    int i, j;
    char *scratch, *packed;
    int bytes, size;
    D_001C4FA0[4] = 1;
    for (i = 0; i < 21; i++) {
        int index = i;
        D_001C4FA0[0x108 / 4 + index] = D_00256028[index][0];
        ((float *)D_001C4FA0)[0xB4 / 4 + index] = 0.65f;
        D_001C4FA0[0x15C / 4 + index] = D_00256028[index][1];
    }
    D_001C4FA0[0x238 / 4] = -1;
    func_00295C50(D_001A79F0_3);
    { int k;
    for (k = 0; k < 21; k++) {
        float x = D_00255C30[k * 8];
        float dx = x - D_00255C30[k * 8 + 2];
        float u = D_00255C30[k * 8 + 4];
        float du = u - D_00255C30[k * 8 + 6];
        float y = D_00255C30[k * 8 + 1];
        float dy = y - D_00255C30[k * 8 + 3];
        float v = D_00255C30[k * 8 + 5];
        float dv = v - D_00255C30[k * 8 + 7];
        float sx = du / dx;
        float sy = dv / dy;
        D_00255ED0[k * 4 + 1] = sx;
        D_00255ED0[k * 4 + 3] = sy;
        D_00255ED0[k * 4] = u - sx * x;
        D_00255ED0[k * 4 + 2] = v - sy * y;
    } }
    {
        float *yb = D_001A9428 + 2, *ys = D_001A9428 + 3;
        float *xb = D_001A9428_gp, *xs = D_001A9428 + 1;
        float *v1 = D_001A93D0 + 7, *u1 = D_001A93D0 + 6;
        float *v0 = D_001A93D0 + 5, *u0 = D_001A93D0 + 4;
        float *y1 = D_001A93D0 + 3, *x1 = D_001A93D0 + 2;
        float *y0 = D_001A93D0 + 1, *x0 = D_001A93D0_gp;
        int k;
        for (k = 0; k < 2; k++) {
            float a = *x0, c = *x1, e = *u0, g = *u1;
            float b = *y0, d = *y1, f = *v0, h = *v1;
            float sx = (e - g) / (a - c);
        float sy = (f - h) / (b - d);
            *xs = sx;
            *xb = e - sx * a;
            *ys = sy;
            *yb = f - sy * b;
            x0 += 8; y0 += 8; x1 += 8; y1 += 8;
            u0 += 8; v0 += 8; u1 += 8; v1 += 8;
            xb += 4; xs += 4; yb += 4; ys += 4;
        }
    }
    if (((int *)D_0014B540_1)[(0x14F4 / 4) + D_001C4FA0[0x230 / 4] * 2] == 0 || D_001A7AFD) {
        if (((int *)D_0014B540_1)[(0x15D4 / 4) + D_001C4FA0[0x230 / 4] * 2] == 0 || !D_001A7AFD) {
            D_001C4FA0[0] = 0;
            D_001C4FA0[0x28 / 4] = 0;
            D_001C4FA0[2] = 0;
            D_001C4FA0[3] = 0;
            D_001C4FA0[1] = 0;
            return;
        }
    }
    D_001C4FA0[0x28 / 4] = 1;
    if (D_001A7AFD)
        bytes = ((int *)D_0014B540_1)[0x15D4 / 4 + D_001C4FA0[0x230 / 4] * 2];
    else
        bytes = ((int *)D_0014B540_1)[0x14F4 / 4 + D_001C4FA0[0x230 / 4] * 2];
    bytes <<= 7;
    scratch = D_001BAED0;
    packed = scratch + 0x9A800;
    if (D_001A7AFD) {
        func_002B7C80(packed,
            ((int *)D_0014B540_1)[0x15D0 / 4 + D_001C4FA0[0x230 / 4] * 2] + ((int *)D_0014B540_1)[0x36C / 4],
            ((int *)D_0014B540_1)[0x15D4 / 4 + D_001C4FA0[0x230 / 4] * 2]);
        D_001C4FA0[0x23C / 4] = 1;
    } else {
        func_002B7C80(packed,
            ((int *)D_0014B540_1)[0x14F0 / 4 + D_001C4FA0[0x230 / 4] * 2] + ((int *)D_0014B540_1)[0x36C / 4],
            ((int *)D_0014B540_1)[0x14F4 / 4 + D_001C4FA0[0x230 / 4] * 2]);
        D_001C4FA0[0x23C / 4] = 0;
    }
    D_001C4FA0[0x238 / 4] = func_002EED58((int)packed, bytes, 0x3C00, (int)D_001A95A0);
    D_001C4FA0[0x240 / 4] = bytes;
    func_0029D4D0(packed, scratch);
    size = ((int *)scratch)[1] - ((int *)scratch)[0];
    D_001C4FA0[0] = (int)func_0028B2B8(size, 0, D_001A95B8, 0x2EB);
    func_00282B50((void *)D_001C4FA0[0], scratch + ((int *)scratch)[0], size);
    D_001C4FA0[0] += 8;
    D_001C4FA0[1] = D_001C4FA0[0];
    size = ((int *)scratch)[2] - ((int *)scratch)[1];
    D_001C4FA0[5] = (int)func_0028B2B8(size, 0, D_001A95B8, 0x2FC);
    func_00282B50((void *)D_001C4FA0[5], scratch + ((int *)scratch)[1], size);
    D_001C4FA0[2] = (int)func_0028B2B8(0x1000, 0, D_001A95B8, 0x303);
    for (i = 0; i < 8; i++) {
        D_001C4FA0[0x30 / 4 + i * 4] = -1;
        D_001C4FA0[0x34 / 4 + i * 4] = -1;
        D_001C4FA0[0x38 / 4 + i * 4] = D_001C4FA0[2] + (i << 9);
    }
    D_001C4FA0[3] = (int)func_0028B2B8(0x8000, 0, D_001A95B8, 0x30C);
    if (func_00294EC8(D_001C4FA0[0x230 / 4]))
        func_00297730((char *)D_001C4FA0[3], D_0018D278 + (D_001C4FA0[0x230 / 4] << 11), D_001C4FA0[5]);
    else
        func_00297400((u8 *)D_001C4FA0[3], (u8 *)D_001C4FA0[0], (short *)D_001C4FA0[1]);
    for (i = 0; i < 32; i++) {
        float dy = 15.5f - (float)i;
        float dy2 = dy * dy;
        for (j = 0; j < 32; j++) {
            float dx = 15.5f - (float)j;
            if (dx * dx + dy2 > 256.0f)
                ((u8 *)D_001C4FA0)[0x1B0 + i * 4 + j / 8] |= 1 << (j % 8);
        }
    }
}
