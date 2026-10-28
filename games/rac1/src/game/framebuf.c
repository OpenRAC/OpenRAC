#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * framebuf.cpp in the original source; text 0x1FAA28-0x1FBC80.
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

ASM_FUNC("asm/handwritten/text", func_001FAA28);

LINKER_REMNANT("asm/remnants/text", func_001FAB20);

typedef unsigned int u128_FAB40 __attribute__((mode(TI)));

typedef struct {
    unsigned long NLOOP:15;
    unsigned long EOP:1;
    unsigned long pad16:16;
    unsigned long id:14;
    unsigned long PRE:1;
    unsigned long PRIM:11;
    unsigned long FLG:2;
    unsigned long NREG:4;
    unsigned long REGS0:4;
    unsigned long REGS1:4;
    unsigned long REGS2:4;
    unsigned long REGS3:4;
    unsigned long REGS4:4;
    unsigned long REGS5:4;
    unsigned long REGS6:4;
    unsigned long REGS7:4;
    unsigned long REGS8:4;
    unsigned long REGS9:4;
    unsigned long REGS10:4;
    unsigned long REGS11:4;
    unsigned long REGS12:4;
    unsigned long REGS13:4;
    unsigned long REGS14:4;
    unsigned long REGS15:4;
} GifTag_FAB40;

typedef struct {
    unsigned long FBP:9;
    unsigned long rest:55;
} GsFbp_FAB40;

typedef struct {
    unsigned long pmode;
    unsigned long smode2;
    GsFbp_FAB40 dispfb;
    unsigned long display;
    unsigned long bgcolor;
    unsigned long pad028;
    GifTag_FAB40 giftag0;       /* 0x030 */
    GsFbp_FAB40 frame0;         /* 0x040 */
    unsigned long draw0[15];
    GifTag_FAB40 giftag1;       /* 0x0C0 */
    GsFbp_FAB40 frame1;         /* 0x0D0 */
    unsigned long draw1[15];
    short display_width;        /* 0x150 */
    short display_height;
    short psm;
    short fbp0;
    short storage_width;        /* 0x158 */
    short storage_height;
    short storage_psm;
    short fbp1;
    short target_width;         /* 0x160 */
    short target_height;
    short target_psm;
    short target_fbp;
    short display_offset_x;     /* 0x168 */
    short display_offset_y;
    short zpsm;
    short zbp;
    int reserved170;            /* 0x170 */
} FsAa_FAB40;

extern FsAa_FAB40 fs_aa_buffer;
extern void *active_fs_aa_buffer MACRO_ADDR;
extern int display_buffer_address MACRO_ADDR;
extern int draw_buffer_address MACRO_ADDR;
extern int depth_buffer_address MACRO_ADDR;
extern char first_clear_packet[];
extern char second_clear_packet[];
extern char fs_aa_draw_packet[];
extern char fs_aa_transfer_packet[];
extern char fs_aa_resample_packet[];
extern char fs_aa_clear_packet[];
extern void sceGsDefDispEnv(void *, short, short, short, short, short);
extern int sce_gs_set_def_draw_env(void *, short, short, short, short, short);

#define FA ((FsAa_FAB40 *)D_0015EFB8)

void SetupFS_AA_buffer(int display_width, int display_height, int storage_width, int storage_height,
                   int display_offset_x, int display_offset_y) {
    unsigned long *q;

    active_fs_aa_buffer = &fs_aa_buffer;
    fs_aa_buffer.display_width = display_width;
    fs_aa_buffer.display_height = display_height;
    fs_aa_buffer.storage_width = storage_width;
    fs_aa_buffer.storage_height = storage_height;
    fs_aa_buffer.display_offset_x = display_offset_x;
    fs_aa_buffer.display_offset_y = display_offset_y;
    fs_aa_buffer.reserved170 = 0;
    fs_aa_buffer.storage_psm = 0;
    fs_aa_buffer.psm = 0;
    fs_aa_buffer.target_psm = 0;
    fs_aa_buffer.zpsm = 0x31;
    fs_aa_buffer.fbp1 = display_buffer_address >> 13;
    fs_aa_buffer.fbp0 = draw_buffer_address >> 13;
    fs_aa_buffer.zbp = depth_buffer_address >> 13;
    sceGsDefDispEnv(&fs_aa_buffer, 0, storage_width, storage_height, display_offset_x,
                  display_offset_y);
    FA->dispfb.FBP = fs_aa_buffer.fbp1;
    sce_gs_set_def_draw_env(&FA->frame0, fs_aa_buffer.psm, fs_aa_buffer.display_width,
                  fs_aa_buffer.display_height, 3, fs_aa_buffer.zpsm);
    FA->frame0.FBP = fs_aa_buffer.fbp0;
    *(unsigned long *)&FA->draw0[1] =
        (long)fs_aa_buffer.zbp | ((unsigned long)(fs_aa_buffer.zpsm & 0xF) << 24);
    *(u128_FAB40 *)&FA->giftag0 = 0;
    FA->giftag0.NLOOP = 8;
    FA->giftag0.EOP = 1;
    FA->giftag0.NREG = 1;
    FA->giftag0.REGS0 = 0xE;
    sce_gs_set_def_draw_env(&FA->frame1, fs_aa_buffer.storage_psm, fs_aa_buffer.storage_width,
                  fs_aa_buffer.storage_height, 0, 0);
    FA->frame1.FBP = fs_aa_buffer.fbp1;
    *(unsigned long *)&FA->draw1[1] = 1UL << 32;
    *(u128_FAB40 *)&FA->giftag1 = 0;
    FA->giftag1.NLOOP = 8;
    FA->giftag1.EOP = 1;
    FA->giftag1.NREG = 1;
    FA->giftag1.REGS0 = 0xE;

    q = (unsigned long *)fs_aa_transfer_packet;
    q[0] = 0x408B400000000001UL;
    q[1] = 0xEEEE;
    q[2] = 0x30000;
    q[3] = 0x47;
    q[4] = 5;
    q[5] = 8;
    q[6] = 0x100000261UL;
    q[7] = 0x14;
    q[8] = ((long)FA->fbp0 << 5) | ((unsigned long)((FA->display_width >> 6) & 0x3F) << 14) |
           ((long)FA->psm << 20) | 0xEA8000000UL;
    q[9] = 6;
    q[10] = 0x4400000000008010UL;
    q[11] = 0x5353;
    {
        unsigned long *p;
        int i;
        for (i = 0, p = q + 12; i < 16; i++) {
            *p++ = i * FA->display_width;
            *p++ = (i * FA->storage_width + (0x8000 - FA->storage_width * 8)) |
                   ((long)(0x7FF8 - FA->storage_height * 8) << 16);
            *p++ = (i + 1) * FA->display_width | ((long)FA->display_height << 20);
            *p++ = ((i + 1) * FA->storage_width + (0x8000 - FA->storage_width * 8)) |
                   ((long)(FA->storage_height * 8 + 0x7FF8) << 16);
        }
    }
    q[76] = 0x4400000000008001UL;
    q[77] = 0x4410;
    q[78] = 0x181;
    q[79] = 0x80000000UL;
    q[80] = 0x6FF8 | ((long)(0x7FF8 - FA->storage_height * 8) << 16);
    q[81] = 0x6FF8 | ((long)(FA->storage_height * 8 + 0x7FF8) << 16);

    q = (unsigned long *)fs_aa_resample_packet;
    q[0] = 0x308B400000000001UL;
    q[1] = 0xEEE;
    q[2] = 0x30000;
    q[3] = 0x47;
    q[4] = 0x100000261UL;
    q[5] = 0x14;
    q[6] = ((long)FA->fbp0 << 5) | ((unsigned long)((FA->display_width >> 6) & 0x3F) << 14) |
           ((long)FA->psm << 20) | 0xEA8000000UL;
    q[7] = 6;
    q[8] = 0x4400000000008010UL;
    q[9] = 0x5353;
    {
        unsigned long *p = q + 10;
        int i;
        for (i = 0; i < 16; i++) {
            *p++ = i * FA->display_width;
            *p++ = (i * 0x200 + 0x6FF8) | ((long)(0x7FF8 - FA->storage_height * 8) << 16);
            *p++ = (i + 1) * FA->display_width | 0x1A000000;
            *p++ = (i * 0x200 + 0x71F8) | ((long)(FA->storage_height * 8 + 0x7FF8) << 16);
        }
    }

    q = (unsigned long *)fs_aa_draw_packet;
    q[0] = 0x408B400000000001UL;
    q[1] = 0xEEEE;
    q[2] = 0x30000;
    q[3] = 0x47;
    q[4] = 5;
    q[5] = 8;
    q[6] = 0x100000261UL;
    q[7] = 0x14;
    q[8] = ((long)FA->fbp0 << 5) | ((unsigned long)((FA->display_width >> 6) & 0x3F) << 14) |
           ((long)FA->psm << 20) | 0xEA8000000UL;
    q[9] = 6;
    q[10] = 0x4400000000008010UL;
    q[11] = 0x5353;
    {
        unsigned long *p = q + 12;
        int i;
        for (i = 0; i < 16; i++) {
            *p++ = i * 0x200;
            *p++ = (i * 0x200 + 0x7000) | ((long)(0x8000 - FA->display_height * 8) << 16);
            *p++ = (i * 0x200 + 0x200) | ((long)FA->display_height << 20);
            *p++ = (i * 0x200 + 0x7200) | ((long)(FA->display_height * 8 + 0x7FF0) << 16);
        }
    }

    q = (unsigned long *)fs_aa_clear_packet;
    q[0] = 0x1000000000000001UL;
    q[1] = 0xE;
    q[2] = 0x30000;
    q[3] = 0x47;
    q[4] = 0x2400000000008001UL;
    q[5] = 0x10;
    q[6] = 0x106;
    q[7] = 0x80008000UL;
    q[8] = 0x2400000000008010UL;
    q[9] = 0x44;
    {
        unsigned long *p = q + 10;
        int i;
        for (i = 0; i < 16; i++) {
            *p++ = (i * 0x200 + 0x6FF8) | ((long)(0x7FF8 - FA->display_height * 8) << 16);
            *p++ = (i * 0x200 + 0x71F8) | ((long)(FA->display_height * 8 + 0x7FF8) << 16);
        }
    }

    q = (unsigned long *)first_clear_packet;
    {
        unsigned long *p = q + 8;
        int i;
        for (i = 0; i < 16; i++) {
            *p++ = (i * 0x200 + 0x6FF8) | ((long)(0x7FF8 - FA->display_height * 8) << 16);
            *p++ = (i * 0x200 + 0x71F8) | ((long)(FA->display_height * 8 + 0x7FF8) << 16);
        }
    }

    q = (unsigned long *)second_clear_packet;
    {
        unsigned long *p = q + 8;
        int i;
        for (i = 0; i < 16; i++) {
            *p++ = (i * 0x200 + 0x6FF8) | ((long)(0x7FF8 - FA->storage_height * 8) << 16);
            *p++ = (i * 0x200 + 0x71F8) | ((long)(FA->storage_height * 8 + 0x7FF8) << 16);
        }
    }
}

extern long background_rgbaq NOT_SDA;

/* Set the background clear colour from 8-bit red, green and blue. */
void SetBackgroundColor(int arg0, int arg1, int arg2) {
    background_rgbaq = (long)arg0 | ((long)arg1 << 8) | ((long)arg2 << 16) | 0x80000000L;
}

extern void *active_fs_aa_buffer MACRO_ADDR;
extern void sce_gs_put_disp_env(void *);

/* PutDispBuffer(void) */
void PutDispBuffer(void) {
    sce_gs_put_disp_env(active_fs_aa_buffer);
}

extern int *g_render_packet_cursor MACRO_ADDR;
extern void sceGsPutDrawEnv(void *);

/* PutDrawBufferLarge: queue the draw environment at D_0015EFB8+0x30 as a
   DMA ref tag when a packet buffer is open, else send it directly. */
void PutDrawBufferLarge(void) {
    if (g_render_packet_cursor != 0) {
        g_render_packet_cursor[0] = 0x30000009;
        g_render_packet_cursor[1] = ((int)active_fs_aa_buffer + 0x30) & 0xFFFFFFF;
        g_render_packet_cursor[2] = 0;
        g_render_packet_cursor[3] = 0x50000009;
        g_render_packet_cursor += 4;
    } else {
        sceGsPutDrawEnv((char *)active_fs_aa_buffer + 0x30);
    }
}

extern char fs_aa_clear_packet[];

/* Appends a DMA tag pair (0x30000015 / &D_00152140 / 0 / 0x50000015) to
   the packet buffer D_00161000 points at, if there is one. */
void append_gif_transfer_packet(void) {
    if (g_render_packet_cursor != 0) {
        g_render_packet_cursor[0] = 0x30000015;
        g_render_packet_cursor[1] = (int)fs_aa_clear_packet;
        g_render_packet_cursor[2] = 0;
        g_render_packet_cursor[3] = 0x50000015;
        g_render_packet_cursor += 4;
    }
}

/* PutDrawBufferSmall(void) */
void PutDrawBufferSmall(void) {
    g_render_packet_cursor[0] = 0x30000009;
    g_render_packet_cursor[1] = ((int)active_fs_aa_buffer + 0xC0) & 0xFFFFFFF;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000009;
    g_render_packet_cursor += 4;
}

typedef struct {
    char unk_000[0x160];
    short w;        /* 0x160 */
    short h;        /* 0x162 */
    short psm;      /* 0x164 */
    short fbp;      /* 0x166 */
    short unk_168[2];
    short zpsm;     /* 0x16C */
    short zbp;      /* 0x16E */
} FrameCfg;
extern FrameCfg fs_aa_buffer_f __asm__("D_00151880");
extern long capture_texture_tex0 MACRO_ADDR;
/* sceGsSetDefDrawEnv */
extern int sce_gs_set_def_draw_env(void *, short, short, short, short, short);

typedef struct {
    unsigned long FBP:9;
    unsigned long pad09:7;
    unsigned long FBW:6;
    unsigned long pad22:2;
    unsigned long PSM:6;
    unsigned long pad30:2;
    unsigned long FBMSK:32;
} GsFrame; /* sceGsFrame */

/* Sets up a (1 << a) x (1 << b) render target at GS address c (see
   func_0023A948): records TEX0 for it in D_0015EFD0 (buffer width
   1 << max(a - 6, 1) pages, PSMCT32, TCC), its size and FBP (c >> 13) in
   D_00151880, then appends a packet to D_00161000: a default draw
   environment (sceGsSetDefDrawEnv with ztest 3) patched with the FBP and
   the configured Z buffer, followed by TEST_1 0x30003 and a black sprite
   over the whole target. FBP is set through the sceGsFrame bitfield, the
   form that sign-extends the halfword before masking as retail does. */
void setup_render_target(int a, int b, int c) {
    int x;
    long *q;
    unsigned long *d;
    long *r;

    x = a - 6;
    if (x <= 0) {
        x = 1;
    }
    fs_aa_buffer_f.fbp = c >> 13;
    fs_aa_buffer_f.w = 1 << a;
    fs_aa_buffer_f.h = 1 << b;
    capture_texture_tex0 = (unsigned long)(c >> 8) | ((unsigned long)(1 << x) << 14)
               | ((unsigned long)0 << 20) | ((unsigned long)a << 26)
               | ((unsigned long)b << 30) | ((unsigned long)1 << 34);
    FastMemZero16(g_render_packet_cursor, 0xF0);
    g_render_packet_cursor[0] = 0x1000000E;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x5000000E;
    g_render_packet_cursor += 4;
    q = (long *)g_render_packet_cursor;
    q[0] = 0x1000000000000008L;
    q[1] = 0xE;
    g_render_packet_cursor += 4;
    d = (unsigned long *)g_render_packet_cursor;
    sce_gs_set_def_draw_env(d, fs_aa_buffer_f.psm, fs_aa_buffer_f.w, fs_aa_buffer_f.h, 3, 0);
    ((GsFrame *)d)->FBP = fs_aa_buffer_f.fbp;
    d[2] = fs_aa_buffer_f.zbp | ((unsigned long)(fs_aa_buffer_f.zpsm & 0xF) << 24);
    g_render_packet_cursor += 0x20;
    r = (long *)g_render_packet_cursor;
    r[0] = 0x1000000000000001L;
    r[1] = 0xE;
    r[2] = 0x30003;
    r[3] = 0x47;
    r[4] = 0x4400000000008001L;
    r[5] = 0x4410;
    r[6] = 0x106;
    r[7] = 0;
    r[8] = (long)(0x8000 - fs_aa_buffer_f.w * 8) | ((long)(0x8000 - fs_aa_buffer_f.h * 8) << 16);
    r[9] = (long)(fs_aa_buffer_f.w * 8 + 0x8000) | ((long)(fs_aa_buffer_f.h * 8 + 0x7FF0) << 16);
    g_render_packet_cursor += 0x14;
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

extern char fs_aa_draw_packet[];

/* AA_BlurPass(void) */
void AA_BlurPass(void) {
    g_render_packet_cursor[0] = 0x30000026;
    g_render_packet_cursor[1] = (int)fs_aa_draw_packet;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000026;
    g_render_packet_cursor += 4;
}

extern char fs_aa_transfer_packet[];

/* Append a DMA tag that calls the frame AA transfer packet to the render packet. */
void append_palette_transfer_packet(void) {
    g_render_packet_cursor[0] = 0x30000029;
    g_render_packet_cursor[1] = (int)fs_aa_transfer_packet;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000029;
    g_render_packet_cursor += 4;
}

extern void VU1_addGSregister_l(int, long) __asm__("func_00234C98");

/* Clears the screen through a GIF packet appended to D_00161000: a fixed header then n = w / 32 pairs of sprite corner registers stepping 0x200 per column across a w x h area centred on 0x8000. Adapted from Lombyte's project (GPL-3.0) for PAL: src/rendering/fun_001fb740.c, FUN_001fb740. */
void draw_centered_capture_background(int w, int h) {
    int *tag;
    long *q;
    long *c;
    int n;
    int i;
    int zero;
    int step;
    int x;
    int y;
    long lo;
    long hi;

    n = w / 32;
    VU1_addGSregister_l(0x42, 0x800000004AL);
    g_render_packet_cursor[0] = (n + 5) | 0x10000000;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = (n + 5) | 0x50000000;
    tag = g_render_packet_cursor;
    q = (long *)(tag + 4);
    g_render_packet_cursor = tag + 4;
    zero = 0;
    q[zero] = 0x1000000000000001L;
    q[1] = 0xE;
    q[2] = 0x32003;
    q[3] = 0x47;
    q[4] = 0x2400000000000001L;
    q[5] = 0x10;
    q[6] = 0x146;
    q[7] = 0x80008080L;
    q[8] = (long)(n | 0x8000) | 0x2400000000000000L;
    q[9] = 0x44;
    i = zero;
    if (n > zero) {
        lo = (long)(0x8000 - h * 8) << 16;
        x = 0x8000 + -(w * 8);
        c = (long *)((char *)tag + 0x60);
        y = 0x8200 + -(w * 8);
        hi = (long)(h * 8 + 0x7FF0) << 16;
        do {
            step = 0x200;
            *c++ = x | lo;
            *c++ = y | hi;
            y += step;
            x += step;
            i++;
        } while (i < n);
    }
    g_render_packet_cursor = g_render_packet_cursor + (n * 4 + 20);
}

extern void vu1_add_vif_code(int);
extern void VU1_addGSregister(int, long);

/* Draws an alpha-blended sprite through a GIF packet appended to
   D_00161000 (a DMA/VIF header, TEST_1 0x33003, then PRIM 0x106 with
   RGBAQ rgba and two XYZ2 corners), with ALPHA_1 (GS register 0x42) set
   to 0x64 around it by func_00234C98 and a VIF flush (func_00234C50,
   0x13000000) on each side. The corners are (x << 4) + 0x8000 minus
   half the w/h extent, in 12.4 fixed point. The int sums are kept in their own locals
   (fold would otherwise share the 0x8000 with the extent), and the four
   64-bit corner values are converted in the order that leaves retail's
   last uses, which the scheduler follows. */
void draw_framebuffer_rect(int x0, int y0, int x1, int y1, int w, int h, unsigned int rgba) {
    long *q;
    int ax, ay, bx, by;
    int hw, hh;

    vu1_add_vif_code(0x13000000);
    VU1_addGSregister(0x42, 0x64);
    ax = (x0 << 4) + 0x8000;
    ay = (y0 << 4) + 0x8000;
    bx = (x1 << 4) + 0x8000;
    by = (y1 << 4) + 0x8000;
    hw = w << 3;
    hh = h << 3;
    g_render_packet_cursor[0] = 0x10000006;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000006;
    g_render_packet_cursor += 4;
    q = (long *)g_render_packet_cursor;
    q[0] = 0x1000000000000001L;
    q[1] = 0xE;
    q[2] = 0x33003;
    q[3] = 0x47;
    q[4] = 0x2400000000000001L;
    q[5] = 0x10;
    q[6] = 0x106;
    q[7] = rgba;
    q[8] = 0x2400000000008001L;
    q[9] = 0x44;
    {
        long X1 = bx - hw;
        long X0 = ax - hw;
        long Y0 = ay - hh;
        long Y1 = by - hh;

        q[10] = X0 | (Y0 << 16);
        q[11] = X1 | (Y1 << 16);
    }
    g_render_packet_cursor += 0x18;
    VU1_addGSregister(0x42, 0x8000000044L);
    vu1_add_vif_code(0x13000000);
}

LINKER_REMNANT("asm/remnants/text", func_001FBC78);
