#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * tfragfunc.cpp in the original source; text 0x234380-0x2347F0.
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
extern int stash_get_reserved_qwords(unsigned int arg0);

/*
 * Retail aligns func_00234380 to 16 bytes, and func_00234350's .s file
 * carried two trailing nops after its endlabel to supply that padding.
 * Those nops sit OUTSIDE the declared function size (0x28), so they were
 * silently lost when this function became C, shifting all 179 later
 * text functions by -8 and giving them spurious 1-byte jal-target diffs.
 * The .s files only ever .align 3 (8 bytes), so the 16-byte alignment
 * has to be restored explicitly here.
 */
__asm__(".align 4");

extern float tfrag_render_ranges[3];
extern int tfrag_lod_dists_raw[3];
extern float fog_params;
extern float tfrag_lod_blend_matrix[4][4];
extern int truncate_float_to_s32_i(float) __asm__("func_001FA898");

/* SetTfragDists(void): the three LOD distances as fixed point (x1024), and
   the matrix mapping distance to the two blend weights. The store order is
   load-bearing: filling the rows first and the two translation terms last
   ([1][3] = b, then [0][3] = a) moves sched1's last use of b, and with it
   the local-alloc priority of b's quantity below ab's -- retail's b in $f21,
   1/(a-b) in $f20, and -0.5 in $f3. */
void SetTfragDists(void) {
    float a, b, c, ab, bc;

    tfrag_lod_dists_raw[0] = truncate_float_to_s32_i(tfrag_render_ranges[0] * 1024.0f);
    tfrag_lod_dists_raw[1] = truncate_float_to_s32_i(tfrag_render_ranges[1] * 1024.0f);
    tfrag_lod_dists_raw[2] = truncate_float_to_s32_i(tfrag_render_ranges[2] * 1024.0f);
    a = tfrag_render_ranges[0] * fog_params;
    b = tfrag_render_ranges[1] * fog_params;
    c = tfrag_render_ranges[2] * fog_params;
    ab = 1.0f / (a - b);
    bc = 1.0f / (b - c);
    FastMemZero16(tfrag_lod_blend_matrix, 0x40);
    tfrag_lod_blend_matrix[0][0] = ab * 0.5f;
    tfrag_lod_blend_matrix[0][1] = -ab;
    tfrag_lod_blend_matrix[1][0] = bc * 0.5f;
    tfrag_lod_blend_matrix[1][1] = -bc;
    tfrag_lod_blend_matrix[2][0] = b * ab * -0.5f;
    tfrag_lod_blend_matrix[2][1] = a * ab;
    tfrag_lod_blend_matrix[3][0] = c * bc * -0.5f;
    tfrag_lod_blend_matrix[3][1] = b * bc;
    tfrag_lod_blend_matrix[1][3] = b;
    tfrag_lod_blend_matrix[0][3] = a;
}

extern int g_render_packet_cursor MACRO_ADDR;
extern int tfrag_tex_dma_tag_slot MACRO_ADDR;
extern int max_tfrag_texture_dma_size MACRO_ADDR;
extern int g_gs_texture_allocation_cursor MACRO_ADDR;
extern char str_tfrag_texture_overflow[];
extern void ComputeTfragTextureUsage(void);
extern int build_tfrag_texture_dma(int);
extern void VU1_texFlush(void);

/* DmaTfragTextures: the tfrag twin of DmaShrubTextures (func_00229C08),
   which also warns "tfrag texture overflow" past 0x400000 bytes and keeps
   the largest size seen in D_00160FC4. */
/* A copy of its twin func_00229C08. */
void DmaTfragTextures(void) {
    int *p = (int *)g_render_packet_cursor;
    int size;

    g_render_packet_cursor += 0x10;
    ((int *)tfrag_tex_dma_tag_slot)[0] = 0x20000000;
    ((int *)tfrag_tex_dma_tag_slot)[1] = g_render_packet_cursor;
    ((int *)tfrag_tex_dma_tag_slot)[2] = 0;
    ((int *)tfrag_tex_dma_tag_slot)[3] = 0;
    if (draw_config[4] != 0 && draw_config[3] != 0) {
        ComputeTfragTextureUsage();
        size = build_tfrag_texture_dma(g_gs_texture_allocation_cursor);
        VU1_texFlush();
        if (size > 0x400000) {
            STUB_printf(str_tfrag_texture_overflow);
        }
        if (max_tfrag_texture_dma_size < size) {
            max_tfrag_texture_dma_size = size;
        }
    }
    ((int *)g_render_packet_cursor)[0] = 0x20000000;
    ((int *)g_render_packet_cursor)[1] = tfrag_tex_dma_tag_slot + 0x10;
    ((int *)g_render_packet_cursor)[2] = 0;
    ((int *)g_render_packet_cursor)[3] = 0;
    g_render_packet_cursor += 0x10;
    p[0] = 0x20000000;
    p[1] = g_render_packet_cursor;
    p[2] = 0;
    p[3] = 0;
}

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

/* Walk the null-terminated chunk list at D_001E1200; for every 0x50-byte
   record in each chunk, look up the remap entry indexed by the byte at
   +0x23 and patch its two nonzero halves into the low 14 bits of the
   words at +0x00 and +0x30. */
/* PatchTfragGifs(void) */
void PatchTfragGifs(void) {
    TexChunk *c;
    char *r;
    TexRemap *e;
    int i;
    int n;

    c = tfrag_gif_chunk_list;
    while (c->items != 0) {
        n = c->count;
        r = c->items;
        for (i = 0; i < n; i++) {
            e = &tfrag_tex_remap_table[*(unsigned char *)(r + 0x23)];
            if (e->a != 0) {
                *(int *)r = (*(int *)r & 0xFFFFC000) | e->a;
            }
            if (e->b != 0) {
                *(int *)(r + 0x30) = (*(int *)(r + 0x30) & 0xFFFFC000) | e->b;
            }
            r += 0x50;
        }
        c++;
    }
}

extern int g_gs_texture_allocation_start MACRO_ADDR;
extern char str_tfrag_texs[];
extern char str_tfrag_draw[];
extern char tfrag_dma_block[];
extern void ProfileMarker_Begin(void *, int); /* empty profiling marker */
extern void ProfileMarker_End(void *, int); /* empty profiling marker */
extern void set_identity_matrix(void *);
extern void write_vif_unpack_packet(int, void *, int);
extern void TfragProc(void);
extern void DmaTfragTextures(void);

/* DrawTfrag, in DrawShrubs' shape. */
void DrawTfrag(void) {
    float m[4][4];
    int p = g_render_packet_cursor;

    tfrag_tex_dma_tag_slot = p;
    g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
    p += 0x10;
    g_render_packet_cursor = p;
    ProfileMarker_Begin(str_tfrag_texs, 1);
    set_identity_matrix(m);
    FastVecScale(m[3], camera_position, -1024.0f);
    m[3][3] = 1.0f;
    sce_vu0_mul_matrix(m, camera_position - 0x100, m);
    write_vif_unpack_packet(5, m, 4);
    write_vif_unpack_packet(0x14D, m, 4);
    if (draw_config[4] != 0) {
        flush_cache(0);
        TfragProc();
    }
    ProfileMarker_Begin(str_tfrag_draw, 2);
    DmaTfragTextures();
    if (draw_config[4] != 0) {
        write_dma_channel(tfrag_dma_block, 0x3000, 0x40);
    }
    ProfileMarker_End(str_tfrag_draw, 2);
}

/* The last function in the object: retail pads to 16 bytes after it
   (two nops in its .s), and vuchain.o starts there. */
__asm__(".align 4");
