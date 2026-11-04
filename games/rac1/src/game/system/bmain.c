#include "common.h"
#include "structs/memcard.h"
#include "structs/disc.h"
#include "structs/pad.h"
#include "structs/misc.h"
#include "core/startup/permcb.h"
#include "game/system/memcard.h"
#include "game/audio/music.h"
#include "game/audio/sound.h"
#include "asm_functions/executable.h"
#include "game/render/draw.h"
#include "game/render/vuchain.h"
#include "game/ui/hud.h"
#include "game/system/stub.h"
#include "core/sound/989snd.h"
#include "game/render/framebuf.h"
#include "core/gs/put_draw_env.h"
#include "core/cdvd/callbacks.h"
#include "core/gs/vif1_gif_reset.h"
#include "game/system/space.h"
#include "core/gs/dma_and_sync_v.h"
#include "game/movie/movie.h"
#include "game/ui/transition.h"
#include "game/system/initonce.h"
#include "game/gameplay/actuator.h"
#include "game/system/pad.h"

/*
 * bmain.cpp in the original source; text 0x1E9808-0x1E9E70.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

extern int pal_mode MACRO_ADDR;
extern int decode_mode MACRO_ADDR;
extern int g_game_mode MACRO_ADDR;
extern int depth_buffer_address MACRO_ADDR;
typedef struct {
    int b;
    int a;
} LevelLoad;
typedef struct {
    char pad0[0x1A78];
    LevelLoad normal[4];
    LevelLoad alt[4];
} Globals137C80;
extern Globals137C80 disc_table_g __asm__("D_00137C80");
extern char voice_pool[];
extern char memory_card_state[];
extern char level_resource_buffers[];

/* Start level `level`: pick its two load parameters from the level
   table in disc_table (the alternate set while pal_mode is set), stop
   sound and wait for the loader (memcard_update_state) to go idle, then load it
   (Movie_Play) and reset the display state. */
void start_level(int level) {
    int a;
    int b;
    int t;

    if (level < 0) {
        return;
    }
    if (pal_mode != 0) {
        b = disc_table_g.alt[level].b;
        a = disc_table_g.alt[level].a;
    } else {
        b = disc_table_g.normal[level].b;
        a = disc_table_g.normal[level].a;
    }
    decode_mode = 2;
    voice_pool[0x6B] |= 8;
    flush_cache(0);
    sound_StopAllSounds();
    music_Stop();
    FadeToBlack(scale_game_frame_count(12));
    g_game_mode = 1;
    flush_cache(0);
    sound_StopAllSounds();
    music_Stop();
    snd_StreamSafeCdSync(0);
    for (;;) {
        struct MemoryCardState *ld = (struct MemoryCardState *)memory_card_state;
        if (ld->state < 3 && ld->err < 0) {
            break;
        }
        memcard_update_state();
    }
    {
        char *g = level_resource_buffers;
        t = *(int *)(g + 0x1C);
    }
    Movie_Play(b, a, t + 0x100000, t + 0x400000, 0);
    sce_cd_sync_s(0);
    sce_gs_sync_v(0);
    wait_for_graphics_pipeline_idle(0, 0);
    sceGsSyncVCallback(vsync_callback);
    Hud_sendTexture(0x1000000, depth_buffer_address, 0x1B, 6, 6, 1);
    decode_mode = 0;
    FadeToBlack(4);
    g_game_mode = 0;
    voice_pool[0x6B] |= 0x10;
}

/* Pad state, as far as this function reads it. */

extern s32 D_0015EE88 MACRO_ADDR;

extern struct DiscTable disc_table_r __asm__("D_00137C80");

typedef struct {
    s32 off;
    s32 size;
} LevelChunk;

typedef struct {
    s32 gfx;
    s32 pad4;
    s32 gfx_alt;
    s32 padC;
    LevelChunk intro[6];
    LevelChunk loading[6];
    s32 code;
} LevelHeader;

typedef struct {
    s32 pad[7];
    s32 bank;
} SoundSlot;

extern s32 D_0015F6C8 MACRO_ADDR;
extern u8 D_00161380[];
extern u8 D_00165530[];
extern s32 D_0015F538 MACRO_ADDR;
extern s32 g_game_mode_r __asm__("D_0015F6E8") MACRO_ADDR;
extern s32 D_0015F05C MACRO_ADDR;
extern s32 decode_mode_r __asm__("D_0015EFD8") MACRO_ADDR;
extern s32 pal_mode_r __asm__("D_0015EE80") MACRO_ADDR;
extern s32 D_0015EE84 MACRO_ADDR;
extern u8 D_0024272F[];
extern struct PadState D_0013CA40;
extern s32 D_00139478[];
extern s32 D_00139480[];
extern char D_001E79C0[];
extern SoundSlot D_00186200[];
extern SoundSlot D_001862E0[];
extern SoundSlot *D_0015F714 MACRO_ADDR;
extern s32 D_0015F710 MACRO_ADDR;
extern u8 D_0016044C[] MACRO_ADDR;
extern s32 D_0015F6E4 MACRO_ADDR;
extern u8 D_0013E130[];

extern void SetBackgroundColor_r(s32, u64, u64) __asm__("func_001FB448");

extern void Movie_Play_r(s32, s32, s32, s32, s32) __asm__("func_0023B670");

extern s32 reset_vif1_gif_r(void) __asm__("func_001207B8");
extern void sceGsResetGraph_r(s16, s16, s16, s16) __asm__("func_00121B78");

/* Start-up sequence before the first level: clears the level bss, shows the intro and
   loading images of the chosen language (PAL fades each in over eight frames) until they end
   or a button is pressed, plays the intro movie, shows the loading image, loads the sound
   bank and files it in the two sound slot tables, applies the video mode setting and starts
   the level load.
   The slot stores are in this order on purpose: the last store through each table, the last
   one of the bank value and the two globals are what the scheduler moves to the front.
   Adapted from Lombyte's project (GPL-3.0) for PAL: src/textbin/world/startlevel.c, startlevel. */
void startlevel(void) {
    LevelHeader *hdr;
    LevelChunk *tbl;
    s32 n;
    s32 i;
    s32 cur;
    s32 prev;
    s32 frames;
    s32 code;
    s32 bank;
    s32 fade;
    u8 *p;

    boot_level_active = 1;
    InitOnce();
    initialize_streaming_state();
    for (i = 0; i < startlevel_clear_end - menu_state_frame_count; i++) {
        menu_state_frame_count[i] = 0;
    }
    prev = 0;
    frames = 0;
    fade = 0x80;
    VU1_initChain();
    DMAC_VIF1_Enable();
    clear_dma_queue_entry();
    game_frame_counter = 0;
    sce_gs_sync_v(0);
    g_game_mode_r = 0;
    ResetGsRegisters();
    ResetGsRegistersPr();
    PutDispBuffer();
    SetBackgroundColor_r(0, 0, 0);
    hdr = (LevelHeader *)(((u32)image_end_round_16k & 0xFFFFC000) + 0x2C0000);
    VU1_initChain();
    while ((cur = check_memory_card()) != 0 && (frames < 11 || g_controller_state.pressed == 0)) {
        if (cur != prev) {
            if (cur == 1) {
                tbl = hdr->intro;
            } else {
                tbl = hdr->loading;
            }
            flush_cache(0);
            fast_decompress(tbl[g_game_language].off + (s32)hdr, hdr->code + (s32)hdr);
            flush_cache(0);
        }
        SetBackgroundColor_r(0, 0, 0);
        PutDrawBufferLarge();
        append_gif_transfer_packet();
        queue_framebuffer_upload(hdr->code + (s32)hdr);
        if (frames < 8) {
            VU1_addGSregister(0x42, 0x8000000044);
            emit_rgba_draw_packet(0, 0, 0, fade);
        }
        PutDrawBufferSmall();
        append_palette_transfer_packet();
        VU1_sendChain();
        VU1_swapChain();
        VU1_syncChain(1);
        sce_gs_sync_v(0);
        prev = cur;
        fade -= 0x10;
        UpdatePad();
        frames++;
    }
    if (prev != 0) {
        FadeToBlack(8);
    }
    decode_mode_r = -1;
    code = hdr->code + (s32)hdr;
    startlevel_code_addr = code;
    if (pal_mode_r == 0) {
        Movie_Play_r(intro_movie_disc_entry_ntsc[0], intro_movie_disc_entry_ntsc[1], (code + 0x3F) & ~0x3F,
                     (code + 0x2C003F) & ~0x3F, 0);
    } else {
        Movie_Play_r(intro_movie_disc_entry_pal[0], intro_movie_disc_entry_pal[1], (code + 0x3F) & ~0x3F,
                     (code + 0x2C003F) & ~0x3F, 0);
    }
    decode_mode_r = 0;
    FadeToBlack(scale_game_frame_count(0xC));
    flush_cache(0);
    if (pal_mode_r != 0) {
        fast_decompress(hdr->gfx_alt + (s32)hdr, hdr->code + (s32)hdr);
    } else {
        fast_decompress(hdr->gfx + (s32)hdr, hdr->code + (s32)hdr);
    }
    flush_cache(0);
    VU1_initChain();
    SetBackgroundColor_r(0, 0, 0);
    PutDrawBufferLarge();
    append_gif_transfer_packet();
    draw_boot_image(hdr->code + (s32)hdr);
    PutDrawBufferSmall();
    append_palette_transfer_packet();
    VU1_sendChain();
    VU1_swapChain();
    VU1_syncChain(1);
    sce_gs_sync_v(0);
    game_frame_counter++;
    STUB_printf(str_loading_sound_bank);
    bank = load_audio_bank_by_location(disc_table_r.sound_bank);
    snd_ResolveBankXREFS();
    startlevel_sound_defs[0].bank = bank;
    startlevel_sound_defs[1].bank = bank;
    startlevel_sound_defs[2].bank = bank;
    startlevel_sound_defs[3].bank = bank;
    startlevel_sound_defs[4].bank = bank;
    startlevel_sound_defs[5].bank = bank;
    moby_class_472_sound_defs[0].bank = bank;
    moby_class_472_sound_defs[1].bank = bank;
    moby_class_472_sound_defs[2].bank = bank;
    moby_class_472_sound_defs[3].bank = bank;
    moby_class_472_sound_defs[4].bank = bank;
    startlevel_sound_defs[6].bank = bank;
    level_sound_defs = startlevel_sound_defs;
    level_sound_def_count = 7;
    transition_do_transition();
    if (pal_mode_r != requested_video_mode[0]) {
        pal_mode_r = requested_video_mode[0];
        reset_vif1_gif_r();
        sceGsResetGraph_r(0, 1, pal_mode_r != 0 ? 3 : 2, 0);
        SetPalMode();
        PutDispBuffer();
    }
    p = level_render_state;
    space_load_level = g_current_level_index;
    *(s16 *)(p + 0x2A) = 1;
    g_current_level_index = -1;
    DoSpaceTransition();
}
