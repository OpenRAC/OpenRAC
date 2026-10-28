#include "common.h"
#include "moby.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * vendor.cpp in the original source; text 0x239628-0x23B670.
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
extern int race_gates_24_status NOT_SDA;
extern unsigned char menu_item_enabled_12 NOT_SDA;
extern int race_gates_47_flag NOT_SDA;
extern unsigned char gSpecialItems[];
extern unsigned char gHaveHeliPack NOT_SDA;
extern int race_gates NOT_SDA;
extern int race_gates_66_status NOT_SDA;
extern unsigned char menu_item_enabled_73 NOT_SDA;
extern int race_gates_70_status NOT_SDA;
extern unsigned char menu_item_enabled_85 NOT_SDA;
extern int race_gates_87_status NOT_SDA;
extern unsigned char race_gates_base NOT_SDA;
extern int item_available NOT_SDA;
extern unsigned char menu_item_enabled_20 NOT_SDA;
extern unsigned char gemlik_available NOT_SDA;
extern unsigned char gHaveMorphORay NOT_SDA;
extern unsigned char gHaveHologuise NOT_SDA;
extern int moby_render_slots[];
extern void copy_matrix_rotation_2(void *, void *) __asm__("func_001FA460");
extern void FastVecScale(void *, void *, float);
extern void FastVecAdd(void *, void *, void *);
extern void matrix_mul_vec3(void *, void *, void *);
extern int g_hero_moby NOT_SDA;
extern float shadow_light_dir[] NOT_SDA;
extern float FastCos(float);
extern float FastSin(float);
extern void flush_cache(int);
extern char normal_lut[];
extern int render_packet_end;
extern short vu1_chain_safe_end;
extern char moby_backup_buffer[];
extern int func_001160D8(void);
extern float random_angle_radians(void);
extern float random_float_between(float, float);
extern void BuildSphericalOffset(void *, float, float, float);
extern void fast_vec_normalize(void *, void *, float);
extern void copy_matrix_rotation(void *);
extern void MatrixToQuaternion(void *, void *);
extern void matrix_copy_rows(void *, void *);
extern float MobyGetFrameValue(void);
extern float round_float_to_decimal_places(int, float);
extern unsigned char gold_bolts_collected[];
extern unsigned char gold_weapon_purchased[];
extern unsigned char gSkillPoints[];
extern void snd_StreamSafeCdCallback(void *);
extern void finish_audio_stream_read(int);
extern short cd_read_state[];
extern void snd_ContinueVAGStream(void *);
extern void snd_StreamSafeCdBreak(void);
extern char load_file_start_failed_string[];
extern int snd_StreamSafeCdRead(int, int, int, void *);
extern int start_audio_stream_read_v(void) __asm__("func_00217628");
extern void sce_gs_sync_v(int);
extern void music_Update(void);
extern void snd_reset_state_and_flush_commands(void);
extern void snd_FlushSoundCommands(void);
extern void snd_reenable_command_flush(void);
extern int snd_StreamSafeCdGetError(void);
extern void music_start_track_by_id(short, short, short);
extern void music_PreseekTrack(short, short, short);
extern short music_flagged_track_started NOT_SDA;
extern char g_controller_state[];
extern int pad_socket_param NOT_SDA;
extern void sce_dbc_init(void);
extern void sce_pad2_init(int);
extern int sce_pad2_create_socket(void *, void *);
extern void poll_pad_device_state(void *);
extern int pause_action_list_mode;
extern char pause_action_list_pal[];
extern char pause_action_list_short[];
extern char menu_system[] NOT_SDA;
extern char animation_asset_read_active[];
extern int selected_level_index;
extern int selected_mission_id;
extern int selected_mission_index;
extern void update_mission_list(void *);
extern int collect_mission_ids(void *, void *, void *, int);
extern int pause_iop_sprite_lists[];
extern char heli_pack_menu_table[];
extern char no_helipack_menu_table[];
extern int pause_memory_card_state NOT_SDA;
extern float FastAddRots(float, float);
extern char *current_menu_page NOT_SDA;
extern void DrawMobyList(int, int);
extern char gCamera[];
extern void rotate_preview_moby_x(void *);
extern void *create_menu_preview_moby_a(int) __asm__("func_00226720");
extern int delete_moby(int);
extern void VU1_addGSregister(int, int);
extern void draw_map_overlay(void);
extern void SetupGifPaging(int);
extern void DoGifPaging(void);
extern void func_001F68E8_c(int, int, long, void *, int)
    __asm__("func_001F68E8");
extern void *msg_string_id(int) __asm__("func_001FE540");
extern short panel_clear_color;
extern void draw_hud_rect_blended(int, int, int, int, long, long);
extern int GetIconFrame(int, int);
extern void HudSprite(int, int, int, int, int, int);
extern void DrawTexturedQuad(int, int, int, int, int, int, int, int, long,
                          long);
extern short fs_aa_buffer[];
extern long transition_overlay_tex0;
extern int acquire_menu_stream_buffer(int);
extern int release_menu_stream_buffer(int);
extern int pad0_pressed_unmasked NOT_SDA;
extern char pause_default_action_list[];
extern char *pause_current_action_list NOT_SDA;
extern unsigned char resident_class_slot_by_id[] NOT_SDA;
extern void *CreateMoby(void);
extern void pack_render_command_fields(void *, int, int, int, int);
typedef struct {
    int key;
    int flags;
} PadBind;
extern PadBind menu_stream_buffers_t[] __asm__("D_001D6448");
extern int get_menu_stream_buffer_size(int handle);
extern int menu_stream_buffers[];
extern char preview_resource_bindings[] NOT_SDA;
extern char *moby_class_resources[] NOT_SDA;
extern int scene_state_buffer_0[];
extern int scene_state_buffer_1[];
extern int scene_state_buffer_2[];
extern char camera_position_a[] __asm__("D_00187180");
extern char coll_hit_pos[];
extern int sound_probe_ignore;
extern void random_spherical_offset(void *, float, float);
extern void FastVecAdd_a(void *, void *, void *) __asm__("func_001F9BD8");
extern void FastVecScale_a(void *, void *, float) __asm__("func_001F9C30");
extern void FastVecSub_a(void *, void *, void *) __asm__("func_001F9BF0");
extern int coll_line_fix_a(void *, void *, int, int, int) __asm__("func_001EFE10");
extern char camera_position[];
extern void FastVecSub(void *, void *, void *);
extern void coll_line_fix(void *, void *, int, int, int);
extern float FastVecDist(int, void *);
extern void calculate_voice_distance_volume(void *, float, float, float);
extern void transform_vector(void *, void *, void *);
extern float vec_length_xy(void *);
extern float fast_arc_tan_a(float, float) __asm__("func_001FA058");
extern void truncate_float_to_s32(float);
extern void sce_cd_sync_s(int);
extern int snd_BankLoadByLoc(void *, int);
extern void snd_StreamSafeCheckCDIdle(int);
extern void snd_StreamSafeCdSync(int);
extern void snd_ResolveBankXREFS(void);
extern void sceSifInitRpc(int arg0);
typedef struct {
    int unk_00;
    int unk_04;
    int unk_08;
    int unk_0C;
} Rec10;
extern Rec10 stash_entries[];
typedef struct {
    short a;
    short b;
} TexRemap;
typedef struct {
    char *items;
    int count;
} TexChunk;
extern TexChunk tfrag_gif_chunk_list[];
extern TexRemap tfrag_tex_remap_table[];
extern short vif_chain_flags;
extern char vu1_sync_timeout_msg[];
extern void reset_graphics(void);
extern void PatchTieGifs(void);
extern char tie_gif_patch_list[];
extern char tie_gif_patch_list_saved[];
extern char tie_tex_remap_table[];
extern char tie_tex_remap_table_saved[];
extern void LightTies(void *);
extern char tie_light_data[];
extern char tie_light_data2[];
extern char vendor_state[];
extern char str_scrolling_status_fmt[];
extern void copy_text_to_shared_buffer(void *arg0);
extern void transform_scaled_vertex_batch(char *arg0, int arg1, void *arg2, char *arg3);

/* functional: medium. The NTSC decomp's pending C for the same instructions (a draft it has not matched
   yet), carried to PAL's names by OpenRAC's tools/port.py. Never compared with retail's bytes here. */
extern s32 capture_glyph_coordinates[64];
extern s32 capture_glyph_advances[64];
extern s32 game_frame_counter MACRO_ADDR;
extern void DrawTexturedQuad(s32, s32, s32, s32, s32, s32, s32, s32, s64, s64);
extern s32 scale_game_frame_count(s32);
extern f32 IntToFloat(s32);
#define TRUNC(x) ((s32 (*)(f32))func_001FA898)(x)
extern u64 GetFrameTex(s32);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/assembly/textbin/fun_00238310.c, render_capture_scrolling_text. */
void render_capture_scrolling_text(char *text, s32 start_x, s32 start_y, f32 scale) {
    s64 texture;
    s32 cursor_x;
    s32 cursor_y;
    s32 glyph_index;
    s32 blink_next_glyph;
    s32 skip_draw;
    s32 long_blink_period;
    s32 short_blink_period;
    s32 blink_period;
    s32 blink_remainder;
    char *cursor_text;
    s32 packed_coordinates;
    s32 texture_v;
    s32 texture_u;
    s32 glyph_width;
    s32 glyph_height;
    u8 character;

    blink_next_glyph = 0;
    cursor_text = text;
    texture = GetFrameTex(GetIconFrame(0xE935, 0));
    cursor_x = start_x;
    cursor_y = start_y;

    for (;;) {
        skip_draw = 0;
        if (*(u8 *)cursor_text == 0) {
            return;
        }
        character = (u8)*cursor_text++;
        glyph_index = (s32)character - 0x20;

        if (glyph_index == 0x42) {
            blink_next_glyph = 1;
        }
        /* Retail retains this branch even though an unsigned byte minus 0x20 cannot reach 0xEA. */
        if (glyph_index == 0xEA) {
            cursor_x = start_x;
            cursor_y += 9;
        }

        if (glyph_index < 0x3B) {
            {
                s32 *glyph_entry;
                glyph_entry = capture_glyph_coordinates + glyph_index;
                if (*glyph_entry != -1 || glyph_index == 0) {
                    if (blink_next_glyph != 0) {
                        skip_draw = 0;
                        blink_next_glyph = 0;
                        long_blink_period = scale_game_frame_count(0x1E);
                        short_blink_period = scale_game_frame_count(0xA);
                        blink_period = long_blink_period + short_blink_period;
                        blink_remainder = game_frame_counter % blink_period;
                        long_blink_period = scale_game_frame_count(0xA);
                        if (blink_remainder < long_blink_period) {
                            skip_draw = 1;
                        }
                    }

                    if (glyph_index != 0 && skip_draw == 0) {
                        packed_coordinates = *glyph_entry;
                        texture_v = (u32)packed_coordinates >> 0x14;
                        texture_u = (packed_coordinates & 0xFFFF) >> 4;

                        if (cursor_x + 9 > 0) {
                            if (cursor_x < 0x100) {
                                glyph_width =
                                    TRUNC(IntToFloat(9) * scale);
                                glyph_height =
                                    TRUNC(IntToFloat(9) * scale);
                                DrawTexturedQuad(cursor_x, cursor_y, glyph_width, glyph_height,
                                                   texture_u, texture_v, 9, 9, (s64)0x80404040,
                                                   texture);
                            }
                        }
                    }

                    cursor_x += TRUNC(
                        IntToFloat(capture_glyph_advances[glyph_index]) * scale);
                }
            }
        }
    }
}

extern int libc_strlen_s(char *) __asm__("func_00116810");
extern int random_integer_below(int);
extern int status_message_text_ids[];
extern void set_scrolling_status_message(void *);
extern void render_capture_scrolling_text(char *, int, int, float);
extern void draw_framebuffer_rect(int, int, int, int, int, int, int);

/* Two base locals, one before the join and one after, make gcse share
   only the %hi and rebuild the %lo base after the join; the first must
   be assigned after the first call. func_00116810 is strlen. */
void update_scrolling_status_message(void) {
    char *v;
    char *w;
    draw_framebuffer_rect(0, 0, 0x200, 0x80, 0x200, 0x80, 0);
    v = vendor_state;
    if (libc_strlen_s(*(char **)(v + 0x2C)) - *(int *)(v + 0x44) / 20 > 0) {
        *(int *)(v + 0x44) += 2;
    } else {
        set_scrolling_status_message(msg_string_id(status_message_text_ids[random_integer_below(0x18)]));
    }
    w = vendor_state;
    render_capture_scrolling_text(*(char **)(w + 0x2C), -*(int *)(w + 0x44), 8, 2.0f);
    draw_framebuffer_rect(0, 0, 4, 0x40, 0x200, 0x80, 0);
    draw_framebuffer_rect(0xE2, 0, 0xE6, 0x40, 0x200, 0x80, 0);
}

extern int capture_moby_pass_enabled MACRO_ADDR;
extern int capture_moby_list;

/* Draws the capture framebuffer rect and its moby list when enabled. */
void render_capture_effect_pass(void) {
    draw_framebuffer_rect(0, 0, 0x200, 0x80, 0x200, 0x80, 0);
    if (capture_moby_pass_enabled == 1) {
        DrawMobyList(capture_moby_list, 1);
    }
}

extern char fmt_thousands_grouped_int[];
extern char fmt_plain_int[];
/* Unprototyped: the two call sites pass different argument counts. */
extern void sprintf();

/* Formats a number into a string, with a thousands separator from 1000 up. */
void format_scaled_display_value(void *arg0, int arg1) {
    if (arg1 >= 1000) {
        sprintf(arg0, fmt_thousands_grouped_int, arg1 / 1000, arg1 % 1000);
    } else {
        sprintf(arg0, fmt_plain_int, arg1);
    }
}

struct VendorMenuState {
    u8 pad_0[0x4];
    s32 pulseTick;
    u8 pad_8[0x44];
    s32 scrollOffset;
    u8 pad_50[0x8];
    s32 selectedIndex;
    u8 pad_5C[0x1B4];
    s32 slotCount;
};

struct VendorItemResource {
    u8 pad_0[0x38];
    u16 iconId;
};

extern u8 item_info_table[];
extern struct VendorMenuState vendor_state_ui __asm__("D_001E66C0");
extern s32 subtract_integer_with_clamp();
extern void func_001FBAB8_u(int, int, int, int, int, int, unsigned int)
    __asm__("func_001FBAB8");

/* Vendor item carousel and icon pulse. The solid black draw calls use the
   unsigned color signature, matching retail's argument setup order. */
void render_vendor_item_icon_strip_pass(void) {
    s32 selectedOffset;
    s32 slotOffset;
    s32 selectedMinusThree;
    s32 x;
    s32 i;
    s32 itemIndex;
    s32 itemX;
    s32 displayIcon;
    s32 itemIcon;
    u16 displayIconId;
    u16 itemIconId;
    s32 *slotKind;
    s32 pulseColor;

    draw_framebuffer_rect(0, 0, 0x200, 0x80, 0x200, 0x80, 0);
    if (vendor_state_ui.slotCount < 8) {
        itemX = 0xC;
        itemIndex = 0;
        selectedOffset = vendor_state_ui.selectedIndex * 0x38;
        pulseColor = ((subtract_integer_with_clamp(((vendor_state_ui.pulseTick * 4) & 0x3F) - 0x20) + 0x40) * 0x10202) | 0x80000000;
        draw_framebuffer_rect(selectedOffset + 8, 2, selectedOffset + 0x40, 0x3A, 0x200, 0x80, pulseColor);
        func_001FBAB8_u(selectedOffset + 0xA, 4, selectedOffset + 0x3E, 0x38, 0x200, 0x80, 0x80000000);
        if (vendor_state_ui.slotCount > 0) {
            do {
                slotKind = (s32 *) (void *) ((u8 *)&vendor_state_ui + 0xD4 + itemIndex * 0x14);
                itemIconId = ((struct VendorItemResource *) (void *) ((u8 *)item_info_table + slotKind[-1] * 0x4C))->iconId;
                if (slotKind[0] == 1) {
                    itemIcon = GetIconFrame(itemIconId, 2);
                } else {
                    itemIcon = GetIconFrame(itemIconId, 0);
                }
                HudSprite(itemIcon, itemX, 6, 0x30, 0x30, 0x80);
                itemX += 0x38;
                itemIndex += 1;
            } while (itemIndex < vendor_state_ui.slotCount);
        }
    } else {
        if (vendor_state_ui.scrollOffset < 0) {
            vendor_state_ui.scrollOffset = (s32) (vendor_state_ui.scrollOffset + 4);
        } else if (vendor_state_ui.scrollOffset > 0) {
            vendor_state_ui.scrollOffset = (s32) (vendor_state_ui.scrollOffset - 4);
        } else {
            draw_framebuffer_rect(0xB0, 2, 0xE8, 0x3A, 0x200, 0x80, ((subtract_integer_with_clamp(((vendor_state_ui.pulseTick * 4) & 0x3F) - 0x20) + 0x40) * 0x10202) - (s32) 0x80000000);
            func_001FBAB8_u(0xB2, 4, 0xE6, 0x38, 0x200, 0x80, 0x80000000);
        }
        x = vendor_state_ui.scrollOffset - 0x64;
        i = -2;
        do {
            selectedMinusThree = vendor_state_ui.selectedIndex - 3;
            slotOffset = ((s32) ((vendor_state_ui.slotCount * 2) + i + selectedMinusThree) % (s32) vendor_state_ui.slotCount) * 0x14;
            displayIconId = ((struct VendorItemResource *) (void *) ((u8 *)item_info_table + *(s32 *)(void *)(slotOffset + ((u8 *)&vendor_state_ui + 0xD0)) * 0x4C))->iconId;
            if (*(s32 *)(void *)(slotOffset + ((u8 *)&vendor_state_ui + 0xD4)) == 1) {
                displayIcon = GetIconFrame(displayIconId, 2);
            } else {
                displayIcon = GetIconFrame(displayIconId, 0);
            }
            i += 1;
            HudSprite(displayIcon, x, 6, 0x30, 0x30, 0x80808080);
            x += 0x38;
        } while (i < 9);
    }
}

struct VM239 {
    char pad0[0x20];
    int f20;
    char pad24[0x1C];
    int f40;
    char pad44[0x14];
    int idx;
    char pad5C[0x74];
    struct { int id; int kind; int pad[3]; } slots[1];
};
struct T239 { int text; char pad[0x48]; };
struct P239 { int a; int b; unsigned short c; unsigned short d; int pad[3]; };

extern void FontPrintSmall_c(int, int, long, void *, int) __asm__("func_001F6968");
extern void format_scaled_display_value(void *arg0, int arg1);
extern void font_print_right_small_c(int, int, long, void *, int) __asm__("func_001F6D88");
extern struct VM239 vendor_state_m __asm__("D_001E66C0");
extern struct T239 item_info_table_t[] __asm__("D_001864D0");
extern struct P239 vendor_item_prices_p[] __asm__("D_001E02B0");
extern unsigned char item_available_b[] __asm__("D_0013D5C8") NOT_SDA;

/* Vendor item panel: draws the selected item's name and description, and for the ammo/alt slots a pulsing price shadow. Adapted from Lombyte's project (GPL-3.0) for PAL: src/ui/menus/fun_002389e0.c, fun_002389e0. */
void render_vendor_item_details_pass(void) {
    char buf[0x100];
    int w;

    draw_framebuffer_rect(0, 0, 0x200, 0x80, 0x200, 0x80, 0);
    DrawMobyList(vendor_state_m.f20 + 0x100, 1);
    DrawMobyList(vendor_state_m.f20, 1);
    if (vendor_state_m.slots[vendor_state_m.idx].kind == 1) {
        FontPrintSmall_c(6, 8, 0x80F0F0F0L, msg_string_id(item_info_table_t[vendor_state_m.slots[vendor_state_m.idx].id].text), -1);
        FontPrintSmall_c(0x18, 0x18, 0x80F0F0F0L, msg_string_id(0x4F5D), -1);
        format_scaled_display_value(buf, vendor_item_prices_p[vendor_state_m.slots[vendor_state_m.idx].id].c);
        font_print_right_small_c(0x76, 0x65, 0x80F0F0F0L, buf, -1);
        if (vendor_state_m.f40 != 0) {
            w = measure_text_width_regular((unsigned char *)buf, -1);
            draw_hud_rect_blended(0x75 - w, 0x6D, 0x7B, 0x70, 0x20959544L, 0);
            draw_hud_rect_blended(0x76 - w, 0x6D, 0x7A, 0x70, 0x30959544L, 0);
            draw_hud_rect_blended(0x77 - w, 0x6D, 0x79, 0x70, 0x40959544L, 0);
            draw_hud_rect_blended(0x78 - w, 0x6D, 0x78, 0x70, 0x50959544L, 0);
            draw_hud_rect_blended(0x79 - w, 0x6D, 0x77, 0x70, 0x60959544L, 0);
            draw_hud_rect_blended(0x7A - w, 0x6D, 0x76, 0x70, 0x70959544L, 0);
            draw_hud_rect_blended(0x7B - w, 0x6D, 0x75, 0x70, 0x80959544L, 0);
            format_scaled_display_value(buf, vendor_state_m.f40 ? vendor_item_prices_p[vendor_state_m.slots[vendor_state_m.idx].id].d
                                                 : vendor_item_prices_p[vendor_state_m.slots[vendor_state_m.idx].id].c);
            font_print_right_small_c(0x76, 0x55, 0x80F0F0F0L, buf, -1);
        }
    } else {
        FontPrintSmall_c(6, 8, 0x80F0F0F0L, msg_string_id(item_info_table_t[vendor_state_m.slots[vendor_state_m.idx].id].text), -1);
        format_scaled_display_value(buf, vendor_item_prices_p[vendor_state_m.slots[vendor_state_m.idx].id].a);
        font_print_right_small_c(0x76, 0x65, item_available_b[0x23] ? 0x80808080L : 0x80F0F0F0L, buf, -1);
        if (item_available_b[0x23] != 0) {
            w = measure_text_width_regular((unsigned char *)buf, -1);
            draw_hud_rect_blended(0x75 - w, 0x6D, 0x7B, 0x70, 0x20959544L, 0);
            draw_hud_rect_blended(0x76 - w, 0x6D, 0x7A, 0x70, 0x30959544L, 0);
            draw_hud_rect_blended(0x77 - w, 0x6D, 0x79, 0x70, 0x40959544L, 0);
            draw_hud_rect_blended(0x78 - w, 0x6D, 0x78, 0x70, 0x50959544L, 0);
            draw_hud_rect_blended(0x79 - w, 0x6D, 0x77, 0x70, 0x60959544L, 0);
            draw_hud_rect_blended(0x7A - w, 0x6D, 0x76, 0x70, 0x70959544L, 0);
            draw_hud_rect_blended(0x7B - w, 0x6D, 0x75, 0x70, 0x80959544L, 0);
            format_scaled_display_value(buf, item_available_b[0x23] ? vendor_item_prices_p[vendor_state_m.slots[vendor_state_m.idx].id].b
                                                 : vendor_item_prices_p[vendor_state_m.slots[vendor_state_m.idx].id].a);
            font_print_right_small_c(0x76, 0x55, 0x80F0F0F0L, buf, -1);
        }
    }
}

typedef struct { int type; int kind; int pad[3]; } VSlotA;
typedef struct {
    char pad0[0x40];
    int alt;
    char pad1[0x14];
    int idx;
    int sel;
    char pad2[0x70];
    VSlotA slots[1];
} VendorA;
typedef struct {
    int first;
    int second;
    unsigned short price;
    unsigned short priceAlt;
    char pad1[2];
    unsigned short max;
    char pad2[8];
} WeaponInfoA;
extern int weapon_ammo_counts[];
extern int gBolts MACRO_ADDR;
extern char vendor_item_prices[];
extern unsigned char discount_purchase_pricing[];
extern void libc_memset(void *, int, int);
extern void font_print_window_small(void *, long, void *, int);

/* Draws the vendor's buy label and price for the selected slot. */
void render_vendor_buy_label_pass(char *unused, int x, int y) {
    char sp[0x18];
    VendorA *v;
    int idx;
    int m;
    int price;
    int sz = 0x18;

    draw_framebuffer_rect(0, 0, 0x200, 0x80, 0x200, 0x80, 0);
    ((void *(*)(void *, int, int))libc_memset)(sp, 0, 0x18);
    *(short *)(sp + 0x2) = y;
    *(short *)(sp + 0x6) = x;
    *(short *)(sp + 0x8) = x >> 1;
    *(short *)(sp + 0xA) = (y >> 1) - 7;
    *(short *)(sp + 0x10) = 0x10;
    *(short *)(sp + 0x12) = 1;
    if (((VendorA *)vendor_state)->sel != 0) {
        if (((VendorA *)vendor_state)->slots[((VendorA *)vendor_state)->idx].kind == 1 &&
            weapon_ammo_counts[((VendorA *)vendor_state)->slots[((VendorA *)vendor_state)->idx].type] >=
                ((WeaponInfoA *)(vendor_item_prices + ((VendorA *)vendor_state)->slots[((VendorA *)vendor_state)->idx].type * sz))->max) {
            m = 0x5238;
        } else {
            v = (VendorA *)vendor_state;
            if (v->slots[v->idx].kind == 1) {
                if (v->alt) {
                    price = ((WeaponInfoA *)(vendor_item_prices + v->slots[v->idx].type * 0x18))->priceAlt;
                } else {
                    price = ((WeaponInfoA *)(vendor_item_prices + v->slots[v->idx].type * 0x18))->price;
                }
                m = 0x5238;
                if (gBolts >= price)
                    m = 0x5239;
            } else {
                int cost;
                if (discount_purchase_pricing[0] != 0) {
                    cost = ((struct { WeaponInfoA a[1]; } *)vendor_item_prices)->a[v->slots[v->idx].type].second;
                } else {
                    cost = ((WeaponInfoA *)(vendor_item_prices + v->slots[v->idx].type * 0x18))->first;
                }
                m = gBolts < cost ? 0x5238 : 0;
            }
        }
    } else {
        m = 0x5239;
    }
    if (m != 0) {
        *(unsigned short *)(sp + 0x12) |= 4;
        font_print_window_small(sp, 0x80F0F0F0L, msg_string_id(m), -1);
        *(unsigned short *)(sp + 0x12) ^= 4;
        *(short *)(sp + 0xA) = (y - *(short *)(sp + 0xE)) >> 1;
        font_print_window_small(sp, 0x80F0F0F0L, msg_string_id(m), -1);
    }
}

extern int weapon_ammo_counts[];
extern int gBolts MACRO_ADDR;
extern char vendor_item_prices[];
extern int FontPrintCenterSmall_c(int, int, long, void *, int) __asm__("func_001F6F40");

typedef struct {
    int type; /* +0x0 */
    int kind; /* +0x4: 1 == ammo */
    int pad[3];
} VendorSlot;

typedef struct {
    char pad0[0x40];
    int alt;             /* +0x40 */
    char pad1[0x14];
    int idx;             /* +0x58 */
    int sel;             /* +0x5C */
    char pad2[0x70];
    VendorSlot slots[1]; /* +0xD0 */
} Vendor;
extern Vendor vendor_state_v __asm__("D_001E66C0");

typedef struct {
    char pad0[8];
    unsigned short price;    /* +0x8 */
    unsigned short priceAlt; /* +0xA */
    char pad1[2];
    unsigned short max;      /* +0xE */
    char pad2[8];
} WeaponInfo; /* 0x18 */

/* Vendor ammo prompt: draws the panel, then shows text 0x4EE0 when
   nothing is selected, or when the selected slot is an ammo slot
   (kind 1) that is not full (D_0013D530[type] < max) and the player's
   D_0015EE98 covers its price (priceAlt when +0x40 is set).
   The kind test is two separate ifs: the first one's failure jumps to
   the second, so CSE sees a label there and keeps the re-test retail
   has. The D_001E02B0 records are reached as char arithmetic cast to a
   struct, which gives retail's index-first addu with the field offset
   left as the load displacement. */
void VendorDrawAmmoPrompt(void) {
    Vendor *v;
    int idx;
    int price;

    draw_framebuffer_rect(0, 0, 0x200, 0x80, 0x200, 0x80, 0);
    v = &vendor_state_v;
    if (v->sel != 0) {
        idx = v->idx;
        if (v->slots[idx].kind == 1 &&
            weapon_ammo_counts[v->slots[idx].type] >=
                ((WeaponInfo *)(vendor_item_prices + v->slots[idx].type * 0x18))->max) {
            return;
        }
        if (v->slots[idx].kind == 1) {
            if (v->alt) {
                price = ((WeaponInfo *)(vendor_item_prices + v->slots[idx].type * 0x18))->priceAlt;
            } else {
                price = ((WeaponInfo *)(vendor_item_prices + v->slots[idx].type * 0x18))->price;
            }
            if (gBolts >= price) {
                FontPrintCenterSmall_c(0x28, 0x14, 0x80F0F0F0L, msg_string_id(0x4EE0), -1);
            }
        }
    } else {
        FontPrintCenterSmall_c(0x28, 0x14, 0x80F0F0F0L, msg_string_id(0x4EE0), -1);
    }
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

LINKER_REMNANT("asm/remnants/text", func_0023A5D8);

extern s32 vendor_flash_timers[];
extern s32 vendor_scroll_timers[];
extern void VU1_addGSregister_r(s32, u64) __asm__("func_00234C98");
extern s32 random_integer_below(s32);
extern s32 subtract_integer_with_clamp(s32);
extern s64 GetEffectTex_r(s32) __asm__("func_001F4868");
extern f32 IntToFloat(s32);
extern void append_subpixel_textured_screen_quad(f32, f32, f32, f32, s32, s32, s32, s32, u64, s64);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/textbin/render_vendor_capture_texture_overlays_pass.c, render_vendor_capture_texture_overlays_pass. */
void render_vendor_capture_texture_overlays_pass(s32 pass_index, f32 capture_width,
                                                 f32 capture_height) {
    s32 flash_timer;
    s32 scroll_opacity;
    s32 flash_opacity;
    f32 random_u;
    f32 random_v;
    f32 zero_offset;
    f32 scroll_offset;
    f32 overlay_height;
    s32 overlay_width;

    VU1_addGSregister_r(0x47, 0x32003);
    if (vendor_flash_timers[pass_index] != 0 || pass_index == 0) {
        if (vendor_flash_timers[pass_index] != 0) {
            vendor_flash_timers[pass_index] += 2;
        }
        flash_timer = vendor_flash_timers[pass_index];
        if (pass_index == 0) {
            /* The floor changes this frame's opacity, not the stored timer. */
            if (flash_timer < 0x18) {
                flash_timer = 0x18;
            }
        }
        random_u = random_integer_below(200);
        random_v = random_integer_below(200);
        zero_offset = 0.0f;
        flash_opacity = 0x80 - subtract_integer_with_clamp(flash_timer - 0x80);
        VU1_addGSregister_r(8, 0);
        flash_opacity *= 2;
        if (flash_opacity > 0x80)
            flash_opacity = 0x80;
        VU1_addGSregister_r(0x42, ((u64)flash_opacity << 32) | 0x68);
        append_subpixel_textured_screen_quad(0.0f, 0.0f, capture_width, capture_height,
                                             random_u + zero_offset, random_v + zero_offset,
                                             capture_width + random_u, capture_height + random_v,
                                             0x808080, GetEffectTex_r(0x1A));
        if (vendor_flash_timers[pass_index] >= 0x100) {
            vendor_flash_timers[pass_index] = 0;
        }
    }
    if (vendor_flash_timers[pass_index] == 0 && random_integer_below(700) == 0) {
        vendor_flash_timers[pass_index] = 2;
    }
    VU1_addGSregister_r(8, 0);
    VU1_addGSregister_r(0x42, 0x8000000044ULL);
    if (pass_index > 0) {
        if (vendor_scroll_timers[pass_index] != 0) {
            vendor_scroll_timers[pass_index] += 2;
            overlay_width = capture_width;
            scroll_opacity =
                0x100 - subtract_integer_with_clamp(vendor_scroll_timers[pass_index] - 0x100);
            if (scroll_opacity > 0x50) {
                scroll_opacity = 0x50;
            }
            scroll_offset =
                -(IntToFloat(0x200 - vendor_scroll_timers[pass_index]) * 0.03125f);
            overlay_height = capture_height + 16.0f;
            append_subpixel_textured_screen_quad(0.0f, scroll_offset, capture_width, overlay_height,
                                                 0, 0, overlay_width, (s32)(overlay_height * 1.5f),
                                                 (scroll_opacity << 24) | 0x505050,
                                                 GetEffectTex_r(0x1C));
            if (vendor_scroll_timers[pass_index] >= 0x200) {
                vendor_scroll_timers[pass_index] = 0;
            }
        } else if (random_integer_below(360) == 0) {
            vendor_scroll_timers[pass_index] = 2;
        }
    }
    if (pass_index == 6) {
        /* The known capture coordinator supplies only passes zero through five. */
        append_subpixel_textured_screen_quad(0.0f, 0.0f, capture_width, capture_height, 0, 0, 0x40,
                                             0x40, 0x80808080, GetEffectTex_r(0x19));
    }
}

extern void setup_render_target(int, int, int);
extern void configure_graphics_projection(int, int, float, float, float, float, float);
extern void VU1_addGSregister_l(int, long) __asm__("func_00234C98");

/* Sets up a (1 << a) x (1 << b) area: func_001FB608 gets the sizes and a
   base below 0x3FF000 by 4 << min(a + b, 16) bytes, rounded down to 8 KB
   (the GS page), func_001F3760 the sizes and the float setup (f, 0,
   524288, 255, 0); then GS registers 0x47 and 0x42 are written through
   func_00234C98, whose value argument is 64-bit (hence the long alias,
   which builds 0x8000000044 as one dli). */
void configure_capture_target(int a, int b, float f) {
    int t = a + b;

    if (t > 16) {
        t = 16;
    }
    setup_render_target(a, b, ((0x3FF000 - (4 << t)) >> 13) << 13);
    configure_graphics_projection(1 << a, 1 << b, f, 0.0f, 524288.0f, 255.0f, 0.0f);
    VU1_addGSregister_l(0x47, 0x30000);
    VU1_addGSregister_l(0x42, 0x8000000044L);
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");


/* Sets up the draw buffer and view context for rendering. */
void setup_view_context(void) {
    PutDrawBufferLarge();
    InitViewContext();
    UpdateViewContext();
}

typedef int u128_3AA38 __attribute__((mode(TI)));
extern float capture_bounds_adjustments[][4];
extern int capture_primary_transition MACRO_ADDR;
extern short capture_secondary_transition_active;
extern short capture_secondary_transition_step;
extern short capture_primary_transition_step;
extern int pal_mode MACRO_ADDR;
extern long capture_texture_tex0 MACRO_ADDR;
extern float FastVecLength(void *);
extern void project_graphics_bounds_float(void *, void *, float *, float *, float *, float *);
extern void draw_centered_capture_background(int, int);
extern void VU1_texFlush(void);
extern int truncate_float_to_s32_r(float) __asm__("func_001FA898");
extern void update_scrolling_status_message_r(char *) __asm__("func_00239838");
extern void render_vendor_item_details_pass_r(char *) __asm__("func_00239CF8");
extern void render_capture_effect_pass_r(char *) __asm__("func_00239948");
extern void render_vendor_buy_label_pass(char *, int, int);
extern void VendorDrawAmmoPrompt_r(char *) __asm__("func_0023A478");
extern void render_vendor_item_icon_strip_pass_r(char *) __asm__("func_00239A00");

/* Renders the six vendor monitor screens: projects each screen's joint box, renders its view off-screen and draws it back. */
void render_vendor_capture_pass_sequence(char *m) {
    float len;
    float f;
    float g;
    int i;
    long tex;

    for (i = 0; i < 6; i++) {
        u128_3AA38 d1[1];
        u128_3AA38 d2[1];
        u128_3AA38 base[1];
        u128_3AA38 out[3];
        int ids[3] = { i * 4, i * 4 + 1, i * 4 + 2 };
        u128_3AA38 tmp[1];
        u128_3AA38 c[4];
        u128_3AA38 pad[5];
        u128_3AA38 a[1];
        u128_3AA38 b[1];
        float sx, sy, sw, sh;
        float x, y, w, h;
        transform_scaled_vertex_batch(m, 3, ids, (char *)out);
        qcopy(base, out);
        FastVecSub(d1, &out[1], out);
        FastVecSub(d2, &out[2], out);
        len = FastVecLength(d1);
        fast_vec_normalize(tmp, d1, capture_bounds_adjustments[i][0]);
        FastVecAdd(base, base, tmp);
        fast_vec_normalize(d1, d1, len - 2.0f * capture_bounds_adjustments[i][2]);
        len = FastVecLength(d2);
        fast_vec_normalize(tmp, d2, capture_bounds_adjustments[i][1]);
        FastVecAdd(base, base, tmp);
        fast_vec_normalize(d2, d2, len - 2.0f * capture_bounds_adjustments[i][3]);
        FastVecAdd(tmp, base, d1);
        FastVecAdd(tmp, tmp, d2);
        project_graphics_bounds_float(base, tmp, &sx, &sy, &sw, &sh);
        if (capture_primary_transition != 0 || *(int *)&capture_secondary_transition_active != 0) {
            f = 1.0f;
            if (*(int *)&capture_secondary_transition_active != 0) {
                f = IntToFloat(8 - *(int *)&capture_secondary_transition_step) * 0.125f;
            }
            if (capture_primary_transition != 0) {
                f = IntToFloat(*(int *)&capture_primary_transition_step) * 0.125f;
            }
            len = FastVecLength(d1);
            g = 1.0f - f;
            fast_vec_normalize(tmp, d1, len * g * 0.5f);
            fast_vec_normalize(d1, d1, len * f);
            FastVecAdd(base, base, tmp);
            len = FastVecLength(d2);
            fast_vec_normalize(tmp, d2, len * g * 0.5f);
            fast_vec_normalize(d2, d2, len * f);
            FastVecAdd(base, base, tmp);
        }
        qcopy(&c[0], base);
        FastVecAdd(&c[1], base, d1);
        FastVecAdd(&c[2], base, d2);
        FastVecAdd(&c[3], &c[2], d1);
        a[0] = c[0];
        b[0] = c[3];
        project_graphics_bounds_float(a, b, &x, &y, &w, &h);
        configure_capture_target(9, 7, 1.0f);
        draw_centered_capture_background(0x200, 0x200);
        VU1_addGSregister_l(0x42, 0x8000000064L);
        switch (i) {
        case 0:
            update_scrolling_status_message_r(m);
            break;
        case 1:
            render_vendor_item_details_pass_r(m);
            break;
        case 2:
            render_capture_effect_pass_r(m);
            break;
        case 3:
            render_vendor_buy_label_pass(m, (int)x, (int)y);
            break;
        case 4:
            VendorDrawAmmoPrompt_r(m);
            break;
        case 5:
            render_vendor_item_icon_strip_pass_r(m);
            break;
        }
        render_vendor_capture_texture_overlays_pass(i, x, y);
        setup_view_context();
        draw_centered_capture_background(0x200, 0x200);
        VU1_addGSregister_l(0x42, 0x8000000064L);
        VU1_addGSregister_l(8, 5);
        VU1_texFlush();
        tex = capture_texture_tex0;
        if (pal_mode != 0) {
            append_subpixel_textured_screen_quad(w, h, x, y, 0, 0, (int)(sx - 1.0f),
                          truncate_float_to_s32_r(sy * 0.92857146f - 1.0f),
                          0x80808080, tex);
        } else {
            append_subpixel_textured_screen_quad(w, h, x, y, 0, 0, (int)(sx - 1.0f), (int)(sy - 1.0f),
                          0x80808080, tex);
        }
        VU1_addGSregister_l(8, 0);
    }
}

LINKER_REMNANT("asm/remnants/text", func_0023B008);

typedef struct {
    int x;
    int y;
    int z;
    unsigned short w;
    unsigned short h;
} ZoneBox;
extern ZoneBox *occl_cell_table MACRO_ADDR;
extern char *surface_height_tiles MACRO_ADDR;
extern int occl_cell_count MACRO_ADDR;
extern float surface_height_grid[];
extern int truncate_float_to_s32_i(float) __asm__("func_001FA898");

/* Index of the zone whose box (in 1/1024 units, 0x800 deep) holds the
   point and whose 4x4 cell mask at +0x1E has the point's cell set, or
   -1. Cells are sized and offset by the grid in D_001E69E0. */
int FindZoneAtPoint(float x, float y, float z) {
    int ix = truncate_float_to_s32_i(x * 1024.0f);
    int iy = truncate_float_to_s32_i(y * 1024.0f);
    int iz = truncate_float_to_s32_i(z * 1024.0f);
    ZoneBox *b = occl_cell_table;
    int i;

    for (i = 0; i < occl_cell_count; i++, b++) {
        char *zn;
        int cx;
        int cy;
        int bit;

        if (ix < b->x || iy < b->y || iz < b->z) {
            continue;
        }
        if (ix >= b->x + b->w || iy >= b->y + b->h || iz >= b->z + 0x800) {
            continue;
        }
        zn = surface_height_tiles + i * 0x1190;
        cx = truncate_float_to_s32_i((x - (*(float *)(zn + 0) + surface_height_grid[2])) / surface_height_grid[4]);
        cy = truncate_float_to_s32_i((y - (*(float *)(zn + 4) + surface_height_grid[3])) / surface_height_grid[5]);
        bit = 1 << (((cx >> 2) & 3) | (cy & 0xC));
        if (*(unsigned short *)(zn + 0x1E) & bit) {
            return i;
        }
    }
    return -1;
}

/* 12 bytes of post-endlabel nop padding in retail -- see func_001F6668. */
__asm__(".section .text\n\tnop\n\tnop\n\tnop\n");

LINKER_REMNANT("asm/remnants/text", func_0023B1E8);

typedef struct {
    f32 samples[16][16];
    f32 pad_400[0x20];
    f32 bottom_edge[16];
    f32 right_edge[16];
    f32 pad_500[0x23];
    f32 corner;
    f32 pad_590[0xC];
} SurfaceHeightLayer;
typedef struct {
    f32 x;
    f32 y;
    f32 z;
    u8 pad_C[0x44];
    SurfaceHeightLayer layers[3];
} SurfaceHeightTile;
typedef struct {
    u8 pad_0[8];
    f32 origin_x;
    f32 origin_y;
    f32 cell_width;
    f32 cell_height;
} SurfaceHeightGrid;
extern SurfaceHeightTile * surface_height_tiles_r __asm__("D_00161290") MACRO_ADDR;
extern SurfaceHeightGrid surface_height_grid_r __asm__("D_001E69E0");
extern short surface_height_layer;
extern s32 FindZoneAtPoint(f32, f32, f32);
extern s32 truncate_float_to_s32_r2(f32) __asm__("func_001FA898");
extern void FastVecCross(f32 *, f32 *, f32 *);
extern void fast_vec_normalize_r(f32 *, f32 *, f32) __asm__("func_001F9DC0");

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/textbin/fun_00239f58.c, sample_surface_height_map. */
s32 sample_surface_height_map(f32 *height, f32 *normal, f32 x, f32 y, f32 z) {
    f32 x_tangent[4] __attribute__((aligned(16)));
    f32 y_tangent[4] __attribute__((aligned(16)));
    SurfaceHeightTile *tile;
    s32 tile_index;
    s32 column;
    s32 row;
    f32 fraction_x;
    f32 fraction_y;
    f32 h00;
    f32 h10;
    f32 h01;
    f32 h11;
    f32 first_row_height;
    f32 weight_x;
    f32 weight_y;

    tile_index = FindZoneAtPoint(x, y, z);
    if (tile_index < 0) {
        return 0;
    }
    tile = &surface_height_tiles_r[tile_index];
    fraction_x = tile->x;
    fraction_y = tile->y;
    fraction_x += surface_height_grid_r.origin_x;
    fraction_y += surface_height_grid_r.origin_y;
    fraction_x = x - fraction_x;
    fraction_y = y - fraction_y;
    column = truncate_float_to_s32_r2(fraction_x / surface_height_grid_r.cell_width);
    row = truncate_float_to_s32_r2(*&fraction_y / surface_height_grid_r.cell_height);
    fraction_x -= IntToFloat(column) * surface_height_grid_r.cell_width;
    weight_x = fraction_x / surface_height_grid_r.cell_width;
    fraction_y -= IntToFloat(row) * surface_height_grid_r.cell_height;
    /* Divide the signed remainder after rounding the row, before choosing the sample bank. */
    weight_y = fraction_y / surface_height_grid_r.cell_height;
    h00 = tile->layers[(*(s32 *)&surface_height_layer)].samples[row][column];
    if (column == 15) {
        h10 = tile->layers[(*(s32 *)&surface_height_layer)].right_edge[row];
    } else {
        h10 = tile->layers[(*(s32 *)&surface_height_layer)].samples[row][column + 1];
    }
    if (row == 15) {
        h01 = tile->layers[(*(s32 *)&surface_height_layer)].bottom_edge[column];
    } else {
        h01 = tile->layers[(*(s32 *)&surface_height_layer)].samples[row + 1][column];
    }
    if (column == 15) {
        if (row == 15) {
            h11 = tile->layers[(*(s32 *)&surface_height_layer)].corner;
        } else {
            h11 = tile->layers[(*(s32 *)&surface_height_layer)].right_edge[row + 1];
        }
    } else if (row == 15) {
        h11 = tile->layers[(*(s32 *)&surface_height_layer)].bottom_edge[column + 1];
    } else {
        h11 = tile->layers[(*(s32 *)&surface_height_layer)].samples[row + 1][column + 1];
    }
    if (height != 0) {
        first_row_height = h00 + (h10 - h00) * weight_x;
        *height = first_row_height +
                  ((h01 + (h11 - h01) * weight_x) - first_row_height) * weight_y + tile->z;
    }
    if (normal != 0) {
        x_tangent[0] = surface_height_grid_r.cell_width;
        x_tangent[1] = 0.0f;
        x_tangent[2] = h10 - h00;
        x_tangent[3] = 1.0f;
        y_tangent[0] = 0.0f;
        y_tangent[1] = surface_height_grid_r.cell_height;
        y_tangent[2] = h01 - h00;
        y_tangent[3] = 1.0f;
        FastVecCross(normal, x_tangent, y_tangent);
        fast_vec_normalize_r(normal, normal, 1.0f);
    }
    return 1;
}
__asm__(".section .text\n\tnop\n\tnop\n\tnop\n");

LINKER_REMNANT("asm/remnants/text", func_0023B510);

/* If flag 0x2 at +0x70 is set, sets +0x54 to 1.0 (0.0 once +0x58 is
   positive) and clears +0x58; then calls func_0023B5D0. The 0x68 bytes of
   linker remnant in front of it were split off. */
extern void update_instance_attachment_points_and_lengths(char *arg0);

void settle_attachment_weight_and_update(char *arg0) {
    if (*(unsigned char *)(arg0 + 0x70) & 0x2) {
        float f0 = *(float *)(arg0 + 0x58);
        float f1 = 0.0f;
        float f2 = 1.0f;
        if (f1 < f0) {
            f2 = f1;
        }
        *(float *)(arg0 + 0x54) = f2;
        *(float *)(arg0 + 0x58) = f1;
    }
    update_instance_attachment_points_and_lengths(arg0);
}

typedef struct {
    char b[0x10];
} Cfg16;

extern Cfg16 attachment_vertex_indices NOT_SDA;
extern float FastVecLength(void *);
extern void FastVecSub_b(void *, void *, void *) __asm__("func_001F9BF0");

/* Run the four-vertex transform over the block hanging off arg0+0x78
   with a fresh copy of the D_00160FD0 parameters (a plain struct
   assignment: at alignment 1 this compiler expands the 16 bytes as
   unaligned ldl/ldr + sdl/sdr pairs, which is retail's shape -- same
   idiom as func_001FFE88), then record the two resulting lengths at
   +0x40 and +0x44. */
void update_instance_attachment_points_and_lengths(char *arg0) {
    float a[4];
    float b[4];
    Cfg16 cfg;
    char *p = ((char *)((Moby *)arg0)->pvars);

    cfg = attachment_vertex_indices;
    transform_scaled_vertex_batch(arg0, 4, &cfg, p);
    FastVecSub_b(a, p + 0x10, p);
    FastVecSub_b(b, p + 0x20, p);
    *(float *)(p + 0x40) = FastVecLength(a);
    *(float *)(p + 0x44) = FastVecLength(b);
}
