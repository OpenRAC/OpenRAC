#include "common.h"
#include "camera.h"
#include "callees.h"
#include "records.h"
#include "vector.h"
#include "structs.h"

/*
 * space.cpp in the original source; text 0x22F128-0x233FF8.
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
extern int current_vu_chain MACRO_ADDR;
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

extern char render_sequence_c[] __asm__("D_0018CC20");
extern float view_tan_half_fov;
extern unsigned char draw_map51_m[4] __asm__("D_0015EEB4") MACRO_ADDR;
extern void FastVecCross(void *, void *, void *);

/* Movie-camera keyframe step (as func_001EB338): the keyframe's +0x1C
   sets D_0018CEB0, its position and X/Y/Z angles set the camera position
   and matrix rows, and with the D_0015EEB4 flag set the row at +0x220 is
   rebuilt from the other two by func_001F9CA0. Returns the flag byte.
   The flag is read through a 4-byte MACRO_ADDR alias: over -G2, so the
   macro expands to lui/lbu through the destination register. */
unsigned char build_object_rotation_matrix(void) {
    char *t = render_sequence_c;
    char *key = *(char **)(t + 0x54) + *(int *)(t + 0x38) * 32;
    unsigned char flag = key[0xC];
    float *ang = (float *)(key + 0x10);
    char *pos;
    char *cam;
    float m[16];

    view_tan_half_fov = ang[3];
    UpdateViewContext();
    pos = camera_position;
    qcopy(pos, key);
    sceVu0UnitMatrix(m);
    sceVu0RotMatrixX(m, m, *(float *)(key + 0x10));
    sceVu0RotMatrixY(m, m, ang[1]);
    sceVu0RotMatrixZ(m, m, ang[2]);
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
    if (draw_map51_m[0] != 0) {
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

extern char quad_default_uvs[];
extern void draw_world_quad_packet(void *, int, int);
extern int g_game_mode MACRO_ADDR;
extern int scale_game_frame_count(int);
extern void scale_vector(void *, void *, float);
extern int bsphere_check_alpha(void *, int *, float);
extern float ProbeGroundHeight(void *, int, float);
typedef struct {
    char pad[0x20];
    int mode;
    short level;
    short set;
} FlareCfg;
extern FlareCfg level_render_state;
extern float light_flare_corners[][4][4];

/* Draws a light flare quad at arg0's position (+0x00; the matrix is at
   +0xC0) when func_001FA9E8 finds it on screen, with the alpha it
   returns. In D_0015F6E8 mode 6 with flare set 3, the flare fades as
   D_0013E130's level passes func_001F98C0(150), and its depth comes from
   func_00214358. The corners are D_001D9B60[set]. */
void draw_light_flare(char *arg0) {
    float a[4];
    Quad_FBE0 q;
    float b[4];
    int alpha;
    float z;
    int i;

    alpha = 0;
    scale_vector(a, arg0, 1.0f / 1024.0f);
    if (bsphere_check_alpha(a, &alpha, 32.0f) < 0) {
        return;
    }
    if (g_game_mode == 6 && level_render_state.mode == 3
        && level_render_state.level > scale_game_frame_count(150)) {
        alpha -= (level_render_state.level - scale_game_frame_count(150)) * 4;
        if (alpha <= 0) {
            return;
        }
    }
    q.gs[1] = GetEffectTex(0);
    q.gs[2] = 0xFF9000000260;
    q.gs[3] = 0x8000000044;
    q.gs[0] = 0;
    FastMemCopy(q.uv, quad_default_uvs, 0x20);
    FastVecScale(b, arg0, 1.0f / 1024.0f);
    z = *(float *)(arg0 + 0x18) + 0.1f;
    if (g_game_mode == 6) {
        z = ProbeGroundHeight(arg0 + 0x10, 0, 0.5f) + 0.1f;
    }
    for (i = 0; i < 4; i++) {
        float *v;

        q.rgba[i] = ((alpha >> 1) << 24) | 0x808080;
        v = q.v[i];
        matrix_mul_vec3(v, light_flare_corners[level_render_state.set][i], arg0 + 0xC0);
        FastVecAdd(v, v, arg0 + 0x10);
        v[2] = z;
    }
    draw_world_quad_packet(&q, 0, 0);
}

/* The five nops of padding after func_0022F258 (they sat after
   `endlabel` in its .s, so the stub carried them); see func_001F6668's
   note in draw.c. */
__asm__(".section .text\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n");

LINKER_REMNANT("asm/remnants/text", func_0022F498);

extern int space_load_level MACRO_ADDR;
extern int space_load_in_progress_m __asm__("D_0015F6FC") MACRO_ADDR;
extern short space_load_pending; /* SDA, gp -0x7670 */

/* Requests a switch to level arg0 and flags the load as pending. */
void initialize_global_state_entry(int arg0) {
    space_load_level = arg0;
    *(int *)&space_load_pending = 1;
    space_load_in_progress_m = 1;
}

LINKER_REMNANT("asm/remnants/text", func_0022F4C0);

typedef struct { float f[4]; } __attribute__((aligned(16))) QuadOffset_2F4C8;

typedef struct {
    QuadOffset_2F4C8 positions[4];   /* 0x00 */
    u32 colors[4];                   /* 0x40 */
    f32 texture_coordinates[4][2];   /* 0x50 */
    u64 reserved;                    /* 0x70 */
    u64 texture;                     /* 0x78 */
    u64 texture_state;               /* 0x80 */
    u64 primitive;                   /* 0x88 */
} GeometryQuad_2F4C8;

typedef struct {
    char *player;                    /* 0x00 */
    u8 pad4[0x22];
    s16 content_variant;             /* 0x26 */
} LevelRenderState_2F4C8;

extern LevelRenderState_2F4C8 level_render_state_2F4C8 __asm__("D_0013E130");
extern f32 quad_uvs_2F4C8[4][2] __asm__("D_001D9C20") NOT_SDA;
extern QuadOffset_2F4C8 quad_positions_2F4C8[8] __asm__("D_001D9C40") NOT_SDA;
extern QuadOffset_2F4C8 quad_positions_variant1_2F4C8[2] __asm__("D_001D9CC0") NOT_SDA;
extern QuadOffset_2F4C8 quad_positions_variant2_2F4C8[2] __asm__("D_001D9CE0") NOT_SDA;
extern QuadOffset_2F4C8 quad_corners_2F4C8[10] __asm__("D_001D9D00") NOT_SDA;
extern short resident_effect_quad_counts_r __asm__("D_001605C0");

void func_001F9C30_2F4C8(float, void *, void *) __asm__("func_001F9C30");
void func_001F9BD8_2F4C8(void *, void *, void *) __asm__("func_001F9BD8");
void func_001F9EC0_2F4C8(void *, void *, void *) __asm__("func_001F9EC0");
long func_001F4868_2F4C8(int) __asm__("func_001F4868");
int func_002140B0_2F4C8(int) __asm__("func_002140B0");
float func_001FA888_2F4C8(int) __asm__("func_001FA888");
void func_001F7EF8_2F4C8(void *, int, int) __asm__("func_001F7EF8");

/* Resident effect quads: picks the row table by level_render_state's
   content_variant, builds a textured GS quad, and for each row of
   D_001605C0[variant] scales the four corners by the object's brightness
   and the row size, offsets them by the row, the player's matrix and the
   object position, and draws the quad.
   Adapted from Lombyte's project (GPL-3.0): src/gameplay/fun_0022e1b0.c, FUN_0022e1b0. */
void DrawResidentEffectQuads(unsigned char *m) {
    GeometryQuad_2F4C8 quad;
    QuadOffset_2F4C8 v;
    QuadOffset_2F4C8 *tbl;
    int i, j, k;

    tbl = quad_positions_2F4C8;
    if (level_render_state_2F4C8.content_variant == 1) tbl = quad_positions_variant1_2F4C8;
    else if (level_render_state_2F4C8.content_variant == 2) tbl = quad_positions_variant2_2F4C8;
    quad.texture = func_001F4868_2F4C8(5);
    quad.texture_state = 0xFF9000000260L;
    quad.primitive = 0x8000000048L;
    quad.reserved = 0;
    for (j = 0; j < 4; j++) {
        quad.texture_coordinates[j][0] = quad_uvs_2F4C8[j][0];
        quad.texture_coordinates[j][1] = quad_uvs_2F4C8[j][1];
    }
    func_001F9C30_2F4C8(0.0009765625f, &v, m);
    for (i = 0; i < ((int *)&resident_effect_quad_counts_r)[level_render_state_2F4C8.content_variant]; i++) {
        int c = m[0xBC];
        unsigned int col;
        float s;
        if (*(short *)(m + 0xB2)) c += func_002140B0_2F4C8(*(short *)(m + 0xB2));
        s = func_001FA888_2F4C8(c) * (tbl[i].f[3] / 40.0f);
        col = (c << 24) | 0x2058B0;
        if (*(short *)(m + 0xA6) == 0x215) col = (c << 24) | 0x308000;
        for (k = 0; k < 4; k++) {
            quad.colors[k] = col;
            func_001F9C30_2F4C8(s, &quad.positions[k], &quad_corners_2F4C8[k]);
            func_001F9BD8_2F4C8(&quad.positions[k], &quad.positions[k], &tbl[i]);
            func_001F9EC0_2F4C8(&quad.positions[k], &quad.positions[k], level_render_state_2F4C8.player + 0xC0);
            func_001F9BD8_2F4C8(&quad.positions[k], &quad.positions[k], &v);
        }
        func_001F7EF8_2F4C8(&quad, 0, 0);
    }
}

/* Engine trail ribbons, reconstructed from retail instructions. */
typedef struct {
    char pad[0x20];
    int mode;
    short level, ship;
    char pad28[0x28];
    int newest, count;
    char pad58[0x68];
    Vec4 ring[2][32];
} TrailState_2F738;
extern TrailState_2F738 trail_state_2F738 __asm__("D_0013E130");

extern float trail_uv_2F738[4][2] __asm__("D_001D9DA0");
extern unsigned int trail_color_a_2F738[][2] __asm__("D_001D9DC0");
extern unsigned int trail_color_b_2F738[][2] __asm__("D_001D9DC4");
extern short trail_width_2F738 __asm__("D_001605D0");
extern unsigned int trail_lerp_2F738(unsigned int, unsigned int, float) __asm__("func_001FA8A8");
extern float trail_float_2F738(int) __asm__("func_001FA888");
extern void trail_scale_2F738(void *, void *, float) __asm__("func_001F9DC0");

/* Returns trail point n of ring t. */
static __inline__ Vec4 *trail_at_2F738(int t, int n) {
    return &trail_state_2F738.ring[t][n];
}

/* Builds and draws the two engine trail ribbons from the ring of recent trail points. */
void build_resident_indexed_texture_warp_meshes(int unused) {
    GeometryQuad_2F4C8 q;
    struct {
        Vec4 across[4];
        Vec4 side[4];
    } workspace;
    int vertex, uv;
    int ring_index;
    Vec4 *secondary_current;
    Vec4 *primary_current;
    Vec4 *secondary_next;
    Vec4 *primary_next;
    Vec4 *primary_after_next;
    QuadOffset_2F4C8 *position_first;
    QuadOffset_2F4C8 *position_second;
    Vec4 *normal_second;

    if (g_game_mode == 6 && trail_state_2F738.mode == 4) {
        q.texture = func_001F4868_2F4C8(0);
    } else {
        q.texture = func_001F4868_2F4C8(0x13);
    }
    q.texture_state = 0xFF9000000260UL;
    q.primitive = 0x8000000048UL;
    q.reserved = 0;
    {
        float *src;
        float *dst1;
        float *dst;
        src = &trail_uv_2F738[0][0];
        dst1 = &q.texture_coordinates[0][1];
        dst = &q.texture_coordinates[0][0];
        uv = 3;
        do {
            dst[0] = src[0];
            dst1[0] = src[1];
            src += 2;
            dst += 2;
            dst1 += 2;
        } while (--uv >= 0);
    }
    {
        int segment;
        for (segment = 0; segment < trail_state_2F738.count - 1; segment++) {
            ring_index = (trail_state_2F738.newest - segment + 31) & 31;
            secondary_current = &trail_state_2F738.ring[1][ring_index];
            primary_current = &trail_state_2F738.ring[0][ring_index];
            secondary_next = &trail_state_2F738.ring[1][(ring_index + 1) & 31];
            {
                int engine;
                primary_next = &trail_state_2F738.ring[0][(ring_index + 1) & 31];
                primary_after_next = &trail_state_2F738.ring[0][(ring_index + 2) & 31];
                for (engine = 0; engine < 2; engine++) {
                    FastVecSub(&workspace.across[0], secondary_current, primary_current);
                    FastVecSub(&workspace.across[1], primary_current, secondary_current);
                    FastVecSub(&workspace.across[2], secondary_next, primary_next);
                    FastVecSub(&workspace.across[3], primary_next, secondary_next);
                    FastVecSub(&workspace.side[1],
                        (Vec4 *)((engine << 9) + (u32)primary_next),
                        (Vec4 *)((engine << 9) + (u32)primary_current));
                    FastVecSub(&workspace.side[3],
                        (Vec4 *)((engine << 9) + (u32)primary_after_next),
                        (Vec4 *)((engine << 9) + (u32)primary_next));
                    if (segment == 0) qcopy(&workspace.side[3], &workspace.side[1]);
                    FastVecCross(&workspace.side[0], &workspace.across[0], &workspace.side[1]);
                    FastVecCross(&workspace.side[1], &workspace.across[1], &workspace.side[1]);
                    FastVecCross(&workspace.side[2], &workspace.across[2], &workspace.side[3]);
                    FastVecCross(&workspace.side[3], &workspace.across[3], &workspace.side[3]);
                    {
                        position_first = &q.positions[0];
                        vertex = 0;
                        for (; vertex < 4; vertex++) {
                            int step;
                            int age;
                            float t;
                            step = vertex >> 1;
                            age = step;
                            age--;
                            t = trail_float_2F738(segment - age) * 0.03125f;
                            q.colors[vertex] = trail_lerp_2F738(
                                trail_color_a_2F738[trail_state_2F738.ship][0],
                                trail_color_b_2F738[trail_state_2F738.ship][0], t);
                            t = 1.0f - t * t;
                            trail_scale_2F738(position_first, &workspace.side[vertex],
                                t * ((float *)&trail_width_2F738)[trail_state_2F738.ship]);
                            func_001F9BD8_2F4C8(position_first, position_first,
                                trail_at_2F738(engine, (ring_index + step) & 31));
                            position_first++;
                        }
                    }
                    {
                        func_001F7EF8_2F4C8(&q, 0, 0);
                        normal_second = &workspace.across[0];
                        vertex = 0;
                        for (position_second = &q.positions[0]; vertex < 4; vertex++) {
                            int step;
                            int age;
                            float t;
                            step = vertex >> 1;
                            age = step;
                            age--;
                            t = trail_float_2F738(segment - age) * 0.03125f;
                            trail_scale_2F738(position_second, normal_second,
                                (1.0f - t * t) * ((float *)&trail_width_2F738)[trail_state_2F738.ship]);
                            func_001F9BD8_2F4C8(position_second, position_second,
                                trail_at_2F738(engine, (ring_index + step) & 31));
                            normal_second++;
                            position_second++;
                        }
                    }
                    func_001F7EF8_2F4C8(&q, 0, 0);
                }
            }
        }
    }
}

extern void VU1_addGSregister_l(int, long) __asm__("func_00234C98");
extern long resident_quad_texture MACRO_ADDR;
extern float resident_quad_level_scales[];
extern float resident_quad_corners[][4];
extern short resident_quad_pos;
extern int g_current_level_index_m __asm__("D_0015EE84") MACRO_ADDR;

/* Its 0x8000000044 is built as ps2eeas built it (tools/ps2eeas_dli.py). */
void draw_resident_textured_quad(void) {
    Quad_FBE0 q;
    float s;
    int i;

    VU1_addGSregister_l(0x47, 0x31801);
    s = 1.0f;
    q.gs[0] = 0;
    q.gs[1] = resident_quad_texture;
    q.gs[2] = 0xFF9000000260;
    q.gs[3] = 0x8000000044;
    FastMemCopy(q.uv, quad_default_uvs, 0x20);
    if ((unsigned)g_current_level_index_m < 0x13) s = resident_quad_level_scales[g_current_level_index_m];
    for (i = 0; i < 4; i++) {
        q.rgba[i] = 0x80808080;
        FastVecScale(q.v[i], resident_quad_corners[i], s);
        FastVecAdd(q.v[i], q.v[i], &resident_quad_pos);
    }
    draw_world_quad_packet(&q, 0, 0);
    VU1_addGSregister_l(0x47, 0x5360B);
}

/* The 64-bit argument form of func_00234C98 is needed here (one call
   passes 0x8000000044). Retail's ori 0x8000 / dsll 24 / ori 0x44 for
   that constant is ps2eeas's expansion of the same `dli`, which
   tools/ps2eeas_dli.py reproduces; GNU as builds it differently. */
extern void VU1_addGSregister_l(int, long) __asm__("func_00234C98");
extern int frame_size;
extern long resident_banner_texture MACRO_ADDR;

/* Draws the 256x32 banner texture near the bottom of the screen with alpha arg0. */
void draw_resident_textured_banner(int arg0) {
    VU1_addGSregister_l(0x47, 0x31801);
    VU1_addGSregister_l(0x42, ((long)0x8000 << 24) | 0x44);
    DrawTexturedQuad(0x20, frame_size - 0x58, 0x100, 0x20, 0, 0, 0x100, 0x20,
                  (arg0 << 24) | 0x808080, resident_banner_texture);
    VU1_addGSregister_l(0x47, 0x5360B);
}

INCLUDE_ASM("asm/nonmatchings/text", func_0022FDC0);

extern s32 g_current_level_index_r __asm__("D_0015EE84") MACRO_ADDR;
struct LevelRenderState_305A0 {
    u8 pad_0[0x58];
    s32 mode;
};
struct LevelDisplayState_305A0 {
    u8 pad_0[0x4];
    s32 screen_height;
};
struct LevelProjectionState_305A0 {
    u8 pad_0[0xB0];
    f32 projection_scale;
};
/* PAL: the two fields tested sit 8 bytes further than in the US build. */
struct MemoryCardState_305A0 {
    u8 pad_0[0xDC];
    s32 state; /* 0xDC */
    s32 pad_E0;
    s32 pending_state; /* 0xE4 */
};
extern struct MemoryCardState_305A0 memory_card_state_r __asm__("D_0013D390");
extern u8 level_available_base[];
extern struct LevelRenderState_305A0 level_render_state_r2 __asm__("D_0013E130");
extern s32 game_frame_counter MACRO_ADDR;
extern f32 gFade MACRO_ADDR;
extern s32 D_0015F704_305A0 SDATA(D_0015F704);
extern s32 render_sequence_time[];
extern s32 render_sequence_slots[];
extern struct LevelProjectionState_305A0 view_context;
extern void append_dma_tag(u32);
extern void BuildViewMatrices();
extern void UpdateViewContext();
extern void ResetGsRegisters();
extern void SetupGifPaging(s32);
extern void DoGifPaging();
extern s64 GetEffectTex_r(s32) __asm__("func_001F4868");
extern void emit_rgba_draw_packet(s32, s32, s32, s32);
extern void DrawTexturedQuad(s32, s32, s32, s32, s32, s32, s32, s32, s64, s64);
extern s32 truncate_float_to_s32_r(f32) __asm__("func_001FA898");
extern void append_gif_transfer_packet();
extern void draw_rotated_sprite(f32, f32, f32, f32, s32, s32, s64, f32);
extern void prune_moby_references(void);
extern void PatchMobyGifs();
extern void DrawMobys(void);
extern void draw_sky_shells();
extern void build_resident_indexed_texture_warp_meshes(s32);
extern void draw_resident_textured_quad();
extern void draw_resident_textured_banner(s32);
extern void render_environment_mapped_object_r(s32) __asm__("func_00233AB8");
extern void VU1_syncChain(s32);
extern void VU1_addGSregister_r(s32, s64) __asm__("func_00234C98");
extern struct LevelDisplayState_305A0 g_screen_extent_r __asm__("D_0013E600");

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/textbin/render_level_frame.c, render_level_frame. */
void render_level_frame(void) {
    f32 rotation_angle;
    f32 screen_y;
    f32 quad_extent;
    s32 overlay_alpha;
    f32 saved_projection_scale;
    struct LevelDisplayState_305A0 *display_state;
    s64 texture;
    append_gif_transfer_packet();
    BuildViewMatrices();
    prune_moby_references();
    ResetGsRegisters();
    saved_projection_scale = view_context.projection_scale;
    D_0015F704_305A0 = -1;
    if (saved_projection_scale < 0.63f) {
        view_context.projection_scale = 0.63f;
    }
    UpdateViewContext();
    BuildViewMatrices();
    draw_sky_shells();
    view_context.projection_scale = saved_projection_scale;
    UpdateViewContext();
    BuildViewMatrices();
    if (level_render_state_r2.mode == 4) {
        SetupGifPaging(1);
        draw_resident_textured_quad();
        DoGifPaging();
    }
    DrawMobys();
    append_dma_tag(0x02080000);
    SetupGifPaging(1);
    if ((g_current_level_index_r != 0) && ((g_current_level_index_r != 1) || (level_available_base[0] != 0))) {
        build_resident_indexed_texture_warp_meshes(render_sequence_slots[0]);
    }
    if ((level_render_state_r2.mode == 4) && (render_sequence_time[0] >= 0x3D)) {
        overlay_alpha = (render_sequence_time[0] - 0x3C) * 2;
        if (overlay_alpha >= 0x81) {
            overlay_alpha = 0x80;
        }
        draw_resident_textured_banner(overlay_alpha);
    }
    if ((g_current_level_index_r != 0) && ((g_current_level_index_r != 1) || (level_available_base[0] != 0))) {
        render_environment_mapped_object_r(render_sequence_slots[0]);
    }
    if ((memory_card_state_r.state >= 3) || (memory_card_state_r.pending_state >= 0)) {
        VU1_addGSregister_r(0x47, 0x3004B);
        quad_extent = 272.0f;
        texture = GetEffectTex_r(2);
        display_state = &g_screen_extent_r;
        DrawTexturedQuad(0x2C, display_state->screen_height - 0x60, 0x40, 0x40, 0, 0, 0x40, 0x40,
                           0x80808080, texture);
        screen_y = (f32)((display_state->screen_height - 0x40) * 16);
        rotation_angle = ((game_frame_counter % 55) * (-6.2831855f)) / 55.0f;
        /* The frame counter contributes only the sprite angle modulo 55. */
        /* Retail passes the full texture value in a2 and the five floats in f12-f16. */
        draw_rotated_sprite(1216.0f, screen_y, quad_extent, 272.0f, 0x40, 0x40,
                            GetEffectTex_r(3), rotation_angle);
    }
    DoGifPaging();
    if (gFade > 0.0f) {
        if (gFade > 1.0f) {
            gFade = 1.0f;
        }
        emit_rgba_draw_packet(0, 0, 0, truncate_float_to_s32_r(gFade * 128.0f));
    }
    VU1_syncChain(0x10);
    PatchMobyGifs();
}

extern float flare_spark_offsets[][6][4];
extern float g_frame_time MACRO_ADDR;
extern int random_integer_below(int);
extern char *PartType02Spawn(void *, void *, void *, int, int, int, int, int, int);

/* Spawns six particles around a moby: random velocity and a position taken from the flare corner table, transformed by the moby's matrices. Adapted from Lombyte's project (GPL-3.0) for PAL: src/rendering/effects/fun_0022f5b0.c, FUN_0022f5b0. */
void emit_six_spark_effects(char *m, float z) {
    float vel[4];
    float vel2[4];
    float pos[4];
    int i;
    int a;
    int b;
    int c;

    for (i = 0; i < 6; i++) {
        vel[0] = random_float_between(-g_frame_time, g_frame_time);
        vel[1] = random_float_between(-g_frame_time, g_frame_time);
        vel[2] = z + random_float_between(g_frame_time * -0.25f, g_frame_time * 0.25f);
        vel[3] = 0.4f;
        qcopy(vel2, vel);
        vel2[3] = 0.6f;
        qcopy(pos, flare_spark_offsets[level_render_state.set][i]);
        matrix_mul_vec3(pos, pos, m + 0xC0);
        FastVecAdd(pos, pos, m + 0x10);
        a = scale_game_frame_count(4);
        b = scale_game_frame_count(4);
        c = scale_game_frame_count(4);
        PartType02Spawn(pos, vel, vel2, 0x24C0C0C0, 0x14C0C0C0, a, b, c + random_integer_below(scale_game_frame_count(4)), -1);
    }
}

/* Adapted from Lombyte's project (GPL-3.0): src/gameplay/update_resident_gameplay_state.c. */
typedef unsigned int u128 __attribute__((mode(TI)));

typedef union {
    u128 quadword;
    f32 components[4];
    u8 bytes[16];
} Vector4_230A90;

typedef struct {
    u8 pad0[6];
    u8 unk6;
    u8 pad7[5];
    u8 count; /* 0x0C */
    u8 padD[0x48 - 0xD];
    s32 frames[1]; /* 0x48 */
} RenderModel_230A90;

typedef struct ResidentRenderObject_230A90 {
    u8 pad0[0x10];
    Vector4_230A90 position; /* 0x10 */
    u8 state;         /* 0x20 */
    u8 pad21[3];
    RenderModel_230A90 *model; /* 0x24 */
    u8 pad28[0xA];
    s16 selected_index; /* 0x32 */
    u16 flags;          /* 0x34 */
    u8 pad36[2];
    s64 lifetime_stamp; /* 0x38 */
    Vector4_230A90 rotation;   /* 0x40 */
    u8 current_frame;
    u8 next_frame;
    u8 selected_a;
    u8 selected_b;
    f32 blend; /* 0x54 */
    u8 pad58[0x71 - 0x58];
    u8 cached_selector;
    u8 opacity;
    u8 unk73;
    u8 pad74[4];
    u8 *animation_positions; /* 0x78 */
    u8 pad7C[3];
    u8 unk7F;
    u8 pad80[0x10];
    s32 color; /* 0x90 */
    s32 unk94; /* 0x94 */
    u8 pad98[0xE];
    s16 class_id; /* 0xA6 */
    u8 padA8[0xA];
    s16 unkB2; /* 0xB2 */
    u8 padB4[8];
    u8 unkBC; /* 0xBC */
    u8 padBD[3];
    Vector4_230A90 transform[4]; /* 0xC0 */
} ResidentRenderObject_230A90;

typedef struct {
    Vector4_230A90 position;
    f32 rotation[4];
} CameraKeyframe_230A90;

typedef struct {
    u8 pad0[0x34];
    s32 time;           /* 0x34 */
    s32 frame;          /* 0x38 */
    s32 sequence_frame; /* 0x3C */
    s16 end;            /* 0x40 */
    u8 pad42[2];
    s16 count; /* 0x44 */
    u8 pad46[0x54 - 0x46];
    CameraKeyframe_230A90 *frames; /* 0x54 */
    u8 pad58[0x178 - 0x58];
    ResidentRenderObject_230A90 *objects[1]; /* 0x178 */
} ResidentPlaybackState_230A90;

typedef struct {
    ResidentRenderObject_230A90 *player; /* 0x00 */
    u8 pad4[4];
    ResidentRenderObject_230A90 *attachment; /* 0x08 */
    u8 padC[4];
    ResidentRenderObject_230A90 *companion_a; /* 0x10 */
    ResidentRenderObject_230A90 *companion_b; /* 0x14 */
    u8 pad18[8];
    s32 state;           /* 0x20 */
    s16 timer;           /* 0x24 */
    s16 content_variant; /* 0x26 */
    s16 skip;            /* 0x28 */
    u8 pad2A[2];
    s16 unk2C; /* 0x2C */
    u8 pad2E[2];
    s32 path;                     /* 0x30 */
    s32 source_camera_index;      /* 0x34 */
    s32 destination_camera_index; /* 0x38 */
    f32 path_progress;            /* 0x3C */
    f32 speed;                    /* 0x40 */
    f32 path_segment_length;      /* 0x44 */
    f32 blend;                    /* 0x48 */
    f32 interpolation_velocity;   /* 0x4C */
    s32 trail;                    /* 0x50 */
    s32 history_count;            /* 0x54 */
    u8 pad58[8];
    Vector4_230A90 unk60;    /* 0x60 */
    Vector4_230A90 unk70;    /* 0x70 */
    Vector4_230A90 startPos; /* 0x80 */
    Vector4_230A90 pathPos;  /* 0x90 */
    Vector4_230A90 startRot; /* 0xA0 */
    f32 unkB0;
    f32 rotY; /* 0xB4 */
    f32 rotZ; /* 0xB8 */
    f32 unkBC;
    Vector4_230A90 trailA[32]; /* 0xC0 */
    Vector4_230A90 trailB[32]; /* 0x2C0 */
} ResidentCinematicState_230A90;

typedef struct {
    s32 point_count;
    s32 pad[3];
    Vector4_230A90 p[1]; /* 0x10: x, y, z, angle */
} ScriptedPath_230A90;

typedef struct {
    u8 pad0[0x30];
    Vector4_230A90 position; /* 0x30 */
    u8 pad40[0x34];
    f32 rotY; /* 0x74 */
    f32 rotZ; /* 0x78 */
    u8 pad7C[4];
} CameraBlendNode_230A90;

typedef struct {
    u8 pad0[0x140];
    Vector4_230A90 position; /* 0x140 */
    Vector4_230A90 unk150;   /* 0x150 */
    u8 pad160[0x350 - 0x160];
    Vector4_230A90 forward; /* 0x350 */
    Vector4_230A90 right;   /* 0x360 */
    Vector4_230A90 up;      /* 0x370 */
} ResidentCameraState_230A90;

typedef struct {
    u8 pad0[0x1C];
    s32 unk1C;
    u8 pad20[0x3A];
    u16 unk5A;
} DialoguePlaybackState_230A90;

typedef struct {
    u8 pad0[0x2080];
    ResidentRenderObject_230A90 *reference_object;
} LevelObjectState_230A90;

typedef struct {
    Vector4_230A90 a;
    Vector4_230A90 b;
} PositionPair_230A90;

extern s32 resident_data_0013CBE4[] __asm__("D_0013CBE4");
extern u8 resident_data_0013D5C8[] __asm__("D_0013D5C8");
extern ResidentCinematicState_230A90 resident_data_0013E130 __asm__("D_0013E130");
extern LevelObjectState_230A90 resident_data_0013F450 __asm__("D_0013F450");
extern u8 resident_data_001414F5[] __asm__("D_001414F5");
extern DialoguePlaybackState_230A90 resident_data_001517D0 __asm__("D_001517D0");
extern f32 resident_data_0015EE60 __asm__("D_0015EE60") MACRO_ADDR;
extern f32 resident_data_0015EE6C __asm__("D_0015EE6C") MACRO_ADDR;
extern f32 resident_data_0015EE70 __asm__("D_0015EE70") MACRO_ADDR;
extern s32 resident_data_0015EE80 __asm__("D_0015EE80") MACRO_ADDR;
extern u8 resident_data_0015EEB4[4] __asm__("D_0015EEB4") MACRO_ADDR;
extern f32 resident_data_0015F53C __asm__("D_0015F53C") MACRO_ADDR;
extern f32 resident_data_0015F540 __asm__("D_0015F540") MACRO_ADDR;
extern s16 resident_data_0015F690 __asm__("D_0015F690");
extern s32 resident_data_0015F6E8 __asm__("D_0015F6E8") MACRO_ADDR;
extern s32 resident_data_0015F6FC[1] __asm__("D_0015F6FC") MACRO_ADDR;
extern ResidentRenderObject_230A90 *resident_data_0016001C __asm__("D_0016001C") MACRO_ADDR;
extern CameraBlendNode_230A90 *resident_data_00160134 __asm__("D_00160134") MACRO_ADDR;
extern s16 resident_duration_base SDATA(D_00160610);
extern ResidentCameraState_230A90 resident_data_00187040 __asm__("D_00187040");
extern ResidentPlaybackState_230A90 resident_data_0018CC20 __asm__("D_0018CC20");
extern f32 resident_data_0018CEB0[4] __asm__("D_0018CEB0");
extern ScriptedPath_230A90 *resident_data_001CC730[] __asm__("D_001CC730");
extern s32 resident_data_001D5F70 __asm__("D_001D5F70") NOT_SDA;
extern PositionPair_230A90 resident_data_001D9D40[] __asm__("D_001D9D40");
extern Vector4_230A90 resident_data_001DA010[] __asm__("D_001DA010");
extern Vector4_230A90 resident_data_001DA040[] __asm__("D_001DA040");
extern u8 resident_effect_entry[] __asm__("func_0022F4C0");

extern void resident_call_002348B8(void) __asm__("func_002348B8");
extern void resident_call_00216EF0(s32) __asm__("func_00216EF0");
extern void resident_call_00216F28(void) __asm__("func_00216F28");
extern void resident_call_00125358(void *) __asm__("func_00125358");
extern void resident_call_001254A0(void *, void *, f32) __asm__("func_001254A0");
extern void resident_call_00125548(void *, void *, f32) __asm__("func_00125548");
extern void resident_call_001253F8(void *, void *, f32) __asm__("func_001253F8");
extern s32 resident_call_00122598(s32) __asm__("func_00122598");
extern void resident_call_001E9770(void *, void *, s32, s32, s32) __asm__("func_001E9770");
extern void resident_call_001E9778(void *) __asm__("func_001E9778");
extern void resident_call_001E9780(void *) __asm__("func_001E9780");
extern void resident_call_001E9788(void) __asm__("func_001E9788");
extern void resident_call_001E9790(ResidentRenderObject_230A90 *) __asm__("func_001E9790");
extern void resident_call_001E9798(ResidentRenderObject_230A90 *) __asm__("func_001E9798");
extern void resident_call_001E97A0(void) __asm__("func_001E97A0");
extern void resident_call_001E97A8(void) __asm__("func_001E97A8");
extern void resident_call_001E97B0(void) __asm__("func_001E97B0");
extern void resident_call_001E97B8(void) __asm__("func_001E97B8");
extern void resident_call_001E97C0(void *, void *, s32, s32) __asm__("func_001E97C0");
extern void resident_call_001E97D0(ResidentRenderObject_230A90 *, ResidentRenderObject_230A90 *) __asm__("func_001E97D0");
extern void resident_call_001EDE50(void) __asm__("func_001EDE50");
extern void resident_call_001F3140(void) __asm__("func_001F3140");
extern void resident_call_001F49B0(void *, ResidentRenderObject_230A90 *) __asm__("func_001F49B0");
extern void resident_call_001F4B68(void *, ResidentRenderObject_230A90 *) __asm__("func_001F4B68");
extern void resident_call_001F4E08(s32) __asm__("func_001F4E08");
extern f32 resident_call_001F98B0(f32) __asm__("func_001F98B0");
extern s32 resident_call_001F98C0(s32) __asm__("func_001F98C0");
extern f32 resident_call_001F9B88(f32) __asm__("func_001F9B88");
extern void resident_call_001F9BC0(void *) __asm__("func_001F9BC0");
extern void resident_call_001F9BD8(void *, void *, void *) __asm__("func_001F9BD8");
extern void resident_call_001F9C08(void *, void *, void *, f32) __asm__("func_001F9C08");
extern void resident_call_001F9C30(void *, void *, f32) __asm__("func_001F9C30");
extern void resident_call_001F9CA0(void *, void *, void *) __asm__("func_001F9CA0");
extern f32 resident_call_001F9D10(void *, void *) __asm__("func_001F9D10");
extern f32 resident_call_001F9D48(void *, void *, void *) __asm__("func_001F9D48");
extern void resident_call_001F9EC0(void *, void *, void *) __asm__("func_001F9EC0");
extern f32 resident_call_001F9F90(f32) __asm__("func_001F9F90");
extern f32 resident_call_001FA058(f32, f32) __asm__("func_001FA058");
extern f32 resident_call_001FA748(f32, f32) __asm__("func_001FA748");
extern f32 resident_call_001FA790(f32, f32) __asm__("func_001FA790");
extern f32 resident_call_001FA888(s32) __asm__("func_001FA888");
extern void resident_call_00202260(void) __asm__("func_00202260");
extern void resident_call_00205220(s32) __asm__("func_00205220");
extern ResidentRenderObject_230A90 *resident_call_0020D348(s32) __asm__("func_0020D348");
extern void resident_call_0020D678(ResidentRenderObject_230A90 *) __asm__("func_0020D678");
extern void resident_call_0020D6D0(ResidentRenderObject_230A90 *) __asm__("func_0020D6D0");
extern void resident_call_0020DE20(void) __asm__("func_0020DE20");
extern void resident_call_0020ED48(ResidentRenderObject_230A90 *) __asm__("func_0020ED48");
extern void resident_call_00213C78(void) __asm__("func_00213C78");
extern void resident_call_00214550(ResidentRenderObject_230A90 *) __asm__("func_00214550");
extern f32 resident_call_00214D88(f32 *, f32 *, f32, f32, f32, f32) __asm__("func_00214D88");
extern void resident_call_00215CA8(ScriptedPath_230A90 *, s32, void *, void *, s32,
                               f32) __asm__("func_00215CA8");
extern void resident_call_00216960(void) __asm__("func_00216960");
extern void resident_call_00217748(s32) __asm__("func_00217748");
extern void resident_call_00218A80(void) __asm__("func_00218A80");
extern void resident_call_00219C70(s32) __asm__("func_00219C70");
extern void resident_call_0022DD68(void) __asm__("func_0022DD68");
extern void resident_call_0022EF68(void) __asm__("func_0022EF68");
extern void resident_call_0022FDC0(void) __asm__("func_0022FDC0");
extern void resident_call_002308C8(ResidentRenderObject_230A90 *, f32) __asm__("func_002308C8");
extern void resident_call_0022F258() __asm__("func_0022F258");
extern void resident_call_0022F738() __asm__("func_0022F738");
extern void resident_call_00233AB8() __asm__("func_00233AB8");

/* Runs the space flight sequence: skip input, end cleanup, camera keyframes and path motion. */
void update_resident_gameplay_state(void) {
    Vector4_230A90 scratch_vectors[4];
    s32 keyframe_flags;
    s32 ticks_per_bank;
    s32 skip;
    s32 object_index;
    s32 point_index;
    ResidentRenderObject_230A90 *object;
    ResidentRenderObject_230A90 *object_cursor;
    ResidentRenderObject_230A90 *expired_object;
    s32 release_index;
    s32 bank_tick_limit;
    ResidentRenderObject_230A90 *player;
    s32 path_point_index;
    f32 camera_blend_step;
    CameraKeyframe_230A90 *keyframe;
    f32 *keyframe_angles;
    u8 *animation_positions;
    s32 current_frame;
    ScriptedPath_230A90 *path;
    f32 segment_angle;
    f32 maximum_turn;
    f32 turn_scale;
    f32 blend;

    resident_data_0013E130.timer++;
    switch (resident_data_0013E130.state) {
    case 0:
    case 8:
        resident_call_001E97B0();
        resident_call_00213C78();
        resident_call_0022EF68();
        resident_call_001E97A0();
        resident_call_00218A80();
        resident_data_0015F53C -= 0.125f;
        resident_data_0018CC20.frame++;
        resident_data_0018CC20.time++;
        if (resident_data_0015F53C < 0.0f) {
            resident_data_0015F53C = 0.0f;
        }
        ticks_per_bank = resident_data_0015EE80 ? 0x50 : 0x60;
        skip = resident_data_0013E130.skip;
        if ((resident_data_0013CBE4[0] & 0x70) && resident_call_001F98C0(30) < resident_data_0018CC20.time &&
            resident_data_0015F53C == 0.0f) {
            if (resident_data_0013E130.state == 0 &&
                resident_data_0018CC20.time <
                    resident_call_001F98C0(((s32 *)&resident_duration_base)[resident_data_0013E130.content_variant] - 30)) {
                skip = 1;
            }
            if (resident_data_0013E130.state == 8 &&
                resident_data_0018CC20.time < resident_data_0018CC20.end - resident_call_001F98C0(30)) {
                skip = 1;
            }
        }
        if (skip) {
            if (resident_data_001517D0.unk5A != 6 && resident_data_001517D0.unk5A != 7) {
                resident_data_001517D0.unk5A = 5;
            }
            if (resident_data_0013E130.state == 0) {
                resident_data_0018CC20.time = resident_call_001F98C0(((s32 *)&resident_duration_base)[resident_data_0013E130.content_variant]);
                resident_data_0018CC20.sequence_frame = resident_data_0018CC20.time / ticks_per_bank;
                resident_call_00205220(resident_data_0018CC20.sequence_frame);
                resident_data_0018CC20.frame = resident_data_0018CC20.time % ticks_per_bank;
                if (resident_data_0013E130.skip) {
                    resident_data_0013E130.skip = 0;
                } else {
                    resident_call_001F4E08(4);
                }
                resident_data_0015F53C = 1.0f;
            } else {
                resident_data_0018CC20.time = resident_data_0018CC20.end;
            }
        }
        if (resident_data_0018CC20.time >= resident_data_0018CC20.end) {
            if (resident_data_001517D0.unk5A != 6 && resident_data_001517D0.unk5A != 7) {
                resident_data_001517D0.unk5A = 5;
            }
            resident_call_002348B8();
            resident_data_0018CEB0[0] = 0.63f;
            resident_call_001F3140();
            /* Retail releases the attachment once per sequence object, even when that object is null. */
            for (release_index = 0; release_index < resident_data_0018CC20.count; release_index++) {
                expired_object = resident_data_0018CC20.objects[release_index];
                if (expired_object != 0) {
                    expired_object->model->count--;
                    expired_object->model->frames[expired_object->model->count] = 0;
                    resident_call_0020D678(expired_object);
                }
                if (resident_data_0013E130.attachment != 0) {
                    resident_call_0020D678(resident_data_0013E130.attachment);
                }
            }
            resident_data_0013E130.player->flags &= ~1;
            for (object_cursor = resident_data_0016001C; object_cursor->state != 0xFF; object_cursor++) {
                if (!(object_cursor->state & 0x80) &&
                    (object_cursor->class_id == 0x4A || object_cursor->class_id == 0xCB)) {
                    object_cursor->flags &= ~0x80;
                }
            }
            if (resident_data_0013E130.state == 0) {
                resident_call_00219C70(0);
                resident_data_001D5F70 = 0xE;
                resident_data_0015F6FC[0] = 1;
                return;
            }
            resident_data_0015F6E8 = 0;
            resident_call_00216F28();
            resident_data_0015F6FC[0] = 1;
            resident_data_001414F5[0] = 0;
            resident_call_001E97B8();
            if (resident_data_0013E130.unk2C != 0) {
                resident_call_001E9788();
                return;
            }
            resident_call_001E97C0(&resident_data_0013E130.unk60, &resident_data_0013E130.unk70, 0, 1);
            return;
        }
        bank_tick_limit = resident_data_0015EE80 ? 0x50 : 0x60;
        if (resident_data_0018CC20.frame >= bank_tick_limit) {
            resident_call_00205220(++resident_data_0018CC20.sequence_frame);
        }
        keyframe = &resident_data_0018CC20.frames[resident_data_0018CC20.frame];
        keyframe_angles = keyframe->rotation;
        keyframe_flags = keyframe->position.bytes[12];
        resident_data_0018CEB0[0] = keyframe_angles[3];
        resident_call_001F3140();
        qcopy(&resident_data_00187040.position, &keyframe->position);
        resident_call_001F9EC0(&resident_data_00187040.position, &resident_data_00187040.position,
                      resident_data_0013E130.player->transform);
        resident_call_001F9BD8(&resident_data_00187040.position, &resident_data_00187040.position,
                    &resident_data_0013E130.player->position);
        segment_angle = resident_call_001FA748(keyframe_angles[2],
                                           resident_data_0013E130.player->rotation.components[2]);
        resident_call_00125358(scratch_vectors);
        resident_call_001254A0(scratch_vectors, scratch_vectors, keyframe->rotation[0]);
        resident_call_00125548(scratch_vectors, scratch_vectors, keyframe_angles[1]);
        resident_call_001253F8(scratch_vectors, scratch_vectors, segment_angle);
        resident_data_00187040.forward.components[0] = -scratch_vectors[2].components[0];
        resident_data_00187040.right.components[0] = -scratch_vectors[0].components[0];
        resident_data_00187040.up.components[0] = scratch_vectors[1].components[0];
        resident_data_00187040.forward.components[1] = -scratch_vectors[2].components[1];
        resident_data_00187040.right.components[1] = -scratch_vectors[0].components[1];
        resident_data_00187040.up.components[1] = scratch_vectors[1].components[1];
        resident_data_00187040.forward.components[2] = -scratch_vectors[2].components[2];
        resident_data_00187040.right.components[2] = -scratch_vectors[0].components[2];
        resident_data_00187040.up.components[2] = scratch_vectors[1].components[2];
        if (resident_data_0015EEB4[0] != 0) {
            resident_call_001F9CA0(&resident_data_00187040.right, &resident_data_00187040.up, &resident_data_00187040.forward);
        }
        for (object_index = 0; object_index < resident_data_0018CC20.count; object_index++) {
            object = resident_data_0018CC20.objects[object_index];
            current_frame = resident_data_0018CC20.frame >> 1;
            object->current_frame = current_frame;
            object->next_frame = current_frame + 1;
            resident_call_0020D6D0(object);
            object->blend = resident_call_001FA888(resident_data_0018CC20.frame & 1) * 0.5f;
            if (keyframe_flags != 0 && (resident_data_0018CC20.frame & 1)) {
                object->blend = 1.0f;
            }
            animation_positions = object->animation_positions;
            resident_call_001F9C30(&scratch_vectors[0], animation_positions + object->current_frame * 16,
                         1.0f - object->blend);
            resident_call_001F9C30(&scratch_vectors[1], animation_positions + object->next_frame * 16,
                         object->blend);
            resident_call_001F9BD8(&object->position, &scratch_vectors[0], &scratch_vectors[1]);
            resident_call_001F9EC0(&object->position, &object->position,
                          resident_data_0013E130.player->transform);
            resident_call_001F9BD8(&object->position, &object->position, &resident_data_0013E130.player->position);
            object->rotation.components[2] = resident_data_0013E130.player->rotation.components[2];
            object->cached_selector = 0xFF;
            resident_call_0020ED48(object);
            if (object->class_id == 0) {
                if (resident_data_0013E130.state == 0) {
                    if (g_current_level_index_r == 10 && resident_data_0013D5C8[6] == 0) {
                        object->unk7F = 0;
                    } else if (resident_data_0018CC20.time <= resident_call_001F98C0(0x3E)) {
                        object->unk7F = 0x18;
                    } else {
                        object->unk7F = 0;
                    }
                } else if (resident_call_001F98C0(360) < resident_data_0018CC20.time) {
                    if (g_current_level_index_r == 10 && resident_data_0013D5C8[6] == 0) {
                        object->unk7F = 0;
                    } else {
                        object->unk7F = 0x18;
                    }
                } else {
                    object->unk7F = 0;
                }
            } else if (object->class_id == 10) {
                if (resident_data_0013E130.state == 0) {
                    if (resident_data_0018CC20.time <= resident_call_001F98C0(350)) {
                        object->unk7F = 0x18;
                    } else {
                        object->unk7F = 0;
                    }
                } else if (resident_data_0018CC20.time <= resident_call_001F98C0(524)) {
                    object->unk7F = 0;
                } else {
                    object->unk7F = 0x18;
                }
            }
            if (object->unk7F != 0) {
                resident_call_00214550(object);
            }
            if (object->class_id == 10) {
                resident_call_001E9798(object);
            }
            if (object->class_id == 0) {
                resident_call_001E9790(object);
                if ((g_current_level_index_r == 10 && resident_data_0013D5C8[6] != 0) ||
                    g_current_level_index_r == 13) {
                    if (resident_data_0013E130.attachment == 0) {
                        resident_data_0013E130.attachment = resident_call_0020D348(0x509);
                        resident_data_0013E130.attachment->selected_index = 0x40;
                        resident_data_0013E130.attachment->flags |= 0x806;
                        resident_data_0013E130.attachment->lifetime_stamp = object->lifetime_stamp;
                        if (resident_data_0013E130.attachment->model->unk6 != 0) {
                            resident_data_0013E130.attachment->unk73 = 0x18;
                        }
                    }
                    resident_call_001E97D0(object, resident_data_0013E130.attachment);
                }
            }
            if ((u16)object->class_id - 0x213U < 3) {
                resident_call_001F4B68(resident_call_00233AB8, object);
                resident_call_001F49B0(resident_call_0022F258, object);
                if (resident_data_0013E130.state == 0) {
                    if (resident_data_0018CC20.time < resident_call_001F98C0(360)) {
                        s32 color_intensity =
                            (s32)(resident_call_001F9F90(
                                      resident_call_001FA888((resident_data_0018CC20.time & 0x3F) - 32) *
                                      0.09817477f) *
                                  80.0f) +
                            120;
                        if (object->class_id == 0x215) {
                            object->color = (color_intensity >> 1) | (color_intensity << 8) |
                                            (color_intensity << 16);
                        } else {
                            object->color =
                                color_intensity | (color_intensity << 8) | (color_intensity << 16);
                        }
                    } else if (resident_data_0018CC20.time < resident_call_001F98C0(0x1F8)) {
                        s32 color_intensity =
                            (s32)(((f32)resident_data_0018CC20.time - resident_call_001F98B0(360.0f)) *
                                  (resident_data_0015EE60 * 1.25f));
                        if (color_intensity >= 256) {
                            color_intensity = 255;
                        }
                        if (object->class_id == 0x215) {
                            object->color = (color_intensity << 8) | (color_intensity << 16);
                        } else {
                            object->color =
                                color_intensity | (color_intensity << 8) | (color_intensity << 16);
                        }
                        if (resident_call_001F98C0(0x1A4) < resident_data_0018CC20.time) {
                            resident_data_0013E130.player->unkBC =
                                (s32)(((f32)resident_data_0018CC20.time - resident_call_001F98B0(420.0f)) *
                                      (resident_data_0015EE60 * 1.2f));
                            resident_data_0013E130.player->unkB2 = 0;
                            resident_call_001F49B0(resident_effect_entry + 8, resident_data_0013E130.player);
                        }
                        if (resident_call_001F98C0(0x1D0) < resident_data_0018CC20.time) {
                            resident_data_0015F540 += 0.025f;
                            if (resident_data_0015F540 > 1.0f) {
                                resident_data_0015F540 = 1.0f;
                            }
                        }
                    } else {
                        object->color = 0xA0A0A0;
                        resident_data_0015F540 -= 0.025f;
                        if (resident_data_0015F540 < 0.0f) {
                            resident_data_0015F540 = 0.0f;
                        }
                    }
                } else if (resident_data_0018CC20.time < resident_call_001F98C0(0xF0)) {
                    s32 color_intensity;
                    if (resident_data_0018CC20.time < resident_call_001F98C0(0xC8)) {
                        resident_call_002308C8(object, resident_data_0015EE6C * -4.0f);
                        resident_call_002308C8(object, resident_data_0015EE6C * -3.0f);
                    }
                    color_intensity = (s32)(resident_call_001F9F90(resident_call_001FA888(
                                                         (resident_data_0018CC20.time & 0x3F) - 32) *
                                                     0.09817477f) *
                                            80.0f) +
                                      120;
                    if (object->class_id == 0x215) {
                        object->color = (color_intensity >> 1) | (color_intensity << 8) |
                                        (color_intensity << 16);
                    } else {
                        object->color =
                            color_intensity | (color_intensity << 8) | (color_intensity << 16);
                    }
                    object->unkBC = 0x32;
                    object->unkB2 = 10;
                    resident_call_001F49B0(resident_effect_entry + 8, object);
                } else if (resident_data_0018CC20.time <= resident_call_001F98C0(360)) {
                    s32 color_intensity;
                    if (resident_data_0018CC20.time < resident_call_001F98C0(300)) {
                        resident_data_0013E130.player->unkBC =
                            (s32)((resident_call_001F98B0(300.0f) - (f32)resident_data_0018CC20.time) *
                                  (resident_data_0015EE60 * 1.5f));
                        resident_data_0013E130.player->unkB2 = 0;
                        resident_call_001F49B0(resident_effect_entry + 8, resident_data_0013E130.player);
                    }
                    color_intensity = (s32)((resident_call_001F98B0(360.0f) - (f32)resident_data_0018CC20.time) *
                                            (resident_data_0015EE60 * 1.25f));
                    if (object->class_id == 0x215) {
                        object->color = (color_intensity << 8) | (color_intensity << 16);
                    } else {
                        object->color =
                            color_intensity | (color_intensity << 8) | (color_intensity << 16);
                    }
                } else {
                    s32 color_intensity = (s32)(resident_call_001F9F90(resident_call_001FA888(
                                                             (resident_data_0018CC20.time & 0x3F) - 32) *
                                                         0.09817477f) *
                                                80.0f) +
                                          120;
                    if (object->class_id == 0x215) {
                        object->color = (color_intensity >> 1) | (color_intensity << 8) |
                                        (color_intensity << 16);
                    } else {
                        object->color =
                            color_intensity | (color_intensity << 8) | (color_intensity << 16);
                    }
                }
            }
        }
        resident_call_0022DD68();
        resident_call_00202260();
        resident_call_0020DE20();
        resident_call_001E97A8();
        break;

    case 3:
        resident_call_001E97B0();
        resident_call_00213C78();
        resident_call_0022EF68();
        resident_call_001E97A0();
        resident_call_00218A80();
        if (resident_data_0013E130.timer == 0) {
            resident_call_00216EF0(0);
            resident_call_00217748(0);
            resident_data_001517D0.unk1C = resident_data_0013E130.content_variant + 0x9C4F;
            resident_call_001F4E08(resident_call_001F98C0(12));
            while ((s16)resident_data_001517D0.unk5A != 3) {
                resident_call_0022DD68();
                resident_call_00122598(0);
            }
            resident_call_00216960();
            resident_data_0013E130.player->selected_a = 1;
            resident_data_0013E130.player->selected_b = 2;
            resident_data_0013E130.player->blend = 0.0f;
            resident_call_0020D6D0(resident_data_0013E130.player);
            resident_data_0013E130.player->opacity = 0xFF;
            resident_data_0013E130.player->unk94 = 0;
            if (resident_data_0013E130.path >= 0) {
                resident_data_0013E130.companion_a = resident_call_0020D348(0);
                resident_data_0013E130.companion_a->selected_index = 0x1FF;
                resident_data_0013E130.companion_a->opacity = 0xFF;
                resident_data_0013E130.companion_a->unk94 = 0;
                resident_data_0013E130.companion_a->flags |= 6;
                resident_data_0013E130.companion_a->lifetime_stamp =
                    resident_data_0013F450.reference_object->lifetime_stamp;
                resident_data_0013E130.companion_b = resident_call_0020D348(10);
                resident_data_0013E130.companion_b->selected_index = 0x1FF;
                resident_data_0013E130.companion_b->opacity = 0xFF;
                resident_data_0013E130.companion_b->unk94 = 0;
                resident_data_0013E130.companion_b->flags |= 6;
                resident_data_0013E130.companion_b->lifetime_stamp =
                    resident_data_0013F450.reference_object->lifetime_stamp;
            }
            resident_call_001E9770(&resident_data_00187040.position, &resident_data_00187040.unk150, 1, 0, 0);
            resident_data_0013E130.blend = 0.0f;
            resident_data_0013E130.interpolation_velocity = 0.0f;
            if (resident_data_0013E130.path >= 0) {
                path = resident_data_001CC730[resident_data_0013E130.path];
                maximum_turn = resident_data_0013E130.blend;
                path->p[0].components[3] = maximum_turn;
                for (path_point_index = 1; path_point_index < path->point_count - 1;
                     path_point_index++) {
                    segment_angle = resident_call_001FA058(path->p[path_point_index].components[0] -
                                                      path->p[path_point_index - 1].components[0],
                                                  path->p[path_point_index].components[1] -
                                                      path->p[path_point_index - 1].components[1]);
                    path->p[path_point_index].components[3] = resident_call_001FA790(
                        resident_call_001FA058(path->p[path_point_index + 1].components[0] -
                                          path->p[path_point_index].components[0],
                                      path->p[path_point_index + 1].components[1] -
                                          path->p[path_point_index].components[1]),
                        segment_angle);
                    if (maximum_turn < resident_call_001F9B88(path->p[path_point_index].components[3])) {
                        maximum_turn = resident_call_001F9B88(path->p[path_point_index].components[3]);
                    }
                }
                segment_angle = resident_call_001FA058(path->p[path->point_count - 1].components[0] -
                                                  path->p[path->point_count - 2].components[0],
                                              path->p[path->point_count - 1].components[1] -
                                                  path->p[path->point_count - 2].components[1]);
                path->p[path->point_count - 1].components[3] = resident_call_001FA790(
                    resident_call_001FA058(
                        path->p[0].components[0] - path->p[path->point_count - 1].components[0],
                        path->p[0].components[1] - path->p[path->point_count - 1].components[1]),
                    segment_angle);
                turn_scale = 0.34906584f / maximum_turn;
                for (point_index = 0; point_index < path->point_count; point_index++) {
                    path->p[point_index].components[3] *= turn_scale;
                }
                resident_data_0013E130.path_segment_length = resident_call_001F9D10(&path->p[0], &path->p[1]);
                resident_data_0013E130.path_progress = 0.0f;
                resident_data_0013E130.speed = 0.0f;
                resident_data_0013E130.trail = 0;
                resident_data_0013E130.history_count = 0;
                player = resident_data_0013E130.player;
                qcopy(&resident_data_0013E130.startPos, &player->position);
                qcopy(&resident_data_0013E130.pathPos, &path->p[0]);
                qcopy(&resident_data_0013E130.startRot, &player->rotation);
                resident_data_0013E130.rotY = resident_call_001FA058(
                    resident_call_001F9D48(&path->p[1], &path->p[0], &resident_data_0013E130.startPos),
                    path->p[0].components[2] - path->p[1].components[2]);
                resident_data_0013E130.rotZ =
                    resident_call_001FA058(path->p[1].components[0] - path->p[0].components[0],
                                  path->p[1].components[1] - path->p[0].components[1]);
            }
        }
        if (resident_data_0013E130.source_camera_index >= 0 &&
            resident_data_0013E130.destination_camera_index >= 0) {
            camera_blend_step = resident_data_0015EE70 * 0.666f;
            resident_call_00214D88(&resident_data_0013E130.blend,
                                       &resident_data_0013E130.interpolation_velocity, 1.0f,
                                       camera_blend_step, camera_blend_step, resident_data_0015EE6C * 0.5f);
            resident_call_001F9C08(&scratch_vectors[0],
                          &resident_data_00160134[resident_data_0013E130.source_camera_index].position,
                          &resident_data_00160134[resident_data_0013E130.destination_camera_index].position,
                          resident_data_0013E130.blend);
            resident_call_001F9BC0(&scratch_vectors[1]);
            scratch_vectors[1].components[1] =
                resident_call_001FA790(
                    resident_data_00160134[resident_data_0013E130.destination_camera_index].rotY,
                    resident_data_00160134[resident_data_0013E130.source_camera_index].rotY) *
                resident_data_0013E130.blend;
            scratch_vectors[1].components[1] =
                resident_call_001FA748(resident_data_00160134[resident_data_0013E130.source_camera_index].rotY,
                                   scratch_vectors[1].components[1]);
            scratch_vectors[1].components[2] =
                resident_call_001FA790(
                    resident_data_00160134[resident_data_0013E130.destination_camera_index].rotZ,
                    resident_data_00160134[resident_data_0013E130.source_camera_index].rotZ) *
                resident_data_0013E130.blend;
            scratch_vectors[1].components[2] =
                resident_call_001FA748(resident_data_00160134[resident_data_0013E130.source_camera_index].rotZ,
                                   scratch_vectors[1].components[2]);
            resident_call_001E9778(&scratch_vectors[0]);
            resident_call_001E9780(&scratch_vectors[1]);
        }
        if (resident_data_0013E130.path >= 0) {
            if (resident_data_0013E130.timer < resident_call_001F98C0(150)) {
                blend =
                    (1.0f - resident_call_001F9F90(resident_call_001FA888(resident_data_0013E130.timer) *
                                     (3.1415927f / resident_call_001FA888(resident_call_001F98C0(120))))) *
                    0.5f;
                if (resident_data_0013E130.timer >= resident_call_001F98C0(120)) {
                    blend = 1.0f;
                }
                resident_data_0013E130.player->blend = blend;
                resident_call_001F9C08(&resident_data_0013E130.player->position, &resident_data_0013E130.startPos,
                              &resident_data_0013E130.pathPos, blend);
                resident_data_0013E130.player->rotation.components[0] = 0.0f;
                resident_data_0013E130.player->rotation.components[1] = resident_call_001FA748(
                    resident_data_0013E130.startRot.components[1],
                    resident_call_001FA790(resident_data_0013E130.rotY,
                                            resident_data_0013E130.startRot.components[1]) *
                        blend);
                resident_data_0013E130.player->rotation.components[2] = resident_call_001FA748(
                    resident_data_0013E130.startRot.components[2],
                    resident_call_001FA790(resident_data_0013E130.rotZ,
                                            resident_data_0013E130.startRot.components[2]) *
                        blend);
                if (resident_data_0015F53C > 0.0f) {
                    resident_data_0015F53C -= 0.125f;
                    if (resident_data_0015F53C < 0.0f) {
                        resident_data_0015F53C = 0.0f;
                    }
                }
                if (blend < 1.0f) {
                    resident_call_002308C8(resident_data_0013E130.player, resident_data_0015EE6C * -3.75f);
                }
            } else {
                ScriptedPath_230A90 *active_path;
                resident_data_0013E130.speed += 0.8f;
                active_path = resident_data_001CC730[resident_data_0013E130.path];
                if (resident_data_0013E130.speed > 100.0f) {
                    resident_data_0013E130.speed = 100.0f;
                }
                if ((resident_data_0013CBE4[0] & 0x70) && resident_data_0015F53C < 0.0625f) {
                    resident_data_0015F53C = 0.0625f;
                }
                if ((f32)(active_path->point_count - 6) < resident_data_0013E130.path_progress &&
                    resident_data_0015F53C < 0.0625f) {
                    resident_data_0015F53C = 0.0625f;
                }
                if (resident_data_0013E130.content_variant < 2) {
                    resident_data_0013E130.player->unkBC = 0x32;
                    resident_data_0013E130.player->unkB2 = 10;
                    resident_call_001F49B0(resident_effect_entry + 8, resident_data_0013E130.player);
                }
                resident_data_0013E130.path_progress +=
                    resident_data_0013E130.speed * resident_data_0015EE6C / resident_data_0013E130.path_segment_length;
                if ((f32)(active_path->point_count - 1) < resident_data_0013E130.path_progress ||
                    resident_data_0015F53C >= 1.0f) {
                    *(s32 *)&resident_data_0015F690 = 1;
                    resident_data_0015F6FC[0] = 1;
                } else {
                    resident_call_00215CA8(active_path, 1, &resident_data_0013E130.player->position,
                                       &resident_data_0013E130.player->rotation, 0,
                                       resident_data_0013E130.path_progress);
                    resident_data_0013E130.player->rotation.components[0] =
                        resident_data_0013E130.player->rotation.components[3];
                    resident_data_0013E130.trail = (resident_data_0013E130.trail + 1) & 0x1F;
                    if (resident_data_0013E130.history_count < 0x20) {
                        resident_data_0013E130.history_count++;
                    }
                    resident_call_001F9EC0(&resident_data_0013E130.trailA[resident_data_0013E130.trail],
                                  &resident_data_001D9D40[resident_data_0013E130.content_variant].a,
                                  resident_data_0013E130.player->transform);
                    resident_call_001F9BD8(&resident_data_0013E130.trailA[resident_data_0013E130.trail],
                                &resident_data_0013E130.trailA[resident_data_0013E130.trail],
                                &resident_data_0013E130.player->position);
                    resident_data_0013E130.trailA[resident_data_0013E130.trail].components[3] = 1.0f;
                    resident_call_001F9EC0(&resident_data_0013E130.trailB[resident_data_0013E130.trail],
                                  &resident_data_001D9D40[resident_data_0013E130.content_variant].b,
                                  resident_data_0013E130.player->transform);
                    resident_call_001F9BD8(&resident_data_0013E130.trailB[resident_data_0013E130.trail],
                                &resident_data_0013E130.trailB[resident_data_0013E130.trail],
                                &resident_data_0013E130.player->position);
                    resident_data_0013E130.trailB[resident_data_0013E130.trail].components[3] = 1.0f;
                    resident_call_001F49B0(resident_call_0022F738,
                                            resident_data_0013E130.player);
                    if (resident_data_0015F53C > 0.0f) {
                        resident_data_0015F53C += 0.0625f;
                        if (resident_data_0015F53C > 1.0f) {
                            resident_data_0015F53C = 1.0f;
                        }
                    }
                }
            }
            {
                ResidentRenderObject_230A90 *p = resident_data_0013E130.player;
                ResidentRenderObject_230A90 *scratch_vectors = resident_data_0013E130.companion_a;
                qcopy(&scratch_vectors->rotation, &p->rotation);
                resident_call_001F9EC0(&scratch_vectors->position,
                              &resident_data_001DA010[resident_data_0013E130.content_variant], p->transform);
            }
            resident_call_001F9BD8(&resident_data_0013E130.companion_a->position,
                        &resident_data_0013E130.companion_a->position,
                        &resident_data_0013E130.player->position);
            resident_call_0020ED48(resident_data_0013E130.companion_a);
            {
                ResidentRenderObject_230A90 *scratch_vectors = resident_data_0013E130.companion_b;
                ResidentRenderObject_230A90 *p = resident_data_0013E130.player;
                qcopy(&scratch_vectors->rotation, &p->rotation);
                resident_call_001F9EC0(&scratch_vectors->position,
                              &resident_data_001DA040[resident_data_0013E130.content_variant], p->transform);
            }
            resident_call_001F9BD8(&resident_data_0013E130.companion_b->position,
                        &resident_data_0013E130.companion_b->position,
                        &resident_data_0013E130.player->position);
            resident_call_0020ED48(resident_data_0013E130.companion_b);
        }
        resident_call_001EDE50();
        resident_call_0022DD68();
        resident_call_00202260();
        resident_call_0020DE20();
        resident_call_001E97A8();
        break;

    case 4:
        resident_call_0022FDC0();
        break;
    case 1:
    case 2:
    case 5:
    case 6:
    case 7:
        break;
    }
}

extern int ship_substate;
extern void prepare_debug_profiler_render(void);
extern void render_level_frame(void);

/* A switch on D_0013E150, whose jump table is linked into the data
   segment at retail's jtbl_001E8C90 (see rac1.ld.sh). */
void dispatch_game_state_update(void) {
    if (space_load_in_progress_m == 0) {
        switch (ship_substate) {
        case 0:
        case 8:
            prepare_debug_profiler_render();
            break;
        case 3:
        case 7:
            *(int *)&draw_enable_mask = 0x7F;
            DrawDebugProfiler();
            break;
        case 4:
            render_level_frame();
            break;
        }
    }
}

extern s32 current_vu_chain MACRO_ADDR;
typedef struct { long q[2]; } dummy_q;
typedef struct {
    s32 tex;         /* 0x00 */
    s32 data_offset; /* 0x04 */
    s32 texCount;    /* 0x08 */
    s32 texInfo;     /* 0x0C */
    s32 mobyCount;   /* 0x10 */
    s32 mobys;       /* 0x14 */
    s32 classCount;  /* 0x18 */
    s32 classes;     /* 0x1C */
    s32 partCount;   /* 0x20 */
    s32 parts;       /* 0x24 */
    s32 pointCount;  /* 0x28 */
    s32 points;      /* 0x2C */
    s32 unk30;
    s32 unk34;
    s32 unk38;
    s32 unk3C;
    s32 unk40;
    s32 unk44;
    s32 sky; /* 0x48 */
    s32 unk4C;
    s32 scenes[5]; /* 0x50 */
    s32 sound;     /* 0x64 */
} LevelHeader;

typedef struct {
    s32 offset;
    u16 id;
    u16 pad;
    s32 pad8[2];
} ClassEntry;

typedef struct {
    s32 offset;
    s32 oclass;
    s32 pad8[2];
    u8 body[0x10];
} MobyEntry;

typedef struct {
    s32 offset;
    s32 size;
} Chunk;

typedef struct {
    u8 pad0[4];
    s32 unk4;
    s32 unk8;
    u8 padC[8];
    u8 *buffer;
    s32 unk18;
} MemInfo;

typedef struct {
    u8 pad0[0x58];
    s32 unk58;
    s32 unk5C;
    s32 chunks[70];
} SceneInfo;

typedef struct { s32 sector; s32 size; } DiscFile_q;
typedef struct { u8 pad[0x13B8]; DiscFile_q unk13B8; } DiscTable_q;
typedef struct {
    s32 pad0, pad4, pad8, padC; s32 pad10;
    s32 pad14;
} Unused_q;
typedef struct {
    s32 state; s16 timer; u8 pad26[0x32]; s32 history_index; s32 history_count; s32 scene_mode; s32 scene_state;
} LevelRenderTail_q;
typedef struct {
    u8 pad0[0x20];
    s32 state;
    s16 timer;
    u8 pad26[0x2A];
    s32 history_index;
    s32 history_count;
    s32 scene_mode;
    s32 scene_state;
} LevelRenderState_q;
extern s32 white_fade_level MACRO_ADDR;
extern s32 g_gs_texture_allocation_base MACRO_ADDR;
extern s32 g_gs_texture_allocation_cursor MACRO_ADDR;
extern s32 g_gs_texture_allocation_start MACRO_ADDR;
extern s32 g_game_language MACRO_ADDR;
extern u8 coll_output_aux_buffer[];
extern u8 resident_class_material_maps[0xE00];
extern s32 resident_class_original_field_2c[];
struct CommonArchiveMemory;
extern struct CommonArchiveMemory level_resource_buffers;
struct DiscTable;
extern struct DiscTable disc_table;
extern u64 special_material_template[3];
extern s32 resident_indexed_texture_count MACRO_ADDR;
extern s32 resident_indexed_texture_addrs[];
extern s32 resident_moby_class_count MACRO_ADDR;
extern s32 D_0016100C MACRO_ADDR;
extern s32 level_texture_payload_address MACRO_ADDR;
extern char resident_indexed_textures[0x1000];
extern u8 point_lights[];
extern u8 point_light_data[];
extern char dir_lights[];
extern u8 default_dir_lights[];
extern u8 * resident_object_pool MACRO_ADDR;
extern u8 * moby_instance_end MACRO_ADDR;
extern u8 * g_moby_vars MACRO_ADDR;
extern u8 * part_instance_pool MACRO_ADDR;
extern s32 part_slot_high_water MACRO_ADDR;
extern u8 * moby_instance_perm_end MACRO_ADDR;
extern s32 part_slot_search_start MACRO_ADDR;
extern s32 part_active_count MACRO_ADDR;
extern u8 part_slot_bitmap[];
extern s32 occl_fog_count MACRO_ADDR;
extern s32 part_clip_dist MACRO_ADDR;
extern short D_001605E0;
extern u8 level_available[20] NOT_SDA;
extern u8 level_init_cleared_block[];
extern u8 level_init_cleared_block2[];
extern u32 sound_bank_handle MACRO_ADDR;
extern void InitMemSlots(void);
extern void VU1_initChain(void);
extern s32 Stream_SubmitRead(void *buf, u32 lsn, u32 sectors);
extern void FadeToBlack(s32 n);
extern s32 fast_decompress(s32, s32);
extern void upload_texture_images(s32 arg0, s32 count, s32 p);
extern s32 log2dim(s32);
extern void relocate_sky_definition(s32 f);
extern void register_moby_class(s32 moby, s32 arg1, s32 arg2, s32 oclass);
extern void unpack_point_records(s32 src, s32 count);
extern void ParseParticleTexs(s32 hdr, s32 base, s32 src, s32 count);
extern s32 upload_mip_texture(s32, void *);
extern void FastMemZero16(void *, s32);
extern void ResetDrawGlobals(void);
extern void parse_space_scene_chunk(s32 index);
extern void snd_BankLoadFromEE_CB(s32 cmd, void *arg, s64 data);
extern void SoundBank_StoreHandle();
#define sequence (*(f32 *)&D_0015F53C)
#define D_001413D0 ((s32 *)&D_001414D0)
#define current_vu_chain_r D_0016100C
#define space_load_in_progress_r (*(s32 *)((u8 *)&D_0015F560 + 0x19C))
#define D_0015F440 D_0015F540
#define gs_texture_allocation_base D_0015EF8C
#define gs_texture_allocation_cursor D_0015EF74
#define gs_texture_allocation_start D_0015EF78
#define current_level_index (*(s32 *)&D_0015EE84)
#define game_language D_0015EE88
#define D_00194180 D_00194280
#define resident_class_slot_by_id_r ((u8 *)&D_001B3E40)
#define D_001B6180 D_001B6500
#define D_001941C0_q (*(MemInfo *)&D_001941C0)
#define disc_table_r (*(DiscTable_q *)&D_00137C80)
#define level_render_state_r (*(LevelRenderState_q *)&D_0013E130)
#define D_0015FF08 D_00160008
#define D_001B5980 D_001B5D00
#define D_0015FF00 D_00160000
#define D_00160580 (*(u64 *)&D_00160680)
#define D_00160588 (*(s64 *)&D_00160688)
#define D_001D9740 D_001D9AD0
#define D_0015FF18 D_00160018
#define moby_instance_end_r D_0016001C
#define moby_vars D_00160028
#define D_001600AC D_001601AC
#define D_001600B4 D_001601B4
#define D_001600B0 D_001601B0
#define D_001600B8 D_001601B8
#define D_001CD780 D_001CDB00
#define D_001604E0 (*(s64 *)&D_001605E0)
#define D_001604F0 (*(s64 *)&D_001605F0)
#define level_available_r D_0013DE48
#define D_0018CB20 (*(SceneInfo *)&D_0018CC20)
#define D_00186310 D_00186410
#define D_00186350 D_00186450
#define sound_bank_handle_r D_0015EE5C
#define init_mem_slots func_00201E10
#define fill_transfer_words ((void (*)(void *dst, s32 value, s32 size))func_001F99B0)
#define init_view_context ((void (*)(void))func_001F3008)
#define update_view_context ((void (*)(void))func_001F3140)
#define vu1_init_chain func_002348E8
#define submit_audio_stream_io_request func_002175C8
#define scale_game_frames ((s32 (*)(s32))func_001F98C0)
#define fade_to_black func_001F4E08
#define sceCdSync ((s32 (*)(s32))func_00120F30)
#define FlushCache ((void (*)(s32))func_00118D80)
#define func_0020B618(a, b) func_0020C468((s32)(a), (s32)(b))
#define highest_set_bit_index func_001F9968
#define copy_blocks_16_forward ((void (*)(void *, void *, s32))func_001F9A98)
#define parse_particle_textures func_00202F00
#define clear_blocks_16 func_001F99D8
#define reset_draw_globals func_001F3B90
#define rand ((s32 (*)(void))func_001160D8)
#define parse_space_scene_chunk_r func_00205220
#define snd_bank_load_from_ee_cb func_0012E1C8
#define store_async_sound_bank_handle func_0022F090
#define sceGsSyncV ((s32 (*)(s32))func_00122598)
#define snd_flush_sound_commands ((s32 (*)(void))func_0012DDC0)
#define snd_resolve_bank_xrefs ((void (*)(void))func_0012E2E8)
/* Sets up a newly loaded level: memory slots, moby classes, textures, sky and view contexts. */
void initialize_level_runtime(void) {
    s64 texture_packet[4];
    LevelHeader *header;
    s32 data_base;
    s32 class_data_base;
    ClassEntry *class_entries;
    ClassEntry *class_entry;
    s32 *output;
    MobyEntry *moby_entry;
    s32 *texture_offsets;
    s32 language_index;
    s32 archive_size;
    u8 *allocation_cursor;
    s32 *scene_offsets;
    s32 *scene_offset;
    Chunk *scene_chunk;
    s32 i;
    s32 j;
    s32 k;
    s32 texture_cursor;
    s32 scene_variant;
    s32 workspace_offset;

    level_render_state_r.state = 4;
    level_render_state_r.timer = -1;
    sequence = 1.0f;
    D_001413D0[0] = 0;
    current_vu_chain_r = 0x100000;
    space_load_in_progress_r = 0;
    D_0015F440 = 0;
    init_mem_slots();
    gs_texture_allocation_cursor = gs_texture_allocation_base;
    gs_texture_allocation_start = gs_texture_allocation_base;
    fill_transfer_words(D_00194180, 0x87654321, 0x10);
    fill_transfer_words(resident_class_slot_by_id_r, -1, 0x800);
    fill_transfer_words(resident_class_material_maps, -1, 0xE00);
    fill_transfer_words(D_001B6180, 0, 0xE0);
    init_view_context();
    update_view_context();
    vu1_init_chain();
    submit_audio_stream_io_request(D_001941C0_q.buffer + 0x400000, disc_table_r.unk13B8.sector,
                                   disc_table_r.unk13B8.size);
    fade_to_black(scale_game_frames(0xC));
    sceCdSync(0);
    FlushCache(0);
    archive_size = func_0020B618(D_001941C0_q.buffer + 0x400000, D_001941C0_q.buffer);
    FlushCache(0);
    header = (LevelHeader *)D_001941C0_q.buffer;
    upload_texture_images((s32)header + header->tex, header->texCount,
                          (s32)header + header->texInfo);
    data_base = (s32)header + header->data_offset;
    class_data_base = data_base + header->unk30;
    special_material_template[0] = (s32)((gs_texture_allocation_base + header->unk40) >> 8) | 0x1D308000 | ((s64)0xB800 << 19) |
                    ((s64)((gs_texture_allocation_base + header->unk44) >> 8) << 37) | 0x8000000000000000LL;
    special_material_template[1] = 0x0000FFA0000000E0LL;
    special_material_template[2] = 0x0040000400004000LL;
    class_entries = (ClassEntry *)((s32)header + header->classes);
    D_0015FF08 = header->classCount;
    for (i = 0; i < D_0015FF08; i++) {
        D_001B5980[i] = class_data_base + class_entries[i].offset +
                        (highest_set_bit_index(class_entries[i].id) << 28);
    }
    copy_blocks_16_forward(resident_indexed_textures, class_entries, D_0015FF08 * 16);
    relocate_sky_definition(data_base + header->sky);
    moby_entry = (MobyEntry *)((s32)header + header->mobys);
    D_0015FF00 = 0;
    for (j = 0; j < header->mobyCount; j++) {
        register_moby_class(moby_entry->offset == 0 ? 0 : data_base + moby_entry->offset,
                            (s32)header + header->classes, (s32)moby_entry->body,
                            moby_entry->oclass);
        moby_entry++;
    }
    level_texture_payload_address = data_base + header->unk38;
    unpack_point_records((s32)header + header->points, header->pointCount);
    {
        s32 particle_data_base = data_base + header->unk34;
        s32 particle_entries = (s32)header + header->parts;

        parse_particle_textures((s32)header + header->unk3C, particle_data_base, particle_entries,
                                header->partCount);
    }
    texture_offsets = (s32 *)(data_base + header->unk4C);
    language_index = game_language - 1;
    if (language_index < 0) {
        language_index = 0;
    }
    upload_mip_texture((s32)texture_offsets + texture_offsets[current_level_index + 1], texture_packet);
    D_00160580 = texture_packet[0];
    upload_mip_texture((s32)texture_offsets +
                           texture_offsets[language_index * 19 + current_level_index + 0x14],
                       texture_packet);
    texture_cursor = gs_texture_allocation_cursor + 0x2000;
    allocation_cursor = D_001941C0_q.buffer + archive_size;
    gs_texture_allocation_cursor = texture_cursor;
    D_001941C0_q.unk18 = (s32)allocation_cursor;
    gs_texture_allocation_start = texture_cursor;
    D_00160588 = texture_packet[0];
    clear_blocks_16(point_lights, 0x100);
    clear_blocks_16(point_light_data, 0x180);
    clear_blocks_16(dir_lights, 0x400);
    copy_blocks_16_forward(dir_lights, D_001D9740, 0x40);
    D_0015FF18 = allocation_cursor;
    fill_transfer_words(allocation_cursor, 0, 0x4000);
    allocation_cursor += 0x4000;
    moby_instance_end_r = D_0015FF18;
    D_0015FF18[0x20] = 0xFF;
    moby_vars = allocation_cursor;
    allocation_cursor += 0x2000;
    D_001600AC = allocation_cursor;
    D_001600B4 = -1;
    moby_instance_perm_end = D_0015FF18 + 0x3F00;
    D_001600B0 = 0;
    D_001600B8 = 0;
    clear_blocks_16(D_001CD780, 0x200);
    reset_draw_globals();
    occl_fog_count = 0x1F4;
    part_clip_dist = 0x1F4000;
    scene_variant = (rand() >> 16) & 3;
    level_render_state_r.scene_state = 0;
    level_render_state_r.scene_mode = scene_variant;
    level_render_state_r.history_index = 0;
    level_render_state_r.history_count = 0;
    qcopy(&shrub_tex_dma_tag, &shrub_colors);
    if (current_level_index == 0 || (current_level_index == 1 && level_available_r[3] == 0)) {
        level_render_state_r.scene_mode = 4;
        level_render_state_r.scene_state = 2;
    }
    fill_transfer_words(&D_0018CB20, 0, 0x1C0);
    fill_transfer_words(D_00186310, 0, 0x40);
    fill_transfer_words(D_00186350, 0, 0x40);
    scene_offsets = header->scenes;
    workspace_offset = current_vu_chain - 0x60000;
    scene_offset = &scene_offsets[level_render_state_r.scene_mode];
    D_0018CB20.unk58 = D_001941C0_q.unk4 + workspace_offset;
    D_0018CB20.unk5C = D_001941C0_q.unk8 + workspace_offset;
    current_vu_chain = workspace_offset;
    scene_chunk = (Chunk *)(data_base + header->scenes[level_render_state_r.scene_mode]);
    /* The archive list is terminated by a zero size and the resident table holds 70 entries. */
    for (k = 0; k < 70 && scene_chunk->size != 0; k++) {
        {
            s32 chunk_offset = scene_chunk->offset + 0x800;
            D_0018CB20.chunks[k] = data_base + *scene_offset + chunk_offset;
        }
        scene_chunk++;
    }
    parse_space_scene_chunk_r(0);
    sound_bank_handle_r = 0xFFFFFFFF;
    snd_bank_load_from_ee_cb(data_base + header->sound, store_async_sound_bank_handle,
                             (u32)&sound_bank_handle_r);
    do {
        FlushCache(0);
        sceGsSyncV(0);
        snd_flush_sound_commands();
    } while (sound_bank_handle_r == 0xFFFFFFFF);
    snd_resolve_bank_xrefs();
}

typedef struct {
    int sector;
    int size;
} WadEntry;
typedef struct {
    char _pad0[0x1938];
    WadEntry movie[12];  /* 0x1938 */
    WadEntry movie2[12]; /* 0x1998 */
} WadToc;
extern WadToc disc_table_t __asm__("D_00137C80");
extern int pal_mode MACRO_ADDR;
extern int g_game_language MACRO_ADDR;
extern int level_resource_buffer_base;
extern int Movie_Play(int, int, int, int, int);
extern void wait_for_graphics_pipeline_idle(int, int);
extern void sceGsSyncVCallback(void *);
extern void vsync_callback(void);
extern void FadeToBlack(int);

/* Plays movie id, taking its {sector, size} from the WAD table of
   contents (from the second table when D_0015EE80 is set). */
void play_level_transition_movie(int id) {
    int sector;
    int size;
    int buf;

    if (pal_mode != 0) {
        sector = disc_table_t.movie2[id].sector;
        size = disc_table_t.movie2[id].size;
    } else {
        sector = disc_table_t.movie[id].sector;
        size = disc_table_t.movie[id].size;
    }
    voice_pool[0x6B] |= 8;
    buf = level_resource_buffer_base;
    Movie_Play(sector, size, (buf + 0x3F) & ~0x3F,
                  (buf + 0x300000 + 0x3F) & ~0x3F, g_game_language);
    sce_gs_sync_v(0);
    wait_for_graphics_pipeline_idle(0, 0);
    sceGsSyncVCallback(vsync_callback);
    FadeToBlack(4);
    voice_pool[0x6B] |= 0x10;
}

extern int *g_render_packet_cursor MACRO_ADDR;
extern int g_screen_extent[];
extern char textured_quad_giftag[];

typedef union {
    struct {
        float u;
        float v;
    } f;
    long bits;
} SpaceUvPair;

/* Append a textured four-corner GIF packet. Screen coordinates use 12.4
   fixed point relative to the viewport origin; UV pairs stay as floats. */
void append_scrolling_textured_quad(int x, int y, int w, int h, unsigned long rgba,
                   unsigned long tex, float u0, float u1, float v0, float v1) {
    int x0 = x * 16 + g_screen_extent[4] - 8;
    int x1 = (x + w) * 16 + g_screen_extent[4] - 8;
    int y0 = y * 16 + g_screen_extent[5] - 8;
    int y1 = (y + h) * 16 + g_screen_extent[5] - 8;
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

    g_render_packet_cursor[0] = 0x10000007;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000007;
    base = g_render_packet_cursor;
    g_render_packet_cursor = base + 4;
    qcopy(base + 4, textured_quad_giftag);
    p = (long *)(base + 8);
    g_render_packet_cursor = base + 8;
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
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x60);
}

extern s32 g_gs_texture_allocation_cursor MACRO_ADDR;
extern s32 g_gs_texture_allocation_start MACRO_ADDR;
typedef struct {
    u64 q[12];
} sceGsLoadImage __attribute__((aligned(16)));
struct DiscFile {
    s32 sector;
    s32 size;
};
struct DiscTable {
    u8 pad_0[0x8];
    struct DiscFile debug_font; /* 0x8 */
    struct DiscFile memcard_data; /* 0x10, read by memcard_update_state and memcard_restore_game */
    struct DiscFile animation_streams[49]; /* 0x18, streamed animations by archive index */
    struct DiscFile music_10000[37]; /* 0x1A0, music tracks 10000.. */
    struct DiscFile unk2C8[6]; /* 0x2C8, one per language */
    struct DiscFile unk2F8[6]; /* 0x2F8, one per language */
    u8 pad_328[0x1D0];
    struct DiscFile level_chunk; /* 0x4F8, read by load_level_chunk_from_disc */
    u8 pad_500[0x3B8];
    /* 0x8B8: render archive sector/length records; extent reaches the next
       known field, not a proven count of valid archives. */
    struct DiscFile render_archives[201];
    s32 music_50000[40][6]; /* 0xF00, tracks 50000.., one location per language */
    struct DiscFile unk12C0; /* 0x12C0 */
    struct DiscFile level_archives[24]; /* 0x12C8, by level index */
    struct DiscFile loading_slides[6]; /* 0x1388, one per language */
    struct DiscFile unk13B8; /* 0x13B8 */
    struct DiscFile music_40000[36]; /* 0x13C0, music tracks 40000.. */
    s32 sound_bank; /* 0x14E0, passed to load_audio_bank_by_location */
    s32 pad_14E4;
    struct DiscFile unk14E8; /* 0x14E8 */
    u8 pad_14F0[0x8];
    struct DiscFile wad_chunks[6]; /* 0x14F8, level wad chunks */
    struct DiscFile help_text; /* 0x1528, per-language archive read by update_menu_help_text_load */
    struct DiscFile unk1530[14]; /* 0x1530, overlays: x of an x/y pair picked by D_0015ED80 */
    struct DiscFile unk15A0[14]; /* 0x15A0, the y half of the 0x1530 pair */
    struct DiscFile animation_table; /* 0x1610, read with animation_asset_read_active set */
    struct DiscFile music_60000[62]; /* 0x1618, music tracks 60000.. */
    struct DiscFile unk1808[19]; /* 0x1808, overlays: x of an x/y pair picked by D_0015ED80 */
    struct DiscFile unk18A0[19]; /* 0x18A0, the y half of the 0x1808 pair */
    struct DiscFile movies[12]; /* 0x1938, level transition movies */
    struct DiscFile movies_alt[12]; /* 0x1998, picked by the video mode flag */
    struct DiscFile unk19F8[3]; /* 0x19F8, overlays: x/y pair picked by D_0015ED80 */
    struct DiscFile unk1A10[3]; /* 0x1A10, the y half of the 0x19F8 pair */
    struct DiscFile unk1A28[5]; /* 0x1A28, overlays: x/y pair picked by D_0015ED80 */
    struct DiscFile unk1A50[5]; /* 0x1A50, the y half of the 0x1A28 pair */
    struct DiscFile start_movies[4]; /* 0x1A78, played by start_level */
    struct DiscFile start_movies_alt[4]; /* 0x1A98, while D_0015ED80 is set */
    u8 pad_1AB8[0xEB0];
    struct DiscFile shared_archive; /* 0x2968 */
    struct DiscFile sound_archive; /* 0x2970 */
    struct DiscFile sound_archive_alt; /* 0x2978, while D_0015ED80 is set */
    u8 pad_2980[0x8];
    struct DiscFile music_20000[36]; /* 0x2988, music tracks 20000.. */
    s32 music_tracks[1]; /* 0x2AA8, by track number */
};
extern struct DiscTable disc_table;
struct CommonArchiveMemory {
    u8 pad_0[0x14];
    s32 archive_base;
};
extern s32 g_gs_texture_allocation_base MACRO_ADDR;
extern struct CommonArchiveMemory level_resource_buffers;
extern void flush_cache(s32 a0);
extern s32 sce_cd_sync_s_r(s32 a0) __asm__("func_00120F30");
extern s32 sceGsExecLoadImage(sceGsLoadImage *load_image, s32 image_address);
extern s32 sceGsSetDefLoadImage(sceGsLoadImage *load_image, s32 block_offset, s32 buffer_width, s32 pixel_storage_format, s32 x, s32 y, s32 width, s32 height);
extern s32 wait_for_graphics_pipeline_idle_r(s32 a0, s32 a1) __asm__("func_00120858");
extern s32 fast_decompress(s32 source_address, s32 destination_address);
extern s32 Stream_SubmitRead(void *a0, u32 a1, u32 a2);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/textbin/fun_00231878.c, prepare_loading_slide_textures. */
void prepare_loading_slide_textures(s32 language_index, s32 first_slide, s32 second_slide,
                                    u64 *shared_texture, u64 *first_texture, u64 *second_texture) {
    s32 texture_bases[6];
    sceGsLoadImage load_image;
    u64 *first_output;
    u64 *second_output;
    s32 upload_index;
    s32 *texture_base_output;
    s32 upload_bytes;
    s32 image_address;
    s32 archive_base;
    struct CommonArchiveMemory *archive_memory;

    first_output = first_texture;
    second_output = second_texture;
    archive_memory = &level_resource_buffers;
    upload_index = 0;
    Stream_SubmitRead((void *)(archive_memory->archive_base + 0x100000),
                           disc_table.loading_slides[language_index].sector,
                           disc_table.loading_slides[language_index].size);
    sce_cd_sync_s_r(0);
    flush_cache(0);
    fast_decompress(archive_memory->archive_base + 0x100000, archive_memory->archive_base);
    flush_cache(0);
    archive_base = archive_memory->archive_base;
    g_gs_texture_allocation_cursor = g_gs_texture_allocation_base;
    g_gs_texture_allocation_start = g_gs_texture_allocation_base;
    texture_base_output = texture_bases;
    for (upload_index = 0; upload_index < 6; upload_index++) {
        if (upload_index == 0) {
            sceGsSetDefLoadImage(&load_image, (g_gs_texture_allocation_cursor << 8) >> 16, 1, 0, 0, 0,
                                 0x10, 0x10);
            upload_bytes = 0x400;
            image_address = (s32)((u8 *)archive_base + *(s32 *)((u8 *)archive_base + 4)) + 0x20;
        } else if (upload_index == 1) {
            sceGsSetDefLoadImage(&load_image, (g_gs_texture_allocation_cursor << 8) >> 16, 1, 0x13, 0,
                                 0, 0x40, 0x40);
            upload_bytes = 0x1000;
            image_address = (s32)((u8 *)archive_base + *(s32 *)((u8 *)archive_base + 4)) + 0x420;
        } else if (upload_index == 2) {
            sceGsSetDefLoadImage(&load_image, (g_gs_texture_allocation_cursor << 8) >> 16, 1, 0, 0, 0,
                                 0x10, 0x10);
            upload_bytes = 0x400;
            image_address =
                (s32)((u8 *)archive_base + *(s32 *)((u8 *)archive_base + first_slide * 4 + 8)) +
                0x20;
        } else if (upload_index == 3) {
            sceGsSetDefLoadImage(&load_image, (g_gs_texture_allocation_cursor << 8) >> 16, 8, 0x13, 0,
                                 0, 0x200, 0x40);
            upload_bytes = 0x8000;
            image_address =
                (s32)((u8 *)archive_base + *(s32 *)((u8 *)archive_base + first_slide * 4 + 8)) +
                0x420;
        } else if (upload_index == 4) {
            sceGsSetDefLoadImage(&load_image, (g_gs_texture_allocation_cursor << 8) >> 16, 1, 0, 0, 0,
                                 0x10, 0x10);
            upload_bytes = 0x400;
            image_address =
                (s32)((u8 *)archive_base + *(s32 *)((u8 *)archive_base + second_slide * 4 + 8)) +
                0x20;
        } else {
            sceGsSetDefLoadImage(&load_image, (g_gs_texture_allocation_cursor << 8) >> 16, 8, 0x13, 0,
                                 0, 0x200, 0x40);
            upload_bytes = 0x8000;
            image_address =
                (s32)((u8 *)archive_base + *(s32 *)((u8 *)archive_base + second_slide * 4 + 8)) +
                0x420;
        }
        flush_cache(0);
        sceGsExecLoadImage(&load_image, image_address);
        wait_for_graphics_pipeline_idle_r(0, 0);
        *texture_base_output = g_gs_texture_allocation_cursor >> 8;
        g_gs_texture_allocation_cursor = g_gs_texture_allocation_cursor + upload_bytes;
        texture_base_output++;
    }
    /* Each TEX0 combines an image base with its paired palette base. */
    {
        u64 texture_bits = ((u64)texture_bases[0] << 37) | (0xB000ULL << 19);
        u64 image_bits = (u64)(texture_bases[1] | 0x19304000);
        *shared_texture = (image_bits | texture_bits) | (1ULL << 63);
    }
    {
        u64 image_bits = (u64)(texture_bases[3] | 0x25320000);
        u64 texture_bits = ((u64)texture_bases[2] << 37) | (0xB000ULL << 19);
        *first_output = (image_bits | texture_bits) | (1ULL << 63);
    }
    {
        u64 image_bits = (u64)(texture_bases[5] | 0x25320000);
        u64 texture_bits = ((u64)texture_bases[4] << 37) | (0xB000ULL << 19);
        *second_output = (image_bits | texture_bits) | (1ULL << 63);
    }
}

typedef struct {
    u8 pad_0[0xC];
    s32 half_height;
} ScreenOfs_32EF0;
extern u8 second_clear_packet[];
/* The memory card state: only the two words the loop tests (PAL offsets, 8 further than the US build). */
struct MemoryCardState_32EF0 {
    u8 pad_0[0xDC];
    s32 state; /* 0xDC */
    s32 pad_E0;
    s32 pending_state; /* 0xE4 */
};
extern struct MemoryCardState_32EF0 memory_card_state_r2 __asm__("D_0013D390");
struct DmaTag_32EF0 {
    u32 tag;
    u32 addr;
    u32 vif0;
    u32 vif1;
};
struct GifTag_32EF0;
union PacketCursor_32EF0 {
    struct DmaTag_32EF0 *tag;
    struct GifTag_32EF0 *gif;
    s32 *words;
    u8 *bytes;
    s32 addr;
};
extern union PacketCursor_32EF0 g_render_packet_cursor_r __asm__("D_00161000") MACRO_ADDR;
extern ScreenOfs_32EF0 g_screen_extent_r2 __asm__("D_0013E600");
/* Retail stores both with `lui $at`: unsized, or the two-byte size makes them small data. */
extern s16 level_archive_load_state_r[] __asm__("D_0015EF48") MACRO_ADDR;
extern s16 level_archive_load_busy_r[] __asm__("D_0015EF4A") MACRO_ADDR;
extern void read_file_entry_with_retry_r(s32) __asm__("func_0012F4A8");
extern void ResetGsRegisters(void);
extern void ResetGsRegistersPr(void);
extern void FadeToBlack(s32);
extern void DrawTexturedQuad_r(s32, s32, s32, s32, s32, s32, s32, s32, u64, u64) __asm__("func_001F5800");
extern f32 IntToFloat(s32);
extern void PutDrawBufferSmall(void);
extern s32 service_level_archive_load(void);
extern void Menu_RunStateHandler(void);
extern void memcard_update_state(void);
extern void append_scrolling_textured_quad_r(s32, s32, s32, s32, u64, u64, f32, f32, f32, f32) __asm__("func_00232A00");
extern void prepare_loading_slide_textures(s32, s32, s32, u64 *, u64 *, u64 *);
extern void VU1_initChain(void);
extern void VU1_swapChain(void);
extern void VU1_sendChain(void);
extern void VU1_syncChain(s32);
extern void VU1_addGSregister_r2(s32, u64) __asm__("func_00234C98");
extern void sce_gs_sync_v(s32);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/textbin/play_level_loading_slides.c, play_level_loading_slides. */
void play_level_loading_slides(s32 language_index, s32 first_slide, s32 second_slide,
                               s32 duration_ticks, s32 preload_level) {
    u64 shared_texture;
    u64 first_texture;
    u64 second_texture;
    s32 frame;
    s32 alpha;
    s32 fade_in_alpha;
    f32 scroll_offset;
    f32 scroll_start;
    f32 scroll_end;

    prepare_loading_slide_textures(language_index, first_slide, second_slide, &shared_texture,
                                   &first_texture, &second_texture);
    if (preload_level != 0) {
        read_file_entry_with_retry_r((*(s32 *)&g_current_level_index));
        level_archive_load_state_r[0] = 0;
        level_archive_load_busy_r[0] = 0;
    }
    sce_gs_sync_v(0);
    VU1_initChain();
    for (frame = 0; frame < duration_ticks && memory_card_state_r2.state < 3 && memory_card_state_r2.pending_state < 0;
         frame++) {
        alpha = 0x80;
        ResetGsRegisters();
        PutDrawBufferSmall();
        VU1_addGSregister_r2(1, (u64)0x8000 << 16);
        VU1_addGSregister_r2(8, 0);
        g_render_packet_cursor_r.words[0] = 0x30000014;
        fade_in_alpha = frame * 4;
        if (frame <= 0x1F) {
            alpha = fade_in_alpha;
        }
        g_render_packet_cursor_r.words[1] = (s32)second_clear_packet;
        g_render_packet_cursor_r.words[2] = 0;
        g_render_packet_cursor_r.words[3] = 0x50000014;
        g_render_packet_cursor_r.words += 4;
        if (duration_ticks - 0x10 < frame) {
            alpha = (duration_ticks - frame) * 8;
        }
        scroll_offset = IntToFloat(frame % 600) * 0.0016666667f;
        if (first_slide == second_slide) {
            append_scrolling_textured_quad_r(0, g_screen_extent_r2.half_height - 0x20, 0x200, 0x40,
                                           (alpha << 24) | 0x808080, shared_texture, 0.0f, 4.0f,
                                           scroll_offset + 0.0f, scroll_offset + 0.4f);
            DrawTexturedQuad_r(0, g_screen_extent_r2.half_height - 0x20, 0x200, 0x40, 0, 0, 0x200, 0x40,
                               0x80808080, first_texture);
        } else {
            scroll_start = scroll_offset + 0.0f;
            scroll_end = scroll_offset + 0.4f;
            append_scrolling_textured_quad_r(0, g_screen_extent_r2.half_height - 0x2E, 0x200, 0x40,
                                           (alpha << 24) | 0x808080, shared_texture, 0.0f, 4.0f,
                                           scroll_start, scroll_end);
            DrawTexturedQuad_r(0, g_screen_extent_r2.half_height - 0x2E, 0x200, 0x40, 0, 0, 0x200, 0x40,
                               0x80808080, first_texture);
            if (frame > 0x40) {
                if (frame < 0x60) {
                    alpha = (frame - 0x40) * 4;
                }
                append_scrolling_textured_quad_r(0, g_screen_extent_r2.half_height, 0x200, 0x40,
                                               (alpha << 24) | 0x808080, shared_texture, 0.0f, 4.0f,
                                               scroll_start, scroll_end);
                DrawTexturedQuad_r(0, g_screen_extent_r2.half_height, 0x200, 0x40, 0, 0, 0x200, 0x40,
                                   0x80808080, second_texture);
            }
        }
        memcard_update_state();
        Menu_RunStateHandler();
        VU1_syncChain(1);
        sce_gs_sync_v(0);
        ResetGsRegistersPr();
        VU1_sendChain();
        VU1_swapChain();
        if (preload_level != 0) {
            if (service_level_archive_load() == 0) {
                if (duration_ticks < frame + 0x14) {
                    duration_ticks = frame + 0x14;
                }
            } else {
                preload_level = 0;
            }
        }
    }
    FadeToBlack(2);
}

typedef struct {
    int pad0[4];
    unsigned int f10;
} Blk4200;
extern Blk4200 coll_output_b __asm__("D_00194200");
typedef struct {
    unsigned char pad0[8];
    unsigned char f8;
    unsigned char pad9[5];
    unsigned char fE;
    unsigned char fF;
} Blk3E48;
extern Blk3E48 level_available_b __asm__("D_0013DE48");
extern unsigned char level_visit_state[];
typedef struct {
    char pad0[0xB];
    char fB;
} Blk17D0;
extern Blk17D0 cd_read_state_b __asm__("D_001517D0");
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
extern Blk8CE00 view_context_b __asm__("D_0018CE00");
typedef struct {
    char pad0[0xDC];
    int fDC;
    char padE0[4];
    int fE4;
} Blk3D390;
extern Blk3D390 memory_card_state_b __asm__("D_0013D390");

extern char *level_sound_defs MACRO_ADDR;
extern int game_frame_counter MACRO_ADDR;
extern int sound_bank_handle_m __asm__("D_0015EE5C") MACRO_ADDR;
extern short D_0015EF48_m SDATA(D_0015EF48);
extern int level_archive_load_busy_i __asm__("D_0015EF4A") MACRO_ADDR;
extern char unloaded_sound_bank_format[];
extern int sce_gs_sync_v_i(int) __asm__("func_00122598");
extern int snd_StreamSafeCdSync_i(int) __asm__("func_0012EF48");
extern int sce_cd_sync_s_i(int) __asm__("func_00120F30");

extern void snd_set_reverb_ex(int, int, int, int, int);
extern void sound_StopAllSounds(void);
extern void music_Stop(void);
extern void snd_UnloadBank(int);
extern void SetBackgroundColor(int, int, int);
extern int load_level_chunk_from_disc(void);
extern void DMAC_VIF1_Disable(void);
extern void play_level_loading_slides(int, int, int, int, int);
extern void initialize_level_runtime(void);
extern int read_file_entry_with_retry(int);
extern void append_palette_transfer_packet(void);
extern void UpdatePad(void);
extern void update_resident_gameplay_state(void);
extern void tick_space_transition(void);
extern int service_level_archive_load(void);
extern void VU1_syncChain(int);

/* DoSpaceTransition: plays the level-to-level transition (dialogue screens, loading loop) and waits for the load. Adapted from Lombyte's project (GPL-3.0) for PAL: src/gameplay/state/do_space_transition.c, do_space_transition. */
void DoSpaceTransition(void) {
    int lvl;
    int ok;
    int done;

    lvl = g_game_language - 1;
    if (lvl < 0) {
        lvl = 0;
    }
    coll_output_b.f10 |= 0x80000000;
    g_game_mode = 6;
    level_render_state.set = 0;
    if (level_available_b.f8 != 0 || space_load_level >= 8) {
        level_render_state.set = 1;
    }
    if (level_available_b.fE != 0 || space_load_level >= 14) {
        level_render_state.set = 2;
    }
    flush_cache(0);
    snd_set_reverb_ex(2, 0, 0, 0, 0);
    snd_reset_state_and_flush_commands();
    snd_FlushSoundCommands();
    sound_StopAllSounds();
    music_Stop();
    cd_read_state_b.fB = 1;
    if (level_sound_defs != 0) {
        snd_UnloadBank(*(int *)(level_sound_defs + 0x1C));
        snd_ResolveBankXREFS();
        STUB_printf(unloaded_sound_bank_format, *(int *)(level_sound_defs + 0x1C));
    }
    sound_bank_handle_m = 0;
    snd_StreamSafeCdCallback(0);
    snd_StreamSafeCdSync_i(0);
    view_context_b.f238 = 16;
    view_context_b.f21C = 524288.0f;
    view_context_b.f228 = 255.0f;
    view_context_b.f230 = 0;
    view_context_b.f234 = 0;
    view_context_b.f218 = 0;
    view_context_b.f22C = 128.0f;
    SetBackgroundColor(0, 0, 0);
    if (space_load_level < 0) {
        while (memory_card_state_b.fDC >= 3 || memory_card_state_b.fE4 >= 0) {
            memcard_update_state();
            Menu_RunStateHandler();
        }
        FadeToBlack(scale_game_frame_count(6));
        g_current_level_index_m = space_load_level;
        load_level_chunk_from_disc();
        sce_cd_sync_s_i(0);
        cd_read_state_b.fB = 0;
        DMAC_VIF1_Disable();
        return;
    }
    if (space_load_level == 0 && level_visit_state[0] == 0) {
        FadeToBlack(scale_game_frame_count(6));
        play_level_loading_slides(lvl, 0, 1, scale_game_frame_count(240), 0);
        play_level_transition_movie(0);
        play_level_loading_slides(lvl, 2, 2, scale_game_frame_count(180), 0);
        play_level_transition_movie(1);
        g_current_level_index_m = space_load_level;
        play_level_loading_slides(lvl, 3, 4, scale_game_frame_count(240), 1);
        play_level_transition_movie(2);
    } else if (g_current_level_index_m == 0 && space_load_level == 1 && level_visit_state[1] == 0) {
        FadeToBlack(scale_game_frame_count(6));
        play_level_loading_slides(lvl, 5, 6, scale_game_frame_count(240), 0);
        play_level_transition_movie(3);
        play_level_transition_movie(4);
        play_level_loading_slides(lvl, 7, 7, scale_game_frame_count(180), 0);
        play_level_transition_movie(5);
        level_visit_state[g_current_level_index_m] = 2;
        g_current_level_index_m = space_load_level;
        play_level_loading_slides(lvl, 8, 8, scale_game_frame_count(240), 1);
    } else {
        if (space_load_level == 4 && level_visit_state[4] == 0) {
            FadeToBlack(scale_game_frame_count(12));
            play_level_loading_slides(lvl, 9, 10, scale_game_frame_count(240), 0);
            play_level_transition_movie(6);
        }
        if (g_current_level_index_m == 7 && level_visit_state[7] != 2 && level_available_b.f8 != 0) {
            FadeToBlack(scale_game_frame_count(12));
            play_level_loading_slides(lvl, 11, 11, scale_game_frame_count(240), 0);
            play_level_transition_movie(7);
        }
        if (space_load_level == 13 && level_visit_state[13] == 0) {
            FadeToBlack(scale_game_frame_count(12));
            play_level_loading_slides(lvl, 12, 13, scale_game_frame_count(240), 0);
            play_level_transition_movie(8);
        }
        if (g_current_level_index_m == 14 && level_visit_state[14] != 2 && level_available_b.fF != 0) {
            FadeToBlack(scale_game_frame_count(12));
            play_level_loading_slides(lvl, 14, 14, scale_game_frame_count(240), 0);
            play_level_transition_movie(9);
        }
        if (space_load_level == 16 && level_visit_state[16] == 0) {
            FadeToBlack(scale_game_frame_count(12));
            play_level_loading_slides(lvl, 15, 16, scale_game_frame_count(240), 0);
            play_level_transition_movie(10);
        }
        if ((unsigned int)g_current_level_index_m < 19) {
            ok = 1;
            if (g_current_level_index_m == 7 && level_available_b.f8 == 0) {
                ok = 0;
            }
            if (g_current_level_index_m == 14 && level_available_b.fF == 0) {
                ok = 0;
            }
            if (ok) {
                level_visit_state[g_current_level_index_m] = 2;
            }
        }
        *(short *)&level_archive_load_busy_i = 1;
        done = 0;
        g_current_level_index_m = space_load_level;
        D_0015EF48_m = 0;
        initialize_level_runtime();
        read_file_entry_with_retry(g_current_level_index_m);
        while (space_load_in_progress_m == 0) {
            VU1_sendChain();
            VU1_swapChain();
            PutDrawBufferSmall();
            append_palette_transfer_packet();
            PutDrawBufferLarge();
            UpdatePad();
            update_resident_gameplay_state();
            dispatch_game_state_update();
            memcard_update_state();
            Menu_RunStateHandler();
            VU1_syncChain(1);
            sce_gs_sync_v_i(0);
            game_frame_counter++;
            tick_space_transition();
            if (!done) {
                done = service_level_archive_load();
            }
        }
        if (!done) {
            do {
                flush_cache(0);
                sce_gs_sync_v_i(0);
                memcard_update_state();
                Menu_RunStateHandler();
                tick_space_transition();
            } while (service_level_archive_load() == 0);
        }
        while (memory_card_state_b.fDC != 2 || memory_card_state_b.fE4 >= 0) {
            flush_cache(0);
            sce_gs_sync_v_i(0);
            memcard_update_state();
            Menu_RunStateHandler();
            tick_space_transition();
        }
    }
    sce_cd_sync_s_i(0);
    cd_read_state_b.fB = 0;
    DMAC_VIF1_Disable();
}

typedef u32 u128_33AB8 __attribute__((mode(TI), aligned(16)));
typedef float FloatVector4[4] __attribute__((aligned(16)));
typedef struct {
    s16 vertex_index;
    s16 pad;
} QuadCornerIndex;
typedef struct {
    QuadCornerIndex corners[4];
} IndexedQuad;
typedef struct {
    u8 pad0[0x10];
    float position_x;
    float position_y;
    u8 pad18[0x8E];
    s16 class_id;
} EnvironmentMappedObject;
extern s32 ship_substate_r[] __asm__("D_0013E150");
extern s32 g_game_mode MACRO_ADDR;
extern short env_map_first_frame;
extern short env_map_was_active;
extern short env_map_blend_timer;
extern short environment_mesh_colors;
extern short environment_mesh_vertex_counts;
extern short environment_mesh_quad_counts;
extern short environment_mesh_normals;
extern short environment_mesh_positions;
extern short environment_mesh_quads;
extern CameraSystem gCamera_r __asm__("D_00187040");
extern FloatVector4 camera_position_r __asm__("D_00187180");
extern Vec4 env_map_world_positions[];
extern float env_map_uvs[][2];
extern float env_map_saved_uvs[][2];
extern u64 GetEffectTex_r2(int) __asm__("func_001F4868");
extern void fast_dec_timer(s32 *);
extern float FastAbsF(float);
extern float FastVecDot(void *, void *);
extern float IntToFloat(int);
extern void MobyTransformScaledVertex(EnvironmentMappedObject *, int, void *);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/rendering/render_environment_mapped_object.c, render_environment_mapped_object. */
void render_environment_mapped_object(EnvironmentMappedObject *object) {
    Vec4 quad_positions[4];
    int colors[4];
    float texture_coordinates[4][2];
    u64 quad_state[4];
    FloatVector4 object_transform[4];
    FloatVector4 normal;
    FloatVector4 reflection;
    FloatVector4 view_direction;
    IndexedQuad *indexed_quads;
    Vec4 *positions;
    Vec4 *normals;
    int quad_count;
    int vertex_count;
    int class_index;
    int color;
    int mapping_enabled;
    float transition_fraction;
    float sphere_denominator;
    float coordinate;
    int element_index;
    int corner_index;

    class_index = object->class_id - 0x212;
    positions = (((Vec4 * *)&environment_mesh_positions))[class_index];
    normals = (((Vec4 * *)&environment_mesh_normals))[class_index];
    indexed_quads = (((IndexedQuad * *)&environment_mesh_quads))[class_index];
    quad_count = (((s32 *)&environment_mesh_quad_counts))[class_index];
    vertex_count = (((s32 *)&environment_mesh_vertex_counts))[class_index];
    if (g_game_mode == 6 && ship_substate_r[0] == 4) {
        quad_state[1] = GetEffectTex_r2(1);
    } else {
        quad_state[1] = GetEffectTex_r2(0x15);
    }
    mapping_enabled = 0;
    color = (((s32 *)&environment_mesh_colors))[class_index];
    quad_state[2] = 0xFF9000000260;
    quad_state[3] = 0x8000000044;
    quad_state[0] = 0;
    colors[3] = color;
    colors[2] = color;
    colors[1] = color;
    colors[0] = color;
    MobyTransformScaledVertex(object, 0, object_transform);
    if (g_game_mode != 0 || (FastAbsF(gCamera_r.pos.f[0] - object->position_x) < 16.0f &&
                            FastAbsF(gCamera_r.pos.f[1] - object->position_y) < 16.0f)) {
        mapping_enabled = 1;
    }
    if (g_game_mode == 6 && ship_substate_r[0] == 4) {
        mapping_enabled = 0;
    }
    if (mapping_enabled != 0 || (*(s32 *)&env_map_first_frame) == 1) {
        (*(s32 *)&env_map_was_active) = 1;
        fast_dec_timer(&(*(s32 *)&env_map_blend_timer));
        transition_fraction =
            IntToFloat((*(s32 *)&env_map_blend_timer)) / IntToFloat(scale_game_frame_count(0x3C));
        for (element_index = 0; element_index < vertex_count; element_index++) {
            transform_vector(&env_map_world_positions[element_index], &positions[element_index],
                             object_transform);
            FastVecSub(view_direction, &env_map_world_positions[element_index], camera_position_r);
            fast_vec_normalize(view_direction, view_direction, 1.0f);
            transform_vector(normal, &normals[element_index], object_transform);
            fast_vec_normalize(normal, normal, 0.1f);
            FastVecScale(reflection, normal, FastVecDot(normal, view_direction) * 2.0f);
            FastVecSub(reflection, view_direction, reflection);
            fast_vec_normalize(reflection, reflection, 1.0f);
            reflection[2] += 1.0f;
            sphere_denominator = vec_length(reflection[2] * 2.0f) * 2.0f;
            if ((*(s32 *)&env_map_first_frame) == 1 || (*(s32 *)&env_map_blend_timer) == 0) {
                env_map_uvs[element_index][0] = reflection[0] / sphere_denominator + 0.5f;
                env_map_uvs[element_index][1] = reflection[1] / sphere_denominator + 0.5f;
            } else {
                coordinate = reflection[0] / sphere_denominator + 0.5f;
                env_map_uvs[element_index][0] =
                    coordinate + (env_map_saved_uvs[element_index][0] - coordinate) * transition_fraction;
                coordinate = reflection[1] / sphere_denominator + 0.5f;
                env_map_uvs[element_index][1] =
                    coordinate + (env_map_saved_uvs[element_index][1] - coordinate) * transition_fraction;
            }
        }
        if ((*(s32 *)&env_map_first_frame) == 1) {
            (*(s32 *)&env_map_first_frame) = 2;
        }
    } else {
        if ((*(s32 *)&env_map_was_active) == 1) {
            (*(s32 *)&env_map_was_active) = 0;
            for (element_index = 0; element_index < vertex_count; element_index++) {
                env_map_saved_uvs[element_index][0] = env_map_uvs[element_index][0];
                env_map_saved_uvs[element_index][1] = env_map_uvs[element_index][1];
                transform_vector(&env_map_world_positions[element_index], &positions[element_index],
                                 object_transform);
            }
        } else {
            for (element_index = 0; element_index < vertex_count; element_index++) {
                transform_vector(&env_map_world_positions[element_index], &positions[element_index],
                                 object_transform);
            }
        }
        (*(s32 *)&env_map_blend_timer) = scale_game_frame_count(0x3C);
    }
    for (element_index = 0; element_index < quad_count; element_index++) {
        for (corner_index = 0; corner_index < 4; corner_index++) {
            int vertex_index = indexed_quads[element_index].corners[corner_index].vertex_index;

            qcopy(&quad_positions[corner_index], &env_map_world_positions[vertex_index]);
            texture_coordinates[corner_index][0] = env_map_uvs[vertex_index][0];
            texture_coordinates[corner_index][1] = env_map_uvs[vertex_index][1];
        }
        draw_world_quad_packet(quad_positions, 0, 0);
    }
}
