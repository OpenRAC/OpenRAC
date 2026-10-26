#include "common.h"
#include "pad.h"
#include "structs.h"

/*
 * bmain.cpp in the original source; text 0x1E9808-0x1E9E70.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
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
extern void flush_cache(int);
extern void sound_StopAllSounds(void);
extern void music_Stop(void);
extern int scale_game_frame_count(int);
extern void FadeToBlack(int);
extern void snd_StreamSafeCdSync(int);
extern void memcard_update_state(void);
extern void Movie_Play(int, int, int, int, int);
extern void sce_cd_sync_s(int);
extern void sce_gs_sync_v(int);
extern void wait_for_graphics_pipeline_idle(int, int);
extern void vsync_callback(void);
extern void sceGsSyncVCallback(void (*)(void));
extern void Hud_sendTexture(int, int, int, int, int, int);

/* Start level `level`: pick its two load parameters from the level
   table in D_00137C80 (the alternate set while D_0015EE80 is set), stop
   sound and wait for the loader (func_00209E68) to go idle, then load it
   (func_0023B670) and reset the display state. */
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
        char *ld = memory_card_state;
        if (*(int *)(ld + 0xDC) < 3 && *(int *)(ld + 0xE4) < 0) {
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

extern s32 g_game_language_r __asm__("D_0015EE88") MACRO_ADDR;

struct DiscTable_E99D8 {
    u8 pad_0[0x14E0];
    s32 sound_bank;                        /* 0x14E0 */
};

extern struct DiscTable_E99D8 disc_table_r __asm__("D_00137C80");

typedef struct {
    s32 off;
    s32 size;
} LevelChunk_E99D8;

typedef struct {
    s32 gfx;
    s32 pad4;
    s32 gfx_alt;
    s32 padC;
    LevelChunk_E99D8 intro[6];
    LevelChunk_E99D8 loading[6];
    s32 code;
} LevelHeader_E99D8;

typedef struct {
    s32 pad[7];
    s32 bank;
} SoundSlot_E99D8;

extern s32 boot_level_active_r __asm__("D_0015F6C8") MACRO_ADDR;
extern u8 menu_state_frame_count_r[] __asm__("D_00161380");
extern u8 startlevel_clear_end_r[] __asm__("D_00165530");
extern s32 game_frame_counter_r __asm__("D_0015F538") MACRO_ADDR;
extern s32 g_game_mode_r __asm__("D_0015F6E8") MACRO_ADDR;
extern s32 startlevel_code_addr_r __asm__("D_0015F05C") MACRO_ADDR;
extern s32 decode_mode_r __asm__("D_0015EFD8") MACRO_ADDR;
extern s32 pal_mode_r __asm__("D_0015EE80") MACRO_ADDR;
extern s32 g_current_level_index_r __asm__("D_0015EE84") MACRO_ADDR;
extern u8 image_end_round_16k_r[] __asm__("D_0024272F");
extern struct PadState g_controller_state_r __asm__("D_0013CA40");
extern s32 intro_movie_disc_entry_ntsc_r[] __asm__("D_00139478");
extern s32 intro_movie_disc_entry_pal_r[] __asm__("D_00139480");
extern char str_loading_sound_bank_r[] __asm__("D_001E79C0");
extern SoundSlot_E99D8 startlevel_sound_defs_r[] __asm__("D_00186200");
extern SoundSlot_E99D8 moby_class_472_sound_defs_r[] __asm__("D_001862E0");
extern SoundSlot_E99D8 *level_sound_defs_r __asm__("D_0015F714") MACRO_ADDR;
extern s32 level_sound_def_count_r __asm__("D_0015F710") MACRO_ADDR;
extern u8 requested_video_mode_r[] __asm__("D_0016044C") MACRO_ADDR;
extern s32 space_load_level_r __asm__("D_0015F6E4") MACRO_ADDR;
extern u8 level_render_state_r[] __asm__("D_0013E130");

extern void InitOnce_r(void) __asm__("func_00201E88");
extern void initialize_streaming_state_r(void) __asm__("func_001E94A8");
extern void VU1_initChain_r(void) __asm__("func_002348E8");
extern void DMAC_VIF1_Enable_r(void) __asm__("func_00235018");
extern void clear_dma_queue_entry_r(void) __asm__("func_001F0F30");
extern s32 sce_gs_sync_v_r(s32) __asm__("func_00122598");
extern void PutDispBuffer_r(void) __asm__("func_001FB470");
extern void ResetGsRegisters_r(void) __asm__("func_001F3C10");
extern void ResetGsRegistersPr_r(void) __asm__("func_001F3D00");
extern void SetBackgroundColor_r(s32, u64, u64) __asm__("func_001FB448");
extern void flush_cache_r(s32) __asm__("func_00118D80");
extern void fast_decompress_r(s32, s32) __asm__("func_0020C468");
extern void PutDrawBufferLarge_r(void) __asm__("func_001FB498");
extern void append_gif_transfer_packet_r(void) __asm__("func_001FB530");
extern void draw_boot_image_r(s32) __asm__("func_00201AF0");
extern void queue_framebuffer_upload_r(s32) __asm__("func_001F7680");
extern void VU1_addGSregister_r(s32, u64) __asm__("func_00234C98");
extern void emit_rgba_draw_packet_r(s32, s32, s32, s32) __asm__("func_001F55C0");
extern void PutDrawBufferSmall_r(void) __asm__("func_001FB598");
extern void append_palette_transfer_packet_r(void) __asm__("func_001FB8A8");
extern void VU1_sendChain_r(void) __asm__("func_002349B8");
extern void VU1_swapChain_r(void) __asm__("func_00234948");
extern void VU1_syncChain_r(s32) __asm__("func_00234AC8");
extern s32 check_memory_card_r(void) __asm__("func_00209BB8");
extern void UpdatePad_r(void) __asm__("func_00218908");
extern void FadeToBlack_r(s32) __asm__("func_001F4E08");
extern void Movie_Play_r(s32, s32, s32, s32, s32) __asm__("func_0023B670");
extern s32 scale_game_frame_count_r(s32) __asm__("func_001F98C0");
extern void STUB_printf_r(char *, ...) __asm__("func_001E9730");
extern s32 load_audio_bank_by_location_r(s32) __asm__("func_0022EA20");
extern void snd_ResolveBankXREFS_r(void) __asm__("func_0012E2E8");
extern s32 transition_do_transition_r(void) __asm__("func_001EBB48");
extern s32 reset_vif1_gif_r(void) __asm__("func_001207B8");
extern void sceGsResetGraph_r(s16, s16, s16, s16) __asm__("func_00121B78");
extern void SetPalMode_r(void) __asm__("func_001F3890");
extern void DoSpaceTransition_r(void) __asm__("func_00233308");

/* Start-up sequence before the first level: clears the level bss, shows the intro and
   loading images of the chosen language (PAL fades each in over eight frames) until they end
   or a button is pressed, plays the intro movie, shows the loading image, loads the sound
   bank and files it in the two sound slot tables, applies the video mode setting and starts
   the level load.
   The slot stores are in this order on purpose: the last store through each table, the last
   one of the bank value and the two globals are what the scheduler moves to the front.
   Adapted from Lombyte's project (GPL-3.0) for PAL: src/textbin/world/startlevel.c, startlevel. */
void startlevel(void) {
    LevelHeader_E99D8 *hdr;
    LevelChunk_E99D8 *tbl;
    s32 n;
    s32 i;
    s32 cur;
    s32 prev;
    s32 frames;
    s32 code;
    s32 bank;
    s32 fade;
    u8 *p;

    boot_level_active_r = 1;
    InitOnce_r();
    initialize_streaming_state_r();
    for (i = 0; i < startlevel_clear_end_r - menu_state_frame_count_r; i++) {
        menu_state_frame_count_r[i] = 0;
    }
    prev = 0;
    frames = 0;
    fade = 0x80;
    VU1_initChain_r();
    DMAC_VIF1_Enable_r();
    clear_dma_queue_entry_r();
    game_frame_counter_r = 0;
    sce_gs_sync_v_r(0);
    g_game_mode_r = 0;
    ResetGsRegisters_r();
    ResetGsRegistersPr_r();
    PutDispBuffer_r();
    SetBackgroundColor_r(0, 0, 0);
    hdr = (LevelHeader_E99D8 *)(((u32)image_end_round_16k_r & 0xFFFFC000) + 0x2C0000);
    VU1_initChain_r();
    while ((cur = check_memory_card_r()) != 0 && (frames < 11 || g_controller_state_r.pressed == 0)) {
        if (cur != prev) {
            if (cur == 1) {
                tbl = hdr->intro;
            } else {
                tbl = hdr->loading;
            }
            flush_cache_r(0);
            fast_decompress_r(tbl[g_game_language_r].off + (s32)hdr, hdr->code + (s32)hdr);
            flush_cache_r(0);
        }
        SetBackgroundColor_r(0, 0, 0);
        PutDrawBufferLarge_r();
        append_gif_transfer_packet_r();
        queue_framebuffer_upload_r(hdr->code + (s32)hdr);
        if (frames < 8) {
            VU1_addGSregister_r(0x42, 0x8000000044);
            emit_rgba_draw_packet_r(0, 0, 0, fade);
        }
        PutDrawBufferSmall_r();
        append_palette_transfer_packet_r();
        VU1_sendChain_r();
        VU1_swapChain_r();
        VU1_syncChain_r(1);
        sce_gs_sync_v_r(0);
        prev = cur;
        fade -= 0x10;
        UpdatePad_r();
        frames++;
    }
    if (prev != 0) {
        FadeToBlack_r(8);
    }
    decode_mode_r = -1;
    code = hdr->code + (s32)hdr;
    startlevel_code_addr_r = code;
    if (pal_mode_r == 0) {
        Movie_Play_r(intro_movie_disc_entry_ntsc_r[0], intro_movie_disc_entry_ntsc_r[1], (code + 0x3F) & ~0x3F,
                        (code + 0x2C003F) & ~0x3F, 0);
    } else {
        Movie_Play_r(intro_movie_disc_entry_pal_r[0], intro_movie_disc_entry_pal_r[1], (code + 0x3F) & ~0x3F,
                        (code + 0x2C003F) & ~0x3F, 0);
    }
    decode_mode_r = 0;
    FadeToBlack_r(scale_game_frame_count_r(0xC));
    flush_cache_r(0);
    if (pal_mode_r != 0) {
        fast_decompress_r(hdr->gfx_alt + (s32)hdr, hdr->code + (s32)hdr);
    } else {
        fast_decompress_r(hdr->gfx + (s32)hdr, hdr->code + (s32)hdr);
    }
    flush_cache_r(0);
    VU1_initChain_r();
    SetBackgroundColor_r(0, 0, 0);
    PutDrawBufferLarge_r();
    append_gif_transfer_packet_r();
    draw_boot_image_r(hdr->code + (s32)hdr);
    PutDrawBufferSmall_r();
    append_palette_transfer_packet_r();
    VU1_sendChain_r();
    VU1_swapChain_r();
    VU1_syncChain_r(1);
    sce_gs_sync_v_r(0);
    game_frame_counter_r++;
    STUB_printf_r(str_loading_sound_bank_r);
    bank = load_audio_bank_by_location_r(disc_table_r.sound_bank);
    snd_ResolveBankXREFS_r();
    startlevel_sound_defs_r[0].bank = bank;
    startlevel_sound_defs_r[1].bank = bank;
    startlevel_sound_defs_r[2].bank = bank;
    startlevel_sound_defs_r[3].bank = bank;
    startlevel_sound_defs_r[4].bank = bank;
    startlevel_sound_defs_r[5].bank = bank;
    moby_class_472_sound_defs_r[0].bank = bank;
    moby_class_472_sound_defs_r[1].bank = bank;
    moby_class_472_sound_defs_r[2].bank = bank;
    moby_class_472_sound_defs_r[3].bank = bank;
    moby_class_472_sound_defs_r[4].bank = bank;
    startlevel_sound_defs_r[6].bank = bank;
    level_sound_defs_r = startlevel_sound_defs_r;
    level_sound_def_count_r = 7;
    transition_do_transition_r();
    if (pal_mode_r != requested_video_mode_r[0]) {
        pal_mode_r = requested_video_mode_r[0];
        reset_vif1_gif_r();
        sceGsResetGraph_r(0, 1, pal_mode_r != 0 ? 3 : 2, 0);
        SetPalMode_r();
        PutDispBuffer_r();
    }
    p = level_render_state_r;
    space_load_level_r = g_current_level_index_r;
    *(s16 *)(p + 0x2A) = 1;
    g_current_level_index_r = -1;
    DoSpaceTransition_r();
}
