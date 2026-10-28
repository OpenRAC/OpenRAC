#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * init.cpp in the original source; text 0x201D58-0x201E10.
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

extern int sce_sif_alloc_iop_heap(int arg0);
extern int sce_sif_set_dma(void *buf, int flag);
extern int sce_sif_dma_stat(int modid);
extern int sif_load_iop_module_buffer(int a, int b, int c);
extern void sce_sif_free_iop_heap();

/* LoadIRXModule(name, args): starts an IOP module load through an RPC
   helper, hands func_00118E20 a 4-word tagged buffer, polls the returned
   id until it is ready, then starts the module and reports the result.
   The nops before the poll loop's branch are the short-loop padding
   (tools/fix_short_loops.py); the result test comes before the
   func_0011CCB0 call, where retail's movz sits in its delay slot. */
int LoadIRXModule(void *arg0, void *arg1) {
    int buf[4];
    int result;
    int modid;
    int status;
    int ret = 1;

    result = sce_sif_alloc_iop_heap((int)arg1);
    buf[0] = (int)arg0;
    buf[1] = result;
    buf[2] = (int)arg1;
    buf[3] = 0;
    modid = sce_sif_set_dma(buf, 1);
    if (modid != 0) {
        while (sce_sif_dma_stat(modid) >= 0) {
        }
        status = sif_load_iop_module_buffer(result, 0, 0);
        if (status < 0) {
            ret = 0;
        }
        sce_sif_free_iop_heap(result);
    }
    return ret;
}
