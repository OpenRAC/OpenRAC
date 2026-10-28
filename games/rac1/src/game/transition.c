#include "common.h"
#include "callees.h"
#include "moby.h"
#include "structs.h"

/*
 * transition.cpp in the original source; text 0x1E9E70-0x1EC038.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/*
 * Close but not yet byte-matching, new evidence for the sq/lq open
 * question below: this function only saves $ra (no $s0-$s7 at all), and
 * retail STILL spills it as `sq` here -- unlike every other function
 * seen so far, where retail consistently uses `sd` for a lone $ra save.
 * This compiler always uses `sd` for $ra regardless. Logic/instructions
 * otherwise identical (return func_0022C7E0(); ... 5 calls in a row,
 * body confirmed correct via objdump before reverting this to
 * INCLUDE_ASM):
 *   func_0022C7E0(); func_0022C188(); func_0022C870();
 *   func_00234C98(0x47, 0x5360B);
 *   func_00234C98(0x4E, 0x1000000 | (D_0015EF88 >> 13));
 * Means the sq/lq choice isn't purely "s-regs vs ra", it's something
 * more granular retail decides per-function (maybe per translation
 * unit, or some other property not yet isolated). See "Open toolchain
 * questions" in docs/DECOMP_PROGRESS.md.
 */
extern void SetupSkyGifPaging(void);
extern void SkyLevelGeneric___maybe(void);
extern void DoSkyGifPaging(void);
extern void VU1_addGSregister(int, long);
extern int depth_buffer_address MACRO_ADDR;

/* func_00234C98 is (int, long) and D_0015EF88 MACRO_ADDR; the older
   sq/sd note is obsolete. */
void Transition_DrawSky(void) {
    SetupSkyGifPaging();
    SkyLevelGeneric___maybe();
    DoSkyGifPaging();
    VU1_addGSregister(0x47, 0x5360B);
    VU1_addGSregister(0x4E, 0x1000000 | (depth_buffer_address >> 13));
}

/* Level init from the level header hdr: loads the frame and fog constants, uploads the VU0
   program, unpacks the shrub and tie records (class lists, rotations, scales, colour
   averages) into the level arena, clears the ent tables and returns the next free address. */

typedef struct { int x0; int x4; unsigned char pad08[0x2C]; int x34; unsigned char pad38[4]; int x3C; } LevelHeaderE9EC8;
typedef struct { float v[3]; float w; } __attribute__((aligned(16))) FRowE9EC8;
typedef struct { float v[3]; int w; } __attribute__((aligned(16))) IRowE9EC8;
typedef struct { FRowE9EC8 m[4]; unsigned char pad40[0x100]; unsigned char x140[0x80]; } ShrubDataE9EC8;
typedef struct { IRowE9EC8 m[3]; FRowE9EC8 m3; } TieDataE9EC8;
typedef struct {
    float v[3]; float scale; void *data; float x14; short x18; unsigned char cls; unsigned char x1B;
    unsigned short x1C; unsigned short x1E;
} ShrubE9EC8;
typedef struct {
    float v[3]; float scale; float radius; unsigned char pad14[3]; unsigned char x17; short x18;
    unsigned char cls; unsigned char x1B; unsigned short x1C; unsigned short x1E;
} TieE9EC8;
typedef struct {
    unsigned char pad00[0x26]; unsigned short count; ShrubE9EC8 *first; unsigned char pad2C[4];
    float x30[3]; float scale; float x40;
} ShrubClassE9EC8;
typedef struct {
    unsigned char pad00[0xC]; float scale; unsigned char pad10[6]; unsigned short count;
    TieE9EC8 *first; float *x1C; float x20;
} TieClassE9EC8;
typedef struct { unsigned char pad00[0x36]; unsigned short x36; unsigned char pad38[2]; short x3A; unsigned char pad3C[4]; } Ent40E9EC8;
typedef struct { unsigned char pad00[0x20]; unsigned char x20; unsigned char pad21[0x15]; short x36; unsigned char pad38[0xC8]; } Ent100E9EC8;
typedef struct { unsigned char pad00[0x23C]; int x23C; int x240; int x244; } DisplayE9EC8;
typedef struct { int x0; unsigned char *x4; unsigned char *x8; int xC; int x10; int x14; } BufE9EC8;
typedef struct { int x0; unsigned char *x4; } S160E9EC8;

extern int FastMemSet();
extern void FastMemZero16(void *, int);
extern void copy_memory_aligned(void *, void *, int);
extern void STUB_printf(char *);
extern void set_video_timing(int);
extern int UpdateFog();
extern void SetBackgroundColor(int, int, int);
extern int VU0_loadMicroProgram();
extern int LightTfrags();
extern int LightShrubs();
extern int LightTies();
extern float FastVecLength(void *);
extern float max_float(float, float);
extern void matrix_mul_vec3(void *, void *, void *);
extern void scale_vector(void *, void *, float);
extern void FastVecAdd(void *, void *, void *);
extern int truncate_float_to_s32(float);
extern float IntToFloat(int);
extern void ResetDrawGlobals(void);
extern void pad_resetState(void);
extern void Font_EnableColorCodes(void);

extern int pal_mode MACRO_ADDR;
extern float frame_scale MACRO_ADDR;
extern int requested_video_mode MACRO_ADDR;
extern unsigned char *level_resource_buffers_data_end;
extern unsigned char g_hero[];
extern unsigned char gCamera[];
extern unsigned char level_start_cleared_state[];
extern BufE9EC8 coll_output;
extern unsigned char image_clear_buffer[];
extern unsigned char coll_output_aux_buffer[];
extern unsigned char moby_grids[];
extern unsigned char moby_slot_bitmap[];
extern DisplayE9EC8 view_context;
extern int fog_color_red MACRO_ADDR;
extern int fog_color_green MACRO_ADDR;
extern int fog_color_blue MACRO_ADDR;
extern float fog_near_distance MACRO_ADDR;
extern float fog_far_distance MACRO_ADDR;
extern float fog_near_intensity MACRO_ADDR;
extern float fog_far_intensity MACRO_ADDR;
extern float tfrag_draw_distance MACRO_ADDR;
extern float tie_draw_distance MACRO_ADDR;
extern float occl_fog_density_set MACRO_ADDR;
extern int occl_fog_count MACRO_ADDR;
extern int part_clip_dist MACRO_ADDR;
extern unsigned char frame_vu0_micro_program[];
extern unsigned char point_lights[];
extern unsigned char point_light_data[];
extern unsigned char dir_lights[];
extern char str_too_many_dir_lights[];
extern int tfrag_render_record_count MACRO_ADDR;
extern Ent40E9EC8 *tfrag_render_records MACRO_ADDR;
extern int tie_count MACRO_ADDR;
extern ShrubE9EC8 *ties MACRO_ADDR;
extern ShrubE9EC8 *ties_end MACRO_ADDR;
extern ShrubDataE9EC8 *tie_data_table MACRO_ADDR;
extern unsigned char object_render_class_slot_by_id[];
extern ShrubClassE9EC8 *object_render_classes[];
extern unsigned char shrub_render_class_slot_by_id[];
extern TieClassE9EC8 *shrub_render_classes[];
extern int shrub_count MACRO_ADDR;
extern TieE9EC8 *shrubs MACRO_ADDR;
extern TieE9EC8 *shrubs_end MACRO_ADDR;
extern TieDataE9EC8 *shrub_data_table MACRO_ADDR;
extern unsigned char *shrub_colors MACRO_ADDR;
extern S160E9EC8 shrub_color_dma_ref_tag;
extern Ent100E9EC8 *resident_object_pool MACRO_ADDR;
extern Ent100E9EC8 *moby_instance_end MACRO_ADDR;
extern Ent100E9EC8 *moby_instance_perm_end MACRO_ADDR;
extern unsigned char *g_moby_vars MACRO_ADDR;
extern unsigned char *part_instance_pool MACRO_ADDR;
extern int part_slot_search_start MACRO_ADDR;
extern int part_slot_high_water MACRO_ADDR;
extern int part_active_count MACRO_ADDR;
extern unsigned char part_slot_bitmap[];
extern unsigned char occl_cam_state[];
extern int world_update_time MACRO_ADDR;
extern float gFade MACRO_ADDR;

/* Level-load parser: builds the runtime object tables from the level header and returns the end pointer. */
unsigned char *initialize_level_render_state(LevelHeaderE9EC8 *hdr)
{
    unsigned char *p;
    unsigned char *c;
    unsigned char *lc;
    unsigned char *sc;
    unsigned char *tc;
    unsigned short *sp;
    unsigned short *spa;
    unsigned short *spb;
    int n;
    int i;
    int i0;
    int i1;
    int i2;
    int i3;
    int i5;
    int i6;
    int j;
    int k;
    int id;
    int cls;
    int last;
    int lastTie;
    int size0;
    int size1;
    int size2;
    int size3;
    int size4;
    int r;
    int g;
    int bl;
    int x;
    int y;
    float s;
    float a;
    float b;
    float len0;
    float len1;
    float len2;
    float rad;
    ShrubE9EC8 *sh;
    ShrubDataE9EC8 *sd;
    TieE9EC8 *tie;
    TieDataE9EC8 *td;
    Ent100E9EC8 *e;
    ShrubE9EC8 *o;
    FRowE9EC8 *r1;
    FRowE9EC8 *r2;
    FRowE9EC8 *r3;
    IRowE9EC8 *t1;
    IRowE9EC8 *t2;
    FRowE9EC8 *t3;

    p = level_resource_buffers_data_end;
    ((void (*)(void *, int, int))FastMemSet)(g_hero, 0, 0x2310);
    ((void (*)(void *, int, int))FastMemSet)(gCamera, 0, 0x3A0);
    ((void (*)(void *, int, int))FastMemSet)(level_start_cleared_state, 0, 0x180);
    if (pal_mode != 0) {
        if (frame_scale == 1.0f) {
            set_video_timing(1);
        }
    } else if (frame_scale != 1.0f) {
        set_video_timing(0);
    }
    *(unsigned char *)&requested_video_mode = *(unsigned char *)&pal_mode;
    coll_output.x4 = image_clear_buffer;
    coll_output.x8 = coll_output_aux_buffer;
    coll_output.xC = 0;
    coll_output.x10 = 0;
    coll_output.x14 = 0;
    FastMemZero16(moby_grids, 0x10000);
    FastMemZero16(moby_slot_bitmap, 0x60);

    c = (unsigned char *)hdr + hdr->x0;
    view_context.x23C = *(int *)c;
    c += 4;
    view_context.x240 = *(int *)c;
    c += 4;
    view_context.x244 = *(int *)c;
    c += 4;
    *(unsigned char *)&fog_color_red = *c;
    c += 4;
    *(unsigned char *)&fog_color_green = *c;
    c += 4;
    *(unsigned char *)&fog_color_blue = *c;
    c += 4;
    fog_near_distance = *(float *)c;
    fog_far_distance = *(float *)(c + 4);
    fog_near_intensity = *(float *)(c + 8);
    fog_far_intensity = *(float *)(c + 12);
    ((void (*)(void))UpdateFog)();
    tfrag_draw_distance = 512000.0f;
    tie_draw_distance = 720.0f;
    occl_fog_density_set = 500.0f;
    occl_fog_count = 500;
    part_clip_dist = 0x1F4000;
    SetBackgroundColor(view_context.x23C, view_context.x240, view_context.x244);
    ((void (*)(void *))VU0_loadMicroProgram)(frame_vu0_micro_program);
    FastMemZero16(point_lights, 0x100);
    FastMemZero16(point_light_data, 0x180);
    FastMemZero16(dir_lights, 0x400);

    lc = (unsigned char *)hdr + hdr->x4;
    n = *(int *)lc;
    lc += 0x10;
    if (n >= 12) {
        STUB_printf(str_too_many_dir_lights);
        n = 12;
    }
    if (n != 0) {
        copy_memory_aligned(dir_lights, lc, n << 6);
    }

    for (i0 = 0; i0 < tfrag_render_record_count; i0++) {
        tfrag_render_records[i0].x36 = 0xFFFF;
    }
    i1 = 0;
    while (i1 < tfrag_render_record_count) {
        sp = (unsigned short *)0x70003000;
        for (j = 0; j < 0x3FF && i1 < tfrag_render_record_count; j++, i1++) {
            *sp++ = (unsigned short)i1;
        }
        *sp = 0xFFFF;
        ((void (*)(void *))LightTfrags)((void *)0x70003000);
    }

    sc = (unsigned char *)hdr + hdr->x34;
    tie_count = *(int *)sc;
    sc += 0x10;
    ties = (ShrubE9EC8 *)p;
    size0 = tie_count << 5;
    p += size0;
    if (tie_count != 0) {
        ((void (*)(void *, int, int))FastMemSet)(ties, 0, size0);
    }
    size1 = tie_count * 0x1C0;
    p = (unsigned char *)(((unsigned int)p + 0x3F) & ~0x3FU);
    tie_data_table = (ShrubDataE9EC8 *)p;
    p += size1;
    if (tie_count != 0) {
        ((void (*)(void *, int, int))FastMemSet)(tie_data_table, 0, size1);
    }
    last = -1;
    ties_end = &ties[tie_count];
    for (i2 = 0; i2 < tie_count; i2++) {
        sh = &ties[i2];
        sd = &tie_data_table[i2];
        sh->cls = object_render_class_slot_by_id[*(int *)sc];
        if (sh->cls != last) {
            object_render_classes[sh->cls]->first = sh;
            object_render_classes[sh->cls]->count = 0;
            last = sh->cls;
        }
        object_render_classes[sh->cls]->count++;
        sh->data = sd;
        sh->x14 = (float)*(int *)(sc + 4);
        sh->x1E = 0xFFFF;
        sh->x1B = 0;
        sh->x1C = 0;
        id = *(int *)(sc + 0xC);
        qcopy(sd, sc + 0x10);
        r1 = &sd->m[1];
        qcopy(r1, sc + 0x20);
        r2 = &sd->m[2];
        qcopy(r2, sc + 0x30);
        r3 = &sd->m[3];
        qcopy(r3, sc + 0x40);
        sh->x18 = (short)id;
        sc += 0x50;
        sd->m[3].w = object_render_classes[sh->cls]->x40;
        matrix_mul_vec3(sh, object_render_classes[sh->cls]->x30, sd);
        s = FastVecLength(sd);
        len1 = FastVecLength(r1);
        s = max_float(s, len1);
        len2 = FastVecLength(r2);
        s = max_float(s, len2);
        sh->scale = object_render_classes[sh->cls]->scale * s;
        scale_vector(sh, sh, sd->m[3].w);
        FastVecAdd(sh, sh, r3);
        tie_data_table[i2].m[0].w = 1.0f / FastVecLength(&tie_data_table[i2].m[0]);
        tie_data_table[i2].m[1].w = 1.0f / FastVecLength(&tie_data_table[i2].m[1]);
        tie_data_table[i2].m[2].w = 1.0f / FastVecLength(&tie_data_table[i2].m[2]);
        copy_memory_aligned(sd->x140, sc, 0x80);
        sc += 0x80;
        sh->x1C = *(unsigned short *)sc;
        sc += 0x10;
    }
    spa = (unsigned short *)0x70000000;
    for (i3 = 0; i3 < tie_count; i3++) {
        *spa++ = (unsigned short)i3;
    }
    *spa = 0xFFFF;
    ((void (*)(void *))LightTies)((void *)0x70000000);

    tc = (unsigned char *)hdr + hdr->x3C;
    shrub_count = *(int *)tc;
    tc += 0x10;
    shrubs = (TieE9EC8 *)p;
    size2 = shrub_count << 5;
    p += size2;
    if (shrub_count != 0) {
        ((void (*)(void *, int, int))FastMemSet)(shrubs, 0, size2);
    }
    p = (unsigned char *)(((unsigned int)p + 0x3F) & ~0x3FU);
    shrub_data_table = (TieDataE9EC8 *)p;
    size3 = shrub_count << 6;
    p += size3;
    if (shrub_count != 0) {
        ((void (*)(void *, int, int))FastMemSet)(shrub_data_table, 0, size3);
    }
    shrub_colors = p;
    size4 = shrub_count * 0x60;
    p += size4;
    p = (unsigned char *)(((unsigned int)p + 0x3F) & ~0x3FU);
    if (shrub_count != 0) {
        ((void (*)(void *, int, int))FastMemSet)(shrub_colors, 0, size4);
    }
    lastTie = -1;
    shrub_color_dma_ref_tag.x4 = shrub_colors;
    shrubs_end = shrubs + shrub_count;

    for (i = 0; i < shrub_count; i++) {
        tie = &shrubs[i];
        td = &shrub_data_table[i];
        cls = shrub_render_class_slot_by_id[*(int *)tc];
        tie->cls = (unsigned char)cls;
        if (cls != lastTie) {
            lastTie = cls;
            shrub_render_classes[cls]->first = tie;
            shrub_render_classes[cls]->count = 0;
        }
        shrub_render_classes[cls]->count++;
        tie->x18 = (short)i;
        tie->radius = *(float *)(tc + 4);
        if (tie->radius < 16.0f) {
            tie->radius = 16.0f;
        }
        tie->x1B = 0;
        tie->x1E = 0xFFFF;
        if (shrub_render_classes[cls]->x1C != 0) {
            tie->x17 = (unsigned char)truncate_float_to_s32(*shrub_render_classes[cls]->x1C);
            rad = IntToFloat(tie->x17) + 24.0f;
            if (tie->radius < rad) {
                tie->radius = rad;
            }
        }
        qcopy(td, tc + 0x10);
        t1 = &td->m[1];
        qcopy(t1, tc + 0x20);
        t2 = &td->m[2];
        qcopy(t2, tc + 0x30);
        t3 = &td->m3;
        qcopy(t3, tc + 0x40);
        tc += 0x50;
        td->m3.w = shrub_render_classes[cls]->x20;
        len0 = FastVecLength(td);
        len1 = FastVecLength(t1);
        a = (len0 + len1) * 0.5f;
        b = FastVecLength(t2);
        x = truncate_float_to_s32(a * 4096.0f);
        y = truncate_float_to_s32(b * 4096.0f);
        if (x > 0x10000) {
            x = 0x10000;
        }
        if (y > 0x10000) {
            y = 0x10000;
        }
        td->m[2].w = (int)((unsigned int)x | ((unsigned int)y << 16));
        r = *(int *)tc;
        tc += 4;
        g = *(int *)tc;
        tc += 4;
        bl = *(int *)tc;
        tc += 8;
        g = (int)(((unsigned int)g << 8) | 0x80000000U);
        td->m[0].w = (int)(((unsigned int)bl << 16) | (unsigned int)g | (unsigned int)r);
        tie->x1C = *(unsigned short *)tc;
        tc += 0x10;
        matrix_mul_vec3(tie, shrub_render_classes[cls], td);
        s = FastVecLength(td);
        len1 = FastVecLength(t1);
        s = max_float(s, len1);
        len2 = FastVecLength(t2);
        s = max_float(s, len2);
        tie->scale = shrub_render_classes[cls]->scale * s;
        scale_vector(tie, tie, td->m3.w);
        FastVecAdd(tie, tie, t3);
    }
    spb = (unsigned short *)0x70000000;
    for (i5 = 0; i5 < shrub_count; i5++) {
        *spb++ = (unsigned short)i5;
    }
    *spb = 0xFFFF;
    ((void (*)(void *))LightShrubs)((void *)0x70000000);

    for (i6 = 0; i6 < shrub_count; i6++) {
        unsigned char *cp;
        int cr;
        int cg;
        int cb;

        cp = shrub_colors + i6 * 0x60;
        cr = 0;
        cg = 0;
        cb = 0;
        for (k = 0; k < 24; k++) {
            cr += cp[0];
            cg += cp[1];
            cb += cp[2];
            cp += 4;
        }
        cr /= 24;
        cg /= 24;
        cb /= 24;
        *(int *)((unsigned char *)&shrub_data_table[i6] + 0x1C) = (cb << 16) | (cg << 8) | cr;
    }

    resident_object_pool = (Ent100E9EC8 *)p;
    ((void (*)(void *, int, int))FastMemSet)(p, 0, 0x4000);
    p += 0x4000;
    moby_instance_end = resident_object_pool;
    *((unsigned char *)resident_object_pool + 0x20) = 0xFF;
    g_moby_vars = p;
    p += 0x2000;
    part_instance_pool = p;
    moby_instance_perm_end = (Ent100E9EC8 *)((unsigned char *)resident_object_pool + 0x3F00);
    p += 0x20000;
    part_slot_high_water = -1;
    part_slot_search_start = 0;
    part_active_count = 0;
    FastMemZero16(part_slot_bitmap, 0x200);
    for (e = resident_object_pool; e != moby_instance_perm_end; e++) {
        e->x36 = 0x7F80;
    }
    for (o = ties; o != ties_end; o++) {
        o->x18 = 0x7F80;
    }
    for (i0 = 0; i0 < tfrag_render_record_count; i0++) {
        tfrag_render_records[i0].x3A = 0x7F80;
    }
    {
        char *st = (char *)occl_cam_state;

        *(int *)(st + 0x1C) = 0;
    }
    ResetDrawGlobals();
    pad_resetState();
    Font_EnableColorCodes();
    world_update_time = 0;
    gFade = 1.0f;
    return p;
}

struct DiscFile_EABE8 {
    s32 sector;
    s32 size;
};

struct DiscTable_EABE8 {
    u8 pad_0[0x14E8];
    struct DiscFile_EABE8 unk14E8;         /* 0x14E8: the transition wad */
    u8 pad_14F0[0x38];
    struct DiscFile_EABE8 help_text;       /* 0x1528 */
};

extern struct DiscTable_EABE8 disc_table_r __asm__("D_00137C80");

typedef struct 
{
  s32 x0;
  s32 data;
  s32 x8;
  s32 xC;
  s32 x10;
  s32 x14;
  s32 n18;
  s32 x1C;
  s32 n20;
  s32 x24;
  s32 n28;
  s32 x2C;
  s32 n30;
  s32 x34;
  s32 n38;
  s32 x3C;
  s32 n40;
  s32 x44;
  s32 n48;
  s32 x4C;
  s32 x50;
  s32 x54;
  s32 x58;
  s32 x5C;
  s32 x60;
  s32 x64;
  s32 x68;
  s32 x6C;
  s32 x70;
  s32 x74;
  s32 x78;
  s32 x7C;
  s32 x80;
  s32 x84;
} WadHeader_EABE8;
typedef struct 
{
  s32 offset;
  u16 x4;
  u16 pad6;
  s32 pad8[2];
} WadTex_EABE8;
typedef struct 
{
  s32 offset;
  s32 x4;
  s32 pad8[2];
  u8 x10[0x10];
} WadClass20_EABE8;
typedef struct 
{
  s32 offset;
  s32 x4;
  s32 pad8[2];
  u8 x10[0x10];
  u8 x20[0x10];
} WadClass30_EABE8;
typedef struct 
{
  s32 offset;
  s32 size;
} WadSound_EABE8;
typedef struct 
{
  u8 pad0[0x14];
  u8 *hdr;
  s32 x18;
  s32 x1C;
} LoadState_EABE8;
typedef struct 
{
  u8 pad0[0x58];
  s32 x58;
  s32 x5C;
  s32 x60[0x46];
} SoundBanks_EABE8;
typedef struct 
{
  s32 offset;
  s32 pad[3];
} TextEntry_EABE8;
typedef struct 
{
  u8 pad0[0x2C];
  s32 count;
} HelpState_EABE8;
extern s32 g_gs_texture_allocation_cursor_r __asm__("D_0015EF74") MACRO_ADDR;
extern s32 g_gs_texture_allocation_start_r __asm__("D_0015EF78") MACRO_ADDR;
extern s32 g_gs_texture_allocation_base_r __asm__("D_0015EF8C") MACRO_ADDR;
extern s64 intro_overlay_tex0_r __asm__("D_0015F048") MACRO_ADDR;
extern s32 intro_overlay_timer_r __asm__("D_0015F058") MACRO_ADDR;
extern s32 help_msg_data_r __asm__("D_0015F060") MACRO_ADDR;
extern s32 help_offset_table_r __asm__("D_0015F064") MACRO_ADDR;
extern s32 level_texture_payload_address_r __asm__("D_0015F560") MACRO_ADDR;
extern TextEntry_EABE8 *help_msgs_r __asm__("D_0015F780") MACRO_ADDR;
extern s32 resident_moby_class_count_r __asm__("D_00160000") MACRO_ADDR;
extern s32 resident_indexed_texture_count_r __asm__("D_00160008") MACRO_ADDR;
extern s32 registered_shrub_render_class_count_r __asm__("D_001604CC") MACRO_ADDR;
extern s32 shrub_render_texture_definition_count_r __asm__("D_001604EC") MACRO_ADDR;
extern s32 tfrag_texture_definition_count_r __asm__("D_00160F94") MACRO_ADDR;
extern s32 current_vu_chain_r __asm__("D_0016100C") MACRO_ADDR;
extern s32 registered_object_render_class_count_r __asm__("D_0016104C") MACRO_ADDR;
extern s32 object_render_texture_definition_count_r __asm__("D_00161064") MACRO_ADDR;
extern u8 moby_class_472_sound_defs_r[] __asm__("D_001862E0");
extern u8 level_init_cleared_block_r[] __asm__("D_00186410");
extern SoundBanks_EABE8 render_sequence_r __asm__("D_0018CC20");
extern LoadState_EABE8 level_resource_buffers_r __asm__("D_001941C0");
extern u8 coll_output_aux_buffer_r[] __asm__("D_00194280");
extern HelpState_EABE8 g_help_state_r __asm__("D_001997D0");
extern u64 special_material_template_r[] __asm__("D_0019E7C0");
extern MobyClass *moby_class_resources_r[] __asm__("D_001B3580");
extern u8 resident_class_slot_by_id_r[] __asm__("D_001B3E40");
extern s32 resident_indexed_texture_addrs_r[] __asm__("D_001B5D00");
extern s32 resident_class_original_field_2c_r[] __asm__("D_001B6500");
extern u8 resident_class_material_maps_r[] __asm__("D_001B6C00");
extern s32 shrub_render_texture_addrs_r[] __asm__("D_001D8840");
extern s32 tfrag_texture_size_table_r[] __asm__("D_001E0C00");
extern s32 object_render_texture_addrs_r[] __asm__("D_001E2900");
extern void calculate_dma_transfer_address_r(void) __asm__("func_002348B8");
extern void FastMemSet_r(void *, s32, s32) __asm__("func_001F99B0");
extern void flush_cache_r(s32) __asm__("func_00118D80");
extern void queue_dma_transfer_r(s32) __asm__("func_001EB300");
extern void wait_for_graphics_pipeline_idle_r(s32, s32) __asm__("func_00120858");
extern s32 initialize_level_render_state_r(u8 *) __asm__("func_001E9EC8");
extern void InitViewContext_r(void) __asm__("func_001F3008");
extern void UpdateViewContext_r(void) __asm__("func_001F3140");
extern char log2dim_r(s32) __asm__("func_001F9968");
extern void InitMemSlots_r(void) __asm__("func_00201E10");
extern void ParseParticleTexs_r(u8 *, u8 *, u8 *, s32) __asm__("func_00202F00");
extern void unpack_point_records_r(u8 *, s32) __asm__("func_00203038");
extern void relocate_sky_definition_r(u8 *) __asm__("func_00203118");
extern void upload_texture_images_r(u8 *, s32, u8 *) __asm__("func_00203958");
extern void register_moby_class_r(u8 *, u8 *, u8 *, s32) __asm__("func_00203E78");
extern void register_object_render_class_r(u8 *, u8 *, u8 *, s32) __asm__("func_00203F68");
extern void register_shrub_render_class_r(u8 *, u8 *, u8 *, u8 *, s32) __asm__("func_00204340");
extern void initialize_tfrag_render_data_r(u8 *, u8 *) __asm__("func_00204918");
extern void parse_space_scene_chunk_r(s32) __asm__("func_00205220");
extern s32 fast_decompress_r(u8 *, u8 *) __asm__("func_0020C468");
extern void start_audio_stream_read_r(u8 *, s32, s32) __asm__("func_00217628");
extern unsigned int Load_r(s32, s32, s32) __asm__("func_002176C8");
extern void update_audio_stream_until_idle_r(s32) __asm__("func_00217748");
extern void VU1_initChain_r(void) __asm__("func_002348E8");
extern s32 sceGsSetDefLoadImage_r(void *, s16, s16, s16, s16, s16, s16, s16) __asm__("func_00122630");
extern s32 sceGsExecLoadImage_r(void *, u8 *) __asm__("func_00122958");

/* Loads the space-transition wad to 0x1400000 and unpacks it at the level arena: textures and
   their tables, the three kinds of moby classes, sound and help text tables, the loading image;
   then resets the sequence state and relocates the eight help text tables.
   Adapted from Lombyte's project (GPL-3.0) for PAL: src/gameplay/state/transition_load_wad.c, transition_load_wad. */
void transition_load_wad(void)
{
  u8 li[0x60];
  WadHeader_EABE8 *hdr;
  u8 *data;
  u8 *base;
  s32 size;
  s32 i;
  s32 j;
  s32 k;
  s32 cnt;
  s32 v;
  s32 *out;
  int new_var;
  WadTex_EABE8 *tex;
  s32 new_var4;
  WadClass20_EABE8 *c20;
  u8 *new_var3;
  WadClass30_EABE8 *c30;
  WadSound_EABE8 *snd;
  TextEntry_EABE8 *te;
  WadTex_EABE8 *new_var2;
  s32 k_800 = 0x800;
  s32 adj;
  s64 t;
  s64 u;
  intro_overlay_timer_r = 0;
  i = 0;
  calculate_dma_transfer_address_r();
  InitMemSlots_r();
  current_vu_chain_r = 0x100000;
  g_gs_texture_allocation_base_r = 0x2C0000;
  g_gs_texture_allocation_start_r = 0x2C0000;
  g_gs_texture_allocation_cursor_r = 0x2C0000;
  FastMemSet_r(coll_output_aux_buffer_r, 0x87654321, 0x10);
  FastMemSet_r(resident_class_slot_by_id_r, -1, k_800);
  FastMemSet_r(resident_class_material_maps_r, -1, 0xE00);
  FastMemSet_r(resident_class_original_field_2c_r, 0, 0xE0);
  InitViewContext_r();
  UpdateViewContext_r();
  VU1_initChain_r();
  start_audio_stream_read_r((u8 *)0x1400000, disc_table_r.unk14E8.sector, disc_table_r.unk14E8.size);
  update_audio_stream_until_idle_r(1);
  flush_cache_r(0);
  size = fast_decompress_r((u8 *)0x1400000, level_resource_buffers_r.hdr);
  flush_cache_r(0);
  hdr = (WadHeader_EABE8 *) level_resource_buffers_r.hdr;
  upload_texture_images_r(((u8 *) hdr) + hdr->x0, hdr->x8, ((u8 *) hdr) + hdr->xC);
  t = ((s64) ((g_gs_texture_allocation_base_r + hdr->x70) >> 8)) | 0x1D308000;
  u = (((s64) ((g_gs_texture_allocation_base_r + hdr->x74) >> 8)) << 37) | (((s64) 0xB800) << 19);
  data = ((u8 *) hdr) + hdr->data;
  base = data + hdr->x60;
  special_material_template_r[0] = (t | u) | (((s64) (-1)) << 63);
  special_material_template_r[1] = 0xFFA0000000E0;
  special_material_template_r[2] = 0x0040000400004000;
  tex = (WadTex_EABE8 *) (((u8 *) hdr) + hdr->x34);
  if ((tfrag_texture_definition_count_r = hdr->n30) > 0)
  {
    do
    {
      tfrag_texture_size_table_r[i] = (((s32) base) + tex[i].offset) + (log2dim_r(tex[i].x4) << 28);
      i++;
    }
    while (i < tfrag_texture_definition_count_r);
  }
  k = 0;
  tex = (new_var2 = (WadTex_EABE8 *) (((u8 *) hdr) + hdr->x3C));
  if ((resident_indexed_texture_count_r = hdr->n38) > 0)
  {
    do
    {
      resident_indexed_texture_addrs_r[k] = (((s32) base) + tex[k].offset) + (log2dim_r(tex[k].x4) << 28);
      k++;
    }
    while (k < resident_indexed_texture_count_r);
  }
  k = 0;
  tex = (new_var2 = (WadTex_EABE8 *) (((u8 *) hdr) + hdr->x44));
  if ((object_render_texture_definition_count_r = hdr->n40) > 0)
  {
    do
    {
      object_render_texture_addrs_r[k] = (((s32) base) + tex[k].offset) + (log2dim_r(tex[k].x4) << 28);
      k++;
    }
    while (k < object_render_texture_definition_count_r);
  }
  k = 0;
  tex = (new_var2 = (WadTex_EABE8 *) (((u8 *) hdr) + hdr->x4C));
  if ((shrub_render_texture_definition_count_r = hdr->n48) > 0)
  {
    do
    {
      shrub_render_texture_addrs_r[k] = (((s32) base) + tex[k].offset) + (log2dim_r(tex[k].x4) << 28);
      k++;
    }
    while (k < shrub_render_texture_definition_count_r);
  }
  initialize_tfrag_render_data_r(data + hdr->x10, ((u8 *) hdr) + hdr->x34);
  relocate_sky_definition_r(data + hdr->x14);
  c20 = (WadClass20_EABE8 *) (((u8 *) hdr) + hdr->x1C);
  resident_moby_class_count_r = 0;
  registered_object_render_class_count_r = 0;
  registered_shrub_render_class_count_r = 0;
  for (k = 0; k < hdr->n18; k++)
  {
    register_moby_class_r((c20->offset != 0) ? (data + c20->offset) : (0), ((u8 *) hdr) + hdr->x3C, c20->x10, c20->x4);
    c20++;
  }

  c20 = (WadClass20_EABE8 *) (((u8 *) hdr) + hdr->x24);
  for (k = 0; k < hdr->n20; k++)
  {
    register_object_render_class_r(data + c20->offset, ((u8 *) hdr) + hdr->x44, c20->x10, c20->x4);
    c20++;
  }

  c30 = (WadClass30_EABE8 *) (((u8 *) hdr) + hdr->x2C);
  for (k = 0; k < hdr->n28; k++)
  {
    register_shrub_render_class_r(data + c30->offset, ((u8 *) hdr) + hdr->x4C, c30->x10, c30->x20, c30->x4);
    c30++;
  }

  level_texture_payload_address_r = (s32) (data + hdr->x68);
  unpack_point_records_r(((u8 *) hdr) + hdr->x5C, hdr->x58);
  new_var3 = data + hdr->x64;
  new_var4 = hdr->x54;
  ParseParticleTexs_r(((u8 *) hdr) + (new_var = hdr->x6C), new_var3, ((u8 *) hdr) + new_var4, hdr->x50);
  sceGsSetDefLoadImage_r(li, (g_gs_texture_allocation_cursor_r << 8) >> 16, 4, 0, 0, 0, 0x100, 0x80);
  flush_cache_r(0);
  sceGsExecLoadImage_r(li, data + hdr->x84);
  wait_for_graphics_pipeline_idle_r(0, 0);
  level_resource_buffers_r.x18 = ((s32) level_resource_buffers_r.hdr) + size;
  v = g_gs_texture_allocation_cursor_r;
  intro_overlay_tex0_r = ((v >> 8) | 0x20010000) | (((s64) 0xB800) << 19);
  g_gs_texture_allocation_cursor_r = v + 0x20000;
  g_gs_texture_allocation_start_r = v + 0x20000;
  level_resource_buffers_r.x1C = initialize_level_render_state_r(data + hdr->x7C);
  FastMemSet_r(&render_sequence_r, 0, 0x1C0);
  FastMemSet_r(level_init_cleared_block_r, 0, 0x40);
  render_sequence_r.x58 = level_resource_buffers_r.x1C;
  render_sequence_r.x5C = (level_resource_buffers_r.x1C += 0x40000);
  level_resource_buffers_r.x1C += 0x40000;
  snd = (WadSound_EABE8 *) (data + hdr->x80);
  cnt = 0;
  if (snd->size != 0)
  {
    out = render_sequence_r.x60;
    do
    {
      cnt++;
      *out = ((s32) (data + hdr->x80)) + (snd->offset + k_800);
      snd++;
      out++;
    }
    while ((cnt < 0x46) && (snd->size != 0));
  }
  parse_space_scene_chunk_r(0);
  help_msg_data_r = level_resource_buffers_r.x1C;
  Load_r(level_resource_buffers_r.x1C, disc_table_r.help_text.sector, disc_table_r.help_text.size);
  help_offset_table_r = help_msg_data_r;
  level_resource_buffers_r.x1C = help_msg_data_r + (disc_table_r.help_text.size << 11);
  for (k = 0; k < 8; k++)
  {
    queue_dma_transfer_r(k);
    j = 0;
    if (g_help_state_r.count > 0)
    {
      te = help_msgs_r;
      adj = ((s32) te) - 8;
      {
        do
        {
          te[j].offset += adj;
          j++;
        }
        while (j < g_help_state_r.count);
      }
    }
  }

  queue_dma_transfer_r(0);
  cnt = (new_var = resident_class_slot_by_id_r[0x472]);
  if (cnt >= 0)
  {
    moby_class_resources_r[cnt]->sound_defs = (s32)moby_class_472_sound_defs_r;
    (*(u8 *)&moby_class_resources_r[cnt]->pad_D) = 5;
  }
}

extern int help_offset_table MACRO_ADDR;
extern int help_msg_data MACRO_ADDR;
extern int help_msg_count;
/* gp-relative, no retail symbol: gp 0x166D00 - 0x7580 = 0x15F780
   (cursor into the table walked below). */
extern short help_msgs;

/* Points the D_0015F780 cursor at entry arg0 of the table D_0015F064
   indexes into D_0015F060, past its 8-byte header, and keeps the
   header's first word in D_001997FC. Both table pointers are MACRO_ADDR
   (one register each), and the index goes first in the addition. */
void queue_dma_transfer(int arg0) {
    int *entry = (int *)(arg0 * 4 + help_offset_table);
    int off = *entry;
    char *p = (char *)(help_msg_data + off);

    help_msg_count = *(int *)p;
    p += 8;
    *(char **)&help_msgs = p;
}

extern char render_sequence[];
extern float view_tan_half_fov;
extern char camera_position[];
extern void UpdateViewContext(void);

/* Transition_UpdateMovieCamera: the current keyframe (0x20 bytes: position,
   a flag byte at +0xC, then X/Y/Z angles) sets the camera position and
   its orientation matrix (negated first two rows); returns the flag. */
unsigned char Transition_UpdateMovieCamera(void) {
    char *t = render_sequence;
    char *key = *(char **)(t + 0x54) + *(int *)(t + 0x38) * 32;
    unsigned char flag = key[0xC];
    float *ang = (float *)(key + 0x10);
    char *pos;
    char *cam;
    float m[16];

    view_tan_half_fov = 0.63f;
    UpdateViewContext();
    pos = camera_position;
    qcopy(pos, key);
    sceVu0UnitMatrix(m);
    sceVu0RotMatrixX(m, m, *(float *)(key + 0x10));
    sceVu0RotMatrixY(m, m, ang[1]);
    sceVu0RotMatrixZ(m, m, ang[2]);
    cam = pos - 0x140;
    *(float *)(cam + 0x350) = -m[8];
    *(float *)(cam + 0x360) = -m[0];
    *(float *)(cam + 0x370) = m[4];
    *(float *)(cam + 0x354) = -m[9];
    *(float *)(cam + 0x364) = -m[1];
    *(float *)(cam + 0x374) = m[5];
    *(float *)(cam + 0x358) = -m[10];
    *(float *)(cam + 0x368) = -m[2];
    *(float *)(cam + 0x378) = m[6];
    return flag;
}


typedef struct {
    u8 pad0[0x10];
    Vec4A position;
    u8 pad20[0x30];
    u8 current_frame;
    u8 next_frame;
    u8 pad52[2];
    f32 frame_fraction;
    u8 pad58[0x19];
    u8 cached_frame;
    u8 pad72[0xD];
    u8 update_enabled;
    u8 pad80[0x26];
    s16 class_id;
} RenderSequenceActor_EB458;

typedef struct {
    u8 pad0[0x78];
    Vec4A *animation_positions;
} RenderSequenceSidecar_EB458;

typedef struct {
    u8 pad0[0x34];
    s32 time;
    s32 frame;
    s32 sequence_frame;
    s16 end_time;
    u8 pad42[2];
    s16 count;
    u8 pad46[0x132];
    RenderSequenceActor_EB458 *actors[1];
} RenderSequenceState_EB458;

extern RenderSequenceState_EB458 render_sequence_r2 __asm__("D_0018CC20");
extern f32 gFade_r __asm__("D_0015F53C") MACRO_ADDR;
extern s32 intro_overlay_alpha_r __asm__("D_0015F050") MACRO_ADDR;
extern s32 language_intro_overlay_alpha_r __asm__("D_0015F054") MACRO_ADDR;
extern s32 g_game_mode_r __asm__("D_0015F6E8") MACRO_ADDR;
extern s32 pad0_pressed_r[] __asm__("D_0013CBE4");
extern void initialize_transfer_command_r(void) __asm__("func_00219E60");
extern void noop_callback_e_r(RenderSequenceActor_EB458 *) __asm__("func_001E9790");
extern void noop_callback_h_r(void) __asm__("func_001E97A8");
extern void noop_callback_i_r(void) __asm__("func_001E97B0");
extern void Transition_UpdateMovieCamera_r(void) __asm__("func_001EB338");
extern s32 scale_game_frame_count_r(s32) __asm__("func_001F98C0");
extern void FastVecAdd_r(Vec4A *, Vec4A *, Vec4A *) __asm__("func_001F9BD8");
extern void FastVecScale_r(Vec4A *, Vec4A *, f32) __asm__("func_001F9C30");
extern f32 FastCos_r(f32) __asm__("func_001F9F90");
extern f32 IntToFloat_r(s32) __asm__("func_001FA888");
extern void update_mode_freeze_r(void) __asm__("func_001FD3E8");
extern void parse_space_scene_chunk_r2(s32) __asm__("func_00205220");
extern void update_moby_animation_state_r(RenderSequenceActor_EB458 *) __asm__("func_0020D6D0");
extern void refresh_moby_spatial_bounds_r(RenderSequenceActor_EB458 *) __asm__("func_0020ED48");
extern void update_menu_system_r(void) __asm__("func_0021A1A0");
extern void sound_update_r(void) __asm__("func_0022DD68");

/* One frame of the space-flight sequence between levels: steps the sequence clock and its
   scene chunks, the movie camera, each actor's animation frame and interpolated position,
   then the intro overlay fades and the mode's own update (pause menu, freeze) and sound.
   Adapted from Lombyte's project (GPL-3.0) for PAL: src/textbin/fun_001eb0a8.c, update_gameplay_frame. */
void update_gameplay_frame(void) {
    Vec4A first_position;
    Vec4A next_position;
    RenderSequenceActor_EB458 *actor;
    Vec4A *animation_positions;
    s32 actor_index;
    f32 fade;

    noop_callback_i_r();
    fade = gFade_r - 0.0625f;
    render_sequence_r2.frame++;
    render_sequence_r2.time++;
    gFade_r = fade;
    if (fade < 0.0f) {
        gFade_r = 0.0f;
    }
    if (render_sequence_r2.time >= render_sequence_r2.end_time) {
        render_sequence_r2.time = 0;
        render_sequence_r2.sequence_frame = 0;
        parse_space_scene_chunk_r2(0);
    } else if (render_sequence_r2.frame >= 0x60) {
        parse_space_scene_chunk_r2(++render_sequence_r2.sequence_frame);
    }
    Transition_UpdateMovieCamera_r();
    for (actor_index = 0; actor_index < render_sequence_r2.count; actor_index++) {
        u8 frame_index;
        actor = render_sequence_r2.actors[actor_index];
        frame_index = render_sequence_r2.frame >> 1;
        actor->current_frame = render_sequence_r2.frame >> 1;
        actor->next_frame = frame_index + 1;
        update_moby_animation_state_r(actor);
        actor->frame_fraction = IntToFloat_r(render_sequence_r2.frame & 1) * 0.5f;
        animation_positions = ((RenderSequenceSidecar_EB458 *)actor)->animation_positions;
        FastVecScale_r(&first_position, &animation_positions[actor->current_frame],
                            1.0f - actor->frame_fraction);
        FastVecScale_r(&next_position, &animation_positions[actor->next_frame],
                            actor->frame_fraction);
        FastVecAdd_r(&actor->position, &first_position, &next_position);
        actor->cached_frame = 0xFF;
        refresh_moby_spatial_bounds_r(actor);
        actor->update_enabled = 0;
        if (actor->class_id == 0) {
            noop_callback_e_r(actor);
        }
    }
    noop_callback_h_r();
    if (g_game_mode_r == 0) {
        intro_overlay_timer_r++;
        if (scale_game_frame_count_r(0x1E) < intro_overlay_timer_r) {
            if (++intro_overlay_alpha_r > 0x40) {
                intro_overlay_alpha_r = 0x40;
            }
        }
        if (scale_game_frame_count_r(0x78) < intro_overlay_timer_r) {
            language_intro_overlay_alpha_r =
                (s32)(FastCos_r((intro_overlay_timer_r - scale_game_frame_count_r(0x78)) % 60 * 0.10471976f +
                                          -3.1415927f) *
                      32.0f) +
                0x60;
        }
        if (pad0_pressed_r[0] & 0x840) {
            initialize_transfer_command_r();
        }
        sound_update_r();
    } else if (g_game_mode_r == 3) {
        intro_overlay_timer_r = scale_game_frame_count_r(0x3C);
        if ((intro_overlay_alpha_r -= 0x10) < 0) {
            intro_overlay_alpha_r = 0;
        }
        if ((language_intro_overlay_alpha_r -= 0x10) < 0) {
            language_intro_overlay_alpha_r = 0;
        }
        update_menu_system_r();
        sound_update_r();
    } else if (g_game_mode_r == 4) {
        update_mode_freeze_r();
        sound_update_r();
    }
}

struct M2c_D_0016045C
{
  u8 pad_0[0x4];
  s16 unk4;
  u8 pad_6[0x2];
};
extern u8 frame_vu0_micro_program[];
extern s32 frame_size[];
extern short g_game_language;
extern short intro_overlay_tex0;
extern s32 intro_overlay_alpha MACRO_ADDR;
extern s32 language_intro_overlay_alpha MACRO_ADDR;
extern f32 gFade MACRO_ADDR;
extern s32 draw_callback_count MACRO_ADDR;
extern s32 g_game_mode MACRO_ADDR;
extern s32 font_state MACRO_ADDR;
extern struct M2c_D_0016045C * level_sky_effect_data MACRO_ADDR;
extern s32 transition_draw_particles[];
extern u8 occl_visibility[];
extern u8 shrub_scratchpad_copy[];
extern u8 tfrag_dma_block[];
extern u8 tie_light_data[];
extern s32 append_dma_tag();
extern s32 FastMemSet();
extern s32 flush_cache();
extern s32 Transition_DrawSky_r() __asm__("func_001E9E70");
extern s32 Transition_UpdateMovieCamera_r2() __asm__("func_001EB338");
extern s32 BuildViewMatrices();
extern s32 UpdateFog();
extern void ResetGsRegisters();
extern s32 SetupGifPaging();
extern s32 DoGifPaging();
extern u64 GetEffectTex();
extern s32 dispatch_callback_list_1();
extern s32 emit_rgba_draw_packet();
extern void DrawTexturedQuad();
extern s32 truncate_float_to_s32(f32);
extern s32 append_gif_transfer_packet();
extern s32 AA_BlurPass();
extern s32 draw_freeze_message_box();
extern s32 prune_moby_references();
extern s32 PatchMobyGifs();
extern s32 DrawMobys();
extern void PartProc();
extern void render_level_effects_and_screen_sprites();
extern s32 patch_shrub_gifs();
extern void DrawShrubs();
extern s32 LightShrubs();
extern s32 PatchTfragGifs();
extern void DrawTfrag();
extern s32 VU0_loadMicroProgram();
extern s32 VU1_syncChain();
extern s32 VU1_addGSregister_r() __asm__("func_00234C98");
extern void vu1_gs_regs_alt();
extern s32 LightTfrags();
extern s32 PatchTieGifs();
extern void DrawTies_1();
extern s32 LightTies();
void Transition_DefaultDraw(s32 *arg0);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/gameplay/state/transition_default_draw.c, transition_default_draw. */
void Transition_DefaultDraw(s32 *arg0)
{
    s32 n;

    if (level_sky_effect_data == 0 || level_sky_effect_data->unk4 != 0) {
        append_gif_transfer_packet();
    }
    FastMemSet(occl_visibility, -1, 0x80);
    Transition_UpdateMovieCamera_r2();
    BuildViewMatrices();
    prune_moby_references();
    ResetGsRegisters();
    font_state = -1;
    if (level_sky_effect_data != 0) {
        Transition_DrawSky_r();
    }
    DrawTfrag();
    append_dma_tag(0x02010000);
    DrawTies_1();
    append_dma_tag(0x02020000);
    DrawShrubs();
    append_dma_tag(0x02040000);
    if (g_game_mode == 3) {
        render_level_effects_and_screen_sprites();
    } else {
        DrawMobys();
    }
    append_dma_tag(0x02080000);
    SetupGifPaging(0);
    vu1_gs_regs_alt();
    if (draw_callback_count != 0) {
        dispatch_callback_list_1();
    }
    vu1_gs_regs_alt();
    if (transition_draw_particles[0] != 0) {
        VU1_addGSregister_r(8, 5);
        vu1_gs_regs_alt();
        flush_cache(0);
        PartProc();
        font_state = 8;
    }
    AA_BlurPass();
    ResetGsRegisters();
    if (intro_overlay_alpha != 0) {
        DrawTexturedQuad(0xEC, 0x10, 0x100, 0x80, 0, 0, 0x100, 0x80,
                      (long)(intro_overlay_alpha << 24 | 0x808080), (*(s64 *)&intro_overlay_tex0));
    }
    if (language_intro_overlay_alpha != 0) {
        n = (*(s32 *)&g_game_language) - 1;
        if (n < 0) {
            n = 0;
        }
        DrawTexturedQuad(0xA0, frame_size[0] - 0x50, 0xC0, 0x60, 0, 0, 0x100, 0x80,
                      (long)(language_intro_overlay_alpha << 24 | 0x808080), GetEffectTex(n + 4));
    }
    DoGifPaging();
    if (gFade > 0.0f) {
        if (gFade > 1.0f) {
            gFade = 1.0f;
        }
        emit_rgba_draw_packet(0, 0, 0, truncate_float_to_s32(gFade * 128.0f));
    }
    VU0_loadMicroProgram(frame_vu0_micro_program);
    flush_cache(0);
    if (g_game_mode == 4) {
        draw_freeze_message_box();
    }
    VU1_syncChain(2);
    LightTfrags(tfrag_dma_block);
    PatchTfragGifs();
    VU1_syncChain(4);
    LightTies(tie_light_data);
    PatchTieGifs();
    VU1_syncChain(8);
    LightShrubs(shrub_scratchpad_copy);
    patch_shrub_gifs();
    VU1_syncChain(0x10);
    PatchMobyGifs();
    UpdateFog();
}

extern char voice_pool[];
extern int bank_sound_count;

extern int bank_sound_count_m __asm__("D_0015F694") MACRO_ADDR;

/* The older register residual was the split load of D_0015F694; read
   through a MACRO_ADDR alias it is retail's one-register load. */
int is_active_state_entry(int arg0, int arg1) {
    if (arg0 >= 0) {
        char *p = voice_pool + arg0 * 0x70;
        if (*(short *)(p + 0x7E) == arg1 + bank_sound_count_m) {
            unsigned char s = p[0x74];
            if (s == 1 || s == 2) {
                return 1;
            }
        }
    }
    return 0;
}

typedef struct {
    s32 off;
    s32 size;
} WadChunk_BB48;

typedef struct {
    char pad0[0x20];
    WadChunk_BB48 chunk[6];
} WadHeader_BB48;

typedef struct {
    s32 sector;
    s32 size;
} DiscFile_BB48;

typedef struct {
    char pad0[0x14F8];
    DiscFile_BB48 wad_chunks[6];
} DiscTable_BB48;

typedef struct {
    s32 start;
    s32 end;
    s32 sound;
    s32 voice;
} SoundCue_BB48;

typedef struct {
    char pad0[0x34];
    s32 time;
    s32 pad38;
    s32 f3C;
} TransferState_BB48;

extern s32 current_level_BB48 __asm__("D_0015EE84") MACRO_ADDR;
extern s32 saved_EEE8_BB48 __asm__("D_0015EEE8") MACRO_ADDR;
extern WadHeader_BB48 *volatile wad_buf_BB48[] __asm__("D_001941C8");
extern DiscTable_BB48 disc_table_BB48 __asm__("D_00137C80");
extern WadHeader_BB48 *level_root_BB48 __asm__("D_0015EF4C") MACRO_ADDR;
extern s32 space_load_pending_BB48 __asm__("D_0015F690") MACRO_ADDR;
extern s32 space_load_in_progress_BB48 __asm__("D_0015F6FC") MACRO_ADDR;
extern s32 dma_index_BB48 __asm__("D_0015EE88") MACRO_ADDR;
extern SoundCue_BB48 sound_cues_BB48[] __asm__("D_001863B0");
extern TransferState_BB48 transfer_BB48 __asm__("D_0018CC20");
extern s32 game_mode_BB48 __asm__("D_0015F6E8") MACRO_ADDR;
extern f32 sequence_BB48 __asm__("D_0015F53C") MACRO_ADDR;
extern s32 intro_alpha_BB48 __asm__("D_0015F050") MACRO_ADDR;
extern s32 language_alpha_BB48 __asm__("D_0015F054") MACRO_ADDR;
extern s32 intro_timer_BB48 __asm__("D_0015F058") MACRO_ADDR;
extern u8 requested_video_mode_BB48 __asm__("D_0016044C") MACRO_ADDR;
extern s32 pal_mode_BB48 __asm__("D_0015EE80") MACRO_ADDR;
extern s32 tex_start_BB48 __asm__("D_0015EF78") MACRO_ADDR;
extern s32 tex_cursor_BB48 __asm__("D_0015EF74") MACRO_ADDR;
extern volatile s32 tex_base_BB48 __asm__("D_0015EF8C") MACRO_ADDR;
extern s32 frame_counter_BB48 __asm__("D_0015F538") MACRO_ADDR;

extern void load_and_init_BB48(void) __asm__("func_00209DC0");
extern void transition_load_wad_BB48(void) __asm__("func_001EABE8");
extern s32 load_BB48(u8 *dst, s32 sector, s32 size) __asm__("func_002176C8");
extern void init_resource_BB48(void) __asm__("func_001FFA90");
extern void init_sif_rpc_BB48(void) __asm__("func_00233FF8");
extern void init_stash_BB48(void) __asm__("func_00234018");
extern void init_hud_BB48(void) __asm__("func_001FF6B8");
extern void load_hud_banks_BB48(void) __asm__("func_002032D0");
extern s32 sync_v_BB48(s32) __asm__("func_00122598");
extern s32 scale_frames_BB48(s32) __asm__("func_001F98C0");
extern s32 snd_reset_BB48(void) __asm__("func_0012EC40");
extern s32 snd_flush_BB48(void) __asm__("func_0012DDC0");
extern void snd_reenable_command_flush_r(void) __asm__("func_0012EC30");
extern void vu1_send_chain_BB48(void) __asm__("func_002349B8");
extern void swap_chain_BB48(void) __asm__("func_00234948");
extern void put_draw_small_BB48(void) __asm__("func_001FB598");
extern void append_palette_BB48(void) __asm__("func_001FB8A8");
extern void put_draw_large_BB48(void) __asm__("func_001FB498");
extern s32 memcard_update_BB48(void) __asm__("func_00209E68");
extern s32 run_state_handler_BB48(void) __asm__("func_00209070");
extern void update_pad_BB48(void) __asm__("func_00218908");
extern void queue_dma_BB48(s32) __asm__("func_001EB300");
extern s32 is_active_BB48(s32, s32) __asm__("func_001EBAF0");
extern s32 alloc_voice_BB48(s32, s32, s32) __asm__("func_0022EEB8");
extern void release_voice_BB48(s32) __asm__("func_0022EAB0");
extern void update_gameplay_BB48(void) __asm__("func_001EB458");
extern void default_draw_BB48(void) __asm__("func_001EB7C0");
extern void start_level_BB48(s32) __asm__("func_001E9808");
extern void parse_space_scene_BB48(s32) __asm__("func_00205220");
extern void vu1_init_chain_BB48(void) __asm__("func_002348E8");
extern void fade_to_black_BB48(s32) __asm__("func_001F4E08");
extern s32 reset_vif1_gif_BB48(void) __asm__("func_001207B8");
extern void reset_graph_BB48(s16, s16, s16, s16) __asm__("func_00121B78");
extern void set_pal_mode_BB48(void) __asm__("func_001F3890");
extern void vu1_sync_chain_BB48(s32) __asm__("func_00234AC8");

/* Adapted from Lombyte's project (GPL-3.0): src/gameplay/state/transition_do_transition.c, transition_do_transition. */
void transition_do_transition(void) {
    WadHeader_BB48 *hdr;
    u8 *p;
    s32 n;
    s32 level;
    s32 wait;
    s32 i;
    s32 old;
    s32 saved;
    s32 first_size;
    u32 alignment_mask = 0xFFFFFFF0;
    DiscTable_BB48 *tbl;

    current_level_BB48 = 0;
    *(volatile u32 *)0x10000010 = 0x83;
    *(volatile u32 *)0x10000000 = 0;
    saved = saved_EEE8_BB48;
    load_and_init_BB48();
    saved_EEE8_BB48 = saved;
    transition_load_wad_BB48();
    tbl = &disc_table_BB48;
    p = (u8 *)wad_buf_BB48[0];
    hdr = (WadHeader_BB48 *)p;
    p = (u8 *)hdr + 0x60;
    level_root_BB48 = hdr;
    first_size = load_BB48(p, ((volatile DiscFile_BB48 *)&tbl->wad_chunks[1])->sector, ((volatile DiscFile_BB48 *)&tbl->wad_chunks[1])->size);
    hdr->chunk[1].size = first_size;
    hdr->chunk[1].off = p - (u8 *)hdr;
    p += ((u32)first_size + 0xF) & alignment_mask;
    hdr->chunk[2].size = load_BB48(p, tbl->wad_chunks[2].sector, tbl->wad_chunks[2].size);
    hdr->chunk[2].off = p - (u8 *)hdr;
    p += ((u32)hdr->chunk[2].size + 0xF) & alignment_mask;
    hdr->chunk[3].size = load_BB48(p, tbl->wad_chunks[3].sector, tbl->wad_chunks[3].size);
    hdr->chunk[3].off = p - (u8 *)hdr;
    p += ((u32)hdr->chunk[3].size + 0xF) & alignment_mask;
    hdr->chunk[4].size = load_BB48(p, tbl->wad_chunks[4].sector, tbl->wad_chunks[4].size);
    hdr->chunk[4].off = p - (u8 *)hdr;
    p += ((u32)hdr->chunk[4].size + 0xF) & alignment_mask;
    hdr->chunk[5].size = load_BB48(p, tbl->wad_chunks[5].sector, tbl->wad_chunks[5].size);
    hdr->chunk[5].off = p - (u8 *)hdr;
    p += ((u32)hdr->chunk[5].size + 0xF) & alignment_mask;
    hdr->chunk[0].size = load_BB48(p, tbl->wad_chunks[0].sector, tbl->wad_chunks[0].size);
    hdr->chunk[0].off = p - (u8 *)hdr;
    init_resource_BB48();
    init_sif_rpc_BB48();
    init_stash_BB48();
    init_hud_BB48();
    load_hud_banks_BB48();
    for (n = *(volatile u32 *)0x10000000 / 0x109; n < scale_frames_BB48(0xB4); n++) {
        sync_v_BB48(0);
    }
    snd_reset_BB48();
    while (snd_flush_BB48() != 0) {
    }
    snd_reenable_command_flush_r();
    level = 1;
    wait = 0;
    while (space_load_pending_BB48 == 0) {
        vu1_send_chain_BB48();
        swap_chain_BB48();
        put_draw_small_BB48();
        append_palette_BB48();
        put_draw_large_BB48();
        memcard_update_BB48();
        run_state_handler_BB48();
        update_pad_BB48();
        queue_dma_BB48(dma_index_BB48);
        space_load_in_progress_BB48 = 0;
        for (i = 0; sound_cues_BB48[i].start != -1; i++) {
            if (sound_cues_BB48[i].end == -1) {
                if (transfer_BB48.time < sound_cues_BB48[i].start) {
                    sound_cues_BB48[i].voice = -1;
                    continue;
                }
                if (sound_cues_BB48[i].voice != -1) {
                    continue;
                }
            } else if (transfer_BB48.time < sound_cues_BB48[i].start ||
                       sound_cues_BB48[i].end < transfer_BB48.time) {
                goto stop;
            } else if (is_active_BB48(sound_cues_BB48[i].voice, sound_cues_BB48[i].sound) != 0) {
                continue;
            }
            sound_cues_BB48[i].voice = alloc_voice_BB48(sound_cues_BB48[i].sound, 0, 0);
            continue;
        stop:
            if (is_active_BB48(sound_cues_BB48[i].voice, sound_cues_BB48[i].sound) != 0) {
                release_voice_BB48(sound_cues_BB48[i].voice);
            }
            sound_cues_BB48[i].voice = -1;
        }
        update_gameplay_BB48();
        default_draw_BB48();
        if (game_mode_BB48 != 0) {
            wait = 0;
        } else if (++wait >= scale_frames_BB48(0x5DC)) {
            start_level_BB48(level);
            wait = 0;
            level = (level + 1) % 4;
            if (level == 0) {
                level++;
            }
            sequence_BB48 = 1.0f;
            transfer_BB48.time = 0;
            transfer_BB48.f3C = 0;
            parse_space_scene_BB48(0);
            intro_alpha_BB48 = 0;
            language_alpha_BB48 = 0;
            intro_timer_BB48 = 0;
            vu1_init_chain_BB48();
            continue;
        }
        if (requested_video_mode_BB48 != pal_mode_BB48) {
            pal_mode_BB48 = pal_mode_BB48 == 0;
            fade_to_black_BB48(4);
            reset_vif1_gif_BB48();
            reset_graph_BB48(0, 1, pal_mode_BB48 != 0 ? 3 : 2, 0);
            old = tex_start_BB48;
            sequence_BB48 = 1.0f;
            set_pal_mode_BB48();
            tex_start_BB48 = old;
        }
        vu1_sync_chain_BB48(1);
        sync_v_BB48(0);
        frame_counter_BB48++;
    }
    if (pal_mode_BB48 == 0) {
        tex_base_BB48 = 0x280000;
    }
    tex_cursor_BB48 = tex_start_BB48 = tex_base_BB48;
}

/* Not a standalone function: single `addiu $sp,$sp,0x30`, no `jr $31` --
   fallthrough fragment, same category as func_00113AD8 in core_text. */
LINKER_REMNANT("asm/remnants/text", func_001EC030);
