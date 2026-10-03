#include "common.h"
#include "structs.h"

/*
 * space.cpp in the original source; text 0x22F128-0x233FF8.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/* Declarations in scope here before the split. */
extern char D_0013E650[];
extern int D_0015F694;
extern void func_001F9A98(void *, void *, int);
extern char D_00189310[];
extern char D_001899D0[];
extern void *D_001871C0 NOT_SDA;
typedef struct {
    char unk_00[8];
    void (*fn_08)(void *);
    char unk_0C[4];
    void (*fn_10)(void *);
} DispatchRec;
extern DispatchRec D_001E8F80[];
extern int D_0018A3B0[];
extern void func_001F99B0();
extern void func_001F2BC8(void);
extern int D_0018C434 NOT_SDA;
extern char D_001940C0[];
extern long D_00151888[3];
extern int D_0015F6FC;
extern short D_0015F534;
extern void func_001FB530(void);
extern void func_001F3D78(void);
extern int D_0015F564;
extern int D_0018DD40[];
extern int D_0018DC40[];
extern short D_0015F59C;
extern int func_001F65B0(unsigned char *arg0, int arg1, void *arg2);
extern unsigned char D_001DF3D0[];
extern unsigned char D_001DF770[];
extern unsigned char D_001DFB10[];
extern void func_001F6668(void *, void *, void *, void *, void *, int,
                          unsigned char *);
extern int func_001F6600(unsigned char *, int);
extern int func_001F6620(unsigned char *, int);
extern int func_001F4868(int);
extern void func_001F7070(void *, void *, void *, void *, int, unsigned char *);
extern void func_001FB498(void);
extern void func_001F3008(void);
extern void func_001F3140(void);
extern int D_0018E840[];
extern long D_00152178 NOT_SDA;
extern int func_001FE4D0(void);
extern char D_00199A68[];
extern short D_0015F780;
extern int D_001941CC NOT_SDA;
extern int D_0019A4E8 NOT_SDA;
extern int func_001FF668(int);
typedef struct {
    char b[0x13];
} Cfg13;
extern Cfg13 D_0019A540 NOT_SDA;
extern Cfg13 D_001E7DD8 NOT_SDA;
extern int func_00116810(void);
extern void func_001166FC(Cfg13 *, void *);
extern short D_0015F9D0;
extern void func_00201960(int, int, int, int, int);
extern void func_002023E0(int);
extern void func_002027C0(int);
extern void func_00204FC0(void *);
extern int D_0018CC20 NOT_SDA;
extern int D_001941C8 NOT_SDA;
extern int D_0016100C;
extern int D_001A0468[];
extern void func_00205830(int a, int b);
typedef struct {
    int _pad0[0x9E];
    int use[5];   /* +0x278 */
    int flags[5]; /* +0x28C */
    int sel;      /* +0x2A0 -- index of the active slot, -1 for none */
    int size[5];  /* +0x2A4 */
} PadSlots;
extern PadSlots D_001A01F0_slots __asm__("D_001A01F0");
extern int D_001A01F0[];
extern int *D_001602E0;
extern unsigned char D_0013D49C NOT_SDA;
extern unsigned char D_0013D49D NOT_SDA;
extern unsigned char D_0013D4A5 NOT_SDA;
extern short D_0015FE24;
extern unsigned char D_0013D4AC NOT_SDA;
extern unsigned char D_0013D4AD NOT_SDA;
extern unsigned char D_0013D4AE NOT_SDA;
extern unsigned char D_0013D4AF NOT_SDA;
extern unsigned char D_0013D4B5 NOT_SDA;
extern int D_001A04B4 NOT_SDA;
extern unsigned char D_0013D4C5 NOT_SDA;
extern int D_001414DC NOT_SDA;
extern unsigned char D_0013D4C0 NOT_SDA;
extern unsigned char D_0013D4C1 NOT_SDA;
extern unsigned char D_0013D4C2 NOT_SDA;
extern unsigned char D_0013D4D3 NOT_SDA;
extern unsigned char D_0013D4D4 NOT_SDA;
extern unsigned char D_0013D4D5 NOT_SDA;
extern unsigned char D_0013D4E0;
extern unsigned char D_0013D4DC NOT_SDA;
extern unsigned char D_0013D4DD NOT_SDA;
extern unsigned char D_0013D4DE NOT_SDA;
extern unsigned char D_0013D4DF NOT_SDA;
extern unsigned char D_0013D4E1 NOT_SDA;
extern unsigned char D_0013D4E9 NOT_SDA;
extern unsigned char D_0013D502 NOT_SDA;
extern unsigned char D_0013D503 NOT_SDA;
extern unsigned char D_0013D504 NOT_SDA;
extern unsigned char D_0013D505 NOT_SDA;
extern unsigned char D_0013D50F NOT_SDA;
extern int D_0013D668[];
extern void func_00209040(void);
extern int func_001FAA28(void *dst, int size, int a, int b);
extern void func_00208860(void *dst);
extern short D_0015EE84;
extern int D_0015EE84_far __asm__("D_0015EE84") NOT_SDA;
extern int D_001A0218[] NOT_SDA;
extern void func_00208458(void *, unsigned char *, int);
extern void func_00208688(void *, unsigned char *);
extern char D_0013D390[];
extern short D_0015EFB0;
extern int D_0015EFB4;
extern int D_001A05C0[];
extern int D_001A08C0[];
extern int func_0020BAD8(int *p);
extern int func_0020BBC8(void *dst, int i, int *table);
extern int func_001236F0(void);
extern int func_001E9730();
extern char D_001E8690[];
extern int D_0013D844 NOT_SDA;
extern unsigned char D_0013D4A8 NOT_SDA;
extern int D_0013D9B4 NOT_SDA;
extern unsigned char D_0013D490[];
extern unsigned char D_0013D5CA NOT_SDA;
extern int D_0013D6B8 NOT_SDA;
extern int D_0013DAE4 NOT_SDA;
extern unsigned char D_0013D4E5 NOT_SDA;
extern int D_0013DB24 NOT_SDA;
extern unsigned char D_0013D4F1 NOT_SDA;
extern int D_0013DC34 NOT_SDA;
extern unsigned char D_0013D605 NOT_SDA;
extern int D_0013D5C8 NOT_SDA;
extern unsigned char D_0013D4B0 NOT_SDA;
extern unsigned char D_0013DE55 NOT_SDA;
extern unsigned char D_0013D5DD NOT_SDA;
extern unsigned char D_0013D5E7 NOT_SDA;
extern int D_001B2F40[];
extern void func_001FA460_2(void *, void *) __asm__("func_001FA460");
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_002116A0(void *, int, int *, void *);
extern void func_001FA540(void *, void *, void *);
extern void func_00211548(void *, int, void *, void *);
extern void func_001F9EC0(void *, void *, void *);
extern int D_001414D0 NOT_SDA;
extern float D_001CAE00[] NOT_SDA;
extern void func_0020E360(void *, void *);
extern float func_001FA058(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_00118D80(int);
extern void func_00212578(int, int);
extern char D_00165600[];
extern int D_0015F718;
extern short D_0015F71C;
extern char D_001B3200[];
extern int func_001160D8(void);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern void func_00215C00(void *, float, float, float);
extern void func_001F9DC0(void *, void *, float);
extern void func_001FA460(void *);
extern void func_002150B0(void *, void *);
extern void func_001FA480(void *, void *);
extern float func_0020D830(void);
extern float func_00215A98(int, float);
extern unsigned char D_0014BFC0[];
extern unsigned char D_0013E620[];
extern unsigned char D_0013D510[];
extern void func_0012F068(void *);
extern void func_002177F0(int);
extern short D_001517D0[];
extern void func_0012EDE0(void *);
extern void func_0012EFE8(void);
extern char D_001E8980[];
extern int func_0012EE98(int, int, int, void *);
extern void func_001F9978(void);
extern int func_00217628_v(void) __asm__("func_00217628");
extern void func_00122598(int);
extern void func_00217130(void);
extern void func_0012EC40(void);
extern void func_0012DDC0(void);
extern void func_0012EC30(void);
extern int func_0012F030(void);
extern void func_002167C0(short, short, short);
extern void func_002169B8(short, short, short);
extern short D_001517F0 NOT_SDA;
extern char D_0013CA40[];
extern int D_001CDAE0 NOT_SDA;
extern void func_00124650(void);
extern void func_00124B88(int);
extern int func_00124BC8(void *, void *);
extern void func_00217F68(void *);
extern int D_0015EF90;
extern char D_001D4B90[];
extern char D_001D4BC0[];
extern char D_001D5F70[] NOT_SDA;
extern char D_001D603B[];
extern int D_001A0414;
extern int D_001CFBF4;
extern int D_001CFAD8;
extern void func_0020C7A0(void *);
extern int func_0020CA50(void *, void *, void *, int);
extern int D_00141FA0[];
extern char D_001D0A50[];
extern char D_001D0A88[];
extern int D_001A0418 NOT_SDA;
extern void func_00226D50(int);
extern float func_001FA748(float, float);
extern char *D_001D5F74 NOT_SDA;
extern void func_0020E180(int, int);
extern char D_00187040[];
extern void func_00220128(void *);
extern void *func_00226720_a(int) __asm__("func_00226720");
extern int func_002267C0(int);
extern void func_00234C98(int, int);
extern void func_00205E70(void);
extern void func_001F4630(int);
extern void func_001F4748(void);
extern void func_001F68E8_c(int, int, long, void *, int)
    __asm__("func_001F68E8");
extern void *func_001FE540_id(int) __asm__("func_001FE540");
extern short D_001602B0;
extern void func_00201640(int, int, int, int, long, long);
extern int func_00200198(int, int);
extern void func_00200468(int, int, int, int, int, int);
extern void func_001F5800(int, int, int, int, int, int, int, int, long,
                          long);
extern short D_00151880[];
extern long D_001A0448;
extern int func_00226EA8(int);
extern int func_00226F68(int);
extern int D_0013CC04 NOT_SDA;
extern char D_001D2678[];
extern char *D_001D5F78 NOT_SDA;
extern void func_001FDF78(int, int, int, int);
extern unsigned char D_001B3E40[] NOT_SDA;
extern void *func_0020D348(void);
extern void func_0020ED48(void *);
extern void func_0020E340(void *, int, int, int, int);
typedef struct {
    int key;
    int flags;
} PadBind;
extern PadBind D_001D6448_t[] __asm__("D_001D6448");
extern int func_00227018(int handle);
extern int D_001D6448[];
extern char D_001D5D58[] NOT_SDA;
extern char *D_001B3580[] NOT_SDA;
extern int D_001D6860[];
extern int D_001D74C0[];
extern int D_001D6760[];
extern char D_00187180_a[] __asm__("D_00187180");
extern char D_00194220[];
extern int D_0013E6BC;
extern void func_002141A8(void *, float, float);
extern void func_001F9BD8_a(void *, void *, void *) __asm__("func_001F9BD8");
extern void func_001F9C30_a(void *, void *, float) __asm__("func_001F9C30");
extern void func_001F9BF0_a(void *, void *, void *) __asm__("func_001F9BF0");
extern int func_001EFE10_a(void *, void *, int, int, int) __asm__("func_001EFE10");
extern char D_00187180[];
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9E58(void *, void *, float);
extern void func_001EFE10(void *, void *, int, int, int);
extern float func_001F9D10(int, void *);
extern void func_0022DA10(void *, float, float, float);
extern void func_001F9EE8(void *, void *, void *);
extern float func_001F9CE8(void *);
extern float func_001F9BB0(float, float, float);
extern float func_001FA058_a(float, float) __asm__("func_001FA058");
extern void func_001FA898(float);
extern void func_00120F30(int);
extern int func_0012E060(void *, int);
extern void func_0012EE70(int);
extern void func_0012EF48(int);
extern void func_0012E2E8(void);

extern char D_0018CC20_c[] __asm__("D_0018CC20");
extern float D_0018CEB0;
extern unsigned char D_0015EEB4_m[4] __asm__("D_0015EEB4") MACRO_ADDR;
extern void func_00125358(float *);
extern void func_001254A0(float *, float *, float);
extern void func_00125548(float *, float *, float);
extern void func_001253F8(float *, float *, float);
extern void func_001F9CA0(void *, void *, void *);

/* Movie-camera keyframe step (as func_001EB338): the keyframe's +0x1C
   sets D_0018CEB0, its position and X/Y/Z angles set the camera position
   and matrix rows, and with the D_0015EEB4 flag set the row at +0x220 is
   rebuilt from the other two by func_001F9CA0. Returns the flag byte.
   The flag is read through a 4-byte MACRO_ADDR alias: over -G2, so the
   macro expands to lui/lbu through the destination register. */
unsigned char func_0022F128(void) {
    char *t = D_0018CC20_c;
    char *key = *(char **)(t + 0x54) + *(int *)(t + 0x38) * 32;
    unsigned char flag = key[0xC];
    float *ang = (float *)(key + 0x10);
    char *pos;
    char *cam;
    float m[16];

    D_0018CEB0 = ang[3];
    UpdateViewContext();
    pos = D_00187180;
    qcopy(pos, key);
    func_00125358(m);
    func_001254A0(m, m, *(float *)(key + 0x10));
    func_00125548(m, m, ang[1]);
    func_001253F8(m, m, ang[2]);
    cam = pos - 0x140;
    *(float *)(cam + 0x350) = -m[8];
    *(float *)(cam + 0x360) = -m[0];
    *(float *)(cam + 0x370) = m[4];
    *(float *)(cam + 0x354) = -m[9];
    *(float *)(cam + 0x364) = -m[1];
    *(float *)(cam + 0x374) = m[5];
    *(float *)(cam + 0x358) = -m[10];
    *(float *)(cam + 0x368) = -m[2];
    *(float *)(cam + 0x378) = m[6];
    if (D_0015EEB4_m[0] != 0) {
        FastVecCross(pos + 0x220, pos + 0x230, pos + 0x210);
    }
    return flag;
}

typedef struct {
    float v[4][4];     /* 0x00 */
    int rgba[4];       /* 0x40 */
    char uv[0x20];     /* 0x50 */
    long gs[4];        /* 0x70 */
} Quad_FBE0;

extern char D_001D9B40[];
extern void func_001F7EF8(void *, int, int);
extern int D_0015F6E8 MACRO_ADDR;
extern int func_001F98C0(int);
extern void func_001F9C48(void *, void *, float);
extern int func_001FA9E8(void *, int *, float);
extern float func_00214358(void *, int, float);
typedef struct {
    char pad[0x20];
    int mode;
    short level;
    short set;
} FlareCfg;
extern FlareCfg D_0013E130;
extern float D_001D9B60[][4][4];

/* Draws a light flare quad at arg0's position (+0x00; the matrix is at
   +0xC0) when func_001FA9E8 finds it on screen, with the alpha it
   returns. In D_0015F6E8 mode 6 with flare set 3, the flare fades as
   D_0013E130's level passes func_001F98C0(150), and its depth comes from
   func_00214358. The corners are D_001D9B60[set]. */
void func_0022F258(char *arg0) {
    float a[4];
    Quad_FBE0 q;
    float b[4];
    int alpha;
    float z;
    int i;

    alpha = 0;
    func_001F9C48(a, arg0, 1.0f / 1024.0f);
    if (func_001FA9E8(a, &alpha, 32.0f) < 0) {
        return;
    }
    if (D_0015F6E8 == 6 && D_0013E130.mode == 3
        && D_0013E130.level > func_001F98C0(150)) {
        alpha -= (D_0013E130.level - func_001F98C0(150)) * 4;
        if (alpha <= 0) {
            return;
        }
    }
    q.gs[1] = GetEffectTex(0);
    q.gs[2] = 0xFF9000000260;
    q.gs[3] = 0x8000000044;
    q.gs[0] = 0;
    FastMemCopy(q.uv, D_001D9B40, 0x20);
    FastVecScale(b, arg0, 1.0f / 1024.0f);
    z = *(float *)(arg0 + 0x18) + 0.1f;
    if (D_0015F6E8 == 6) {
        z = func_00214358(arg0 + 0x10, 0, 0.5f) + 0.1f;
    }
    for (i = 0; i < 4; i++) {
        float *v;

        q.rgba[i] = ((alpha >> 1) << 24) | 0x808080;
        v = q.v[i];
        func_001F9EC0(v, D_001D9B60[D_0013E130.set][i], arg0 + 0xC0);
        FastVecAdd(v, v, arg0 + 0x10);
        v[2] = z;
    }
    func_001F7EF8(&q, 0, 0);
}

/* The five nops of padding after func_0022F258 (they sat after
   `endlabel` in its .s, so the stub carried them); see func_001F6668's
   note in draw.c. */
__asm__(".section .text\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n");

LINKER_REMNANT("asm/remnants/text", func_0022F498);

extern int D_0015F6E4 MACRO_ADDR;
extern int D_0015F6FC_m __asm__("D_0015F6FC") MACRO_ADDR;
extern short D_0015F690; /* SDA, gp -0x7670 */

void func_0022F4A0(int arg0) {
    D_0015F6E4 = arg0;
    *(int *)&D_0015F690 = 1;
    D_0015F6FC_m = 1;
}

INCLUDE_ASM("asm/nonmatchings/text", func_0022F4C0);

INCLUDE_ASM("asm/nonmatchings/text", func_0022F738);

extern void func_00234C98_l(int, long) __asm__("func_00234C98");
extern long D_00160680 MACRO_ADDR;
extern char D_001D9B40[];
extern float D_001D9E20[];
extern float D_001D9DE0[][4];
extern short D_001605F0;
extern void func_001F7EF8(void *, int, int);
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;

/* Its 0x8000000044 is built as ps2eeas built it (tools/ps2eeas_dli.py). */
void func_0022FBE0(void) {
    Quad_FBE0 q;
    float s;
    int i;

    func_00234C98_l(0x47, 0x31801);
    s = 1.0f;
    q.gs[0] = 0;
    q.gs[1] = D_00160680;
    q.gs[2] = 0xFF9000000260;
    q.gs[3] = 0x8000000044;
    FastMemCopy(q.uv, D_001D9B40, 0x20);
    if ((unsigned)D_0015EE84_m < 0x13) s = D_001D9E20[D_0015EE84_m];
    for (i = 0; i < 4; i++) {
        q.rgba[i] = 0x80808080;
        FastVecScale(q.v[i], D_001D9DE0[i], s);
        FastVecAdd(q.v[i], q.v[i], &D_001605F0);
    }
    func_001F7EF8(&q, 0, 0);
    func_00234C98_l(0x47, 0x5360B);
}

/* The 64-bit argument form of func_00234C98 is needed here (one call
   passes 0x8000000044). Retail's ori 0x8000 / dsll 24 / ori 0x44 for
   that constant is ps2eeas's expansion of the same `dli`, which
   tools/ps2eeas_dli.py reproduces; GNU as builds it differently. */
extern void func_00234C98_l(int, long) __asm__("func_00234C98");
extern int D_0013E604;
extern long D_00160688 MACRO_ADDR;

void func_0022FD20(int arg0) {
    func_00234C98_l(0x47, 0x31801);
    func_00234C98_l(0x42, ((long)0x8000 << 24) | 0x44);
    DrawTexturedQuad(0x20, D_0013E604 - 0x58, 0x100, 0x20, 0, 0, 0x100, 0x20,
                  (arg0 << 24) | 0x808080, D_00160688);
    func_00234C98_l(0x47, 0x5360B);
}

INCLUDE_ASM("asm/nonmatchings/text", func_0022FDC0);

INCLUDE_ASM("asm/nonmatchings/text", func_002305A0);

extern float D_001D9EF0[][6][4];
extern float D_0015EE6C MACRO_ADDR;
extern int func_002140B0(int);
extern char *func_00219780(void *, void *, void *, int, int, int, int, int, int);

/* Spawns six particles around a moby: random velocity and a position taken from the flare corner table, transformed by the moby's matrices. Adapted from Lombyte (MIT) for PAL: src/rendering/effects/fun_0022f5b0.c, FUN_0022f5b0. */
void func_002308C8(char *m, float z) {
    float vel[4];
    float vel2[4];
    float pos[4];
    int i;
    int a;
    int b;
    int c;

    for (i = 0; i < 6; i++) {
        vel[0] = func_002140F8(-D_0015EE6C, D_0015EE6C);
        vel[1] = func_002140F8(-D_0015EE6C, D_0015EE6C);
        vel[2] = z + func_002140F8(D_0015EE6C * -0.25f, D_0015EE6C * 0.25f);
        vel[3] = 0.4f;
        qcopy(vel2, vel);
        vel2[3] = 0.6f;
        qcopy(pos, D_001D9EF0[D_0013E130.set][i]);
        func_001F9EC0(pos, pos, m + 0xC0);
        func_001F9BD8(pos, pos, m + 0x10);
        a = func_001F98C0(4);
        b = func_001F98C0(4);
        c = func_001F98C0(4);
        func_00219780(pos, vel, vel2, 0x24C0C0C0, 0x14C0C0C0, a, b, c + func_002140B0(func_001F98C0(4)), -1);
    }
}

INCLUDE_ASM("asm/nonmatchings/text", func_00230A90);

extern int D_0013E150;
extern void func_001F45F0(void);
extern void func_002305A0(void);

/* A switch on D_0013E150, whose jump table is linked into the data
   segment at retail's jtbl_001E8C90 (see rac1.ld.sh). */
void func_00232200(void) {
    if (D_0015F6FC_m == 0) {
        switch (D_0013E150) {
        case 0:
        case 8:
            drawNormalFrame();
            break;
        case 3:
        case 7:
            *(int *)&D_0015F534 = 0x7F;
            DrawDebugProfiler();
            break;
        case 4:
            func_002305A0();
            break;
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/text", func_00232278);

typedef struct {
    int sector;
    int size;
} WadEntry;
typedef struct {
    char _pad0[0x1938];
    WadEntry movie[12];  /* 0x1938 */
    WadEntry movie2[12]; /* 0x1998 */
} WadToc;
extern WadToc D_00137C80_t __asm__("D_00137C80");
extern int D_0015EE80 MACRO_ADDR;
extern int D_0015EE88 MACRO_ADDR;
extern int D_001941D4;
extern int func_0023B670(int, int, int, int, int);
extern void func_00120858(int, int);
extern void func_00123168(void *);
extern void func_0012F308(void);
extern void func_001F4E08(int);

/* Plays movie id, taking its {sector, size} from the WAD table of
   contents (from the second table when D_0015EE80 is set). */
void func_00232920(int id) {
    int sector;
    int size;
    int buf;

    if (D_0015EE80 != 0) {
        sector = D_00137C80_t.movie2[id].sector;
        size = D_00137C80_t.movie2[id].size;
    } else {
        sector = D_00137C80_t.movie[id].sector;
        size = D_00137C80_t.movie[id].size;
    }
    D_0013E650[0x6B] |= 8;
    buf = D_001941D4;
    func_0023B670(sector, size, (buf + 0x3F) & ~0x3F,
                  (buf + 0x300000 + 0x3F) & ~0x3F, D_0015EE88);
    func_00122598(0);
    func_00120858(0, 0);
    func_00123168(func_0012F308);
    FadeToBlack(4);
    D_0013E650[0x6B] |= 0x10;
}

extern int *D_00161000 MACRO_ADDR;
extern int D_0013E600[];
extern char D_00160950[];

typedef union {
    struct {
        float u;
        float v;
    } f;
    long bits;
} SpaceUvPair;

/* Append a textured four-corner GIF packet. Screen coordinates use 12.4
   fixed point relative to the viewport origin; UV pairs stay as floats. */
void func_00232A00(int x, int y, int w, int h, unsigned long rgba,
                   unsigned long tex, float u0, float u1, float v0, float v1) {
    int x0 = x * 16 + D_0013E600[4] - 8;
    int x1 = (x + w) * 16 + D_0013E600[4] - 8;
    int y0 = y * 16 + D_0013E600[5] - 8;
    int y1 = (y + h) * 16 + D_0013E600[5] - 8;
    SpaceUvPair uv[4];
    long *p;
    int *base;

    uv[0].f.u = u0;
    uv[0].f.v = v0;
    uv[1].f.u = u1;
    uv[1].f.v = v0;
    uv[2].f.u = u0;
    uv[2].f.v = v1;
    uv[3].f.u = u1;
    uv[3].f.v = v1;

    D_00161000[0] = 0x10000007;
    D_00161000[1] = 0;
    D_00161000[2] = 0;
    D_00161000[3] = 0x50000007;
    base = D_00161000;
    D_00161000 = base + 4;
    qcopy(base + 4, D_00160950);
    p = (long *)(base + 8);
    D_00161000 = base + 8;
    p[0] = tex;
    p[1] = 0x54;
    p[2] = (rgba & 0xFFFFFFFFL) | (0xFE00L << 46);
    p[3] = uv[0].bits;
    p[4] = x0 | ((long)y0 << 16) | ((long)0xFFFFF0 << 32);
    p[5] = uv[1].bits;
    p[6] = x1 | ((long)y0 << 16) | ((long)0xFFFFF0 << 32);
    p[7] = uv[2].bits;
    p[8] = x0 | ((long)y1 << 16) | ((long)0xFFFFF0 << 32);
    p[9] = uv[3].bits;
    p[10] = x1 | ((long)y1 << 16) | ((long)0xFFFFF0 << 32);
    p[11] = 0;
    D_00161000 = (int *)((char *)D_00161000 + 0x60);
}

INCLUDE_ASM("asm/nonmatchings/text", func_00232B90);

INCLUDE_ASM("asm/nonmatchings/text", func_00232EF0);

typedef struct {
    int pad0[4];
    unsigned int f10;
} Blk4200;
extern Blk4200 D_00194200_b __asm__("D_00194200");
typedef struct {
    unsigned char pad0[8];
    unsigned char f8;
    unsigned char pad9[5];
    unsigned char fE;
    unsigned char fF;
} Blk3E48;
extern Blk3E48 D_0013DE48_b __asm__("D_0013DE48");
extern unsigned char D_0013DE60[];
typedef struct {
    char pad0[0xB];
    char fB;
} Blk17D0;
extern Blk17D0 D_001517D0_b __asm__("D_001517D0");
typedef struct {
    char pad0[0x218];
    int f218;
    float f21C;
    char pad220[8];
    float f228;
    float f22C;
    int f230;
    int f234;
    int f238;
} Blk8CE00;
extern Blk8CE00 D_0018CE00_b __asm__("D_0018CE00");
typedef struct {
    char pad0[0xDC];
    int fDC;
    char padE0[4];
    int fE4;
} Blk3D390;
extern Blk3D390 D_0013D390_b __asm__("D_0013D390");

extern char *D_0015F714 MACRO_ADDR;
extern int D_0015F538 MACRO_ADDR;
extern int D_0015EE5C_m __asm__("D_0015EE5C") MACRO_ADDR;
extern short D_0015EF48_m __asm__("D_0015EF48");
extern int D_0015EF4A_i __asm__("D_0015EF4A") MACRO_ADDR;
extern char D_001E8CB8[];
extern int func_00122598_i(int) __asm__("func_00122598");
extern int func_0012EF48_i(int) __asm__("func_0012EF48");
extern int func_00120F30_i(int) __asm__("func_00120F30");

extern void func_0012F0A8(int, int, int, int, int);
extern void func_0022EFE8(void);
extern void func_00216D88(void);
extern void func_0012E318(int);
extern void func_001FB448(int, int, int);
extern void func_00209E68(void);
extern void func_00209070(void);
extern int func_00204BE8(void);
extern void func_002350A8(void);
extern void func_00232EF0(int, int, int, int, int);
extern void func_00232278(void);
extern int func_0012F4A8(int);
extern void func_002349B8(void);
extern void func_00234948(void);
extern void func_001FB598(void);
extern void func_001FB8A8(void);
extern void func_00218908(void);
extern void func_00230A90(void);
extern void func_00228110(void);
extern int func_00204C60(void);
extern void func_00234AC8(int);

/* DoSpaceTransition: plays the level-to-level transition (dialogue screens, loading loop) and waits for the load. Adapted from Lombyte (MIT) for PAL: src/gameplay/state/do_space_transition.c, do_space_transition. */
void func_00233308(void) {
    int lvl;
    int ok;
    int done;

    lvl = D_0015EE88 - 1;
    if (lvl < 0) {
        lvl = 0;
    }
    D_00194200_b.f10 |= 0x80000000;
    D_0015F6E8 = 6;
    D_0013E130.set = 0;
    if (D_0013DE48_b.f8 != 0 || D_0015F6E4 >= 8) {
        D_0013E130.set = 1;
    }
    if (D_0013DE48_b.fE != 0 || D_0015F6E4 >= 14) {
        D_0013E130.set = 2;
    }
    func_00118D80(0);
    func_0012F0A8(2, 0, 0, 0, 0);
    func_0012EC40();
    func_0012DDC0();
    func_0022EFE8();
    func_00216D88();
    D_001517D0_b.fB = 1;
    if (D_0015F714 != 0) {
        func_0012E318(*(int *)(D_0015F714 + 0x1C));
        func_0012E2E8();
        func_001E9730(D_001E8CB8, *(int *)(D_0015F714 + 0x1C));
    }
    D_0015EE5C_m = 0;
    func_0012F068(0);
    func_0012EF48_i(0);
    D_0018CE00_b.f238 = 16;
    D_0018CE00_b.f21C = 524288.0f;
    D_0018CE00_b.f228 = 255.0f;
    D_0018CE00_b.f230 = 0;
    D_0018CE00_b.f234 = 0;
    D_0018CE00_b.f218 = 0;
    D_0018CE00_b.f22C = 128.0f;
    func_001FB448(0, 0, 0);
    if (D_0015F6E4 < 0) {
        while (D_0013D390_b.fDC >= 3 || D_0013D390_b.fE4 >= 0) {
            func_00209E68();
            func_00209070();
        }
        func_001F4E08(func_001F98C0(6));
        D_0015EE84_m = D_0015F6E4;
        func_00204BE8();
        func_00120F30_i(0);
        D_001517D0_b.fB = 0;
        func_002350A8();
        return;
    }
    if (D_0015F6E4 == 0 && D_0013DE60[0] == 0) {
        func_001F4E08(func_001F98C0(6));
        func_00232EF0(lvl, 0, 1, func_001F98C0(240), 0);
        func_00232920(0);
        func_00232EF0(lvl, 2, 2, func_001F98C0(180), 0);
        func_00232920(1);
        D_0015EE84_m = D_0015F6E4;
        func_00232EF0(lvl, 3, 4, func_001F98C0(240), 1);
        func_00232920(2);
    } else if (D_0015EE84_m == 0 && D_0015F6E4 == 1 && D_0013DE60[1] == 0) {
        func_001F4E08(func_001F98C0(6));
        func_00232EF0(lvl, 5, 6, func_001F98C0(240), 0);
        func_00232920(3);
        func_00232920(4);
        func_00232EF0(lvl, 7, 7, func_001F98C0(180), 0);
        func_00232920(5);
        D_0013DE60[D_0015EE84_m] = 2;
        D_0015EE84_m = D_0015F6E4;
        func_00232EF0(lvl, 8, 8, func_001F98C0(240), 1);
    } else {
        if (D_0015F6E4 == 4 && D_0013DE60[4] == 0) {
            func_001F4E08(func_001F98C0(12));
            func_00232EF0(lvl, 9, 10, func_001F98C0(240), 0);
            func_00232920(6);
        }
        if (D_0015EE84_m == 7 && D_0013DE60[7] != 2 && D_0013DE48_b.f8 != 0) {
            func_001F4E08(func_001F98C0(12));
            func_00232EF0(lvl, 11, 11, func_001F98C0(240), 0);
            func_00232920(7);
        }
        if (D_0015F6E4 == 13 && D_0013DE60[13] == 0) {
            func_001F4E08(func_001F98C0(12));
            func_00232EF0(lvl, 12, 13, func_001F98C0(240), 0);
            func_00232920(8);
        }
        if (D_0015EE84_m == 14 && D_0013DE60[14] != 2 && D_0013DE48_b.fF != 0) {
            func_001F4E08(func_001F98C0(12));
            func_00232EF0(lvl, 14, 14, func_001F98C0(240), 0);
            func_00232920(9);
        }
        if (D_0015F6E4 == 16 && D_0013DE60[16] == 0) {
            func_001F4E08(func_001F98C0(12));
            func_00232EF0(lvl, 15, 16, func_001F98C0(240), 0);
            func_00232920(10);
        }
        if ((unsigned int)D_0015EE84_m < 19) {
            ok = 1;
            if (D_0015EE84_m == 7 && D_0013DE48_b.f8 == 0) {
                ok = 0;
            }
            if (D_0015EE84_m == 14 && D_0013DE48_b.fF == 0) {
                ok = 0;
            }
            if (ok) {
                D_0013DE60[D_0015EE84_m] = 2;
            }
        }
        *(short *)&D_0015EF4A_i = 1;
        done = 0;
        D_0015EE84_m = D_0015F6E4;
        D_0015EF48_m = 0;
        func_00232278();
        func_0012F4A8(D_0015EE84_m);
        while (D_0015F6FC_m == 0) {
            func_002349B8();
            func_00234948();
            func_001FB598();
            func_001FB8A8();
            func_001FB498();
            func_00218908();
            func_00230A90();
            func_00232200();
            func_00209E68();
            func_00209070();
            func_00234AC8(1);
            func_00122598_i(0);
            D_0015F538++;
            func_00228110();
            if (!done) {
                done = func_00204C60();
            }
        }
        if (!done) {
            do {
                func_00118D80(0);
                func_00122598_i(0);
                func_00209E68();
                func_00209070();
                func_00228110();
            } while (func_00204C60() == 0);
        }
        while (D_0013D390_b.fDC != 2 || D_0013D390_b.fE4 >= 0) {
            func_00118D80(0);
            func_00122598_i(0);
            func_00209E68();
            func_00209070();
            func_00228110();
        }
    }
    func_00120F30_i(0);
    D_001517D0_b.fB = 0;
    func_002350A8();
}

INCLUDE_ASM("asm/nonmatchings/text", func_00233AB8);
