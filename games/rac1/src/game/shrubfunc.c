#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * shrubfunc.cpp in the original source; text 0x229C08-0x229F00.
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

extern int g_render_packet_cursor MACRO_ADDR;
extern int shrub_tex_dma_tag MACRO_ADDR;
extern int max_shrub_texture_dma_size MACRO_ADDR;
extern int g_gs_texture_allocation_cursor MACRO_ADDR;
extern char str_shrub_texture_overflow[];
extern int BuildShrubTextureDma(int);
extern void VU1_texFlush(void);

/* DmaShrubTextures: DmaMobyTextures' splice around the shrub texture
   upload, which also warns past 0x400000 bytes and keeps the largest
   size seen in D_001604F8. */
void DmaShrubTextures(void) {
    int *p = (int *)g_render_packet_cursor;
    int size;

    g_render_packet_cursor += 0x10;
    ((int *)shrub_tex_dma_tag)[0] = 0x20000000;
    ((int *)shrub_tex_dma_tag)[1] = g_render_packet_cursor;
    ((int *)shrub_tex_dma_tag)[2] = 0;
    ((int *)shrub_tex_dma_tag)[3] = 0;
    if (draw_config[8] != 0 && draw_config[7] != 0) {
        size = BuildShrubTextureDma(g_gs_texture_allocation_cursor);
        VU1_texFlush();
        if (size > 0x400000) {
            STUB_printf(str_shrub_texture_overflow);
        }
        if (max_shrub_texture_dma_size < size) {
            max_shrub_texture_dma_size = size;
        }
    }
    ((int *)g_render_packet_cursor)[0] = 0x20000000;
    ((int *)g_render_packet_cursor)[1] = shrub_tex_dma_tag + 0x10;
    ((int *)g_render_packet_cursor)[2] = 0;
    ((int *)g_render_packet_cursor)[3] = 0;
    g_render_packet_cursor += 0x10;
    p[0] = 0x20000000;
    p[1] = g_render_packet_cursor;
    p[2] = 0;
    p[3] = 0;
}

extern int shrub_class_index_list[];
extern char *shrub_render_classes[];
extern short shrub_paging_table[];
typedef struct { char *ptr; int pad; } ShrubRec_29D48;

/* `items` declared inside the middle loop, and `rec = &items[i];
   rec->ptr` rather than `items[i].ptr`, give retail's base-first addu. */
void patch_shrub_gifs(void) {
    int *p;
    for (p = shrub_class_index_list; *p >= 0; p++) {
        char *obj = shrub_render_classes[*p];
        int i;
        for (i = 0; i < *(short *)(obj + 0x28); i++) {
            ShrubRec_29D48 *items = (ShrubRec_29D48 *)(obj + 0x40);
            ShrubRec_29D48 *rec = &items[i]; int *hdr = (int *)(rec->ptr + 0x10);
            char *e = (char *)hdr + hdr[1] * 0x10 + 0x10;
            int j;
            for (j = 0; j < hdr[0]; j++) {
                short *ent = &shrub_paging_table[*(unsigned char *)(e + 0x13) * 2];
                if (ent[0] != 0) *(int *)(e + 0x30) = (*(int *)(e + 0x30) & 0xFFFFC000) | ent[0];
                if (ent[1] != 0) *(int *)(e + 0x20) = (*(int *)(e + 0x20) & 0xFFFFC000) | ent[1];
                e += 0x40;
            }
        }
    }
}

extern int g_gs_texture_allocation_start MACRO_ADDR;
extern int shrub_draw_enabled;
extern char str_shrub_texs[];
extern char str_shrub_draw[];
extern char shrub_scratchpad_copy[];
extern void ProfileMarker_Begin(void *, int); /* empty profiling marker */
extern void ProfileMarker_End(void *, int); /* empty profiling marker */
extern void ShrubProc(void);
extern void DmaShrubTextures(void);

/* DrawShrubs. The packet pointer goes through a local advanced in place;
   `D_00161000 += 0x10` is 4 bytes short (its store lands in the jal
   delay slot as a $gp store). */
void DrawShrubs(void) {
    int p = g_render_packet_cursor;

    shrub_tex_dma_tag = p;
    g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
    p += 0x10;
    g_render_packet_cursor = p;
    ProfileMarker_Begin(str_shrub_texs, 1);
    if (shrub_draw_enabled != 0) {
        flush_cache(0);
        ShrubProc();
        write_dma_channel(shrub_scratchpad_copy, 0x3200, 0x40);
    }
    ProfileMarker_Begin(str_shrub_draw, 7);
    DmaShrubTextures();
    ProfileMarker_End(str_shrub_draw, 7);
}
