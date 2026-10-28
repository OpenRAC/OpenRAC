#include "common.h"
#include "pad.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * pad.cpp in the original source; text 0x217F68-0x218928.
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


extern int sce_pad2_get_state(int);
extern int sce_pad2_get_button_profile(int, void *);
extern int sce_vib_get_profile(int, void *);
extern int scePad2Read(int, void *);
extern void ClearPadInput(); /* ClearPadInput(PAD &), defined below */
extern void process_pad_input(void *, void *, int);

/* UpdatePad(PAD &): latches a few per-frame fields, then asks
   func_00124EE0 for the port's state. Not 1: the input is cleared and
   the pad state machine reset. Otherwise state 0 reads two 4-byte info
   blocks (func_00124DF0 -> +0x180, with +0x1DC = 0x79 when the first word
   is 0xFFFFFFFF, and func_00125218 -> +0x184; more than 4 bytes counts
   as none) and moves to state 1, or 2 when the first block was too
   long; state 1 reads the buttons into func_002181F0 and counts +0x18C
   down; state 2 clears the input and sets +0x18C to 2. The two copy
   loops per block are as retail has them (0..n, then n..3). */
void poll_pad_device_state(void *arg0) {
    PadState *p = arg0;
    unsigned char buf[0x40];
    int n, i, s;

    p->held = 0;
    p->prev_held = p->unk348;
    p->prev_raw_held = p->raw_held;
    p->raw_held = 0;
    p->device_state = sce_pad2_get_state(p->socket);
    if (p->device_state == 1) {
        switch (s = p->profile_state) {
        case 0:
            n = sce_pad2_get_button_profile(p->socket, buf);
            if (*(unsigned int *)buf == 0xFFFFFFFF) {
                p->unk1DC = 0x79;
            } else {
                p->unk1DC = 0;
            }
            if (n < 5) {
                for (i = 0; i < n; i++) {
                    p->profile[i] = buf[i];
                }
                p->profile_state = 1;
            } else {
                n = 0;
                p->profile_state = 2;
            }
            for (i = n; i < 4; i++) {
                p->profile[i] = buf[i];
            }
            n = sce_vib_get_profile(p->socket, buf);
            if (n < 5) {
                for (i = 0; i < n; i++) {
                    p->act[i] = buf[i];
                }
            } else {
                n = 0;
            }
            for (i = n; i < 4; i++) {
                p->act[i] = buf[i];
            }
            ClearPadInput(p);
            p->unk18C = 2;
            break;
        case 1:
            n = scePad2Read(p->socket, buf);
            process_pad_input(p, buf, n);
            if (p->unk18C != 0) {
                p->unk18C--;
            }
            break;
        case 2:
            ClearPadInput(p);
            p->unk18C = s;
            break;
        }
    } else {
        ClearPadInput(p);
        p->profile_state = 0;
    }
    p->unk188 = 0;
    p->unk189 = 0;
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

/* ClearPadInput(PAD &) */
typedef struct {
    char unk_000[0x100];
    int a[16];      /* 0x100 */
    int b[16];      /* 0x140 */
    char unk_180[0x20];
    int f1A0, f1A4, f1A8, f1AC;
    int f1B0, f1B4, f1B8, f1BC;
    int f1C0, f1C4, f1C8, f1CC;
    int f1D0, f1D4, f1D8;
} PadClr;

/* ClearPadInput(PAD &). The stores are in the source order that gives
   retail's schedule. a and b are separate members, so each is its own
   loop giv; the second one (b) becomes the loop's base. */
void ClearPadInput(PadClr *p) {
    int i;

    p->f1B0 = 0;
    p->f1A0 = 0;
    p->f1A4 = 0;
    p->f1A8 = 0;
    p->f1D0 = 1;
    p->f1B4 = 0;
    p->f1B8 = 0;
    p->f1C0 = 0;
    p->f1C4 = 0;
    p->f1C8 = 0;
    p->f1D4 = 1;
    p->f1D8 = 0;
    for (i = 0; i < 16; i++) {
        p->a[i] = 0;
        p->b[i] = 0;
    }
}


/* A 12-byte small-data block (func_00227DB0 saves it whole); the byte at +4 mirrors left and
   right. A member behind the start of the object is a two-instruction access for the
   compiler, so it never lands in a delay slot. */
struct PadOptions_181F0 {
    s32 unk0;
    u8 mirror;                /* D_0015EEB4 */
    u8 pad5[7];
};
extern struct PadOptions_181F0 gCheats_r __asm__("D_0015EEB0") MACRO_ADDR;
extern PadState g_controller_state_r __asm__("D_0013CA40");
extern s32 scale_game_frame_count_r(s32) __asm__("func_001F98C0");
extern float vec_length_xy_r(float *) __asm__("func_001F9CE8");
extern float fast_arc_tan_r(float, float) __asm__("func_001FA058");
extern float FastDiffRots_r(float, float) __asm__("func_001FA850");
extern float IntToFloat_r(s32) __asm__("func_001FA888");

void func_002181F0_r(PadState *p, u8 *buf, s32 len) __asm__("func_002181F0");

/* Turns one pad report into the pad's state: buttons, the four stick axes with a dead zone
   and the twelve pressure values, the left/right mirror option, stick directions as buttons,
   pressed/released edges, the two input-lock modes, a stick flick detector over the last
   30 frames, and key repeat.
   Adapted from Lombyte's project (GPL-3.0) for PAL: src/input/pad/process_pad_input.c, process_pad_input. */
void func_002181F0_r(PadState *p, u8 *buf, s32 len) {
    float v[2];
    s32 i, j, k;
    s32 cur, old, ncur, nold;
    float mag, ang;

    p->raw_held = p->held = ((buf[0] << 8) | buf[1]) ^ 0xFFFF;
    for (i = 15; i >= 0; i--) {
        p->analog[i] = 0;
    }
    if (len >= 6) {
        for (i = 0; i < 4; i++) {
            s32 d = buf[i + 2] - 0x7F;
            if (d < 0)
                d = -d;
            if (d >= 0x30) {
                float f = IntToFloat_r(d - 0x30) / IntToFloat_r(0x4C);
                p->analog[i] = f;
                if (f > 1.0f) {
                    p->analog[i] = 1.0f;
                }
                if (buf[i + 2] < 0x7F) {
                    p->analog[i] = -p->analog[i];
                }
            }
        }
    }
    if (len >= 0x12) {
        for (i = 4; i < 16; i++) {
            p->analog[i] = IntToFloat_r(buf[i + 2]) * 0.003921569f;
        }
    }
    if (gCheats_r.mirror) {
        p->analog[2] = -p->analog[2];
        p->analog[0] = -p->analog[0];
        if (p->held & 0x8000) {
            p->held = (p->held & ~0x8000) | 0x2000;
        } else if (p->held & 0x2000) {
            p->held = (p->held & ~0x2000) | 0x8000;
        }
        p->raw_held = p->held;
    }
    {
        float *dst = p->analog_prev, *src = p->analog;
        for (i = 15; i >= 0; i--) {
            *dst++ = *src++;
        }
    }
    if (p->analog[2] != 0.0f || p->analog[3] != 0.0f) {
        p->stick_moved = 1;
    } else {
        p->stick_moved = 0;
    }
    if (p->analog[2] < 0.0f)
        p->held |= 0x8000;
    if (p->analog[2] > 0.0f)
        p->held |= 0x2000;
    if (p->analog[3] < 0.0f)
        p->held |= 0x1000;
    if (p->analog[3] > 0.0f)
        p->held |= 0x4000;
    if (g_controller_state_r.unk18C != 0) {
        p->pressed = 0;
        p->released = 0;
    } else {
        p->pressed = ~p->prev_held & p->held;
        p->released = ~p->held & p->prev_held;
    }
    p->no_buttons = p->held == 0;
    p->no_direction = (p->held & 0xF000) == 0;
    p->raw_pressed = ~p->prev_held & p->raw_held;
    p->raw_released = ~p->held & p->prev_raw_held;
    p->pressed_unmasked = p->pressed;
    p->released_unmasked = p->released;
    p->held_unmasked = p->held;
    p->unk348 = p->held;
    if (gCheats_r.mirror) {
        p->analog_prev[2] = -p->analog_prev[2];
        p->analog_prev[0] = -p->analog_prev[0];
        if (((s32)p->held_unmasked) & 0x8000) {
            p->held_unmasked = (((s32)p->held_unmasked) & ~0x8000) | 0x2000;
        } else if (((s32)p->held_unmasked) & 0x2000) {
            p->held_unmasked = (((s32)p->held_unmasked) & ~0x2000) | 0x8000;
        }
        if (((s32)p->pressed_unmasked) & 0x8000) {
            p->pressed_unmasked = (((s32)p->pressed_unmasked) & ~0x8000) | 0x2000;
        } else if (((s32)p->pressed_unmasked) & 0x2000) {
            p->pressed_unmasked = (((s32)p->pressed_unmasked) & ~0x2000) | 0x8000;
        }
        if (((s32)p->released_unmasked) & 0x8000) {
            p->released_unmasked = (((s32)p->released_unmasked) & ~0x8000) | 0x2000;
        } else if (((s32)p->released_unmasked) & 0x2000) {
            p->released_unmasked = (((s32)p->released_unmasked) & ~0x2000) | 0x8000;
        }
    }
    if (p->mode == 1) {
        p->held &= ~0x5030;
        p->pressed &= ~0x5030;
        p->released &= ~0x5030;
        p->analog[0] = 0.0f;
        p->analog[1] = 0.0f;
        p->mode = 0;
    }
    if (p->mode == 2) {
        p->held &= 0x900;
        p->pressed &= 0x900;
        p->released &= 0x900;
        p->raw_held &= 0x900;
        p->no_direction = 1;
        p->analog[2] = 0.0f;
        p->analog[3] = 0.0f;
        p->mode = 0;
    }
    v[0] = p->analog[2];
    v[1] = p->analog[3];
    mag = vec_length_xy_r(v);
    ang = fast_arc_tan_r(v[0], v[1]);
    p->hist_mag[p->hist_idx] = mag;
    p->hist_ang[p->hist_idx] = ang;
    if (mag > 0.9f) {
        for (j = 1; j < scale_game_frame_count_r(4); j++) {
            float m = p->hist_mag[(p->hist_idx - j + 30) % 30];
            if (m > 0.9f)
                break;
            if (m < 0.25f) {
                p->pressed |= 0x10000;
                break;
            }
        }
        if (!(p->pressed & 0x10000)) {
            for (k = 1; k < scale_game_frame_count_r(5); k++) {
                if (FastDiffRots_r(p->hist_ang[(p->hist_idx - k + 30) % 30], ang) >
                    0.9599311f) {
                    p->pressed |= 0x10000;
                    break;
                }
            }
        }
    }
    p->hist_btn[p->hist_idx] = p->pressed;
    p->hist_idx = (p->hist_idx + 1) % 30;
    if (++p->hist_count > 30) {
        p->hist_count = 30;
    }
    if (p->rep_rate) {
        cur = p->held;
        if (cur != 0 && cur == p->prev_held) {
            u8 t = --p->rep_timer;
            if (0 == t || t == 0xFF) {
                p->rep_timer = p->rep_rate;
                p->pressed = cur;
            }
        } else {
            p->rep_timer = p->rep_delay;
        }
    }
}

extern void poll_pad_device_state(void *);

/* UpdatePad(void) */
void UpdatePad(void) {
    poll_pad_device_state(g_controller_state);
}
