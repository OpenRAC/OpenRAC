#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * skyfunc.cpp in the original source; text 0x22BEB0-0x22CEB8.
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

LINKER_REMNANT("asm/remnants/text", func_0022BEB0);

/* Sky shell draw loop (a variant of func_0022C5A0): shell 0 re-reads the
   base transform; shells 1-5 scroll their float by D_00160500 times a
   per-shell rate, get a per-shell spin and scale, rebuild the sky transform
   from D_001D9A70, scale its three rows, copy D_00160560 into the
   translation, and every shell is drawn by func_0022C9A8. Each case does
   its own add and store, and the shell table is indexed through its own
   pointer, which gives retail's base-first address add. */
extern char *level_sky_effect_data_r __asm__("D_0016055C") MACRO_ADDR;
extern f32 D_00160500_2BEB0 SDATA(D_00160500);
extern u8 sky_matrix_r[] __asm__("D_001D9A70");
extern u8 sky_shell_translation_r[] __asm__("D_00160560") MACRO_ADDR;
extern void set_identity_matrix_r(void *) __asm__("func_001FA190");
extern void clear_u64_value_r(void *) __asm__("func_001F9BC0");
extern void euler_to_matrix_ee_r(void *, void *) __asm__("func_001FA238");
extern void scale_vector_r(void *, void *, f32) __asm__("func_001F9C48");
extern void SkyDrawShell_r(s32) __asm__("func_0022C9A8");

/* Draw the sky shells, each layer scrolling and rotating at its own rate. */
void DrawSkyShellsScrolling(void) {
    f32 rot[4];
    f32 scale;
    f32 *p;
    f32 d;
    u8 *t;
    u8 **tab;
    s32 i;

    *(s16 *)(level_sky_effect_data_r + 4) = 0;
    set_identity_matrix_r(sky_matrix_r);
    clear_u64_value_r(rot);
    for (i = 0; i < *(s16 *)(level_sky_effect_data_r + 6); i++) {
        tab = (u8 **)(level_sky_effect_data_r + 0x20);
        p = (f32 *)(tab[i] + 8);
        scale = 1.0f;
        switch (i) {
        case 1:
            rot[1] = 0.02f;
            scale = 3.0f;
            *p += D_00160500_2BEB0 * 0.55f;
            break;
        case 2:
            rot[1] = -0.02f;
            scale = 2.5f;
            *p += D_00160500_2BEB0 * 0.6f;
            break;
        case 3:
            rot[1] = 0.01f;
            scale = 2.0f;
            *p += D_00160500_2BEB0 * 0.7f;
            break;
        case 4:
            rot[1] = -0.01f;
            scale = 1.5f;
            *p += D_00160500_2BEB0 * 0.75f;
            break;
        case 5:
            scale = 1.25f;
            *p += D_00160500_2BEB0 * 0.8f;
            break;
        default:
            set_identity_matrix_r(sky_matrix_r);
            break;
        }
        if (i > 0) {
            f32 v = (f32)i * 2000.0f;
            s32 n = (s32)(*p + v);
            rot[2] = (f32)(n % 10000) * 0.00062831853f - 3.1415927f;
            euler_to_matrix_ee_r(sky_matrix_r, rot);
            t = sky_matrix_r;
            scale_vector_r(t, t, scale);
            scale_vector_r(t + 0x10, t + 0x10, scale);
            scale_vector_r(t + 0x20, t + 0x20, scale);
            qcopy(t + 0x30, sky_shell_translation_r);
        }
        SkyDrawShell_r(i);
    }
}

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
extern struct LevelSkyEffectData * level_sky_effect_data_r2 __asm__("D_0016055C") MACRO_ADDR;
extern u8 sky_matrix[];
extern void srand(s32);
extern f32 FastAbsF(f32);
extern f32 FastCos(f32);
extern f32 FastSin(f32);
extern void set_identity_matrix(u8 *);
extern f32 FastAddRots(f32, f32);
extern f32 IntToFloat(s32);
extern s32 random_integer_below(s32);
extern f32 random_angle_radians();
extern void SkyDrawShell(s32);
extern void SkySpriteProc();
extern void VU1_addGSregister_r(s32, u64) __asm__("func_00234C98");
extern s32 func_001160D8();

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/rendering/sky/update_sky_effects.c, update_sky_effects. */
void SkyLevelGeneric___maybe(void) {
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

    level_sky_effect_data_r2->relocation_state = 0;
    set_identity_matrix(sky_matrix);
    SkyDrawShell(0);
    SkyDrawShell(1);
    if (level_sky_effect_data_r2->effect_count == 0) {
        level_sky_effect_data_r2->effect_count = 0x100;
        srand(0x3039);
        for (initialization_index = 0; initialization_index < level_sky_effect_data_r2->effect_count;
             initialization_index++) {
            radius = 50.0f;
            random_color_enabled = 1;
            effect_flags = 0x48;
            base_color = 0x30505050;
            effect = &level_sky_effect_data_r2->effects[initialization_index];
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
                color_delta = random_integer_below(0x100);
                effect->texture_index = random_color_enabled;
                effect->flags = effect_flags;
                effect->state.angles.azimuth = color_delta;
                effect->angle = random_angle_radians();
                effect->size =
                    IntToFloat(random_integer_below(0x18) + 0x20) * 0.00390625f;
                azimuth = FastAddRots(-3.0f, random_angle_radians() * 0.2f);
                elevation = random_angle_radians();
                elevation = elevation * 0.09f;
                elevation = elevation + 1.2f;
                trig_product = FastCos(azimuth);
                trig_product = trig_product * FastSin(elevation);
                trig_product = trig_product * radius;
                effect->position_x = trig_product;
                trig_product = FastSin(azimuth);
                trig_product = trig_product * FastSin(elevation);
                trig_product = trig_product * radius;
                effect->position_y = trig_product;
                effect->position_z = FastCos(elevation) * radius;
                color_delta = random_integer_below(0x18);
                alpha_delta = random_integer_below(0x20) << 0x18;
                if ((func_001160D8() >> 0x10) & 1) {
                    effect->state.base_color = alpha_delta + ((color_delta << 0x10) + base_color);
                } else {
                    effect->state.base_color =
                        (alpha_delta + ((color_delta << 8) + base_color)) | color_delta;
                }
            }
        }
    }
    for (effect_index = 0; effect_index < level_sky_effect_data_r2->effect_count; effect_index++) {
        effect = &level_sky_effect_data_r2->effects[effect_index];
        if (effect->randomize_color == 0) {
            orbit = &effect->state.angles;
            angles = &effect->state.angles.azimuth;
            effect->state.angles.azimuth = (s16)(effect->state.angles.azimuth + 1);
            angles[1] = (u16)(angles[1] + 1);
            azimuth = IntToFloat((orbit->azimuth & 0xFFF) - 0x800) * 0.0015339808f;
            elevation = IntToFloat((angles[1] & 0xFFF) - 0x800) * 0.0015339808f;
            trig_product = FastCos(azimuth);
            trig_product = trig_product * FastSin(elevation);
            trig_product = trig_product * 50.0f;
            effect->position_x = trig_product;
            trig_product = FastSin(azimuth);
            trig_product = trig_product * FastSin(elevation);
            trig_product = trig_product * 50.0f;
            effect->position_y = trig_product;
            effect->position_z = FastAbsF(FastCos(elevation)) * 50.0f;
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
    SkySpriteProc();
    VU1_addGSregister_r(0x42, (0x8000ULL << 0x18) | 0x44);
    SkyDrawShell(2);
    SkyDrawShell(3);
}

struct SkyShellSet_2C5A0 {
    u8 pad_0[4];
    u16 relocation_state;
    s16 shell_count;
};
struct SkyTransform_2C5A0 {
    u8 pad[0x30];
    u64 translation;
};
extern f32 D_00160504_2C5A0 SDATA(D_00160504);
extern struct SkyShellSet_2C5A0 * level_sky_effect_data_r3 __asm__("D_0016055C") MACRO_ADDR;
extern u8 sky_shell_translation_r2[] __asm__("D_00160560") MACRO_ADDR;
extern struct SkyTransform_2C5A0 sky_matrix_r2 __asm__("D_001D9A70");
extern s32 depth_buffer_address_r __asm__("D_0015EF88") MACRO_ADDR;
extern void clear_u64_value(f32 *);
extern void scale_vector(void *, void *, f32);
extern void set_identity_matrix_r2(void *) __asm__("func_001FA190");
extern void euler_to_matrix_ee(void *, f32 *);
extern void SetupSkyGifPaging(void);
extern void DoSkyGifPaging(void);
extern void VU1_addGSregister_r2(s32, s64) __asm__("func_00234C98");

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/rendering/sky/draw_sky_shells.c, draw_sky_shells. */
void draw_sky_shells(void) {
    f32 rotation_angles[4];
    f32 shell_scale;
    u8 *transform_row;
    s32 shell_index;

    shell_index = 0;
    SetupSkyGifPaging();
    level_sky_effect_data_r3->relocation_state = 0;
    set_identity_matrix_r2(&sky_matrix_r2);
    clear_u64_value(rotation_angles);
    if (level_sky_effect_data_r3->shell_count > 0) {
        do {
            shell_scale = 1.0f;
            switch (shell_index) {
            case 0:
                *(s32 *)&rotation_angles[1] = 0;
                rotation_angles[2] = D_00160504_2C5A0;
            case 1:
                *(s32 *)&rotation_angles[1] = 0;
                rotation_angles[2] = FastAddRots(D_00160504_2C5A0, rotation_angles[1]);
                shell_scale = 1.0f;
                break;
            case 2:
                rotation_angles[1] = -0.075f;
                rotation_angles[2] = FastAddRots(D_00160504_2C5A0, -0.15f);
                shell_scale = 1.25f;
                break;
            case 3:
                rotation_angles[1] = 0.05f;
                rotation_angles[2] = FastAddRots(D_00160504_2C5A0, 0.125f);
                shell_scale = 1.5f;
                break;
            case 4:
                rotation_angles[1] = 0.1f;
                rotation_angles[2] = FastAddRots(D_00160504_2C5A0, -0.05f);
                shell_scale = 1.75f;
                break;
            case 5:
                rotation_angles[1] = -0.15f;
                rotation_angles[2] = FastAddRots(D_00160504_2C5A0, 0.1f);
                shell_scale = 2.0f;
                break;
            }
            euler_to_matrix_ee(&sky_matrix_r2, rotation_angles);
            transform_row = (u8 *)&sky_matrix_r2;
            scale_vector(transform_row, transform_row, shell_scale);
            scale_vector(transform_row + 0x10, transform_row + 0x10, shell_scale);
            scale_vector(transform_row + 0x20, transform_row + 0x20, shell_scale);
            transform_row += 0x30;
            qcopy(transform_row, sky_shell_translation_r2);
            SkyDrawShell(shell_index);
            shell_index++;
        } while (shell_index < level_sky_effect_data_r3->shell_count);
    }
    DoSkyGifPaging();
    VU1_addGSregister_r2(0x47, 0x5360B);
    VU1_addGSregister_r2(0x4E, 0x1000000 | (depth_buffer_address_r >> 13));
}

extern int g_render_packet_cursor MACRO_ADDR;
extern int sky_gif_paging_tag MACRO_ADDR;
extern int g_gs_texture_allocation_start MACRO_ADDR;
extern int g_gs_texture_allocation_cursor MACRO_ADDR;
extern int pending_texture_upload_count MACRO_ADDR;
extern char str_sky_texs[];
extern void ProfileMarker_Begin(void *, int);
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
extern SkyDef *level_sky_effect_data MACRO_ADDR;

/*
 * SetupSkyGifPaging(void): reserves 0x10 bytes of the VU1 chain at
 * D_00160570 and clears the tag of each of the sky's GIF pages. Retail
 * loads the page count before the two global stores: read as a struct
 * field through a pointer it cannot alias a scalar global (gcc's
 * fixed_scalar_and_varying_struct_p), where a cast access would keep it
 * behind them.
 */
void SetupSkyGifPaging(void) {
    int p;
    SkyDef *s;
    int i;

    p = g_render_packet_cursor;
    sky_gif_paging_tag = p;
    p += 0x10;
    g_render_packet_cursor = p;
    ProfileMarker_Begin(str_sky_texs, 1);
    s = level_sky_effect_data;
    g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
    pending_texture_upload_count = 0;
    for (i = 0; i < s->npages; i++) {
        s->pages[i].tag = 0;
    }
}

extern int *g_render_packet_cursor_p __asm__("D_00161000") MACRO_ADDR;
extern int *sky_gif_paging_tag_p __asm__("D_00160570") MACRO_ADDR;
extern int *sky_gif_paging_start MACRO_ADDR;
extern void flush_pending_texture_uploads(void);
extern void VU1_texFlush(void);

/* DoSkyGifPaging: DoGifPaging's twin (func_001F4748) for the sky. Pushes
   two 4-word GIF tags (0x20000000 in the first word) onto the D_00161000
   packet, D_00160574 marking where it started and D_00160570's tag
   pointing at the second; in between, when D_0018A3B0[1] is set,
   func_0020C2F8 and func_00234E80 add their own. Then restores
   D_0015EF74 from D_0015EF78. The first advance goes through a local
   advanced in place, which keeps the old and new pointer in one register
   as retail does. */
void DoSkyGifPaging(void) {
    int *p = g_render_packet_cursor_p;

    sky_gif_paging_start = p;
    p += 4;
    g_render_packet_cursor_p = p;
    sky_gif_paging_tag_p[0] = 0x20000000;
    sky_gif_paging_tag_p[1] = (int)g_render_packet_cursor_p;
    sky_gif_paging_tag_p[2] = 0;
    sky_gif_paging_tag_p[3] = 0;
    if (draw_config[1] != 0) {
        flush_pending_texture_uploads();
        VU1_texFlush();
    }
    g_render_packet_cursor_p[0] = 0x20000000;
    g_render_packet_cursor_p[1] = (int)(sky_gif_paging_tag_p + 4);
    g_render_packet_cursor_p[2] = 0;
    g_render_packet_cursor_p[3] = 0;
    g_render_packet_cursor_p += 4;
    sky_gif_paging_start[0] = 0x20000000;
    sky_gif_paging_start[1] = (int)g_render_packet_cursor_p;
    sky_gif_paging_start[2] = 0;
    sky_gif_paging_start[3] = 0;
    g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

LINKER_REMNANT("asm/remnants/text", func_0022C9A0);

extern void SkyDrawShellTextured(void *);
extern void SkyDrawShellGouraud(void *);

/* Minimal layout for one sky shell: only the flag at +4 is read here. */
struct SkyShell {
    u8 pad_0[4];
    s32 gouraud_enabled;   /* 0x04: nonzero draws the Gouraud path */
};

/* SkyDrawShell(int) */
void SkyDrawShell(s32 shell_index) {
    SkyDef *def = level_sky_effect_data;

    if (shell_index < def->count) {
        struct SkyShell *shell = def->shells[shell_index];
        if (shell->gouraud_enabled != 0) {
            SkyDrawShellGouraud(shell);
        } else {
            SkyDrawShellTextured(shell);
        }
    }
}

typedef u32 u128_2CA00 __attribute__((mode(TI), aligned(16)));
struct DmaTag_2CA00 {
    u32 tag;
    u32 addr;
    u32 vif0;
    u32 vif1;
};
struct GifTag_2CA00;
union PacketCursor_2CA00 {
    struct DmaTag_2CA00 *tag;
    struct GifTag_2CA00 *gif;
    s32 *words;
    u8 *bytes;
    s32 addr;
};
extern union PacketCursor_2CA00 g_render_packet_cursor_r __asm__("D_00161000") MACRO_ADDR;
struct SkyTile_2CA00 {
    u128_2CA00 bounds; /* 16-byte aligned: tile fields are addressed from the tile base */
    s32 address;
    s16 unk4;
    s16 unk6;
    s16 unk8;
    s16 unkA;
    s16 unkC;
    s16 unkE;
};
struct Shell_2CA00 {
    s32 count;
    u8 pad4[0xC];
    struct SkyTile_2CA00 tiles[1];
};
extern short sky_tile_spr_buffers;
extern s32 D_00160510_2CA00 SDATA(D_00160510);
extern u8 sky_textured_gs_block[];
extern void sky_bsphere_check_r(struct SkyTile_2CA00 *tiles, s32 count, u8 *visibility) __asm__("func_0022D7E0");
extern void DmaToSpr_r(s32 address, s32 qwc, s32 destination) __asm__("func_0020C210");
extern void DmaToSprSync_r(void) __asm__("func_0020C230");
extern s32 sky_transform_vertices_r(s32, s32, s32, s32) __asm__("func_0022D2AC");
extern void sky_build_gif_textured_r(s32, s32, s32, s32) __asm__("func_0022D520");

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/rendering/sky/sky_draw_shell_textured.c, sky_draw_shell_textured. */
void func_0022CA00_r(struct Shell_2CA00 *shell) __asm__("func_0022CA00");
/* SkyDrawShellTextured */
void func_0022CA00_r(struct Shell_2CA00 *shell) {
    u8 visibility[shell->count];
    s32 i;
    s32 dest;
    s32 c_dest;
    s32 a_dest;
    s32 next;

    if (shell->count == 0) {
        return;
    }

    sky_bsphere_check_r(shell->tiles, shell->count, visibility);

    g_render_packet_cursor_r.words[0] = 0x30000007;
    *(s32 *)((u32)g_render_packet_cursor_r.words + 4) = (s32)sky_textured_gs_block;
    *(s32 *)((u32)g_render_packet_cursor_r.words + 8) = 0;
    *(s32 *)((u32)g_render_packet_cursor_r.words + 12) = 0x50000007;
    g_render_packet_cursor_r.words += 4;

    D_00160510_2CA00 = 1 - D_00160510_2CA00;
    if (visibility[0] == 1) {
        DmaToSpr_r(shell->tiles[0].address, shell->tiles[0].unkE >> 4,
                                 (((s32 *)&sky_tile_spr_buffers))[D_00160510_2CA00]);
    }

    for (i = 0; i < shell->count; i++) {
        if (visibility[i] == 1) {
            DmaToSprSync_r();
        }

        next = i + 1;
        D_00160510_2CA00 = 1 - D_00160510_2CA00;
        if (next < shell->count && visibility[next] == 1) {
            DmaToSpr_r(shell->tiles[next].address, shell->tiles[next].unkE >> 4,
                                     (((s32 *)&sky_tile_spr_buffers))[D_00160510_2CA00]);
        }

        if (visibility[i] == 1) {
            dest = (((s32 *)&sky_tile_spr_buffers))[1 - D_00160510_2CA00];
            c_dest = dest + shell->tiles[i].unkC;
            a_dest = dest + shell->tiles[i].unkA;
            if (sky_transform_vertices_r(dest + shell->tiles[i].unk8, 0x70002000, shell->tiles[i].unk4,
                             (&shell->tiles[i])->unkC) == 0) {
                sky_build_gif_textured_r(shell->tiles[i].unk6, c_dest, a_dest, 0x70002000);
            }
        }
    }
}

typedef u32 u128_2CC40 __attribute__((mode(TI), aligned(16)));
struct DmaTag_2CC40 {
    u32 tag;
    u32 addr;
    u32 vif0;
    u32 vif1;
};
struct GifTag_2CC40;
union PacketCursor_2CC40 {
    struct DmaTag_2CC40 *tag;
    struct GifTag_2CC40 *gif;
    s32 *words;
    u8 *bytes;
    s32 addr;
};
extern union PacketCursor_2CC40 g_render_packet_cursor_r2 __asm__("D_00161000") MACRO_ADDR;
struct SkyTile_2CC40 {
    u128_2CC40 bounds; /* 16-byte aligned: tile fields are addressed from the tile base */
    s32 address;
    s16 unk4;
    s16 unk6;
    s16 unk8;
    s16 unkA;
    s16 unkC;
    s16 unkE;
};
struct Shell_2CC40 {
    s32 count;
    u8 pad4[0xC];
    struct SkyTile_2CC40 tiles[1];
};
extern s32 D_00160510_2CC40 SDATA(D_00160510);
extern u8 sky_gouraud_gs_block[];
extern void sky_bsphere_check_r2(struct SkyTile_2CC40 *tiles, s32 count, u8 *visibility) __asm__("func_0022D7E0");
extern void DmaToSpr_r2(s32 address, s32 qwc, s32 destination) __asm__("func_0020C210");
extern void DmaToSprSync_r2(void) __asm__("func_0020C230");
extern s32 sky_transform_vertices_r2(s32, s32, s32, s32) __asm__("func_0022D2AC");
extern void sky_build_gif_gouraud_r(s32, s32, s32, s32) __asm__("func_0022D3F8");
extern s32 VU1_addGSregister_r3(s32, s64) __asm__("func_00234C98");

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/rendering/sky/sky_draw_shell_gouraud.c, sky_draw_shell_gouraud. */
void func_0022CC40_r(struct Shell_2CC40 *shell) __asm__("func_0022CC40");
/* SkyDrawShellGouraud */
void func_0022CC40_r(struct Shell_2CC40 *shell) {
    u8 visibility[shell->count];
    s32 i;
    s32 dest;
    s32 c_dest;
    s32 a_dest;
    s32 next;

    if (shell->count == 0) {
        return;
    }

    sky_bsphere_check_r2(shell->tiles, shell->count, visibility);

    g_render_packet_cursor_r2.words[0] = 0x30000007;
    *(s32 *)((u32)g_render_packet_cursor_r2.words + 4) = (s32)sky_gouraud_gs_block;
    *(s32 *)((u32)g_render_packet_cursor_r2.words + 8) = 0;
    *(s32 *)((u32)g_render_packet_cursor_r2.words + 12) = 0x50000007;
    g_render_packet_cursor_r2.words += 4;

    D_00160510_2CC40 = 1 - D_00160510_2CC40;
    if (visibility[0] == 1) {
        DmaToSpr_r2(shell->tiles[0].address, shell->tiles[0].unkE >> 4,
                                 (((s32 *)&sky_tile_spr_buffers))[D_00160510_2CC40]);
    }

    for (i = 0; i < shell->count; i++) {
        if (visibility[i] == 1) {
            DmaToSprSync_r2();
        }

        next = i + 1;
        D_00160510_2CC40 = 1 - D_00160510_2CC40;
        if (next < shell->count && visibility[next] == 1) {
            DmaToSpr_r2(shell->tiles[next].address, shell->tiles[next].unkE >> 4,
                                     (((s32 *)&sky_tile_spr_buffers))[D_00160510_2CC40]);
        }

        if (visibility[i] == 1) {
            dest = (((s32 *)&sky_tile_spr_buffers))[1 - D_00160510_2CC40];
            c_dest = dest + shell->tiles[i].unkC;
            a_dest = dest + shell->tiles[i].unkA;
            if (sky_transform_vertices_r2(dest + shell->tiles[i].unk8, 0x70002000, shell->tiles[i].unk4,
                             (&shell->tiles[i])->unkC) == 0) {
                sky_build_gif_gouraud_r(shell->tiles[i].unk6, c_dest, a_dest, 0x70002000);
            }
        }
    }

    VU1_addGSregister_r3(0x47, 0x3180B);
    VU1_addGSregister_r3(0x4E, (depth_buffer_address_r >> 13) | 0x1000000 | ((s64)1 << 32));
}
