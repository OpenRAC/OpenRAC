#include "common.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"
#include "structs/pad.h"
#include "core/sound/989snd.h"
#include "game/moby/mobyutil.h"
#include "game/ui/hud.h"
#include "asm_functions/executable.h"
#include "game/system/stub.h"
#include "game/audio/music.h"
#include "game/ui/help.h"
#include "core/libc/sprintf.h"
#include "core/gs/put_draw_env.h"
#include "core/cdvd/read.h"
#include "core/mc/memory_card.h"
#include "core/libc/find_substring.h"

/*
 * stream.cpp in the original source; text 0x217588-0x217F68.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

typedef struct {
    char b[0x13];
} Cfg13;

extern unsigned char gSpecialItems[];

extern void finish_audio_stream_read(int);
extern short cd_read_state[];

/* snd_StreamSafeCdBreak returns int (989snd.c): with the result in $v0, the
   constant goes to the register retail uses. */
void request_audio_stream_break(void) {
    short *p = cd_read_state;
    if (p[4] != 0) {
        snd_StreamSafeCdBreak();
        ((char *)p)[0xA] = 1;
    }
}

typedef struct {
    char b[4];
} StreamHdr;
extern StreamHdr cd_read_request_mode_template;
/* MACRO_ADDR for retail's one-register lbu; the explicit size 4 (over
   -G2) is what keeps the assembler from expanding it through $gp. */
extern unsigned char cd_read_spindle_speed[4] MACRO_ADDR;
extern int cd_read_wait_frames MACRO_ADDR;
extern int cd_read_slowdown_applied MACRO_ADDR;

/* Start a CD stream read of arg2 sectors from arg1 into arg0 at the current spindle speed. */
int submit_audio_stream_io_request(int arg0, int arg1, int arg2) {
    StreamHdr hdr = cd_read_request_mode_template;

    hdr.b[1] = cd_read_spindle_speed[0];
    cd_read_wait_frames = 0;
    cd_read_slowdown_applied = 0;
    sce_cd_read(arg1, arg2, arg0, &hdr);
    return 1;
}

extern char cd_read_start_failure_message[];

/* Start a stream: refuse if one is already running (+0x8) or the length
   is zero, otherwise hand the request to snd_StreamSafeCdRead with the header
   block at cd_read_state+0x30 and record it. Returns the length in the
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
        STUB_printf(cd_read_start_failure_message);
        raise_kernel_trap();
        return 0;
    }
    *(int *)(d + 0x14) = arg0;
    *(short *)(d + 8) = 1;
    *(int *)(d + 0xC) = arg1;
    *(int *)(d + 0x10) = arg2;
    return arg2 << 11;
}

/*
 * Close, not exact (15/124), same size so inert. Semantics confirmed:
 * call start_audio_stream_read, and while it succeeded and the flag at
 * cd_read_state+0x8 stays set, pump the five service calls; return the
 * original result either way.
 *
 * Residual is purely which callee-saved register holds what: retail puts
 * the result in $s0 and the cd_read_state base in $s1, this compiler the
 * other way round, and the prologue save order follows from that.
 * Tried and did not move it: swapping the two local declarations (the
 * documented declaration-order lever -- it does not apply when a local
 * is initialised straight from a call, since the live range starts at
 * the call regardless), and restructuring the loop as guard + do/while
 * so the base local is assigned late exactly as retail does. Allocator
 * destination-choice question, not source shape.
 */
/* Load: call start_audio_stream_read and, if it succeeded, keep pumping the five
   service calls while the cd_read_state+0x8 state word stays nonzero;
   return start_audio_stream_read's result either way.
   As in update_audio_stream_until_idle below, the base pointer is assigned in the loop
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

/* Same pump loop as Load above, gated on the argument instead
   of on a call's result: with arg0 set, keep servicing while the
   cd_read_state+8 state word stays nonzero; with it clear, run the four
   service calls exactly once. Either way return the state word. */
/* With arg0 set, keep pumping the five service calls while the
   cd_read_state+8 state word stays nonzero; with it clear, run the four that
   are not sce_gs_sync_v once. Either way return the state word.
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

extern char sound_handle_id_conflict_diagnostic[];

/* `*p = arg0` happens on the 0xFFFFFFFF path too (the older decode made
   it conditional), and D_001E8C60's message is printed. */
void set_sound_handle_id(int arg0, long arg1) {
    short *p = (short *)(int)arg1;
    if (p != 0) {
        if (*(unsigned int *)p != 0xFFFFFFFF) {
            STUB_printf(sound_handle_id_conflict_diagnostic);
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

extern s32 menu_help_hud_item SDATA(menu_help_hud_item);
extern s32 world_update_time SDATA(world_update_time);
extern s32 world_update_time_far __asm__("D_0015F6F0") MACRO_ADDR;
extern s32 menu_entry_context[];
extern Suffixes menu_bolt_threshold_thousands_separators;
extern char help_text_integer_placeholder[];
extern char bolt_threshold_number_format[];
extern s32 g_game_language MACRO_ADDR;
extern Weapon vendor_item_prices[];
extern s32 gBolts MACRO_ADDR;

extern u8 item_available[];
extern u8 items_acquired[];
extern u8 gSpecialItems[];

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
                menu_entry_context[2] = (s32)arg0;
                menu_entry_context[3] = (s32)menu;
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
    menu_entry_context[5] = e->unk0;
    kind = e->unk8;
    if (kind == 6) {
        sfx = menu_bolt_threshold_thousands_separators;
        libc_strcpy((Cfg13 *)((char *)0x70000000), msg_string(e->unk0));
        p = find_substring(((char *)0x70000000), help_text_integer_placeholder);
        if (p != 0) {
            p[1] = 's';
        }
        t = vendor_item_prices[e->unkA].unk14;
        sprintf(num, bolt_threshold_number_format, t / 1000, sfx.s[g_game_language % 6], t % 1000);
        sprintf(buf, ((char *)0x70000000), num);
    } else {
        sprintf(buf, msg_string(e->unk0));
    }
    if (menu_entry_context[5] != 0) {
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
            ok = item_available[e->unkA] != 0;
            break;
        case 3:
            ok = 0;
            if (item_available[e->unkA] != 0) {
                ok = items_acquired[e->unkA] == 0;
            }
            break;
        case 4:
            ok = gSpecialItems[e->unkA] != 0;
            break;
        case 5:
            ok = !(count_unspent_gold_bolts() < e->unkA);
            break;
        case 6:
            ok = gBolts >= vendor_item_prices[e->unkA].unk14 && count_unspent_gold_bolts() >= 4;
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
void reset_pad_history(void) {
    char *p = g_controller_state;
    *(short *)(p + 0x18E) = 0;
    *(int *)(p + 0x190) = 0;
}

LINKER_REMNANT("asm/remnants/text", func_00217ED8);

extern int controller_socket_params NOT_SDA;

extern void sce_pad2_init_v(int) __asm__("func_00124B88");

/* Minimal layouts for init_pads: the port/slot socket parameter (NTSC name unknown)
   and the leading part of PadState from structs/pad.h, up to the fields init_pads writes. */
typedef struct {
    s32 port;
    s32 slot;
} PadSocketParam;

/* Init the pad library and open controller ports 0 and 1. */
void init_pads(void) {
    PadSocketParam *p = (PadSocketParam *)&controller_socket_params;

    sce_dbc_init();
    sce_pad2_init_v(0);
    controller_socket_params = 2;
    p->slot = 0;
    ((struct PadState *)g_controller_state)->socket = sce_pad2_create_socket(p, g_controller_state);
    controller_socket_params = 2;
    p->slot = 1;
    ((struct PadState *)g_controller_state)->profile_state = 0;
    ((struct PadState *)g_controller_state)->device_state = 0;
}
