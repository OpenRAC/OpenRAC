#include "common.h"
#include "structs/misc.h"
#include "core/cdvd/callbacks.h"
#include "core/cdvd/read.h"

/*
 * movie/strfile.cpp in the original source; text 0x23CE18-0x23CEC8.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/* StrFile: only the fields these functions touch are known. */
typedef struct StrFile {
    int second;
    int first;
} StrFile;

/* No recovered name. Stores its two arguments in the object (first argument at offset 4)
 * and returns 1. */
int mpeg_cd_stream_open(StrFile *f, int first, int second) {
    f->first = first;
    f->second = second;
    return 1;
}
/* No recovered name. Nothing to free, returns 1. */
int mpeg_cd_stream_close(StrFile *f) {
    return 1;
}

/* Reads movie sectors and advances the cursor after a synchronous read. */
int read_cd_stream_sectors(void *arg0, int arg1, int arg2, int arg3) {
    struct {
        unsigned char retries, speed, pattern, padding;
    } mode;
    int r = 0;
    int n = arg2 >> 11;

    mode.retries = 0x64;
    mode.speed = 0;
    mode.pattern = 0;
    sce_cd_read(*(int *)((char *)arg0 + 4), n, arg1, &mode);
    if (arg3 == 0) {
        *(int *)((char *)arg0 + 4) = *(int *)((char *)arg0 + 4) + n;
        sce_cd_sync_s(0);
        r = arg2;
    }
    return r;
}
