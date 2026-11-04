#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "core/cdvd/read.h"
#include "core/gs/put_draw_env.h"
#include "core/cdvd/callbacks.h"

/*
 * wad.cpp (0x12F348-0x12F580), the last object in core_text. Name from
 * the NTSC split.
 */

extern unsigned char cd_read_spindle_speed[4] MACRO_ADDR;

/* wad_GetSectors_FiiPv(lba, sectors, buf): sceCdRead with the usual
 * request/poll/retry shape (see load_disc_sectors_into_global_buffer/read_file_entry_with_retry below),
 * returning the byte count read (sectors << 11 = sectors * 0x800). */
int wad_GetSectors_FiiPv(int lba, int sectors, void *buf) {
    char hdr[4];

    hdr[0] = 0x20;
    hdr[1] = cd_read_spindle_speed[0];
    hdr[2] = 0;
    hdr[3] = 0;

    do {
        sce_cd_read(lba, sectors, buf, &hdr);
        while (sce_cd_sync_s(1) != 0) {
            sce_gs_sync_v(0);
        }
    } while (sce_cd_get_error() != 0);

    return sectors << 11;
}

typedef struct {
    char b[4];
} StreamHdr;
extern int disc_table[];

/* Same request/poll/retry shape as wad_GetSectors_FiiPv above, reading 6 fixed
   sectors into a stack buffer and copying 0x2960 bytes of it into
   disc_table. The nop after `jal sce_cd_sync_s` in the poll loop is
   ps2eeas's short-loop padding (tools/ps2eeas_nops.py). */
int load_disc_sectors_into_global_buffer(void) {
    char buf[0x3000];
    StreamHdr hdr;
    unsigned int i;

    hdr.b[0] = 0x20;
    hdr.b[1] = cd_read_spindle_speed[0];
    hdr.b[2] = 0;
    hdr.b[3] = 0;

    do {
        sce_cd_read(0x5DC, 6, buf, &hdr);
        while (sce_cd_sync_s(1) != 0) {
            sce_gs_sync_v(0);
        }
    } while (sce_cd_get_error() != 0);

    for (i = 0; i < 0x2960; i++) {
        ((char *)disc_table)[i] = buf[i];
    }
    return 1;
}

extern int disc_table_level_entries[];
extern int pad_globals_base[];

/* As load_disc_sectors_into_global_buffer but the sector number comes from an 8-byte-stride
   table indexed by arg0 and re-read on every retry, and 0x2434 bytes are
   copied into pad_globals_base. The entry address is formed after the header
   stores, which orders the two constant-address computations as retail
   does. */
int read_file_entry_with_retry(int arg0) {
    int *entry;
    char buf[0x2800];
    StreamHdr hdr;
    unsigned int i;

    hdr.b[0] = 0x20;
    hdr.b[1] = cd_read_spindle_speed[0];
    hdr.b[2] = 0;
    hdr.b[3] = 0;
    entry = &disc_table_level_entries[arg0 * 2];

    do {
        sce_cd_read(*entry, 5, buf, &hdr);
        while (sce_cd_sync_s(1) != 0) {
            sce_gs_sync_v(0);
        }
    } while (sce_cd_get_error() != 0);

    for (i = 0; i < 0x2434; i++) {
        ((char *)pad_globals_base)[i] = buf[i];
    }
    return 1;
}
