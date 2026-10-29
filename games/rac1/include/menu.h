#ifndef MENU_H
#define MENU_H

/*
 * The pause menu's page, system and item grid cell records, laid out as the
 * NTSC decomp's ui/menus/menu_system.h and menu_screen.h have them.
 */

#include "common.h"

struct MenuScreen;

struct MenuScreenWords {
    u8 pad_30[0x4];
    s32 unk34; /* 0x34: flags 0x40 / 0x80 (draw_map_screen) */
    s32 saved_text_count; /* 0x38: previous help_msg_count */
    s32 unk3C; /* 0x3C: stream buffer (FUN_00225660) */
    u8 pad_40[0x4];
    union {
        struct {
            u8 pad_44[0xC];
            s32 load_state; /* 0x50: 0, 1 or 3 */
            s32 saved_text_table; /* 0x54: previous help_msgs_m */
            u8 pad_58[0x8];
        };
        s32 slots[24]; /* 0x44: moby handles, each freed on leave */
    };
};

struct MenuOption {
    void *unk0; /* 0x0: 0 ends the list */
    u8 *flag; /* 0x4: toggled by confirm (pad 0x40) */
    u8 pad_8[0x8];
    u32 flags; /* 0x10: bit 0 = fade out and toggle D_0016034C instead */
};

struct MenuOptionListData {
    u8 pad_30[0x4];
    struct MenuOption *list; /* 0x34: ended by a null unk0 */
    s32 selection; /* 0x38: pad 0x1000 up, 0x4000 down */
    s32 fade_timer; /* 0x3C: counts down after fade_to_black, drives D_0015F43C */
};

struct MenuChoice {
    s32 unk0; /* 0x0: 0 ends the list */
    u8 *value; /* 0x4: chosen option; confirm advances it */
    s32 option[4]; /* 0x8: 0 ends them early */
};

struct MenuChoiceListData {
    u8 pad_30[0x4];
    struct MenuChoice *list; /* 0x34: ended by a zero unk0 */
    s32 selection; /* 0x38: pad 0x1000 up, 0x4000 down */
};

struct MenuMissionListData {
    s32 choice[19]; /* 0x30: selected mission per level; -1 while unfocused */
    s32 count; /* 0x7C: collect_mission_ids result */
};

struct MenuItemGridData {
    s32 flags; /* 0x30: 2 fixed row step, 4/8 lockable, 0x20 no equipped frame */
    f32 margin_x; /* 0x34: left margin */
    f32 margin_y; /* 0x38: top margin */
    s32 selected_cell; /* 0x3C: highlighted cell */
    s32 rows; /* 0x40 */
    s32 cols; /* 0x44 */
    struct MenuGridCell *cells; /* 0x48: rows * cols */
    struct MenuScreen *up; /* 0x4C */
    struct MenuScreen *down; /* 0x50 */
    struct MenuScreen *left; /* 0x54 */
    struct MenuScreen *right; /* 0x58 */
};

struct MenuIcon {
    u16 icon; /* 0x0: get_icon_frame icon */
    s16 frame; /* 0x2: get_icon_frame frame */
};

struct MenuIconListData {
    u8 pad_30[0xC];
    s32 cursor; /* 0x3C: highlighted icon */
    s32 count; /* 0x40: icons drawn */
    u8 pad_44[0x4];
    struct MenuIcon *list; /* 0x48 */
    u8 pad_4C[0x10];
    s32 first_y; /* 0x5C: y of the first icon, 0x260 per icon */
};

struct MenuSaveSlotData {
    s32 flags; /* 0x30: bit 0 pad repeat mask, 0x2000 second variant */
    u8 pad_34[0xC];
    s32 slot; /* 0x40: memory card slot 0..4, mirrored in D_0015EE34 */
    u8 pad_44[0x4];
    s32 save_data; /* 0x48: buffer for prepare_save_game / FUN_00226a70 */
    s32 step; /* 0x4C: 0 idle, 1 start saving, 2 started */
};

struct MenuPromptData {
    u8 pad_30[0x24];
    s32 buffer; /* 0x54: stream buffer */
};

struct MenuItemSlotsData {
    s32 items[8]; /* 0x30: item id per slot */
    s32 cursor; /* 0x50: slot 0..7, pad 8/4 step it */
};

struct MenuCycleData {
    u8 pad_30[0x24];
    s32 selection; /* 0x54: one of twelve, wraps */
};

struct MenuTextItem {
    s16 text; /* 0x0: help text id; 0 ends the list */
    s16 action; /* 0x2: confirm action, see above */
    union {
        s32 value; /* page, close value, level or language */
        struct {
            u16 lo; /* action 6: menu_system.unkE4 */
            s16 hi; /* action 6: menu_action_messages index, 0 = none */
        } half;
    } param; /* 0x4 */
    s16 subtext; /* 0x8: second line text id, 0 = none */
    s16 fade_timer; /* 0xA: up while selected, down otherwise */
};

struct MenuTextListData {
    s32 flags; /* 0x30: 1 pad repeat, 2 no highlight, 4/8 font, 0x10 fixed step, 0x20 D_001A0314, 0x1000 wrap, 0x8000 scroll once */
    struct MenuTextItem *items; /* 0x34: ended by a zero text */
    struct MenuScreen *up; /* 0x38: focus above the first entry */
    struct MenuScreen *down; /* 0x3C: focus below the last entry */
    s32 selected; /* 0x40: highlighted entry */
    s32 scroll; /* 0x44: text offset, keeps the selection visible */
};

struct MenuStreamEntry {
    s32 sector; /* 0x0: start_audio_stream_read sector */
    s32 sector_count; /* 0x4: 0 = nothing to load */
};

typedef struct MenuStreamEntry MenuStreamEntry;

struct MenuStreamData {
    struct MenuStreamEntry *entries; /* 0x30 */
    s32 flags; /* 0x34: see above */
    s32 texture_width; /* 0x38: draw size of the streamed image */
    s32 texture_height; /* 0x3C */
    s32 *language_base; /* 0x40: base entry for flags 0x1000 */
    s32 state; /* 0x44: see above */
    s32 buffer[2]; /* 0x48: stream buffers */
    s32 loaded_entry[2]; /* 0x50: entry held by each buffer, -1 none */
    s32 fixed_entry; /* 0x58: entry for flags 1, -1 none */
    s32 elapsed_frames; /* 0x5C: updates since enter */
    s32 read_offset; /* 0x60: data offset of a flags 0x20 read */
};

struct MenuItemPreviewData {
    u8 pad_30[0x4];
    s32 state; /* 0x34 */
    f32 rotation_angle; /* 0x38 */
    u8 pad_3C[0x8];
    struct Moby *moby; /* 0x44: preview moby (create_menu_preview_moby) */
    struct Moby *second_moby; /* 0x48: optional, drawn after moby */
};

struct MenuLabelData {
    s32 flags; /* 0x30: see above */
    s32 text_id; /* 0x34: help text id, or table of text ids per value */
    u32 text_stride; /* 0x38: byte stride of the text_id table */
    s32 scroll_offset; /* 0x3C: text scroll, 1/16 px */
    u8 pad_40[0x4];
    s32 fade_timer; /* 0x44: reset to scale_game_frames(menu_fade_duration) */
    s32 cached_value; /* 0x48: value shown while fading */
    s32 value_variant; /* 0x4C */
};

struct MenuTextBoxData {
    s32 flags;                  /* 0x30 */
    union {
        s32 text_id;            /* 0x34: single text id */
        s32 *text_list;         /* 0x34: -1-terminated list of text ids */
        char *page;             /* 0x34: credits page pointer */
    };
    u8 pad_38[0x4];             /* 0x38 */
    s32 scroll;                 /* 0x3C: line scroll */
    s32 countdown;              /* 0x40: scaled timer */
    u8 pad_44[0xC];             /* 0x44 */
    s32 step;                   /* 0x50: sequence state/step */
    s32 scroll_done;            /* 0x54: flagged when text ends */
};

struct MenuScreen {
    s32 (*update)(struct MenuScreen *); /* 0x00 */
    s32 (*draw)(struct MenuScreen *);   /* 0x04: per-frame draw callback */
    void (*enter)(struct MenuScreen *, s32); /* 0x08 */
    void (*leave)(struct MenuScreen *, s32); /* 0x0C */
    union {
        s32 unk10; /* 0x10: bit 2 set by FUN_0021d1f8 */
        s32 flags;
    };
    s32 moby; /* 0x14: its menu moby D_001D5D90[i]; menu sounds play on it */
    s32 x; /* 0x18: left edge */
    s32 y; /* 0x1C: top edge */
    s32 width; /* 0x20 */
    s32 height; /* 0x24 */
    u8 pad_28[0x8];
    union {
        struct MenuScreenWords raw; /* not understood yet */
        struct MenuOptionListData options; /* FUN_00220e28 */
        struct MenuChoiceListData choices; /* FUN_002212b8 */
        struct MenuMissionListData missions; /* FUN_0021c4c0 */
        struct MenuItemGridData grid; /* draw_menu_item_grid */
        struct MenuIconListData icons; /* FUN_00219fa0 */
        struct MenuSaveSlotData save; /* saving_data_menu, saving_data_menu2 */
        struct MenuItemSlotsData slots; /* FUN_0021c7a0 */
        struct MenuCycleData cycle; /* update_menu_cycle_selection */
        struct MenuTextListData list; /* draw_menu_text_list, fun_0021abf8 */
        struct MenuStreamData stream; /* fun_0021fdc8, fun_0021f990 */
        struct MenuItemPreviewData preview; /* FUN_0021e110, update_item_preview_moby */
        struct MenuLabelData label; /* fun_0021a328 */
        struct MenuPromptData prompt; /* draw_prompt_box, fun_00221a48 */
        struct MenuTextBoxData textbox; /* draw_menu_text_box, update_timed_menu_sequence */
    } data; /* 0x30 */
};

typedef struct MenuScreen MenuScreen;

struct MenuPage {
    s32 moby_anims[14]; /* 0x00: animation per menu moby */
    struct MenuPage *back; /* 0x38: page for the back button (pad 0x10); 0 = none */
    s32 state; /* 0x3C: menu_system.state while this page is current */
    struct MenuScreen *focus; /* 0x40: screen with input focus */
    struct MenuScreen *screens[14]; /* 0x44 */
    u8 pad_7C[0x4];
    struct MenuScreen *pending_focus; /* 0x80: becomes focus on the next update */
    s32 confirmed; /* 0x84: confirm page answer, 1 = yes */
};

typedef struct MenuPage MenuPage;

struct MenuSystem {
    /* Page switching (FUN_002192a8) */
    s32 state; /* 0x000: menu state, from page->state; 0x2D at init */
    struct MenuPage *current; /* 0x004: active page */
    struct MenuPage *next; /* 0x008: requested page */
    s32 close_request; /* 0x00C: nonzero closes the menu */
    s32 unk10; /* 0x010: buffer address D_001940C0[2] + 0xA0000 */
    s32 timer; /* 0x014: 12 on a switch, 2 on close; counts down */
    s32 saved_texture_start; /* 0x018: gs_texture_allocation_start to restore on close */
    s32 current_gadget; /* 0x01C: gadget of the queued preview animation, -1 none */
    u8 pad_20[0x10];
    s32 equipped[4]; /* 0x030: gadget id held by each slot */
    u8 pad_40[0x60];

    /* Preview animation stream */
    s32 stream_buffer[2]; /* 0x0A0: double buffer */
    s32 unkA8; /* 0x0A8: fun_00226848 first-resource argument */
    s32 unkAC; /* 0x0AC: fun_00226848 count argument */
    s32 unkB0[3]; /* 0x0B0: buffer addresses read by fun_00226848 */
    s32 read_offset; /* 0x0BC: offset of the data in the read buffer */
    s32 unkC0; /* 0x0C0: -1 at preview init */
    u8 pad_C4[0x4];
    u8 loaded_animation[2]; /* 0x0C8: animation held by each buffer, 0xFF none */
    u8 read_buffer_index; /* 0x0CA: buffer being read */
    u8 pending_buffer; /* 0x0CB: buffer index + 1 while a read is in flight */
    u32 streamed_animation_base; /* 0x0CC: first animation id read from the stream archive */

    struct MenuPage *previous; /* 0x0D0: page left by the last switch */
    s32 unkD4; /* 0x0D4: 0 saving_data_menu, 1/2 saving_data_menu2; picks draw_prompt_box text */
    s32 unkD8; /* 0x0D8: fun_002196b8 draws slot 6 only while set */
    s32 unkDC; /* 0x0DC: pause_all_sounds: (mode == 0x23) */
    s32 unkE0; /* 0x0E0: memcard_make_whole_save / memcard_restore_game argument */
    s32 unkE4; /* 0x0E4: written with unkF0/unkF4 before close_request 3 */
    u8 pad_E8[0x4];
    s32 unkEC; /* 0x0EC: menu_action_messages[] entry (fun_0021abf8) */
    struct MenuPage *unkF0; /* 0x0F0: menu_system.current, or D_001CF418 (draw_map_screen) */
    s32 unkF4; /* 0x0F4: 0, 2, 0xB or 0xF */
    s32 unkF8; /* 0x0F8: read by pause_all_sounds */

    /* Buffers carved at init (FUN_00218f98) */
    s32 unkFC; /* 0x0FC: from D_001940C0[1] */
    s32 unk100; /* 0x100: from D_001940C0[2] */
    s32 unk104; /* 0x104: D_001940C0[1] + 0xA0000 */
    s32 help_text_buffer; /* 0x108: help text read target, D_001940C0[1] + 0xDC000 */
    s32 unk10C; /* 0x10C: D_001940C0[2] + 0x180000 */

    s32 update_count; /* 0x110: FUN_002192a8 updates */
    u8 pad_114[0x4];

    /* Preview moby resources */
    s32 resource_table_toggle; /* 0x118: alternates per resource table select */
    s32 unk11C; /* 0x11C: an oclass, -1 none */
    s32 unk120; /* 0x120: an oclass, -1 none */

    s32 close_locked; /* 0x124: nonzero blocks start/back closing the menu */
    s32 card_op_pending; /* 0x128: card save/load started, waiting for the card */
    s32 card_op_text; /* 0x12C: text shown while pending (0x4FB5 save, 0x4FB6 load) */
    u8 pad_130[0x4];
    s32 unk134; /* 0x134: hides pages / items with flags 8 */
    s32 unk138; /* 0x138: hides pages / items with flags 4 */
    s32 unk13C; /* 0x13C: written by FUN_00218d78 */
    s32 unk140; /* 0x140: oclass at the last resource table select */
    s32 last_resource_table_toggle; /* 0x144: resource_table_toggle at the last select */
    union {
        s32 unk148; /* 0x148: PAL only, counts down to 0 */
        s32 select_cooldown;
    };
    u8 pad_14C[0x4];
    s32 unk150; /* 0x150: PAL only: 0 after action 4, 1 after action 5 */
    union {
        s32 unk154; /* 0x154: PAL only, updates since the memory card went idle */
        s32 ticks;
    };
};

typedef struct MenuSystem MenuSystem;

struct MenuGridCell {
    u16 icon; /* 0x0: get_icon_frame icon */
    s16 frame; /* 0x2: first frame */
    s16 kind; /* 0x4: 0 = item (owned D_0013D4C0[id]), else D_0013D388[id] */
    s16 id; /* 0x6: item id */
    s16 stream_entry; /* 0x8: stream entry shown for this cell (fun_0021fdc8) */
};

typedef struct MenuGridCell MenuGridCell;

/* PVars layout for the 14 panel mobys in panel_slots (D_001D6120). */
typedef struct PanelMobyPvars {
    Vec4 frame_vectors[4];            /* 0x00..0x40: corner vectors */
    f32 width;                        /* 0x40 */
    f32 height;                       /* 0x44 */
    u8 pad_48[0x8];
    s32 screen_x;                     /* 0x50 */
    s32 screen_y;                     /* 0x54 */
    s32 projected_width;              /* 0x58 */
    s32 projected_height;             /* 0x5C */
} PanelMobyPvars;

#endif /* MENU_H */
