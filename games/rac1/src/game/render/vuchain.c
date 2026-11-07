#include "common.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"
#include "asm_functions/executable.h"
#include "data/executable.h"
#include "game/render/draw.h"
#include "game/system/stub.h"
#include "core/gs/dma_and_sync_v.h"

/*
 * vuchain.cpp in the original source; text 0x2347F0-0x2352C8.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

extern void VU1_addGSregister(int, long);

#define VIF0_STAT ((volatile unsigned int *)0x10008000)
#define VIF0_FBRST ((volatile unsigned int *)0x10008020)
#define VIF0_BASE ((volatile unsigned int *)0x10008030)

/* VU0_loadMicroProgram(long *): waits for VIF0 to go idle, resets it,
 * points it at the (uncached-masked) micro program address, kicks off
 * MSCALL 0x45 by writing the code word directly, then waits for VIF0 to
 * go idle again. */
void VU0_loadMicroProgram(long *prog) {
    while (*VIF0_STAT & 0x100) {
        spin_wait(0x10);
    }
    *VIF0_FBRST = 0;
    *VIF0_BASE = (unsigned int)prog & 0x0FFFFFFF;
    *VIF0_STAT = 0x145;
    while (*VIF0_STAT & 0x100) {
        spin_wait(0x10);
    }
}

extern int g_current_level_index MACRO_ADDR;
extern int current_vu_chain MACRO_ADDR;
extern int vu_chain_table[];

/* Advances the index at current_vu_chain, wrapping at 0x13. The old note had
   the condition backwards: `slti; movz x,$0,t` is `if (idx >= 0x13)
   idx = 0`. Both globals are MACRO_ADDR, which gives the one-register
   load and the $gp store in the jr slot. */
void select_level_vu_chain(void) {
    int idx = g_current_level_index;
    if (idx >= 0x13) {
        idx = 0;
    }
    current_vu_chain = vu_chain_table[idx];
}

/*
 * The VU1 chain state. g_render_packet_cursor (the write pointer), vu1_chain_buffer_index (the
 * buffer index) and the two limits render_packet_end/vu1_chain_safe_end are all reached
 * with retail's one-instruction macro form, so they are MACRO_ADDR: the
 * assembler expands them through $at, or through $gp where the access
 * lands in a delay slot.
 */
extern int *g_render_packet_cursor MACRO_ADDR;
extern int previous_render_packet_cursor MACRO_ADDR;

extern int vu1_chain_buffer_index MACRO_ADDR;

extern int render_packet_end MACRO_ADDR;
extern int vu1_chain_safe_end MACRO_ADDR;
extern int vu1_chain_buffers[2];
extern int level_resource_buffers[];

extern int vu1_chain_reserved_bytes MACRO_ADDR;

/* `end` reads g_render_packet_cursor back right after storing it (CSE folds the
   load away), which changes how the stores are scheduled: that, not
   any order of the four stores, gives retail's. */
void VU1_initChain(void) {
    int base = level_resource_buffers[1];
    int end;

    vu1_chain_buffers[0] = base;
    vu1_chain_buffers[1] = level_resource_buffers[2];
    vu1_chain_buffer_index = 0;
    g_render_packet_cursor = (int *)base;
    end = (int)g_render_packet_cursor + current_vu_chain - vu1_chain_reserved_bytes;
    render_packet_end = end;
    vu1_chain_safe_end = end - 0x2000;
}

/* VU1_swapChain(void) */
void VU1_swapChain(void) {
    int idx = 1 - vu1_chain_buffer_index;
    int base = vu1_chain_buffers[idx];
    int end;

    previous_render_packet_cursor = (int)g_render_packet_cursor;
    vu1_chain_buffer_index = idx;
    g_render_packet_cursor = (int *)base;
    end = (int)g_render_packet_cursor + current_vu_chain - vu1_chain_reserved_bytes;
    render_packet_end = end;
    vu1_chain_safe_end = end - 0x2000;
}

extern int vu1_chain_peak_bytes MACRO_ADDR;
extern volatile int vif_chain_flags SDATA(vif_chain_flags);
extern char vu1_chain_overflow_message[];

/* VU1_sendChain. vif_chain_flags is volatile, so its store stays out of the
   delay slot (reorg never moves a volatile access there). */
void VU1_sendChain(void) {
    int size;
    int err;
    int *chan;

    /* KEPT: volatile: the store stays out of the delay slot. */
    vif_chain_flags |= 0x1F;
    size = (int)g_render_packet_cursor - vu1_chain_buffers[vu1_chain_buffer_index];
    err = 0;
    if (vu1_chain_peak_bytes < size) {
        vu1_chain_peak_bytes = size;
        if (current_vu_chain < size) {
            STUB_printf(vu1_chain_overflow_message);
            err = 1;
        }
    }
    if (err == 0) {
        g_render_packet_cursor[0] = 0x70000000;
        g_render_packet_cursor[1] = 0;
        g_render_packet_cursor[2] = 0;
        g_render_packet_cursor[3] = 0;
        chan = sce_dma_get_chan(1);
        *chan |= 0xC0;
        flush_cache(0);
        sce_dma_send(chan, vu1_chain_buffers[vu1_chain_buffer_index]);
    } else {
        /* KEPT: volatile, as above. */
        vif_chain_flags = 0;
    }
}

extern volatile int vif_chain_flags SDATA(vif_chain_flags);
extern char vu1_chain_sync_timeout_message[];

/* VU1_syncChain(int) */
void VU1_syncChain(int mask) {
    int i = 0;
    while ((*(int *)&vif_chain_flags & mask) != 0) {
        spin_wait(0x400);
        if (i > 100000) {
            STUB_printf(vu1_chain_sync_timeout_message);
            reset_graphics();
            break;
        }
        i++;
    }
}

/*
 * VU1_addDataRef(void *, int). Retail reloads g_render_packet_cursor before every
 * field write -- the writes could alias the pointer itself -- which is
 * exactly what writing through the global (rather than through a local
 * copy) produces.
 */
void VU1_addDataRef(void *data, int qwc) {
    g_render_packet_cursor[0] = qwc | 0x30000000;
    g_render_packet_cursor[1] = (int)data;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0;
    g_render_packet_cursor += 4;
}

LINKER_REMNANT("asm/remnants/text", func_00234B98);

/* Append a DMA packet that unpacks qwc quadwords from src to VU1 memory address arg0. */
void write_vif_unpack_packet(int arg0, void *src, int qwc) {
    g_render_packet_cursor[0] = qwc | 0x10000000;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0x1000404;
    g_render_packet_cursor[3] = arg0 | (qwc << 16) | 0x6C000000;
    g_render_packet_cursor += 4;
    FastMemCopy(g_render_packet_cursor, src, qwc * 16);
    g_render_packet_cursor += qwc * 4;
}

LINKER_REMNANT("asm/remnants/text", func_00234C48);

/* Append an empty DMA cnt tag carrying the VIF code arg0. */
void vu1_add_vif_code(int arg0) {
    g_render_packet_cursor[0] = 0x10000000;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = arg0;
    g_render_packet_cursor += 4;
}

/*
 * VU1_addGSregister(unsigned int, unsigned long). Real name per the
 * Lombyte's project NTSC project (github.com/mateuszklysz/Lombyte's project); their NTSC
 * build's version, VU1_addGSregister__FUiUlb, takes a third `bool` arg
 * and is 4 bytes larger (0xb4 vs this PAL build's 176 bytes) -- PAL
 * evidently dropped the bool, confirmed by every real call site in this
 * codebase using only 2 args.
 */
void VU1_addGSregister(int arg0, long arg1) {
    g_render_packet_cursor[0] = 0x10000002;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000002;
    g_render_packet_cursor[4] = 0x8001;
    g_render_packet_cursor[5] = 0x10000000;
    g_render_packet_cursor[6] = 0xE;
    g_render_packet_cursor[7] = 0;
    *(long *)((char *)g_render_packet_cursor + 0x20) = arg1;
    g_render_packet_cursor[10] = arg0;
    g_render_packet_cursor[11] = 0;
    g_render_packet_cursor += 0xC;
}

LINKER_REMNANT("asm/remnants/text", func_00234D50);

extern int g_screen_extent[];

/* VU1_setScissor(x0, x1, y0, y1): clamps the rectangle to the screen
   (g_screen_extent[0]/[1] are its width and height) and queues it as GS
   register 0x40 (SCISSOR_1: x0 | x1 << 16 | y0 << 32 | y1 << 48) in the
   same packet VU1_addGSregister builds. Each clamp is a
   `cond ? bound : arg` select assigned back to the argument, and the
   fields are sign-extended to 64 bits, not zero-extended. */
/* VU1_setScissor(x0, x1, y0, y1): clamps the rectangle to the screen
   (g_screen_extent[0]/[1] are its width and height) and queues it as GS
   register 0x40 (SCISSOR_1: x0 | x1 << 16 | y0 << 32 | y1 << 48) in the
   same packet VU1_addGSregister builds. */
void VU1_setScissor(int x0, int x1, int y0, int y1) {
    x0 = x0 < 0 ? 0 : x0;
    x1 = x1 > g_screen_extent[0] - 1 ? g_screen_extent[0] - 1 : x1;
    y0 = y0 < 0 ? 0 : y0;
    y1 = y1 > g_screen_extent[1] - 1 ? g_screen_extent[1] - 1 : y1;
    g_render_packet_cursor[0] = 0x10000002;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000002;
    g_render_packet_cursor[4] = 0x8001;
    g_render_packet_cursor[5] = 0x10000000;
    g_render_packet_cursor[6] = 0xE;
    g_render_packet_cursor[7] = 0;
    *(long *)(g_render_packet_cursor + 8) = (long)x0 | ((long)x1 << 16) | ((long)y0 << 32) | ((long)y1 << 48);
    g_render_packet_cursor[10] = 0x40;
    g_render_packet_cursor[11] = 0;
    g_render_packet_cursor += 0xC;
}

__asm__(".section .text\n\tnop\n");

LINKER_REMNANT("asm/remnants/text", func_00234E78);

extern char vu1_gs_regs_tex_flush[];

/* VU1_texFlush(void) */
void VU1_texFlush(void) {
    g_render_packet_cursor[0] = 0x30000003;
    g_render_packet_cursor[1] = (int)vu1_gs_regs_tex_flush;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000003;
    g_render_packet_cursor += 4;
}

extern char normal_gs_regs_dma_payload[];

/* VU1_gsRegsNormal(void) */
void VU1_gsRegsNormal(void) {
    g_render_packet_cursor[0] = 0x30000003;
    g_render_packet_cursor[1] = (int)normal_gs_regs_dma_payload;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000003;
    g_render_packet_cursor += 4;
}

extern char vu1_gs_regs_alt_dma_source[];

/* Append a DMA ref tag that sends the alt GS register packet (3 qwords) via DIRECT. */
void vu1_gs_regs_alt(void) {
    g_render_packet_cursor[0] = 0x30000003;
    g_render_packet_cursor[1] = (int)vu1_gs_regs_alt_dma_source;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000003;
    g_render_packet_cursor += 4;
}

LINKER_REMNANT("asm/remnants/text", func_00234FA0);

/* Append a DMA ref tag that sends the draw environment (11 qwords) via DIRECT. */
void vu1_gs_regs_font(void) {
    g_render_packet_cursor[0] = 0x3000000B;
    g_render_packet_cursor[1] = (int)draw_environment;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x5000000B;
    g_render_packet_cursor += 4;
}

LINKER_REMNANT("asm/remnants/text", func_00235008);

extern int vif1_dma_handler_id MACRO_ADDR;
extern int vu1_halt_handler_id MACRO_ADDR;

extern void enable_dmac_atomic(int);

extern void print_register_values_and_halt(void);

/*
 * DMAC_VIF1_Enable(void): unless the handlers are installed, set CIM1 in
 * D_STAT and install vif1_dma_interrupt_handler (channel 1) and print_register_values_and_halt (0xF),
 * then enable channel 1. The D_STAT read is volatile but the store is
 * plain: a volatile store stops reorg's try_merge_delay_insns at it and
 * leaves a second copy of the handler's lui on the fall-through path.
 */
void DMAC_VIF1_Enable(void) {
    if (vif1_dma_handler_id == 0 && vu1_halt_handler_id == 0) {
        if ((*(volatile int *)0x1000E010 & 0x20000) == 0) {
            *(int *)0x1000E010 = 0x20000;
        }
        vif1_dma_handler_id = add_dmac_handler(1, (void *)vif1_dma_interrupt_handler, (void *)0);
        vu1_halt_handler_id = add_dmac_handler(0xF, (void *)print_register_values_and_halt, (void *)0);
        enable_dmac_atomic(1);
    }
}

/*
 * DMAC_VIF1_Disable(void). The two handler ids are MACRO_ADDR: the copy
 * scheduled into the branch delay slot is $gp-relative, the copy on the
 * taken path is the lui/%lo expansion of the same macro. The volatile
 * D_STAT write is what forces the reload on that path.
 */

/* Stop VIF1 DMA, remove the VIF1 DMA and VU1 halt interrupt handlers. */
void DMAC_VIF1_Disable(void) {
    if ((*(volatile int *)0x1000E010 & 0x20000) != 0) {
        *(volatile int *)0x1000E010 = 0x20000;
    }
    remove_dmac_handler(1, vif1_dma_handler_id);
    remove_dmac_handler(0xF, vu1_halt_handler_id);
    disable_dmac_atomic(1);
    vif1_dma_handler_id = 0;
    vu1_halt_handler_id = 0;
}

ASM_FUNC("asm/handwritten/text", func_00235118);

extern char dmac_bus_error_message[];
extern char dma1_register_values_format[];
extern char dma_halt_question_message[];

/* Print the VIF1 DMA register values and hang (VU1 timeout handler). */
void print_register_values_and_halt(void) {
    int a = *(volatile int *)0x10009000;
    int b = *(volatile int *)0x10009030;

    STUB_printf(dmac_bus_error_message);
    STUB_printf(dma1_register_values_format, a, b);
    STUB_printf(dma_halt_question_message);
    while (1) {
        ;
    }
}

/*
 * Append one quadword to the packet at g_render_packet_cursor. This one wants the
 * pointer typed as a quadword: stepping an `int *` by 4 folds the copy
 * retail keeps (`daddu $5,$2,$0`) into the increment and comes out one
 * instruction short, so the same symbol is declared a second time under
 * a quadword type.
 */
typedef struct {
    int w[4];
} Qword;
extern Qword *g_render_packet_cursor_q __asm__("D_00161000") MACRO_ADDR;

/* Append an interrupting DMA tag with qword count arg0 to the second packet list. */
void append_dma_tag(int arg0) {
    Qword *p = g_render_packet_cursor_q++;
    p->w[0] = arg0 + 0x90000000;
    p->w[1] = 0;
    p->w[2] = 0;
    p->w[3] = 0;
}
