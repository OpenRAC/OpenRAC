/* NON_MATCHING func_L00_0028AF90 -- src/overlays/shared/shrubproc_0028A198.c
 * Best so far: BYTES 24/1360 (98.2% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
void func_001FA190();
void func_001FA190_8AF90b(void *) __asm__("func_001FA190");
void func_0022CEB8(void);
typedef union {
    struct { s16 h0; s16 h2; } h;
    struct { u8 c0; u8 c1; u8 c2; u8 c3; } c;
    u32 w;
} PU;
typedef struct {
    u8 type;
    u8 b1;
    u8 b2;
    u8 b3;
    u32 col;
    f32 f8;
    PU u;
    f32 f10;
    f32 f14;
    f32 f18;
    f32 f1C;
} Part;
typedef struct {
    u8 pad0[4];
    s16 h4;
    u8 pad6[2];
    s16 count;
    u8 padA[0x1C - 0xA];
    Part *parts;
} Sys;
extern Sys * D_L00_001605DC_8AF90 __asm__("D_L00_001605DC") MACRO_ADDR;
extern short D_L00_001605C0;
extern short D_L00_001605B0;
extern u8 D_L00_001BDB70_8AF90[] __asm__("D_L00_001BDB70");
extern u8 D_L00_001BDB40[];
extern void func_001FA190_8AF90c(void *) __asm__("func_001FA190");
extern void func_0022C9A8(s32);
extern s32 func_001160D8(void);
extern f32 func_00214158(void);
extern s32 func_002140B0(s32);
extern f32 func_001FA888(s32);
extern f32 func_001F9F90(f32);
extern f32 func_001F9FA8(f32);
extern void func_0022CEB8_8AF90b(void) __asm__("func_0022CEB8");
extern void func_00234C98_8AF90(s32, u64) __asm__("func_00234C98");
void func_001FA190_8AF90d(void *) __asm__("func_001FA190");
void func_0022CEB8_8AF90c(void) __asm__("func_0022CEB8");
void func_001FA190_8AF90e(void *) __asm__("func_001FA190");
void func_001FA190_8AF90f(void *) __asm__("func_001FA190");
extern void func_001FA190_8AF90g(void *) __asm__("func_001FA190");
extern void func_0022CEB8_8AF90d(void) __asm__("func_0022CEB8");
void func_001FA190_8AF90h(void *) __asm__("func_001FA190");
void func_001FA190_8AF90i(void *) __asm__("func_001FA190");
void func_0022CEB8_8AF90e(void) __asm__("func_0022CEB8");
void func_001FA190_8AF90j(void *) __asm__("func_001FA190");

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/overlays/shared/unclassified_00288ec0.c, FUN_L00_00289cb8. */
void func_L00_0028AF90(void) {
    Part *p;
    Part *p2;
    PU *q2;
    s32 i2;
    s32 k2;
    PU *q;
    s32 i;
    s32 k;
    s32 j;
    s32 r;
    s32 r1;
    s32 r2;
    f32 a;
    f32 b;

    D_L00_001605DC_8AF90->h4 = 0;
    func_001FA190_8AF90j(D_L00_001BDB70_8AF90);
    func_0022C9A8(0);
    if (D_L00_001605DC_8AF90->count == 0) {
        D_L00_001605DC_8AF90->count = 0x100;
        for (i = 0; i < D_L00_001605DC_8AF90->count; i++) {
            p = &D_L00_001605DC_8AF90->parts[i];
            if (i >= 0xF0) {
                p->type = 0;
                q = &p->u;
                q->h.h0 = func_001160D8() >> 16;
                q->h.h2 = func_001160D8() >> 16;
                p->b3 = 0x48;
                p->b2 = 1;
                p->f1C = 0.18f;
                p->col = (((u32 *)&D_L00_001605C0))[(func_001160D8() >> 16) & 3];
            } else if (i >= 0xEA) {
                r = i - 0xEA;
                p->b2 = 2;
                k = r >> 1;
                p->b3 = 0x48;
                j = r & 1;
                p->f8 = func_00214158();
                p->type = 2;
                p->col = (((u32 *)&D_L00_001605B0))[k];
                qcopy(&p->f10, D_L00_001BDB40 + k * 16);
                p->u.c.c0 = k;
                {
                    PU *qq = &p->u;
                    qq->c.c1 = j;
                    qq->c.c2 = j * 0x18;
                }
            } else {
                p->type = 3;
                p->u.h.h0 = func_002140B0(0x100);
                p->b2 = (func_001160D8() >> 16) & 1;
                p->b3 = 0x48;
                p->f8 = func_00214158();
                p->f1C = func_001FA888(func_002140B0(0x30) + 0x20) * 0.00390625f;
                a = func_00214158();
                b = func_00214158();
                p->f10 = func_001F9F90(a) * func_001F9FA8(b) * 50.0f;
                p->f14 = func_001F9FA8(a) * func_001F9FA8(b) * 50.0f;
                p->f18 = func_001F9F90(b) * 50.0f;
                r1 = func_002140B0(0x18) + 8;
                r2 = func_002140B0(0x20) << 24;
                if ((func_001160D8() >> 16) & 1) {
                    { u32 t = (r1 << 16) + 0x30505050; p->u.w = r2 + t; }
                } else {
                    { u32 t = (r1 << 8) + 0x30505050; p->u.w = (r2 + t) | r1; }
                }
            }
        }
    }
    for (i2 = 0; i2 < D_L00_001605DC_8AF90->count; i2++) {
        p2 = &D_L00_001605DC_8AF90->parts[i2];
        if (p2->type == 0) {
            q2 = &p2->u;
            p2->u.h.h0++;
            q2->h.h2++;
            a = func_001FA888((p2->u.h.h0 & 0xFFF) - 0x800) * 0.0015339808f;
            b = func_001FA888((q2->h.h2 & 0xFFF) - 0x800) * 0.0015339808f;
            p2->f10 = func_001F9F90(a) * func_001F9FA8(b) * 50.0f;
            p2->f14 = func_001F9FA8(a) * func_001F9FA8(b) * 50.0f;
            p2->f18 = func_001F9F90(b) * 50.0f;
            p2->col &= 0xFFFFFF;
            if (((u16)p2->u.h.h0 & 0x3F) < 8) {
                p2->col |= 0x70000000;
            } else {
                p2->col |= 0x24000000;
            }
        } else if (p2->type == 2) {
            q2 = &p2->u;
            q2->c.c2 = (q2->c.c2 + 1) % 0x30;
            if (q2->c.c2 == 0) {
                p2->f8 = func_001FA888((q2->c.c1 << 5) + ((func_001160D8() >> 16) & 0xF) - 8) * 0.024543693f;
            }
            k2 = q2->c.c2 * 4;
            if (k2 > 0x60) k2 = 0xC0 - k2;
            {
            s32 rr = func_001160D8() >> 16;
            u32 x;
            x = (((u32 *)&D_L00_001605B0))[p2->u.c.c0];
            x += (rr & 0xF00) << 8;
            x += (rr & 0xF0) << 4;
            x += rr & 0xF;
            x += k2 << 24;
            p2->col = x;
            }
        } else if (p2->type == 3) {
            s32 rs = func_001160D8() >> 16;
            u32 x;
            {
                u32 y = ((rs & 0x1F00) << 10) + 0xFFDFDFE0;
                x = p2->u.w;
                x += y;
            }
            x += (rs & 0x1F0) << 6;
            x += (rs & 0x1F) << 2;
            p2->col = x;
        }
    }
    func_0022CEB8_8AF90e();
    func_00234C98_8AF90(0x42, 0x8000000044LL);
    func_0022C9A8(1);
    func_0022C9A8(2);
}
