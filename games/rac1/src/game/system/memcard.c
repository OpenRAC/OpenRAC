#include "common.h"
#include "structs/memcard.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"
#include "core/cdvd/read.h"
#include "core/mc/memory_card.h"
#include "core/mpeg/ipu.h"
#include "game/ui/menu.h"
#include "data/executable.h"
#include "asm_functions/executable.h"
#include "game/system/stub.h"
#include "game/ui/help.h"
#include "core/gs/put_draw_env.h"
#include "core/libc/sprintf.h"

/*
 * memcard.cpp in the original source; text 0x209A60-0x20C210.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

extern char memory_card_state[];

extern char memcard_save_dir_name[];
extern char memcard_dir_search_path[];
extern char memcard_icon_path_a[];
extern char memcard_icon_path_b[];
extern char memcard_icon_path_c[];
extern char memcard_slot_path_format[];

/* Builds a 13-byte name/version stamp in memcard_save_dir_name from the record's
   region byte (arg0[0x12]) and the record's date/version fields, then
   copies that stamp into the six on-disk name buffers. */
void memcard_get_name(void *arg0) {
    unsigned char *a = (unsigned char *)arg0;
    unsigned char region = a[0x12];
    int i;

    if (region == 'E') {
        memcard_save_dir_name[2] = region;
    } else if (region == 'P') {
        memcard_save_dir_name[2] = 'I';
    }
    for (i = 3; i < 7; i++) {
        memcard_save_dir_name[i] = a[i + 0xD];
    }
    for (i = 8; i < 0xB; i++) {
        memcard_save_dir_name[i] = a[i + 0xD];
    }
    for (i = 0xB; i < 0xD; i++) {
        memcard_save_dir_name[i] = a[i + 0xE];
    }
    libc_strncpy(memcard_dir_search_path, memcard_save_dir_name, 0xD);
    libc_strncpy(memcard_icon_path_a, memcard_save_dir_name, 0xD);
    libc_strncpy(memcard_icon_path_b, memcard_save_dir_name, 0xD);
    libc_strncpy(memcard_icon_path_c, memcard_save_dir_name, 0xD);
    libc_strncpy(memcard_icon_path_c + 0x14, memcard_save_dir_name, 0xD);
    libc_strncpy(memcard_slot_path_format, memcard_save_dir_name, 0xD);
}

extern char memcard_save_dir_name[];
extern int memory_card_state_i[] __asm__("D_0013D390");

/* The comparison is bound to a local before the shift:
   `(a < b) << 1` folds into `a < b ? 2 : 0` (li/slt/movn), not retail's
   slti/sll. */
int check_memory_card(void) {
    int type, free, format, cmd, result;

    result = sce_mc_get_info(memory_card_state_i[0], memory_card_state_i[1], &type, &free, &format);
    while (sce_mc_sync(1, &memory_card_state_i[0x30], &memory_card_state_i[0x31]) == 0) {
        sce_gs_sync_v(0);
    }
    if (memory_card_state_i[0x31] == -5 || memory_card_state_i[0x31] < -9 || type != 2) {
        return 1;
    }
    if (memory_card_state_i[0x31] != -2 && format != 0) {
        result = sce_mc_get_dir(memory_card_state_i[0], memory_card_state_i[1], memcard_save_dir_name, 0, -1, 0);
        while (sce_mc_sync(1, &cmd, &result) == 0) {
            sce_gs_sync_v(0);
        }
        if (result <= 0) {
            int t = free < 350;
            return t << 1;
        }
    }
    return 0;
}

extern int mc_gamedata[];
extern int mc_leveldata[];
extern int memcard_GetDataSize(int *p);
extern int memcard_RestoreData(void *src, int i, int *table);

extern char out_of_date_save_message[];

/* The counterpart of memcard_MakeWholeSave below: both descriptor sizes are
   recomputed and checked against the ones stored in the blob, then the
   blobs are read back in the same order they were written. The read
   cursor is the parameter, which retail keeps in $s2. */
/* memcard_RestoreGame */
void memcard_RestoreGame(int arg0) {
    char *p = (char *)arg0;
    int a;
    int b;
    int i;

    a = memcard_GetDataSize(mc_gamedata);
    b = memcard_GetDataSize(mc_leveldata);
    if (*(int *)p != a || *(int *)(p + 4) != b) {
        STUB_printf(out_of_date_save_message);
        return;
    }
    p += 8;
    memcard_RestoreData(p, 0, mc_gamedata);
    p += a;
    for (i = 0; i < 0x14; i++) {
        memcard_RestoreData(p, i, mc_leveldata);
        p += b;
    }
}

extern int disc_table[];

extern void update_audio_stream_until_idle(int);

extern void memcard_RestoreGame(int);
extern int playback_mode MACRO_ADDR;
extern int music_volume MACRO_ADDR;
extern int sound_volume MACRO_ADDR;
extern int g_current_level_index_m __asm__("D_0015EE84") MACRO_ADDR;

extern int Load(int, int, int);

/* Load returns int and takes three arguments (stream.c). */
void load_and_initialize_level_chunk(void) {
    int a;
    int b;
    int s0;
    int s1;
    int s2;

    calculate_ring_buffer_bounds(disc_table[5] << 11, &a, &b);
    update_audio_stream_until_idle(1);
    Load(a, disc_table[4], disc_table[5]);
    s2 = sound_volume;
    s1 = music_volume;
    s0 = playback_mode;
    memcard_RestoreGame(a + *(int *)(a + 0x10));
    sound_volume = s2;
    music_volume = s1;
    playback_mode = s0;
    g_current_level_index_m = 0;
}

/* Adapted from Lombyte's project (GPL-3.0): src/storage/memory_card/memcard_update_state.c; PAL directory validation reconstructed from assembly. */
typedef struct {
    u8 pad0[16];
    u32 size;
    u8 pad14[12];
    char name[32];
} McDirEntry;
extern McDirEntry mc_directory[] __asm__("D_001A0980");
extern char memcard_icon_path_a_filename[], memcard_icon_path_b_filename[], special_items_base[],
    memcard_slot_file_prefix[], memcard_slot_file_suffix[];
extern void libc_memset(void *, s32, s32);

typedef struct {
    s32 port;
    s32 slot;
    s32 type;
    s32 free;
    s32 format;
    s32 x14;
    s32 x18;
    s32 x1C;
    u8 pad_20[0x8C];
    s32 used_blocks; /* +0xAC */
    s32 request; /* +0xB0 */
    s32 xAC; /* +0xB4 */
    s32 xB0;
    s32 xB4;
} McCard_209E68;

typedef struct {
    McCard_209E68 card[1];
    s32 cmd;
    s32 result;
    s32 xC0;
    s32 cur;
    s32 xC8;
    s32 busy;
    s32 fd;
    s32 state;
    s32 sub;
    s32 xDC;
    s32 xE0;
    s32 err;
    s32 errCard;
    void *buf;
    s32 size;
} McState;

typedef struct {
    s32 x0;
    s32 x4;
    s32 x8;
    s32 xC;
    s32 x10;
} McChunk;

typedef struct {
    u8 pad_0[0x10];
    s32 x10;
    s32 x14;
} McLevel;

extern McState mc_data_0013D390 __asm__("D_0013D390");
extern McLevel mc_data_00137C80 __asm__("D_00137C80");
extern s32 mc_data_00137C94[] __asm__("D_00137C94");
extern char mc_data_0013D2D0[] __asm__("D_0013D2D0");
extern char mc_data_0013D2E8[] __asm__("D_0013D2E8");
extern char mc_data_0013D300[] __asm__("D_0013D300");
extern char mc_data_0013D320[] __asm__("D_0013D320");
extern char mc_data_0013D340[] __asm__("D_0013D340");
extern char mc_data_0013D370[] __asm__("D_0013D370");
extern u8 mc_data_0014EFD0[] __asm__("D_0014EFD0");
extern u8 mc_data_001507D0[] __asm__("D_001507D0");
extern s16 mc_data_001517D8[] __asm__("D_001517D8");
extern s32 mc_data_0015EE84 __asm__("D_0015EE84") MACRO_ADDR;
extern s32 mc_data_0015EF90 __asm__("D_0015EF90") MACRO_ADDR;
extern char mc_data_0015FF68[] __asm__("D_0015FF68");
extern char mc_data_0015FF70[] __asm__("D_0015FF70");
extern u32 mc_data_001A05C0[] __asm__("D_001A05C0");
extern u32 mc_data_001A08C0[] __asm__("D_001A08C0");
extern u8 mc_data_001A0980[] __asm__("D_001A0980");
extern char mc_data_001A09A0[] __asm__("D_001A09A0");
extern char mc_data_001E8518[] __asm__("D_001E8518");

extern void mc_call_001E9730(char *fmt, ...) __asm__("func_001E9730");
extern s32 mc_call_0020BAD8(const u32 *packet) __asm__("func_0020BAD8");
extern void mc_call_001F9978() __asm__("func_001F9978");

extern void mc_call_001FDF10(s32, McChunk **, s32 *) __asm__("func_001FDF10");
extern void mc_call_0020BCB0(u8 *, s32, s32) __asm__("func_0020BCB0");
extern s32 mc_call_0020BD70(u8 *, s32, u32 *) __asm__("func_0020BD70");
extern s32 mc_call_00217628(McChunk *, s32, s32) __asm__("func_00217628");

extern s32 mc_call_001238B0(s32 port, s32 slot, char *name, s32 mode) __asm__("func_001238B0");

extern char *mc_call_001166FC(char *, const char *) __asm__("func_001166FC");

/* Memory card state machine: polls the async call, then steps through card checks. */
void memcard_update_state(void) {
    union {
        char name[0x40];
        struct {
            s32 seen[5];
            char name[0x40] __attribute__((aligned(16)));
        } directory;
    } workspace;
    McState *finished_card;

    if (mc_data_0013D390.busy) {
        mc_data_0013D390.busy = sce_mc_sync(1, &mc_data_0013D390.cmd, &mc_data_0013D390.result) == 0;
        return;
    }
    mc_data_0013D390.busy = 1;

    switch (mc_data_0013D390.state) {
        case 0:
            if (mc_data_0013D390.cur < 0) {
                mc_data_0013D390.cur = 0;
            }
            if (sce_mc_get_info(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                                mc_data_0013D390.card[mc_data_0013D390.cur].slot,
                                &mc_data_0013D390.card[mc_data_0013D390.cur].type,
                                &mc_data_0013D390.card[mc_data_0013D390.cur].free,
                                &mc_data_0013D390.card[mc_data_0013D390.cur].format) == 0) {
                mc_data_0013D390.state = 1;
            }
            break;

        case 1:
            if (mc_data_0013D390.result != 0) {
                mc_data_0013D390.card[mc_data_0013D390.cur].x14 = -4;
                mc_data_0013D390.card[mc_data_0013D390.cur].x1C = mc_data_0013D390.result;
                mc_data_0013D390.card[mc_data_0013D390.cur].xAC = -1;
                mc_data_0013D390.card[mc_data_0013D390.cur].used_blocks = 0;
                if (mc_data_0013D390.card[mc_data_0013D390.cur].request == 1) {
                    if (mc_data_0013D390.result == -1)
                        mc_data_0013D390.card[mc_data_0013D390.cur].request = 2;
                    else
                        mc_data_0013D390.card[mc_data_0013D390.cur].request = 0;
                }
            } else {
                if (mc_data_0013D390.card[mc_data_0013D390.cur].request == 1)
                    mc_data_0013D390.card[mc_data_0013D390.cur].request = 0;
            }
            if (++mc_data_0013D390.cur < 1) {
                mc_data_0013D390.state = 0;
            } else {
                mc_data_0013D390.state = 2;
            }
            mc_data_0013D390.busy = 0;
            break;

        case 2:
            if (mc_data_0013D390.xDC >= 0 && mc_data_0013D390.card[0].request != 3) {
                if (mc_data_0013D390.card[0].x1C == 0) {
                    if (mc_data_0013D390.xE0 >= 0) {
                        mc_data_0013D390.cur = mc_data_0013D390.xE0;
                        mc_data_0013D390.state = mc_data_0013D390.xDC;
                        mc_data_0013D390.err = 0;
                    } else {
                        mc_data_0013D390.err = 0x271A;
                    }
                } else if (mc_data_0013D390.xDC == 7) {
                    mc_data_0013D390.card[0].request = 0;
                }
                mc_data_0013D390.xDC = -1;
                mc_data_0013D390.xE0 = -1;
            } else {
                mc_data_0013D390.cur++;
                if (mc_data_0013D390.card[0].request == 3)
                    mc_data_0013D390.card[0].request = 1;
                if (mc_data_0013D390.cur >= 11 || mc_data_0013D390.card[0].request == 1) {
                    mc_data_0013D390.cur = 0;
                    mc_data_0013D390.state = 0;
                }
            }
            mc_data_0013D390.busy = 0;
            break;

        case 3:
            if (mc_data_0013D390.card[mc_data_0013D390.cur].format == 0) {
                if (sce_mc_format(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                                  mc_data_0013D390.card[mc_data_0013D390.cur].slot) == 0) {
                    mc_data_0013D390.state = 4;
                }
                break;
            }
            mc_data_0013D390.busy = 0;
            mc_data_0013D390.state = 2;
            break;

        case 4:
            if (mc_data_0013D390.result != 0) {
                mc_data_0013D390.err = 1;
                mc_data_0013D390.errCard = mc_data_0013D390.cur;
            }
            mc_data_0013D390.state = 0;
            mc_data_0013D390.cur = 0;
            mc_data_0013D390.busy = 0;
            break;

        case 5:
            if (mc_data_0013D390.card[mc_data_0013D390.cur].format == 0) {
                if (sce_mc_unformat(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                                    mc_data_0013D390.card[mc_data_0013D390.cur].slot) == 0) {
                    mc_data_0013D390.state = 6;
                }
                break;
            }
            mc_data_0013D390.busy = 0;
            mc_data_0013D390.state = 2;
            break;

        case 6:
            if (mc_data_0013D390.result != 0) {
                mc_data_0013D390.err = 2;
                mc_data_0013D390.errCard = mc_data_0013D390.cur;
            }
            mc_data_0013D390.state = 0;
            mc_data_0013D390.cur = 0;
            mc_data_0013D390.busy = 0;
            break;

        case 7:
            if (sce_mc_get_dir(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                               mc_data_0013D390.card[mc_data_0013D390.cur].slot, mc_data_0013D2E8, 0, 11,
                               mc_data_001A0980) == 0) {
                mc_data_0013D390.sub = 0;
                mc_data_0013D390.state = 8;
            }
            break;

        case 8:
            switch (mc_data_0013D390.sub) {
                case 0:
                    if (mc_data_0013D390.result > 0) {
                        s32 icon0;
                        s32 icon1;
                        s32 icon2;
                        s32 i;
                        s32 blocks;
                        icon0 = 0;
                        libc_memset(workspace.directory.seen, 0, 20);
                        icon1 = 0;
                        icon2 = 0;
                        blocks = 0;
                        for (i = 0; i < mc_data_0013D390.result; i++) {
                            McDirEntry *entry = &mc_directory[i];
                            char *filename = (char *)((s32)mc_directory + i * 64 + 32);
                            s32 slot;
                            blocks += (entry->size + 1023U) >> 10;
                            if (libc_strcmp(filename, memcard_icon_path_a_filename) == 0)
                                icon0 = 1;
                            if (libc_strcmp(filename, memcard_icon_path_b_filename) == 0)
                                icon1 = 1;
                            if (libc_strcmp(filename, special_items_base) == 0)
                                icon2 = 1;
                            if (libc_strncmp(filename, memcard_slot_file_prefix, 4) == 0 &&
                                libc_strncmp(filename + 5, memcard_slot_file_suffix, 4) == 0) {
                                slot = (u8)entry->name[4] - '0';
                                if (slot < 0)
                                    continue;
                                if (slot < 5)
                                    workspace.directory.seen[slot] = 1;
                            }
                        }
                        blocks += (mc_data_0013D390.result + 2) / 2;
                        if ((u32)(blocks - 350) < 2 && mc_data_0013D390.result == 10 && icon0 && icon1 && icon2 &&
                            workspace.directory.seen[0] && workspace.directory.seen[1] && workspace.directory.seen[2] &&
                            workspace.directory.seen[3] && workspace.directory.seen[4]) {
                            mc_data_0013D390.card[mc_data_0013D390.cur].used_blocks = 0;
                            mc_data_0013D390.card[mc_data_0013D390.cur].x14 = -1;
                            mc_data_0013D390.sub = 1;
                            mc_data_0013D390.busy = 0;
                        } else {
                            mc_data_0013D390.card[mc_data_0013D390.cur].used_blocks = blocks;
                            mc_data_0013D390.card[mc_data_0013D390.cur].x14 = -2;
                            goto reset_request;
                        }
                    } else {
                        mc_data_0013D390.card[mc_data_0013D390.cur].x14 = -2;
                        if (mc_data_0013D390.result == -2) {
                            mc_data_0013D390.err = 3;
                            mc_data_0013D390.errCard = mc_data_0013D390.cur;
                        } else if (mc_data_0013D390.result >= 0) {
                            mc_data_0013D390.card[mc_data_0013D390.cur].x14 = -2;
                        } else if (mc_data_0013D390.result != -4) {
                            mc_data_0013D390.err = 4;
                            mc_data_0013D390.errCard = mc_data_0013D390.cur;
                        }
                        mc_data_0013D390.state = 0;
                        mc_data_0013D390.cur = 0;
                        mc_data_0013D390.busy = 0;
                        if (mc_data_0013D390.card[0].request == 2)
                            mc_data_0013D390.card[0].request = 0;
                    }
                    break;
                case 1:
                    sprintf(workspace.directory.name, mc_data_0013D370, 0);
                    if (mc_call_001238B0(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                                         mc_data_0013D390.card[mc_data_0013D390.cur].slot, workspace.directory.name,
                                         1) == 0) {
                        mc_data_0013D390.sub++;
                    }
                    break;
                case 2:
                    if (mc_data_0013D390.result >= 0) {
                        mc_data_0013D390.fd = mc_data_0013D390.result;
                        mc_data_0013D390.sub = 3;
                    } else {
                        mc_data_0013D390.card[mc_data_0013D390.cur].x14 = -2;
                        mc_data_0013D390.errCard = mc_data_0013D390.cur;
                        if (mc_data_0013D390.result == -7) {
                            mc_data_0013D390.err = 0x2710;
                        } else if (mc_data_0013D390.result == -5) {
                            mc_data_0013D390.err = 0x2711;
                        } else if (mc_data_0013D390.result == -4) {
                            mc_data_0013D390.err = 0x2712;
                        } else if (mc_data_0013D390.result == -3) {
                            mc_data_0013D390.err = 0x2713;
                        } else if (mc_data_0013D390.result == -2) {
                            mc_data_0013D390.err = 3;
                        } else {
                            mc_data_0013D390.err = 4;
                        }
                        mc_data_0013D390.state = 0;
                        mc_data_0013D390.cur = 0;
                        if (mc_data_0013D390.card[0].request == 2)
                            mc_data_0013D390.card[0].request = 0;
                    }
                    mc_data_0013D390.busy = 0;
                    break;
                case 3:
                    mc_data_0013D390.size = 8;
                    if (sce_mc_read(mc_data_0013D390.fd, &mc_data_0013D390.card[mc_data_0013D390.cur].xB0, 8) == 0) {
                        mc_data_0013D390.sub = 4;
                    }
                    break;
                case 4:
                    if (mc_data_0013D390.result == mc_data_0013D390.size) {
                        mc_data_0013D390.card[mc_data_0013D390.cur].xAC = 0;
                        if (mc_data_0013D390.card[mc_data_0013D390.cur].xB0 != mc_call_0020BAD8(mc_data_001A05C0)) {
                            mc_data_0013D390.card[mc_data_0013D390.cur].xAC++;
                        }
                        if (mc_data_0013D390.card[mc_data_0013D390.cur].xB4 != mc_call_0020BAD8(mc_data_001A08C0)) {
                            mc_data_0013D390.card[mc_data_0013D390.cur].xAC++;
                        }
                        mc_data_0013D390.busy = 0;
                        mc_data_0013D390.sub = 5;
                    } else {
                        mc_data_0013D390.card[mc_data_0013D390.cur].x14 = -2;
                        mc_data_0013D390.errCard = mc_data_0013D390.cur;
                        if (mc_data_0013D390.result >= 0) {
                            mc_data_0013D390.err = 0x2716;
                            if (sce_mc_close(mc_data_0013D390.fd) == 0) {
                                mc_data_0013D390.state = 0;
                                mc_data_0013D390.cur = 0;
                                mc_data_0013D390.busy = 0;
                                if (mc_data_0013D390.card[0].request == 2)
                                    mc_data_0013D390.card[0].request = 0;
                            }
                            break;
                        }
                        if (mc_data_0013D390.result == -5) {
                            mc_data_0013D390.err = 0x2711;
                        } else if (mc_data_0013D390.result == -4) {
                            mc_data_0013D390.err = 0x2714;
                        } else if (mc_data_0013D390.result == -3) {
                            mc_data_0013D390.err = 0x2715;
                        } else if (mc_data_0013D390.result == -2) {
                            mc_data_0013D390.err = 3;
                        } else {
                            mc_data_0013D390.err = 4;
                        }
                        mc_data_0013D390.state = 0;
                        mc_data_0013D390.cur = 0;
                        mc_data_0013D390.busy = 0;
                        if (mc_data_0013D390.card[0].request == 2)
                            mc_data_0013D390.card[0].request = 0;
                    }
                    break;
                case 5:
                    if (sce_mc_close(mc_data_0013D390.fd) == 0) {
                        mc_data_0013D390.busy = 0;
                        mc_data_0013D390.state = 21;
                    }
                    break;
            }
            break;

        case 9:
            mc_data_0013D390.state = 10;
            mc_data_0013D390.sub = 0;
            mc_data_0013D390.card[mc_data_0013D390.cur].x14 = 0;
            /* fall through */
        case 10:
            switch (mc_data_0013D390.sub) {
                case 0:
                    if (mc_data_0013D390.card[mc_data_0013D390.cur].free +
                            mc_data_0013D390.card[mc_data_0013D390.cur].used_blocks >=
                        350) {
                        mc_data_0013D390.sub = 1;
                    } else {
                        mc_data_0013D390.err = 7;
                        mc_data_0013D390.errCard = mc_data_0013D390.cur;
                        mc_data_0013D390.state = 0;
                        mc_data_0013D390.cur = 0;
                    }
                    mc_data_0013D390.busy = 0;
                    break;
                case 1:
                    if (sce_mc_mkdir(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                                     mc_data_0013D390.card[mc_data_0013D390.cur].slot, mc_data_0013D2D0) == 0) {
                        mc_data_0013D390.sub = 2;
                    }
                    break;
                case 2:
                    if (mc_data_0013D390.result == 0 || mc_data_0013D390.result == -4) {
                        mc_data_0013D390.sub = 3;
                        break;
                    }
                    mc_data_0013D390.errCard = mc_data_0013D390.cur;
                    if (mc_data_0013D390.result == -3) {
                        mc_data_0013D390.err = 7;
                    } else if (mc_data_0013D390.result == -2) {
                        mc_data_0013D390.err = 6;
                    } else {
                        mc_data_0013D390.err = 0xD;
                    }
                    mc_data_0013D390.state = 0;
                    mc_data_0013D390.cur = 0;
                    mc_data_0013D390.busy = 0;
                    break;
                case 3: {
                    McChunk *chunk;
                    s32 size;

                    mc_call_001FDF10(mc_data_00137C80.x14 << 11, &chunk, &size);
                    mc_call_00217628(chunk, mc_data_00137C80.x10, mc_data_00137C80.x14);
                    mc_data_0013D390.sub = 4;
                    mc_data_0013D390.busy = 0;
                    break;
                }
                case 4:
                    if (mc_data_001517D8[0] == 0) {
                        mc_data_0013D390.sub = 5;
                    }
                    mc_data_0013D390.busy = 0;
                    break;
                case 5:
                case 9:
                case 13:
                case 18:
                    switch (mc_data_0013D390.sub) {
                        case 5:
                            mc_call_001166FC(workspace.name, mc_data_0013D300);
                            break;
                        case 9:
                            mc_call_001166FC(workspace.name, mc_data_0013D320);
                            break;
                        case 18:
                            mc_call_001166FC(workspace.name, mc_data_0013D340);
                            break;
                        case 13:
                            sprintf(workspace.name, mc_data_0013D370, mc_data_0013D390.card[mc_data_0013D390.cur].x14);
                            break;
                    }
                    if (mc_call_001238B0(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                                         mc_data_0013D390.card[mc_data_0013D390.cur].slot, workspace.name,
                                         0x203) == 0) {
                        mc_data_0013D390.fd = -1;
                        mc_data_0013D390.sub++;
                    }
                    break;
                case 6:
                case 10:
                case 14:
                case 19: {
                    McChunk *chunk;
                    s32 size;

                    if (mc_data_0013D390.result >= 0) {
                        if (mc_data_0013D390.fd < 0) {
                            mc_data_0013D390.fd = mc_data_0013D390.result;
                        }
                        mc_call_001FDF10(mc_data_00137C94[0] << 11, &chunk, &size);
                        switch (mc_data_0013D390.sub) {
                            case 6:
                                mc_data_0013D390.size = 0x3C4;
                                mc_data_0013D390.buf = (u8 *)chunk + chunk->x0;
                                break;
                            case 10:
                                mc_data_0013D390.size = chunk->xC;
                                mc_data_0013D390.buf = (u8 *)chunk + chunk->x8;
                                break;
                            case 19:
                                if (mc_data_0015EF90 != 0) {
                                    mc_data_0013D390.size = 0x3C00;
                                } else {
                                    mc_data_0013D390.size = 0x3C04;
                                }
                                mc_data_0013D390.buf = &mc_data_0015EE84;
                                break;
                            case 14: {
                                s32 n = mc_call_0020BAD8(mc_data_001A05C0);
                                mc_data_0013D390.size = n + mc_call_0020BAD8(mc_data_001A08C0) * 20 + 8;
                                mc_data_0013D390.buf = (u8 *)chunk + chunk->x10;
                                break;
                            }
                        }
                        if (sce_mc_write_buffer(mc_data_0013D390.fd, mc_data_0013D390.buf, mc_data_0013D390.size) ==
                            0) {
                            mc_data_0013D390.sub++;
                        }
                    } else {
                        mc_data_0013D390.err = 0xA;
                        mc_data_0013D390.errCard = mc_data_0013D390.cur;
                        mc_data_0013D390.state = 0;
                        mc_data_0013D390.cur = 0;
                        mc_data_0013D390.busy = 0;
                    }
                    break;
                }
                case 7:
                case 11:
                case 15:
                case 20:
                    if (mc_data_0013D390.result == mc_data_0013D390.size) {
                        if (sce_mc_close(mc_data_0013D390.fd) == 0) {
                            mc_data_0013D390.sub++;
                        }
                        break;
                    }
                    mc_data_0013D390.errCard = mc_data_0013D390.cur;
                    if (mc_data_0013D390.result >= 0) {
                        finished_card = &mc_data_0013D390;
                        finished_card->err = 0xB;
                        if (sce_mc_close(finished_card->fd) == 0) {
                            finished_card->busy = 0;
                            goto reset_state;
                        }
                        break;
                    }
                    if (mc_data_0013D390.result == -4) {
                        mc_data_0013D390.err = 8;
                    } else if (mc_data_0013D390.result == -3) {
                        mc_data_0013D390.err = 7;
                    } else if (mc_data_0013D390.result == -2) {
                        mc_data_0013D390.err = 6;
                    } else {
                        mc_data_0013D390.err = 0xD;
                    }
                    mc_data_0013D390.state = 0;
                    mc_data_0013D390.cur = 0;
                    mc_data_0013D390.busy = 0;
                    break;
                case 8:
                case 12:
                case 16:
                    if (mc_data_0013D390.result == 0) {
                        mc_data_0013D390.sub++;
                    } else {
                        mc_data_0013D390.err = 0xC;
                        mc_data_0013D390.errCard = mc_data_0013D390.cur;
                        mc_data_0013D390.state = 0;
                        mc_data_0013D390.cur = 0;
                    }
                    mc_data_0013D390.busy = 0;
                    break;
                case 17:
                    if (++mc_data_0013D390.card[mc_data_0013D390.cur].x14 < 5) {
                        mc_data_0013D390.sub = 13;
                    } else {
                        mc_data_0013D390.sub++;
                    }
                    mc_data_0013D390.busy = 0;
                    break;
                case 21:
                    mc_data_0013D390.card[0].x14 = -1;
                    mc_data_0013D390.state = 0;
                    mc_data_0013D390.cur = 0;
                    mc_data_0013D390.card[0].xAC = 0;
                    break;
            }
            break;

        case 11:
            mc_data_0013D390.state = 12;
            mc_data_0013D390.sub = 0;
            mc_data_0013D390.card[mc_data_0013D390.cur].x14 = -2;
            /* fall through */
        case 12:
            switch (mc_data_0013D390.sub) {
                case 0:
                    if (sce_mc_get_dir(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                                       mc_data_0013D390.card[mc_data_0013D390.cur].slot, mc_data_0013D2E8, 0, 1,
                                       mc_data_001A0980) == 0) {
                        mc_data_0013D390.sub = 1;
                    }
                    break;
                case 1:
                    if (mc_data_0013D390.result == 1) {
                        sprintf(workspace.name, mc_data_0015FF68, mc_data_0013D2D0, mc_data_001A09A0);
                        mc_call_001E9730(mc_data_0015FF70, workspace.name);
                        if (sce_mc_delete(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                                          mc_data_0013D390.card[mc_data_0013D390.cur].slot, workspace.name) == 0) {
                            mc_data_0013D390.sub = 2;
                        }
                    } else {
                        if (mc_data_0013D390.result == 0) {
                            mc_data_0013D390.sub = 3;
                        } else {
                            mc_data_0013D390.errCard = mc_data_0013D390.cur;
                            if (mc_data_0013D390.result == -5) {
                                mc_data_0013D390.err = 0x10;
                            } else if (mc_data_0013D390.result == -4) {
                                mc_data_0013D390.err = 0xF;
                            } else if (mc_data_0013D390.result == -2) {
                                mc_data_0013D390.err = 0xE;
                            } else {
                                mc_data_0013D390.err = 0x12;
                            }
                            mc_data_0013D390.state = 0;
                            mc_data_0013D390.cur = 0;
                        }
                        mc_data_0013D390.busy = 0;
                    }
                    break;
                case 2:
                    if (sce_mc_get_dir(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                                       mc_data_0013D390.card[mc_data_0013D390.cur].slot, mc_data_0013D2E8, 1, 1,
                                       mc_data_001A0980) == 0) {
                        mc_data_0013D390.sub = 1;
                    }
                    break;
                case 3:
                    if (sce_mc_delete(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                                      mc_data_0013D390.card[mc_data_0013D390.cur].slot, mc_data_0013D2D0) == 0) {
                        mc_call_001E9730(mc_data_001E8518, mc_data_0013D2D0);
                        mc_data_0013D390.sub = 4;
                    }
                    break;
                case 4:
                    if (mc_data_0013D390.result != 0) {
                        mc_data_0013D390.errCard = mc_data_0013D390.cur;
                        if (mc_data_0013D390.result == -6) {
                            mc_data_0013D390.err = 0x11;
                        } else if (mc_data_0013D390.result == -5) {
                            mc_data_0013D390.err = 0x10;
                        } else if (mc_data_0013D390.result == -4) {
                            mc_data_0013D390.err = 0xF;
                        } else if (mc_data_0013D390.result == -2) {
                            mc_data_0013D390.err = 0xE;
                        } else {
                            mc_data_0013D390.err = 0x12;
                        }
                    }
                    mc_data_0013D390.state = 0;
                    mc_data_0013D390.cur = 0;
                    mc_data_0013D390.busy = 0;
                    break;
            }
            break;

        case 21:
            mc_data_0013D390.state = 22;
            mc_data_0013D390.card[mc_data_0013D390.cur].x18 = -1;
            mc_data_0013D390.xC8 = 0;
            break;

        case 22:
            mc_data_0013D390.card[mc_data_0013D390.cur].x18++;
            if (mc_data_0013D390.card[mc_data_0013D390.cur].x18 < 5) {
                mc_data_0013D390.sub = 0;
                mc_data_0013D390.state = 23;
            } else {
                mc_data_0013D390.state = 0;
                mc_data_0013D390.cur = 0;
                mc_data_0013D390.busy = 0;
                mc_data_0013D390.card[0].request = 0;
            }
            break;
        case 13:
            if (mc_data_0013D390.card[mc_data_0013D390.cur].x14 < 0) {
                mc_data_0013D390.err = 0x13;
                mc_data_0013D390.state = 0;
                mc_data_0013D390.cur = 0;
                mc_data_0013D390.busy = 0;
                if (mc_data_0013D390.card[0].request == 2)
                    mc_data_0013D390.card[0].request = 0;
                break;
            }
            mc_data_0013D390.xC8 = 0;
            mc_data_0013D390.sub = 0;
            mc_data_0013D390.state = 14;
            /* fall through */
        case 14:
        case 23:
            switch (mc_data_0013D390.sub) {
                case 0:
                    if (mc_data_0013D390.state == 23) {
                        sprintf(workspace.name, mc_data_0013D370, mc_data_0013D390.card[mc_data_0013D390.cur].x18);
                    } else {
                        sprintf(workspace.name, mc_data_0013D370, mc_data_0013D390.card[mc_data_0013D390.cur].x14);
                    }
                    if (mc_call_001238B0(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                                         mc_data_0013D390.card[mc_data_0013D390.cur].slot, workspace.name, 1) == 0) {
                        mc_data_0013D390.sub++;
                    }
                    break;
                case 1:
                    if (mc_data_0013D390.result >= 0) {
                        mc_data_0013D390.fd = mc_data_0013D390.result;
                        mc_data_0013D390.sub = 2;
                    } else {
                        mc_data_0013D390.errCard = mc_data_0013D390.cur;
                        if (mc_data_0013D390.result == -7) {
                            mc_data_0013D390.err = 0x14;
                        } else if (mc_data_0013D390.result == -5) {
                            mc_data_0013D390.err = 0x15;
                        } else if (mc_data_0013D390.result == -4) {
                            mc_data_0013D390.err = 0x16;
                        } else if (mc_data_0013D390.result == -3) {
                            mc_data_0013D390.err = 0x17;
                        } else if (mc_data_0013D390.result == -2) {
                            mc_data_0013D390.err = 0x18;
                        } else {
                            mc_data_0013D390.err = 0x1C;
                        }
                        mc_data_0013D390.state = 0;
                        mc_data_0013D390.cur = 0;
                        if (mc_data_0013D390.card[0].request == 2)
                            mc_data_0013D390.card[0].request = 0;
                    }
                    mc_data_0013D390.busy = 0;
                    break;
                case 2:
                    mc_data_0013D390.size = 8;
                    if (sce_mc_read(mc_data_0013D390.fd, &mc_data_0013D390.card[mc_data_0013D390.cur].xB0, 8) == 0) {
                        mc_data_0013D390.sub = 3;
                    }
                    break;
                case 3:
                    if (mc_data_0013D390.result == mc_data_0013D390.size) {
                        mc_data_0013D390.busy = 0;
                        mc_data_0013D390.sub = 4;
                        break;
                    }
                    mc_data_0013D390.errCard = mc_data_0013D390.cur;
                    if (mc_data_0013D390.result >= 0) {
                        mc_data_0013D390.err = 0x1B;
                        if (sce_mc_close(mc_data_0013D390.fd) == 0) {
                            mc_data_0013D390.state = 0;
                            mc_data_0013D390.cur = 0;
                            mc_data_0013D390.busy = 0;
                            if (mc_data_0013D390.card[0].request == 2)
                                mc_data_0013D390.card[0].request = 0;
                        }
                        break;
                    }
                    if (mc_data_0013D390.result == -5) {
                        mc_data_0013D390.err = 0x15;
                    } else if (mc_data_0013D390.result == -4) {
                        mc_data_0013D390.err = 0x19;
                    } else if (mc_data_0013D390.result == -3) {
                        mc_data_0013D390.err = 0x1A;
                    } else if (mc_data_0013D390.result == -2) {
                        mc_data_0013D390.err = 0x18;
                    } else {
                        mc_data_0013D390.err = 0x1C;
                    }
                    mc_data_0013D390.state = 0;
                    mc_data_0013D390.cur = 0;
                    mc_data_0013D390.busy = 0;
                    if (mc_data_0013D390.card[0].request == 2)
                        mc_data_0013D390.card[0].request = 0;
                    break;
                case 4:
                    mc_data_0013D390.size = mc_data_0013D390.card[mc_data_0013D390.cur].xB0;
                    if (mc_data_0013D390.size > 0x1800) {
                        mc_call_001F9978();
                    }
                    if (sce_mc_read(mc_data_0013D390.fd, mc_data_0014EFD0, mc_data_0013D390.size) == 0) {
                        mc_data_0013D390.sub = 5;
                    }
                    break;
                case 5:
                    if (mc_data_0013D390.result == mc_data_0013D390.size) {
                        if (mc_data_0013D390.state == 23) {
                            mc_call_0020BCB0(mc_data_0014EFD0, mc_data_0013D390.cur,
                                             mc_data_0013D390.card[mc_data_0013D390.cur].x18);
                            mc_data_0013D390.sub = 8;
                        } else {
                            mc_data_0013D390.card[mc_data_0013D390.cur].xAC =
                                mc_call_0020BD70(mc_data_0014EFD0, 0, mc_data_001A05C0);
                            mc_data_0013D390.sub = 6;
                        }
                        mc_data_0013D390.busy = 0;
                        break;
                    }
                    mc_data_0013D390.errCard = mc_data_0013D390.cur;
                    if (mc_data_0013D390.result >= 0) {
                        mc_data_0013D390.err = 0x1B;
                        if (sce_mc_close(mc_data_0013D390.fd) == 0) {
                            mc_data_0013D390.state = 0;
                            mc_data_0013D390.cur = 0;
                            mc_data_0013D390.busy = 0;
                            if (mc_data_0013D390.card[0].request == 2)
                                mc_data_0013D390.card[0].request = 0;
                        }
                        break;
                    }
                    if (mc_data_0013D390.result == -5) {
                        mc_data_0013D390.err = 0x15;
                    } else if (mc_data_0013D390.result == -4) {
                        mc_data_0013D390.err = 0x19;
                    } else if (mc_data_0013D390.result == -3) {
                        mc_data_0013D390.err = 0x1A;
                    } else if (mc_data_0013D390.result == -2) {
                        mc_data_0013D390.err = 0x18;
                    } else {
                        mc_data_0013D390.err = 0x1C;
                    }
                    mc_data_0013D390.state = 0;
                    mc_data_0013D390.cur = 0;
                    mc_data_0013D390.busy = 0;
                    if (mc_data_0013D390.card[0].request == 2)
                        mc_data_0013D390.card[0].request = 0;
                    break;
                case 6:
                    mc_data_0013D390.size = mc_data_0013D390.card[mc_data_0013D390.cur].xB4;
                    if (mc_data_0013D390.size > 0x1000) {
                        mc_call_001F9978();
                    }
                    if (sce_mc_read(mc_data_0013D390.fd, mc_data_001507D0, mc_data_0013D390.size) == 0) {
                        mc_data_0013D390.sub = 7;
                    }
                    break;
                case 7:
                    if (mc_data_0013D390.result == mc_data_0013D390.size) {
                        mc_data_0013D390.card[mc_data_0013D390.cur].xAC +=
                            mc_call_0020BD70(mc_data_001507D0, mc_data_0013D390.xC8, mc_data_001A08C0);
                        if (++mc_data_0013D390.xC8 < 20) {
                            mc_data_0013D390.sub = 6;
                        } else {
                            mc_data_0013D390.sub = 8;
                        }
                        mc_data_0013D390.busy = 0;
                        break;
                    }
                    mc_data_0013D390.errCard = mc_data_0013D390.cur;
                    if (mc_data_0013D390.result >= 0) {
                        mc_data_0013D390.err = 0x1B;
                        if (sce_mc_close(mc_data_0013D390.fd) == 0) {
                            mc_data_0013D390.state = 0;
                            mc_data_0013D390.cur = 0;
                            mc_data_0013D390.busy = 0;
                            if (mc_data_0013D390.card[0].request == 2)
                                mc_data_0013D390.card[0].request = 0;
                        }
                        break;
                    }
                    if (mc_data_0013D390.result == -5) {
                        mc_data_0013D390.err = 0x15;
                    } else if (mc_data_0013D390.result == -4) {
                        mc_data_0013D390.err = 0x19;
                    } else if (mc_data_0013D390.result == -3) {
                        mc_data_0013D390.err = 0x1A;
                    } else if (mc_data_0013D390.result == -2) {
                        mc_data_0013D390.err = 0x18;
                    } else {
                        mc_data_0013D390.err = 0x1C;
                    }
                reset_request:
                    mc_data_0013D390.state = 0;
                    mc_data_0013D390.cur = 0;
                    mc_data_0013D390.busy = 0;
                    if (mc_data_0013D390.card[0].request == 2)
                        mc_data_0013D390.card[0].request = 0;
                    break;
                case 8:
                    if (sce_mc_close(mc_data_0013D390.fd) == 0) {
                        if (mc_data_0013D390.state == 23) {
                            mc_data_0013D390.busy = 0;
                            mc_data_0013D390.state = 22;
                        } else {
                            finished_card = &mc_data_0013D390;
                            finished_card->card[0].request = 0;
                            goto reset_state;
                        }
                    }
                    break;
            }
            break;

        case 15:
            if (mc_data_0013D390.card[mc_data_0013D390.cur].xAC != 0) {
                mc_data_0013D390.busy = 0;
                mc_data_0013D390.state = 0;
                mc_data_0013D390.cur = 0;
                mc_data_0013D390.err = 0x2717;
                break;
            }
            if (mc_data_0013D390.card[mc_data_0013D390.cur].x14 < 0) {
                mc_data_0013D390.busy = 0;
                mc_data_0013D390.state = 0;
                mc_data_0013D390.cur = 0;
                mc_data_0013D390.err = 0x1D;
                break;
            }
            mc_data_0013D390.sub = 0;
            mc_data_0013D390.state = 16;
            /* fall through */
        case 16:
            switch (mc_data_0013D390.sub) {
                case 0:
                    sprintf(workspace.name, mc_data_0013D370, mc_data_0013D390.card[mc_data_0013D390.cur].x14);
                    if (mc_call_001238B0(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                                         mc_data_0013D390.card[mc_data_0013D390.cur].slot, workspace.name, 2) == 0) {
                        mc_data_0013D390.sub = 1;
                    }
                    break;
                case 1:
                    if (mc_data_0013D390.result >= 0) {
                        mc_data_0013D390.fd = mc_data_0013D390.result;
                        mc_data_0013D390.sub = 2;
                    } else {
                        mc_data_0013D390.errCard = mc_data_0013D390.cur;
                        if (mc_data_0013D390.result == -7) {
                            mc_data_0013D390.err = 0x1E;
                        } else if (mc_data_0013D390.result == -5) {
                            mc_data_0013D390.err = 0x1F;
                        } else if (mc_data_0013D390.result == -4) {
                            mc_data_0013D390.err = 0x20;
                        } else if (mc_data_0013D390.result == -3) {
                            mc_data_0013D390.err = 0x21;
                        } else if (mc_data_0013D390.result == -2) {
                            mc_data_0013D390.err = 0x22;
                        } else {
                            mc_data_0013D390.err = 0x26;
                        }
                        mc_data_0013D390.state = 0;
                        mc_data_0013D390.cur = 0;
                    }
                    mc_data_0013D390.busy = 0;
                    break;
                case 2:
                    if (sce_mc_seek(mc_data_0013D390.fd, 8, 0) == 0) {
                        mc_data_0013D390.sub = 3;
                    }
                    break;
                case 3:
                    if (mc_data_0013D390.result >= 0) {
                        mc_data_0013D390.sub = 4;
                    } else {
                        mc_data_0013D390.state = 0;
                        mc_data_0013D390.cur = 0;
                        mc_data_0013D390.busy = 0;
                    }
                    break;
                case 4:
                    if (sce_mc_write_buffer(mc_data_0013D390.fd, mc_data_0014EFD0,
                                            mc_call_0020BAD8(mc_data_001A05C0)) == 0) {
                        mc_data_0013D390.sub = 5;
                    }
                    break;
                case 5:
                    if (mc_data_0013D390.result == mc_call_0020BAD8(mc_data_001A05C0)) {
                        mc_data_0013D390.sub = 6;
                        break;
                    }
                    mc_data_0013D390.errCard = mc_data_0013D390.cur;
                    if (mc_data_0013D390.result >= 0) {
                        finished_card = &mc_data_0013D390;
                        finished_card->err = 0x25;
                        if (sce_mc_close(finished_card->fd) == 0) {
                            finished_card->busy = 0;
                            goto reset_state;
                        }
                        break;
                    }
                    if (mc_data_0013D390.result == -5) {
                        mc_data_0013D390.err = 0x1F;
                    } else if (mc_data_0013D390.result == -4) {
                        mc_data_0013D390.err = 0x23;
                    } else if (mc_data_0013D390.result == -3) {
                        mc_data_0013D390.err = 0x24;
                    } else if (mc_data_0013D390.result == -2) {
                        mc_data_0013D390.err = 0x22;
                    } else {
                        mc_data_0013D390.err = 0x26;
                    }
                    mc_data_0013D390.state = 0;
                    mc_data_0013D390.cur = 0;
                    mc_data_0013D390.busy = 0;
                    break;
                case 6:
                    if (mc_data_0013D390.xC8 == 0) {
                        mc_data_0013D390.sub = 8;
                        break;
                    }
                    if (sce_mc_seek(mc_data_0013D390.fd, mc_data_0013D390.xC8 * mc_call_0020BAD8(mc_data_001A08C0),
                                    1) == 0) {
                        mc_data_0013D390.sub = 7;
                    }
                    break;
                case 7:
                    if (mc_data_0013D390.result >= 0) {
                        mc_data_0013D390.sub = 8;
                    } else {
                        mc_data_0013D390.state = 0;
                        mc_data_0013D390.cur = 0;
                        mc_data_0013D390.busy = 0;
                    }
                    break;
                case 8:
                    if (sce_mc_write_buffer(mc_data_0013D390.fd, mc_data_001507D0,
                                            mc_call_0020BAD8(mc_data_001A08C0)) == 0) {
                        mc_data_0013D390.sub = 9;
                    }
                    break;
                case 9:
                    if (mc_data_0013D390.result == mc_call_0020BAD8(mc_data_001A08C0)) {
                        mc_data_0013D390.sub = 10;
                        break;
                    }
                    mc_data_0013D390.errCard = mc_data_0013D390.cur;
                    if (mc_data_0013D390.result >= 0) {
                        finished_card = &mc_data_0013D390;
                        finished_card->err = 0x25;
                        if (sce_mc_close(finished_card->fd) == 0) {
                            finished_card->busy = 0;
                        reset_state:
                            finished_card->cur = 0;
                            finished_card->state = 0;
                        }
                        break;
                    }
                    if (mc_data_0013D390.result == -5) {
                        mc_data_0013D390.err = 0x1F;
                    } else if (mc_data_0013D390.result == -4) {
                        mc_data_0013D390.err = 0x23;
                    } else if (mc_data_0013D390.result == -3) {
                        mc_data_0013D390.err = 0x24;
                    } else if (mc_data_0013D390.result == -2) {
                        mc_data_0013D390.err = 0x18;
                    } else {
                        mc_data_0013D390.err = 0x1C;
                    }
                    mc_data_0013D390.state = 0;
                    mc_data_0013D390.cur = 0;
                    mc_data_0013D390.busy = 0;
                    break;
                case 10:
                    finished_card = &mc_data_0013D390;
                    if (sce_mc_close(finished_card->fd) == 0) {
                        finished_card->state = 0;
                        finished_card->cur = 0;
                    }
                    break;
            }
            break;

        case 17: {
            McChunk *chunk;
            s32 size;

            mc_call_001FDF10(mc_data_00137C80.x14 << 11, &chunk, &size);
            mc_call_00217628(chunk, mc_data_00137C80.x10, mc_data_00137C80.x14);
            mc_data_0013D390.sub = 18;
            mc_data_0013D390.busy = 0;
            break;
        }

        case 18:
            if (mc_data_001517D8[0] == 0) {
                McChunk *chunk;
                s32 size;

                mc_call_001FDF10(mc_data_00137C94[0] << 11, &chunk, &size);
                mc_data_0013D390.sub = 19;
                mc_data_0013D390.buf = (u8 *)chunk + chunk->x10;
            }
            mc_data_0013D390.busy = 0;
            break;

        case 19:
            if (mc_data_0013D390.card[mc_data_0013D390.cur].xAC != 0) {
                mc_data_0013D390.busy = 0;
                mc_data_0013D390.state = 0;
                mc_data_0013D390.cur = 0;
                mc_data_0013D390.err = 0x2718;
                break;
            }
            if (mc_data_0013D390.card[mc_data_0013D390.cur].x14 < 0) {
                mc_data_0013D390.busy = 0;
                mc_data_0013D390.state = 0;
                mc_data_0013D390.cur = 0;
                mc_data_0013D390.err = 0x27;
                break;
            }
            mc_data_0013D390.xC8 = 0;
            mc_data_0013D390.sub = 0;
            mc_data_0013D390.state = 20;
            /* fall through */
        case 20:
            switch (mc_data_0013D390.sub) {
                case 0:
                    sprintf(workspace.name, mc_data_0013D370, mc_data_0013D390.card[mc_data_0013D390.cur].x14);
                    if (mc_call_001238B0(mc_data_0013D390.card[mc_data_0013D390.cur].port,
                                         mc_data_0013D390.card[mc_data_0013D390.cur].slot, workspace.name, 2) == 0) {
                        mc_data_0013D390.sub = 1;
                    }
                    break;
                case 1:
                    if (mc_data_0013D390.result >= 0) {
                        s32 n;

                        mc_data_0013D390.fd = mc_data_0013D390.result;
                        n = mc_call_0020BAD8(mc_data_001A05C0);
                        mc_data_0013D390.size = n + mc_call_0020BAD8(mc_data_001A08C0) * 20 + 8;
                        if (sce_mc_write_buffer(mc_data_0013D390.fd, mc_data_0013D390.buf, mc_data_0013D390.size) ==
                            0) {
                            mc_data_0013D390.sub = 2;
                        }
                        break;
                    }
                    mc_data_0013D390.err = 0x2B;
                    mc_data_0013D390.errCard = mc_data_0013D390.cur;
                    mc_data_0013D390.state = 0;
                    mc_data_0013D390.cur = 0;
                    mc_data_0013D390.busy = 0;
                    break;
                case 2:
                    if (mc_data_0013D390.result == mc_data_0013D390.size) {
                        if (sce_mc_close(mc_data_0013D390.fd) == 0) {
                            mc_data_0013D390.sub = 3;
                        }
                        break;
                    }
                    mc_data_0013D390.errCard = mc_data_0013D390.cur;
                    if (mc_data_0013D390.result >= 0) {
                        mc_data_0013D390.err = 0xB;
                        if (sce_mc_close(mc_data_0013D390.fd) == 0) {
                            mc_data_0013D390.state = 0;
                            mc_data_0013D390.cur = 0;
                            mc_data_0013D390.busy = 0;
                        }
                        break;
                    }
                    if (mc_data_0013D390.result == -4) {
                        mc_data_0013D390.err = 0x28;
                    } else if (mc_data_0013D390.result == -3) {
                        mc_data_0013D390.err = 0x29;
                    } else if (mc_data_0013D390.result == -2) {
                        mc_data_0013D390.err = 0x2A;
                    } else {
                        mc_data_0013D390.err = 0x2D;
                    }
                    mc_data_0013D390.state = 0;
                    mc_data_0013D390.cur = 0;
                    mc_data_0013D390.busy = 0;
                    break;
                case 3:
                    if (mc_data_0013D390.result != 0) {
                        mc_data_0013D390.err = 0x2C;
                        mc_data_0013D390.errCard = mc_data_0013D390.cur;
                    }
                    mc_data_0013D390.state = 0;
                    mc_data_0013D390.cur = 0;
                    mc_data_0013D390.busy = 0;
                    break;
            }
            break;
    }
}

extern int mc_gamedata[];
extern int mc_leveldata[];
extern int memcard_GetDataSize(int *p);
extern int memcard_PrepData(void *dst, int i, int *table);

/* Serialise both descriptor tables into the caller's buffer: the two
   sizes first, then one blob for mc_gamedata and twenty for mc_leveldata,
   each call returning how far to advance. The write cursor is the
   parameter itself -- retail keeps both in $s2. */
/* memcard_MakeWholeSave */
void memcard_MakeWholeSave(char *out) {
    int i;

    *(int *)out = memcard_GetDataSize(mc_gamedata);
    *(int *)(out + 4) = memcard_GetDataSize(mc_leveldata);
    out += 8;
    out += memcard_PrepData(out, 0, mc_gamedata);
    for (i = 0; i < 0x14; i++) {
        out += memcard_PrepData(out, i, mc_leveldata);
    }
}

/* Unprototyped deliberately: this is the varargs function, so callers
   legitimately pass different argument counts/types, and two agents
   declared it incompatibly. Both callers are byte-exact and must not
   be edited; an unprototyped declaration satisfies all call sites
   with identical codegen. (Only varargs *definitions* need stdarg.h;
   calling one is fine.) */

extern char memcard_error_message[];

/* memcard_Init */
void memcard_Init(void) {
    if (sce_mc_init()) {
        STUB_printf(memcard_error_message);
    }
}

/* memcard_GetDataSize */
int memcard_GetDataSize(int *p) {
    int n = 8;
    while (p[0] != 0) {
        n += 8;
        n += p[1];
        p += 4;
        n = (n + 3) & ~3;
    }
    return n + 8;
}

/* memcard_Checksum: 0 for more than 0x1800 bytes, else a 16-bit
   shift-register checksum (polynomial 0x1F45). Only the low half of
   the 0xEDB88320 seed (the CRC-32 polynomial) reaches the result. */
int memcard_Checksum(void *data, int len) {
    unsigned char *p = data;
    unsigned char *end;
    int crc;
    int j;

    if (len > 0x1800) {
        return 0;
    }
    end = p + len;
    crc = 0xEDB88320;
    while (p < end) {
        crc ^= *p++ << 8;
        for (j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1F45;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc & 0xFFFF;
}

/* memcard_TestChecksum. `len` is read before the test: retail's load sits
   in the beqz slot, and a load that may trap is never taken from the
   fall-through. */
int memcard_TestChecksum(char *buf) {
    int *p = (int *)buf;
    int len = p[0];
    int sum = p[1];
    int result = 0;
    if (sum != 0)
        result = memcard_Checksum(p + 2, len) == sum;
    return result;
}

extern int memcard_Checksum(void *data, int len);

/* Walks a descriptor table (four words per entry: base, element size,
   tag, unused) and writes one { tag, size, data } record per entry into
   the caller's buffer, four-byte aligned, terminated by { -1, 0 }. The
   header it leaves at the front is { payload length, checksum }, and the
   return value is the total including that header. */
/*
 * Byte mismatch, correct size (0x E8), 39 of 232 bytes: the allocator puts
 * the table cursor in $s1 and the write cursor in $s0, where retail has
 * them the other way round, and it initialises the cursor before the
 * `table[0] != 0` test instead of after it. Everything else is retail's
 * instruction sequence.
 *
 * Spellings tried, all identical output: a separate `int *t = table` local
 * versus advancing the parameter itself; the source address computed at
 * the call versus in a local. That local IS load-bearing for the rest --
 * computing it before the two header stores is what makes the compiler
 * reload table[1] for them, as retail does (1807 -> 517 differing words).
 * The remaining residual is allocator destination choice, the recorded
 * dead end.
 */
struct SaveBlock {
    unsigned char *base;
    int size;
    int id;
    int pad;
};

struct SaveHeader {
    int size;
    int checksum;
};

/* memcard_PrepData: pack the save blocks (id, size, data, 4-aligned) after
   an 8-byte header, end with an id -1 record, checksum it. Adapted from
   Lombyte's project (GPL-3.0) for PAL. */
int memcard_PrepData(void *dst, int slot, int *table) {
    struct SaveHeader *out = dst;
    struct SaveBlock *blk = table;
    unsigned char *p;
    int total;
    unsigned char *src;

    total = 0;
    p = (unsigned char *)(out + 1);
    if (blk->base != 0) {
        struct SaveBlock *block = blk;
        int mask = -4;

        do {
            src = block->base + slot * block->size;
            total += 8;
            ((int *)p)[0] = block->id;
            ((int *)p)[1] = block->size;
            p += 8;
            copy_memory_aligned(p, src, block->size);
            p += block->size;
            total += block->size;
            p = (unsigned char *)(((int)p + 3) & mask);
            total = (total + 3) & mask;
            block++;
        } while (block->base != 0);
    }
    total += 8;
    ((int *)p)[1] = 0;
    ((int *)p)[0] = -1;
    out->checksum = memcard_Checksum(out + 1, total);
    out->size = total;
    return total + 8;
}

typedef struct {
    int a; /* 0x00 */
    int b; /* 0x04 */
    int c; /* 0x08 */
    int d; /* 0x0C */
    char name[8]; /* 0x10 */
    int valid; /* 0x18 */
} McEntry; /* 0x1C */
typedef struct {
    char hdr[0x20];
    McEntry e[5];
    char pad[0x14];
} McSlot; /* 0xC0 */
extern McSlot memory_card_state_s[] __asm__("D_0013D390");
extern int memcard_TestChecksum(char *); /* memcard_TestChecksum */

/* memcard_RestoreInfo(char *, int, int). Advancing the buf parameter
   itself and copying the name with memcpy both matter for retail's
   registers; re-indexing the entry per store keeps its daddu copies. */
void memcard_RestoreInfo(char *buf, int slot, int idx) {
    memory_card_state_s[slot].e[idx].valid = memcard_TestChecksum(buf) == 0;
    buf += 0x10;
    memory_card_state_s[slot].e[idx].a = *(int *)buf;
    buf += 0xC;
    memory_card_state_s[slot].e[idx].b = *(int *)buf;
    buf += 0xC;
    memory_card_state_s[slot].e[idx].c = *(int *)buf;
    buf += 0xC;
    memory_card_state_s[slot].e[idx].d = *(int *)buf;
    buf += 0xC;
    memcpy(memory_card_state_s[slot].e[idx].name, buf, 8);
}

struct RestoreEntry {
    unsigned char *data;
    int size;
    int id;
    int status;
};
struct RestoreBlock {
    int id;
    int size;
    unsigned char data[1];
};
typedef struct {
    char pad0[0xB4];
    int errors; /* 0xB4 */
    char padB8[8];
} RestoreCardSlot;
typedef struct {
    RestoreCardSlot slot[1];
    char padC0[0xC];
    int cur; /* 0xCC */
} RestoreCardState;
extern RestoreCardState memory_card_state_c __asm__("D_0013D390");
extern int memcard_restore_mismatch_count SDATA(memcard_restore_mismatch_count); /* mismatch count */

/* memcard_RestoreData: unpack the save blocks after buf's 8-byte header
   into the table entries with the same id (slot selects the element),
   marking each entry 1 (same size), -1 (block shorter) or -2 (longer),
   and count blocks with no entry, a wrong total and entries left
   unrestored as errors, stored in the current card slot. Adapted from
   Lombyte's project (GPL-3.0) for PAL. */
int memcard_RestoreData(void *src, int slot, int *table) {
    unsigned char *buf = src;
    struct RestoreEntry *tbl = (struct RestoreEntry *)table;
    struct RestoreEntry *e;
    int errors;
    int total;
    int i;
    int n;
    unsigned char *dst;

    if (memcard_TestChecksum((char *)buf) == 0) {
        return 1;
    }
    buf += 8;
    errors = 0;
    total = 8;
    for (i = 0; tbl[i].data != 0; i++) {
        tbl[i].status = 0;
    }
    while (((struct RestoreBlock *)buf)->id != -1) {
        i = 0;
        if (tbl[0].data == 0) {
            goto missing;
        }
        while (tbl[i].id != ((struct RestoreBlock *)buf)->id) {
            i++;
            if (tbl[i].data == 0) {
                goto missing;
            }
        }
        e = (struct RestoreEntry *)(((unsigned int)i << 4) + (unsigned int)tbl);
        if (e->data != 0) {
            int bsize = ((struct RestoreBlock *)buf)->size;

            dst = e->data + slot * e->size;
            if (bsize == e->size) {
                n = e->size;
                e->status = 1;
            } else if (bsize < e->size) {
                n = bsize;
                e->status = -1;
            } else {
                n = e->size;
                e->status = -2;
            }
            if (libc_memcmp(dst, ((struct RestoreBlock *)buf)->data, n) != 0) {
                memcard_restore_mismatch_count++;
            }
            copy_memory_aligned(dst, ((struct RestoreBlock *)buf)->data, n);
            total += ((n + 3) & ~3) + 8;
        } else {
        missing:
            errors++;
        }
        buf += ((((struct RestoreBlock *)buf)->size + 3) & ~3) + 8;
    }
    total += 8;
    if (total != memcard_GetDataSize((int *)tbl)) {
        errors++;
    }
    buf += 8;
    for (i = 0; tbl[i].data != 0 && tbl[i].id != ((struct RestoreBlock *)buf)->id; i++) {
        if (tbl[i].status <= 0) {
            errors++;
        }
    }
    memory_card_state_c.slot[memory_card_state_c.cur].errors = errors;
    return errors;
}

extern char save_clock_time[] MACRO_ADDR;

extern unsigned char level_visit_state[];
extern int gBolts MACRO_ADDR;
extern int game_clock_ticks MACRO_ADDR;
extern int D_0015EF20 MACRO_ADDR;
extern int mode_freeze_flags MACRO_ADDR;
extern char memcard_game_data_buffer[];
extern char memcard_level_data_buffer[];

/* memcard_Save(slot, flags): starts the save (sce_cd_read_clock/sce_scf_get_local_time_from_rtc
   on save_clock_time, copy_level_object_positions, save_map_coverage on the map_coverage_pages page for
   g_current_level_index). With no valid card slot (memory_card_state+0xC8, its record's
   +0x14) it returns whether `slot` is 0. Otherwise it ORs `slot` into the
   pending mask (+0xFC); unless nothing is pending, a save is running
   (+0xDC >= 3) or a result is waiting (+0xE4 >= 0), it stamps +0xD0 with
   g_current_level_index (temporarily switching g_current_level_index to `flags` and setting that
   level_visit_state byte), fills entry e[+0x14] (four globals and the 8-byte
   name), serialises both tables (memcard_PrepData), restores the byte and
   g_current_level_index, and sets result 0xF. Returns whether the result is 0xF.
   save_clock_time is MACRO_ADDR (retail rebuilds its address at each use), the
   name field is reached from a `q + 0x30` base, and each block reads
   memory_card_state through its own `char *` local (retail keeps only the %hi). */
int memcard_Save(int slot, int flags) {
    unsigned char saved;

    sce_cd_read_clock(save_clock_time);
    sce_scf_get_local_time_from_rtc(save_clock_time);
    copy_level_object_positions();
    save_map_coverage(map_coverage_pages + (g_current_level_index_m << 11));
    {
        char *p = memory_card_state;
        int active = ((struct MemoryCardState *)p)->xC8;
        int mask;

        if (active == -1 || *(int *)(p + active * 0xC0 + 0x14) < 0) {
            return slot == 0;
        }
        mask = *(int *)(p + 0xFC) | slot;
        *(int *)(p + 0xFC) = mask;
        if (mask == 0) {
            goto done;
        }
        if (slot == 0) {
            mode_freeze_flags |= 0x200;
        }
        if (((struct MemoryCardState *)p)->state >= 3) {
            goto done;
        }
        if (((struct MemoryCardState *)p)->err >= 0) {
            goto done;
        }
        ((struct MemoryCardState *)p)->xD0 = g_current_level_index_m;
        saved = 0;
        if (flags >= 0) {
            g_current_level_index_m = flags;
            saved = level_visit_state[flags];
            if (saved == 0) {
                level_visit_state[flags] = 1;
            }
        }
    }
    {
        char *q = memory_card_state;
        char *names = q + 0x30;

        *(int *)(q + *(int *)(q + 0x14) * 0x1C + 0x24) = gBolts;
        *(int *)(q + *(int *)(q + 0x14) * 0x1C + 0x20) = g_current_level_index_m;
        *(int *)(q + *(int *)(q + 0x14) * 0x1C + 0x2C) = game_clock_ticks;
        memcpy(names + *(int *)(q + 0x14) * 0x1C, save_clock_time, 8);
        *(int *)(q + *(int *)(q + 0x14) * 0x1C + 0x28) = D_0015EF20;
        memcard_PrepData(memcard_game_data_buffer, 0, mc_gamedata);
        memcard_PrepData(memcard_level_data_buffer, ((struct MemoryCardState *)q)->xD0, mc_leveldata);
        if (flags >= 0) {
            level_visit_state[g_current_level_index_m] = saved;
            g_current_level_index_m = ((struct MemoryCardState *)q)->xD0;
        }
        if (((struct MemoryCardState *)q)->err < 0) {
            ((struct MemoryCardState *)q)->err = 0xF;
            ((struct MemoryCardState *)q)->errCard = ((struct MemoryCardState *)q)->xC8;
        }
    }
done: {
    struct MemoryCardState *r = (struct MemoryCardState *)memory_card_state;
    return r->err == 0xF;
}
}
__asm__(".section .text\n\tnop\n");
