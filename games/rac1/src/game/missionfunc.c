#include "common.h"
#include "hero.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * missionfunc.cpp in the original source; text 0x20C7A0-0x20D348.
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
extern unsigned char menu_item_enabled_0 NOT_SDA;
extern unsigned char g_menu_item_enabled_1 NOT_SDA;
extern unsigned char menu_item_enabled_9 NOT_SDA;
extern short menu_selection_count;
extern unsigned char menu_item_enabled_16 NOT_SDA;
extern unsigned char menu_item_enabled_17 NOT_SDA;
extern unsigned char menu_item_enabled_18 NOT_SDA;
extern unsigned char menu_item_enabled_19 NOT_SDA;
extern unsigned char menu_item_enabled_25 NOT_SDA;
extern int map_region_enabled_3 NOT_SDA;
extern unsigned char menu_item_enabled_41 NOT_SDA;
extern int hero_control_mode NOT_SDA;
extern unsigned char menu_item_enabled_36 NOT_SDA;
extern unsigned char menu_item_enabled_37 NOT_SDA;
extern unsigned char menu_item_enabled_38 NOT_SDA;
extern unsigned char menu_item_enabled_55 NOT_SDA;
extern unsigned char menu_item_enabled_56 NOT_SDA;
extern unsigned char menu_item_enabled_57 NOT_SDA;
extern unsigned char menu_item_enabled_68;
extern unsigned char menu_item_enabled_64 NOT_SDA;
extern unsigned char menu_item_enabled_65 NOT_SDA;
extern unsigned char menu_item_enabled_66 NOT_SDA;
extern unsigned char menu_item_enabled_67 NOT_SDA;
extern unsigned char menu_item_enabled_69 NOT_SDA;
extern unsigned char menu_item_enabled_77 NOT_SDA;
extern unsigned char menu_item_enabled_102 NOT_SDA;
extern unsigned char menu_item_enabled_103 NOT_SDA;
extern unsigned char menu_item_enabled_104 NOT_SDA;
extern unsigned char menu_item_enabled_105 NOT_SDA;
extern unsigned char menu_item_enabled_115 NOT_SDA;
extern int max_map_coverage_size[];
extern void DebugEnter_Nop(void);
extern int compress_map_coverage(void *dst, int size, int a, int b);
extern void downsample_map_coverage(void *dst);
extern short g_current_level_index;
extern int g_current_level_index_far __asm__("D_0015EE84") NOT_SDA;
extern int map_coverage_active[] NOT_SDA;
extern void decode_compressed_occlusion_map(void *, unsigned char *, int);
extern void expand_map_coverage(void *, unsigned char *);
extern char memory_card_state[];
extern short mode_freeze_state;
extern int mode_freeze_flags;
extern int mc_gamedata[];
extern int mc_leveldata[];
extern int memcard_GetDataSize(int *p);
extern int memcard_PrepData(void *dst, int i, int *table);
extern int sce_mc_init(void);
extern int STUB_printf();
extern char memcard_error_message[];

extern int check_mission_condition(short type, int arg);
extern char *planet_mission_lists[];
extern unsigned char level_visit_state[];
extern char mission_list_state[];

/* Per-frame objective update over the 0x28-byte node list of the current
   pad slot (D_001A2EF0[D_001A01F0[0x89]], kept at D_001A2F90+0x10):
   status +0x24 = 0 when a gated node (+0x10 & 4) has its D_0013DE60 byte
   clear or its first condition (func_0020C940) fails, else 2 or 1 by the
   second condition; then each +0x1C callback's result goes to +0x26; then
   the count of status-1 nodes without +0x10 & 2 goes to D_001A2F90+0xC.
   Returns whether that count is 0. The head is stored and then read back
   (CSE forwards it, leaving retail's copy into $s0 after the null test),
   and each block reads D_001A2F90 through its own `char *` local. */
int update_mission_list(void) {
    char *node;

    {
        char *p = mission_list_state;
        *(char **)(p + 0x10) = planet_mission_lists[level_map_selection[0x89]];
        if (*(char **)(p + 0x10) == 0) {
            return 0;
        }
        node = *(char **)(p + 0x10);
    }
    while (*(short *)node != 0) {
        if ((*(unsigned short *)(node + 0x10) & 4) && level_visit_state[level_map_selection[0x89]] == 0) {
            *(short *)(node + 0x24) = 0;
        } else if (!check_mission_condition(*(short *)(node + 2), *(int *)(node + 4))) {
            *(short *)(node + 0x24) = 0;
        } else if (!check_mission_condition(*(short *)(node + 8), *(int *)(node + 0xC))) {
            *(short *)(node + 0x24) = 1;
        } else {
            *(short *)(node + 0x24) = 2;
        }
        node += 0x28;
    }
    {
        char *p = mission_list_state;
        for (node = *(char **)(p + 0x10); *(short *)node != 0; node += 0x28) {
            if (*(int (**)(int))(node + 0x1C) != 0) {
                *(short *)(node + 0x26) = (*(int (**)(int))(node + 0x1C))(*(int *)(node + 0x20));
            }
        }
    }
    {
        char *p = mission_list_state;
        *(int *)(p + 0xC) = 0;
        for (node = *(char **)(p + 0x10); *(short *)node != 0; node += 0x28) {
            if (*(short *)(node + 0x24) == 1 && !(*(unsigned short *)(node + 0x10) & 2)) {
                *(int *)(p + 0xC) += 1;
            }
        }
    }
    {
        char *p = mission_list_state;
        return *(int *)(p + 0xC) == 0;
    }
}

extern unsigned char level_available[];
extern unsigned char item_available_b[] __asm__("D_0013D5C8");
extern unsigned char items_acquired[];
typedef struct { int a, b, c, d; } Rec16_C940;
extern Rec16_C940 race_gates_r[] __asm__("D_0013D6B8");
extern unsigned char gSpecialItems[];
extern unsigned char gold_bolts_collected[][4];

/* A 10-case switch; the explicit `case 9: break;` keeps retail's table. */
int check_mission_condition(short type, int arg) {
    switch (type) {
    case 0:
        return 1;
    case 1:
        return level_available[arg] != 0;
    case 2:
        return item_available_b[arg] != 0;
    case 3:
        return items_acquired[arg] != 0;
    case 4:
        if (arg < 0x79) return race_gates_r[arg].d != 0;
        break;
    case 5:
        if (arg < 0x79) return race_gates_r[arg].d >= 2;
        break;
    case 6:
        return gSpecialItems[arg] != 0;
    case 7:
        return ((int (*)(void))arg)() != 0;
    case 8:
        return gold_bolts_collected[arg >> 16][arg & 0xFFFF] != 0;
    case 9:
        break;
    }
    return 0;
}

typedef struct {
    short id;             /* 0x00 */
    char _pad02[0xE];
    unsigned short flags; /* 0x10 */
    short base;           /* 0x12 */
    short ids[8];         /* 0x14 */
    short status;         /* 0x24 */
    short sel;            /* 0x26 */
} MissionNode;
extern MissionNode *current_mission_nodes;

/* Lists the current objectives (the 0x28-byte node list at D_001A2FA0,
   ended by id 0): each node not hidden (flags & 2), with a status, and
   not an optional (flags & 1) one already done (status 2) goes to
   out[count] as its id (with arg3: 0x5243 when done, else ids[sel]),
   sets bit count of *mask when done and puts base + sel in *nums++.
   Bit 31 of *mask says every visible node is done. Returns the count.
   out is indexed by count: loop strength reduction then gives retail's
   pointer and its copy for the second store. */
int collect_mission_ids(int *out, int *mask, int *nums, int arg3) {
    MissionNode *p = current_mission_nodes;
    int count = 0;
    int all = 1;

    *out = 0;
    if (mask != 0) {
        *mask = 0;
    }
    if (nums != 0) {
        *nums = -1;
    }
    if (p == 0) {
        return 0;
    }
    while (p->id != 0) {
        unsigned short f = p->flags;
        short t = p->status;

        if (t != 2 && !(f & 2)) {
            all = 0;
        }
        if (!(f & 2) && t != 0 && !((f & 1) && t == 2)) {
            out[count] = p->id;
            if (arg3 != 0) {
                out[count] = (p->status == 2) ? 0x5243 : p->ids[p->sel];
            }
            if (mask != 0 && p->status == 2) {
                *mask |= 1 << count;
            }
            if (nums != 0) {
                *nums++ = p->base + p->sel;
            }
            count++;
        }
        p++;
    }
    if (mask != 0 && all != 0) {
        *mask |= 0x80000000;
    }
    return count;
}
__asm__(".section .text\n\tnop\n");

extern int race_gates_24_status NOT_SDA;
extern unsigned char menu_item_enabled_12 NOT_SDA;

/* Predicate: true when D_0013D844 and the flag byte D_0013D4A8 are both set. */
int MissionCond_Obj18AndSpecial18(void) {
    if (race_gates_24_status != 0 && menu_item_enabled_12 != 0) return 1;
    return 0;
}

extern int race_gates_47_flag NOT_SDA;

/* Predicate: true when D_0013D9B4 and special items 0x20 and 0x21 are all set. */
int MissionCond_Obj2FAndSpecials(void) {
    if (race_gates_47_flag != 0 && gSpecialItems[0x20] != 0 && gSpecialItems[0x21] != 0) return 1;
    return 0;
}

extern unsigned char gHaveHeliPack NOT_SDA;
extern int race_gates NOT_SDA;

/* Predicate: true when both mission words at D_0013D6B8 are non-zero. */
int MissionCond_Obj3FAnd40(void) {
    char *base = (char *)&race_gates;
    if (*(int *)(base + 0x40C) != 0 && *(int *)(base + 0x3FC) != 0) return 1;
    return 0;
}

extern int race_gates_66_status NOT_SDA;
extern unsigned char menu_item_enabled_73 NOT_SDA;

/* Predicate: true when D_0013DAE4 and the flag byte D_0013D4E5 are both set. */
int MissionCond_Obj42AndSpecial55(void) {
    if (race_gates_66_status != 0 && menu_item_enabled_73 != 0) return 1;
    return 0;
}

extern int race_gates_70_status NOT_SDA;
extern unsigned char menu_item_enabled_85 NOT_SDA;

/* Predicate: true when D_0013DB24 and the flag byte D_0013D4F1 are both set. */
int MissionCond_Obj46AndSpecial61(void) {
    if (race_gates_70_status != 0 && menu_item_enabled_85 != 0) return 1;
    return 0;
}

extern int race_gates_87_status NOT_SDA;
extern unsigned char race_gates_base NOT_SDA;

/* Predicate: true when D_0013DC34 and the flag byte D_0013D605 are both set. */
int MissionCond_Obj57AndItem3D(void) {
    if (race_gates_87_status != 0 && race_gates_base != 0) return 1;
    return 0;
}

extern int item_available NOT_SDA;

/* Predicate: true when two entries of the D_0013D5C8 table are both set. */
int MissionCond_Items1FAnd21(void) {
    unsigned char *base = (unsigned char *)&item_available;
    if (base[0x21] != 0 && base[0x1F] != 0) return 1;
    return 0;
}

/* Returns whether entry arg0 of the D_0013D5C8 table is set. */
int is_item_available(int arg0) {
    unsigned char *base = (unsigned char *)&item_available;
    return base[arg0] != 0;
}

/* Two flat `&&` returns over race_gates_r[20/24/22].d; retail's reuse of
   the %hi register comes out by itself. */
int get_map_point_mission_status(void) {
    if (race_gates_r[20].d != 0 && race_gates_r[24].d == 0) {
        return 1;
    }
    if (race_gates_r[24].d != 0 && gHaveHeliPack != 0 && race_gates_r[22].d == 0) {
        return 2;
    }
    return 0;
}

extern unsigned char menu_item_enabled_20 NOT_SDA;

/* Returns 0 when D_0013D9B4 is clear, else 2 or 1 depending on D_0013D4B0. */
int MissionCond_Obj2FVariant(void) {
    if (race_gates_47_flag != 0) {
        return menu_item_enabled_20 ? 2 : 1;
    }
    return 0;
}

extern unsigned char gemlik_available NOT_SDA;

/* Predicate: true when both D_0013D4F1 and D_0013DE55 are set. */
int MissionCond_Special61AndFlag0D(void) {
    if (menu_item_enabled_85 != 0 && gemlik_available != 0) return 1;
    return 0;
}

extern unsigned char gHaveMorphORay NOT_SDA;

/* Returns 2 when the Morph-o-Ray is owned, else whether D_0013DC34 is set. */
int get_morph_o_ray_status(void) {
    if (gHaveMorphORay != 0) return 2;
    return race_gates_87_status != 0;
}

extern unsigned char gHaveHologuise NOT_SDA;

/* Returns whether the Hologuise is owned. */
int has_hologuise(void) {
    return gHaveHologuise != 0;
}

/* Returns 2 or the state of another entry of the D_0013D5C8 table, depending on the first. */
int Mission_GetHologuiseOrMapStatus(void) {
    unsigned char *base = (unsigned char *)&item_available;
    if (base[0x1F] != 0) return 2;
    return base[0x21] != 0;
}

typedef struct {
    u8 pad0[0x24];
    s16 active;
    u8 pad26[2];
} MapIconLink_0CDE0;

typedef struct {
    f32 x;
    f32 y;
    f32 z;
    s32 flag;
} MapPoint_0CDE0;

typedef struct {
    u8 pad0[0x10];
    f32 x;
    f32 y;
    u8 pad18[0x30];
    f32 z;
} MapWorldObject_0CDE0;

struct TextRegion_0CDE0 {
    s16 top;
    s16 bottom;
    s16 left;
    s16 right;
    s16 anchor_x;
    s16 anchor_y;
    s16 measured_width;
    s16 rendered_height;
    s16 line_advance;
    u16 flags;
    s16 subpixel_x_sixteenths;
    s16 subpixel_y_sixteenths;
};
/* One entry of a level's map icon list; the list ends at the first entry with flags bit 4. */
struct MapIcon_0CDE0 {
    s16 id;             /* 0x00: moby index (current level) or D_0013D6B8 index; -1..-9 are fixed positions */
    s16 link;           /* 0x02: -1 always active, else the link's active word decides */
    u16 flags;          /* 0x04: 0x4 ends the list, 0x10 label shown */
    u16 texture_id;     /* 0x06 */
    s16 frame_index;    /* 0x08 */
    s16 label_text_id;  /* 0x0A: 0 = no label */
    s16 label_item;     /* 0x0C */
    u16 label_width;    /* 0x0E */
    u16 label_height;   /* 0x10 */
    s16 label_offset_x; /* 0x12 */
    s16 label_offset_y; /* 0x14 */
    u8 pad16[0x2];
    f32 x;              /* 0x18: map position 0..1 */
    f32 y;              /* 0x1C */
    f32 angle;          /* 0x20 */
    s32 active;         /* 0x24 */
};
/* Map screen state at D_001A01F0, as far as this function reads it. */
struct MapState_0CDE0 {
    u8 pad0[0x20];
    struct MapIcon_0CDE0 *icons; /* 0x020: icon list of the shown level */
    u8 pad24[0xE0];
    s32 pan_x[20];               /* 0x104 */
    s32 pan_y[20];               /* 0x154 */
};
extern struct MapState_0CDE0 level_map_selection_r __asm__("D_001A01F0");
extern struct Hero g_hero_r __asm__("D_0013F450");
extern s32 g_current_level_index_r __asm__("D_0015EE84") MACRO_ADDR;
extern u8 level_visit_state_r[] __asm__("D_0013DE60");
extern struct MapIcon_0CDE0 *map_icon_lists_r[] __asm__("D_001A2F40");
extern s32 D_001A02F4_0CDE0[] __asm__("D_001A02F4");
extern s32 map_use_alt_level_layout_r __asm__("D_0015FE20") MACRO_ADDR;
extern MapWorldObject_0CDE0 *menu_action_messages_r[] __asm__("D_00199578");
extern MapPoint_0CDE0 race_gates_r2[] __asm__("D_0013D6B8");
extern s32 D_0013D6C4_0CDE0[] __asm__("D_0013D6C4");
extern struct {
    u8 pad0[0x10];
    MapIconLink_0CDE0 *links;
} D_001A2F90_0CDE0 __asm__("D_001A2F90");

extern void func_00208C38_0CDE0(f32 *outx, f32 *outy, s32 view, f32 x,
                                f32 y) __asm__("func_00208C38");
extern void FormatMenuItemText_r(s32 idx, char *dst) __asm__("func_00208AB0");
extern void func_001F75D0_0CDE0(struct TextRegion_0CDE0 *, long, char *,
                                    int) __asm__("func_001F75D0");

/* Map screen icon refresh: picks the level's icon list, centres the map on the hero when
   it is the current level, places the fixed icons (-1..-9) and projects the others to map
   coordinates, then sets each icon's active flag and sizes its label box.
   Adapted from Lombyte's project (GPL-3.0) for PAL: src/ui/map/update_map_icons.c, update_map_icons. */
void update_map_icons(s32 level, s32 flag) {
    char label_text[128];
    f32 map_x;
    f32 map_y;
    struct MapIcon_0CDE0 *icon;
    s32 icon_index;

    if (level < 19 && level_visit_state_r[level] != 0) {
        level_map_selection_r.icons = map_icon_lists_r[level];
    } else {
        level_map_selection_r.icons = 0;
    }

    if (g_hero_r.motion.pos.f[2] != 0.0f && level == g_current_level_index_r && flag) {
        func_00208C38_0CDE0(&map_x, &map_y, map_use_alt_level_layout_r ? level + 100 : level, g_hero_r.motion.pos.f[0],
                            g_hero_r.motion.pos.f[1]);
        level_map_selection_r.pan_x[level] = (s32)(map_x * 4096.0f) << 16;
        level_map_selection_r.pan_y[level] = (s32)(map_y * 4096.0f) << 16;
    } else {
        /* D_001A02F4_0CDE0 is &level_map_selection_r.posx: a null-guarded reset that can
           never run, but retail still emits it with p folded to 0. */
        s32 *p = D_001A02F4_0CDE0;
        if (p == 0) {
            p[level] = 0x8000000;
            p[level + 20] = 0x8000000;
        }
    }

    if (level_map_selection_r.icons == 0) {
        return;
    }

    if (!(level_map_selection_r.icons->flags & 4)) {
        icon_index = 0;
        do {
            icon = &level_map_selection_r.icons[icon_index];
            if (icon->id == -1) {
                icon->x = 0.234375f;
                icon->y = 0.30078125f;
            } else if (icon->id == -2) {
                icon->x = 0.5390625f;
                icon->y = 0.365234375f;
            } else if (icon->id == -3) {
                icon->x = 0.58203125f;
                icon->y = 0.6875f;
            } else if (icon->id == -4) {
                icon->x = 0.720703125f;
                icon->y = 0.728515625f;
            } else if (icon->id == -5) {
                icon->x = 0.384765625f;
                icon->y = 0.396484375f;
            } else if (icon->id == -7) {
                icon->x = 0.8671875f;
                icon->y = 0.23046875f;
            } else if (icon->id == -8) {
                icon->x = 0.48046875f;
                icon->y = 0.5703125f;
            } else if (icon->id == -9) {
                icon->x = 0.625f;
                icon->y = 0.72265625f;
            } else {
                if (level == g_current_level_index_r) {
                    if (menu_action_messages_r[icon->id] != 0) {
                        func_00208C38_0CDE0(&icon->x, &icon->y, level, menu_action_messages_r[icon->id]->x,
                                            menu_action_messages_r[icon->id]->y);
                        level_map_selection_r.icons[icon_index].angle = menu_action_messages_r[icon->id]->z;
                    }
                } else {
                    func_00208C38_0CDE0(&icon->x, &icon->y, level, race_gates_r2[icon->id].x,
                                        race_gates_r2[icon->id].y);
                    level_map_selection_r.icons[icon_index].angle = race_gates_r2[icon->id].z;
                }
            }
            icon_index++;
        } while (!(level_map_selection_r.icons[icon_index].flags & 4));
    }

    for (icon_index = 0; !(level_map_selection_r.icons[icon_index].flags & 4); icon_index++) {
        icon = &level_map_selection_r.icons[icon_index];
        icon->flags &= ~0x10;
        icon->active = 0;
        if (icon->link == -1) {
            icon->active = 1;
        } else {
            icon->active = D_001A2F90_0CDE0.links[icon->link].active == 1;
        }
        if ((icon->flags & 0x1000) && (D_0013D6C4_0CDE0[icon->id * 4] ^ 1) & 1) {
            icon->active = 0;
        }
        if (level_map_selection_r.icons[icon_index].label_text_id != 0) {
            s32 label_height_changed = 0;
            s16 previous_label_height;

            level_map_selection_r.icons[icon_index].flags |= 0x10;
            FormatMenuItemText_r(icon_index, label_text);
            {
                struct TextRegion_0CDE0 text_window = {
                    0, icon->label_height, 0, icon->label_width, 4, 4, 0, 0, 0xF, 4};
                func_001F75D0_0CDE0(&text_window, 0x80FFA888L, label_text, -1);
                icon->label_height = text_window.rendered_height + 8;
                previous_label_height = text_window.rendered_height;
                do {
                    text_window.right -= 4;
                    func_001F75D0_0CDE0(&text_window, 0x80FFA888L, label_text, -1);
                    if (text_window.rendered_height != previous_label_height) {
                        label_height_changed = 1;
                    }
                } while (!label_height_changed);
                icon->label_width = text_window.right + 4;
            }
        }
    }
}
__asm__(".section .text\n\tnop\n\tnop\n");
