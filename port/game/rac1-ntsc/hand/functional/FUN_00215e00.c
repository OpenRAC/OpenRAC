/* Carried by port/tools/versionmap/carry_functions.py from the decompilation of Ratchet & Clank (PAL), src/game/music.c (games/rac1): func_00216C50, matched C there.
 * Kind `same` in the version map (../../README.md): the two versions have the same instructions for it.
 * Names and addresses are this version's. Functional C, not matched C. */

typedef short s16_c215e00;
typedef int s32_c215e00;
typedef unsigned char u8_c215e00;
typedef unsigned int u32_c215e00;
extern short D_001516D0[];
extern int D_00137B80[];
extern short D_001516D0[];
extern void func_0012EC08(int, int, int, int, short, int, int, int,
                          int, void (*)(int, long), long);
extern short D_001516D0[];
extern void func_00216A80(int, long);
typedef struct {
    u8_c215e00 pad_0[0x2AA8];
    s32_c215e00 music_tracks[1];
} MusicTrackTable_c215e00;
typedef struct {
    u32_c215e00 handle;
    s16_c215e00 track;
    s16_c215e00 volume;
    s16_c215e00 flags;
    s16_c215e00 state;
    s16_c215e00 fade_flags;
    s16_c215e00 unkE;
    s16_c215e00 crossfade_enabled;
    u8_c215e00 pad_12[0x2];
    s32_c215e00 poll_interval;
    s32_c215e00 remaining_time;
} MusicTransitionSlot_c215e00;
typedef struct {
    u8_c215e00 pad_00[0x6C];
    MusicTransitionSlot_c215e00 transition;
} MusicTransitionBlock_c215e00;
int FUN_00215e00(int arg0, int arg1, int arg2, int arg3) {
    MusicTransitionBlock_c215e00 *s = (MusicTransitionBlock_c215e00 *)D_001516D0;
    MusicTrackTable_c215e00 *base;
    s32_c215e00 *tbl;
    long h;
    if (s->transition.handle == 0) {
        base = (MusicTrackTable_c215e00 *)D_00137B80;
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
            func_0012EC08(h, 0, 0, 0, arg3, 0, 1, 0, 0x20,
                          func_00216A80, (long)(unsigned int)&s->transition.handle);
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}
extern short D_001516D0[];
extern s32_c215e00 FUN_00215e00(s32_c215e00, s32_c215e00, s32_c215e00, s32_c215e00);
