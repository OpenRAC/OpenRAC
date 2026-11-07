#include "common.h"
#include "structs/pad.h"
#include "callees.h"
#include "structs/misc.h"
#include "game/movie/movie.h"
#include "game/system/pad.h"
#include "asm_functions/executable.h"
#include "game/system/stub.h"
#include "core/sound/989snd.h"
#include "core/gs/put_draw_env.h"
#include "core/cdvd/callbacks.h"
#include "game/movie/videodec.h"
#include "game/movie/audiodec.h"
#include "game/movie/readbuf.h"
#include "game/movie/vobuf.h"
#include "game/movie/strfile.h"
#include "game/movie/disp.h"

extern int decoder_buffer_address MACRO_ADDR;
extern int movie_play_step MACRO_ADDR;
extern int movie_clear_gp SDATA(D_00161318);
extern char *decoder_state MACRO_ADDR;
extern char *movie_gp_base SDATA(D_0016130C);

extern int initAll(int, int, int);
extern int read_mpeg(char *, char *, int *);
extern void termAll(void);
/* No recovered name. Sets movie globals, runs initialization and playback, then clears them. */
int play_mpeg_movie(int a, int b, int c, char *state, int e) {
    decoder_buffer_address = c;
    decoder_state = state;
    movie_play_step = 0;
    movie_clear_gp = 0;
    change_thread_priority(get_thread_id(), 1);
    movie_play_step = 1;
    if (initAll(a, b, e)) {
        char *base = movie_gp_base;
        movie_play_step = 2;
        read_mpeg(base + 0xD9048, base, base + 0xD9040);
    }
    termAll();
    decoder_buffer_address = 0;
    decoder_state = 0;
    return 0;
}
extern int movie_frame_count MACRO_ADDR;
extern int decode_mode MACRO_ADDR;
extern int challenge_mode MACRO_ADDR;
extern int D_0015EF20 MACRO_ADDR;
extern int g_current_level_index MACRO_ADDR;
/* pad state at g_controller_state: two words read at 0x1A0/0x1A4 */
extern PadState g_controller_state;
/* global block at voice_pool: word at 0x5C */
typedef struct MvGlobals {
    char pad0[0x5C];
    int f5C;
} __attribute__((aligned(16))) MvGlobals;
extern MvGlobals voice_pool;

extern void proceedAudio(void);

extern int sce_mpeg_demux_pss_ring_buffer(char *, int, int, char *, int);

/* No recovered name. Playback loop: feeds file data (*file bytes) through the
 * read buffer at `rb` into the video decoder `vd`, starts audio and display once
 * the output buffer fills, aborts on a pad combination, then drains the decoder.
 * movie_play_step records the step reached. Returns 1 if aborted. */
int read_mpeg(char *vd, char *rb, int *file) {
    int aborted = 0;
    int started = 0;
    int left = *file;
    int toread;
    int space;
    int got;
    int avail;
    int used;
    int put;
    int get;

    movie_play_step = 3;
    flush_cache(0);
    toread = left;
    flush_cache(2);
    sce_gs_sync_v(0);
    movie_play_step = 4;

    while (left >= 5 && videoDecGetState(vd) != 3) {
        int hit;

        movie_play_step = 5;
        movie_frame_count++;
        UpdatePad();
        movie_play_step = 6;
        if (decode_mode != -1) {
            movie_play_step = 7;
            if (decode_mode == 2 && g_controller_state.pressed != 0) {
                hit = 1;
            } else if ((challenge_mode || D_0015EF20 || decode_mode || g_current_level_index <= 0) &&
                       (g_controller_state.pressed & PAD_START)) {
                hit = 1;
            } else if ((g_controller_state.held & (PAD_L2 | PAD_R2 | PAD_L1 | PAD_R1)) == 0xF &&
                       (g_controller_state.pressed & PAD_START)) {
                hit = 1;
            } else {
                hit = 0;
            }
            if (hit) {
                aborted = 1;
                videoDecAbort(decoder_state + 0xD9048);
            }
            movie_play_step = 8;
        }
        movie_play_step = 9;
        space = readBufBeginPut(rb, &put);
        movie_play_step = 10;
        if (toread > 0 && space > 0xFFFF) {
            movie_play_step = 11;
            got = read_cd_stream_sectors(file, put, 0x10000, 0);
            movie_play_step = 12;
            readBufEndPut(rb, got);
            toread -= got;
            movie_play_step = 13;
        }
        movie_play_step = 14;
        proceedAudio();
        movie_play_step = 15;
        switchThread();
        movie_play_step = 16;
        avail = readBufBeginGet(rb, &get);
        movie_play_step = 17;
        if (avail > 0) {
            int size = *(int *)(rb + 0x50008);

            movie_play_step = 18;
            used = sce_mpeg_demux_pss_ring_buffer(vd, get, avail, rb, size);
            movie_play_step = 19;
            readBufEndGet(rb, used);
            left -= used;
            movie_play_step = 20;
        }
        movie_play_step = 21;
        proceedAudio();
        movie_play_step = 22;
        if (!started && voBufIsFull(decoder_state + 0xD9168) && isAudioOK()) {
            movie_play_step = 23;
            started = 1;
            startDisplay(1);
            movie_play_step = 24;
            audioDecStart(decoder_state + 0xD9100);
            movie_play_step = 25;
        }
    }

    while (!videoDecFlush(vd)) {
        proceedAudio();
        switchThread();
    }
    while (!videoDecIsFlushed(vd) && videoDecGetState(vd) != 3) {
        proceedAudio();
        switchThread();
    }
    endDisplay();
    audioDecReset(decoder_state + 0xD9100);
    snd_SetMasterVolume(5, voice_pool.f5C);
    snd_FlushSoundCommands();
    movie_play_step = 0;
    movie_frame_count = 0;
    return aborted;
}

/* switchThread -- calls rotate_thread_ready_queue(1). */
void switchThread(void) {
    rotate_thread_ready_queue(1);
}

/* isAudioOK -- true when the audio decoder at offset 0xD9100 of the movie state holds 0x1000
 * bytes or more. */
int isAudioOK(void) {
    return audio_dec_is_preset(decoder_state + 0xD9100);
}
struct AudioDecoderState {
    char pad0[0xD90F8];
    int dmac_handler_id;
    int intc_handler_id;
};
struct AudioDecoderThreadArgs {
    char pad0[4];
    void (*entry)();
    void *stack;
    int stack_size;
    void *gp;
    int priority;
    char pad18[8];
    int gs_tag;
    char pad24[0xC];
};
extern int decoder_thread MACRO_ADDR;
extern char movie_open_error_text[];
extern char gp_base[] MACRO_ADDR;
extern void sce_mpeg_init(void);

extern int enable_dmac_atomic(int);
extern void vblankHandler();
extern void handler_endimage();
extern void videoDecMain();
extern int videoCallback();
extern int pcmCallback();
/* initAll(int, int, int) */
int initAll(int stream_source, int source_mode, int callback_context) {
    struct AudioDecoderThreadArgs decoder_thread_args;
    int thread_id;
    int opened;
    *((volatile int *)0x1000E000) |= 3;
    *((volatile int *)0x1000E010) = 4;
    readBufCreate(movie_gp_base);
    sce_mpeg_init();
    videoDecCreate(((char *)decoder_state) + 0xD9048, decoder_buffer_address + 0x1C85C0, 0xEB768,
                   ((char *)decoder_state) + 0x52040, decoder_buffer_address + 0x1C7180, 0x100,
                   ((char *)decoder_state) + 0xD6040, 0x200);
    audioDecCreate(((char *)decoder_state) + 0xD9100, ((char *)decoder_state) + 0x50040, 0x2000,
                   decoder_buffer_address + 0x1C8190);
    videoDecSetStream(((char *)decoder_state) + 0xD9048, 0, 0, videoCallback, decoder_state);
    videoDecSetStream(((char *)decoder_state) + 0xD9048, 3, callback_context, pcmCallback, decoder_state);
    voBufCreate(((char *)decoder_state) + 0xD9168, (decoder_buffer_address & 0x0FFFFFFF) | 0x20000000,
                decoder_buffer_address + 0x1A0000, 2);
    decoder_thread_args.stack = ((char *)decoder_state) + 0xD2040;
    decoder_thread_args.entry = videoDecMain;
    decoder_thread_args.stack_size = 0x4000;
    decoder_thread_args.priority = 1;
    decoder_thread_args.gp = gp_base;
    decoder_thread_args.gs_tag = 0;
    thread_id = create_thread(&decoder_thread_args);
    decoder_thread = thread_id;
    start_thread(thread_id, ((char *)decoder_state) + 0xD9048);
    if (mpeg_cd_stream_open(((char *)decoder_state) + 0xD9040, stream_source, source_mode) == 0) {
        opened = 0;
        STUB_printf(movie_open_error_text);
    } else {
        opened = 1;
    }
    ((struct AudioDecoderState *)decoder_state)->intc_handler_id = add_intc_handler(2, vblankHandler, 0);
    enable_intc_atomic(2);
    ((struct AudioDecoderState *)decoder_state)->dmac_handler_id = add_dmac_handler(2, handler_endimage, 0);
    enable_dmac_atomic(2);
    return opened;
}
extern int movie_thread_gp SDATA(D_00161310);

/* termAll(void): shuts down workers and decoders, then clears the DMA enable bit. */
void termAll(void) {
    sce_cd_sync_s(0);
    readBufDelete(decoder_state);
    voBufDelete(decoder_state + 0xD9168);
    terminate_thread(movie_thread_gp);
    delete_thread(movie_thread_gp);
    disable_dmac_atomic(2);
    remove_dmac_handler(2, *(int *)(decoder_state + 0xD90F8));
    disable_intc_atomic(2);
    remove_intc_handler(2, *(int *)(decoder_state + 0xD90FC));
    videoDecDelete(decoder_state + 0xD9048);
    audioDecDelete(decoder_state + 0xD9100);
    mpeg_cd_stream_close(decoder_state + 0xD9040);
    sce_cd_sync_s(0);
    *(volatile unsigned int *)0x1000E000 &= ~2u;
}

extern char movie_error_message[];

/* ErrMessage -- passes the message to STUB_printf with the string at
 * movie_error_message as its first argument. */
void ErrMessage(char *fmt) {
    STUB_printf(movie_error_message, fmt);
}

/* proceedAudio(void) -- audioDecSend on the audio decoder at offset 0xD9100 of the movie state. */
void proceedAudio(void) {
    audioDecSend(decoder_state + 0xD9100);
}
