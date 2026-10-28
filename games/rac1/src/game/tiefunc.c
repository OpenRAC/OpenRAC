#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * tiefunc.cpp in the original source; text 0x236958-0x236F00.
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

extern int g_render_packet_cursor MACRO_ADDR;
extern int tie_tex_dma_tag_slot MACRO_ADDR;
extern int max_tie_texture_dma_size MACRO_ADDR;
extern int g_gs_texture_allocation_cursor MACRO_ADDR;
extern char str_tie_texture_overflow[];
extern int BuildTieTextureDma(int);
extern void VU1_texFlush(void);

/* DmaTieTextures: the same splice for ties, largest size in D_00161074. */
void DmaTieTextures(void) {
    int *p = (int *)g_render_packet_cursor;
    int size;

    g_render_packet_cursor += 0x10;
    ((int *)tie_tex_dma_tag_slot)[0] = 0x20000000;
    ((int *)tie_tex_dma_tag_slot)[1] = g_render_packet_cursor;
    ((int *)tie_tex_dma_tag_slot)[2] = 0;
    ((int *)tie_tex_dma_tag_slot)[3] = 0;
    if (draw_config[6] != 0 && draw_config[5] != 0) {
        size = BuildTieTextureDma(g_gs_texture_allocation_cursor);
        VU1_texFlush();
        if (size > 0x400000) {
            STUB_printf(str_tie_texture_overflow);
        }
        if (max_tie_texture_dma_size < size) {
            max_tie_texture_dma_size = size;
        }
    }
    ((int *)g_render_packet_cursor)[0] = 0x20000000;
    ((int *)g_render_packet_cursor)[1] = tie_tex_dma_tag_slot + 0x10;
    ((int *)g_render_packet_cursor)[2] = 0;
    ((int *)g_render_packet_cursor)[3] = 0;
    g_render_packet_cursor += 0x10;
    p[0] = 0x20000000;
    p[1] = g_render_packet_cursor;
    p[2] = 0;
    p[3] = 0;
}

extern char *object_render_classes[];
extern char tie_gif_patch_list[];
extern char tie_tex_remap_table[];

/* PatchTieGifs: patch_shrub_gifs' shape (func_00229D48). For each tie index
   in the -1-terminated list at D_001E3300, walk the tie's 0x50-byte
   records and patch the nonzero halves of the remap entry picked by byte
   0x33 into the low 14 bits of the words at +0x00 and +0x20. */
void PatchTieGifs(void) {
    int *p;
    for (p = (int *)tie_gif_patch_list; *p >= 0; p++) {
        char *tie = object_render_classes[*p];
        char *r = *(char **)(tie + 0x2C);
        int i;
        for (i = 0; i < *(unsigned char *)(tie + 0x23); i++) {
            short *ent = (short *)(tie_tex_remap_table + *(unsigned char *)(r + 0x33) * 4);
            if (ent[0] != 0) *(int *)r = (*(int *)r & 0xFFFFC000) | ent[0];
            if (ent[1] != 0) *(int *)(r + 0x20) = (*(int *)(r + 0x20) & 0xFFFFC000) | ent[1];
            r += 0x50;
        }
    }
}

extern void PatchTieGifs(void);
extern char tie_gif_patch_list_saved[];
extern char tie_tex_remap_table_saved[];

/* Patch the tie GIF tags, save the patch list and texture remap table, then patch again. */
void copy_render_buffer_pair(void) {
    PatchTieGifs();
    FastMemCopy(tie_gif_patch_list, tie_gif_patch_list_saved, 0x200);
    FastMemCopy(tie_tex_remap_table, tie_tex_remap_table_saved, 0x400);
    PatchTieGifs();
}

extern void LightTies(void *);
extern char tie_light_data[];
extern char tie_light_data2[];

/* Light the ties from both tie light data blocks. */
void register_entity_render_resources(void) {
    LightTies(tie_light_data);
    LightTies(tie_light_data2);
}

extern int g_gs_texture_allocation_start MACRO_ADDR;
extern int tie_proc_enabled;
extern char str_tie_texs[];
extern char str_tie_draw[];
extern char tie_packet_tail_tags[];
extern void ProfileMarker_Begin(void *, int); /* empty profiling marker */
extern void ProfileMarker_End(void *, int); /* empty profiling marker */
extern void TieProc(void);
extern void DmaTieTextures(void); /* DmaTieTextures */

/* DrawTies_1: DrawShrubs' shape, the packet pointer advanced through a
   local. */
void DrawTies_1(void) {
    int p = g_render_packet_cursor;

    tie_tex_dma_tag_slot = p;
    g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
    p += 0x10;
    g_render_packet_cursor = p;
    ProfileMarker_Begin(str_tie_texs, 1);
    if (tie_proc_enabled != 0) {
        flush_cache(0);
        TieProc();
        write_dma_channel(tie_light_data, 0x3600, 0x40);
    }
    ProfileMarker_Begin(str_tie_draw, 5);
    DmaTieTextures();
    FastMemCopy((void *)g_render_packet_cursor, tie_packet_tail_tags, 0x20);
    ProfileMarker_End(str_tie_draw, 5);
}

extern int registered_object_render_class_count MACRO_ADDR;

/* D_0018A3B0 read as a struct: a member access is MEM_IN_STRUCT_P, so
   sched2 knows it cannot alias the fixed-address scalar stores before it
   (a plain `int *` index is not, and the load then waits for them). */
typedef struct {
    int unk00[6];
    int unk18;
} DrawCfg_236CA8;

/* DrawTies_2: DrawTies_1's two passes, the first with the odd ties' 0x8
   flag set (and the saved tie data swapped in around it), the second with
   the even ones'. Each flag loop reads the tie count into its own local
   (the loop's stores could alias it) and the flag is a `short` (so the
   `&= ~8` stays an int AND with -9, not andi 0xFFF7). */
void DrawTies_2(void) {
    {
        int i;
        int n = registered_object_render_class_count;
        for (i = 1; i < n; i += 2) {
            *(short *)(object_render_classes[i] + 0x24) |= 8;
        }
    }
    {
        int p = g_render_packet_cursor;
        tie_tex_dma_tag_slot = p;
        g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
        p += 0x10;
        g_render_packet_cursor = p;
    }
    ProfileMarker_Begin(str_tie_texs, 1);
    {
        DrawCfg_236CA8 *d = (DrawCfg_236CA8 *)draw_config;
        if (d->unk18 != 0) {
            flush_cache(0);
            TieProc();
            write_dma_channel(tie_light_data2, 0x3600, 0x40);
        }
    }
    DmaTieTextures();
    FastMemCopy(tie_gif_patch_list_saved, tie_gif_patch_list, 0x200);
    FastMemCopy(tie_tex_remap_table_saved, tie_tex_remap_table, 0x400);
    {
        int i;
        int n = registered_object_render_class_count;
        for (i = 1; i < n; i += 2) {
            *(short *)(object_render_classes[i] + 0x24) &= ~8;
        }
    }
    {
        int i;
        int n = registered_object_render_class_count;
        for (i = 0; i < n; i += 2) {
            *(short *)(object_render_classes[i] + 0x24) |= 8;
        }
    }
    {
        DrawCfg_236CA8 *d = (DrawCfg_236CA8 *)draw_config;
        int p = g_render_packet_cursor;
        tie_tex_dma_tag_slot = p;
        g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
        p += 0x10;
        g_render_packet_cursor = p;
        if (d->unk18 != 0) {
            flush_cache(0);
            TieProc();
            write_dma_channel(tie_light_data, 0x3600, 0x40);
        }
    }
    ProfileMarker_Begin(str_tie_draw, 5);
    DmaTieTextures();
    {
        int i;
        int n = registered_object_render_class_count;
        for (i = 0; i < n; i += 2) {
            *(short *)(object_render_classes[i] + 0x24) &= ~8;
        }
    }
    FastMemCopy((void *)g_render_packet_cursor, tie_packet_tail_tags, 0x20);
    ProfileMarker_End(str_tie_draw, 5);
}
