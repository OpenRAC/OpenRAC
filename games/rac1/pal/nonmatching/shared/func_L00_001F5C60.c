/* NON_MATCHING func_L00_001F5C60 -- src/overlays/shared/draw_001F3A78.c
 * Best so far: BYTES 1/4884 (100.0% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   index directly optimizes bits 0 and 2 to shifts or reverses conditional moves.
 *   Returning an offset preserves selection and addition; positive early return
 *   in p9 instead gave movz and 91 bytes different. The reversed early return
 *   in p10 recovers movn and leaves only the one commuted addition.
 *   p11 reverses the C operand order and p12 names a mission local. Both emit the
 *   identical final function to p10 (same one-byte difference). Stopped under the
 *   three-identical-variants rule, not the overall budget. No integration, no
 *   progress update, no nonmatching edits. Released claim.
 */
#include "common.h"
typedef struct {
 s32 mode, bits, draw_bits, group, item, camera_mode, display_mode, occl_mode;
 s32 f20, f24, f28, f2C, f30, f34, selection, mission;
 char pad40[0xB0];
 s32 fF0, fF4;
 u8 saved_timer, point_timer;
} DebugMenu60;
extern DebugMenu60 debug_menu60 __asm__("D_L00_0016C158");
typedef struct {
 char pad0[0x28];
 s32 ammo28, ammo2C, unused30, ammo34, unused38, ammo3C, ammo40, ammo44;
 s32 unused48, ammo4C, ammo50, unused54, unused58, ammo5C, ammo60;
} AmmoDisplay60;
extern char D_0013D50F[];
extern int D_0013E15A[];
extern u8 D_0014171B[];
extern s32 D_0015EE84 MACRO_ADDR;
extern s32 D_0015EE98 MACRO_ADDR;
extern s32 D_L00_0015F6BC MACRO_ADDR;
extern volatile s32 D_L00_00161260 MACRO_ADDR;
extern void func_001FB530(void);
extern void func_L00_001F3A78(void);
extern int func_001F0FF8(int,int,int,char*);
extern void func_001F0F78(int,int,int,char*);
extern int func_00116248(char*,const char*,...);
extern double promote_float60(float) __asm__("func_00120778");
extern char D_L00_0015F108[];
extern char D_L00_0015F110[];
extern char D_L00_0015F120[];
extern char D_L00_0015F128[];
extern char D_L00_0015F130[];
extern char D_L00_0015F140[];
extern char D_L00_0015F148[];
extern char D_L00_0015F150[];
extern char D_L00_0015F158[];
extern char D_L00_0015F160[];
extern char D_L00_0015F168[];
extern char D_L00_0015F170[];
extern char D_L00_0015F178[];
extern char D_L00_0015F180[];
extern char D_L00_0015F188[];
extern char D_L00_0015F190[];
extern char D_L00_0015F1A0[];
extern char D_L00_0015F1A8[];
extern char D_L00_0015F1B8[];
extern char D_L00_0015F1C0[];
extern char D_L00_0015F1C8[];
extern char D_L00_0015F1D0[];
extern char D_L00_0015F1D8[];
extern char D_L00_0015F1E0[];
extern char D_L00_0015F1E8[];
extern char D_L00_0015F1F0[];
extern char D_L00_0015F1F8[];
extern char D_L00_0015F200[];
extern char D_L00_0015F208[];
extern char D_L00_0015F218[];
extern char D_L00_0015F228[];
extern char D_L00_0015F238[];
extern char D_L00_0015F248[];
extern char D_L00_0015F250[];
extern char D_L00_0015F258[];
extern char D_L00_0015F260[];
extern char D_L00_0015F268[];
extern char D_L00_0015F270[];
extern char D_L00_0015F280[];
extern char D_L00_0015F290[];
extern char D_L00_0015F2A0[];
extern char D_L00_0015F2B0[];
extern char D_L00_0015F2C0[];
extern char D_L00_0015F2C8[];
extern char D_L00_0015F2D8[];
extern char D_L00_0015F2E8[];
extern char D_L00_0015F2F0[];
extern char D_L00_0015F300[];
extern char D_L00_0015F310[];
extern char D_L00_0015F320[];
extern char D_L00_0015F328[];
extern char D_L00_0015F330[];
extern char D_L00_0015F338[];
extern char D_L00_0015F340[];
extern char D_L00_0015F348[];
extern char D_L00_0015F350[];
extern char D_L00_0015F358[];
extern char D_L00_0015F360[];
extern char D_L00_0015F368[];
extern char D_L00_0015F370[];
extern char D_L00_0015F378[];
extern char D_L00_0015F380[];
extern char D_L00_0015F388[];
extern char D_L00_0015F398[];
extern char D_L00_0015F3A8[];
extern char D_L00_0015F3B8[];
extern char *D_L00_00160CD8[];
extern char *D_L00_00160D00[];
extern char *D_L00_00160D68[];
extern char *D_L00_00160D80[];
extern char *D_L00_00160DA0[];
extern char *D_L00_00160DB0[];
extern char *D_L00_00160DC8[];
extern char *D_L00_001C4128[];
extern float D_L00_0015F548 MACRO_ADDR;
extern float D_L00_0015F54C MACRO_ADDR;
extern float D_L00_0015F550 MACRO_ADDR;
extern float D_L00_0015F554 MACRO_ADDR;
extern u8 D_L00_0015F544[] MACRO_ADDR;
extern u8 D_L00_0015F545[] MACRO_ADDR;
extern u8 D_L00_0015F546[] MACRO_ADDR;

static __inline__ int flag_offset60(int flags) {
    if (!flags) return 0;
    return sizeof(char *);
}

void func_L00_001F5C60(void) {
    char buf[16];
    AmmoDisplay60 *ammo;
    u8 *weapon_order;

    func_001FB530();
    if (D_L00_0015F6BC == 0) {
        func_001F0FF8(0x100, 0x10, 0x80C0C000, D_L00_0015F108);
        func_001F0FF8(0x100, 0x18, 0x80C0C000, D_L00_0015F110);
        func_001F0F78(0xC, 0x2C, 0x8000C000, D_L00_0015F120);
        func_001F0F78(0xC, 0x32, 0x8000C000, D_L00_0015F128);
        func_001F0F78(0x14, 0x40, 0x8000C0C0, D_L00_0015F130);
        func_001F0F78(0x6C, 0x40, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.draw_bits & 1)));
        func_001F0F78(0x14, 0x4F, 0x8000C0C0, D_L00_0015F140);
        func_001F0F78(0x6C, 0x4F, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.draw_bits & 2)));
        func_001F0F78(0x14, 0x5E, 0x8000C0C0, D_L00_0015F148);
        func_001F0F78(0x6C, 0x5E, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.draw_bits & 4)));
        func_001F0F78(0x14, 0x6D, 0x8000C0C0, D_L00_0015F150);
        func_001F0F78(0x6C, 0x6D, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.draw_bits & 8)));
        func_001F0F78(0x14, 0x7C, 0x8000C0C0, D_L00_0015F158);
        func_001F0F78(0x6C, 0x7C, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.draw_bits & 0x10)));
        if (debug_menu60.group == 0) {
            func_001F0F78(9, (debug_menu60.item * 0xF) + 0x40, 0x800000C0, D_L00_0015F160);
        }
        func_001F0F78(0xA8, 0x2C, 0x8000C000, D_L00_0015F168);
        func_001F0F78(0xA8, 0x32, 0x8000C000, D_L00_0015F170);
        func_001F0F78(0xB0, 0x40, 0x8000C0C0, D_L00_0015F178);
        func_001F0F78(0x108, 0x40, 0x80F0F0F0, D_L00_00160D00[debug_menu60.camera_mode]);
        func_001F0F78(0xB0, 0x4F, 0x8000C0C0, D_L00_0015F180);
        func_001F0F78(0x108, 0x4F, 0x80F0F0F0, D_L00_001C4128[debug_menu60.display_mode]);
        func_001F0F78(0xB0, 0x5E, 0x8000C0C0, D_L00_0015F188);
        func_001F0F78(0x108, 0x5E, 0x80F0F0F0, D_L00_00160DC8[debug_menu60.occl_mode]);
        func_001F0F78(0xB0, 0x6D, 0x8000C0C0, D_L00_0015F190);
        func_001F0F78(0x108, 0x6D, 0x80F0F0F0, D_L00_00160CD8[(debug_menu60.f20 != 0) ? 0 : 1]);
        func_001F0F78(0xB0, 0x7C, 0x8000C0C0, D_L00_0015F1A0);
        func_001F0F78(0x108, 0x7C, 0x80F0F0F0, D_L00_00160CD8[debug_menu60.f24]);
        func_001F0F78(0xB0, 0x8B, 0x8000C0C0, D_L00_0015F1A8);
        func_001F0F78(0x108, 0x8B, 0x80F0F0F0, D_L00_00160D68[debug_menu60.f28]);
        func_001F0F78(0xB0, 0x9A, 0x8000C0C0, D_L00_0015F1B8);
        func_001F0F78(0x108, 0x9A, 0x80F0F0F0, D_L00_00160D80[debug_menu60.f2C]);
        func_001F0F78(0xB0, 0xA9, 0x8000C0C0, D_L00_0015F1C0);
        func_001F0F78(0x108, 0xA9, 0x80F0F0F0, D_L00_00160DA0[debug_menu60.f30]);
        func_001F0F78(0xB0, 0xB8, 0x8000C0C0, D_L00_0015F1C8);
        func_001F0F78(0x108, 0xB8, 0x80F0F0F0, D_L00_00160DB0[debug_menu60.f34]);
        func_001F0F78(0xB0, 0xC7, 0x8000C0C0, D_L00_0015F1D0);
        if (debug_menu60.selection >= 0) {
            func_00116248(buf, D_L00_0015F1D8, debug_menu60.selection);
            func_001F0F78(0x108, 0xC7, 0x80F0F0F0, buf);
        } else {
            func_001F0F78(0x108, 0xC7, 0x80F0F0F0, D_L00_0015F1E0);
        }
        func_001F0F78(0xB0, 0xD6, 0x8000C0C0, D_L00_0015F1E8);
        {
            int mission = debug_menu60.mission;
            func_00116248(buf, D_L00_0015F1F0, mission, (D_0014171B + 0xAA35)[mission + D_0015EE84 * 16]);
        }
        func_001F0F78(0x108, 0xD6, 0x80F0F0F0, buf);
        if (debug_menu60.group == 1) {
            func_001F0F78(0xA5, (debug_menu60.item * 0xF) + 0x40, 0x800000C0, D_L00_0015F160);
        }
        func_001F0F78(0x158, 0x2C, 0x8000C000, D_L00_0015F1F8);
        func_001F0F78(0x158, 0x32, 0x8000C000, D_L00_0015F170);
        func_001F0F78(0x160, 0x40, 0x8000C0C0, D_L00_0015F200);
        func_001F0F78(0x1CC, 0x40, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.bits & 1)));
        func_001F0F78(0x160, 0x4F, 0x8000C0C0, D_L00_0015F208);
        func_001F0F78(0x1CC, 0x4F, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.bits & 2)));
        func_001F0F78(0x160, 0x5E, 0x8000C0C0, D_L00_0015F218);
        func_001F0F78(0x1CC, 0x5E, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.bits & 4)));
        func_001F0F78(0x160, 0x6D, 0x8000C0C0, D_L00_0015F228);
        func_001F0F78(0x1CC, 0x6D, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.bits & 8)));
        func_001F0F78(0x160, 0x7C, 0x8000C0C0, D_L00_0015F238);
        func_001F0F78(0x1CC, 0x7C, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.bits & 0x10)));
        func_001F0F78(0x160, 0x8B, 0x8000C0C0, D_L00_0015F248);
        func_001F0F78(0x1CC, 0x8B, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.bits & 0x20)));
        func_001F0F78(0x160, 0x9A, 0x8000C0C0, D_L00_0015F250);
        func_001F0F78(0x1CC, 0x9A, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.bits & 0x40)));
        func_001F0F78(0x160, 0xA9, 0x8000C0C0, D_L00_0015F258);
        func_001F0F78(0x1CC, 0xA9, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.bits & 0x80)));
        func_001F0F78(0x160, 0xB8, 0x8000C0C0, D_L00_0015F260);
        func_001F0F78(0x1CC, 0xB8, 0x80F0F0F0, *(char **)((char *)D_L00_00160CD8 + flag_offset60(debug_menu60.bits & 0x100)));
        if (debug_menu60.group == 2) {
            func_001F0F78(0x155, (debug_menu60.item * 0xF) + 0x3E, 0x800000C0, D_L00_0015F160);
        }
        func_001F0F78(0xC, 0xA6, 0x8000C000, D_L00_0015F268);
        func_001F0F78(0xC, 0xAC, 0x8000C000, D_L00_0015F170);
        if (debug_menu60.group == 3) {
            func_001F0F78(9, (debug_menu60.item * 0xF) + 0xB8, 0x800000C0, D_L00_0015F160);
        }
        func_001F0F78(0x14, 0xBA, 0x8000C0C0, D_L00_0015F270);
        weapon_order = (u8 *)D_0013E15A + 0x4C6;
        func_00116248(buf, D_L00_0015F1D8, (s32) weapon_order[0x9]);
        func_001F0F78(0x7A, 0xBA, 0x80406080, buf);
        func_001F0F78(0x14, 0xC9, 0x8000C0C0, D_L00_0015F280);
        func_00116248(buf, D_L00_0015F1D8, (s32) weapon_order[0x15]);
        func_001F0F78(0x7A, 0xC9, 0x80406080, buf);
        func_001F0F78(0x14, 0xD8, 0x8000C0C0, D_L00_0015F290);
        func_00116248(buf, D_L00_0015F1D8, (s32) weapon_order[0x19]);
        func_001F0F78(0x7A, 0xD8, 0x80406080, buf);
        func_001F0F78(0x14, 0xE7, 0x8000C0C0, D_L00_0015F2A0);
        ammo = (AmmoDisplay60 *)(D_0013D50F + 0x21);
        func_00116248(buf, D_L00_0015F1D8, ammo->ammo5C);
        func_001F0F78(0x8C, 0xE7, 0x80F0F0F0, buf);
        func_001F0F78(0x14, 0xF6, 0x8000C0C0, D_L00_0015F2B0);
        func_00116248(buf, D_L00_0015F1D8, ammo->ammo60);
        func_001F0F78(0x8C, 0xF6, 0x80F0F0F0, buf);
        func_001F0F78(0x14, 0x105, 0x8000C0C0, D_L00_0015F2C0);
        func_00116248(buf, D_L00_0015F1D8, ammo->ammo28);
        func_001F0F78(0x8C, 0x105, 0x80F0F0F0, buf);
        func_00116248(buf, D_L00_0015F1D8, (s32) weapon_order[0xA]);
        func_001F0F78(0x7A, 0x105, 0x80406080, buf);
        func_001F0F78(0x14, 0x114, 0x8000C0C0, D_L00_0015F2C8);
        func_00116248(buf, D_L00_0015F1D8, ammo->ammo2C);
        func_001F0F78(0x8C, 0x114, 0x80F0F0F0, buf);
        func_00116248(buf, D_L00_0015F1D8, (s32) weapon_order[0xB]);
        func_001F0F78(0x7A, 0x114, 0x80406080, buf);
        func_001F0F78(0x14, 0x123, 0x8000C0C0, D_L00_0015F2D8);
        func_00116248(buf, D_L00_0015F1D8, ammo->ammo34);
        func_001F0F78(0x8C, 0x123, 0x80F0F0F0, buf);
        func_001F0F78(0x14, 0x132, 0x8000C0C0, D_L00_0015F2E8);
        func_00116248(buf, D_L00_0015F1D8, ammo->ammo50);
        func_001F0F78(0x8C, 0x132, 0x80F0F0F0, buf);
        func_00116248(buf, D_L00_0015F1D8, (s32) weapon_order[0x14]);
        func_001F0F78(0x7A, 0x132, 0x80406080, buf);
        func_001F0F78(0x14, 0x141, 0x8000C0C0, D_L00_0015F2F0);
        func_00116248(buf, D_L00_0015F1D8, ammo->ammo40);
        func_001F0F78(0x8C, 0x141, 0x80F0F0F0, buf);
        func_00116248(buf, D_L00_0015F1D8, (s32) weapon_order[0x10]);
        func_001F0F78(0x7A, 0x141, 0x80406080, buf);
        func_001F0F78(0x14, 0x150, 0x8000C0C0, D_L00_0015F300);
        func_00116248(buf, D_L00_0015F1D8, ammo->ammo4C);
        func_001F0F78(0x8C, 0x150, 0x80F0F0F0, buf);
        func_00116248(buf, D_L00_0015F1D8, (s32) weapon_order[0x13]);
        func_001F0F78(0x7A, 0x150, 0x80406080, buf);
        func_001F0F78(0x14, 0x15F, 0x8000C0C0, D_L00_0015F310);
        func_00116248(buf, D_L00_0015F1D8, ammo->ammo44);
        func_001F0F78(0x8C, 0x15F, 0x80F0F0F0, buf);
        func_00116248(buf, D_L00_0015F1D8, (s32) weapon_order[0x11]);
        func_001F0F78(0x7A, 0x15F, 0x80406080, buf);
        func_001F0F78(0x14, 0x16E, 0x8000C0C0, D_L00_0015F320);
        func_00116248(buf, D_L00_0015F1D8, ammo->ammo3C);
        func_001F0F78(0x8C, 0x16E, 0x80F0F0F0, buf);
        func_00116248(buf, D_L00_0015F1D8, (s32) weapon_order[0xF]);
        func_001F0F78(0x7A, 0x16E, 0x80406080, buf);
        func_001F0F78(0x14, 0x17D, 0x8000C0C0, D_L00_0015F328);
        func_00116248(buf, D_L00_0015F1D8, D_0015EE98);
        func_001F0F78(0x8C, 0x17D, 0x80F0F0F0, buf);
        func_001F0F78(0xBC, 0x100, 0x8000C000, D_L00_0015F330);
        func_001F0F78(0xBC, 0x106, 0x8000C000, D_L00_0015F338);
        if (debug_menu60.group == 4) {
            func_001F0F78(0xB9, (debug_menu60.item * 0xF) + 0x112, 0x800000C0, D_L00_0015F160);
        }
        func_001F0F78(0xC4, 0x114, 0x8000C0C0, D_L00_0015F340);
        func_00116248(buf, D_L00_0015F348, promote_float60(D_L00_0015F548 * 0.0009765625f));
        func_001F0F78(0x11C, 0x114, 0x80F0F0F0, buf);
        func_001F0F78(0xC4, 0x123, 0x8000C0C0, D_L00_0015F350);
        func_00116248(buf, D_L00_0015F348, promote_float60(100.0f - (D_L00_0015F550 / 2.55f)));
        func_001F0F78(0x11C, 0x123, 0x80F0F0F0, buf);
        func_001F0F78(0xC4, 0x132, 0x8000C0C0, D_L00_0015F358);
        func_00116248(buf, D_L00_0015F348, promote_float60(D_L00_0015F54C * 0.0009765625f));
        func_001F0F78(0x11C, 0x132, 0x80F0F0F0, buf);
        func_001F0F78(0xC4, 0x141, 0x8000C0C0, D_L00_0015F360);
        func_00116248(buf, D_L00_0015F348, promote_float60(100.0f - (D_L00_0015F554 / 2.55f)));
        func_001F0F78(0x11C, 0x141, 0x80F0F0F0, buf);
        func_001F0F78(0xC4, 0x150, 0x8000C0C0, D_L00_0015F368);
        func_00116248(buf, D_L00_0015F1D8, (s32) D_L00_0015F544[0]);
        func_001F0F78(0x11C, 0x150, 0x80F0F0F0, buf);
        func_001F0F78(0xC4, 0x15F, 0x8000C0C0, D_L00_0015F370);
        func_00116248(buf, D_L00_0015F1D8, (s32) D_L00_0015F545[0]);
        func_001F0F78(0x11C, 0x15F, 0x80F0F0F0, buf);
        func_001F0F78(0xC4, 0x16E, 0x8000C0C0, D_L00_0015F378);
        func_00116248(buf, D_L00_0015F1D8, (s32) D_L00_0015F546[0]);
        func_001F0F78(0x11C, 0x16E, 0x80F0F0F0, buf);
        func_001F0F78(0x160, 0x100, 0x8000C000, D_L00_0015F380);
        func_001F0F78(0x160, 0x106, 0x8000C000, D_L00_0015F170);
        if (debug_menu60.group == 5) {
            func_001F0F78(0x15D, (debug_menu60.item * 0xF) + 0x112, 0x800000C0, D_L00_0015F160);
        }
        func_001F0F78(0x168, 0x114, 0x8000C0C0, D_L00_0015F388);
        func_001F0F78(0x1CA, 0x114, 0x80F0F0F0, D_L00_00160CD8[(debug_menu60.fF0 != 0) ? 0 : 1]);
        func_001F0F78(0x168, 0x123, 0x8000C0C0, D_L00_0015F398);
        func_001F0F78(0x1CA, 0x123, 0x80F0F0F0, D_L00_00160CD8[debug_menu60.fF4]);
        func_001F0F78(0x168, 0x132, 0x8000C0C0, D_L00_0015F3A8);
        buf[1] = 0;
        buf[0] = debug_menu60.point_timer;
        func_001F0F78(0x1CA, 0x132, 0x80F0F0F0, buf);
        func_001F0F78(0x168, 0x141, 0x8000C0C0, D_L00_0015F3B8);
        buf[1] = 0;
        buf[0] = debug_menu60.saved_timer;
        func_001F0F78(0x1CA, 0x141, 0x80F0F0F0, buf);
        func_L00_001F3A78();
        while (D_L00_00161260 & 1) {}
    }
}
