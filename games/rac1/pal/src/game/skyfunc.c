#include "common.h"
#include "structs.h"

/*
 * skyfunc.cpp in the original source; text 0x22BEB0-0x22CEB8.
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

INCLUDE_ASM("asm/nonmatchings/text", func_0022BEB0);

struct LevelSkyEffectData {
    u8 pad_0[4];
    s16 relocation_state;
    u8 pad_6[2];
    s16 effect_count;
    u8 pad_A[0x12];
    struct SkyEffect *effects;
};
union SkyEffectColorOrAngles {
    struct SkyAngles {
        s16 azimuth;
        u16 elevation;
    } angles;
    u32 base_color;
};
struct SkyEffect {
    u8 randomize_color;
    u8 pad1[1];
    u8 texture_index;
    u8 flags;
    u32 color;
    f32 angle;
    union SkyEffectColorOrAngles state;
    f32 position_x;
    f32 position_y;
    f32 position_z;
    f32 size;
};
extern struct LevelSkyEffectData * D_0016055C_2C188 __asm__("D_0016055C") MACRO_ADDR;
extern u8 D_001D9A70[];
extern void func_001160C8(s32);
extern f32 func_001F9B88(f32);
extern f32 func_001F9F90(f32);
extern f32 func_001F9FA8(f32);
extern void func_001FA190(u8 *);
extern f32 func_001FA748(f32, f32);
extern f32 func_001FA888(s32);
extern s32 func_002140B0(s32);
extern f32 func_00214158();
extern void func_0022C9A8(s32);
extern void func_0022CEB8();
extern void func_00234C98_2C188(s32, u64) __asm__("func_00234C98");
extern s32 func_001160D8();

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/rendering/sky/update_sky_effects.c, update_sky_effects. */
void func_0022C188(void) {
    f32 trig_product;
    f32 elevation;
    f32 azimuth;
    s32 color_delta;
    u32 base_color;
    s32 initialization_index;
    s32 effect_index;
    s32 alpha_delta;
    s32 random_bits;
    s32 random_color_enabled;
    s32 effect_flags;
    f32 radius;
    struct SkyEffect *effect;
    struct SkyAngles *orbit;
    s16 *angles;

    D_0016055C_2C188->relocation_state = 0;
    func_001FA190(D_001D9A70);
    func_0022C9A8(0);
    func_0022C9A8(1);
    if (D_0016055C_2C188->effect_count == 0) {
        D_0016055C_2C188->effect_count = 0x100;
        func_001160C8(0x3039);
        for (initialization_index = 0; initialization_index < D_0016055C_2C188->effect_count;
             initialization_index++) {
            radius = 50.0f;
            random_color_enabled = 1;
            effect_flags = 0x48;
            base_color = 0x30505050;
            effect = &D_0016055C_2C188->effects[initialization_index];
            if (initialization_index >= 0xF6) {
                effect->randomize_color = 0;
                angles = &effect->state.angles.azimuth;
                effect->state.angles.azimuth = (s16)(func_001160D8() >> 0x10);
                angles[1] = (s16)(func_001160D8() >> 0x10);
                /* Retail stores the float bits 0x3E23D70A. */
                effect->size = 0.16f;
                effect->flags = effect_flags;
                effect->texture_index = random_color_enabled;
            } else {
                effect->randomize_color = random_color_enabled;
                color_delta = func_002140B0(0x100);
                effect->texture_index = random_color_enabled;
                effect->flags = effect_flags;
                effect->state.angles.azimuth = color_delta;
                effect->angle = func_00214158();
                effect->size =
                    func_001FA888(func_002140B0(0x18) + 0x20) * 0.00390625f;
                azimuth = func_001FA748(-3.0f, func_00214158() * 0.2f);
                elevation = func_00214158();
                elevation = elevation * 0.09f;
                elevation = elevation + 1.2f;
                trig_product = func_001F9F90(azimuth);
                trig_product = trig_product * func_001F9FA8(elevation);
                trig_product = trig_product * radius;
                effect->position_x = trig_product;
                trig_product = func_001F9FA8(azimuth);
                trig_product = trig_product * func_001F9FA8(elevation);
                trig_product = trig_product * radius;
                effect->position_y = trig_product;
                effect->position_z = func_001F9F90(elevation) * radius;
                color_delta = func_002140B0(0x18);
                alpha_delta = func_002140B0(0x20) << 0x18;
                if ((func_001160D8() >> 0x10) & 1) {
                    effect->state.base_color = alpha_delta + ((color_delta << 0x10) + base_color);
                } else {
                    effect->state.base_color =
                        (alpha_delta + ((color_delta << 8) + base_color)) | color_delta;
                }
            }
        }
    }
    for (effect_index = 0; effect_index < D_0016055C_2C188->effect_count; effect_index++) {
        effect = &D_0016055C_2C188->effects[effect_index];
        if (effect->randomize_color == 0) {
            orbit = &effect->state.angles;
            angles = &effect->state.angles.azimuth;
            effect->state.angles.azimuth = (s16)(effect->state.angles.azimuth + 1);
            angles[1] = (u16)(angles[1] + 1);
            azimuth = func_001FA888((orbit->azimuth & 0xFFF) - 0x800) * 0.0015339808f;
            elevation = func_001FA888((angles[1] & 0xFFF) - 0x800) * 0.0015339808f;
            trig_product = func_001F9F90(azimuth);
            trig_product = trig_product * func_001F9FA8(elevation);
            trig_product = trig_product * 50.0f;
            effect->position_x = trig_product;
            trig_product = func_001F9FA8(azimuth);
            trig_product = trig_product * func_001F9FA8(elevation);
            trig_product = trig_product * 50.0f;
            effect->position_y = trig_product;
            effect->position_z = func_001F9B88(func_001F9F90(elevation)) * 50.0f;
            if ((u32)(orbit->azimuth & 0x3F) < 8U) {
                effect->color = 0x702020F0;
            } else {
                effect->color = 0x202020F0;
            }
        } else {
            u32 color_value;
            u32 color_offset;
            random_bits = func_001160D8() >> 0x10;
            color_offset = ((random_bits & 0x1F00) << 0xA) + 0xFFDFDFE0;
            color_value = effect->state.base_color + color_offset;
            color_value += (random_bits & 0x1F0) << 6;
            color_value += (random_bits & 0x1F) << 2;
            effect->color = color_value;
        }
    }
    func_0022CEB8();
    func_00234C98_2C188(0x42, (0x8000ULL << 0x18) | 0x44);
    func_0022C9A8(2);
    func_0022C9A8(3);
}

INCLUDE_ASM("asm/nonmatchings/text", func_0022C5A0);

extern int D_00161000 MACRO_ADDR;
extern int D_00160570 MACRO_ADDR;
extern int D_0015EF78 MACRO_ADDR;
extern int D_0015EF74 MACRO_ADDR;
extern int D_0015F558 MACRO_ADDR;
extern char D_00160550[];
extern void func_001F2560(void *, int);
typedef struct {
    long tag;
    long unk_08;
} SkyGifPage;
typedef struct {
    char unk_00[6];
    short count;          /* 0x6: shells */
    char unk_08[4];
    short npages;         /* 0xC: GIF pages */
    short unk_0E;
    SkyGifPage *pages;    /* 0x10 */
    char unk_14[0xC];
    void *shells[1];      /* 0x20 */
} SkyDef;
extern SkyDef *D_0016055C MACRO_ADDR;

/*
 * SetupSkyGifPaging(void): reserves 0x10 bytes of the VU1 chain at
 * D_00160570 and clears the tag of each of the sky's GIF pages. Retail
 * loads the page count before the two global stores: read as a struct
 * field through a pointer it cannot alias a scalar global (gcc's
 * fixed_scalar_and_varying_struct_p), where a cast access would keep it
 * behind them.
 */
void func_0022C7E0(void) {
    int p;
    SkyDef *s;
    int i;

    p = D_00161000;
    D_00160570 = p;
    p += 0x10;
    D_00161000 = p;
    func_001F2560(D_00160550, 1);
    s = D_0016055C;
    D_0015EF74 = D_0015EF78;
    D_0015F558 = 0;
    for (i = 0; i < s->npages; i++) {
        s->pages[i].tag = 0;
    }
}

extern int *D_00161000_p __asm__("D_00161000") MACRO_ADDR;
extern int *D_00160570_p __asm__("D_00160570") MACRO_ADDR;
extern int *D_00160574 MACRO_ADDR;
extern void func_0020C2F8(void);
extern void func_00234E80(void);

/* DoSkyGifPaging: DoGifPaging's twin (func_001F4748) for the sky. Pushes
   two 4-word GIF tags (0x20000000 in the first word) onto the D_00161000
   packet, D_00160574 marking where it started and D_00160570's tag
   pointing at the second; in between, when D_0018A3B0[1] is set,
   func_0020C2F8 and func_00234E80 add their own. Then restores
   D_0015EF74 from D_0015EF78. The first advance goes through a local
   advanced in place, which keeps the old and new pointer in one register
   as retail does. */
void func_0022C870(void) {
    int *p = D_00161000_p;

    D_00160574 = p;
    p += 4;
    D_00161000_p = p;
    D_00160570_p[0] = 0x20000000;
    D_00160570_p[1] = (int)D_00161000_p;
    D_00160570_p[2] = 0;
    D_00160570_p[3] = 0;
    if (D_0018A3B0[1] != 0) {
        func_0020C2F8();
        VU1_texFlush();
    }
    D_00161000_p[0] = 0x20000000;
    D_00161000_p[1] = (int)(D_00160570_p + 4);
    D_00161000_p[2] = 0;
    D_00161000_p[3] = 0;
    D_00161000_p += 4;
    D_00160574[0] = 0x20000000;
    D_00160574[1] = (int)D_00161000_p;
    D_00160574[2] = 0;
    D_00160574[3] = 0;
    D_0015EF74 = D_0015EF78;
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

LINKER_REMNANT("asm/remnants/text", func_0022C9A0);

extern void func_0022CA00(void *);
extern void func_0022CC40(void *);

/* SkyDrawShell(int) */
void func_0022C9A8(int idx) {
    SkyDef *def = D_0016055C;

    if (idx < def->count) {
        void *shell = def->shells[idx];
        if (*(int *)((char *)shell + 4) != 0) {
            SkyDrawShellGouraud(shell);
        } else {
            SkyDrawShellTextured(shell);
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/text", func_0022CA00); /* SkyDrawShellTextured */

INCLUDE_ASM("asm/nonmatchings/text", func_0022CC40); /* SkyDrawShellGouraud */
