#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"
#include "pad.h"

/*
 * stream.cpp in the original source; text 0x217588-0x217F68.
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

extern int snd_StreamSafeCdBreak_i(void) __asm__("func_0012EFE8");

/* func_0012EFE8 returns int (989snd.c): with the result in $v0, the
   constant goes to the register retail uses. */
void request_audio_stream_break(void) {
    short *p = cd_read_state;
    if (p[4] != 0) {
        snd_StreamSafeCdBreak_i();
        ((char *)p)[0xA] = 1;
    }
}

typedef struct {
    char b[4];
} StreamHdr;
extern StreamHdr stream_read_mode_template;
/* MACRO_ADDR for retail's one-register lbu; the explicit size 4 (over
   -G2) is what keeps the assembler from expanding it through $gp. */
extern unsigned char cd_read_spindle_speed[4] MACRO_ADDR;
extern int cd_read_wait_frames MACRO_ADDR;
extern int cd_read_slowdown_applied MACRO_ADDR;
extern void sce_cd_read(int, int, int, void *);

/* Start a CD stream read of arg2 sectors from arg1 into arg0 at the current spindle speed. */
int Stream_SubmitRead(int arg0, int arg1, int arg2) {
    StreamHdr hdr = stream_read_mode_template;

    hdr.b[1] = cd_read_spindle_speed[0];
    cd_read_wait_frames = 0;
    cd_read_slowdown_applied = 0;
    sce_cd_read(arg1, arg2, arg0, &hdr);
    return 1;
}

extern char load_file_start_failed_string[];
extern int snd_StreamSafeCdRead(int, int, int, void *);

/* Start a stream: refuse if one is already running (+0x8) or the length
   is zero, otherwise hand the request to func_0012EE98 with the header
   block at D_001517D0+0x30 and record it. Returns the length in the
   units the caller wants, arg2 << 11; on a rejected request it reports
   the failure and returns 0. */
int start_audio_stream_read(int arg0, int arg1, int arg2) {
    char *d = (char *)cd_read_state;

    if (*(short *)(d + 8) != 0) {
        return 0;
    }
    if (arg2 == 0) {
        return 0;
    }
    if (snd_StreamSafeCdRead(arg1, arg2, arg0, d + 0x30) == 0) {
        STUB_printf(load_file_start_failed_string);
        raise_kernel_trap();
        return 0;
    }
    *(int *)(d + 0x14) = arg0;
    *(short *)(d + 8) = 1;
    *(int *)(d + 0xC) = arg1;
    *(int *)(d + 0x10) = arg2;
    return arg2 << 11;
}

/* This caller sets up no arguments at all; func_00217628 is defined
   just above with three. Reach it through an alias rather than
   redeclaring it. */
extern int start_audio_stream_read_v(void) __asm__("func_00217628");
extern void sce_gs_sync_v(int);
extern void music_Update(void);
extern void snd_reset_state_and_flush_commands(void);
extern void snd_FlushSoundCommands(void);
extern void snd_reenable_command_flush(void);

/*
 * Close, not exact (15/124), same size so inert. Semantics confirmed:
 * call func_00217628, and while it succeeded and the flag at
 * D_001517D0+0x8 stays set, pump the five service calls; return the
 * original result either way.
 *
 * Residual is purely which callee-saved register holds what: retail puts
 * the result in $s0 and the D_001517D0 base in $s1, this compiler the
 * other way round, and the prologue save order follows from that.
 * Tried and did not move it: swapping the two local declarations (the
 * documented declaration-order lever -- it does not apply when a local
 * is initialised straight from a call, since the live range starts at
 * the call regardless), and restructuring the loop as guard + do/while
 * so the base local is assigned late exactly as retail does. Allocator
 * destination-choice question, not source shape.
 */
/* Load: call func_00217628 and, if it succeeded, keep pumping the five
   service calls while the D_001517D0+0x8 state word stays nonzero;
   return func_00217628's result either way.
   As in func_00217748 below, the base pointer is assigned in the loop
   condition: the exit test jump.c duplicates in front of the loop gets
   its own temporary, and the loop's copy goes to $s1 with the result in
   $s0, as in retail. */
/* Load */
int Load(int arg0, int arg1, int arg2) {
    char *d;
    int r = start_audio_stream_read(arg0, arg1, arg2);
    if (r != 0) {
        while (d = (char *)cd_read_state, *(short *)(d + 0x8) != 0) {
            sce_gs_sync_v(0);
            music_Update();
            snd_reset_state_and_flush_commands();
            snd_FlushSoundCommands();
            snd_reenable_command_flush();
        }
    }
    return r;
}

/* Same pump loop as func_002176C8 above, gated on the argument instead
   of on a call's result: with arg0 set, keep servicing while the
   D_001517D0+8 state word stays nonzero; with it clear, run the four
   service calls exactly once. Either way return the state word. */
/* With arg0 set, keep pumping the five service calls while the
   D_001517D0+8 state word stays nonzero; with it clear, run the four that
   are not func_00122598 once. Either way return the state word.
   The base pointer is assigned IN the loop condition: the exit test that
   jump.c duplicates in front of the loop then gets its own pseudo (retail's
   $a0 for the guard), loop.c hoists the loop's copy into $s0 and CSE turns
   it into `daddu $s0,$a0,$0`. A second, separate local for the return keeps
   its base a temp ($v1) sharing the %hi with the guard through $s1. */
struct MusicStreamState {
    u8 pad_0[0x8];
    s16 read_state;            /* 0x08: 0 idle, 1 reading, 2 done */
};

/* Pump music and stream updates, until the CD read is idle if arg0; return the read state. */
short update_audio_stream_until_idle(int arg0) {
    if (arg0 != 0) {
        struct MusicStreamState *d;
        while (d = (struct MusicStreamState *)cd_read_state, d->read_state != 0) {
            sce_gs_sync_v(0);
            music_Update();
            snd_reset_state_and_flush_commands();
            snd_FlushSoundCommands();
            snd_reenable_command_flush();
        }
    } else {
        music_Update();
        snd_reset_state_and_flush_commands();
        snd_FlushSoundCommands();
        snd_reenable_command_flush();
    }
    {
        struct MusicStreamState *d = (struct MusicStreamState *)cd_read_state;
        return d->read_state;
    }
}

extern int snd_StreamSafeCdGetError(void);

/* CD read callback: mark the music stream read finished, or done if more data is ready. */
void finish_audio_stream_read(int callback_value) {
    struct MusicStreamState *state;
    if (callback_value == 1) {
        state = (struct MusicStreamState *)cd_read_state;
        state->read_state = 0;
        if (snd_StreamSafeCdGetError() != 0) {
            state->read_state = 2;
        }
    }
}

/* Stream callback: move a music channel from state 2 to ready (3) once the voice is valid. */
void music_channel_ready_callback(int arg0, long arg1) {
    short *p = (short *)(int)arg1;
    if (p != 0 && arg0 != 0 && p[5] == 2) {
        p[5] = 3;
    }
}

extern void music_start_track_by_id(short, short, short);

/* Stream callback: store the voice handle and advance the channel, or restart the secondary track. */
void music_secondary_start_callback(int arg0, long arg1) {
    short *p = (short *)(int)arg1;
    if (p == 0) {
        return;
    }
    *(int *)p = arg0;
    if (arg0 != 0) {
        if (p[5] == 1) {
            p[5] = 2;
        }
    } else {
        short *b = cd_read_state;
        music_start_track_by_id(b[0x2A], b[0x2C], b[0x2B]);
    }
}

extern void music_PreseekTrack(short, short, short);

/* Stream callback: store the voice handle and advance the channel, or redo the primary preseek. */
void music_primary_preseek_callback(int arg0, long arg1) {
    short *p = (short *)(int)arg1;
    if (p == 0) {
        return;
    }
    *(int *)p = arg0;
    if (arg0 != 0) {
        if (p[5] == 1) {
            p[5] = 2;
        }
    } else {
        short *b = cd_read_state;
        music_PreseekTrack(b[0x1C], b[0x1E], b[0x1D]);
    }
}

extern short music_flagged_track_started NOT_SDA;

/* Stream callback: store the voice handle and mark the transition track playing, or reset it. */
void music_transition_start_callback(int arg0, long arg1) {
    short *p = (short *)(int)arg1;
    if (p != 0) {
        *(int *)p = arg0;
        if (arg0 != 0) {
            short state = p[5];
            if (state == 1) {
                p[5] = 4;
                if (p[8] != 0) {
                    music_flagged_track_started = state;
                }
            }
        } else {
            p[5] = 0;
        }
    }
}

/* Stream callback: switch a replaced primary track to playing, or reset the channel on failure. */
void music_primary_replace_callback(int arg0, long arg1) {
    short *p = (short *)(int)arg1;
    if (p != 0) {
        if (arg0 < 0) {
            *(int *)p = arg0;
        }
        if (arg0 != 0) {
            if (p[5] == 9) {
                p[5] = 4;
                if (p[8] != 0) {
                    music_flagged_track_started = 1;
                }
            }
        } else {
            p[5] = 0;
        }
    }
}

/* Stream callback: store the voice handle and mark the primary track started, or reset it. */
void music_primary_start_callback(int arg0, long arg1) {
    short *p = (short *)(int)arg1;
    if (p != 0) {
        *(int *)p = arg0;
        if (arg0 != 0) {
            if (p[5] == 1) {
                p[5] = 8;
            }
        } else {
            p[5] = 0;
        }
    }
}

extern char sound_handle_in_use_message[];

/* `*p = arg0` happens on the 0xFFFFFFFF path too (the older decode made
   it conditional), and D_001E8C60's message is printed. */
void set_sound_handle_id(int arg0, long arg1) {
    short *p = (short *)(int)arg1;
    if (p != 0) {
        if (*(unsigned int *)p != 0xFFFFFFFF) {
            STUB_printf(sound_handle_in_use_message);
        } else {
            *(int *)p = arg0;
            if (arg0 == 0) {
                p[5] = 7;
            }
        }
    }
}

/* Stream callback: save the remaining time and its quarter, then advance the channel. */
void music_remaining_time_callback(int arg0, long arg1) {
    char *p = (char *)(int)arg1;
    short *b;
    if (p == 0) {
        return;
    }
    *(int *)(p + 0x18) = arg0;
    if (*(short *)(p + 0x10) == 0) {
        return;
    }
    b = cd_read_state;
    if (b[0x10] != 1) {
        return;
    }
    if (arg0 == 0) {
        return;
    }
    b[0x10] = 2;
    *(int *)((char *)b + 0x24) = *(int *)(p + 0x18);
    *(int *)((char *)b + 0x28) = *(int *)(p + 0x18) / 4;
}

LINKER_REMNANT("asm/remnants/text", func_00217AD0);

/* Adapted from Lombyte's project (GPL-3.0): src/ui/menus/fun_00216c48.c. */

typedef struct {
    s32 unk0;
    u16 unk4;
    s16 unk6;
    s16 unk8;
    s16 unkA;
    s16 unkC;
    s16 unkE;
    u16 unk10;
} MenuItem;

typedef struct {
    u8 pad0[0x4];
    u16 unk4;
    u8 pad6[0x2];
    u8 unk8;
    u8 pad9[0x2D];
    s16 unk36;
    s32 unk38;
    u8 *unk3C;
} Menu;

typedef struct {
    s32 unk0;
    u8 pad4[0x10];
    u16 unk14;
    u8 pad16[0x2];
} Weapon;

typedef struct {
    char *s[6];
} Suffixes;

extern s32 menu_help_hud_item SDATA(D_00160170);
extern s32 world_update_time SDATA(D_0015F6F0);
extern s32 world_update_time_far __asm__("D_0015F6F0") MACRO_ADDR;
extern s32 menu_entry_trigger_state[];
extern Suffixes menu_time_suffixes;
extern char menu_entry_find_token[];
extern char menu_time_value_format[];
extern s32 g_game_language MACRO_ADDR;
extern Weapon vendor_item_prices[];
extern s32 gBolts MACRO_ADDR;

extern u8 stream_item_flags[] __asm__("D_0013D5C8");
extern u8 items_acquired[];
extern u8 gSpecialItems[];

extern void noop_callback_n(s32);
extern s32 remove_hud_item(s32);
extern void noop_callback_o(s32);
extern void *msg_string(s32);

extern char *find_substring(char *, char *);
extern s32 sprintf(char *, const char *, ...);
extern void copy_text_to_shared_buffer(void *);
extern s32 compute_clamped_count_difference(void);

void process_menu_entry(void *arg0, Menu *menu, s32 arg2);

/* Handle menu cursor moves and build the help text for the selected entry (arg2: cursor moved). */
void process_menu_entry(void *arg0, Menu *menu, s32 arg2) {
    char buf[0x100];
    Suffixes sfx;
    char num[0x20];
    MenuItem *e;
    char *p;
    s32 ok;
    s32 next;
    u16 flags;
    s16 n;
    s16 kind;
    s32 t;

    e = (MenuItem *)(menu->unk3C + menu->unk36 * 0x1C);
    if (menu->unk3C == 0) {
        return;
    }
    if (arg2 != 0) {
        menu->unk4 = menu->unk36;
        menu->unk38 = world_update_time;
        n = e->unk6;
        if (menu->unk36 != n) {
            flags = e->unk10;
            menu->unk36 = e->unk6;
            e = (MenuItem *)(menu->unk3C + n * 0x1C);
            menu->unk8 = *(u8 *)&e->unk10 & 1;
            if (flags & 4) {
                menu_entry_trigger_state[2] = (s32)arg0;
                menu_entry_trigger_state[3] = (s32)menu;
                if (e->unk4 & 0x4000) {
                    noop_callback_n((s16)(e->unk4 ^ 0x4000));
                } else {
                    remove_hud_item(menu_help_hud_item);
                    menu_help_hud_item = -1;
                    noop_callback_o((s16)e->unk4);
                }
            }
        }
    }
    if (menu->unk36 == -1) {
        return;
    }
    menu_entry_trigger_state[5] = e->unk0;
    kind = e->unk8;
    if (kind == 6) {
        sfx = menu_time_suffixes;
        libc_strcpy((Cfg13*)((char *)0x70000000), msg_string(e->unk0));
        p = find_substring(((char *)0x70000000), menu_entry_find_token);
        if (p != 0) {
            p[1] = 's';
        }
        t = vendor_item_prices[e->unkA].unk14;
        sprintf(num, menu_time_value_format, t / 1000, sfx.s[g_game_language % 6], t % 1000);
        sprintf(buf, ((char *)0x70000000), num);
    } else {
        sprintf(buf, msg_string(e->unk0));
    }
    if (menu_entry_trigger_state[5] != 0) {
        copy_text_to_shared_buffer(buf);
    } else if (menu_help_hud_item != -1) {
        remove_hud_item(menu_help_hud_item);
        menu_help_hud_item = -1;
    }
    if (e->unk8 == 0) {
        return;
    }
    ok = 0;
    switch (e->unk8) {
    case 1:
        ok = !(gBolts < vendor_item_prices[e->unkA].unk0);
        break;
    case 2:
        ok = stream_item_flags[e->unkA] != 0;
        break;
    case 3:
        ok = 0;
        if (stream_item_flags[e->unkA] != 0) {
            ok = items_acquired[e->unkA] == 0;
        }
        break;
    case 4:
        ok = gSpecialItems[e->unkA] != 0;
        break;
    case 5:
        ok = !(compute_clamped_count_difference() < e->unkA);
        break;
    case 6:
        ok = gBolts >= vendor_item_prices[e->unkA].unk14 && compute_clamped_count_difference() >= 4;
        break;
    default:
        ok = 0;
        break;
    }
    if (ok) {
        next = e->unkC;
    } else {
        next = e->unkE;
    }
    if (next != menu->unk36) {
        menu->unk36 = next;
        menu->unk38 = world_update_time_far;
        e = (MenuItem *)(menu->unk3C + next * 0x1C);
        menu->unk8 = *(u8 *)&e->unk10 & 1;
        process_menu_entry(arg0, menu, 0);
    }
}

extern char g_controller_state[];

/* Clear the controller state's pending input fields. */
void pad_resetState(void) {
    char *p = g_controller_state;
    *(short *)(p + 0x18E) = 0;
    *(int *)(p + 0x190) = 0;
}

LINKER_REMNANT("asm/remnants/text", func_00217ED8);

extern int pad_socket_param NOT_SDA;
extern void sce_dbc_init(void);
extern void sce_pad2_init(int);
extern int sce_pad2_create_socket(void *, void *);

/* Minimal layouts for init_pads: the port/slot socket parameter (NTSC name unknown)
   and the leading part of PadState from pad.h, up to the fields init_pads writes. */
typedef struct {
    s32 port;
    s32 slot;
} PadSocketParam;

/* Init the pad library and open controller ports 0 and 1. */
void init_pads(void) {
    PadSocketParam *p = (PadSocketParam *)&pad_socket_param;

    sce_dbc_init();
    sce_pad2_init(0);
    pad_socket_param = 2;
    p->slot = 0;
    ((struct PadState *)g_controller_state)->socket = sce_pad2_create_socket(p, g_controller_state);
    pad_socket_param = 2;
    p->slot = 1;
    ((struct PadState *)g_controller_state)->profile_state = 0;
    ((struct PadState *)g_controller_state)->device_state = 0;
}
