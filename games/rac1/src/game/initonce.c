#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * initonce.cpp in the original source; text 0x201E10-0x202260.
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

extern char image_end_round_16k[];
extern int current_vu_chain MACRO_ADDR;
extern int level_resource_buffers[];

/*
 * InitMemSlots(void): the memory slot table D_001941C0, from the
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

/* Externs this function needs that initonce.c does not already declare. */
extern void reset_vif1_gif(void);
extern int sceDmaReset(int);
extern void sce_cd_init(int);
extern void sce_cd_disk_ready(int);
extern int sce_sif_reboot_iop(void *);
extern int sce_sif_sync_iop(void);
extern int STUB_printf(); /* debug-log stub: void func_001E9730(int, ...) */
extern void enable_cache(int);
extern void sceSifInitRpc(int);
extern int disable_interrupts(void); /* DIntr */
extern void sce_sif_init_iop_heap(void);
extern void enable_interrupts(void); /* EIntr */
extern int sce_cd_mmode(int);
extern int sce_fs_reset(void);
extern int wad_GetSectors_FiiPv(int lba, int sectors, void *buf);
extern void flush_cache(int);
extern void memcard_get_name(void *arg0);
extern void sceGsResetGraph(int, int, int, int);
extern void InitDma(void);
extern void SetPalMode(void);
extern void VU0_loadMicroProgram(void *);
extern int load_disc_sectors_into_global_buffer(void);
extern void fast_decompress(void *, void *);
extern int LoadIRXModule(void *arg0, void *arg1);
extern void init_pads(void);
extern void vsync_callback(void); /* vsync_callback(int) */
extern int sceGsSyncVCallback(void *arg0);
extern void memcard_Init(void);
extern void VU1_initChain(void);
extern void initialize_gameplay_sound_system(void);
extern int sceGsSetDefLoadImage(void *, short, short, short, short, short, short, short);
extern int sceGsExecLoadImage(void *src, void *dst);
extern void wait_for_graphics_pipeline_idle(int, unsigned short);
extern void LoadDebugFont(void);
extern void initialize_sif_rpc(void);
extern void initialize_alpha_lookup_table(void);
extern int sce_scf_get_language(void);

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
