#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * hud.cpp in the original source; text 0x1FF668-0x201D58.
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
extern int FontPrintCenterLarge(int a, int b, int c, int d, int e);

typedef struct {
    unsigned short id;
    char pad[6];
} HudIconRec;
extern HudIconRec *hud_icon_table;

/* Finds arg0 in the table D_0019A504 points to (ended by id 0xFFFF).
   The older decode read D_0019A504 as the table itself. */
int Hud_GetIconIndex(int arg0) {
    HudIconRec *tab = hud_icon_table;
    int i = 0;

    while (tab[i].id != 0xFFFF && tab[i].id != arg0) {
        i++;
    }
    return i;
}

extern int hud_state NOT_SDA;
extern int queue_animation_update(int, int, int, int, int, int, int);
extern char *Hud_HeapAlloc_d(int, int, char *, int) __asm__("func_001FFAB8");
extern char hud_file_str[];
extern unsigned char *hud_bank_main MACRO_ADDR;
extern unsigned char *hud_bank_cursor MACRO_ADDR;
extern unsigned char *hud_bank_end MACRO_ADDR;
extern unsigned char *hud_bank_aux MACRO_ADDR;
typedef struct {
    int unk00, unk04;
    char pad08[0x18];
    int unk20, unk24;
    int unk28, unk2C, unk30, unk34, unk38;
    char pad3C[0x28];
    int unk64, unk68, unk6C;
    int unk70;
    char pad74[8];
    int unk7C;
    char pad80[0x10];
} HudBank;
extern HudBank hud_chan_slots_b[] __asm__("D_00199C60") NOT_SDA;

/* HUD bank init: clears the first two words of the D_0019A4E8 arena,
   resets the 13 0x90-byte bank records at D_00199C60 (+0x64 = -1, the
   pending +0x20 selector = 0x10000, func_001FFB38(i, 0xFFFF, 0, 0, 0, 0,
   1), then +0x7C = 0, +0x6C = -6, +0x04 = +0x24 = 0), allocates the two
   HUD buffers once through the debug allocator (size, 0, "hud.cpp",
   line 277/278), sets D_0015FAC0/D_0015FABC from the first, clears
   0x2800 bytes of it and sets its byte +0x20 to 0xFF. The records are
   indexed through a struct array: each field access is its own giv,
   and loop.c's pick (the +0x24 one) is retail's loop pointer. */
void init_hud(void) {
    int *arena = &hud_state;
    int i;
    unsigned char *buf;

    arena[0] = 0;
    arena[1] = 0;
    for (i = 0; i < 13; i++) {
        hud_chan_slots_b[i].unk64 = -1;
        hud_chan_slots_b[i].unk20 = 0x10000;
        queue_animation_update(i, 0xFFFF, 0, 0, 0, 0, 1);
        hud_chan_slots_b[i].unk7C = 0;
        hud_chan_slots_b[i].unk6C = -6;
        hud_chan_slots_b[i].unk04 = 0;
        hud_chan_slots_b[i].unk24 = 0;
    }
    if (hud_bank_main == 0) {
        hud_bank_main = (unsigned char *)Hud_HeapAlloc_d(0x2800, 0, hud_file_str, 277);
        hud_bank_aux = (unsigned char *)Hud_HeapAlloc_d(0x1400, 0, hud_file_str, 278);
    }
    buf = hud_bank_main;
    hud_bank_end = buf + 0x2800;
    hud_bank_cursor = buf;
    FastMemZero16(buf, 0x2800);
    hud_bank_main[0x20] = 0xFF;
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

typedef struct {
    int offset;
    int unk4;
} HudEntry;

typedef struct {
    char pad0[0x14];
    int limits1[1];
    char pad18[0x1C];
    int limits2[1];
    char pad38[0x3C];
    int banks[1];
} HudBankHeader;

typedef struct {
    char pad0[0x18];
    HudBankHeader *header;
    char pad1C[8];
    HudEntry *table2;
    HudEntry *table1;
} HudArena;

extern HudArena hud_state_arena __asm__("D_0019A4E8");

/* LinkHudBank(int, char *) */
void LinkHudBank(int bank, int addr) {
    int *banks;
    int *slot;
    int start;
    int limit;
    int i;

    banks = (int *)((char *)hud_state_arena.header + 0x74);
    slot = &banks[bank];
    if (*slot != 0) {
        return;
    }

    addr = (addr + 0xF) & 0xFFFFFFF0U;
    *slot = addr;

    if (bank != 0) {
        start = hud_state_arena.header->limits1[bank - 1];
    } else {
        start = 0;
    }
    limit = hud_state_arena.header->limits1[bank];

    for (i = start; i < limit; i++) {
        hud_state_arena.table1[i].offset &= 0x7FFFFFFF;
        hud_state_arena.table1[i].offset += addr;
    }

    if (bank != 0) {
        start = hud_state_arena.header->limits2[bank - 1];
    } else {
        start = 0;
    }
    limit = hud_state_arena.header->limits2[bank];

    for (i = start; i < limit; i++) {
        hud_state_arena.table2[i].offset &= 0x7FFFFFFF;
        hud_state_arena.table2[i].offset += addr;
    }
}

LINKER_REMNANT("asm/remnants/text", func_001FF950);

extern s32 depth_buffer_address MACRO_ADDR;
struct HudAnimDef {
    u16 id;
    u16 count;
    u16 start;
    u8 flags;
    u8 pad7;
};
struct HudFrameRef {
    s16 palette_index;
    s16 image_index;
};
struct HudTexPage {
    u32 source_address;
    u16 gs_block_offset;
    u8 width_log2;
    u8 height_log2;
};
struct HudTexCounts {
    u8 pad0[0x14];
    s32 mid_ends[8];
    s32 ends[16];
    s32 loaded[16];
};
struct HudBank {
    s32 unk0;
    s32 off4;
    s32 off8;
    s32 offC;
    s32 off10;
    u8 pad14[0x40];
    s32 has54;
    s32 size58;
    s32 has5C;
    s32 has60;
    s32 has64;
    u8 pad68[0x2C];
    s32 unk94;
    s32 unk98;
    s32 unk9C;
    s32 unkA0;
    s32 unkA4;
};
struct HudState {
    s32 serial; /* 0x00: next animation serial (queue_animation_update) */
    s32 unk4;
    s32 unk8;
    s32 z; /* 0x0C: sprite Z in the high word of XYZ */
    s32 heap_cur; /* 0x10: hud_heap_alloc cursor */
    s32 heap_end; /* 0x14 */
    union {
        struct HudTexCounts *counts;
        struct HudBank *bank;
    } header; /* 0x18 */
    struct HudAnimDef *anim_defs; /* 0x1C */
    struct HudFrameRef *frame_refs; /* 0x20 */
    struct HudTexPage *image_pages; /* 0x24 */
    struct HudTexPage *palette_pages; /* 0x28 */
    s32 unk2C;
    s32 unk30;
};
extern struct HudState hud_state_r __asm__("D_0019A4E8");
extern void LinkHudBank(s32 bank, s32 base);
extern void Hud_sendTexture_r(u32 data, s32 dbp, s32 psm, s32 wlog, s32 hlog, s32 immediate) __asm__("func_00201348");

/* Uploads the image pages of one HUD bank to GS memory, from the depth buffer address up, and records each page's block offset. Bank 0's pages are linked first when the bank file is not linked yet.
   Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/ui/hud/hud_send_resident_bank.c, hud_send_resident_bank. */
void Hud_SendResidentBank(s32 bank, s32 base, s32 immediate) {
    struct HudTexCounts *counts;
    s32 address;
    s32 bank_offset;
    s32 first;
    s32 end;
    s32 i;
    s32 page;
    s32 width_log2;
    s32 height_log2;
    s32 size;

    if (hud_state_r.header.counts->loaded[bank] == 0) {
        LinkHudBank(0, base);
        address = depth_buffer_address;
    } else {
        address = depth_buffer_address;
    }
    counts = hud_state_r.header.counts;
    bank_offset = bank << 2;
    first = bank == 0 ? 0 : counts->ends[bank - 1];
    i = first;
    end = *(s32 *)((u8 *)hud_state_r.header.counts + bank_offset + 0x34); /* ends[bank] */
    for (; i < end; i++) {
        page = address >> 8;
        width_log2 = hud_state_r.image_pages[i].width_log2;
        height_log2 = hud_state_r.image_pages[i].height_log2;
        size = 1 << (width_log2 + height_log2);
        Hud_sendTexture_r(hud_state_r.image_pages[i].source_address, page, 0x1B, width_log2,
                         height_log2, immediate);
        address += size * 4;
        hud_state_r.image_pages[i].gs_block_offset = page;
    }
}

extern int hud_heap_base NOT_SDA;
extern int hud_state NOT_SDA;

/* Hud_HeapReset(void) */
void Hud_HeapReset(void) {
    int *p = &hud_state;
    int v = hud_heap_base;
    p[5] = v + 0x64000;
    p[4] = v;
}

/* Bump allocator out of the D_0019A4E8 arena: p[4] is the cursor,
   p[5] the limit. Rounds the request up to 16 bytes. */
/* Hud_HeapAlloc(unsigned int, char *, char *, int) */
int Hud_HeapAlloc(int size) {
    int *p = &hud_state;
    int cur;

    if (p[4] == 0) {
        Hud_HeapReset();
    }
    if (p[5] - p[4] < size) {
        return 0;
    }
    cur = p[4];
    size = (size + 15) & 0xFFFFFFF0;
    p[4] = cur + size;
    return cur;
}

/*
 * 12 bytes of nop padding follow func_001FFAB8 in retail, after
 * `endlabel` in asm/nonmatchings/text/func_001FFAB8.s -- the same trap
 * documented above func_001F6668. Dropping it shifted the rest of the
 * segment by -8 (the next .align 3 only recovered 4 of the 12) and broke
 * func_00202790 and func_002208F8, both of which were exact and are
 * instruction-for-instruction identical apart from their jal targets.
 */
__asm__(".section .text
	nop
	nop
	nop
");

extern char hud_chan_slots_raw[] __asm__("D_00199C60") NOT_SDA;
extern int g_game_mode MACRO_ADDR;
extern void apply_pending_animation(void *arg0);

/* Finds or creates the residency record for arg0's slot (idx = arg0 & 0xF)
   in D_00199C60 (0x90-byte records), matching arg1..arg6 against the
   record's cached selectors; returns the record's +0x64 bank id, calling
   func_001FFC48 to reload the bank when a bit of (arg0's high nibbles &
   the record's +0x04 flags) is set. Sibling of func_001FFCB0/func_001FFD30
   in this file, which use the same record layout. */
int queue_animation_update(int arg0, int arg1, int arg2, int arg3, int arg4,
                 int arg5, int arg6) {
    int idx = arg0 & 0xF;
    HudBank *p = (HudBank *)(hud_chan_slots_raw + idx * 0x90);
    int masked0 = arg0 & 0xFFF0;
    int r3;
    int cur;

    if (g_game_mode == 5 && idx != 2 && idx != 0) {
        return 0;
    }
    if (p->unk2C == arg5 && p->unk28 == arg6
        && p->unk20 == arg1 && p->unk24 == masked0
        && p->unk30 == arg2 && p->unk34 == arg3
        && p->unk38 == arg4) {
        return p->unk64;
    }
    cur = hud_state;
    p->unk2C = arg5;
    r3 = masked0 & p->unk04;
    p->unk64 = cur;
    r3 = r3 & 0x20;
    cur = cur + 1;
    p->unk28 = arg6;
    hud_state = cur;
    p->unk20 = arg1;
    p->unk30 = arg2;
    p->unk34 = arg3;
    p->unk38 = arg4;
    p->unk68 = 1;
    p->unk24 = masked0;
    p->unk7C = 0;
    p->unk70 = 0;
    if (r3 != 0) {
        apply_pending_animation(p);
    }
    return p->unk64;
}

extern void load_animation_definition(void *, int);

/* Copies the six pending words at +0x24..+0x38 into +0x04..+0x18 after
   func_001FFD30 has seen +0x20, calls the handler now at +0x10 if there
   is one, and clears +0x68. The copies are in retail's store order, and
   the +0x10 copy is unconditional (retail stores it in the beqz's delay
   slot), which also makes the scheduler load +0x30 first. */
typedef struct AnimRec {
    u8 pad_00[0x4];
    s32 a;                   /* 0x04 */
    s32 d;                   /* 0x08 */
    s32 e;                   /* 0x0C */
    void (*fn)(void *);      /* 0x10: handler, called with the record if nonzero */
    s32 c;                   /* 0x14 */
    s32 b;                   /* 0x18 */
    u8 pad_1C[0x4];
    s32 next_id;             /* 0x20: definition id for load_animation_definition */
    s32 next_a;              /* 0x24 */
    s32 next_d;              /* 0x28 */
    s32 next_e;              /* 0x2C */
    void (*next_fn)(void *); /* 0x30 */
    s32 next_c;              /* 0x34 */
    s32 next_b;              /* 0x38 */
    u8 pad_3C[0x2C];
    s32 pending;             /* 0x68: cleared once the pending values are applied */
} AnimRec;

/* Loads the queued animation and copies its pending values into place. */
void apply_pending_animation(void *arg0) {
    struct AnimRec *anim = (struct AnimRec *)arg0;

    load_animation_definition(arg0, anim->next_id);
    anim->a = anim->next_a;
    anim->c = anim->next_c;
    anim->b = anim->next_b;
    anim->e = anim->next_e;
    anim->d = anim->next_d;
    anim->fn = anim->next_fn;
    if (anim->fn != 0) {
        anim->fn(arg0);
    }
    anim->pending = 0;
}

/* The 13 0x90-byte records at D_00199C60, looked up by their +0x64. */
typedef struct {
    int unk00, unk04;
    char pad08[0x1C];
    int unk24;
    char pad28[0x3C];
    int unk64, unk68;
    char pad6C[0x24];
} HudRec90;
extern HudRec90 hud_chan_slots[] NOT_SDA;
/* func_001FFB38 returns the bank id; this caller ignores it, and its
   match was made against a void view. */
extern void queue_animation_update_v(int, int, int, int, int, int, int) __asm__("func_001FFB38");

/* Calls func_001FFB38(i, 0xFFFF, 0, 0, 0, 0, 0) on the record whose
   +0x64 is arg0 and returns 1, or returns 0 when none is. The nop in
   the search loop is ps2eeas's short-loop padding. */
int remove_hud_item(int arg0) {
    int i;
    for (i = 0; i < 13; i++) {
        if (hud_chan_slots[i].unk64 == arg0) {
            break;
        }
    }
    if (i < 13) {
        queue_animation_update_v(i, 0xFFFF, 0, 0, 0, 0, 0);
        return 1;
    }
    return 0;
}

extern int Hud_GetIconIndex(int);

/* tbl[7] is volatile: retail re-loads that pointer field before each of
   the three record accesses rather than caching it, while keeping the
   r*8 offset in a register. */
void load_animation_definition(void *arg0, int arg1) {
    char *self = (char *)arg0;
    volatile int *tbl;
    int r;
    unsigned short v;

    r = Hud_GetIconIndex(arg1);
    /* base materialized only after the call, so it lands in a temp
       register rather than a callee-saved one; tbl[7] is volatile because
       retail re-loads that pointer field before each record access. */
    tbl = (volatile int *)&hud_state;
    v = *(unsigned short *)((char *)tbl[7] + r * 8);
    *(short *)(self + 0x40) = r;
    *(int *)(self + 0) = v;
    *(char *)(self + 0x42) = *(unsigned char *)((char *)tbl[7] + r * 8 + 6);
    *(int *)(self + 0x44) = *(unsigned short *)((char *)tbl[7] + r * 8 + 4);
}

LINKER_REMNANT("asm/remnants/text", func_001FFD98);

/* Stores arg1 into +0x24 of the record whose +0x64 is arg0, and into
   +0x04 when its +0x68 is 0. Indexing the extern array at each access,
   not through a cached base pointer, gives retail's per-access %lo. */
void Hud_SetBarValueById(int arg0, int arg1) {
    int i;
    for (i = 0; i < 13; i++) {
        if (hud_chan_slots[i].unk64 == arg0) {
            break;
        }
    }
    if (i < 13) {
        hud_chan_slots[i].unk24 = arg1;
        if (hud_chan_slots[i].unk68 == 0) {
            hud_chan_slots[i].unk04 = arg1;
        }
    }
}

LINKER_REMNANT("asm/remnants/text", func_001FFE18);

typedef struct {
    char b[0x13];
} Cfg13;

extern Cfg13 message_text_buffer NOT_SDA;
extern Cfg13 message_too_long_label NOT_SDA;
extern int libc_strlen(void);
extern void libc_strcpy(Cfg13 *, void *);

/* The struct copy is a plain assignment: at alignment 1 this compiler
   expands the 0x13 bytes as unaligned ldl/ldr + sdl/sdr pairs with the
   trailing three bytes done singly, which is exactly retail's shape. */
extern int libc_strlen_hud(void *) __asm__("func_00116810");
/* Copies text into the shared message buffer, preloading the too-long label when under 80 chars. */
void copy_text_to_shared_buffer(void *arg0) {
    if ((unsigned int)libc_strlen_hud(arg0) < 0x50) {
        message_text_buffer = message_too_long_label;
    }
    libc_strcpy(&message_text_buffer, arg0);
}

LINKER_REMNANT("asm/remnants/text", func_001FFF08);

extern short ResourceCounter;   /* SDA (gp -0x7330) */

/* Counts the resource counter down by one until it reaches 0. */
void update_resource_counter(void) {
    int v = *(int *)&ResourceCounter;
    if (v != 0) {
        *(int *)&ResourceCounter = v - 1;
    }
}

/* D_00199C60's 0x90-byte bank records, seen through their +0x18 update
   callback. */
typedef struct HudBankCb {
    char pad00[0x18];
    void (*update)(struct HudBankCb *);
    char pad1C[0x74];
} HudBankCb;

extern char hud_state_raw[] __asm__("D_0019A4E8");
extern HudBankCb hud_chan_slots_cb[] __asm__("D_00199C60") NOT_SDA;
extern int letterbox_enabled MACRO_ADDR;
extern int hud_msg_count MACRO_ADDR;
extern int hud_msg_alpha MACRO_ADDR;
extern int hud_msg_y MACRO_ADDR;
extern char hud_msg_text1[];
extern unsigned char hud_msg_text2[];
extern int scale_game_frame_count(int);
extern void draw_framed_text(int, int, int, int);

/* Per-frame HUD update: unless the arena's +0x30 skip flag (cleared
   here) or D_0015F544 is set, reset the arena's +0xC colour, run each
   bank's +0x18 callback, then fade the D_0015F764 alpha (0..128) in
   while the D_0015F760 timer runs, or out once it stops, and draw
   the overlay text D_001994D8 (plus D_00199528 above 1000) with it. */
void update_hud(void) {
    char *arena = hud_state_raw;
    int i;
    HudBankCb *p;
    int col;

    if (*(int *)(arena + 0x30) != 0 || letterbox_enabled != 0) {
        *(int *)(arena + 0x30) = 0;
        return;
    }
    *(int *)(arena + 0xC) = 0xFFFFF0;
    p = hud_chan_slots_cb;
    for (i = 0; i < 13; i++) {
        if (p->update != 0) {
            p->update(p);
        }
        p++;
    }
    if ((hud_msg_count != 0 || hud_msg_alpha != 0) && g_game_mode == 0) {
        if (hud_msg_count != 0) {
            hud_msg_alpha += 128 / scale_game_frame_count(8);
            if (hud_msg_alpha > 128) {
                hud_msg_alpha = 128;
            }
        } else {
            hud_msg_alpha -= 128 / scale_game_frame_count(8);
            if (hud_msg_alpha < 0) {
                hud_msg_alpha = 0;
            }
        }
        col = (hud_msg_alpha << 24) + 0xF0F0F0;
        draw_framed_text(0x100, hud_msg_y, col, (int)hud_msg_text1);
        if (hud_msg_text2[0] != 0 && hud_msg_count > 1000) {
            draw_framed_text(0x100, hud_msg_y, col, (int)hud_msg_text2);
        }
        if (hud_msg_count != 0) {
            hud_msg_count--;
        }
        if (hud_msg_count == 1000) {
            hud_msg_count = 0;
        }
    } else {
        hud_msg_y = 100;
    }
}

LINKER_REMNANT("asm/remnants/text", func_00200190);


/* GetIconFrame(iconId, sub): finds the icon's {id, count, base} row
   (func_001FF668 gives its index) in the arena's +0x1C table; frame =
   base + sub. Returns frame when the id is known, sub < count, and
   neither the +0x28 entry (for the frame's first index) nor the +0x24
   entry (for its second) has the top bit set; else 0. Row and pair
   are int sums so their addu takes the index first. Both flag tests
   are `& 0x80000000`: the first becomes a bltz, but the constant it
   loaded is reused by the second test, as in retail. */
int GetIconFrame(int iconId, int sub) {
    int idx;
    char *arena;
    unsigned short *row;
    int frame;
    short *pair;

    idx = Hud_GetIconIndex(iconId);
    arena = hud_state_raw;
    row = (unsigned short *)(idx * 8 + *(int *)(arena + 0x1C));
    if (row[0] == 0xFFFF) {
        return 0;
    }
    if (sub >= row[1]) {
        return 0;
    }
    frame = row[2] + sub;
    pair = (short *)(frame * 4 + *(int *)(arena + 0x20));
    if (*(int *)(*(char **)(arena + 0x28) + pair[0] * 8) & 0x80000000) {
        return 0;
    }
    if ((*(int *)(*(char **)(arena + 0x24) + pair[1] * 8) & 0x80000000) == 0) {
        return frame;
    }
    return 0;
}

__asm__(".section .text\n\tnop\n");

extern s32 g_gs_texture_allocation_cursor_r __asm__("D_0015EF74") MACRO_ADDR;
struct TextureUpload_00248 {
    s32 clut_data; /* 0x00: EE address of the CLUT (fun_00204cf0: texture CLUT pointer) */
    s16 unk4; /* 0x04: every writer stores 0 */
    s16 cbp; /* 0x06: GS block of the CLUT, TEX0.CBP */
    s32 image_data; /* 0x08: EE address of the pixels (fun_00204cf0: texture pixel pointer) */
    u8 tw; /* 0x0C: log2 width, TEX0.TW */
    u8 th; /* 0x0D: log2 height, TEX0.TH */
    s16 tbp; /* 0x0E: GS block of the pixels, TEX0.TBP0 */
};
struct FrameTextureRef_00248 {
    s16 palette_index;
    s16 image_index;
};
struct FramePalettePage_00248 {
    s32 source_address;
    u16 gs_block_offset;
};
struct FrameImagePage_00248 {
    s32 source_address;
    u16 gs_block_offset;
    u8 width_log2;
    u8 height_log2;
};
extern s32 g_gs_texture_allocation_base_r __asm__("D_0015EF8C") MACRO_ADDR;
extern s32 pending_texture_upload_count_r __asm__("D_0015F558") MACRO_ADDR;
extern struct TextureUpload_00248 pending_texture_uploads_r[] __asm__("D_0018D140");
struct FrameTextureTables_00248 {
    u8 pad00[0x20];
    s32 frame_references_address;
    s32 image_pages_address;
    s32 palette_pages_address;
};
extern struct FrameTextureTables_00248 hud_state_r2 __asm__("D_0019A4E8");

/* GetFrameTex: returns the TEX0 word of a HUD frame, allocating GS blocks for its palette and image pages
   and queueing their uploads the first time.
   Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/textbin/gameplay/animation/get_frame_texture.c, get_frame_texture. */
u64 func_00200248_r(s32 frame_id) __asm__("func_00200248");
/* GetFrameTex(int) */
u64 func_00200248_r(s32 frame_id) {
    struct FrameTextureRef_00248 *frame;
    struct FramePalettePage_00248 *palette_page;
    struct FrameImagePage_00248 *image_page;
    struct TextureUpload_00248 *packet;
    struct TextureUpload_00248 *initial_packet;
    struct TextureUpload_00248 *palette_packet;
    u32 packet_offset;
    u32 queued_transfer;
    s32 image_upload_count;
    s32 allocation_cursor;
    u8 width_log2;
    u8 height_log2;
    s32 palette_source_address;
    s32 return_mode;
    s32 return_shift;
    u64 tex0_word;
    u64 palette_word;

    frame =
        (struct FrameTextureRef_00248 *)(frame_id * 4 + hud_state_r2.frame_references_address);
    palette_page = (struct FramePalettePage_00248 *)(hud_state_r2.palette_pages_address +
                                               frame->palette_index * 8);
    image_page = (struct FrameImagePage_00248 *)(hud_state_r2.image_pages_address +
                                           frame->image_index * 8);
    queued_transfer = 0;

    if (palette_page->gs_block_offset == 0 || image_page->gs_block_offset == 0) {
        /* Retail fills this first packet even when the queue is already full. */
        packet_offset = pending_texture_upload_count_r * 0x10;
        palette_source_address = palette_page->source_address;
        initial_packet = pending_texture_uploads_r + pending_texture_upload_count_r;
        initial_packet->clut_data = palette_source_address;
        initial_packet->unk4 = 0;
        initial_packet->cbp = 0x3FF0;
        *(s32 *)((u8 *)pending_texture_uploads_r + packet_offset + 8) = palette_page->source_address;
        initial_packet->tw = 5;
        initial_packet->th = 5;
        initial_packet->tbp = 0x3FF0;
    }

    if (palette_page->gs_block_offset == 0) {
        palette_page->gs_block_offset = g_gs_texture_allocation_cursor_r >> 8;
        g_gs_texture_allocation_cursor_r += 0x400;
        if (pending_texture_upload_count_r < 0x40) {
            queued_transfer = 1;
            palette_packet = pending_texture_uploads_r + pending_texture_upload_count_r;
            palette_packet->clut_data = palette_page->source_address;
            palette_packet->unk4 = 0;
            palette_packet->cbp = palette_page->gs_block_offset;
        }
    }

    if (image_page->gs_block_offset == 0) {
        height_log2 = image_page->height_log2;
        width_log2 = image_page->width_log2;
        allocation_cursor = g_gs_texture_allocation_cursor_r;
        image_page->gs_block_offset = allocation_cursor >> 8;
        if (height_log2 < width_log2) {
            height_log2 += width_log2 - height_log2;
        }
        g_gs_texture_allocation_cursor_r = allocation_cursor + (1 << (height_log2 * 2));
        image_upload_count = pending_texture_upload_count_r;
        if (image_upload_count < 0x40) {
            queued_transfer = 1;
            packet = pending_texture_uploads_r + image_upload_count;
            *(s32 *)((u8 *)pending_texture_uploads_r + image_upload_count * 0x10 + 8) =
                image_page->source_address;
            packet->tw = image_page->width_log2;
            packet->th = image_page->height_log2;
            packet->tbp = image_page->gs_block_offset;
        }
    }

    if (queued_transfer != 0) {
        pending_texture_upload_count_r += 1;
    }

    return_shift = image_page->width_log2 - 6;
    if (return_shift < 0) {
        return_shift = 0;
    }
    return_mode = 0x13;
    if (image_page->gs_block_offset < (g_gs_texture_allocation_base_r >> 8)) {
        return_mode = 0x1B;
    }
    return_shift = 1 << return_shift;
    tex0_word = image_page->gs_block_offset | ((u64)return_shift << 14);
    tex0_word |= (u64)return_mode << 20;
    tex0_word |= (u64)image_page->width_log2 << 26;
    tex0_word |= (u64)image_page->height_log2 << 30;
    palette_word = (u64)palette_page->gs_block_offset << 37;
    palette_word |= (u64)0x8000 << 19;
    tex0_word |= palette_word;
    tex0_word |= (u64)-1 << 63;
    return tex0_word;
}

extern int *g_render_packet_cursor MACRO_ADDR;
extern int g_screen_extent[];
extern long GetFrameTex(int);

/* Draw HUD texture tex as a sprite at (x, y), w x h, with alpha: a
   1-tag + 5-quadword PACKED GIF packet as func_00200CA0's, with the
   texture's TEX0 from func_00200248 and its full size (1 << the
   entry's +6/+7 log2 sizes) as the far UV. The entry is found through
   the arena's +0x20 index table into its +0x24 texture table. */
void HudSprite(int tex, int x, int y, int w, int h, int alpha) {
    char *arena = hud_state_raw;
    unsigned char *e = (unsigned char *)(*(char **)(arena + 0x24)
        + *(short *)(*(char **)(arena + 0x20) + tex * 4 + 2) * 8);
    int th;
    int tw;
    int *base;
    long *p;

    tw = 1 << e[6];
    th = 1 << e[7];
    g_render_packet_cursor[0] = 0x10000005;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000005;

    base = g_render_packet_cursor;
    g_render_packet_cursor = base + 4;
    p = (long *)g_render_packet_cursor;
    p[0] = 0x7400000000008001L;
    p[1] = 0x5353106;
    p[2] = GetFrameTex(tex);
    p[3] = 0x156;
    p[4] = ((long)alpha << 24) | 0x7F7F7F;
    p[5] = 0;
    p[6] = (x * 16 + g_screen_extent[4] - 8)
         | ((long)(y * 16 + g_screen_extent[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[7] = (th << 20) + tw * 16;
    p[8] = ((x + w) * 16 + g_screen_extent[4] - 8)
         | ((long)((y + h) * 16 + g_screen_extent[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[9] = 0;
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x50);
}

/* func_00200468 drawn as a four-vertex strip (PRIM 0x154) instead of a
   sprite, the texture turned a quarter: corners (x, y+h), (x, y),
   (x+w, y+h), (x+w, y) take UVs (tw, 0), (tw, th), (0, 0), (0, th). */
void draw_hud_sprite_flipped(int tex, int x, int y, int w, int h, int alpha) {
    char *arena = hud_state_raw;
    unsigned char *e = (unsigned char *)(*(char **)(arena + 0x24)
        + *(short *)(*(char **)(arena + 0x20) + tex * 4 + 2) * 8);
    int th;
    int tw;
    int *base;
    long *p;

    tw = 1 << e[6];
    th = 1 << e[7];
    g_render_packet_cursor[0] = 0x10000007;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000007;

    base = g_render_packet_cursor;
    g_render_packet_cursor = base + 4;
    p = (long *)g_render_packet_cursor;
    p[0] = 0xB400000000008001L;
    p[1] = 0x53535353106L;
    p[2] = GetFrameTex(tex);
    p[3] = 0x154;
    p[4] = ((long)alpha << 24) | 0x7F7F7F;
    p[5] = tw * 16;
    p[6] = (x * 16 + g_screen_extent[4] - 8)
         | ((long)((y + h) * 16 + g_screen_extent[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[7] = (th << 20) + tw * 16;
    p[8] = (x * 16 + g_screen_extent[4] - 8)
         | ((long)(y * 16 + g_screen_extent[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[9] = 0;
    p[10] = ((x + w) * 16 + g_screen_extent[4] - 8)
          | ((long)((y + h) * 16 + g_screen_extent[5] - 8) << 16)
          | ((long)*(int *)(arena + 0xC) << 32);
    p[11] = th << 20;
    p[12] = ((x + w) * 16 + g_screen_extent[4] - 8)
          | ((long)(y * 16 + g_screen_extent[5] - 8) << 16)
          | ((long)*(int *)(arena + 0xC) << 32);
    p[13] = 0;
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x70);
}

LINKER_REMNANT("asm/remnants/text", func_002008B0);

/* func_00200468 with the position and size already in 16ths of a pixel. */
void draw_hud_sprite_subpixel(int tex, int x, int y, int w, int h, int alpha) {
    char *arena = hud_state_raw;
    unsigned char *e = (unsigned char *)(*(char **)(arena + 0x24)
        + *(short *)(*(char **)(arena + 0x20) + tex * 4 + 2) * 8);
    int th;
    int tw;
    int *base;
    long *p;

    tw = 1 << e[6];
    th = 1 << e[7];
    g_render_packet_cursor[0] = 0x10000005;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000005;

    base = g_render_packet_cursor;
    g_render_packet_cursor = base + 4;
    p = (long *)g_render_packet_cursor;
    p[0] = 0x7400000000008001L;
    p[1] = 0x5353106;
    p[2] = GetFrameTex(tex);
    p[3] = 0x156;
    p[4] = ((long)alpha << 24) | 0x7F7F7F;
    p[5] = 0;
    p[6] = (x + g_screen_extent[4] - 8)
         | ((long)(y + g_screen_extent[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[7] = (th << 20) + tw * 16;
    p[8] = (x + w + g_screen_extent[4] - 8)
         | ((long)(y + h + g_screen_extent[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[9] = 0;
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x50);
}

/* func_002008B8 with an explicit texture window: UVs run from (u, v) to
   (u, v) plus the texture's size in 16ths. */
void draw_hud_sprite_uv(int tex, int x, int y, int w, int h, int u, int v, int alpha) {
    struct HudState *hud = &hud_state_r;
    struct HudTexPage *image_page;
    int th;
    int tw;
    int *base;
    long *p;

    image_page = &hud->image_pages[hud->frame_refs[tex].image_index];
    tw = 1 << (image_page->width_log2 + 4);
    th = 1 << (image_page->height_log2 + 4);
    g_render_packet_cursor[0] = 0x10000005;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000005;

    base = g_render_packet_cursor;
    g_render_packet_cursor = base + 4;
    p = (long *)g_render_packet_cursor;
    p[0] = 0x7400000000008001L;
    p[1] = 0x5353106;
    p[2] = GetFrameTex(tex);
    p[3] = 0x156;
    p[4] = ((long)alpha << 24) | 0x7F7F7F;
    p[5] = u | ((long)v << 16);
    p[6] = (x + g_screen_extent[4] - 8)
         | ((long)(y + g_screen_extent[5] - 8) << 16)
         | ((long)hud->z << 32);
    p[7] = (u + tw) | ((long)(v + th) << 16);
    p[8] = (x + w + g_screen_extent[4] - 8)
         | ((long)(y + h + g_screen_extent[5] - 8) << 16)
         | ((long)hud->z << 32);
    p[9] = 0;
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x50);
}

extern int *g_render_packet_cursor MACRO_ADDR;

/* Appends a 1-tag + 5-quadword PACKED GIF packet to D_00161000: a GIFtag
   (0x10000005 / 0 / 0 / 0x50000005), fixed GS register data, arg0
   verbatim, a colour of 0x7F7F7F with alpha arg9, then a UV/XYZ pair
   per corner: UV (arg7, arg8) and (arg7 + (1 << (arg3 + 4)),
   arg8 + (1 << (arg4 + 4))); XY (arg1, arg2) and (arg1 + arg5,
   arg2 + arg6), offset by D_0013E600[4]/[5] - 8, with Z from the HUD
   arena's +0xC word. The payload is written through the advanced
   D_00161000 (retail stores relative to it), and the arena is reached
   through a local pointer so its address is built in a register. */
struct SpritePacket {
    u32 dma_word0;
    u32 dma_addr;
    u32 dma_word2;
    u32 dma_word3;
    u64 gif_tag;
};

typedef struct {
    struct SpritePacket *p;
} SpriteCursor;

/* The HUD arena's depth word at +0xC (sprite_depth_state.z_and_fog in the NTSC decomp). */
struct SpriteDepthState {
    u8 pad_00[0xC];
    s32 z_and_fog;                 /* 0x0C */
};

extern SpriteCursor render_packet_cursor __asm__("D_00161000") MACRO_ADDR;

void append_power_of_two_textured_screen_sprite(long texture_register_value, s32 screen_x, s32 screen_y,
                                                s32 texture_width_log2, s32 texture_height_log2,
                                                s32 screen_width, s32 screen_height, s32 texture_u,
                                                s32 texture_v, s32 alpha) {
    struct SpritePacket *tag;
    u64 *packet_words;
    struct SpriteDepthState *arena = (struct SpriteDepthState *)hud_state_raw;

    render_packet_cursor.p->dma_word0 = 0x10000005;
    render_packet_cursor.p->dma_addr = 0;
    render_packet_cursor.p->dma_word2 = 0;
    render_packet_cursor.p->dma_word3 = 0x50000005;
    tag = render_packet_cursor.p;
    packet_words = (u64 *)((u8 *)tag + 0x10);
    render_packet_cursor.p = (struct SpritePacket *)packet_words;
    tag->gif_tag = 0x7400000000008001;
    packet_words[1] = 0x5353106;
    packet_words[2] = texture_register_value;
    packet_words[4] = ((s64)alpha << 24) | 0x7F7F7F;
    packet_words[3] = 0x156;
    packet_words[5] = texture_u | ((s64)texture_v << 16);
    packet_words[6] = (screen_x + g_screen_extent[4] - 8)
                    | ((s64)(screen_y + g_screen_extent[5] - 8) << 16)
                    | ((long)arena->z_and_fog << 32);
    packet_words[7] = (texture_u + (1 << (texture_width_log2 + 4)))
                    | ((s64)(texture_v + (1 << (texture_height_log2 + 4))) << 16);
    packet_words[8] = (screen_x + screen_width + g_screen_extent[4] - 8)
                    | ((s64)(screen_y + screen_height + g_screen_extent[5] - 8) << 16)
                    | ((long)arena->z_and_fog << 32);
    packet_words[9] = 0;
    render_packet_cursor.p = (struct SpritePacket *)((u8 *)render_packet_cursor.p + 0x50);
}

extern int *g_render_packet_cursor MACRO_ADDR;
extern float FastCos(float);
extern float FastSin(float);
extern void FastVecAdd(float *, float *, float *);
extern void FastVecSub(float *, float *, float *);

/* func_00200CA0's rotated sibling: a 1-tag + 6-quadword PACKED GIF
   packet for a w x h sprite at (x, y) turned by rot. The four corners
   are pos +- a +- b with a = h * (sin, cos) and b = w * (-cos, sin)
   (func_001F9BD8 adds, func_001F9BF0 subtracts vectors), each packed as
   XYZ2 with D_0013E600[4]/[5] - 8 and the HUD arena's +0xC depth; UVs
   run from (0, 0) to (u * 16, v << 20). */
void draw_rotated_sprite(int u, int v, long tex, float x, float y, float w, float h, float rot) {
    float a[4];
    float b[4];
    float pos[4];
    float c0[4];
    float c1[4];
    float c2[4];
    float c3[4];
    int *base;
    long *p;
    int *vp;
    char *arena;

    pos[0] = x;
    pos[1] = y;
    a[0] = h * FastSin(rot);
    a[1] = h * FastCos(rot);
    b[0] = -w * FastCos(rot);
    b[1] = w * FastSin(rot);
    FastVecAdd(c0, pos, a);
    FastVecSub(c0, c0, b);
    FastVecAdd(c1, pos, a);
    FastVecAdd(c1, c1, b);
    FastVecSub(c2, pos, a);
    FastVecSub(c2, c2, b);
    FastVecSub(c3, pos, a);
    FastVecAdd(c3, c3, b);

    g_render_packet_cursor[0] = 0x10000007;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000007;
    base = g_render_packet_cursor;
    g_render_packet_cursor = base + 4;
    p = (long *)g_render_packet_cursor;
    vp = g_screen_extent;
    arena = hud_state_raw;
    p[0] = 0xB400000000008001L;
    p[1] = 0x53535353106L;
    p[2] = tex;
    p[3] = 0x154;
    p[4] = 0x807F7F7FL;
    p[5] = u * 16;
    p[6] = ((int)c0[0] + vp[4] - 8)
         | ((long)((int)c0[1] + vp[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[7] = v * 0x100000 + u * 16;
    p[8] = ((int)c1[0] + vp[4] - 8)
         | ((long)((int)c1[1] + vp[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[9] = 0;
    p[10] = ((int)c2[0] + vp[4] - 8)
          | ((long)((int)c2[1] + vp[5] - 8) << 16)
          | ((long)*(int *)(arena + 0xC) << 32);
    p[11] = v * 0x100000;
    p[12] = ((int)c3[0] + vp[4] - 8)
          | ((long)((int)c3[1] + vp[5] - 8) << 16)
          | ((long)*(int *)(arena + 0xC) << 32);
    p[13] = 0;
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x70);
}

typedef struct {
    char pad0[0xC];
    int depth;
} HudDepthView;

/* HUD sprite packet with explicit UV corners, adapted from Lombyte's project (GPL-3.0). */
void draw_hud_sprite_rect(int tex, int x0, int y0, int x1, int y1,
                   int u0, int v0, int u1, int v1, int alpha) {
    int *base;
    long *p;
    HudDepthView *arena;

    g_render_packet_cursor[0] = 0x10000005;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000005;
    base = g_render_packet_cursor;
    g_render_packet_cursor = base + 4;
    p = (long *)g_render_packet_cursor;
    p[0] = ((long)0xE800 << 47) | 0x8001;
    p[1] = 0x5353106;
    p[2] = GetFrameTex(tex);
    p[3] = 0x156;
    p[4] = ((long)alpha << 24) | 0x7F7F7F;
    p[5] = u0 | ((long)v0 << 16);
    arena = (HudDepthView *)hud_state_raw;
    p[6] = (x0 + g_screen_extent[4] - 8)
         | ((long)(y0 + g_screen_extent[5] - 8) << 16)
         | ((long)arena->depth << 32);
    p[7] = u1 | ((long)v1 << 16);
    p[8] = (x1 + g_screen_extent[4] - 8)
         | ((long)(y1 + g_screen_extent[5] - 8) << 16)
         | ((long)arena->depth << 32);
    p[9] = 0;
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x50);
}

extern int *g_render_packet_cursor MACRO_ADDR;
extern void sceGsSetDefLoadImage(void *, int, int, int, int, int, int, int);
extern void flush_cache(int);
extern void sceGsExecLoadImage(void *, void *);

/* Hud_sendTexture(char *tex, int x, int y, int levelShift, int sizeShift,
   int useStackBuf): builds a mip level count (clamped to >=1) and a
   packed GIF NLOOP for the closing tag, then either (useStackBuf==0)
   opens a GS packet in D_00161000 and writes straight into it, or
   builds into a stack buffer; either way calls func_00122630 to fill it
   with (x, levels, y, 1<<levelShift, 1<<sizeShift), then either closes
   the GS packet in place or flushes the cache and hands the stack
   buffer to func_00122958.

   p7 is set before the branch, as retail does in its delay slot, but
   truncated to short after it, with the other short arguments. */
void Hud_sendTexture(char *arg0, int arg1, int arg2, int arg3, int arg4, int arg5) {
    char buf[0x60];
    int nloop2;
    int levels;
    int p7;
    short p5, p6, p10, p11;
    void *dst;
    int *base;

    nloop2 = 1 << (arg3 + arg4 - 4);
    levels = (1 << arg3) >> 6;
    if (levels <= 0) {
        levels = 1;
    }

    p7 = arg2;
    if (arg5 == 0) {
        g_render_packet_cursor[0] = 0x10000006;
        g_render_packet_cursor[1] = 0;
        g_render_packet_cursor[2] = 0;
        g_render_packet_cursor[3] = 0x50000006;
        base = g_render_packet_cursor;
        dst = base + 4;
        g_render_packet_cursor = base + 0x1C;
    } else {
        dst = buf;
    }

    p5 = arg1;
    p6 = levels;
    p7 = (short)p7;
    p10 = 1 << arg3;
    p11 = 1 << arg4;

    sceGsSetDefLoadImage(dst, p5, p6, p7, 0, 0, p10, p11);

    if (arg5 == 0) {
        int *cur = g_render_packet_cursor;
        cur[0] = 0x30000000 | nloop2;
        g_render_packet_cursor[1] = (int)arg0;
        g_render_packet_cursor[2] = 0;
        g_render_packet_cursor[3] = 0x50000000 | nloop2;
        g_render_packet_cursor = g_render_packet_cursor + 4;
    } else {
        flush_cache(0);
        sceGsExecLoadImage(dst, arg0);
    }
}

extern int *g_render_packet_cursor MACRO_ADDR;

/* func_002017C8's sibling: same 1-tag + 3-quadword PACKED GIF packet, but
   register-id constant 0x41 (not 0x46) at +0x20, and a FIXED mask
   0x00FFFFF000000000 in the vertices' upper 32 bits instead of a Z
   parameter -- one fewer int argument than func_002017C8. */
struct Screen {
    s32 pad_00[4];
    s32 left;                       /* 0x10 */
    s32 top;                        /* 0x14 */
};

extern struct Screen screen_extent __asm__("D_0013E600") NOT_SDA;

/* Emits a GS packet that draws a HUD rectangle, in pixel or subpixel coordinates. */
void draw_hud_rect(int x0, int y0, int x1, int y1, long prim, int pixels) {
    long *packet_words;

    g_render_packet_cursor[0] = 0x10000003;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000003;

    packet_words = (long *)g_render_packet_cursor;
    g_render_packet_cursor = (int *)(packet_words + 2);
    packet_words[2] = 0x4400000000000001UL;
    packet_words[3] = 0x4410;
    packet_words[4] = 0x41;
    packet_words[5] = prim;
    if (pixels != 0) {
        packet_words[6] = (x0 + screen_extent.left - 8)
            | ((long)(y0 + screen_extent.top - 8) << 16)
            | 0x00FFFFF000000000L;
        packet_words[7] = (x1 + screen_extent.left - 8)
            | ((long)(y1 + screen_extent.top - 8) << 16)
            | 0x00FFFFF000000000L;
    } else {
        packet_words[6] = (x0 * 16 + screen_extent.left - 0x10)
            | ((long)(y0 * 16 + screen_extent.top - 0x10) << 16)
            | 0x00FFFFF000000000L;
        packet_words[7] = (x1 * 16 + screen_extent.left - 0x10)
            | ((long)(y1 * 16 + screen_extent.top - 0x10) << 16)
            | 0x00FFFFF000000000L;
    }
    g_render_packet_cursor += 12;
}

extern int *g_render_packet_cursor MACRO_ADDR;

/* func_002014B8's sibling (register-id constant 0x46, like
   func_002017C8's, instead of 0x41): same 1-tag + 3-quadword PACKED GIF
   packet with a FIXED mask 0x00FFFFF000000000 in the vertices' upper 32
   bits instead of a Z parameter. */
void draw_hud_rect_blended(int arg0, int arg1, int arg2, int arg3, long arg4,
                    int arg5) {
    int *base;

    g_render_packet_cursor[0] = 0x10000003;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000003;

    base = g_render_packet_cursor;
    g_render_packet_cursor = base + 4;
    *(long *)((char *)base + 0x10) = 0x4400000000000001UL;
    *(long *)((char *)base + 0x18) = 0x4410;
    *(long *)((char *)base + 0x20) = 0x46;
    *(long *)((char *)base + 0x28) = arg4;
    if (arg5 != 0) {
        *(long *)((char *)base + 0x30) =
            (arg0 + g_screen_extent[4] - 8)
            | ((long)(arg1 + g_screen_extent[5] - 8) << 16)
            | 0x00FFFFF000000000L;
        *(long *)((char *)base + 0x38) =
            (arg2 + g_screen_extent[4] - 8)
            | ((long)(arg3 + g_screen_extent[5] - 8) << 16)
            | 0x00FFFFF000000000L;
    } else {
        *(long *)((char *)base + 0x30) =
            (arg0 * 16 + g_screen_extent[4] - 0x10)
            | ((long)(arg1 * 16 + g_screen_extent[5] - 0x10) << 16)
            | 0x00FFFFF000000000L;
        *(long *)((char *)base + 0x38) =
            (arg2 * 16 + g_screen_extent[4] - 0x10)
            | ((long)(arg3 * 16 + g_screen_extent[5] - 0x10) << 16)
            | 0x00FFFFF000000000L;
    }
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x30);
}

extern int *g_render_packet_cursor MACRO_ADDR;

/* Appends a 1-tag + 3-quadword PACKED GIF packet to D_00161000: a GIFtag
   (0x10000003 / 0 / 0 / 0x50000003), then TEX-ish GS register data at
   +0x10/+0x18/+0x20, arg4 verbatim at +0x28, then two packed XYZ2-style
   vertices at +0x30/+0x38: X = argX(+D_0013E600[4])-8, Y =
   argY(+D_0013E600[5])-8, Z = arg5<<32, when arg6 != 0 (raw coordinates);
   or the same with argX/argY scaled by 16 and offset -0x10 when arg6==0
   (tile coordinates). D_00161000 is re-read at every use (never cached
   across a store to it), matching this file's other packet builders. */
void draw_hud_rect_depth(int arg0, int arg1, int arg2, int arg3, long arg4,
                    int arg5, int arg6) {
    int *base;

    g_render_packet_cursor[0] = 0x10000003;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000003;

    base = g_render_packet_cursor;
    g_render_packet_cursor = base + 4;
    *(long *)((char *)base + 0x10) = 0x4400000000000001UL;
    *(long *)((char *)base + 0x18) = 0x4410;
    *(long *)((char *)base + 0x20) = 0x46;
    *(long *)((char *)base + 0x28) = arg4;
    if (arg6 != 0) {
        *(long *)((char *)base + 0x30) =
            (arg0 + g_screen_extent[4] - 8)
            | ((long)(arg1 + g_screen_extent[5] - 8) << 16)
            | ((long)arg5 << 32);
        *(long *)((char *)base + 0x38) =
            (arg2 + g_screen_extent[4] - 8)
            | ((long)(arg3 + g_screen_extent[5] - 8) << 16)
            | ((long)arg5 << 32);
    } else {
        *(long *)((char *)base + 0x30) =
            (arg0 * 16 + g_screen_extent[4] - 0x10)
            | ((long)(arg1 * 16 + g_screen_extent[5] - 0x10) << 16)
            | ((long)arg5 << 32);
        *(long *)((char *)base + 0x38) =
            (arg2 * 16 + g_screen_extent[4] - 0x10)
            | ((long)(arg3 * 16 + g_screen_extent[5] - 0x10) << 16)
            | ((long)arg5 << 32);
    }
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x30);
}

LINKER_REMNANT("asm/remnants/text", func_00201948);

extern int GetIconFrame(int, int);
extern void HudSprite(int, int, int, int, int, int);
extern void draw_hud_sprite_flipped(int, int, int, int, int, int);

/* Draw a 3-part stretchable bar: left cap, stretched middle, right cap
 * (mirrored via func_00200650), all using the same GetIconFrame glyph
 * in its two variants. */
void draw_stretchable_ui_frame(int a0, int a1, int a2, int a3, int a4) {
    int v0 = GetIconFrame(0x7580, 0);
    int v1 = GetIconFrame(0x7580, 1);

    HudSprite(v1, a0, a1, 0x20, a3, a4);
    HudSprite(v0, a0 + 0x20, a1, a2 - 0x40, a3, a4);
    a0 = a0 + a2;
    draw_hud_sprite_flipped(v1, a0 - 0x20, a1, 0x20, a3, a4);
}

extern s32 FontPrintCenterLarge_r(s32, s32, u64, s32, s32) __asm__("func_001F6FD8");
extern void draw_stretchable_ui_frame(s32, s32, s32, s32, s32);
void draw_framed_text(s32 x, s32 y, s32 color, s32 text);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/textbin/ui/frames/draw_framed_text.c, draw_framed_text. */
void draw_framed_text(s32 x, s32 y, s32 color, s32 text) {
    s32 alpha = (s32)color >> 24;
    s32 text_left;
    s32 left;

    if (alpha > 0x50) {
        alpha = 0x50;
    }
    text_left = FontPrintCenterLarge_r(x + 1, y + 1, (s64)color & (s64)(s32)0xFF000000, text, -1);
    left = text_left - 0x20;
    draw_stretchable_ui_frame(left, y - 8, (x - left) * 2, 0x20, alpha);
    FontPrintCenterLarge_r(x, y, color, text, -1);
}

LINKER_REMNANT("asm/remnants/text", func_00201AE0);

struct DmaTag {
    u32 dma_control;
    u32 address;
    u32 vif0;
    u32 vif1;
};
struct TagPtr {
    struct DmaTag *p;
};
extern struct TagPtr g_render_packet_cursor_r __asm__("D_00161000") MACRO_ADDR;
extern s32 pal_mode MACRO_ADDR;
extern s32 draw_buffer_address MACRO_ADDR;
void draw_boot_image(u32 image_address);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/textbin/draw_boot_image.c, draw_boot_image. */
void draw_boot_image(u32 image_address) {
    struct DmaTag *tag;
    struct DmaTag *next;
    u64 *register_words;
    s32 remaining_rows;
    s32 destination_block;
    s64 upload_rows;
    s32 rows_after_upload;
    s64 image_quadword_count;

    remaining_rows = pal_mode ? 0x1C0 : 0x1A0;
    destination_block = draw_buffer_address >> 8;
    do {
        rows_after_upload = remaining_rows - 0x80;
        upload_rows = 0x80;
        if (rows_after_upload < 0) {
            upload_rows = remaining_rows;
        }
        g_render_packet_cursor_r.p->dma_control = 0x10000006;
        g_render_packet_cursor_r.p->address = 0;
        g_render_packet_cursor_r.p->vif0 = 0;
        g_render_packet_cursor_r.p->vif1 = 0x50000006;
        tag = g_render_packet_cursor_r.p;
        next = tag + 7;
        g_render_packet_cursor_r.p = tag + 1;
        register_words = (u64 *)(tag + 1);
        register_words[0] = 0x4000000000000001;
        register_words[1] = 0xEEEEEEE;
        register_words[2] = ((u64)destination_block << 32) | 0x0008000000000000;
        register_words[3] = 0x50;
        register_words[4] = 0;
        register_words[5] = 0x51;
        register_words[6] = ((u64)upload_rows << 32) | 0x200;
        register_words[7] = 0x52;
        register_words[8] = 0;
        register_words[9] = 0x53;
        image_quadword_count = upload_rows << 7;
        register_words[10] = ((u64)image_quadword_count) | 0x0800000000008000;
        register_words[11] = 0;
        g_render_packet_cursor_r.p = next;
        g_render_packet_cursor_r.p->dma_control = image_quadword_count | 0x30000000;
        g_render_packet_cursor_r.p->address = image_address;
        {
            s64 upload_bytes = upload_rows << 11;
            image_address += upload_bytes;
        }
        g_render_packet_cursor_r.p->vif0 = 0;
        g_render_packet_cursor_r.p->vif1 = (s32)image_quadword_count | 0x50000000;
        g_render_packet_cursor_r.p++;
        {
            s64 uploaded_blocks = upload_rows << 3;
            destination_block += uploaded_blocks;
        }
        remaining_rows = rows_after_upload;
    } while (remaining_rows > 0);
    /* Finish the upload chain with a TEXFLUSH register write. */
    {
        struct DmaTag *final_tag;
        struct DmaTag *final_next;
        u64 *final_words;
        g_render_packet_cursor_r.p->dma_control = 0x10000002;
        g_render_packet_cursor_r.p->address = 0;
        g_render_packet_cursor_r.p->vif0 = 0;
        g_render_packet_cursor_r.p->vif1 = 0x50000002;
        final_tag = g_render_packet_cursor_r.p;
        final_next = final_tag + 3;
        final_words = (u64 *)(final_tag + 1);
        g_render_packet_cursor_r.p = final_tag + 1;
        final_words[0] = 0x1000000000008001;
        final_words[1] = 0xE;
        final_words[2] = 0;
        final_words[3] = 0x3F;
        g_render_packet_cursor_r.p = final_next;
    }
}
