#include "common.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"
#include "core/gs/vif1_gif_reset.h"
#include "core/cdvd/read.h"
#include "core/gs/dma_and_sync_v.h"
#include "core/startup/permcb.h"
#include "game/render/draw.h"
#include "game/render/vuchain.h"
#include "asm_functions/executable.h"
#include "game/audio/sound.h"
#include "game/system/stub.h"
#include "game/system/memcard.h"
#include "core/cdvd/callbacks.h"
#include "core/gs/def_load_image.h"
#include "core/gs/load_image.h"
#include "game/system/stash.h"
#include "core/mpeg/ipu.h"
#include "core/sif/reset_iop.h"
#include "core/storage/wad.h"
#include "game/system/init.h"
#include "game/system/bloaders.h"

/*
 * initonce.cpp in the original source; text 0x201E10-0x202260.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

extern char image_end_round_16k[];
extern int current_vu_chain MACRO_ADDR;
extern int level_resource_buffers[];

/*
 * InitMemSlots(void): the memory slot table level_resource_buffers, from the
 * 16 KB-aligned end of the image up.
 *
 * The store order is the scheduler's, and it is predictable: before
 * reload, ties in priority go to the insn that frees a register. The
 * source's last store also frees the table base, so it goes first;
 * stores that are the last use of their value follow in source order,
 * and a value stored twice (d, into [5] and [6]) has its first store
 * emitted last. The allocation follows the schedule.
 */
void InitMemSlots(void) {
    int base = (int)image_end_round_16k & 0xFFFFC000;
    int size = current_vu_chain;
    int a = base + size;
    int b = a + size;
    int c = b + 0x64000;
    int d = c + 0x30000;
    level_resource_buffers[0] = base;
    level_resource_buffers[5] = d;
    level_resource_buffers[6] = d;
    level_resource_buffers[8] = 0x7000000;
    level_resource_buffers[9] = 0x7100000;
    level_resource_buffers[1] = base;
    level_resource_buffers[2] = a;
    level_resource_buffers[3] = b;
    level_resource_buffers[4] = c;
    level_resource_buffers[10] = 0x7200000;
}

extern void sceSifInitRpc(int);

extern void sce_sif_init_iop_heap(void);

extern int sce_fs_reset(void);

extern void init_pads(void);

extern char iop_reboot_image_path[]; /* "cdrom0:\\IOPRP243.IMG;1" */
extern char rebooted_iop_message[]; /* "Rebooted IOP\n" */
extern int pause_action_list_mode MACRO_ADDR;
extern int pal_mode MACRO_ADDR;
extern int vu0_micro_program;
extern int disc_table[];
extern char D_1FF8000[];
extern char image_clear_buffer[];
extern char loaded_irx_modules_message[]; /* "loaded irx modules\n" */
extern int g_game_language MACRO_ADDR;
extern char unsupported_lang_message[]; /* "Unsupported language %d\n" */

/* InitOnce(void). Boots the IOP with cdrom0:\IOPRP243.IMG, waits for it,
 * reads the boot-info sector (lba 0x121) into a stack buffer, sets the
 * NTSC/PAL-ish flag from byte 0x33 of that sector, sets up video and the
 * VU/DMA chain, loads a set of overlay segments from a table right below
 * the 16 KB-aligned end of the image, sets up memory slots, loads a
 * palette/font image, and finally maps sceCdGetDiskType's result to the
 * current-language global.
 * Adapted from Lombyte's project (GPL-3.0) for PAL: runtime/startup/init_once.c, init_once. */
void InitOnce(void) {
    unsigned char buf[0x800];
    char *seg;
    int base;
    int *tbl;
    int region;
    int lang;

    reset_vif1_gif();
    sceDmaReset(1);
    sce_cd_init(0);
    sce_cd_disk_ready(0);
    do {
    } while (sce_sif_reboot_iop(iop_reboot_image_path) == 0);
    do {
    } while (sce_sif_sync_iop() == 0);
    STUB_printf(rebooted_iop_message);
    enable_cache(3);
    sceSifInitRpc(0);
    disable_interrupts();
    sce_sif_init_iop_heap();
    enable_interrupts();
    sce_cd_init(0);
    sce_cd_disk_ready(0);
    sce_cd_mmode(2);
    sce_fs_reset();
    wad_GetSectors_FiiPv(0x121, 1, buf);
    flush_cache(0);

    region = buf[0x33] != 0x4E;
    pause_action_list_mode = pal_mode = region;
    memcard_get_name(buf);

    sceGsResetGraph(0, 1, (pal_mode != 0) ? 3 : 2, 0);
    InitDma();
    SetPalMode();
    VU0_loadMicroProgram(&vu0_micro_program);
    load_disc_sectors_into_global_buffer();

    base = (int)image_end_round_16k & 0xFFFFC000;
    tbl = (int *)(base + 0x2C0000);
    seg = D_1FF8000 - (disc_table[0x4B1] << 0xB);
    wad_GetSectors_FiiPv(disc_table[0x4B0], disc_table[0x4B1], seg);
    flush_cache(0);
    fast_decompress(seg, tbl);
    flush_cache(0);
    LoadIRXModule((void *)(tbl[0x1C] + (int)tbl), (void *)(int)tbl[0x1D]);
    LoadIRXModule((void *)(tbl[0x1E] + (int)tbl), (void *)(int)tbl[0x1F]);
    LoadIRXModule((void *)(tbl[0x20] + (int)tbl), (void *)(int)tbl[0x21]);
    LoadIRXModule((void *)(tbl[0x22] + (int)tbl), (void *)(int)tbl[0x23]);
    LoadIRXModule((void *)(tbl[0x24] + (int)tbl), (void *)(int)tbl[0x25]);
    LoadIRXModule((void *)(tbl[0x26] + (int)tbl), (void *)(int)tbl[0x27]);
    LoadIRXModule((void *)(tbl[0x26] + (int)tbl), (void *)(int)tbl[0x27]);
    LoadIRXModule((void *)(tbl[0x2A] + (int)tbl), (void *)(int)tbl[0x2B]);
    LoadIRXModule((void *)(tbl[0x2C] + (int)tbl), (void *)(int)tbl[0x2D]);
    LoadIRXModule((void *)(tbl[0x28] + (int)tbl), (void *)(int)tbl[0x29]);

    STUB_printf(loaded_irx_modules_message);
    sce_cd_disk_ready(0);
    init_pads();
    sceGsSyncVCallback(vsync_callback);
    current_vu_chain = 0x160000;
    InitMemSlots();
    sce_cd_disk_ready(0);
    memcard_Init();
    InitViewContext();
    UpdateViewContext();
    VU1_initChain();
    sce_cd_disk_ready(0);
    initialize_gameplay_sound_system();
    FastMemSet(image_clear_buffer, 0x80808080, 0x100);
    sceGsSetDefLoadImage(buf, 0x3FFB, 1, 0, 0, 0, 8, 8);
    flush_cache(0);
    sceGsExecLoadImage(buf, image_clear_buffer);
    wait_for_graphics_pipeline_idle(0, 0);
    LoadDebugFont();
    *(volatile int *)0x10000810 = 0x82;
    *(volatile int *)0x10000800 = 0;
    initialize_sif_rpc();
    initialize_alpha_lookup_table();

    lang = sce_scf_get_language();
    switch (lang) {
        case 2:
            g_game_language = 2;
            return;
        case 4:
            g_game_language = 3;
            return;
        case 3:
            g_game_language = 4;
            return;
        case 5:
            g_game_language = 5;
            return;
        default:
            STUB_printf(unsupported_lang_message, lang);
        /* fallthrough */
        case 1:
            g_game_language = 0;
            return;
    }
}

LINKER_REMNANT("asm/remnants/text", func_00202258);
