#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"
#include "hero.h"

/*
 * menu.cpp in the original source; text 0x2071A8-0x209A60.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/* Declarations in scope here before the split. */
extern char voice_pool[];
extern int bank_sound_count;
extern void FastMemCopy(void *, void *, int);
extern char backup_cam[];
extern char gCamBackupState[];
extern void *cur_cam NOT_SDA;
extern DispatchRec camera_types[];
extern int draw_config[];
extern void FastMemSet();
extern void BuildOcclVisibility(void);
extern int occl_update NOT_SDA;
extern char occl_visibility[];
extern long gs_display_settings[3];
extern int space_load_in_progress;
extern short draw_enable_mask;
extern void append_gif_transfer_packet(void);
extern void DrawDebugProfiler(void);
extern int draw_callback_count;
extern int draw_callback_args[];
extern int draw_callback_funcs[];
extern short GlobalStateFlag;
extern int measure_text_width(unsigned char *arg0, int arg1, void *arg2);
extern unsigned char normal_font_metrics[];
extern unsigned char small_font_metrics[];
extern unsigned char large_font_metrics[];
extern void FontPrint(void *, void *, void *, void *, void *, int,
                          unsigned char *);
extern int measure_text_width_regular(unsigned char *, int);
extern int measure_text_width_small(unsigned char *, int);
extern int GetEffectTex(int);
extern void PutDrawBufferLarge(void);
extern void InitViewContext(void);
extern void UpdateViewContext(void);
extern int bit_swap_lut[];
extern long background_rgbaq NOT_SDA;
extern int Help_FindIndex(void);
extern char s_paradox_this_message_does_not[];
extern short help_msgs;
extern int hud_heap_base NOT_SDA;
extern int hud_state NOT_SDA;
extern int Hud_GetIconIndex(int);
typedef struct {
    char b[0x13];
} Cfg13;
extern Cfg13 message_text_buffer NOT_SDA;
extern Cfg13 message_too_long_label NOT_SDA;
extern int libc_strlen(void);
extern void libc_strcpy(Cfg13 *, void *);
extern short ResourceCounter;
extern void draw_stretchable_ui_frame(int, int, int, int, int);
extern void CreatePointLight(int);
extern void DetachPointLight(int);
extern void update_world_object_animation(void *);
extern int render_sequence NOT_SDA;
extern int vu1_chain_buffer_2 NOT_SDA;
extern int current_vu_chain;
extern int map_slot_use[];
extern void move_map_entry_slot(int a, int b);
extern PadSlots level_map_selection_slots __asm__("D_001A01F0");
extern int level_map_selection[];
extern int *menu_level_indices;

/* Map hit test: point_is_clockwise of (x1, y1) against one fixed map edge. */
int Menu_IsPointInFixedQuad(int x1, int y1) {
    return point_is_clockwise(x1, y1, 0xD3, 0xDB, 0x129, 0xF9);
}

extern unsigned char menu_item_enabled_0 NOT_SDA;

/* Map filter: shown once menu item flag 0 is set. */
int map_filter_l01_1(void) {
    return menu_item_enabled_0 != 0;
}

extern unsigned char g_menu_item_enabled_1 NOT_SDA;

/* Map filter: shown once menu item flag 1 is set. */
int map_filter_l01_2(void) {
    return g_menu_item_enabled_1 != 0;
}

extern unsigned char menu_item_enabled_9 NOT_SDA;

/* Map filter: shown once menu item flag 9 is set. */
int map_filter_l02_0(void) {
    return menu_item_enabled_9 != 0;
}

typedef struct {
    char _pad0[0x12E4];
    unsigned char unk12E4;
    char _pad12E5[0x208C - 0x12E5];
    int unk208C;
} Menu13F450;
extern Menu13F450 g_hero;

/* Menu hit test: arg3 is the third float ($f14). b (state 16) is
   computed before a, as retail evaluates it; the second if's own
   `arg1 >= 0xC8` is retail's second test of $a1. */
int map_filter_l01_3(void *unused_input, int reveal_test_selector, float unused1, float unused2, float reveal_test_value) {
    Hero *hero_ptr = (Hero *)&g_hero;
    int hero_state_blocks_high_range_reveal = hero_ptr->state.control_mode == 16;
    int hero_state_blocks_low_range_reveal = hero_ptr->state.control_mode == 17 || hero_ptr->state.control_mode == 18 || hero_ptr->base_condition == 1;

    if (reveal_test_selector < 0xC8 && reveal_test_value >= 39.5f && reveal_test_value <= 42.5f && !hero_state_blocks_low_range_reveal) {
        return 1;
    }
    if (reveal_test_selector >= 0xC8 && reveal_test_value >= 87.0f && !hero_state_blocks_high_range_reveal) {
        return 1;
    }
    return 0;
}

extern int hero_control_mode_early __asm__("D_001414DC") NOT_SDA;
extern unsigned char menu_item_enabled_2 NOT_SDA;

/* `if (x) return 1;` in both arms with one shared `return 0;` keeps the
   flag test a beqz with the li in its slot; a result variable or a
   return 0 per arm becomes sltu. */
int map_filter_l01_4(int arg0, float unused1, float unused2, float arg1) {
    int is16 = hero_control_mode_early == 0x10;

    if (arg0 < 0x100) {
        if (menu_item_enabled_2 != 0) {
            return 1;
        }
    } else if (arg1 >= 58.0f && arg1 <= 86.0f && !is16) {
        return 1;
    }
    return 0;
}

/* The struct's address in a local keeps one base register for both
   field reads. */
int map_filter_l01_5(int reveal_test_selector, float unused1, float unused2, float reveal_test_value) {
    if (reveal_test_selector < 0x100) {
        struct Hero *hero_ptr = (struct Hero *)&g_hero;
        return hero_ptr->state.control_mode == 17 || hero_ptr->state.control_mode == 18 || hero_ptr->base_condition == 1;
    }
    return (reveal_test_value >= 95.0f) ? 1 : 0;
}

extern short menu_selection_count;   /* declared small so -G2 puts it in SDA */

/* Map filter: shown while the menu selection count is zero. */
int map_filter_l03_0(void) {
    return *(int *)&menu_selection_count == 0;
}

extern unsigned char menu_item_enabled_16 NOT_SDA;

/* Map filter: shown once menu item flag 16 is set. */
int map_filter_l04_0(void) {
    return menu_item_enabled_16 != 0;
}

extern unsigned char menu_item_enabled_17 NOT_SDA;

/* Map filter: shown once menu item flag 17 is set. */
int map_filter_l04_1(void) {
    return menu_item_enabled_17 != 0;
}

extern unsigned char menu_item_enabled_18 NOT_SDA;

/* Map filter: shown once menu item flag 18 is set. */
int map_filter_l04_2(void) {
    return menu_item_enabled_18 != 0;
}

extern unsigned char menu_item_enabled_19 NOT_SDA;

/* Map filter: shown once menu item flag 19 is set. */
int map_filter_l04_3(void) {
    return menu_item_enabled_19 != 0;
}

extern unsigned char menu_item_enabled_25 NOT_SDA;

/* Map filter: shown once menu item flag 25 is set. */
int map_filter_l06_0(void) {
    return menu_item_enabled_25 != 0;
}

typedef u32 u128 __attribute__((mode(TI), aligned(16)));
struct RegionPlayerState {
    u8 pad0[0x12E4];
    u8 region_mode;
    u8 pad12E5[0x208C - 0x12E5];
    s32 active_state;
};
extern struct RegionPlayerState g_hero_r __asm__("D_0013F450");
extern s32 region_enabled[];
union RegionVector {
    u128 region_center;
    f32 f[4];
};
extern s32 point_is_clockwise(s32, s32, s32, s32, s32, s32);
extern f32 vec_distance_xy_r(union RegionVector *, union RegionVector *) __asm__("func_001F9D48");
s32 map_filter_l07_0(s32 projected_x, s32 projected_y, f32 x, f32 y, f32 z);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/textbin/fun_00206bd8.c, passes_projected_region_callback_0. */
s32 map_filter_l07_0(s32 projected_x, s32 projected_y, f32 x, f32 y, f32 z) {
    s32 special_state;

    if (point_is_clockwise(projected_x, projected_y, 0x189, 0x16F, 0x87, 0xED)) {
        if (point_is_clockwise(projected_x, projected_y, 0x75, 0x146, 0x18B, 0x1CC) &&
            point_is_clockwise(projected_x, projected_y, 0x141, 0x130, 0xB0, 0x130)) {
            if (43.9f <= z) {
                return 1;
            }
            return 0;
        }
    } else if (point_is_clockwise(projected_x, projected_y, 0x191, 0xCD, 0xD2, 0x13B)) {
        union RegionVector position;
        union RegionVector region_center;

        position.region_center = 0;
        region_center.region_center = 0;
        position.f[0] = x;
        position.f[1] = y;
        region_center.f[0] = 337.5f;
        region_center.f[1] = 250.0f;
        if (vec_distance_xy_r(&position, &region_center) <= 7.0f) {
            return 1;
        }
        if (point_is_clockwise(projected_x, projected_y, 0x142, 0x12A, 0x173, 0xFC) && region_enabled[0]) {
            return 1;
        }
        return 0;
    } else {
        special_state = 0;
        if ((u32)(g_hero_r.active_state - 0x11) < 2 || g_hero_r.region_mode == 1) {
            special_state = 1;
        }
        if (special_state && point_is_clockwise(projected_x, projected_y, 0x12B, 0xB8, 0x13A, 0xF2) &&
            point_is_clockwise(projected_x, projected_y, 0x130, 0xED, 0x165, 0xCB) &&
            point_is_clockwise(projected_x, projected_y, 0x161, 0xDC, 0x13B, 0xB0) &&
            point_is_clockwise(projected_x, projected_y, 0x156, 0xA8, 0x119, 0xD1)) {
            return 1;
        }
    }
    return 0;
}

extern int map_region_enabled_0 NOT_SDA;

/* Menu hit test (func_00207200's family: `a` is the Menu13F450 state test
   shared with func_00207340/func_00207930). Two independent ways to hit:
   the first box with arg1 >= 0x135 and D_001A04A8 set and arg3 >= 47.7,
   which can succeed even when `a` is true; or, only when `a` is true,
   all three of a second group of boxes hit. */
int map_filter_l07_1(int arg0, int arg1, float unused1, float unused2,
                   float arg3) {
    Menu13F450 *s = &g_hero;
    int a = s->unk208C == 17 || s->unk208C == 18 || s->unk12E4 == 1;

    if (point_is_clockwise(arg0, arg1, 0x93, 0x168, 0x182, 0x168) != 0
        && arg1 >= 0x135 && map_region_enabled_0 != 0 && arg3 >= 47.7f) {
        return 1;
    }
    if (!a) {
        return 0;
    }
    if (point_is_clockwise(arg0, arg1, 0xC5, 0x9A, 0x13C, 0xE1) != 0
        && point_is_clockwise(arg0, arg1, 0xD6, 0xC3, 0x157, 0xC5) != 0
        && point_is_clockwise(arg0, arg1, 0x107, 0xDA, 0x171, 0xA0) != 0) {
        return 1;
    }
    return 0;
}

/* Menu hit test. Left of x 0xE9 (arg1 < 0xE9) the point must be inside
   all four func_00209048 boxes. Otherwise it needs a hit in each of
   three pairs of boxes, where both boxes of a pair are always tested.
   Each pair's results are locals of their own block: the first one then
   lives in one basic block across the second call, so local-alloc gives
   it $s0 before the arguments are allocated, as retail has it. The two
   compound conditions (`&&` chain, `!a && !b`) keep the return values
   as branches instead of an sltu. */
int map_filter_l07_2(int arg0, int arg1) {
    if (arg1 < 0xE9) {
        if (point_is_clockwise(arg0, arg1, 0x132, 0xA0, 0x15F, 0xD8)
            && point_is_clockwise(arg0, arg1, 0x14D, 0xD8, 0x181, 0x9A)
            && point_is_clockwise(arg0, arg1, 0x182, 0xB4, 0x137, 0x93)
            && point_is_clockwise(arg0, arg1, 0x157, 0x8C, 0x130, 0xAA)) {
            return 1;
        }
        return 0;
    }
    {
        int a = point_is_clockwise(arg0, arg1, 0x8F, 0x115, 0x148, 0x14B);
        int b = point_is_clockwise(arg0, arg1, 0xE7, 0x108, 0x127, 0x164);
        if (a == 0 && b == 0) {
            return 0;
        }
    }
    {
        int a = point_is_clockwise(arg0, arg1, 0xED, 0x15F, 0x154, 0x10E);
        int b = point_is_clockwise(arg0, arg1, 0xA2, 0x12B, 0x16F, 0x147);
        if (a == 0 && b == 0) {
            return 0;
        }
    }
    {
        int a = point_is_clockwise(arg0, arg1, 0x132, 0x163, 0x141, 0xCC);
        int b = point_is_clockwise(arg0, arg1, 0xC2, 0x108, 0x1A0, 0x12E);
        if (!a && !b) {
            return 0;
        }
        return 1;
    }
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

extern int map_region_enabled_5 NOT_SDA;

/* Menu hit test in two layouts (func_00207200's family). The first arm
   needs its own `return 0`: jump.c then sets v0 = 1 before the test
   there too, which gives retail's registers and lets the arm be
   cross-jumped into the second one. */
int map_filter_l07_3(int px, int py, float unused1, float unused2, float reveal_test_value) {
    if (py < 0xBB) {
        Hero *hero_ptr = (Hero *)&g_hero;
        int hero_state_blocks_low_y_reveal = hero_ptr->state.control_mode == 17 || hero_ptr->state.control_mode == 18 || hero_ptr->base_condition == 1;

        if (!hero_state_blocks_low_y_reveal && map_region_enabled_5 != 0) {
            return 1;
        }
        return 0;
    }
    if (reveal_test_value >= 51.5f && reveal_test_value <= 54.0f
        && point_is_clockwise(px, py, 0x10A, 0xE5, 0x124, 0xF9) != 0) {
        return 1;
    }
    return 0;
}

extern int map_region_enabled_3 NOT_SDA;

/* Same hit test as func_002071A8, run against two boxes: nonzero when
   D_001A04B4 is set and either box is hit. Retail sets the result before
   each test (0, 1, 0, then 1 on the fall-through), so the C does the same.
   The last store goes through a short: jump.c folds `r = 0; if (b) r = 1;`
   into an sltu only when the store sets a whole register, and a subreg
   store keeps retail's beqz. */
int map_filter_l07_4(int x1, int y1) {
    int a = point_is_clockwise(x1, y1, 0x99, 0xED, 0x160, 0x117);
    int b = point_is_clockwise(x1, y1, 0x10E, 0xF7, 0x13D, 0x119);
    int r = 0;

    if (map_region_enabled_3 != 0) {
        r = 1;
        if (a == 0) {
            r = 0;
            if (b != 0) {
                *(short *)&r = 1;
            }
        }
    }
    return r;
}

extern int map_region_enabled_6 NOT_SDA;
extern int map_region_enabled_4 NOT_SDA;

/* Hit test in two layouts: arg3 (the third float, $f14) must lie in
   [73.5, 80] with D_001A04C0 set, or for arg1 >= 0x9D in [51.5, 54] with
   D_001A04B8 set. Written as `<`/`>` rejections so they compile to retail's
   c.lt/bc1t; the second arm's `return 0` cross-jumps into the first's. */
int map_filter_l07_5(void *arg0, int arg1, float unused1, float unused2, float arg3) {
    if (arg1 < 0x9D) {
        if (arg3 < 73.5f || arg3 > 80.0f || map_region_enabled_6 == 0) {
            return 0;
        }
        return 1;
    } else {
        if (arg3 < 51.5f || arg3 > 54.0f || map_region_enabled_4 == 0) {
            return 0;
        }
        return 1;
    }
}

extern int map_region_enabled_1 NOT_SDA;
extern int point_is_clockwise(int, int, int, int, int, int);

/* Menu hit test (func_00207200's family): for arg1 >= 0x105, needs
   D_001A04AC set and arg3 >= 47.7; otherwise returns `a` directly when
   arg1 < 0xC1, else `a` unless func_00209048's box test hits (which
   clears it to 0). */
int map_filter_l07_6(int px, int py, float fparg0, float fparg1, float reveal_test_value) {
    Hero *hero = (Hero *)&g_hero;
    int hit = hero->state.control_mode == 17 || hero->state.control_mode == 18 || hero->base_condition == 1;
    int result;

    if (py >= 0x105) {
        if (map_region_enabled_1 == 0) {
            return 0;
        }
        return (reveal_test_value >= 47.7f) ? 1 : 0;
    }
    if (py < 0xC1) {
        return hit;
    }
    result = hit;
    if (point_is_clockwise(px, py, 0xD9, 0xB8, 0x156, 0xD2) != 0) {
        result = 0;
    }
    return result;
}

/* Menu hit test (func_00207200's family): hits unless the menu state
   blocks it, arg3 ($f14) is below 71.5, or the first box is hit, and
   then only if the second box is not. */
s32 map_filter_l07_7(s32 px, s32 py, f32 fparg0, f32 fparg1, f32 reveal_test_value) {
    struct Hero *hero = (struct Hero *)&g_hero;
    s32 var_4_15 = hero->state.control_mode == 17 || hero->state.control_mode == 18 || hero->base_condition == 1;

    if (var_4_15) {
        return 0;
    }
    if (reveal_test_value < 71.5f) {
        return 0;
    }
    if (point_is_clockwise(px, py, 0x131, 0xE2, 0xC6, 0x93) != 0) {
        return 0;
    }
    return point_is_clockwise(px, py, 0x190, 0x89, 0xD1, 0xFB) == 0;
}

extern unsigned char menu_item_enabled_41 NOT_SDA;
extern int hero_control_mode NOT_SDA;

/* Map filter: above y 0x100 needs hero control mode 15, else menu item flag 41. */
int map_filter_l08_0(int arg0, int arg1) {
    if (arg1 >= 0x101) {
        return hero_control_mode == 0xF;
    }
    return menu_item_enabled_41 != 0;
}

/* The two unused float parameters put the threshold's argument in
   $f14 (floats count consecutively from $f12). The nops after the mtc1
   and the compare are ps2eeas's (tools/ps2eeas_nops.py). */
int map_filter_l08_1(int arg0, float unused1, float unused2, float arg1) {
    if (arg0 < 0xE0) {
        return (arg1 >= 47.75f) ? 1 : 0;
    }
    return (arg1 < 29.0f) ? 1 : 0;
}

/* `(a && b) ? 1 : 0` gives retail's bc1f then bc1tl; the other arm is
   a plain `? 1 : 0`. */
int map_filter_l08_2(int arg0, float unused1, float unused2, float arg1) {
    if (arg0 < 0xE0) {
        return (arg1 >= 44.0f && arg1 <= 45.0f) ? 1 : 0;
    }
    return (arg1 >= 37.0f) ? 1 : 0;
}

/* func_00207D38's twin. */
int map_filter_l08_3(int arg0, float unused1, float unused2, float arg1) {
    if (arg0 < 0xE0) {
        return (arg1 >= 42.0f && arg1 <= 43.0f) ? 1 : 0;
    }
    return (arg1 >= 39.0f) ? 1 : 0;
}

/* Returning the compare as `? 1 : 0` gives bc1t with the `li 1` in its
   slot; folded into `&&` it becomes bc1tl. The two nops, after the mtc1
   and after the compare, are ps2eeas's (tools/ps2eeas_nops.py). */
int map_filter_l08_4(int arg0, float unused1, float unused2, float arg1) {
    if (arg0 >= 0xE0) {
        return 0;
    }
    return (arg1 <= 38.0f) ? 1 : 0;
}

extern unsigned char menu_item_enabled_36 NOT_SDA;
extern unsigned char menu_item_enabled_37 NOT_SDA;
extern unsigned char menu_item_enabled_38 NOT_SDA;
extern unsigned char menu_item_enabled_55 NOT_SDA;
extern unsigned char menu_item_enabled_56 NOT_SDA;
extern unsigned char menu_item_enabled_57 NOT_SDA;

/* Map filter: shown once menu item flag 36 is set. */
int map_filter_l08_5(void) {
    return menu_item_enabled_36 != 0;
}

/* Map filter: shown once menu item flag 37 is set. */
int map_filter_l08_6(void) {
    return menu_item_enabled_37 != 0;
}

/* Map filter: shown once menu item flag 38 is set. */
int map_filter_l08_7(void) {
    return menu_item_enabled_38 != 0;
}

/* Map filter: shown once menu item flag 55 is set. */
int map_filter_l09_0(void) {
    return menu_item_enabled_55 != 0;
}

/* Map filter: shown once menu item flag 56 is set. */
int map_filter_l09_1(void) {
    return menu_item_enabled_56 != 0;
}

/* Map filter: shown once menu item flag 57 is set. */
int map_filter_l09_2(void) {
    return menu_item_enabled_57 != 0;
}

extern unsigned char menu_item_enabled_68;

extern unsigned char menu_item_enabled_68_far __asm__("D_0013D4E0") NOT_SDA;
/* The recorded C was right; the two ps2eeas nops were the only
   residual. */
int map_filter_l10_0(int arg0, float unused1, float unused2, float arg1) {
    if (arg0 >= 0xBE) return menu_item_enabled_68_far != 0;
    return (arg1 >= 58.5f) ? 1 : 0;
}

extern unsigned char menu_item_enabled_64 NOT_SDA;
extern unsigned char menu_item_enabled_65 NOT_SDA;
extern unsigned char menu_item_enabled_66 NOT_SDA;
extern unsigned char menu_item_enabled_67 NOT_SDA;
extern unsigned char menu_item_enabled_69 NOT_SDA;

/* Map filter: shown once menu item flag 64 is set. */
int map_filter_l10_1(void) {
    return menu_item_enabled_64 != 0;
}

/* Map filter: shown once menu item flag 65 is set. */
int map_filter_l10_2(void) {
    return menu_item_enabled_65 != 0;
}

/* Map filter: shown once menu item flag 66 is set. */
int map_filter_l10_3(void) {
    return menu_item_enabled_66 != 0;
}

/* Map filter: shown once menu item flag 67 is set. */
int map_filter_l10_4(void) {
    return menu_item_enabled_67 != 0;
}

/* Map filter: shown once menu item flag 69 is set. */
int map_filter_l10_5(void) {
    return menu_item_enabled_69 != 0;
}

extern unsigned char menu_item_enabled_74 NOT_SDA;

/* Map filter: height band 232-235 plus flag 74 below 0xDD, else height at most 180. */
int map_filter_l11_0(int arg0, float unused1, float unused2, float arg1) {
    if (arg0 < 0xDD) {
        return arg1 >= 232.0f && arg1 <= 235.0f && menu_item_enabled_74 != 0;
    }
    return (arg1 <= 180.0f) ? 1 : 0;
}

extern unsigned char menu_item_enabled_75 NOT_SDA;

/* Map filter: height at least 242 below 0xDD, else height at most 180 plus flag 75. */
int map_filter_l11_1(int arg0, float unused1, float unused2, float arg1) {
    if (arg0 < 0xDD) {
        return (arg1 >= 242.0f) ? 1 : 0;
    }
    return arg1 <= 180.0f && menu_item_enabled_75 != 0;
}

extern unsigned char menu_item_enabled_76 NOT_SDA;

/* Map filter: height band 238-241 below 0xDD, else height at most 180 plus flag 76. */
int map_filter_l11_2(int arg0, float unused1, float unused2, float arg1) {
    if (arg0 < 0xDD) {
        return (arg1 >= 238.0f && arg1 <= 241.0f) ? 1 : 0;
    }
    return arg1 <= 180.0f && menu_item_enabled_76 != 0;
}

extern unsigned char menu_item_enabled_78 NOT_SDA;

/* The Menu13F450 state test of func_00207340, computed up front (retail
   evaluates it before the arg0 branch), gates the arg0 < 0x15F arm; the
   other arm is func_00208160's second test with D_0013D4EA. */
int map_filter_l11_3(int reveal_test_selector, float unused1, float unused2, float reveal_test_value) {
    struct Hero *hero_ptr = (struct Hero *)&g_hero;
    int hero_state_allows_reveal = hero_ptr->state.control_mode == 17 || hero_ptr->state.control_mode == 18 || hero_ptr->base_condition == 1;

    if (reveal_test_selector < 0x15F) {
        return (reveal_test_value >= 200.0f) ? hero_state_allows_reveal : 0;
    }
    return reveal_test_value >= 233.0f && reveal_test_value <= 235.0f && menu_item_enabled_78 != 0;
}

extern unsigned char menu_item_enabled_79 NOT_SDA;

/* Map filter: height band 228-230 below 0x15F, else band 233-235 plus flag 79. */
int map_filter_l11_4(int arg0, float unused1, float unused2, float arg1) {
    if (arg0 < 0x15F) {
        return (arg1 >= 228.0f && arg1 <= 230.0f) ? 1 : 0;
    }
    return arg1 >= 233.0f && arg1 <= 235.0f && menu_item_enabled_79 != 0;
}

extern unsigned char menu_item_enabled_77 NOT_SDA;

/* Map filter: shown once menu item flag 77 is set. */
int map_filter_l11_5(void) {
    return menu_item_enabled_77 != 0;
}

/* arg0 is unused; the two unused float parameters put arg3 in $f14
   (floats count consecutively from $f12, one register each; see
   func_00207CE0). With `r` defaulting to 1, reorg turns the reset into
   retail's bc1fl with `r = 0` in its delay slot. The nop after the mtc1
   is ps2eeas's (tools/ps2eeas_nops.py). */
int map_filter_l12_0(void *arg0, int arg1, float unused1, float unused2, float arg3) {
    int r = 1;

    if (arg1 < 0x141 && !(63.5f <= arg3)) {
        r = 0;
    }
    return r;
}

/* Map filter that always returns 1. */
int MenuL12_IsAlwaysActive1(void) {
    return 1;
}

/* Map filter that always returns 1. */
int map_filter_l12_2(void) {
    return 1;
}

typedef float FVec4[4] __attribute__((aligned(16)));
extern float vec_distance_xy(float *, float *);

/* True when (x, y) is within 35 of the point (161.5, 249), or always
   while D_001A04A8 is set. Both vectors are 16-byte aligned like the
   SDK's sceVu0FVECTOR; a partial initializer then clears the whole
   quadword with one por/sq (a plain float[4] gets a memset call). */
int map_filter_l15_0(int x, int y) {
    if (map_region_enabled_0 == 0) {
        FVec4 p = { (float)x, (float)y };
        FVec4 c = { 0 };

        c[0] = 161.5f;
        c[1] = 249.0f;
        return vec_distance_xy(p, c) <= 35.0f ? 1 : 0;
    }
    return 1;
}

extern unsigned char menu_item_enabled_102 NOT_SDA;

/* Map filter: shown once menu item flag 102 is set. */
int map_filter_l16_0(void) {
    return menu_item_enabled_102 != 0;
}

extern unsigned char menu_item_enabled_103 NOT_SDA;

/* Map filter: shown once menu item flag 103 is set. */
int map_filter_l16_1(void) {
    return menu_item_enabled_103 != 0;
}

extern unsigned char menu_item_enabled_104 NOT_SDA;
extern unsigned char menu_item_enabled_105 NOT_SDA;
extern unsigned char menu_item_enabled_115 NOT_SDA;

/* Map filter: shown once menu item flag 104 is set. */
int map_filter_l16_2(void) {
    return menu_item_enabled_104 != 0;
}

/* Map filter: shown once menu item flag 105 is set. */
int map_filter_l16_3(void) {
    return menu_item_enabled_105 != 0;
}

/* Map filter: shown once menu item flag 115 is set. */
int map_filter_l18_0(void) {
    return menu_item_enabled_115 != 0;
}

extern int level_map_selection[];
extern int max_map_coverage_size[];
extern void DebugEnter_Nop(void);
extern int compress_map_coverage(void *dst, int size, int a, int b);
extern void downsample_map_coverage(void *dst);

/* The same object, twice. gp is 0x00166D00, so the `lw $4,-0x7E7C($28)`
   in retail's delay slot and the `lui/%lo(D_0015EE84)` a few
   instructions later are the SAME address; retail reaches it both ways
   inside one function. Declared small it lands in SDA at -G2, declared
   int it stays out. */
extern int g_current_level_index MACRO_ADDR;

/* Read a sector into arg0 from the open file (D_001A01F0 slot state) or
   clear it when none is open, and track the largest read per slot in
   D_0013D668. Adapted from Lombyte's project (GPL-3.0) for PAL. */
void save_map_coverage(int arg0) {
    int n;

    DebugEnter_Nop();
    if (level_map_selection[0xA] == 0) {
        FastMemSet((void *)arg0, 0, 0x800);
        return;
    }
    n = compress_map_coverage((void *)arg0, 0x800, level_map_selection[5], level_map_selection[3]);
    if (n == -1) {
        downsample_map_coverage((void *)arg0);
    }
    if (max_map_coverage_size[g_current_level_index] < n) {
        max_map_coverage_size[g_current_level_index] = n;
    }
}

extern int map_coverage_active[] NOT_SDA;
extern void decode_compressed_occlusion_map(void *, unsigned char *, int);
extern void expand_map_coverage(void *, unsigned char *);

/* Load a saved map coverage chunk, decoding it as compressed or plain by its first bit. */
void load_map_chunk(void *arg0, unsigned char *arg1, int arg2) {
    /* func_00209040 is empty in retail and takes no arguments; $4/$5/$6
       still hold our own incoming arguments across it, which is why retail
       saves them into $16-$18 rather than reloading. */
    DebugEnter_Nop();
    if (!map_coverage_active[0]) {
        return;
    }
    if (*arg1 & 1) {
        decode_compressed_occlusion_map(arg0, arg1, arg2);
    } else {
        expand_map_coverage(arg0, arg1);
    }
}

/* Decodes the run-length compressed 1-bit occlusion map into 0x8000 bytes at dst: runs are expanded one byte per bit in the scratchpad, packed eight to a byte and copied out 0x400 bytes at a time. */
void decode_compressed_occlusion_map(void *dst, unsigned char *ctrl, int spans) {
    unsigned char *dst_end;
    unsigned char *end;
    unsigned char *span;
    unsigned char *cursor;
    unsigned char *next_dst;
    unsigned char *next;
    unsigned char *wrapped;
    unsigned char *r;
    unsigned char *w;
    unsigned char *from;
    unsigned char *to;
    int count;
    int n;
    int bit;

    dst_end = (unsigned char *)dst + 0x8000;
    end = (unsigned char *)0x70002000;
    span = (unsigned char *)spans;
    count = *ctrl >> 1;
    ctrl++;
    FastMemSet((void *)0x70000000, 0, 0x2400);
    bit = 1;
    cursor = (unsigned char *)0x70000000;
    for (;;) {
        next_dst = (unsigned char *)dst + 0x400;
        do {
            cursor += *span++;
            n = *span++;
            if (n != 0) {
                do {
                    n--;
                    next = cursor + 1;
                    if (count == 0) {
                        do {
                            count = *ctrl++;
                            bit = !bit;
                        } while (count == 0);
                    }
                    *cursor = bit;
                    count--;
                    cursor = next;
                } while (n != 0);
            }
        } while (cursor < end);
        r = (unsigned char *)0x70000000;
        w = (unsigned char *)0x70000000;
        do {
            *w = *r++;
            *w |= *r++ << 1;
            *w |= *r++ << 2;
            *w |= *r++ << 3;
            *w |= *r++ << 4;
            *w |= *r++ << 5;
            *w |= *r++ << 6;
            *w |= *r++ << 7;
            w++;
        } while (r < end);
        FastMemCopy(dst, (void *)0x70000000, 0x400);
        dst = next_dst;
        if (next_dst == dst_end) {
            return;
        }
        FastMemSet((void *)0x70000000, 0, 0x2000);
        wrapped = cursor - 0x2000;
        from = (unsigned char *)0x70002000;
        to = (unsigned char *)0x70000000;
        if (cursor > end) {
            do {
                *to++ = *from++;
            } while (from < cursor);
        }
        FastMemSet((void *)0x70002000, 0, 0x400);
        cursor = wrapped;
    }
}

/* Expands 128 rows of 16 source bytes into 4-bit-per-pixel masks: a
   256-entry table maps each byte to a word with nibble k set to 0xF when
   bit k is set, each row's 16 bytes are looked up into a 64-byte row
   buffer, and the buffer is copied to dst four times (0x100 bytes of dst
   per source row). The first bit is a plain store over the zeroed entry
   (retail's store-in-delay-slot), and the row buffer is filled through
   a block-local pointer, which gives retail's separate buffer copies. */
void expand_map_coverage(void *dst, unsigned char *src) {
    unsigned int table[256];
    unsigned int rowbuf[16];
    int i, j, row;

    for (i = 0; i < 256; i++) {
        table[i] = 0;
        if (i & 1) {
            table[i] = 0xF;
        }
        if (i & 0x2) {
            table[i] |= 0xF0;
        }
        if (i & 0x4) {
            table[i] |= 0xF00;
        }
        if (i & 0x8) {
            table[i] |= 0xF000;
        }
        if (i & 0x10) {
            table[i] |= 0xF0000;
        }
        if (i & 0x20) {
            table[i] |= 0xF00000;
        }
        if (i & 0x40) {
            table[i] |= 0xF000000;
        }
        if (i & 0x80) {
            table[i] |= 0xF0000000;
        }
    }
    for (row = 0; row < 128; row++) {
        unsigned int *p = rowbuf;
        for (j = 0; j < 16; j++) {
            *p++ = table[*src++];
        }
        FastMemCopy(dst, rowbuf, 0x40);
        dst = (char *)dst + 0x40;
        FastMemCopy(dst, rowbuf, 0x40);
        dst = (char *)dst + 0x40;
        FastMemCopy(dst, rowbuf, 0x40);
        dst = (char *)dst + 0x40;
        FastMemCopy(dst, rowbuf, 0x40);
        dst = (char *)dst + 0x40;
    }
}

LINKER_REMNANT("asm/remnants/text", func_00208858);

/* Bit counts of the 16 nibbles. */
typedef struct {
    int n[16];
} NibbleBits;

extern NibbleBits nibble_popcount_table;
extern unsigned char *map_coverage_buffer;


/* Condenses D_001A01FC's 0x200 rows of 64 bytes (two 4-bit samples
   each) into a 1-bit image at arg0: every four rows the per-column
   bit counts (D_001E83B0 is the nibble popcount table, copied onto the
   stack) are summed, and a column with 8 or more set bits becomes a
   set bit, packed eight columns to a byte. The first byte is then
   flagged with 0x02 and bit 0 cleared. */
void downsample_map_coverage(void *arg0) {
    unsigned char *dst = arg0;
    int acc[128];
    NibbleBits bits;
    unsigned char *src;
    unsigned char *out;
    int i, j, k, m;

    bits = nibble_popcount_table;
    src = map_coverage_buffer;
    out = dst;
    for (i = 0; i < 0x200; i++) {
        if ((i & 3) == 0) {
            FastMemSet(acc, 0, 0x200);
        }
        for (j = 0; j < 128; j += 2) {
            unsigned int b = *src++;
            acc[j] += bits.n[b & 0xF];
            acc[j + 1] += bits.n[b >> 4];
        }
        if (i % 4 == 3) {
            for (k = 0; k < 128; k++) {
                if (acc[k] < 8) {
                    acc[k] = 0;
                } else {
                    acc[k] = 1 << (k % 8);
                }
            }
            for (m = 0; m < 128; m += 8) {
                *out++ = acc[m] | acc[m + 1] | acc[m + 2] | acc[m + 3]
                       | acc[m + 4] | acc[m + 5] | acc[m + 6] | acc[m + 7];
            }
        }
    }
    dst[0] = (dst[0] & 0xFE) | 2;
}

extern void *msg_string_id(int) __asm__("func_001FE540");
extern char menu_item_price_fmt[]; /* "%d" */
extern char menu_token_error_text[]; /* "error" */
extern char vendor_item_prices[];
extern int sprintf_3(void *, char *, int) __asm__("func_00116248");
extern int sprintf_2(void *, char *) __asm__("func_00116248");

/* Copies the text of menu entry `bank` (text id at +0xA of its 0x28-byte
   record in the table at D_001A01F0[8]) into dst, expanding the first
   '%' escape: "%b" becomes the entry's D_001E02B0 value (the record's
   +0xC item, first int of its 0x18-byte row) printed with "%d", any
   other letter becomes "error". The strings are unsigned char (one lbu
   serves both the test and the copy), and each block that reads
   D_001A01F0 has its own char * local, which gives retail's kept %hi
   and rebuilt %lo. */
void FormatMenuItemText(int bank, unsigned char *dst) {
    unsigned char buf[16];
    unsigned char *src;
    unsigned char *p;
    char *t;

    t = (char *)level_map_selection;
    src = msg_string_id(*(short *)(*(char **)(t + 0x20) + bank * 0x28 + 0xA));
    p = buf;
    if (src == 0) {
        return;
    }
    while (*src != 0 && *src != '%') {
        *dst++ = *src++;
    }
    if (*src == 0) {
        *dst = *src;
        return;
    }
    src++;
    if (*src == 'b') {
        char *t2 = (char *)level_map_selection;
        sprintf_3(buf, menu_item_price_fmt,
                        *(int *)(vendor_item_prices + *(short *)(*(char **)(t2 + 0x20) + bank * 0x28 + 0xC) * 0x18));
    } else {
        sprintf_2(buf, menu_token_error_text);
    }
    src++;
    while (*p != 0) {
        *dst++ = *p++;
    }
    while (*src != 0) {
        *dst++ = *src++;
    }
    *dst = 0;
}

/* Retail carries 12 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n\tnop\n\tnop\n");

typedef struct {
    float a, b, c, d;
} OrientEntry;
extern OrientEntry level_map_transforms[];

/* arg2 (a table index, offset by 100 when the +100 variant is wanted)
   selects a row of D_0019EA70 (19 rows, 0..0x12); -1 means "use
   D_0015EE84"; anything out of [0,0x13) collapses to row 0. Row 6 with
   the +100 variant blends against fixed constants instead of the
   generic per-row formula. */
void world_to_map_coords(float *out0, float *out1, int arg2, float arg3, float arg4) {
    int idx;
    int flag = 0;

    if (arg2 >= 100) {
        arg2 -= 100;
        flag = 1;
    }

    idx = arg2;
    if (idx == -1) {
        idx = g_current_level_index;
    }
    if (idx < 0) {
        idx = 0;
    }
    if (idx >= 0x13) {
        idx = 0;
    }

    if (idx == 6 && flag) {
        *out0 = (level_map_transforms[6].d * arg4 + 1053.0f) * (1.0f / 512.0f);
        *out1 = (740.0f - level_map_transforms[6].b * arg3) * (1.0f / 512.0f);
        return;
    }

    *out0 = (level_map_transforms[idx].a + level_map_transforms[idx].b * arg3) * (1.0f / 512.0f);
    *out1 = (level_map_transforms[idx].c + level_map_transforms[idx].d * arg4) * (1.0f / 512.0f);
}

LINKER_REMNANT("asm/remnants/text", func_00208D30);

typedef struct {
    float x;
    float y;
    int icon;           /* icon id, or -1 for a plain square */
    unsigned int sub;   /* the icon's frame, or the square's colour */
} MapMarker;

extern int marker_half_size MACRO_ADDR;
extern float marker_scale_by_level[];
extern char hud_state_raw[] __asm__("D_0019A4E8");
extern void draw_hud_rect_blended(int, int, int, int, long, int);
extern int GetIconFrame(int, int);
extern void draw_hud_sprite_subpixel(int, int, int, int, int, int);

/* Draw the level's map markers inside the given screen rectangle, scaled by map zoom. */
void draw_map_markers(int left, int top, int right, int bottom) {
    float nx;
    float ny;
    int n = level_map_selection[0x2C];
    MapMarker *m = (MapMarker *)level_map_selection[7];

    while (n-- != 0) {
        int sx;
        int sy;

        world_to_map_coords(&nx, &ny, g_current_level_index, m->x, m->y);
        sx = left + (int)((float)(right - left) * nx);
        sy = top + (int)((float)(bottom - top) * ny);
        if (sx < -0x199 || sy < -0x199 || sx >= 0x219A || sy >= 0x1B9A) {
            m++;
            continue;
        }
        if (m->icon == -1) {
            draw_hud_rect_blended(sx - marker_half_size, sy - marker_half_size, sx + marker_half_size,
                          sy + marker_half_size, m->sub, 1);
        } else {
            int tex = GetIconFrame(m->icon, m->sub);
            int *st = level_map_selection;
            float *sc = (float *)(st + 0x2D);   /* the zoom scale table, at 0xB4 of the same block (the assembly's D_001A02A4) */
            char *arena = hud_state_raw;
            unsigned char *e = (unsigned char *)(*(char **)(arena + 0x24)
                + *(short *)(*(char **)(arena + 0x20) + tex * 4 + 2) * 8);
            float scale = (2.0f * sc[st[0x89]] + 5.0f) / 13.0f;
            float w = scale * (float)(1 << (e[6] + 4));

            draw_hud_sprite_subpixel(tex, sx - (int)(scale * (float)(1 << (e[6] + 3))),
                          sy - (int)(scale * (float)(1 << (e[7] + 3))),
                          (int)w, (int)(scale * (float)(1 << (e[7] + 4))), 0x80);
        }
        m++;
    }
}

extern int level_object_index_ranges[];
extern void *menu_action_messages[];
extern char race_gates[];

/* Copies three floats (+0x10, +0x14, +0x48) of each of level
   D_0015EE84's objects in D_00199578 into the 16-byte slots at
   D_0013D6B8; the index range is D_001E06B8[i]..[i + 1]. Binding the
   two bases to locals in retail's order orders their %hi halves. */
void copy_level_object_positions(void) {
    int i = g_current_level_index;
    if ((unsigned int)i < 0x13) {
        int start = level_object_index_ranges[i];
        int end = level_object_index_ranges[i + 1];
        if (start < end) {
            void **sbase = menu_action_messages;
            char *dbase = race_gates;
            float *dst = (float *)(dbase + start * 16);
            void **src = &sbase[start];
            start = end - start;
            do {
                char *p = (char *)*src;
                if (p != 0) {
                    dst[0] = *(float *)(p + 0x10);
                    dst[1] = *(float *)(p + 0x14);
                    dst[2] = *(float *)(p + 0x48);
                }
                dst += 4;
                start--;
                src++;
            } while (start != 0);
        }
    }
}

/* Empty debug hook; retail calls it but it does nothing. */
void DebugEnter_Nop(void) {
}

/*
 * Close but not exact (16/36 bytes): 2D cross-product orientation test.
 * arg0 -= arg2; arg1 -= arg3; arg4 -= arg2; arg5 -= arg3;
 * return (arg4*arg1 - arg5*arg0) < 0;
 * Same operations/order/register reuse (in-place subtraction) as
 * retail, confirmed via objdump, but the final sign test compiles to
 * `srl $2,$2,0x1f` here where retail uses `slti $2,$2,0` -- two
 * different instructions computing the identical 0/1 result for
 * "value < 0". Not a scheduling/register question like the usual
 * near-misses, a pure instruction-selection choice for the same
 * boolean-from-sign-bit pattern; not investigated further.
 */
/*
 * 4/36 (one instruction): 2D cross-product orientation test — is (x2,y2)
 * left of the (x0,y0)->(x1,y1) edge. Every register and all 7 arithmetic
 * instructions match retail exactly; the sole diff is the final sign
 * test, where this compiler emits `srl $2,$2,31` and retail has
 * `slti $2,$2,0`. Identical result, different instruction selection, and
 * it is not steerable from source: `< 0`, `<= -1`, `< 1-1`, a named
 * local, and `?1:0` all canonicalize to the same `srl`; only widening to
 * `long` changes it (to `dsrl32`, which is worse).
 *
 * The subtractions must be written in this order (x2/y2 before x1/y1) —
 * the last-source-statement-emits-first rotation applies to runs of
 * independent *arithmetic* statements, not just stores.
 */
/* Sign of the 2D cross product (x2-x0,y2-y0) x (x1-x0,y1-y0): true when
   the second vector is clockwise from the first.

   The `< 0` has to be written as an explicit if/return pair. Returning
   the comparison directly makes GCC emit the cheap sign-bit extract
   `srl $v0,$v0,31`; retail has `slti $v0,$v0,0`, which is what you get
   when the comparison feeds a branch rather than being the return value
   itself. That one instruction was this function's entire residual. */
int point_is_clockwise(int x1, int y1, int x0, int y0, int x2, int y2) {
    int cross;

    x2 -= x0;
    y2 -= y0;
    x1 -= x0;
    y1 -= y0;
    cross = x2 * y1 - y2 * x1;
    if (cross < 0) {
        return 1;
    }
    return 0;
}

extern char memory_card_state[];
extern int mode_freeze_state MACRO_ADDR;
extern int mode_freeze_flags MACRO_ADDR;
extern int memory_card_error_latch MACRO_ADDR;
extern int menu_state_frame_count MACRO_ADDR;
extern void (*menu_state_handlers[])(void);

/* Calls the handler for the state in D_0015EFB0 from the table at
   D_001A0538. Bits 0x80 and 0x100 of D_0015EFB4 request states 0x15 and
   0x14; D_00161380 counts calls since the state last changed. */
void Menu_RunStateHandler(void) {
    char *b = memory_card_state;
    int old;

    if (*(int *)(b + 0xEC) != 0 || *(int *)(b + 0x1C) != 0) {
        memory_card_error_latch = 1;
    }
    old = mode_freeze_state;
    if (mode_freeze_flags & 0x80) {
        mode_freeze_state = 0x15;
        mode_freeze_flags = (mode_freeze_flags & ~0x80) | 0x40;
    }
    if (mode_freeze_flags & 0x100) {
        mode_freeze_state = 0x14;
        mode_freeze_flags = (mode_freeze_flags & ~0x100) | 0x40;
    }
    menu_state_handlers[mode_freeze_state]();
    menu_state_frame_count++;
    if (mode_freeze_state != old) {
        menu_state_frame_count = 0;
    }
}

extern char memory_card_state[];
/* menu.cpp's state word and flags. Stored through the assembler's lui
   macro, and $gp-relative where the access sits in a delay slot (see
   tools/check_macro_slots.py). */
extern int memory_card_status;

/* Word view of the D_0013D390 record: indexing an int array here, rather
   than casting byte offsets off a char pointer, is what lets GCC keep the
   record address's lui/addiu pair together before the D_0015EFB0 store. */
extern int memory_card_state_i[] __asm__("D_0013D390");

/* Sets menu state 3, clears the record's word at +0xFC and copies +0xC4
   into +0x1C. */
void save_card_state_init(void) {
    mode_freeze_state = 3;
    memory_card_state_i[0x3F] = 0;
    memory_card_state_i[0x07] = memory_card_state_i[0x31];
}

/* Clears the 4 and 2 flag bits of D_0015EFB4, then picks the next
   state into D_0015EFB0 from the 0x80/0x100/0x200 bits and the
   D_0013D390 record. The cross-jumped `b` back into the 0x80 arm is
   one GNU as pads as a short loop and ps2eeas did not;
   tools/ps2eeas_nops.py writes it as a .word. */
void save_card_state_good_save(void) {
    int flags = mode_freeze_flags;
    char *b;
    int nf;
    mode_freeze_flags = flags & ~4;
    b = memory_card_state;
    nf = mode_freeze_flags & ~2;
    mode_freeze_flags = nf;
    if (*(int *)(b + 0xFC) == 0) {
        mode_freeze_state = 3;
        return;
    }
    if (flags & 0x80) {
        mode_freeze_state = 0x15;
        mode_freeze_flags = (nf ^ 0x80) | 0x40;
        return;
    }
    if (flags & 0x100) {
        mode_freeze_state = 0x14;
        mode_freeze_flags = (nf ^ 0x100) | 0x40;
        return;
    }
    if (*(int *)(b + 0x1C) != 0) {
        *(int *)(b + 0xFC) = 0;
        mode_freeze_flags = nf | 1;
        mode_freeze_state = 2;
        return;
    }
    if (flags & 0x200) {
        mode_freeze_flags = nf ^ 0x200;
        mode_freeze_state = 0x16;
    }
}

/* Card menu warning state: clear a busy slot, then go to state 3 when flag 1 clears. */
void save_card_state_warning(void) {
    char *b = memory_card_state;
    char *base = b + 0xB0;
    int idx = *(int *)(b + 0xCC);
    int *slot = (int *)(base + idx * 0xC0);
    if (*slot == 2) {
        *slot = 0;
    }
    if ((mode_freeze_flags ^ 1) & 1) {
        mode_freeze_state = 3;
    }
}

/* Card menu state 3: reset the pending card requests and the busy slot, then go to state 4. */
void CardMenu_State03ClearSlots(void) {
    char *b = memory_card_state;
    char *base = b + 0xB0;
    int idx = *(int *)(b + 0xCC);
    int *slot;
    *(int *)(b + 0xE4) = -1;
    *(int *)(b + 0xE8) = -1;
    *(int *)(b + 0x20) = -1;
    *(int *)(b + 0x3C) = -1;
    *(int *)(b + 0x58) = -1;
    *(int *)(b + 0x74) = -1;
    *(int *)(b + 0x90) = -1;
    slot = (int *)(base + idx * 0xC0);
    if (*slot == 2) {
        *slot = 0;
    }
    mode_freeze_state = 4;
}

/* Card menu state 4: once the card is checked, pick state 9 or 5 (unformatted) from its result. */
void CardMenu_State04AdvanceOnStatus(void) {
    char *b = memory_card_state;
    int v;
    mode_freeze_flags &= ~0x20;
    if (*(int *)(b + 8) != 2) {
        return;
    }
    v = *(int *)(b + 0x1C);
    if (v == 0) {
        mode_freeze_state = 9;
    } else if (v == -1) {
        *(int *)(b + 0x1C) = 0;
        mode_freeze_state = 9;
    } else if (v == -2) {
        mode_freeze_state = 5;
    }
}

/* Card unformatted state: back to state 3 if the card changed, to state 6 on confirm. */
void save_card_state_unformatted(void) {
    if (memory_card_status != -2) {
        mode_freeze_state = 3;
        return;
    }
    if (mode_freeze_flags & 2) {
        mode_freeze_state = 6;
    }
}

extern int boot_level_active MACRO_ADDR;

/* Card format prompt: flag 0x20 returns to state 5 (0x17 at boot), flag 8 goes to state 7. */
void save_card_state_prompt_format(void) {
    int flags;
    if (memory_card_status != -2) {
        mode_freeze_state = 3;
        return;
    }
    flags = mode_freeze_flags;
    if (flags & 0x20) {
        mode_freeze_flags = flags ^ 0x20;
        if (boot_level_active != 0) {
            mode_freeze_state = 0x17;
            return;
        }
        mode_freeze_state = 5;
        return;
    }
    if (flags & 8) {
        mode_freeze_flags = flags ^ 8;
        mode_freeze_state = 7;
    }
}

/* Card format request: queue the format command if idle, then go to state 8. */
void save_card_state_format_pending(void) {
    char *s = memory_card_state;
    *(int *)(s + 0x1C) = 0;
    if (*(int *)(s + 0xE4) < 0) {
        *(int *)(s + 0xE8) = 0;
        *(int *)(s + 0xE4) = 3;
    }
    mode_freeze_state = 8;
}

/* Card formatting state: when the format ends, go to state 0xE or to failure state 0x11. */
void save_card_state_formatting(void) {
    char *s = memory_card_state;
    if (*(int *)(s + 0xDC) == 2 && *(int *)(s + 0xE4) < 0) {
        if (*(int *)(s + 0xEC) != 0) {
            mode_freeze_state = 0x11;
            mode_freeze_flags |= 0x40;
            return;
        }
        mode_freeze_state = 0xE;
    }
}

/* Card formatted state: back to state 3 if the card changed, to state 0xA on confirm. */
void save_card_state_formatted(void) {
    if (memory_card_status != 0) {
        mode_freeze_state = 3;
        return;
    }
    if (mode_freeze_flags & 6) {
        mode_freeze_state = 0xA;
    }
}

/* Card save check: when idle, queue the save-file scan and go to state 0xB. */
void save_card_state_check_save(void) {
    char *s = memory_card_state;
    if (*(int *)(s + 0xDC) == 2 && *(int *)(s + 0xE4) < 0) {
        *(int *)(s + 0xE4) = 7;
        *(int *)(s + 0xE8) = 0;
        mode_freeze_state = 0xB;
    }
}

extern char memory_card_state[];

/* Dispatch-table handler for menu.cpp's D_0013D390 state record. First,
   guarded by unkDC>=3 || unkE4>=0, clears a per-index slot in the
   0xB0-byte array (D_0013D390+0xB0 + idx*0xC0) from 1 to 2 -- named as
   its own `base` local, computed before `idx`, matching the sibling
   func_00209238's identical slot-lookup shape (that ordering is what
   makes the allocator put the base pointer in $a0 and the literal 1 in
   $a1, as retail does; keeping `b` mutated in place puts them the other
   way round). Then it re-reads the record fresh and sets D_0015EFB0
   (the next-state word) from unk1C/unkEC/unk14/unkC+unkAC. */
void CardMenu_State11MarkSlotChoose(void) {
    char *b = memory_card_state;
    int v;
    if (*(int *)(b + 0xDC) >= 3 || *(int *)(b + 0xE4) >= 0) {
        char *base = b + 0xB0;
        int idx = *(int *)(b + 0xCC);
        int *slot = (int *)(base + idx * 0xC0);
        if (*slot == 1) {
            *slot = 2;
        }
    }
    b = memory_card_state;
    if (*(int *)(b + 0x1C) < -1 || *(int *)(b + 0xEC) != 0) {
        mode_freeze_state = 3;
        return;
    }
    v = *(int *)(b + 0x14);
    if (v == -2) {
        if (*(int *)(b + 0xC) + *(int *)(b + 0xAC) < 0x15E) {
            mode_freeze_state = 0x13;
        } else {
            mode_freeze_state = 0xC;
        }
        return;
    }
    if (v >= -1) {
        mode_freeze_state = 0x10;
    }
}

/* Card no-save state: back to state 3 if the card changed, to state 0xD on confirm. */
void save_card_state_no_save(void) {
    if (memory_card_status != 0) {
        mode_freeze_state = 3;
        return;
    }
    if (mode_freeze_flags & 2) {
        mode_freeze_state = 0xD;
    }
}

/* Create-save prompt: flag 0x20 returns to state 0xC (0x18 at boot), flag 0x10 goes to 0xE. */
void save_card_state_prompt_create_save(void) {
    int flags;
    if (memory_card_status != 0) {
        mode_freeze_state = 3;
        return;
    }
    flags = mode_freeze_flags;
    if (flags & 0x20) {
        mode_freeze_flags = flags ^ 0x20;
        if (boot_level_active != 0) {
            mode_freeze_state = 0x18;
            return;
        }
        mode_freeze_state = 0xC;
        return;
    }
    if (flags & 0x10) {
        mode_freeze_flags = flags ^ 0x10;
        mode_freeze_state = 0xE;
    }
}

/* Create-save request: when idle, queue the create command and go to state 0xF. */
void save_card_state_create_save_pending(void) {
    char *s = memory_card_state;
    if (*(int *)(s + 0xDC) == 2 && *(int *)(s + 0xE4) < 0) {
        *(int *)(s + 0xE4) = 9;
        *(int *)(s + 0xE8) = 0;
        mode_freeze_state = 0xF;
    }
}

/* Creating-save state: when done, rescan and go to state 0x10, or fail to state 0x12. */
void save_card_state_creating_save(void) {
    char *s = memory_card_state;
    if (*(int *)(s + 0xDC) == 2 && *(int *)(s + 0xE4) < 0) {
        if (*(int *)(s + 0xEC) != 0) {
            mode_freeze_state = 0x12;
            mode_freeze_flags |= 0x40;
            return;
        }
        *(int *)(s + 0xE4) = 7;
        *(int *)(s + 0xC8) = 0;
        *(int *)(s + 0x14) = 0;
        *(int *)(s + 0xE8) = 0;
        mode_freeze_state = 0x10;
    }
}

/* func_00209188's sibling: clears the 4 and 2 bits, then the same
   0x80/0x100 arms and the D_0013D390 record's tests. Its cross-jumped
   `b` is written as a .word too (tools/ps2eeas_nops.py). */
void CardMenu_State16NextState(void) {
    int flags;
    char *b;
    if (mode_freeze_flags & 4) {
        mode_freeze_flags &= ~4;
    }
    if (mode_freeze_flags & 2) {
        mode_freeze_flags &= ~2;
    }
    flags = mode_freeze_flags;
    if (flags & 0x80) {
        mode_freeze_state = 0x15;
        mode_freeze_flags = (flags ^ 0x80) | 0x40;
        return;
    }
    if (flags & 0x100) {
        mode_freeze_state = 0x14;
        mode_freeze_flags = (flags ^ 0x100) | 0x40;
        return;
    }
    b = memory_card_state;
    if (*(int *)(b + 0x1C) != 0) {
        mode_freeze_state = 3;
        return;
    }
    if (*(int *)(b + 0xFC) != 0) {
        mode_freeze_state = 1;
    }
}


/* Format failed state: free the busy slot and go to state 3 once the message closes. */
void save_card_state_format_failed(void) {
    char *b = memory_card_state;
    char *base = b + 0xB0;
    int idx = *(int *)(b + 0xCC);
    int *slot = (int *)(base + idx * 0xC0);
    if (*slot == 2) {
        *slot = 0;
    }
    if (!(mode_freeze_flags & 0x40)) {
        mode_freeze_state = 3;
    }
}

/* Create failed state: free the busy slot and go to state 3 once the message closes. */
void save_card_state_create_failed(void) {
    char *b = memory_card_state;
    char *base = b + 0xB0;
    int idx = *(int *)(b + 0xCC);
    int *slot = (int *)(base + idx * 0xC0);
    if (*slot == 2) {
        *slot = 0;
    }
    if (!(mode_freeze_flags & 0x40)) {
        mode_freeze_state = 3;
    }
}

/* Card no-room state: go back to state 3 when the card status changes. */
void save_card_state_no_room(void) {
    if (memory_card_status != 0) {
        mode_freeze_state = 3;
    }
}

/* Load failed state: free the busy slot and go to state 3 once the message closes. */
void save_card_state_load_failed(void) {
    char *b = memory_card_state;
    char *base = b + 0xB0;
    int idx = *(int *)(b + 0xCC);
    int *slot = (int *)(base + idx * 0xC0);
    if (*slot == 2) {
        *slot = 0;
    }
    if (!(mode_freeze_flags & 0x40)) {
        mode_freeze_state = 3;
    }
}

/* Save failed state: free the busy slot and go to state 3 once the message closes. */
void save_card_state_save_failed(void) {
    char *b = memory_card_state;
    char *base = b + 0xB0;
    int idx = *(int *)(b + 0xCC);
    int *slot = (int *)(base + idx * 0xC0);
    if (*slot == 2) {
        *slot = 0;
    }
    if (!(mode_freeze_flags & 0x40)) {
        mode_freeze_state = 3;
    }
}

/* Card state 22: when idle, finish to state 1 or show the error state 0x15. */
void CardMenu_State22StartOrFinish(void) {
    char *b = memory_card_state;
    if (*(int *)(b + 0xDC) < 3 && *(int *)(b + 0xE4) < 0) {
        if (*(int *)(b + 0xEC) != 0) {
            *(int *)(b + 0xFC) = 0;
            mode_freeze_state = 0x15;
            mode_freeze_flags |= 0x440;
            return;
        }
        mode_freeze_state = 1;
    }
}

/* Begin prompt on an unformatted card: state 3 if the card changed, state 5 on flag 0x20. */
void save_card_state_prompt_begin_unformatted(void) {
    int flags;
    if (memory_card_status != -2) {
        mode_freeze_state = 3;
        return;
    }
    flags = mode_freeze_flags;
    if (flags & 0x20) {
        mode_freeze_flags = flags ^ 0x20;
        mode_freeze_state = 5;
    }
}

/* Begin prompt on a card with no save: state 3 if the card changed, state 0xC on flag 0x20. */
void save_card_state_prompt_begin_no_save(void) {
    int flags;
    if (memory_card_status != 0) {
        mode_freeze_state = 3;
        return;
    }
    flags = mode_freeze_flags;
    if (flags & 0x20) {
        mode_freeze_flags = flags ^ 0x20;
        mode_freeze_state = 0xC;
    }
}
