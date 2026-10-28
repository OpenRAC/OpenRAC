#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"
#include "music.h"

/*
 * music.cpp in the original source; text 0x2161E0-0x217588.
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

extern short cd_read_state[];
extern int snd_InitVAGStreamingEx(int, int, int, int);
extern int snd_FlushSoundCommands(void);
extern void register_audio_stream_callback(void);

/* Music init: resets the music state at D_001517D0 (+0x30 = 0x20, the
   other bytes and the three playing records' state and +0x0A flag
   cleared, +0x1C/+0x22 set to -1), starts VAG streaming through
   func_0012EC60 (snd_InitVAGStreamingEx), waits until func_0012DDC0
   (snd_FlushSoundCommands) has nothing left, and calls func_00216270.
   The two -1 stores are one chained assignment written last: that
   shares one register between the word and the byte store, and leaves
   the other stores in source order, as retail has them. */
void reset_music(void) {
    char *s = (char *)cd_read_state;

    *(char *)(s + 0x30) = 0x20;
    *(int *)(s + 0x00) = 0;
    *(char *)(s + 0x31) = 0;
    *(char *)(s + 0x32) = 0;
    *(char *)(s + 0x33) = 0;
    *(int *)(s + 0x34) = 0;
    *(short *)(s + 0x3E) = 0;
    *(int *)(s + 0x50) = 0;
    *(short *)(s + 0x5A) = 0;
    *(int *)(s + 0x6C) = 0;
    *(short *)(s + 0x76) = 0;
    *(char *)(s + 0x22) = *(int *)(s + 0x1C) = -1;
    snd_InitVAGStreamingEx(4, 0xF000, 0, 1);
    while (snd_FlushSoundCommands() != 0) {
    }
    register_audio_stream_callback();
}

extern void snd_StreamSafeCdCallback(void *);
extern void finish_audio_stream_read(int);

/* Register the CD read callback used by the music stream. */
void register_audio_stream_callback(void) {
    snd_StreamSafeCdCallback(finish_audio_stream_read);
}

/* The sound tables in the WAD table of contents at D_00137C80, as far as
   the play functions show them: 8-byte records per sound id whose first
   word is the stream handle, except the 50000 range, which has six
   handles per id (D_0015EE88 picks one). The [1] bounds are placeholders,
   not known counts. Reaching a table as a member of this struct, rather
   than by pointer arithmetic, is what gives retail's addu operand order.

   D_001517D0 holds three playing records (music_Playing, 0x1C bytes)
   at 0x34, 0x50 and 0x6C: +0x00 state (-1 while starting), +0x04 the
   id, +0x06 the value also passed as func_0012ED48's fifth argument,
   +0x08 another argument, +0x0A and +0x10 flags, +0x14 10, +0x18 48000
   (the sample rate). The stores are written in the order retail emits
   them. The first one must not need a constant in a register: if it
   does, the allocator spends $a0 on the constant, and the handle can no
   longer load straight into $a0. An older note here recorded that
   residual as an allocator dead end. */
typedef struct {
    int handle;
    int unk_04;
} SndRec;
typedef struct {
    char _pad0000[0x1A0];
    SndRec ids10000[1]; /* 0x01A0 */
    char _pad01A8[0xF00 - 0x1A8];
    int ids50000[1][6]; /* 0x0F00 */
    char _pad0F18[0x13C0 - 0xF18];
    SndRec ids40000[1]; /* 0x13C0 */
    char _pad13C8[0x1618 - 0x13C8];
    SndRec ids60000[1]; /* 0x1618 */
    char _pad1620[0x2988 - 0x1620];
    SndRec ids20000[1]; /* 0x2988 */
} SndToc;
extern int disc_table[];
extern short cd_read_state[];
extern int g_game_language MACRO_ADDR;
extern void music_secondary_start_callback(int, long);
extern void snd_PlayVAGStreamByLocEx_CB(int, int, int, int, short, int, int, int,
                          int, void (*)(int, long), long);

/* Plays sound arg0 in 60000..: the other play functions below differ
   only in their table. */
typedef struct {
    u32 handle;                 /* 0x00: stream handle; 0xFFFFFFFF while starting (unsigned, as retail builds it) */
    s16 track;                  /* 0x04 */
    s16 volume;                 /* 0x06 */
    s16 flags;                  /* 0x08 */
    s16 state;                  /* 0x0A: set 1 on start */
    s16 fade_flags;             /* 0x0C */
    s16 unkE;                   /* 0x0E */
    s16 crossfade_enabled;      /* 0x10 */
    u8 pad_12[0x2];
    s32 poll_interval;          /* 0x14 */
    s32 remaining_time;         /* 0x18 */
} SndSecondaryChannel;

/* 0x1C */

typedef struct {
    u8 pad_00[0x50];
    SndSecondaryChannel secondary;  /* 0x50: the secondary record in D_001517D0 */
} SndStreamBlock;

/* Start music track 60000+ on the secondary stream channel if that channel is free. */
void music_start_track_60000(int arg0, int arg1, int arg2) {
    SndToc *toc = (SndToc *)disc_table;
    int i = arg0 - 60000;
    int h;
    SndStreamBlock *s;

    h = toc->ids60000[i].handle;
    if (h == 0) {
        return;
    }
    s = (SndStreamBlock *)cd_read_state;
    if (s->secondary.handle != 0) {
        return;
    }
    s->secondary.handle = 0xFFFFFFFF;
    s->secondary.state = 1;
    s->secondary.track = arg0;
    s->secondary.flags = arg1;
    s->secondary.poll_interval = 10;
    s->secondary.remaining_time = 0xBB80;
    s->secondary.volume = arg2;
    s->secondary.crossfade_enabled = 0;
    snd_PlayVAGStreamByLocEx_CB(h, 0, 0, 0, arg2, 0, 2, 0, 0x21,
                  music_secondary_start_callback, (long)(unsigned int)&s->secondary);
}

typedef struct {
    u8 pad_00[0x50];
    struct MusicStreamChannel secondary;     /* 0x50 */
} TrackStreamState;

typedef struct {
    u8 pad_0000[0xF00];
    s32 music_50000[40][6];     /* 0xF00: tracks 50000.., one handle per language */
} TrackDiscTable;

/* Start music track 50000+ (per-language) on the secondary stream channel if it is free. */
void music_start_track_50000(int track, int flags, int volume) {
    TrackDiscTable *track_disc_table = (TrackDiscTable *)disc_table;
    int i = track - 50000;
    int handle;
    TrackStreamState *music_stream_state;

    handle = track_disc_table->music_50000[i][g_game_language];
    if (handle == 0) {
        return;
    }
    music_stream_state = (TrackStreamState *)cd_read_state;
    if (music_stream_state->secondary.handle != 0) {
        return;
    }
    *(u32 *)&music_stream_state->secondary.handle = 0xFFFFFFFF;
    music_stream_state->secondary.state = 1;
    music_stream_state->secondary.track = track;
    music_stream_state->secondary.flags = flags;
    music_stream_state->secondary.poll_interval = 10;
    music_stream_state->secondary.remaining_time = 48000;
    music_stream_state->secondary.volume = volume;
    music_stream_state->secondary.crossfade_enabled = 0;
    snd_PlayVAGStreamByLocEx_CB(handle, 0, 0, 0, (s16)volume, 0, 2, 0, 0x21,
                  music_secondary_start_callback,
                  (long)(unsigned int)&music_stream_state->secondary);
}

typedef struct {
    u8 pad_00[0x50];
    struct MusicStreamChannel secondary;
} MusicStreamTail;

/* Start music track 40000+ on the secondary stream channel if that channel is free. */
void music_start_track_40000(int track, int track_flags, int volume) {
    SndToc *toc = (SndToc *)disc_table;
    int i = track - 40000;
    int location;
    MusicStreamTail *s;

    location = toc->ids40000[i].handle;
    if (location == 0) {
        return;
    }
    s = (MusicStreamTail *)cd_read_state;
    if (s->secondary.handle != 0) {
        return;
    }
    *(unsigned int *)&s->secondary.handle = 0xFFFFFFFF;
    s->secondary.state = 1;
    s->secondary.track = track;
    s->secondary.flags = track_flags;
    s->secondary.poll_interval = 10;
    s->secondary.remaining_time = 0xBB80;
    s->secondary.volume = volume;
    s->secondary.crossfade_enabled = 0;
    snd_PlayVAGStreamByLocEx_CB(location, 0, 0, 0, volume, 0, 2, 0, 0x21,
                  music_secondary_start_callback, (long)(unsigned int)&s->secondary);
}

/* A byte flag; declared int-sized so MACRO_ADDR keeps it off $gp (-G2). */
extern int gHelpDeskVoice_w __asm__("D_0015EF1C") MACRO_ADDR;
extern char dialogue_track_table_by_lang[];
extern char music_player_dialogue[];

/* The same through a per-language table at D_0011C278; volume only
   when the D_0015EF1C flag is set. */
void music_start_track_30000(int arg0, int arg1, int arg2) {
    int *p = (int *)(dialogue_track_table_by_lang + (arg0 * 4 + g_game_language * 0x258));
    char *s;

    if (*p == 0) {
        return;
    }
    s = (char *)cd_read_state;
    if (*(int *)(s + 0x50) != 0) {
        return;
    }
    *(unsigned int *)(s + 0x50) = 0xFFFFFFFF;
    *(short *)(s + 0x5A) = 1;
    *(short *)(s + 0x54) = arg0;
    *(short *)(s + 0x58) = arg1;
    *(int *)(s + 0x64) = 10;
    *(int *)(s + 0x68) = 0xBB80;
    *(short *)(s + 0x56) = arg2;
    *(short *)(s + 0x60) = 0;
    snd_PlayVAGStreamByLocEx_CB(*p, 0, 0, 0, *(unsigned char *)&gHelpDeskVoice_w ? arg2 : 0,
                  0, 2, 0, 0x21, music_secondary_start_callback, (long)(unsigned int)music_player_dialogue);
}

typedef struct {
    u32 handle;              /* 0x00: 0 none, 0xFFFFFFFF starting */
    s16 track;               /* 0x04 */
    s16 volume;              /* 0x06 */
    s16 flags;               /* 0x08 */
    s16 state;               /* 0x0A */
    s16 fade_flags;          /* 0x0C */
    s16 unkE;                /* 0x0E */
    s16 crossfade_enabled;   /* 0x10 */
    u8 pad_12[0x2];
    s32 poll_interval;       /* 0x14 */
    s32 remaining_time;      /* 0x18 */
} MusicTrackChannel;

typedef struct {
    u8 pad_00[0x50];
    MusicTrackChannel secondary; /* 0x50 */
} MusicStreamLayout;

/* Start music track 20000+ on the secondary stream channel if that channel is free. */
void music_start_track_20000(int track, int track_flags, int volume) {
    SndToc *toc = (SndToc *)disc_table;
    int index = track - 20000;
    int handle;
    MusicStreamLayout *state;

    handle = toc->ids20000[index].handle;
    if (handle == 0) {
        return;
    }
    state = (MusicStreamLayout *)cd_read_state;
    if (state->secondary.handle != 0) {
        return;
    }
    state->secondary.handle = 0xFFFFFFFF;
    state->secondary.state = 1;
    state->secondary.track = track;
    state->secondary.flags = track_flags;
    state->secondary.poll_interval = 10;
    state->secondary.remaining_time = 0xBB80;
    state->secondary.volume = volume;
    state->secondary.crossfade_enabled = 0;
    snd_PlayVAGStreamByLocEx_CB(handle, 0, 0, 0, volume, 0, 2, 0, 0x21,
                  music_secondary_start_callback, (long)(unsigned int)&state->secondary);
}

/* The PAL music stream state at D_001517D0: only the secondary channel record
   (at 0x50, 0x1C bytes) that music_start_track_10000 writes. */
typedef struct {
    s32 handle;            /* 0x00: stream handle; -1 while starting */
    s16 track;             /* 0x04 */
    s16 volume;            /* 0x06 */
    s16 flags;             /* 0x08 */
    s16 state;             /* 0x0A: set 1 on start */
    s16 fade_flags;        /* 0x0C */
    s16 unkE;              /* 0x0E */
    s16 crossfade_enabled; /* 0x10 */
    u8 pad_12[0x2];
    s32 poll_interval;     /* 0x14 */
    s32 remaining_time;    /* 0x18 */
} PalStreamChannel;

typedef struct {
    u8 pad_00[0x50];
    PalStreamChannel secondary; /* 0x50 */
} PalStreamState;

/* Start music track 10000+ on the secondary stream channel if that channel is free. */
void music_start_track_10000(int track, int flags, int volume) {
    SndToc *toc = (SndToc *)disc_table;
    int i = track - 10000;
    int stream;
    PalStreamState *rec;

    stream = toc->ids10000[i].handle;
    if (stream == 0) {
        return;
    }
    rec = (PalStreamState *)cd_read_state;
    if (rec->secondary.handle != 0) {
        return;
    }
    *(u32 *)&rec->secondary.handle = 0xFFFFFFFF;
    rec->secondary.state = 1;
    rec->secondary.track = track;
    rec->secondary.flags = flags;
    rec->secondary.poll_interval = 10;
    rec->secondary.remaining_time = 0xBB80;
    rec->secondary.volume = volume;
    rec->secondary.crossfade_enabled = 0;
    snd_PlayVAGStreamByLocEx_CB(stream, 0, 0, 0, volume, 0, 2, 0, 0x21,
                  music_secondary_start_callback, (long)(unsigned int)&rec->secondary);
}

extern char scene_sounds[];

/* music_Play dispatcher: routes to the range-specific play function
   (siblings above, one per id range) for arg0 >= 10000; below that it's
   the same music_Playing setup those siblings do, inline, with the sound
   handle read from D_0013A764[arg0][D_0015EE88] (stride 0x250) instead of
   a SndToc table. */
typedef struct {
    u8 pad_00[0x50];
    struct MusicStreamChannel secondary;
} MusicStateSecondaryView;

/* Start a track on the secondary channel, picking the table by id range (else a scene sound). */
void music_start_track_by_id(int track, int track_flags, int volume) {
    MusicStateSecondaryView *s;
    int handle;

    if (track >= 60000) {
        music_start_track_60000(track, track_flags, volume);
        return;
    }
    if (track >= 50000) {
        music_start_track_50000(track, track_flags, volume);
        return;
    }
    if (track >= 40000) {
        music_start_track_40000(track, track_flags, volume);
        return;
    }
    if (track >= 30000) {
        music_start_track_30000(track, track_flags, volume);
        return;
    }
    if (track >= 20000) {
        music_start_track_20000(track, track_flags, volume);
        return;
    }
    if (track >= 10000) {
        music_start_track_10000(track, track_flags, volume);
        return;
    }
    handle = *(int *)((char *)scene_sounds + track * 0x250 + g_game_language * 4);
    if (handle == 0) {
        return;
    }
    s = (MusicStateSecondaryView *)cd_read_state;
    if (s->secondary.handle != 0) {
        return;
    }
    *(u32 *)&s->secondary.handle = 0xFFFFFFFF;
    s->secondary.state = 1;
    s->secondary.track = track;
    s->secondary.flags = track_flags;
    s->secondary.poll_interval = 10;
    s->secondary.remaining_time = 0xBB80;
    s->secondary.volume = volume;
    s->secondary.crossfade_enabled = 0;
    snd_PlayVAGStreamByLocEx_CB(handle, 0, 0, 0, volume, 0, 2, 0, 0x21,
                  music_secondary_start_callback, (long)(unsigned int)&s->secondary);
}

extern short cd_read_state[];
extern void snd_ContinueVAGStream(void *);

/* Resume the secondary stream once its channel is ready; return 1 if it was resumed. */
int Music_ContinueStreamIfReady(void) {
    char *d = (char *)cd_read_state;
    void *p = *(void **)(d + 0x50);
    if (p != 0) {
        if (*(short *)(d + 0x5A) == 3) {
            snd_ContinueVAGStream(p);
            *(short *)(d + 0x5A) = 4;
            return 1;
        }
    }
    return 0;
}

extern void music_primary_preseek_callback(int, long);

/* music_PreseekTrack(int, int, int) */
typedef struct {
    u32 handle;            /* 0x00: 0 none, 0xFFFFFFFF starting */
    s16 track;             /* 0x04 */
    s16 volume;            /* 0x06 */
    s16 flags;             /* 0x08 */
    s16 state;             /* 0x0A */
    s16 fade_flags;        /* 0x0C */
    s16 unkE;              /* 0x0E */
    s16 crossfade_enabled; /* 0x10 */
    u8 pad_12[0x2];
    s32 poll_interval;     /* 0x14 */
    s32 remaining_time;    /* 0x18 */
} PreseekChannel;

typedef struct {
    u8 pad_00[0x34];
    PreseekChannel primary; /* 0x34 */
} PreseekStreamState;

/* Preseek a music track on the primary stream channel if that channel is free. */
void music_PreseekTrack(int arg0, int arg1, int arg2) {
    PreseekStreamState *music_stream_state = (PreseekStreamState *)cd_read_state;
    u8 *disc_table_base;
    s32 track_location_table_byte_offset;
    s32 *entry;
    long stream_location;

    if (music_stream_state->primary.handle == 0) {
        disc_table_base = (u8 *)disc_table;
        track_location_table_byte_offset = 0x2AA8;
        entry = (s32 *)(disc_table_base + track_location_table_byte_offset) + arg0;
        if (*entry != 0) {
            music_stream_state->primary.state = 1;
            music_stream_state->primary.track = arg0;
            music_stream_state->primary.poll_interval = 10;
            music_stream_state->primary.remaining_time = 0xBB80;
            music_stream_state->primary.handle = 0xFFFFFFFF;
            music_stream_state->primary.flags = arg1;
            music_stream_state->primary.volume = arg2;
            music_stream_state->primary.crossfade_enabled = 0;
            stream_location = *entry;
            snd_PlayVAGStreamByLocEx_CB(stream_location, 0, 0, 0, arg2, 0, 1, 0, 0x21,
                          music_primary_preseek_callback,
                          (long)(unsigned int)&music_stream_state->primary.handle);
        }
    }
}

extern void music_primary_start_callback(int, long);

/* music_StartTrack(int, int, int) */
typedef struct {
    u8 pad_00[0x34];
    u32 handle;                /* 0x34: 0 none, 0xFFFFFFFF starting */
    s16 track;                 /* 0x38 */
    s16 volume;                /* 0x3A */
    s16 flags;                 /* 0x3C */
    s16 state;                 /* 0x3E: 1 on start */
    u8 pad_40[0x4];
    s16 crossfade_enabled;     /* 0x44 */
    u8 pad_46[0x2];
    s32 poll_interval;         /* 0x48 */
    s32 remaining_time;        /* 0x4C */
} MusicPrimaryStream;

/* Start a music track on the primary stream channel if that channel is free. */
void music_StartTrack(int arg0, int arg1, int arg2) {
    MusicPrimaryStream *s = (MusicPrimaryStream *)cd_read_state;
    char *base;
    int *tbl;
    long h;

    if (s->handle != 0) {
        return;
    }
    base = (char *)disc_table;
    tbl = (int *)(base + 0x2AA8);
    if (tbl[arg0] == 0) {
        return;
    }
    s->state = 1;
    h = tbl[arg0];
    s->track = arg0;
    s->poll_interval = 10;
    s->remaining_time = 0xBB80;
    s->handle = 0xFFFFFFFF;
    s->flags = arg1;
    s->volume = arg2;
    s->crossfade_enabled = 0;
    snd_PlayVAGStreamByLocEx_CB(h, 0, 0, 0, arg2, 0, 1, 0, 0x20,
                  music_primary_start_callback, (long)(unsigned int)&s->handle);
}

extern void music_primary_replace_callback(int, long);

/* music_StartTrackBody(int, int, int): when the music record at +0x34 is
   already playing (its word neither 0 nor -1) and not in state 9, starts
   track arg0 + 1 of the table at D_00137C80 + 0x2AA8 on it: state 9, the
   track, arg1 and arg2 recorded, 10 and 48000 stored, then
   func_0012ED48 with the current word passed on as its eighth argument
   and 0x24 or 0x20 by arg1's bit 0. The handle is read twice as in
   func_00216A90 (CSE merges the loads, and the address stays in a
   register as in retail), and the 0x24/0x20 choice comes before the
   stores, which keeps arg1 in $a1. */
void music_StartTrackBody(int arg0, int arg1, int arg2) {
    char *s = (char *)cd_read_state;
    char *base;
    int *tbl;
    unsigned int cur;
    long h;
    int i;
    int flags;

    if (*(short *)(s + 0x3E) == 9) {
        return;
    }
    cur = *(unsigned int *)(s + 0x34);
    if (cur == 0 || cur == 0xFFFFFFFF) {
        return;
    }
    base = (char *)disc_table;
    tbl = (int *)(base + 0x2AA8);
    i = arg0 + 1;
    if (tbl[i] == 0) {
        return;
    }
    h = tbl[i];
    flags = (arg1 & 1) ? 0x24 : 0x20;
    *(short *)(s + 0x38) = arg0;
    *(short *)(s + 0x3C) = arg1;
    *(short *)(s + 0x3E) = 9;
    *(int *)(s + 0x48) = 10;
    *(int *)(s + 0x4C) = 0xBB80;
    *(short *)(s + 0x3A) = arg2;
    *(short *)(s + 0x44) = 0;
    snd_PlayVAGStreamByLocEx_CB(h, 0, 0, 0, arg2, 0, 1, cur, flags,
                  music_primary_replace_callback, (long)(unsigned int)(s + 0x34));
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

extern void music_transition_start_callback(int, long);

/* The success path ends in the call, so `return 1` does not break the
   block's trailing call run (the scheduler then keeps retail's order). */
typedef struct {
    u8 pad_0[0x2AA8];
    s32 music_tracks[1];               /* 0x2AA8, by track number */
} MusicTrackTable;

/* One channel record (0x1C bytes), minimal layout for the transition slot. */
typedef struct {
    u32 handle;                        /* 0x00 */
    s16 track;                         /* 0x04 */
    s16 volume;                        /* 0x06 */
    s16 flags;                         /* 0x08 */
    s16 state;                         /* 0x0A */
    s16 fade_flags;                    /* 0x0C */
    s16 unkE;                          /* 0x0E */
    s16 crossfade_enabled;             /* 0x10 */
    u8 pad_12[0x2];
    s32 poll_interval;                 /* 0x14 */
    s32 remaining_time;                /* 0x18 */
} MusicTransitionSlot;

/* The stream state up to the transition record at 0x6C. */
typedef struct {
    u8 pad_00[0x6C];
    MusicTransitionSlot transition;    /* 0x6C */
} MusicTransitionBlock;

/* Start a crossfade transition track on its channel; return 1 if it was started. */
int music_Transition(int arg0, int arg1, int arg2, int arg3) {
    MusicTransitionBlock *s = (MusicTransitionBlock *)cd_read_state;
    MusicTrackTable *base;
    s32 *tbl;
    long h;

    if (s->transition.handle == 0) {
        base = (MusicTrackTable *)disc_table;
        tbl = base->music_tracks;
        if (tbl[arg1] != 0) {
            s->transition.flags = arg2;
            h = tbl[arg1];
            s->transition.poll_interval = 10;
            s->transition.remaining_time = 0xBB80;
            s->transition.handle = 0xFFFFFFFF;
            s->transition.track = arg0;
            s->transition.state = 1;
            s->transition.crossfade_enabled = 1;
            s->transition.volume = arg3;
            snd_PlayVAGStreamByLocEx_CB(h, 0, 0, 0, arg3, 0, 1, 0, 0x20,
                          music_transition_start_callback, (long)(unsigned int)&s->transition.handle);
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

/* A block-local `char *e` for the final stores (%hi kept, %lo rebuilt),
   written in the order the rotation rule gives. */
void Music_QueueTrack(int arg0, int arg1) {
    char *d = (char *)cd_read_state;

    if (*(short *)(d + 0x38) == arg0) {
        if (*(signed char *)(d + 0x22) == -1) {
            return;
        }
    }
    if (*(short *)(d + 0x3E) == 0) {
        if (*(short *)(d + 0x76) == 0) {
            *(short *)(d + 0x38) = arg0;
            return;
        }
    }
    {
        char *e = (char *)cd_read_state;
        e[0x22] = arg0;
        e[0x23] = arg1;
    }
}

extern void snd_StopAllStreams(void);
extern void snd_StreamSafeCheckCDIdle(int);

/* music_Stop: waits (func_0012DDC0, snd_FlushSoundCommands) while each
   of the three playing records at +0x34, +0x6C and +0x50 is still
   starting (state 0xFFFFFFFF), stops the streams (func_0012ED10), flushes
   until nothing is left, calls func_0012EE70(1), then clears the records
   and the current id (+0x22 kept in +0x38 when set). Each wait loop has
   its own pointer assigned in the loop condition: jump.c's copy of the
   exit test in front of the loop then gets its own pseudo, and the loop
   keeps retail's copy in $s1. The stores are in the order that gives
   retail's schedule. */
/* 0x1C */

typedef struct {
    u8 pad00[0x20];
    s16 crossfade_state;           /* 0x20 */
    s8 requested_track;            /* 0x22: -1 = none */
    s8 requested_transition_track; /* 0x23 */
    u8 pad24[0x10];
    struct MusicStreamChannel primary;    /* 0x34 */
    struct MusicStreamChannel secondary;  /* 0x50 */
    struct MusicStreamChannel transition; /* 0x6C */
} MusicStreamBlock;

/* Wait for pending stream opens, stop all music streams and reset the channel state. */
void music_Stop(void) {
    {
        MusicStreamBlock *d;
        while (d = (MusicStreamBlock *)cd_read_state, d->primary.handle == 0xFFFFFFFF) {
            snd_FlushSoundCommands();
        }
    }
    {
        MusicStreamBlock *d;
        while (d = (MusicStreamBlock *)cd_read_state, d->transition.handle == 0xFFFFFFFF) {
            snd_FlushSoundCommands();
        }
    }
    {
        MusicStreamBlock *d;
        while (d = (MusicStreamBlock *)cd_read_state, d->secondary.handle == 0xFFFFFFFF) {
            snd_FlushSoundCommands();
        }
    }
    snd_StopAllStreams();
    while (snd_FlushSoundCommands() != 0) {
    }
    snd_StreamSafeCheckCDIdle(1);
    {
        MusicStreamBlock *s = (MusicStreamBlock *)cd_read_state;
        s->primary.state = 0;
        s->primary.flags = 0;
        s->primary.handle = 0;
        if (s->requested_track != -1) {
            s->primary.track = s->requested_track;
        }
        s->secondary.state = 0;
        s->secondary.flags = 0;
        s->secondary.handle = 0;
        s->transition.state = 0;
        s->transition.flags = 0;
        s->transition.handle = 0;
        s->crossfade_state = 0;
        s->requested_track = -1;
        s->requested_transition_track = -1;
    }
}

extern short cd_read_state[];

/* music_Pause(int) */
void music_Pause(int arg0) {
    if (arg0 != 0) {
        cd_read_state[0x2F] = 0;
        cd_read_state[0x2E] = -0x8000;
    }
    cd_read_state[0x3D] = 0;
    cd_read_state[0x20] = -0x8000;
    cd_read_state[0x21] = 0;
    cd_read_state[0x3C] = -0x8000;
}

/* music_Unpause(void) */
void music_Unpause(void) {
    short *p = cd_read_state;
    p[0x20] = 4;
    p[0x3C] = 4;
    p[0x2E] = 4;
}

typedef struct {
    unsigned int handle; /* 0x00: 0 none, 0xFFFFFFFF starting/released */
    short id;            /* 0x04 */
    short unk06;
    short unk08;
    short state;         /* 0x0A: low bits the state, 0x8000 paused */
    short fade;          /* 0x0C: 0x8000 fading */
    short fadeT;         /* 0x0E */
} MusicPlaying;

extern void snd_stop_sound(int);
extern void snd_PauseVAGStream(int);
extern int FastDecTimer(void *);
extern void snd_GetVAGStreamTimeRemaining_CB(int, void (*)(int, long), long);
extern void snd_SoundIsStillPlaying_CB(int, void (*)(int, long), long);
extern void snd_IsVAGStreamBuffered_CB(int, void (*)(int, long), long);
extern void music_remaining_time_callback(int, long);
extern void set_sound_handle_id(int, long);
extern void music_channel_ready_callback(int, long);

/* music_UpdateStream(music_Playing &): with a live handle and state other
   than 9, state 5 stops the stream (state 6) and state 6 without a
   handle resets; a fading record (+0xC bit 15) pauses the handle once
   (+0xA bit 15) and polls func_001F9938 until it reports 2 (+0xC = 4),
   otherwise a paused one is resumed. Unpaused, states 1/8/9 are left
   alone, 2 hands the handle to func_0012EE40 unless it is 0xFFFFFFFF,
   and anything but 2/3 releases the handle through func_0012EE10 and
   func_0012E588. With no live handle (or state 9) the state is cleared
   when it is 7 or the handle is 0. The handle is unsigned (retail builds
   0xFFFFFFFF with lui/ori), the special case is the else arm (retail
   places it last), and the dispatch reads p->state directly so the 2/3
   range test works on the loaded halfword as retail's does. */
void music_UpdateStream(MusicPlaying *p) {
    int h;

    if (p->state != 9 && p->handle != 0 && p->handle != 0xFFFFFFFF) {
        if (p->state == 5) {
            if (p->handle != 0) {
                snd_stop_sound(p->handle);
                p->state = 6;
            } else {
                p->state = 0;
            }
        } else if (p->state == 6) {
            if (p->handle == 0) {
                p->state = 0;
            }
        }
        if (p->handle == 0) {
            return;
        }
        if (p->fade & 0x8000) {
            if (!(p->state & 0x8000)) {
                snd_PauseVAGStream(p->handle);
                p->state |= 0x8000;
            }
            if (FastDecTimer(&p->fadeT) == 2) {
                p->fade = 4;
            }
        } else if (p->state & 0x8000) {
            snd_ContinueVAGStream((void *)p->handle);
            p->state ^= 0x8000;
        }
        if (p->state & 0x8000) {
            return;
        }
        if (p->state == 1 || p->state == 8 || p->state == 9) {
            return;
        }
        if (p->state != 2 && p->state != 3) {
            h = p->handle;
            p->handle = 0xFFFFFFFF;
            snd_GetVAGStreamTimeRemaining_CB(h, music_remaining_time_callback, (long)(unsigned int)p);
            snd_SoundIsStillPlaying_CB(h, set_sound_handle_id, (long)(unsigned int)p);
            return;
        }
        if (p->handle != 0xFFFFFFFF && p->state == 2) {
            snd_IsVAGStreamBuffered_CB(p->handle, music_channel_ready_callback, (long)(unsigned int)p);
        }
    } else if (p->state == 7 || p->handle == 0) {
        p->state = 0;
    }
}

struct MusicCdReadMode {
    u8 trycount; /* 0x20 from reset_music */
    u8 spindlctrl;
    u8 datapattern;
    u8 pad;
};
struct MusicStreamState {
    s32 unk0; /* 0x00: cleared by reset_music */
    u8 pad_4[0x4];
    s16 read_state; /* 0x08: 0 idle, 1 reading, 2 done */
    u8 break_requested; /* 0x0A: set by request_audio_stream_break */
    u8 updates_suspended; /* 0x0B: set around space transitions */
    s32 read_sector; /* 0x0C */
    s32 read_sector_count; /* 0x10 */
    s32 read_dst; /* 0x14 */
    u8 pad_18[0x4];
    s32 queued_secondary_track; /* 0x1C: -1 = none; overlays queue track + 40000 (l13 lines: + 50000) and wait for secondary.state == 3 */
    s16 crossfade_state; /* 0x20: 1 -> 2 in music_remaining_time_callback */
    s8 requested_track; /* 0x22: -1 = none */
    s8 requested_transition_track; /* 0x23 */
    s32 crossfade_remaining_time; /* 0x24 */
    s32 crossfade_interval; /* 0x28: remaining time / 4 */
    s32 retry_timer; /* 0x2C */
    struct MusicCdReadMode cd_mode; /* 0x30: 4th sceCdRead argument */
    struct MusicStreamChannel primary; /* 0x34 */
    struct MusicStreamChannel secondary; /* 0x50 */
    struct MusicStreamChannel transition; /* 0x6C */
};
extern struct MusicStreamState cd_read_state_r __asm__("D_001517D0");
extern void snd_set_sound_params_cb(s32, s32, s32, s32, s32, s32, void (*)(u32, s64), s64);
extern void set_sound_handle_id_r(u32, s64) __asm__("func_00217A08");
extern void snd_ContinueVAGStream_r(s32) __asm__("func_0012EDE0");
extern s32 snd_StreamSafeCdSync(s32);
extern s32 scale_game_frame_count(s32);
extern s32 fast_dec_timer(void *);
extern void music_start_track_by_id(s32, s32, s32);
extern void music_PreseekTrack(s32, s32, s32);
extern void music_StartTrack(s32, s32, s32);
extern void music_StartTrackBody(s32, s32, s32);
extern s32 music_Transition(s32, s32, s32, s32);
extern void music_UpdateStream_r(void *) __asm__("func_00216F48");
extern s32 start_audio_stream_read(s32, s32, s32);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/textbin/audio/music/music_update.c, music_update. */
void music_Update(void) {
    s8 requested_track;
    s32 fade_volume;
    s32 handle;
    s16 pending_state;

    if (cd_read_state_r.updates_suspended != 0) {
        return;
    }
    if (!(cd_read_state_r.primary.fade_flags & 0x8000) && !(cd_read_state_r.primary.state & 0x8000)) {
        if (cd_read_state_r.primary.handle == 0 && (cd_read_state_r.primary.flags & 1) &&
            cd_read_state_r.requested_track == -1) {
            music_StartTrack(cd_read_state_r.primary.track, (s16)cd_read_state_r.primary.flags,
                              cd_read_state_r.primary.volume);
        } else if (cd_read_state_r.primary.state != 9 && cd_read_state_r.primary.handle != 0) {
            if (cd_read_state_r.primary.handle != 0xFFFFFFFF && cd_read_state_r.primary.state == 8) {
                music_StartTrackBody(cd_read_state_r.primary.track, cd_read_state_r.primary.flags,
                                       cd_read_state_r.primary.volume);
            }
        }
    }
    if (fast_dec_timer(&cd_read_state_r.retry_timer) != 0) {
        requested_track = cd_read_state_r.requested_track;
        if (requested_track != -1 && cd_read_state_r.crossfade_state == 0) {
            if (cd_read_state_r.primary.track != requested_track) {
                if (cd_read_state_r.requested_transition_track == -1 ||
                    music_Transition(requested_track, cd_read_state_r.requested_transition_track,
                                     cd_read_state_r.primary.flags, cd_read_state_r.primary.volume) != 0) {
                    cd_read_state_r.retry_timer = scale_game_frame_count(7) * 60.0f;
                }
            } else {
                cd_read_state_r.requested_track = -1;
            }
        }
    }
    if (cd_read_state_r.queued_secondary_track >= 0) {
        if (cd_read_state_r.secondary.handle != 0) {
            if ((u16)cd_read_state_r.secondary.state - 6 >= 2U) {
                cd_read_state_r.secondary.state = 5;
            }
        } else {
            music_start_track_by_id(cd_read_state_r.queued_secondary_track, 0, 0x400);
            cd_read_state_r.queued_secondary_track = -1;
        }
    }
    if (cd_read_state_r.primary.state != 9 && cd_read_state_r.primary.handle != 0xFFFFFFFF &&
        !(cd_read_state_r.primary.fade_flags & 0x8000) && !(cd_read_state_r.primary.state & 0x8000) &&
        !(cd_read_state_r.transition.fade_flags & 0x8000) && !(cd_read_state_r.transition.state & 0x8000)) {
        switch (cd_read_state_r.crossfade_state) {
        case 2:
            /* Fade the primary stream against the transition's remaining time. */
            fade_volume =
                cd_read_state_r.primary.volume *
                (cd_read_state_r.crossfade_interval -
                 (cd_read_state_r.crossfade_remaining_time - cd_read_state_r.transition.remaining_time)) /
                cd_read_state_r.crossfade_interval;
            if (fade_volume <= 0 ||
                ((cd_read_state_r.transition.state != 4 ||
                  cd_read_state_r.transition.crossfade_enabled == 0) &&
                 cd_read_state_r.transition.handle == 0) ||
                (handle = cd_read_state_r.primary.handle) == 0) {
                cd_read_state_r.crossfade_state = 3;
                cd_read_state_r.primary.state = 5;
            } else if (cd_read_state_r.primary.state != 9) {
                /* The callback receives the old handle; mark its slot pending before submission. */
                *(u32 *)&cd_read_state_r.primary.handle = 0xFFFFFFFF;
                snd_set_sound_params_cb(handle, 5, fade_volume, 0, 0, 0, set_sound_handle_id_r,
                                        (s64)&cd_read_state_r.primary.handle);
            }
            break;
        case 3:
            if (cd_read_state_r.primary.state == 0 && (cd_read_state_r.transition.crossfade_enabled != 0 ||
                                                   cd_read_state_r.transition.handle == 0)) {
                music_PreseekTrack(cd_read_state_r.requested_track, cd_read_state_r.primary.flags,
                                    cd_read_state_r.primary.volume);
                cd_read_state_r.requested_track = -1;
                cd_read_state_r.crossfade_state = 4;
            }
            break;
        case 4:
            if (cd_read_state_r.primary.state == 3) {
                if (cd_read_state_r.transition.remaining_time < cd_read_state_r.crossfade_interval ||
                    cd_read_state_r.transition.state != 4 ||
                    (cd_read_state_r.transition.crossfade_enabled == 0 &&
                     cd_read_state_r.transition.handle == 0)) {
                    snd_ContinueVAGStream_r(cd_read_state_r.primary.handle);
                    cd_read_state_r.primary.state = 8;
                    cd_read_state_r.crossfade_state = 5;
                }
            }
            break;
        case 5:
            if (cd_read_state_r.transition.state != 4 ||
                cd_read_state_r.transition.crossfade_enabled == 0) {
                cd_read_state_r.crossfade_state = 0;
            }
            break;
        }
    }
    music_UpdateStream_r(&cd_read_state_r.primary.handle);
    music_UpdateStream_r(&cd_read_state_r.transition.handle);
    music_UpdateStream_r(&cd_read_state_r.secondary.handle);
    if (cd_read_state_r.break_requested != 0) {
        if (snd_StreamSafeCdSync(1) == 0) {
            cd_read_state_r.read_state = 0;
            cd_read_state_r.break_requested = 0;
        }
    } else {
        pending_state = cd_read_state_r.read_state;
        if (pending_state == 2) {
            cd_read_state_r.read_state = 0;
            start_audio_stream_read(cd_read_state_r.read_dst, cd_read_state_r.read_sector,
                                    cd_read_state_r.read_sector_count);
            if (cd_read_state_r.read_state == 0) {
                cd_read_state_r.read_state = pending_state;
            }
        }
    }
}
