#include "common.h"
#include "vector.h"
#include "callees.h"
#include "records.h"
#include "hero.h"
#include "structs.h"

/*
 * sound.cpp in the original source; text 0x22D8C0-0x22F128.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/* Declarations in scope here before the split. */
extern char voice_pool[];
extern int bank_sound_count MACRO_ADDR;
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

/* Jitter the vector at arg0 between 0.5 and 6.0, fold in the reference
   frame at D_00187180, and if the emitter accepts it, re-derive it from
   D_00194220, damp to 3/4 and fold the frame in again. */
void clamp_voice_position_to_collision(void *arg0) {
    random_spherical_offset(arg0, 0.5f, 6.0f);
    FastVecAdd_a(arg0, arg0, camera_position_a);
    if (coll_line_fix_a(camera_position_a, arg0, 0x82, sound_probe_ignore, 0) != 0) {
        FastVecSub_a(arg0, coll_hit_pos, camera_position_a);
        FastVecScale_a(arg0, arg0, 0.75f);
        FastVecAdd_a(arg0, arg0, camera_position_a);
    }
}

extern char camera_position[];
extern void FastVecSub(void *, void *, void *);
extern void FastVecAdd(void *, void *, void *);
extern void coll_line_fix(void *, void *, int, int, int);

/* Aim the vector from the frame at D_00187180 toward arg0+0x20, scaled to
   3/4 and 64, and return the line test from arg1 to its end. The int
   return keeps retail's argument order for func_001F9BD8. */
int test_voice_occlusion(void *arg0, void *arg1) {
    float v[4];

    FastVecSub(v, (char *)arg0 + 0x20, camera_position);
    FastVecScale(v, v, 0.75f);
    clamp_vector_length(v, v, 64.0f);
    FastVecAdd(v, v, camera_position);
    return coll_line_fix_a(arg1, v, 0x82, *(int *)((char *)arg0 + 0x18), 0);
}

extern float IntToFloat(int);
extern int truncate_float_to_s32_i(float) __asm__("func_001FA898");

/* Interpolates between the ints at +0x8 and +0xC as x runs from lo to
   hi, squared when bit 0 of +0x19 is set. Each arm is an if/else-if
   chain setting one `r` with a single return; early returns let jump.c
   move the last arm's return block to the end. It returns int, and its
   one caller here uses a void alias. */
struct VoiceVolumeDefinition {
    u8 pad_00[0x8];
    s32 far_volume;               /* 0x08 */
    s32 near_volume;              /* 0x0C */
    u8 pad_10[0x9];
    u8 attenuation_flags;         /* 0x19: bit 0 selects the squared falloff */
};

/* Map distance x to a voice volume between near (lo) and far (hi), linear or squared falloff. */
int calculate_voice_distance_volume(void *arg0, float x, float lo, float hi) {
    struct VoiceVolumeDefinition *definition = (struct VoiceVolumeDefinition *)arg0;
    int r;
    if (definition->attenuation_flags & 1) {
        if (x <= lo) r = definition->near_volume;
        else if (hi <= x) r = definition->far_volume;
        else r = definition->far_volume + truncate_float_to_s32_i((hi - x) * (hi - x) * IntToFloat(definition->near_volume - definition->far_volume) / ((hi - lo) * (hi - lo)));
    } else {
        if (x <= lo) r = definition->near_volume;
        else if (hi <= x) r = definition->far_volume;
        else r = definition->far_volume + truncate_float_to_s32_i((hi - x) * IntToFloat(definition->near_volume - definition->far_volume) / (hi - lo));
    }
    return r;
}

extern char camera_position[];
extern float FastVecDist(int, void *);
/* This caller's view of func_0022DA10 above: it ignores the result, and
   declaring it int would move its next temporary to $v1. */
extern void calculate_voice_distance_volume_v(void *, float, float, float) __asm__("func_0022DA10");

/* Set a voice's volume from its distance to the camera using its near/far range. */
void calculate_voice_volume(void *arg0, int arg1) {
    char *s = (char *)arg0;
    float v = FastVecDist(arg1, camera_position);
    float *p = *(float **)(s + 0x8);
    calculate_voice_distance_volume_v(p, v, p[0], p[1]);
}

extern void transform_vector(void *, void *, void *);
extern float vec_length_xy(void *);
extern float fast_arc_tan_a(float, float) __asm__("func_001FA058");
extern void truncate_float_to_s32(float);

/* Build the vector at arg1 in the D_00187180 frame, project it against
   arg2, and steer by its heading: atan2 of the xy part, negated and
   converted from radians to degrees (180 * 1/pi), scaled by how far the
   projection runs past 1.0, clamped to 0..1. arg0 is unused. */
void calculate_voice_pan(void *arg0, void *arg1, void *arg2) {
    float v[4];
    float t;

    FastVecSub_a(v, arg1, camera_position_a);
    transform_vector(v, v, arg2);
    t = clamp_float(vec_length_xy(v) - 1.0f, 0.0f, 1.0f);
    truncate_float_to_s32(-fast_arc_tan_a(v[0], v[1]) * 180.0f * t * 0.31830987f);
}

typedef u32 u128_2DBE8 __attribute__((mode(TI), aligned(16)));
typedef struct {
    s32 x0;
    u8 x4;
    u8 pad5[0x6B];
} SndVoice_2DBE8;
typedef struct {
    u128_2DBE8 q[4];
    s32 x40;
    s32 x44;
    s32 vol[6];
    u8 pad60[0x10];
    SndVoice_2DBE8 voices[30];
} SndState_2DBE8;
extern int playback_mode MACRO_ADDR;
extern int music_volume MACRO_ADDR;
extern int sound_volume MACRO_ADDR;
extern void snd_StartSoundSystem(void);
extern void snd_SetPlaybackMode(int);
extern void snd_set_mixer_mode(int, int);
extern void snd_PreAllocReverbWorkArea(int, int);
extern void snd_SetGroupVoiceRange(int, int, int);
extern void snd_SetMasterVolume(int, int);
extern void reset_music(void);

/* Reset the voice pool, set up the sound library, voices and volumes, and reset music. */
void initialize_gameplay_sound_system(void) {
    s32 i;
    s32 p;
    s32 end;
    s32 v;

    for (i = 0; i < 4; i++) {
        qzero(&((SndState_2DBE8 *)voice_pool)->q[i]);
    }
    ((SndState_2DBE8 *)voice_pool)->x40 = 0;
    ((SndState_2DBE8 *)voice_pool)->x44 = 0;
    p = (s32)voice_pool;
    end = p + 0xD20;
    do {
        ((SndVoice_2DBE8 *)p)[1].x0 = 0;
        ((SndVoice_2DBE8 *)p)[1].x4 = 0;
        p += 0x70;
    } while (p < end);
    snd_StartSoundSystem();
    snd_SetPlaybackMode(playback_mode == 0);
    snd_set_mixer_mode(0, 1);
    snd_PreAllocReverbWorkArea(2, 4);
    snd_SetGroupVoiceRange(1, 0x18, 0x2F);
    snd_SetGroupVoiceRange(2, 0x18, 0x2F);
    snd_SetGroupVoiceRange(4, 0x18, 0x2F);
    v = sound_volume;
    ((SndState_2DBE8 *)voice_pool)->vol[0] = (v * 8) / 10;
    ((SndState_2DBE8 *)voice_pool)->vol[1] = music_volume;
    ((SndState_2DBE8 *)voice_pool)->vol[2] = (v * 8) / 10;
    ((SndState_2DBE8 *)voice_pool)->vol[3] = (v * 7) / 10;
    ((SndState_2DBE8 *)voice_pool)->vol[4] = (v * 7) / 10;
    ((SndState_2DBE8 *)voice_pool)->vol[5] = v;
    reset_music();
    snd_SetMasterVolume(0, ((SndState_2DBE8 *)voice_pool)->vol[0]);
    snd_SetMasterVolume(1, ((SndState_2DBE8 *)voice_pool)->vol[1]);
    snd_SetMasterVolume(2, ((SndState_2DBE8 *)voice_pool)->vol[2]);
    snd_SetMasterVolume(3, ((SndState_2DBE8 *)voice_pool)->vol[3]);
    snd_SetMasterVolume(4, ((SndState_2DBE8 *)voice_pool)->vol[4]);
    snd_SetMasterVolume(5, ((SndState_2DBE8 *)voice_pool)->vol[5]);
}

typedef unsigned int u128_DD68 __attribute__((mode(TI), aligned(16)));

typedef struct {
    u8 pad00[0x18];
    u8 source_state;
    u8 attenuation_flags;
    s16 sound_index;
    s32 bank_handle;
} VoiceDefinition_DD68;

typedef struct {
    u8 pad00[0x10];
    Vec4 position;
    u8 state;
    u8 pad21[0x9F];
    Vec4 rotation;
} VoiceMoby_DD68;

typedef struct {
    u32 handle;                         /* 0x00 */
    u8 state;                           /* 0x04 */
    u8 flags;                           /* 0x05 */
    u8 pad06[2];
    VoiceDefinition_DD68 *definition;   /* 0x08 */
    s32 pad0C;
    s32 volume;                         /* 0x10 */
    s32 pitch_bend;                     /* 0x14 */
    VoiceMoby_DD68 *moby;               /* 0x18 */
    s32 owner_context;                  /* 0x1C */
    Vec4 position;          /* 0x20 */
    Vec4 position_offset;   /* 0x30 */
    u32 occlusion_history_position;     /* 0x40 */
    u8 occlusion_history[36];           /* 0x44 */
    u8 pad68[8];
} VoiceSlot_DD68;

typedef struct {
    Vec4 listener_history[4];   /* 0x000 */
    s32 listener_history_position;          /* 0x040 */
    s32 pad44;
    s32 group_0_volume;                     /* 0x048 */
    s32 group_1_volume;
    s32 group_2_volume;
    s32 group_3_volume;
    s32 group_4_volume;
    s32 group_5_volume;
    s32 pad60;
    s32 reverb_depth;                       /* 0x064 */
    u8 reverb_type;
    u8 reverb_delay;
    u8 reverb_feedback;
    u8 reverb_commands;
    s32 pending_commands;                   /* 0x06C */
    VoiceSlot_DD68 voices[30];              /* 0x070 */
    s32 padD90[2];
    s32 occlusion_frame;                    /* 0xD98 */
    s32 padD9C;
    Vec4 occlusion_samples[6];  /* 0xDA0 */
} VoiceRuntimeState_DD68;

extern VoiceRuntimeState_DD68 voice_pool_DD68 __asm__("D_0013E650");
extern f32 water_level_DD68[] __asm__("D_0013F740");
extern s32 use_alt_fog_DD68[] __asm__("D_001873D4");
extern u8 camera_position_DD68[] __asm__("D_00187180");
extern char camera_rows_DD68[] __asm__("D_00187390");
extern s32 game_mode_DD68 __asm__("D_0015F6E8") MACRO_ADDR;
extern s32 boot_level_active_DD68 __asm__("D_0015F6C8") MACRO_ADDR;
extern s32 world_update_time_DD68 __asm__("D_0015F6F0") MACRO_ADDR;
extern f32 frame_time_DD68 __asm__("D_0015EE6C") MACRO_ADDR;

extern s32 FastCountChars_DD68(u8 *, s32) __asm__("func_001F9F60");
extern int snd_GetDopplerPitchMod_DD68(int) __asm__("func_0012F2E0");
extern void FastMemSet_DD68(void *, s32, s32) __asm__("func_001F99B0");
extern void snd_ReadGlobalTableEntry_DD68(void) __asm__("func_0012EC30");
extern s32 snd_FlushSoundCommands_DD68(void) __asm__("func_0012DDC0");
extern void snd_SetMasterVolume_DD68(s32, s32) __asm__("func_0012E348");
extern void snd_PlaySoundVolPanPMPB_DD68() __asm__("func_0012E448");
extern void snd_StopSound_DD68(s32) __asm__("func_0012E4A8");
extern void snd_SoundIsStillPlaying_DD68() __asm__("func_0012E588");
extern void snd_SetSoundParams_DD68() __asm__("func_0012E600");
extern void snd_ResetStateAndFlush_DD68() __asm__("func_0012EC40");
extern void snd_SetReverbEx_DD68(s32, s32, s32, s32, s32) __asm__("func_0012F0A8");
extern void snd_AutoReverb_DD68(s32, s32, s32, s32) __asm__("func_0012F120");
extern void FastVecAdd_DD68(void *, void *, void *) __asm__("func_001F9BD8");
extern void FastVecSub_DD68(void *, void *, void *) __asm__("func_001F9BF0");
extern void FastVecScale_DD68(void *, void *, f32) __asm__("func_001F9C30");
extern f32 FastVecDot_DD68(void *, void *) __asm__("func_001F9C78");
extern f32 FastVecLength_DD68(void *) __asm__("func_001F9CB8");
extern void FastVecNormalize_DD68(void *, void *, f32) __asm__("func_001F9DC0");
extern void FastVecTransform_DD68(void *, void *, void *) __asm__("func_001F9EC0");
extern void FastMatTranspose_DD68(void *, void *) __asm__("func_001FA4A0");
extern f32 IntToFloat_DD68(s32) __asm__("func_001FA888");
extern s32 truncate_float_to_s32_DD68(f32) __asm__("func_001FA898");
extern void music_Update_DD68() __asm__("func_00217130");
extern void Sound_PickOcclusionOrigin_DD68(void *) __asm__("func_0022D8C0");
extern u8 Sound_LineTest_DD68(VoiceSlot_DD68 *, void *) __asm__("func_0022D970");
extern s32 Sound_ApplyDistanceFade_DD68(VoiceSlot_DD68 *, Vec4 *) __asm__("func_0022DB00");
extern s32 Sound_ComputePan_DD68(VoiceSlot_DD68 *, Vec4 *, void *) __asm__("func_0022DB48");
extern void Sound_StoreHandleAdvance_DD68() __asm__("func_0022F0A8");
extern void Sound_StoreHandleReset_DD68() __asm__("func_0022F0F0");

/* sound_update: once per frame. Flushes the sound commands and the reverb request, measures the listener's
   velocity from its last four camera positions and sets the group volumes; then for each voice works out its
   radial velocity, distance volume and occlusion (a 36-sample history per voice), and starts, updates or stops
   its sound. Ends with the music update and the command flush. Returns 0; callees.h has it as void, hence the
   alias.
   Adapted from Lombyte's project (GPL-3.0) for PAL: src/audio/voices/sound_update.c, sound_update (FUN_0022ca50). Two of its
   loops are written plainly here: the occlusion-history copy as history[offset + i] = history[offset], and the
   sample clamp with a counter of its own. With Lombyte's byte-index local for the copy, the second voice loop is
   short enough at the first loop pass (192 insns against 208) that the voice_flags address giv is reduced there
   rather than in the rerun, and two spill slots (sp+0x22C/0x230) swap. */
s32 func_0022DD68_r(void) __asm__("func_0022DD68");
s32 func_0022DD68_r(void) {
    VoiceSlot_DD68 *voices;
    Vec4 listener_occlusion_position;
    Vec4 listener_velocity;
    Vec4 relative_velocity;
    s32 flags[30];
    s32 voice_volumes[30];
    f32 radial_velocities[30];
    Vec4 direction;
    Vec4 listener_matrix[4];
    s32 slot_index;
    s32 underwater;
    s32 *voice_flags;
    s32 *volumes;
    VoiceMoby_DD68 *moby;
    f32 water_height;
    s32 history_index;
    s32 divisor;
    s32 current_index;
    s32 previous_history_index;
    s32 listener_sample_count;
    s32 group_volume;
    s32 history_offset;
    s32 sample_index;
    s32 repeat_index;
    s32 owner_removed;
    s32 distance_volume;
    s32 occluded_samples;
    s32 parameter_mask;
    s32 pan;
    s32 pitch_modifier;
    s32 pitch_bend;
    s32 handle;
    s32 command_flags;

    underwater = use_alt_fog_DD68[0];
    water_height = 0.0f;
    if (underwater != 0) {
        water_height = water_level_DD68[0];
    }
    snd_FlushSoundCommands_DD68();

    if (voice_pool_DD68.reverb_commands & 8) {
        snd_SetReverbEx_DD68(2, 0, 0, 0, 0);
    } else if (voice_pool_DD68.reverb_commands & 0x13) {
        snd_SetReverbEx_DD68(2, voice_pool_DD68.reverb_type, voice_pool_DD68.reverb_depth,
                             voice_pool_DD68.reverb_delay, voice_pool_DD68.reverb_feedback);
    } else if (voice_pool_DD68.reverb_commands & 4) {
        snd_AutoReverb_DD68(2, voice_pool_DD68.reverb_depth, 0xC, 3);
    }
    voice_pool_DD68.reverb_commands = 0;

    if ((game_mode_DD68 == 0 || game_mode_DD68 == 2) && boot_level_active_DD68 == 0) {
        Sound_PickOcclusionOrigin_DD68(&listener_occlusion_position);
    }

    qzero(&listener_velocity);
    history_index = voice_pool_DD68.listener_history_position + 1;
    history_index %= 4;
    voice_pool_DD68.listener_history_position = history_index;
    qcopy(&voice_pool_DD68.listener_history[history_index], camera_position_DD68);

    listener_sample_count = 0;
    divisor = 18;
    current_index = history_index;
    previous_history_index = (current_index + 3) % 4;
    if (previous_history_index != current_index) {
        do {
            FastVecSub_DD68(&relative_velocity, &voice_pool_DD68.listener_history[current_index],
                            &voice_pool_DD68.listener_history[previous_history_index]);
            if (!(FastVecLength_DD68(&relative_velocity) < frame_time_DD68 * 60.0f)) {
                break;
            }
            listener_sample_count++;
            FastVecAdd_DD68(&listener_velocity, &listener_velocity, &relative_velocity);
            current_index = previous_history_index;
            previous_history_index = (current_index + 3) % 4;
        } while (previous_history_index != voice_pool_DD68.listener_history_position);
    }
    if (listener_sample_count >= 2) {
        FastVecScale_DD68(&listener_velocity, &listener_velocity,
                          1.0f / IntToFloat_DD68(listener_sample_count));
    }

    if (game_mode_DD68 == 2) {
        snd_SetMasterVolume_DD68(1, 0);
        snd_SetMasterVolume_DD68(0, voice_pool_DD68.group_0_volume / 2);
        snd_SetMasterVolume_DD68(3, voice_pool_DD68.group_3_volume / 2);
    } else {
        group_volume = voice_pool_DD68.group_1_volume;
        if (underwater) {
            group_volume = group_volume * 3 / 5;
        }
        snd_SetMasterVolume_DD68(1, group_volume);
        snd_SetMasterVolume_DD68(0, voice_pool_DD68.group_0_volume);
        snd_SetMasterVolume_DD68(3, voice_pool_DD68.group_3_volume);
    }
    snd_SetMasterVolume_DD68(2, voice_pool_DD68.group_2_volume);
    snd_SetMasterVolume_DD68(4, voice_pool_DD68.group_4_volume);
    snd_SetMasterVolume_DD68(5, voice_pool_DD68.group_5_volume);

    voice_flags = flags;
    volumes = voice_volumes;
    FastMemSet_DD68(voice_flags, 0, 0x78);
    FastMemSet_DD68(volumes, 0, 0x78);
    FastMemSet_DD68(radial_velocities, 0, 0x78);

    for (slot_index = 0; slot_index < 30; slot_index++) {
        {
            s32 state = voice_pool_DD68.voices[slot_index].state;
            if (state != 7) {
                if (voice_pool_DD68.voices[slot_index].handle == 0) {
                    continue;
                }
                if (voice_pool_DD68.voices[slot_index].handle == -1) {
                    continue;
                }
            }
        }
        moby = voice_pool_DD68.voices[slot_index].moby;
        owner_removed = 0;
        /* Fakematch: the goto pair keeps retail's branch order for the two removal states. */
        if (moby != 0) {
            u8 owner_state = moby->state;
            if (owner_state == 0xFE)
                goto owner_removed_label;
            if (owner_state != 0xFD)
                goto owner_checked_label;
        owner_removed_label:
            owner_removed = 1;
        owner_checked_label:;
        }
        if (owner_removed) {
            voice_pool_DD68.voices[slot_index].moby = 0;
        }
        {
            s32 state = voice_pool_DD68.voices[slot_index].state;
            if (state == 4 ||
                (state != 6 && owner_removed &&
                 voice_pool_DD68.voices[slot_index].definition->source_state != 0)) {
                voice_flags[slot_index] = 0x20;
                continue;
            }
        }
        if (game_mode_DD68 != 0 && game_mode_DD68 != 2 && game_mode_DD68 != 6) {
            s32 state = voice_pool_DD68.voices[slot_index].state;
            if (state != 7) {
                voice_flags[slot_index] = 0x10;
                continue;
            }
        }

        if (voice_pool_DD68.voices[slot_index].moby != 0 &&
            !(voice_pool_DD68.voices[slot_index].flags & 8)) {
            if (voice_pool_DD68.voices[slot_index].flags & 0x40) {
                FastVecTransform_DD68(&direction, &voice_pool_DD68.voices[slot_index].position_offset,
                                      &moby->rotation);
                FastVecAdd_DD68(&direction, &direction, &moby->position);
                FastVecSub_DD68(&relative_velocity, &direction,
                                &voice_pool_DD68.voices[slot_index].position);
                qcopy(&voice_pool_DD68.voices[slot_index].position, &direction);
            } else {
                voice_pool_DD68.voices[slot_index].position.f[2] -= 1.0f;
                FastVecSub_DD68(&relative_velocity,
                                &voice_pool_DD68.voices[slot_index].moby->position,
                                    &voice_pool_DD68.voices[slot_index].position);
                qcopy(&voice_pool_DD68.voices[slot_index].position,
                      &voice_pool_DD68.voices[slot_index].moby->position);
                voice_pool_DD68.voices[slot_index].position.f[2] += 1.0f;
            }
        } else {
            qzero(&relative_velocity);
        }
        FastVecSub_DD68(&relative_velocity, &relative_velocity, &listener_velocity);
        FastVecSub_DD68(&direction, camera_position_DD68, &voice_pool_DD68.voices[slot_index].position);
        FastVecNormalize_DD68(&direction, &direction, 1.0f);
        radial_velocities[slot_index] = FastVecDot_DD68(&direction, &relative_velocity);

        if (!(voice_pool_DD68.voices[slot_index].flags & 0x10)) {
            distance_volume = Sound_ApplyDistanceFade_DD68(&voice_pool_DD68.voices[slot_index],
                                                           &voice_pool_DD68.voices[slot_index].position);
            volumes[slot_index] = distance_volume * voice_pool_DD68.voices[slot_index].volume / 1024;
            if (!(voice_pool_DD68.voices[slot_index].definition->attenuation_flags & 2)) {
                voice_flags[slot_index] |= 8;
            }
        } else {
            distance_volume = 0x400;
            voice_volumes[slot_index] = voice_pool_DD68.voices[slot_index].volume;
        }
        if (underwater && !(voice_pool_DD68.voices[slot_index].definition->attenuation_flags & 4) &&
            water_height < voice_pool_DD68.voices[slot_index].position.f[2]) {
            volumes[slot_index] /= 2;
        }
        voice_flags[slot_index] |= 1;
        if (distance_volume < 0x20 && voice_volumes[slot_index] < 0x20 &&
            (voice_pool_DD68.voices[slot_index].flags & 4)) {
            voice_flags[slot_index] = 0x20;
            continue;
        }
        {
            s32 voice_flags_byte = voice_pool_DD68.voices[slot_index].flags;
            /* Fakematch: the xor test gives retail's xori/andi flag test. */
            if ((voice_flags_byte ^ 1) & 1) {
                voice_flags[slot_index] |= 2;
                if (!(voice_pool_DD68.voices[slot_index].flags & 0x20)) {
                    voice_flags[slot_index] |= 4;
                }
            }
        }
    }

    voices = voice_pool_DD68.voices;
    for (slot_index = 0; slot_index < 30; slot_index++) {
        if (!(voice_flags[slot_index] & 8)) {
            continue;
        }
        {
            s32 state = voice_pool_DD68.voices[slot_index].state;
            if (state == 7) {
                if ((game_mode_DD68 != 0 && game_mode_DD68 != 2) || boot_level_active_DD68 != 0) {
                    continue;
                }
                occluded_samples = 0;
                if (world_update_time_DD68 != voice_pool_DD68.occlusion_frame) {
                    {
                        s32 i;
                        for (i = 0; i < 6; i++) {
                            Sound_PickOcclusionOrigin_DD68(&voice_pool_DD68.occlusion_samples[i]);
                        }
                    }
                    voice_pool_DD68.occlusion_frame = world_update_time_DD68;
                }
                for (history_offset = 0, sample_index = 0; history_offset < 36;
                     history_offset += 6, sample_index++) {
                    voice_pool_DD68.voices[slot_index].occlusion_history[history_offset] =
                        Sound_LineTest_DD68(&voice_pool_DD68.voices[slot_index],
                                            &voice_pool_DD68.occlusion_samples[sample_index]);
                    for (repeat_index = 1; repeat_index < 6; repeat_index++) {
                        voice_pool_DD68.voices[slot_index].occlusion_history[history_offset + repeat_index] =
                            voice_pool_DD68.voices[slot_index].occlusion_history[history_offset];
                    }
                    if (voice_pool_DD68.voices[slot_index].occlusion_history[history_offset]) {
                        occluded_samples += 6;
                    }
                }
                if (occluded_samples >= 36) {
                    voice_volumes[slot_index] = 0;
                } else if (occluded_samples >= 19) {
                    voice_volumes[slot_index] = (36 - occluded_samples) * voice_volumes[slot_index] / divisor;
                }
            } else {
                if ((game_mode_DD68 == 0 || game_mode_DD68 == 2) && boot_level_active_DD68 == 0) {
                    voice_pool_DD68.voices[slot_index].occlusion_history_position =
                        (voice_pool_DD68.voices[slot_index].occlusion_history_position + 1) % 36;
                    /* Active sources test every other frame; other voices every fourth. */
                    {
                        s32 test_now = (slot_index ^ world_update_time_DD68) & 1;
                        if (!(voice_pool_DD68.voices[slot_index].flags & 4)) {
                            test_now = (slot_index & 3) == (world_update_time_DD68 & 3);
                        }
                        if (test_now) {
                            voice_pool_DD68.voices[slot_index].occlusion_history
                                [voice_pool_DD68.voices[slot_index].occlusion_history_position] =
                                Sound_LineTest_DD68(&voice_pool_DD68.voices[slot_index],
                                                    &listener_occlusion_position);
                        } else {
                            voice_pool_DD68.voices[slot_index].occlusion_history
                                [voice_pool_DD68.voices[slot_index].occlusion_history_position] =
                                voice_pool_DD68.voices[slot_index].occlusion_history
                                    [(voice_pool_DD68.voices[slot_index].occlusion_history_position + 35) % 36];
                        }
                    }
                }
                {
                    s32 occluded =
                        FastCountChars_DD68(voices[slot_index].occlusion_history, 36);
                    if (occluded >= 36) {
                        voice_volumes[slot_index] = 0;
                    } else if (occluded >= 19) {
                        voice_volumes[slot_index] = (36 - occluded) * voice_volumes[slot_index] / divisor;
                    }
                }
            }
        }
    }

    FastMatTranspose_DD68(listener_matrix, camera_rows_DD68);
    for (slot_index = 0; slot_index < 30; slot_index++) {
        command_flags = voice_flags[slot_index];
        if (command_flags == 0) {
            continue;
        }
        handle = voice_pool_DD68.voices[slot_index].handle;
        voice_pool_DD68.voices[slot_index].handle = -1;
        if (command_flags & 0x20) {
            {
                s32 state = voice_pool_DD68.voices[slot_index].state;
                if (state == 7) {
                    voice_pool_DD68.voices[slot_index].moby = 0;
                    voice_pool_DD68.voices[slot_index].owner_context = 0;
                    voice_pool_DD68.voices[slot_index].state = 0;
                } else {
                    snd_StopSound_DD68(handle);
                    voice_pool_DD68.voices[slot_index].state = 6;
                    snd_SoundIsStillPlaying_DD68(handle, Sound_StoreHandleReset_DD68,
                                                 &voice_pool_DD68.voices[slot_index]);
                }
            }
        } else if (command_flags & 0x10) {
            snd_SoundIsStillPlaying_DD68(handle, Sound_StoreHandleReset_DD68,
                                         &voice_pool_DD68.voices[slot_index]);
        } else {
            pitch_bend = voice_pool_DD68.voices[slot_index].pitch_bend;
            parameter_mask = pitch_bend ? 0x11 : 1;
            pan = 0;
            pitch_modifier = 0;
            if (command_flags & 2) {
                pan = Sound_ComputePan_DD68(&voice_pool_DD68.voices[slot_index],
                                            &voice_pool_DD68.voices[slot_index].position, listener_matrix);
                parameter_mask |= 6;
            }
            if (voice_flags[slot_index] & 4) {
                parameter_mask |= 8;
                pitch_modifier = snd_GetDopplerPitchMod_DD68(
                    truncate_float_to_s32_DD68(radial_velocities[slot_index] * 300.0f));
            }
            if (underwater && !(voice_pool_DD68.voices[slot_index].definition->attenuation_flags & 8)) {
                parameter_mask |= 8;
                pitch_modifier -= 0x5F4;
            }
            {
                s32 state = voice_pool_DD68.voices[slot_index].state;
                if (state != 7) {
                    snd_SetSoundParams_DD68(handle, parameter_mask, voice_volumes[slot_index], pan,
                                            pitch_modifier, pitch_bend, Sound_StoreHandleReset_DD68,
                                            &voice_pool_DD68.voices[slot_index]);
                } else {
                    voice_pool_DD68.voices[slot_index].state = 1;
                    snd_PlaySoundVolPanPMPB_DD68(voice_pool_DD68.voices[slot_index].definition->bank_handle,
                                                 voice_pool_DD68.voices[slot_index].definition->sound_index,
                                                voice_volumes[slot_index], pan, pitch_modifier, pitch_bend,
                                                Sound_StoreHandleAdvance_DD68, &voice_pool_DD68.voices[slot_index]);
                }
            }
        }
    }

    music_Update_DD68();
    snd_ResetStateAndFlush_DD68();
    snd_FlushSoundCommands_DD68();
    snd_ReadGlobalTableEntry_DD68();
    voice_pool_DD68.pending_commands = 0;
    return 0;
}

extern void sce_cd_sync_s(int);
extern int snd_BankLoadByLoc(void *, int);
extern int snd_StreamSafeCheckCDIdle(int);
extern void snd_StreamSafeCdSync(int);
extern void snd_ResolveBankXREFS(void);

/* Reentrancy-guarded: base+0x44 is held at 1 for the duration. The
   return value is func_0012E060's, captured in the delay slot of the
   *next* call. That callee, func_0012EE70 (snd_StreamSafeCheckCDIdle),
   returns int: a value call resets $v0's readers, which lets the
   capture sink into its slot. */
int load_audio_bank_by_location(void *arg0) {
    char *base = voice_pool;
    int r = 0;

    if (*(int *)(base + 0x44) == 0) {
        *(int *)(base + 0x44) = 1;
        sce_cd_sync_s(0);
        r = snd_BankLoadByLoc(arg0, 0);
        snd_StreamSafeCheckCDIdle(1);
        snd_StreamSafeCdSync(0);
        snd_ResolveBankXREFS();
        *(int *)(base + 0x44) = 0;
    }
    return r;
}

/* 12 bytes of post-endlabel nop padding in retail -- see func_001F6668. */
__asm__(".section .text\n\tnop\n\tnop\n\tnop\n");

/* Stop a voice slot: free a queued one at once, else mark it for stopping (idx < 0: no-op). */
void sound_KillChannel(int idx) {
    if (idx >= 0) {
        char *e = voice_pool + idx * 0x70;
        unsigned char st = *(unsigned char *)(e + 0x74);
        if (st == 7) {
            *(int *)(e + 0x88) = 0;
            *(int *)(e + 0x8C) = 0;
            *(unsigned char *)(e + 0x74) = 0;
        } else if (st != 0 && st != 6) {
            *(unsigned char *)(e + 0x74) = 4;
        }
    }
}

typedef int u128_2EB08 __attribute__((mode(TI)));
typedef struct {
    u8 pad0[0x10];
    s32 pitch_bend_min;
    s32 pitch_bend_max;
    u8 source_state;
    u8 pad19;
    u16 source_value;
    s32 pad1C;
} VoiceDefinition_2EB08;
typedef struct {
    u8 pad0[0x10];
    Vec4 position;
    u8 pad20[0x86];
    s16 class_id;
} VoiceMoby_2EB08;
typedef struct {
    u32 handle;
    u8 state;
    u8 flags;
    u8 pad6[2];
    VoiceDefinition_2EB08 *definition;
    u16 source_value;
    s16 linked_index;
    s32 volume;
    s32 pitch_bend;
    VoiceMoby_2EB08 *owner;
    s32 reserved1C;
    Vec4 position;
    Vec4 position_offset;
    s32 history_position;
    u8 history[0x2C];
} VoiceSlot_2EB08;
typedef struct {
    u8 header[0x70];
    VoiceSlot_2EB08 voices[30];
} VoicePool_2EB08;
/* Each selected voice is reached relative to the pool base. */
typedef struct {
    u8 header[0x70];
    VoiceSlot_2EB08 voice;
} VoicePoolWindow_2EB08;
typedef struct {
    Vec4 value;
    u8 pad10[0x60];
} VoicePositionRecord_2EB08;
extern VoicePool_2EB08 voice_pool_r __asm__("D_0013E650");
extern Hero g_hero_r __asm__("D_0013F450");
extern VoicePositionRecord_2EB08 voice_positions_r[] __asm__("D_0013E6E0");
extern u8 D_0013E6C0_2EB08[] __asm__("D_0013E6C0");
extern void clear_u64_value_r(Vec4 *) __asm__("func_001F9BC0");
extern s32 random_integer_below_r(s32) __asm__("func_002140B0");
extern s32 calculate_voice_volume_r(VoiceSlot_2EB08 *, Vec4 *) __asm__("func_0022DB00");

s32 func_0022EB08_r(VoiceDefinition_2EB08 *, u32, VoiceMoby_2EB08 *, Vec4 *,
                    s32) __asm__("func_0022EB08");

/* Takes a free voice of the sound pool for a definition (26 voices, 30 for the hero, the
   held item and class 0x472), places it at the moby or the given position, drops it when
   it would be inaudible and picks its pitch bend. Returns the voice index or -1.
   Adapted from Lombyte's project (GPL-3.0) for PAL: src/textbin/fun_0022d7f0.c, allocate_voice_slot. */
s32 func_0022EB08_r(VoiceDefinition_2EB08 *definition, u32 flags, VoiceMoby_2EB08 *moby,
                    Vec4 *position, s32 volume) {
    s32 volume_offset;
    s32 pitch_bend_max;
    s32 pitch_bend_min;
    s32 slot_index;
    s32 result;
    s32 slot_limit;
    s32 pitch_bend;
    s32 source_inactive;
    VoicePoolWindow_2EB08 *committed_slot;
    VoicePoolWindow_2EB08 *slot;
    VoicePoolWindow_2EB08 *pitch_slot;
    VoicePoolWindow_2EB08 *position_slot;

    source_inactive = definition->source_state == 0;
    if ((flags & 4) == 0) {
        if ((source_inactive ^ 1) != 0) {
            result = -1;
            goto return_result;
        }
        slot_limit = 0x1A;
        goto check_moby;
    } else {
        if (source_inactive) {
            result = -1;
            goto return_result;
        }
        slot_limit = 0x1A;
    }
check_moby:
    if (moby == 0) {
        goto find_free_slot;
    }
    if (((VoiceMoby_2EB08 *)g_hero_r.moby) == moby) {
        goto use_extended_pool;
    }
    if (((VoiceMoby_2EB08 *)g_hero_r.items[0].moby) == moby) {
        goto use_extended_pool;
    }
    if (moby->class_id != 0x472) {
        goto find_free_slot;
    }
use_extended_pool:
    slot_limit = 0x1E;
find_free_slot:
    for (slot_index = 0; slot_index < slot_limit; slot_index++) {
        if (voice_pool_r.voices[slot_index].state == 0) {
            break;
        }
    }
    if (slot_index >= slot_limit) {
        goto allocation_failed;
    }
    slot = (VoicePoolWindow_2EB08 *)((u8 *)&voice_pool_r + slot_index * 0x70);
    slot->voice.definition = definition;
    slot->voice.source_value = (u16)definition->source_value;
    slot->voice.linked_index = -1;
    slot->voice.volume = volume;
    slot->voice.owner = 0;
    slot->voice.reserved1C = 0;
    qzero(&voice_pool_r.voices[slot_index].position_offset);
    if (position != 0) {
        goto set_position;
    }
    if (moby == 0) {
        goto clear_position;
    }
set_position:
    if (moby == 0) {
        goto copy_explicit_position;
    }
    qcopy(&voice_positions_r[slot_index].value, &moby->position);
    position_slot = (VoicePoolWindow_2EB08 *)((u8 *)voice_positions_r + slot_index * 0x70 - 0x90);
    position_slot->voice.position.f[2] += 1.0f;
    goto calculate_volume;
copy_explicit_position:
    qcopy(&voice_positions_r[slot_index].value, position);
    goto calculate_volume;
clear_position:
    flags |= 0x11;
    clear_u64_value_r(&voice_positions_r[slot_index].value);
calculate_volume:
    if (flags & 0x10) {
        goto commit_if_audible;
    }
    volume_offset = slot_index * 0x70;
    result = calculate_voice_volume_r((VoiceSlot_2EB08 *)(D_0013E6C0_2EB08 + volume_offset),
                                 (Vec4 *)(D_0013E6C0_2EB08 + volume_offset + 0x20));
    goto check_calculated_volume;
commit_if_audible:
    result = volume;
check_calculated_volume:
    if (result < 0x20) {
        result = -1;
        goto return_result;
    }
    committed_slot = (VoicePoolWindow_2EB08 *)((u8 *)&voice_pool_r + slot_index * 0x70);
    committed_slot->voice.flags = flags;
    committed_slot->voice.state = 7;
    committed_slot->voice.history_position = 0;
    pitch_bend_max = definition->pitch_bend_max;
    pitch_bend_min = definition->pitch_bend_min;
    if (pitch_bend_max != pitch_bend_min) {
        pitch_bend =
            random_integer_below_r(pitch_bend_max - pitch_bend_min) + definition->pitch_bend_min;
    } else {
        pitch_bend = pitch_bend_max;
    }
    pitch_slot = (VoicePoolWindow_2EB08 *)((u8 *)&voice_pool_r + slot_index * 0x70);
    pitch_slot->voice.pitch_bend = pitch_bend;
    pitch_slot->voice.handle = 0xFFFFFFFF;
    goto return_index;
allocation_failed:
    slot_index = -1;
return_index:
    result = slot_index;
return_result:
    return result;
}


/* Starts entry idx of the sound bank arg2 points at; the same shape as
   its siblings func_0022EE28 and func_0022EEB8. */
int Moby_PlaySound(int idx, int arg1, int arg2) {
    char *p;
    char *tbl;
    int h;

    if (arg2 == 0) {
        return -1;
    }
    p = *(char **)(arg2 + 0x24);
    if (p == 0) {
        return -1;
    }
    tbl = *(char **)(p + 0x28);
    if (tbl == 0) {
        return -1;
    }
    if (idx >= *(unsigned char *)(p + 0xD)) {
        return -1;
    }
    h = allocate_voice_slot(tbl + idx * 32, arg1, arg2, 0, 0x400);
    if (h >= 0) {
        char *rec = voice_pool + h * 0x70;
        *(int *)(rec + 0x88) = arg2;
        *(short *)(rec + 0x7E) = idx;
    }
    return h;
}

extern char *level_sound_defs MACRO_ADDR;

/* Play bank sound idx on a free voice owned by arg2; return the voice slot or -1. */
int PlayBankSound(int idx, int arg1, int arg2) {
    int h;

    if (idx >= bank_sound_count) {
        return -1;
    }
    h = allocate_voice_slot(level_sound_defs + idx * 32, arg1, arg2, 0, 0x400);
    if (h >= 0) {
        char *rec = voice_pool + h * 0x70;
        *(int *)(rec + 0x88) = arg2;
        *(short *)(rec + 0x7E) = idx;
    }
    return h;
}

extern int level_sound_def_count MACRO_ADDR;

struct VoiceGroupEntryWindow {
    u8 pad_00[0x7E];
    s16 linked_index;          /* 0x7E */
    u8 pad_80[0x8];
    s32 owner;                 /* 0x88 */
};

/* Play a level group sound (after the bank sounds) on a free voice; return the slot or -1. */
int allocate_voice_for_group_entry(int group_entry_index, int flags, int owner) {
    int entry_index;
    int slot_index;
    struct VoiceGroupEntryWindow *slot;

    entry_index = group_entry_index + bank_sound_count;
    if (entry_index >= level_sound_def_count) {
        return -1;
    }
    slot_index = allocate_voice_slot(level_sound_defs + entry_index * 32, flags, owner, 0, 0x400);
    if (slot_index >= 0) {
        slot = (struct VoiceGroupEntryWindow *)(voice_pool + slot_index * 0x70);
        slot->owner = owner;
        slot->linked_index = entry_index;
    }
    return slot_index;
}

LINKER_REMNANT("asm/remnants/text", func_0022EF50);

/* Return the voice pool base. */
static inline char *SndSys(void) {
    return voice_pool;
}

typedef struct VoiceRequest {
    u8 pad_00[4];
    void (*fn)(struct VoiceRequest *);
    u8 pad_08[0x88];
} VoiceRequest;

typedef struct VoiceRequestTable {
    u8 pad_000[0xD90];
    s32 count;
    VoiceRequest *handlers;
} VoiceRequestTable;

/*
 * Calls each entry's +4 callback in the sound system's 0x90-byte entry
 * table (count at +0xD90, table at +0xD94), re-reading both every
 * iteration so a callback may grow the table. The inline accessor gives
 * each read of D_0013E650 its own pseudo: the duplicated exit test
 * keeps a temporary and the loop a callee-saved copy, as in retail.
 * `i * 0x90 + (int)table` (not an offset variable) gives the
 * offset-first addu.
 */
void enqueue_voice_request(void) {
    s32 i;

    for (i = 0; i < ((VoiceRequestTable *)SndSys())->count; i++) {
        VoiceRequest *handler = (VoiceRequest *)(i * 0x90 + (s32)((VoiceRequestTable *)SndSys())->handlers);

        if (handler->fn != 0) {
            handler->fn(handler);
        }
    }
}

/* 8 bytes of post-endlabel nop padding in retail -- see func_001F6668. */
__asm__(".section .text\n\tnop\n\tnop\n");

typedef u32 u128 __attribute__((mode(TI), aligned(16)));
typedef struct {
    s32 x0;
    u8 x4;
    u8 pad5[0x6B];
} SndVoice;
typedef struct {
    u128 q[4];
    s32 x40;
    u8 pad44[0x2C];
    SndVoice voices[30];
} SndState;
extern SndState voice_pool_r2 __asm__("D_0013E650");
extern s32 snd_FlushSoundCommands_r() __asm__("func_0012DDC0");
extern s32 snd_StopAllSounds();
extern s32 snd_reset_state_and_flush_commands_r() __asm__("func_0012EC40");

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/audio/voices/sound_stop_all_sounds.c, sound_stop_all_sounds. */
void sound_StopAllSounds(void) {
    s32 i;
    SndState *s;
    u128 *q;
    s32 p;
    s32 end;

    snd_reset_state_and_flush_commands_r();
    snd_FlushSoundCommands_r();
    snd_StopAllSounds();
    while (snd_FlushSoundCommands_r() != 0) {
    }
    for (i = 0; i < 4; i++) {
        qzero(&voice_pool_r2.q[i]);
    }
    s = &voice_pool_r2;
    s->x40 = 0;
    p = (s32)s;
    end = p + 0xD20;
    do {
        ((SndVoice *)p)[1].x0 = 0;
        ((SndVoice *)p)[1].x4 = 0;
        p += 0x70;
    } while (p < end);
}

/* Sound library callback: store the returned handle at arg1 when arg1 is set. */
void SoundBank_StoreHandle(int arg0, long arg1) {
    int *p = (int *)(int)arg1;
    if (p != 0) {
        *p = arg0;
    }
}

/* Voice start callback: store the handle and mark the voice playing, or clear it on failure. */
void voice_start_callback(int arg0, long arg1) {
    unsigned char *p = (unsigned char *)(int)arg1;
    if (p != 0) {
        *(int *)p = arg0;
        if (arg0 != 0) {
            if (p[4] == 1) {
                p[4] = 2;
            }
        } else {
            *(int *)(p + 0x18) = 0;
            *(int *)(p + 0x1C) = 0;
            p[4] = 0;
        }
    }
}

/* Voice update callback: store the handle, or clear the voice slot when the handle is 0. */
void voice_update_callback(int arg0, long arg1) {
    int *p = (int *)(int)arg1;
    if (p != 0) {
        *p = arg0;
        if (arg0 == 0) {
            *(int *)((char *)p + 0x18) = 0;
            *(int *)((char *)p + 0x1C) = 0;
            *(unsigned char *)((char *)p + 4) = 0;
        }
    }
}

LINKER_REMNANT("asm/remnants/text", func_0022F120);
