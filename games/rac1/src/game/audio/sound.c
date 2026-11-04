#include "common.h"
#include "structs/vector.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/hero.h"
#include "structs/misc.h"
#include "game/moby/mobyutil.h"
#include "structs/voice.h"
#include "asm_functions/executable.h"
#include "game/math/fastfunc.h"
#include "core/sound/989snd.h"
#include "game/audio/music.h"
#include "core/cdvd/callbacks.h"

/*
 * sound.cpp in the original source; text 0x22D8C0-0x22F128.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

extern char voice_pool[];
extern int bank_sound_count MACRO_ADDR;

extern char camera_position_a[] __asm__("D_00187180");
extern char coll_hit_pos[];
extern int sound_probe_ignore;

/* Jitter the vector at arg0 between 0.5 and 6.0, fold in the reference
   frame at camera_position, and if the emitter accepts it, re-derive it from
   coll_hit_pos, damp to 3/4 and fold the frame in again. */
void clamp_voice_position_to_collision(void *arg0) {
    random_spherical_offset(arg0, 0.5f, 6.0f);
    FastVecAdd(arg0, arg0, camera_position_a);
    if (coll_line_fix(camera_position_a, arg0, 0x82, sound_probe_ignore, 0) != 0) {
        FastVecSub(arg0, coll_hit_pos, camera_position_a);
        FastVecScale(arg0, arg0, 0.75f);
        FastVecAdd(arg0, arg0, camera_position_a);
    }
}

extern char camera_position[];

/* Aim the vector from the frame at camera_position toward arg0+0x20, scaled to
   3/4 and 64, and return the line test from arg1 to its end. The int
   return keeps retail's argument order for FastVecAdd. */
int test_voice_occlusion(void *arg0, void *arg1) {
    float v[4];

    FastVecSub(v, (char *)arg0 + 0x20, camera_position);
    FastVecScale(v, v, 0.75f);
    clamp_vector_length(v, v, 64.0f);
    FastVecAdd(v, v, camera_position);
    return coll_line_fix(arg1, v, 0x82, *(int *)((char *)arg0 + 0x18), 0);
}

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
        if (x <= lo)
            r = definition->near_volume;
        else if (hi <= x)
            r = definition->far_volume;
        else
            r = definition->far_volume +
                truncate_float_to_s32_i((hi - x) * (hi - x) *
                                        IntToFloat(definition->near_volume - definition->far_volume) /
                                        ((hi - lo) * (hi - lo)));
    } else {
        if (x <= lo)
            r = definition->near_volume;
        else if (hi <= x)
            r = definition->far_volume;
        else
            r = definition->far_volume +
                truncate_float_to_s32_i((hi - x) * IntToFloat(definition->near_volume - definition->far_volume) /
                                        (hi - lo));
    }
    return r;
}

extern char camera_position[];

/* This caller's view of calculate_voice_distance_volume above: it ignores the result, and
   declaring it int would move its next temporary to $v1. */
extern void calculate_voice_distance_volume_v(void *, float, float, float) __asm__("func_0022DA10");

/* Set a voice's volume from its distance to the camera using its near/far range. */
void calculate_voice_volume(void *arg0, int arg1) {
    char *s = (char *)arg0;
    float v = FastVecDist(arg1, camera_position);
    float *p = *(float **)(s + 0x8);
    calculate_voice_distance_volume_v(p, v, p[0], p[1]);
}

extern void truncate_float_to_s32(float);

/* Build the vector at arg1 in the camera_position frame, project it against
   arg2, and steer by its heading: atan2 of the xy part, negated and
   converted from radians to degrees (180 * 1/pi), scaled by how far the
   projection runs past 1.0, clamped to 0..1. arg0 is unused. */
void calculate_voice_pan(void *arg0, void *arg1, void *arg2) {
    float v[4];
    float t;

    FastVecSub(v, arg1, camera_position_a);
    transform_vector(v, v, arg2);
    t = clamp_float(vec_length_xy(v) - 1.0f, 0.0f, 1.0f);
    truncate_float_to_s32(-fast_arc_tan(v[0], v[1]) * 180.0f * t * 0.31830987f);
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
extern s32 D_001873D4[];
extern u8 camera_position_DD68[] __asm__("D_00187180");
extern char camera_rows_DD68[] __asm__("D_00187390");
extern s32 D_0015F6E8 MACRO_ADDR;
extern s32 D_0015F6C8 MACRO_ADDR;
extern s32 D_0015F6F0 MACRO_ADDR;
extern f32 D_0015EE6C MACRO_ADDR;

extern void snd_ResetStateAndFlush_DD68() __asm__("func_0012EC40");

extern s32 truncate_float_to_s32_DD68(f32) __asm__("func_001FA898");

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

    underwater = use_alt_fog[0];
    water_height = 0.0f;
    if (underwater != 0) {
        water_height = water_level_DD68[0];
    }
    snd_FlushSoundCommands();

    if (voice_pool_DD68.reverb_commands & 8) {
        snd_set_reverb_ex(2, 0, 0, 0, 0);
    } else if (voice_pool_DD68.reverb_commands & 0x13) {
        snd_set_reverb_ex(2, voice_pool_DD68.reverb_type, voice_pool_DD68.reverb_depth, voice_pool_DD68.reverb_delay,
                          voice_pool_DD68.reverb_feedback);
    } else if (voice_pool_DD68.reverb_commands & 4) {
        snd_AutoReverb(2, voice_pool_DD68.reverb_depth, 0xC, 3);
    }
    voice_pool_DD68.reverb_commands = 0;

    if ((g_game_mode == 0 || g_game_mode == 2) && boot_level_active == 0) {
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
            FastVecSub(&relative_velocity, &voice_pool_DD68.listener_history[current_index],
                       &voice_pool_DD68.listener_history[previous_history_index]);
            if (!(FastVecLength(&relative_velocity) < g_frame_time * 60.0f)) {
                break;
            }
            listener_sample_count++;
            FastVecAdd(&listener_velocity, &listener_velocity, &relative_velocity);
            current_index = previous_history_index;
            previous_history_index = (current_index + 3) % 4;
        } while (previous_history_index != voice_pool_DD68.listener_history_position);
    }
    if (listener_sample_count >= 2) {
        FastVecScale(&listener_velocity, &listener_velocity, 1.0f / IntToFloat(listener_sample_count));
    }

    if (g_game_mode == 2) {
        snd_SetMasterVolume(1, 0);
        snd_SetMasterVolume(0, voice_pool_DD68.group_0_volume / 2);
        snd_SetMasterVolume(3, voice_pool_DD68.group_3_volume / 2);
    } else {
        group_volume = voice_pool_DD68.group_1_volume;
        if (underwater) {
            group_volume = group_volume * 3 / 5;
        }
        snd_SetMasterVolume(1, group_volume);
        snd_SetMasterVolume(0, voice_pool_DD68.group_0_volume);
        snd_SetMasterVolume(3, voice_pool_DD68.group_3_volume);
    }
    snd_SetMasterVolume(2, voice_pool_DD68.group_2_volume);
    snd_SetMasterVolume(4, voice_pool_DD68.group_4_volume);
    snd_SetMasterVolume(5, voice_pool_DD68.group_5_volume);

    voice_flags = flags;
    volumes = voice_volumes;
    FastMemSet(voice_flags, 0, 0x78);
    FastMemSet(volumes, 0, 0x78);
    FastMemSet(radial_velocities, 0, 0x78);

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
        /* FAKEMATCH: the goto pair keeps retail's branch order for the two removal states. */
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
                (state != 6 && owner_removed && voice_pool_DD68.voices[slot_index].definition->source_state != 0)) {
                voice_flags[slot_index] = 0x20;
                continue;
            }
        }
        if (g_game_mode != 0 && g_game_mode != 2 && g_game_mode != 6) {
            s32 state = voice_pool_DD68.voices[slot_index].state;
            if (state != 7) {
                voice_flags[slot_index] = 0x10;
                continue;
            }
        }

        if (voice_pool_DD68.voices[slot_index].moby != 0 && !(voice_pool_DD68.voices[slot_index].flags & 8)) {
            if (voice_pool_DD68.voices[slot_index].flags & 0x40) {
                matrix_mul_vec3(&direction, &voice_pool_DD68.voices[slot_index].position_offset, &moby->rotation);
                FastVecAdd(&direction, &direction, &moby->position);
                FastVecSub(&relative_velocity, &direction, &voice_pool_DD68.voices[slot_index].position);
                qcopy(&voice_pool_DD68.voices[slot_index].position, &direction);
            } else {
                voice_pool_DD68.voices[slot_index].position.f[2] -= 1.0f;
                FastVecSub(&relative_velocity, &voice_pool_DD68.voices[slot_index].moby->position,
                           &voice_pool_DD68.voices[slot_index].position);
                qcopy(&voice_pool_DD68.voices[slot_index].position, &voice_pool_DD68.voices[slot_index].moby->position);
                voice_pool_DD68.voices[slot_index].position.f[2] += 1.0f;
            }
        } else {
            qzero(&relative_velocity);
        }
        FastVecSub(&relative_velocity, &relative_velocity, &listener_velocity);
        FastVecSub(&direction, camera_position_DD68, &voice_pool_DD68.voices[slot_index].position);
        fast_vec_normalize(&direction, &direction, 1.0f);
        radial_velocities[slot_index] = FastVecDot(&direction, &relative_velocity);

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
            /* FAKEMATCH: the xor test gives retail's xori/andi flag test. */
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
                if ((g_game_mode != 0 && g_game_mode != 2) || boot_level_active != 0) {
                    continue;
                }
                occluded_samples = 0;
                if (world_update_time != voice_pool_DD68.occlusion_frame) {
                    {
                        s32 i;
                        for (i = 0; i < 6; i++) {
                            Sound_PickOcclusionOrigin_DD68(&voice_pool_DD68.occlusion_samples[i]);
                        }
                    }
                    voice_pool_DD68.occlusion_frame = world_update_time;
                }
                for (history_offset = 0, sample_index = 0; history_offset < 36; history_offset += 6, sample_index++) {
                    voice_pool_DD68.voices[slot_index].occlusion_history[history_offset] = Sound_LineTest_DD68(
                        &voice_pool_DD68.voices[slot_index], &voice_pool_DD68.occlusion_samples[sample_index]);
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
                if ((g_game_mode == 0 || g_game_mode == 2) && boot_level_active == 0) {
                    voice_pool_DD68.voices[slot_index].occlusion_history_position =
                        (voice_pool_DD68.voices[slot_index].occlusion_history_position + 1) % 36;
                    /* Active sources test every other frame; other voices every fourth. */
                    {
                        s32 test_now = (slot_index ^ world_update_time) & 1;
                        if (!(voice_pool_DD68.voices[slot_index].flags & 4)) {
                            test_now = (slot_index & 3) == (world_update_time & 3);
                        }
                        if (test_now) {
                            voice_pool_DD68.voices[slot_index]
                                .occlusion_history[voice_pool_DD68.voices[slot_index].occlusion_history_position] =
                                Sound_LineTest_DD68(&voice_pool_DD68.voices[slot_index], &listener_occlusion_position);
                        } else {
                            voice_pool_DD68.voices[slot_index]
                                .occlusion_history[voice_pool_DD68.voices[slot_index].occlusion_history_position] =
                                voice_pool_DD68.voices[slot_index].occlusion_history
                                    [(voice_pool_DD68.voices[slot_index].occlusion_history_position + 35) % 36];
                        }
                    }
                }
                {
                    s32 occluded = FastCountChars(voices[slot_index].occlusion_history, 36);
                    if (occluded >= 36) {
                        voice_volumes[slot_index] = 0;
                    } else if (occluded >= 19) {
                        voice_volumes[slot_index] = (36 - occluded) * voice_volumes[slot_index] / divisor;
                    }
                }
            }
        }
    }

    matrix_from_basis_rows(listener_matrix, camera_rows_DD68);
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
                    snd_stop_sound(handle);
                    voice_pool_DD68.voices[slot_index].state = 6;
                    snd_SoundIsStillPlaying_CB(handle, Sound_StoreHandleReset_DD68,
                                               &voice_pool_DD68.voices[slot_index]);
                }
            }
        } else if (command_flags & 0x10) {
            snd_SoundIsStillPlaying_CB(handle, Sound_StoreHandleReset_DD68, &voice_pool_DD68.voices[slot_index]);
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
                pitch_modifier =
                    snd_GetDopplerPitchMod(truncate_float_to_s32_DD68(radial_velocities[slot_index] * 300.0f));
            }
            if (underwater && !(voice_pool_DD68.voices[slot_index].definition->attenuation_flags & 8)) {
                parameter_mask |= 8;
                pitch_modifier -= 0x5F4;
            }
            {
                s32 state = voice_pool_DD68.voices[slot_index].state;
                if (state != 7) {
                    snd_set_sound_params_cb(handle, parameter_mask, voice_volumes[slot_index], pan, pitch_modifier,
                                            pitch_bend, Sound_StoreHandleReset_DD68,
                                            &voice_pool_DD68.voices[slot_index]);
                } else {
                    voice_pool_DD68.voices[slot_index].state = 1;
                    snd_play_sound_vol_pan_pmpb(voice_pool_DD68.voices[slot_index].definition->bank_handle,
                                                voice_pool_DD68.voices[slot_index].definition->sound_index,
                                                voice_volumes[slot_index], pan, pitch_modifier, pitch_bend,
                                                Sound_StoreHandleAdvance_DD68, &voice_pool_DD68.voices[slot_index]);
                }
            }
        }
    }

    music_Update();
    snd_ResetStateAndFlush_DD68();
    snd_FlushSoundCommands();
    snd_reenable_command_flush();
    voice_pool_DD68.pending_commands = 0;
    return 0;
}

/* Reentrancy-guarded: base+0x44 is held at 1 for the duration. The
   return value is snd_BankLoadByLoc's, captured in the delay slot of the
   *next* call. That callee, snd_StreamSafeCheckCDIdle,
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

/* 12 bytes of post-endlabel nop padding in retail -- see FontPrint. */
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
} VoiceDefinition;
typedef struct {
    u8 pad0[0x10];
    Vec4 position;
    u8 pad20[0x86];
    s16 class_id;
} VoiceMoby;
typedef struct {
    u32 handle;
    u8 state;
    u8 flags;
    u8 pad6[2];
    VoiceDefinition *definition;
    u16 source_value;
    s16 linked_index;
    s32 volume;
    s32 pitch_bend;
    VoiceMoby *owner;
    s32 reserved1C;
    Vec4 position;
    Vec4 position_offset;
    s32 history_position;
    u8 history[0x2C];
} VoiceSlot;
typedef struct {
    u8 header[0x70];
    VoiceSlot voices[30];
} VoicePool;
/* Each selected voice is reached relative to the pool base. */
typedef struct {
    u8 header[0x70];
    VoiceSlot voice;
} VoicePoolWindow;
typedef struct {
    Vec4 value;
    u8 pad10[0x60];
} VoicePositionRecord;
extern VoicePool voice_pool_r __asm__("D_0013E650");
extern Hero D_0013F450;
extern VoicePositionRecord D_0013E6E0[];
extern u8 D_0013E6C0_2EB08[] __asm__("D_0013E6C0");

extern s32 calculate_voice_volume_r(VoiceSlot *, Vec4 *) __asm__("func_0022DB00");

s32 func_0022EB08_r(VoiceDefinition *, u32, VoiceMoby *, Vec4 *, s32) __asm__("func_0022EB08");

/* Takes a free voice of the sound pool for a definition (26 voices, 30 for the hero, the
   held item and class 0x472), places it at the moby or the given position, drops it when
   it would be inaudible and picks its pitch bend. Returns the voice index or -1.
   Adapted from Lombyte's project (GPL-3.0) for PAL: src/textbin/fun_0022d7f0.c, allocate_voice_slot. */
s32 func_0022EB08_r(VoiceDefinition *definition, u32 flags, VoiceMoby *moby, Vec4 *position, s32 volume) {
    s32 volume_offset;
    s32 pitch_bend_max;
    s32 pitch_bend_min;
    s32 slot_index;
    s32 result;
    s32 slot_limit;
    s32 pitch_bend;
    s32 source_inactive;
    VoicePoolWindow *committed_slot;
    VoicePoolWindow *slot;
    VoicePoolWindow *pitch_slot;
    VoicePoolWindow *position_slot;

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
    if (((VoiceMoby *)g_hero.moby) == moby) {
        goto use_extended_pool;
    }
    if (((VoiceMoby *)g_hero.items[0].moby) == moby) {
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
    slot = (VoicePoolWindow *)((u8 *)&voice_pool_r + slot_index * 0x70);
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
    qcopy(&voice_positions[slot_index].value, &moby->position);
    position_slot = (VoicePoolWindow *)((u8 *)voice_positions + slot_index * 0x70 - 0x90);
    position_slot->voice.position.f[2] += 1.0f;
    goto calculate_volume;
copy_explicit_position:
    qcopy(&voice_positions[slot_index].value, position);
    goto calculate_volume;
clear_position:
    flags |= 0x11;
    clear_u64_value(&voice_positions[slot_index].value);
calculate_volume:
    if (flags & 0x10) {
        goto commit_if_audible;
    }
    volume_offset = slot_index * 0x70;
    result = calculate_voice_volume_r((VoiceSlot *)(D_0013E6C0_2EB08 + volume_offset),
                                      (Vec4 *)(D_0013E6C0_2EB08 + volume_offset + 0x20));
    goto check_calculated_volume;
commit_if_audible:
    result = volume;
check_calculated_volume:
    if (result < 0x20) {
        result = -1;
        goto return_result;
    }
    committed_slot = (VoicePoolWindow *)((u8 *)&voice_pool_r + slot_index * 0x70);
    committed_slot->voice.flags = flags;
    committed_slot->voice.state = 7;
    committed_slot->voice.history_position = 0;
    pitch_bend_max = definition->pitch_bend_max;
    pitch_bend_min = definition->pitch_bend_min;
    if (pitch_bend_max != pitch_bend_min) {
        pitch_bend = random_integer_below(pitch_bend_max - pitch_bend_min) + definition->pitch_bend_min;
    } else {
        pitch_bend = pitch_bend_max;
    }
    pitch_slot = (VoicePoolWindow *)((u8 *)&voice_pool_r + slot_index * 0x70);
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
   its siblings PlayBankSound and allocate_voice_for_group_entry. */
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

/* Play a level group sound (after the bank sounds) on a free voice; return the slot or -1. */
int allocate_voice_for_group_entry(int group_entry_index, int flags, int owner) {
    int entry_index;
    int slot_index;
    VoicePoolRow *slot;

    entry_index = group_entry_index + bank_sound_count;
    if (entry_index >= level_sound_def_count) {
        return -1;
    }
    slot_index = allocate_voice_slot(level_sound_defs + entry_index * 32, flags, owner, 0, 0x400);
    if (slot_index >= 0) {
        slot = (VoicePoolRow *)(voice_pool + slot_index * 0x70);
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
 * each read of voice_pool its own pseudo: the duplicated exit test
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

/* 8 bytes of post-endlabel nop padding in retail -- see FontPrint. */
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
