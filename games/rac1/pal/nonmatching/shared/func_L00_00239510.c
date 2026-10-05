/* NON_MATCHING func_L00_00239510 -- src/overlays/shared/hud_00235960.c
 * Best so far: BYTES 12/1028 (98.8% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
int func_001F9850(int);
extern short D_L00_0015F80C;
s32 func_001F9850_39510b(s32) __asm__("func_001F9850");
extern short D_L00_0015F80C_39510b __asm__("D_L00_0015F80C");
int func_001F9850_39510c(int) __asm__("func_001F9850");
typedef struct { int f0, f4, f8, fC, f10, f14, f18; } Ent;
typedef struct {
    u8 p0[0x64]; int w64; u8 p68[4]; int w6C; u8 b70; u8 b71; u8 p72[2]; int w74; int w78; int w7C;
} Obj;
typedef struct { u8 p0[0x148]; float f148; float f14C; u8 p150[0x70]; int w1C0; int w1C4; u8 p1C8[4]; int w1CC; } G_t;
extern G_t D_0013CA40;
extern int D_L00_0017E604;
extern short D_L00_0015FB48_39510 __asm__("D_L00_0015FB48");
extern short D_L00_0015F860;
extern short D_L00_0015FB54;
extern Ent ** D_L00_0015FB78_39510 __asm__("D_L00_0015FB78") MACRO_ADDR;
extern int D_L00_0017DD38;
extern int D_L00_0017DD3C;
extern int D_L00_0017DD40;
extern int D_L00_0017DD44;
extern int D_00141710[];
extern int D_L00_0015FB50 MACRO_ADDR;
extern int D_L00_0015FB4C MACRO_ADDR;
extern int D_L00_0015F4F8 MACRO_ADDR;
extern int func_001F9850_39510d(int) __asm__("func_001F9850");
extern float func_001F9B50(float);
extern float func_L00_001FF860(float, float);
extern int func_001FFCB0(int);
int func_001F9850_39510e(int) __asm__("func_001F9850");
s32 func_001F9850_39510f(s32) __asm__("func_001F9850");
extern int func_001F9850_39510g(int) __asm__("func_001F9850");

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/overlays/shared/ui_text_002377b8.c, FUN_L00_00238b80. */
void func_L00_00239510(Obj *o) {
    float x, y, len, ang;
    int old, cur, n, t;
    u8 *c = &o->b70;

    o->w7C = func_001F9850_39510g(0xB4);
    o->w6C = 0x18;
    D_0013CA40.w1CC = 2;
    x = D_0013CA40.f148;
    y = D_0013CA40.f14C;
    len = func_001F9B50(x * x + y * y);
    if (len != 0.0f) {
        x /= len;
        y /= len;
    }
    ang = func_L00_001FF860(x, y);
    if ((o->w78 >> 24) == 0 && D_L00_0017E604 != 0) {
        if ((D_0013CA40.w1C4 & 0xF000) || len < 0.5f || --o->w78 == -1) {
            o->w74 = -1;
            o->w78 = 0x10000FF;
        }
    }
    old = o->w74;
    if (*((s8 *)o + 0x7B) == 1) {
        if (0.9f < len) {
            n = (*(int *)&D_L00_0015FB48_39510);
            o->w74 = (int)((ang + 3.1415927f + 3.1415927f + 1.5707964f + 3.1415927f / (float)n) * ((float)n / 6.2831855f)) % n;
        }
        if (D_0013CA40.w1C4 & 0xF000) {
            cur = o->w74;
            if (D_0013CA40.w1C4 & 0x1000) {
                if (cur == -1) t = D_L00_0017DD40; else t = D_L00_0015FB78_39510[(*(int *)&D_L00_0015F80C_39510b)][cur].f10;
                cur = t;
            }
            if (D_0013CA40.w1C4 & 0x4000) {
                if (cur == -1) t = D_L00_0017DD44; else t = D_L00_0015FB78_39510[(*(int *)&D_L00_0015F80C_39510b)][cur].f14;
                cur = t;
            }
            if (D_0013CA40.w1C4 & 0x8000) {
                if (cur == -1) t = D_L00_0017DD38; else t = D_L00_0015FB78_39510[(*(int *)&D_L00_0015F80C_39510b)][cur].f8;
                cur = t;
            }
            if (D_0013CA40.w1C4 & 0x2000) {
                if (cur == -1) t = D_L00_0017DD3C; else t = D_L00_0015FB78_39510[(*(int *)&D_L00_0015F80C_39510b)][cur].fC;
                cur = t;
            }
            o->w74 = cur;
        }
    }
    if (o->w74 != old) {
        c[1] = 0;
    } else if (c[1] < (*(int *)&D_L00_0015F860)) {
        c[1]++;
    }
    if (!(D_0013CA40.w1C0 & 0x10)) {
        if (o->w74 == -1) D_00141710[0] = 0;
        else D_00141710[0] = D_L00_0015FB78_39510[(*(int *)&D_L00_0015F80C_39510b)][o->w74].f18;
        if (c[0] != 0) c[0]--;
    } else if (c[0] < (*(int *)&D_L00_0015F860)) {
        c[0]++;
    }
    if (c[0] == 0) {
        { int a = o->w78; int b = o->w74; (*(int *)&D_L00_0015FB54) = b; D_L00_0015FB50 = a; }
        func_001FFCB0(o->w64);
        o->w6C = -6;
        D_L00_0015FB4C = D_L00_0015F4F8;
    }
}
