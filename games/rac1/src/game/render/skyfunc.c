#include "common.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"
#include "game/render/draw.h"
#include "game/render/skyfunc.h"
#include "game/render/vuchain.h"
#include "core/libc/printf_rand.h"
#include "data/executable.h"
#include "game/math/fastfunc.h"
#include "game/moby/mobyutil.h"
#include "asm_functions/executable.h"

/*
 * skyfunc.cpp in the original source; text 0x22BEB0-0x22CEB8.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

LINKER_REMNANT("asm/remnants/text", func_0022BEB0);

/* Sky shell draw loop (a variant of draw_sky_shells): shell 0 re-reads the
   base transform; shells 1-5 scroll their float by sky_shell_scroll_step times a
   per-shell rate, get a per-shell spin and scale, rebuild the sky transform
   from sky_matrix, scale its three rows, copy sky_shell_translation into the
   translation, and every shell is drawn by SkyDrawShell. Each case does
   its own add and store, and the shell table is indexed through its own
   pointer, which gives retail's base-first address add. */
extern char *level_sky_effect_data_r __asm__("D_0016055C") MACRO_ADDR;
extern f32 D_00160500_2BEB0 SDATA(D_00160500);
extern u8 sky_matrix_r[] __asm__("D_001D9A70");
extern u8 sky_shell_translation_r[] __asm__("D_00160560") MACRO_ADDR;

extern void SkyDrawShell_r(s32) __asm__("func_0022C9A8");

/* Draw the sky shells, each layer scrolling and rotating at its own rate. */
void DrawSkyShellsScrolling(void) {
    f32 rot[4];
    f32 scale;
    f32 *p;
    f32 d;
    u8 *t;
    u8 **tab;
    s32 i;

    *(s16 *)(level_sky_effect_data_r + 4) = 0;
    set_identity_matrix(sky_matrix_r);
    clear_u64_value(rot);
    for (i = 0; i < *(s16 *)(level_sky_effect_data_r + 6); i++) {
        tab = (u8 **)(level_sky_effect_data_r + 0x20);
        p = (f32 *)(tab[i] + 8);
        scale = 1.0f;
        switch (i) {
            case 1:
                rot[1] = 0.02f;
                scale = 3.0f;
                *p += D_00160500_2BEB0 * 0.55f;
                break;
            case 2:
                rot[1] = -0.02f;
                scale = 2.5f;
                *p += D_00160500_2BEB0 * 0.6f;
                break;
            case 3:
                rot[1] = 0.01f;
                scale = 2.0f;
                *p += D_00160500_2BEB0 * 0.7f;
                break;
            case 4:
                rot[1] = -0.01f;
                scale = 1.5f;
                *p += D_00160500_2BEB0 * 0.75f;
                break;
            case 5:
                scale = 1.25f;
                *p += D_00160500_2BEB0 * 0.8f;
                break;
            default:
                set_identity_matrix(sky_matrix_r);
                break;
        }
        if (i > 0) {
            f32 v = (f32)i * 2000.0f;
            s32 n = (s32)(*p + v);
            rot[2] = (f32)(n % 10000) * 0.00062831853f - 3.1415927f;
            euler_to_matrix_ee(sky_matrix_r, rot);
            t = sky_matrix_r;
            scale_vector(t, t, scale);
            scale_vector(t + 0x10, t + 0x10, scale);
            scale_vector(t + 0x20, t + 0x20, scale);
            qcopy(t + 0x30, sky_shell_translation_r);
        }
        SkyDrawShell_r(i);
    }
}

struct LevelSkyEffectData {
    u8 pad_0[4];
    s16 relocation_state;
    u8 pad_6[2];
    s16 effect_count;
    u8 pad_A[0x12];
    struct SkyEffect *effects;
};
union SkyEffectColorOrAngles {
    struct SkyAngles {
        s16 azimuth;
        u16 elevation;
    } angles;
    u32 base_color;
};
struct SkyEffect {
    u8 randomize_color;
    u8 pad1[1];
    u8 texture_index;
    u8 flags;
    u32 color;
    f32 angle;
    union SkyEffectColorOrAngles state;
    f32 position_x;
    f32 position_y;
    f32 position_z;
    f32 size;
};
extern struct LevelSkyEffectData *level_sky_effect_data_r2 __asm__("D_0016055C") MACRO_ADDR;
extern u8 sky_matrix[];

extern void SkyDrawShell(s32);

extern void VU1_addGSregister_r(s32, u64) __asm__("func_00234C98");

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/rendering/sky/update_sky_effects.c, update_sky_effects. */
void SkyLevelGeneric___maybe(void) {
    f32 trig_product;
    f32 elevation;
    f32 azimuth;
    s32 color_delta;
    u32 base_color;
    s32 initialization_index;
    s32 effect_index;
    s32 alpha_delta;
    s32 random_bits;
    s32 random_color_enabled;
    s32 effect_flags;
    f32 radius;
    struct SkyEffect *effect;
    struct SkyAngles *orbit;
    s16 *angles;

    level_sky_effect_data_r2->relocation_state = 0;
    set_identity_matrix(sky_matrix);
    SkyDrawShell(0);
    SkyDrawShell(1);
    if (level_sky_effect_data_r2->effect_count == 0) {
        level_sky_effect_data_r2->effect_count = 0x100;
        srand(0x3039);
        for (initialization_index = 0; initialization_index < level_sky_effect_data_r2->effect_count;
             initialization_index++) {
            radius = 50.0f;
            random_color_enabled = 1;
            effect_flags = 0x48;
            base_color = 0x30505050;
            effect = &level_sky_effect_data_r2->effects[initialization_index];
            if (initialization_index >= 0xF6) {
                effect->randomize_color = 0;
                angles = &effect->state.angles.azimuth;
                effect->state.angles.azimuth = (s16)(func_001160D8() >> 0x10);
                angles[1] = (s16)(func_001160D8() >> 0x10);
                /* Retail stores the float bits 0x3E23D70A. */
                effect->size = 0.16f;
                effect->flags = effect_flags;
                effect->texture_index = random_color_enabled;
            } else {
                effect->randomize_color = random_color_enabled;
                color_delta = random_integer_below(0x100);
                effect->texture_index = random_color_enabled;
                effect->flags = effect_flags;
                effect->state.angles.azimuth = color_delta;
                effect->angle = random_angle_radians();
                effect->size = IntToFloat(random_integer_below(0x18) + 0x20) * 0.00390625f;
                azimuth = FastAddRots(-3.0f, random_angle_radians() * 0.2f);
                elevation = random_angle_radians();
                elevation = elevation * 0.09f;
                elevation = elevation + 1.2f;
                trig_product = FastCos(azimuth);
                trig_product = trig_product * FastSin(elevation);
                trig_product = trig_product * radius;
                effect->position_x = trig_product;
                trig_product = FastSin(azimuth);
                trig_product = trig_product * FastSin(elevation);
                trig_product = trig_product * radius;
                effect->position_y = trig_product;
                effect->position_z = FastCos(elevation) * radius;
                color_delta = random_integer_below(0x18);
                alpha_delta = random_integer_below(0x20) << 0x18;
                if ((func_001160D8() >> 0x10) & 1) {
                    effect->state.base_color = alpha_delta + ((color_delta << 0x10) + base_color);
                } else {
                    effect->state.base_color = (alpha_delta + ((color_delta << 8) + base_color)) | color_delta;
                }
            }
        }
    }
    for (effect_index = 0; effect_index < level_sky_effect_data_r2->effect_count; effect_index++) {
        effect = &level_sky_effect_data_r2->effects[effect_index];
        if (effect->randomize_color == 0) {
            orbit = &effect->state.angles;
            angles = &effect->state.angles.azimuth;
            effect->state.angles.azimuth = (s16)(effect->state.angles.azimuth + 1);
            angles[1] = (u16)(angles[1] + 1);
            azimuth = IntToFloat((orbit->azimuth & 0xFFF) - 0x800) * 0.0015339808f;
            elevation = IntToFloat((angles[1] & 0xFFF) - 0x800) * 0.0015339808f;
            trig_product = FastCos(azimuth);
            trig_product = trig_product * FastSin(elevation);
            trig_product = trig_product * 50.0f;
            effect->position_x = trig_product;
            trig_product = FastSin(azimuth);
            trig_product = trig_product * FastSin(elevation);
            trig_product = trig_product * 50.0f;
            effect->position_y = trig_product;
            effect->position_z = absolute_float(FastCos(elevation)) * 50.0f;
            if ((u32)(orbit->azimuth & 0x3F) < 8U) {
                effect->color = 0x702020F0;
            } else {
                effect->color = 0x202020F0;
            }
        } else {
            u32 color_value;
            u32 color_offset;
            random_bits = func_001160D8() >> 0x10;
            color_offset = ((random_bits & 0x1F00) << 0xA) + 0xFFDFDFE0;
            color_value = effect->state.base_color + color_offset;
            color_value += (random_bits & 0x1F0) << 6;
            color_value += (random_bits & 0x1F) << 2;
            effect->color = color_value;
        }
    }
    SkySpriteProc();
    VU1_addGSregister_r(0x42, (0x8000ULL << 0x18) | 0x44);
    SkyDrawShell(2);
    SkyDrawShell(3);
}

struct SkyShellSet {
    u8 pad_0[4];
    u16 relocation_state;
    s16 shell_count;
};
struct SkyTransform {
    u8 pad[0x30];
    u64 translation;
};
extern f32 D_00160504_2C5A0 SDATA(D_00160504);
extern struct SkyShellSet *level_sky_effect_data_r3 __asm__("D_0016055C") MACRO_ADDR;
extern u8 sky_shell_translation_r2[] __asm__("D_00160560") MACRO_ADDR;
extern struct SkyTransform sky_matrix_r2 __asm__("D_001D9A70");
extern s32 depth_buffer_address MACRO_ADDR;

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/rendering/sky/draw_sky_shells.c, draw_sky_shells. */
void draw_sky_shells(void) {
    f32 rotation_angles[4];
    f32 shell_scale;
    u8 *transform_row;
    s32 shell_index;

    shell_index = 0;
    SetupSkyGifPaging();
    level_sky_effect_data_r3->relocation_state = 0;
    set_identity_matrix(&sky_matrix_r2);
    clear_u64_value(rotation_angles);
    if (level_sky_effect_data_r3->shell_count > 0) {
        do {
            shell_scale = 1.0f;
            switch (shell_index) {
                case 0:
                    *(s32 *)&rotation_angles[1] = 0;
                    rotation_angles[2] = D_00160504_2C5A0;
                    /* fall through */
                case 1:
                    *(s32 *)&rotation_angles[1] = 0;
                    rotation_angles[2] = FastAddRots(D_00160504_2C5A0, rotation_angles[1]);
                    shell_scale = 1.0f;
                    break;
                case 2:
                    rotation_angles[1] = -0.075f;
                    rotation_angles[2] = FastAddRots(D_00160504_2C5A0, -0.15f);
                    shell_scale = 1.25f;
                    break;
                case 3:
                    rotation_angles[1] = 0.05f;
                    rotation_angles[2] = FastAddRots(D_00160504_2C5A0, 0.125f);
                    shell_scale = 1.5f;
                    break;
                case 4:
                    rotation_angles[1] = 0.1f;
                    rotation_angles[2] = FastAddRots(D_00160504_2C5A0, -0.05f);
                    shell_scale = 1.75f;
                    break;
                case 5:
                    rotation_angles[1] = -0.15f;
                    rotation_angles[2] = FastAddRots(D_00160504_2C5A0, 0.1f);
                    shell_scale = 2.0f;
                    break;
            }
            euler_to_matrix_ee(&sky_matrix_r2, rotation_angles);
            transform_row = (u8 *)&sky_matrix_r2;
            scale_vector(transform_row, transform_row, shell_scale);
            scale_vector(transform_row + 0x10, transform_row + 0x10, shell_scale);
            scale_vector(transform_row + 0x20, transform_row + 0x20, shell_scale);
            transform_row += 0x30;
            qcopy(transform_row, sky_shell_translation_r2);
            SkyDrawShell(shell_index);
            shell_index++;
        } while (shell_index < level_sky_effect_data_r3->shell_count);
    }
    DoSkyGifPaging();
    VU1_addGSregister(0x47, 0x5360B);
    VU1_addGSregister(0x4E, 0x1000000 | (depth_buffer_address >> 13));
}

extern int g_render_packet_cursor MACRO_ADDR;
extern int sky_gif_paging_tag MACRO_ADDR;
extern int g_gs_texture_allocation_start MACRO_ADDR;
extern int g_gs_texture_allocation_cursor MACRO_ADDR;
extern int pending_texture_upload_count MACRO_ADDR;
extern char sky_textures_label[];

typedef struct {
    long tag;
    long unk_08;
} SkyGifPage;
typedef struct {
    char unk_00[6];
    short count; /* 0x6: shells */
    char unk_08[4];
    short npages; /* 0xC: GIF pages */
    short unk_0E;
    SkyGifPage *pages; /* 0x10 */
    char unk_14[0xC];
    void *shells[1]; /* 0x20 */
} SkyDef;
extern SkyDef *level_sky_effect_data MACRO_ADDR;

/*
 * SetupSkyGifPaging(void): reserves 0x10 bytes of the VU1 chain at
 * sky_gif_paging_tag and clears the tag of each of the sky's GIF pages. Retail
 * loads the page count before the two global stores: read as a struct
 * field through a pointer it cannot alias a scalar global (gcc's
 * fixed_scalar_and_varying_struct_p), where a cast access would keep it
 * behind them.
 */
void SetupSkyGifPaging(void) {
    int p;
    SkyDef *s;
    int i;

    p = g_render_packet_cursor;
    sky_gif_paging_tag = p;
    p += 0x10;
    g_render_packet_cursor = p;
    noop_core_callback_secondary(sky_textures_label, 1);
    s = level_sky_effect_data;
    g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
    pending_texture_upload_count = 0;
    for (i = 0; i < s->npages; i++) {
        s->pages[i].tag = 0;
    }
}

extern int *g_render_packet_cursor_p __asm__("D_00161000") MACRO_ADDR;
extern int *sky_gif_paging_tag_p __asm__("D_00160570") MACRO_ADDR;
extern int *sky_gif_paging_bypass_tag MACRO_ADDR;

/* DoSkyGifPaging: DoGifPaging's twin (DoGifPaging) for the sky. Pushes
   two 4-word GIF tags (0x20000000 in the first word) onto the g_render_packet_cursor
   packet, sky_gif_paging_bypass_tag marking where it started and sky_gif_paging_tag's tag
   pointing at the second; in between, when draw_config[1] is set,
   flush_pending_texture_uploads and VU1_texFlush add their own. Then restores
   g_gs_texture_allocation_cursor from g_gs_texture_allocation_start. The first advance goes through a local
   advanced in place, which keeps the old and new pointer in one register
   as retail does. */
void DoSkyGifPaging(void) {
    int *p = g_render_packet_cursor_p;

    sky_gif_paging_bypass_tag = p;
    p += 4;
    g_render_packet_cursor_p = p;
    sky_gif_paging_tag_p[0] = 0x20000000;
    sky_gif_paging_tag_p[1] = (int)g_render_packet_cursor_p;
    sky_gif_paging_tag_p[2] = 0;
    sky_gif_paging_tag_p[3] = 0;
    if (draw_config[1] != 0) {
        flush_pending_texture_uploads();
        VU1_texFlush();
    }
    g_render_packet_cursor_p[0] = 0x20000000;
    g_render_packet_cursor_p[1] = (int)(sky_gif_paging_tag_p + 4);
    g_render_packet_cursor_p[2] = 0;
    g_render_packet_cursor_p[3] = 0;
    g_render_packet_cursor_p += 4;
    sky_gif_paging_bypass_tag[0] = 0x20000000;
    sky_gif_paging_bypass_tag[1] = (int)g_render_packet_cursor_p;
    sky_gif_paging_bypass_tag[2] = 0;
    sky_gif_paging_bypass_tag[3] = 0;
    g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

LINKER_REMNANT("asm/remnants/text", func_0022C9A0);

/* Minimal layout for one sky shell: only the flag at +4 is read here. */
struct SkyShell {
    u8 pad_0[4];
    s32 gouraud_enabled; /* 0x04: nonzero draws the Gouraud path */
};

/* SkyDrawShell(int) */
void SkyDrawShell(s32 shell_index) {
    SkyDef *def = level_sky_effect_data;

    if (shell_index < def->count) {
        struct SkyShell *shell = def->shells[shell_index];
        if (shell->gouraud_enabled != 0) {
            SkyDrawShellGouraud(shell);
        } else {
            SkyDrawShellTextured(shell);
        }
    }
}

typedef u32 u128_2CA00 __attribute__((mode(TI), aligned(16)));
struct DmaTag {
    u32 tag;
    u32 addr;
    u32 vif0;
    u32 vif1;
};
struct GifTag_2CA00;
union PacketCursor {
    struct DmaTag *tag;
    struct GifTag_2CA00 *gif;
    s32 *words;
    u8 *bytes;
    s32 addr;
};
extern union PacketCursor g_render_packet_cursor_r __asm__("D_00161000") MACRO_ADDR;
struct SkyTile {
    u128_2CA00 bounds; /* 16-byte aligned: tile fields are addressed from the tile base */
    s32 address;
    s16 unk4;
    s16 unk6;
    s16 unk8;
    s16 unkA;
    s16 unkC;
    s16 unkE;
};
struct Shell {
    s32 count;
    u8 pad4[0xC];
    struct SkyTile tiles[1];
};
extern s32 sky_tile_spr_buffers SDATA(sky_tile_spr_buffers);
extern s32 D_00160510_2CA00 SDATA(D_00160510);
extern u8 sky_textured_gs_block[];

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/rendering/sky/sky_draw_shell_textured.c, sky_draw_shell_textured. */
void func_0022CA00_r(struct Shell *shell) __asm__("func_0022CA00");
/* SkyDrawShellTextured */
void func_0022CA00_r(struct Shell *shell) {
    u8 visibility[shell->count];
    s32 i;
    s32 dest;
    s32 c_dest;
    s32 a_dest;
    s32 next;

    if (shell->count == 0) {
        return;
    }

    sky_bsphere_check(shell->tiles, shell->count, visibility);

    g_render_packet_cursor_r.words[0] = 0x30000007;
    *(s32 *)((u32)g_render_packet_cursor_r.words + 4) = (s32)sky_textured_gs_block;
    *(s32 *)((u32)g_render_packet_cursor_r.words + 8) = 0;
    *(s32 *)((u32)g_render_packet_cursor_r.words + 12) = 0x50000007;
    g_render_packet_cursor_r.words += 4;

    D_00160510_2CA00 = 1 - D_00160510_2CA00;
    if (visibility[0] == 1) {
        DmaToSpr(shell->tiles[0].address, shell->tiles[0].unkE >> 4, ((&sky_tile_spr_buffers))[D_00160510_2CA00]);
    }

    for (i = 0; i < shell->count; i++) {
        if (visibility[i] == 1) {
            DmaToSprSync();
        }

        next = i + 1;
        D_00160510_2CA00 = 1 - D_00160510_2CA00;
        if (next < shell->count && visibility[next] == 1) {
            DmaToSpr(shell->tiles[next].address, shell->tiles[next].unkE >> 4,
                     ((&sky_tile_spr_buffers))[D_00160510_2CA00]);
        }

        if (visibility[i] == 1) {
            dest = ((&sky_tile_spr_buffers))[1 - D_00160510_2CA00];
            c_dest = dest + shell->tiles[i].unkC;
            a_dest = dest + shell->tiles[i].unkA;
            if (sky_transform_vertices(dest + shell->tiles[i].unk8, 0x70002000, shell->tiles[i].unk4,
                                       (&shell->tiles[i])->unkC) == 0) {
                sky_build_gif_textured(shell->tiles[i].unk6, c_dest, a_dest, 0x70002000);
            }
        }
    }
}

typedef u32 u128_2CC40 __attribute__((mode(TI), aligned(16)));
struct DmaTag_2CC40 {
    u32 tag;
    u32 addr;
    u32 vif0;
    u32 vif1;
};
struct GifTag_2CC40;
union PacketCursor_2CC40 {
    struct DmaTag_2CC40 *tag;
    struct GifTag_2CC40 *gif;
    s32 *words;
    u8 *bytes;
    s32 addr;
};
extern union PacketCursor_2CC40 g_render_packet_cursor_r2 __asm__("D_00161000") MACRO_ADDR;
struct SkyTile_2CC40 {
    u128_2CC40 bounds; /* 16-byte aligned: tile fields are addressed from the tile base */
    s32 address;
    s16 unk4;
    s16 unk6;
    s16 unk8;
    s16 unkA;
    s16 unkC;
    s16 unkE;
};
struct Shell_2CC40 {
    s32 count;
    u8 pad4[0xC];
    struct SkyTile_2CC40 tiles[1];
};
extern s32 D_00160510_2CC40 SDATA(D_00160510);
extern u8 sky_gouraud_gs_block[];
extern void sky_bsphere_check_r2(struct SkyTile_2CC40 *tiles, s32 count, u8 *visibility) __asm__("func_0022D7E0");

extern s32 VU1_addGSregister_r3(s32, s64) __asm__("func_00234C98");

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/rendering/sky/sky_draw_shell_gouraud.c, sky_draw_shell_gouraud. */
void func_0022CC40_r(struct Shell_2CC40 *shell) __asm__("func_0022CC40");
/* SkyDrawShellGouraud */
void func_0022CC40_r(struct Shell_2CC40 *shell) {
    u8 visibility[shell->count];
    s32 i;
    s32 dest;
    s32 c_dest;
    s32 a_dest;
    s32 next;

    if (shell->count == 0) {
        return;
    }

    sky_bsphere_check_r2(shell->tiles, shell->count, visibility);

    g_render_packet_cursor_r2.words[0] = 0x30000007;
    *(s32 *)((u32)g_render_packet_cursor_r2.words + 4) = (s32)sky_gouraud_gs_block;
    *(s32 *)((u32)g_render_packet_cursor_r2.words + 8) = 0;
    *(s32 *)((u32)g_render_packet_cursor_r2.words + 12) = 0x50000007;
    g_render_packet_cursor_r2.words += 4;

    D_00160510_2CC40 = 1 - D_00160510_2CC40;
    if (visibility[0] == 1) {
        DmaToSpr(shell->tiles[0].address, shell->tiles[0].unkE >> 4, ((&sky_tile_spr_buffers))[D_00160510_2CC40]);
    }

    for (i = 0; i < shell->count; i++) {
        if (visibility[i] == 1) {
            DmaToSprSync();
        }

        next = i + 1;
        D_00160510_2CC40 = 1 - D_00160510_2CC40;
        if (next < shell->count && visibility[next] == 1) {
            DmaToSpr(shell->tiles[next].address, shell->tiles[next].unkE >> 4,
                     ((&sky_tile_spr_buffers))[D_00160510_2CC40]);
        }

        if (visibility[i] == 1) {
            dest = ((&sky_tile_spr_buffers))[1 - D_00160510_2CC40];
            c_dest = dest + shell->tiles[i].unkC;
            a_dest = dest + shell->tiles[i].unkA;
            if (sky_transform_vertices(dest + shell->tiles[i].unk8, 0x70002000, shell->tiles[i].unk4,
                                       (&shell->tiles[i])->unkC) == 0) {
                sky_build_gif_gouraud(shell->tiles[i].unk6, c_dest, a_dest, 0x70002000);
            }
        }
    }

    VU1_addGSregister_r3(0x47, 0x3180B);
    VU1_addGSregister_r3(0x4E, (depth_buffer_address >> 13) | 0x1000000 | ((s64)1 << 32));
}
