#include "common.h"
#include "camera.h"
#include "hero.h"
#include "vector.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * draw.cpp in the original source; text 0x1F0F30-0x1F7C60.
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

/* Resets the texture DMA state words to 1. */
void clear_dma_queue_entry(void) {
    int *p = draw_config;
    int val = 1;
    int i = 0x13;
    p = (int *)((char *)p + 0x4C);
    for (; i >= 0; i--, p--) {
        *p = val;
    }
}

LINKER_REMNANT("asm/remnants/text", func_001F0F70);

typedef struct {
    int x;
    int y;
    int color;
    char *str;
} DrawTextRec;
extern DrawTextRec font_text_slots[];
extern short font_text_cursor;
extern short font_text_slot_index;
extern char font_text_format[];
extern int sprintf();

/* Queues one text item: D_0018AC00[n] = {x, y, colour, pool position},
   then sprintf(pool, "%s", str) (D_0015F108 is "%s") advances the
   D_0015F100 string pool past the copy. Indexing the table at every
   store gives retail's two addu forms; a `DrawTextRec *` local folds
   them into one register and comes out 12 bytes short. */
void print_debug_text(int x, int y, int color, char *str) {
    int n = *(int *)&font_text_slot_index;

    font_text_slots[n].x = x;
    font_text_slots[n].y = y;
    font_text_slots[n].color = color;
    font_text_slots[n].str = *(char **)&font_text_cursor;
    *(int *)&font_text_slot_index = n + 1;
    *(char **)&font_text_cursor += sprintf(*(char **)&font_text_cursor, font_text_format, str) + 1;
}

LINKER_REMNANT("asm/remnants/text", func_001F0FF0);

extern int font_char_widths[];

/* Draws `str` centred on x: sums the per-character widths in D_00189EC0
   (indexed by char - 0x20, anything past the table using entry 0x20),
   moves x left by half the total and queues the text with
   func_001F0F78. Returns the adjusted x. `idx = ch` followed by the
   out-of-range override gives retail's sltiu 0x60 + movn; the reverse
   (default first, then `if (ch < 0x60)`) becomes sltu + movz. */
int print_debug_text_centered(int x, int y, int color, char *str) {
    int w = 0;
    unsigned char *p = (unsigned char *)str;

    while (*p != 0) {
        unsigned char ch = *p++ - 0x20;
        int idx = ch;
        if (ch >= 0x60) {
            idx = 0x20;
        }
        w += font_char_widths[idx];
    }
    x -= w >> 1;
    print_debug_text(x, y, color, str);
    return x;
}

__asm__(".section .text\n\tnop\n");

LINKER_REMNANT("asm/remnants/text", func_001F1088);

LINKER_REMNANT("asm/remnants/text", func_001F10A0);

/* A fog preset: an RGB byte triple and four floats. */
typedef struct {
    unsigned char r, g, b, pad;
    float f[4];
} FogPreset;
typedef struct {
    char pad0[0x1A0];
    int held;        /* 0x1A0 */
    int pressed;     /* 0x1A4 */
} __attribute__((aligned(16))) Pad1088;
extern Pad1088 g_controller_state;

typedef struct {
    int u0;          /* 0x00 */
    int flags4;      /* 0x04 */
    int flags8;      /* 0x08 */
    int page;        /* 0x0C */
    int cursor;      /* 0x10 */
    int a14, a18, a1C, a20, a24, a28, a2C, a30, a34, a38, a3C;
    char pad40[0x44];
    float f84, f88, f8C, f90;  /* 0x84 */
    int x94, y98, z9C;         /* 0x94 */
    int uA0, uA4;              /* 0xA0 */
    char padA8[0x48];
    int aF0, aF4;              /* 0xF0 */
    unsigned char bF8, bF9;    /* 0xF8 */
} Dbg1088;
extern Dbg1088 occl_cam_state;

typedef struct {
    char pad0[0x140];
    float pos[4];    /* 0x140 */
    float rot[4];    /* 0x150 */
} Cam1088;
extern Cam1088 gCamera;

extern Hero g_hero;

extern FogPreset fog_color_red MACRO_ADDR;
extern unsigned char fog_color_green[] MACRO_ADDR;
extern unsigned char fog_color_blue[] MACRO_ADDR;
extern float fog_near_distance MACRO_ADDR;
extern float fog_far_distance MACRO_ADDR;
extern float fog_near_intensity MACRO_ADDR;
extern float fog_far_intensity MACRO_ADDR;

typedef struct {
    char pad0[0xE];
    unsigned short max;  /* 0x0E */
    char pad10[0x8];
} Item1088;
extern Item1088 vendor_item_prices[];

typedef struct {
    char pad0[0x3E];
    short cap;           /* 0x3E */
    char pad40[0xC];
} Gadget1088;
extern Gadget1088 item_info_table[];

typedef struct {
    int v[6];
} Tbl1088;
extern Tbl1088 render_debug_menu_row_counts;

extern int view_context[];
extern int g_game_mode MACRO_ADDR;
extern int part_clip_dist MACRO_ADDR;
extern char *occl_grid_data MACRO_ADDR;
extern short fs_aa_buffer[];
extern float tfrag_draw_distance MACRO_ADDR;
extern float tie_draw_distance MACRO_ADDR;
extern float occl_fog_density_set MACRO_ADDR;
extern int occl_fog_count MACRO_ADDR;
extern int g_current_level_index MACRO_ADDR;
extern unsigned char saved_mission_state[];
extern unsigned char debug_mission_state_mirror[] MACRO_ADDR;
extern short debug_weapon_item_ids;
extern int weapon_ammo_counts[];
extern int gBolts MACRO_ADDR;
extern unsigned char gold_weapon_purchased[];
extern int camera_sample_point_count MACRO_ADDR;
extern float *camera_sample_points;
extern char camera_sample_path_suffix[];
extern char camera_sample_bin_name[];
extern char camera_sample_dir[];
extern char camera_sample_text_name[];
extern char camera_sample_line_format[];
extern char camera_sample_alt_line_format[];

extern void SetBackgroundColor(int, int, int);
extern void FastVecSub(void *, void *, void *);
extern float vec_length_xy(void *);
extern float FastSubRots(float, float);
extern float IntToFloat(int);
extern void UpdateViewContext(void);
extern void UpdateFog(void);
extern int libc_strlen(char *);
extern int sceOpen(char *, int);
extern int sceLseek(int, int, int);
extern int sceWrite(int, void *, int);
extern int sceRead(int, void *, int);
extern int sceClose(int);

/* Debug menu: pages of toggles, counters, fog and occlusion sample tools. */
void update_render_debug_menu(void) {
    Tbl1088 t;

    ((int (*)(void))sound_update)();
    if (g_controller_state.pressed & 0x500) {
        g_game_mode = occl_cam_state.uA4;
        SetBackgroundColor(view_context[0x8F], view_context[0x90], view_context[0x91]);
        occl_cam_state.bF8 = 0x6E;
        occl_cam_state.bF9 = 0x6E;
        occl_cam_state.x94 = (int)gCamera.pos[0] >> 2;
        occl_cam_state.y98 = (int)gCamera.pos[1] >> 2;
        occl_cam_state.z9C = (int)gCamera.pos[2] >> 2;
        return;
    } else {
        t = render_debug_menu_row_counts;

        if (g_controller_state.pressed & 0x2000) {
            int p = ++occl_cam_state.page;
            if (p == 3) {
                occl_cam_state.page = 0;
            } else if (p == 6) {
                occl_cam_state.page = 3;
            }
        } else if (g_controller_state.pressed & 0x8000) {
            int p = --occl_cam_state.page;
            if (p < 0) {
                occl_cam_state.page = 2;
            } else if (p == 2) {
                occl_cam_state.page = 5;
            }
        }

        if ((g_controller_state.pressed & 0x1000) && --occl_cam_state.cursor < 0) {
            if (occl_cam_state.page >= 3) {
                occl_cam_state.page = (occl_cam_state.page + 3) % 6;
            }
            occl_cam_state.cursor = t.v[occl_cam_state.page] - 1;
        } else if (g_controller_state.pressed & 0x4000) {
            if (++occl_cam_state.cursor >= t.v[occl_cam_state.page]) {
                if (occl_cam_state.page < 3) {
                    occl_cam_state.page = (occl_cam_state.page + 9) % 6;
                }
                occl_cam_state.cursor = 0;
            }
        }

        if (occl_cam_state.cursor >= t.v[occl_cam_state.page]) {
            occl_cam_state.cursor = t.v[occl_cam_state.page] - 1;
        }
    }

    switch (occl_cam_state.page) {
    case 0:
        if (g_controller_state.pressed & 0x40) {
            int bit = 1 << occl_cam_state.cursor;
            occl_cam_state.flags8 ^= bit;
            if (bit == 0x10) {
                if (occl_cam_state.flags8 & 0x10) {
                    occl_cam_state.flags8 = bit;
                } else {
                    occl_cam_state.flags8 = 0xF;
                }
            }
        }
        break;
    case 1:
        switch (occl_cam_state.cursor) {
        case 0:
            if (g_controller_state.pressed & 0x50) {
                if (g_controller_state.pressed & 0x40) {
                    if (++occl_cam_state.a14 >= 4) {
                        occl_cam_state.a14 = 0;
                    }
                }
                if (g_controller_state.pressed & 0x10) {
                    if (--occl_cam_state.a14 < 0) {
                        occl_cam_state.a14 = 3;
                    }
                }
                switch (occl_cam_state.a14) {
                case 0:
                    occl_cam_state.flags8 = 0xF;
                    break;
                case 1:
                    occl_cam_state.flags8 = 0;
                    break;
                case 2: {
                    float v[4];
                    occl_cam_state.flags8 = 0;
                    FastVecSub(v, g_hero.motion.pos.f, gCamera.pos);
                    occl_cam_state.f8C = vec_length_xy(v);
                    occl_cam_state.f88 = FastSubRots(fast_arc_tan(v[0], v[1]), gCamera.rot[2]);
                    occl_cam_state.f90 = g_hero.motion.pos.f[2] - gCamera.pos[2];
                    occl_cam_state.f84 = FastSubRots(g_hero.motion.rot.f[2], gCamera.rot[2]);
                    break;
                }
                case 3:
                    occl_cam_state.flags8 = 6;
                    break;
                }
            }
            break;
        case 1:
            if (g_controller_state.pressed & 0x40) {
                if (++occl_cam_state.a18 >= 8) {
                    occl_cam_state.a18 = 0;
                }
            }
            if (g_controller_state.pressed & 0x10) {
                if (--occl_cam_state.a18 < 0) {
                    occl_cam_state.a18 = 7;
                }
            }
            break;
        case 2:
            if (occl_grid_data == 0) {
                occl_cam_state.a1C = 0;
                break;
            }
            if (g_controller_state.pressed & 0x40) {
                if (++occl_cam_state.a1C >= 3) {
                    occl_cam_state.a1C = 0;
                }
            }
            if (g_controller_state.pressed & 0x10) {
                if (--occl_cam_state.a1C < 0) {
                    occl_cam_state.a1C = 2;
                }
            }
            break;
        case 3:
            if (g_controller_state.pressed & 0x50) {
                occl_cam_state.a20 = !occl_cam_state.a20;
            }
            break;
        case 4:
            if (g_controller_state.pressed & 0x50) {
                occl_cam_state.a24 = !occl_cam_state.a24;
            }
            break;
        case 5:
            if (g_controller_state.pressed & 0x50) {
                if ((occl_cam_state.a28 = !occl_cam_state.a28) != 0) {
                    tfrag_draw_distance = 65536.0f;
                    tie_draw_distance = 64.0f;
                    occl_fog_density_set = 40.0f;
                    occl_fog_count = 0x40;
                    part_clip_dist = 0x40000;
                } else {
                    tfrag_draw_distance = 512000.0f;
                    tie_draw_distance = 720.0f;
                    occl_fog_density_set = 500.0f;
                    occl_fog_count = 500;
                    part_clip_dist = 0x1F4000;
                }
            }
            break;
        case 6:
            if (g_controller_state.pressed & 0x40) {
                if (++occl_cam_state.a2C >= 3) {
                    occl_cam_state.a2C = 0;
                }
            }
            if (g_controller_state.pressed & 0x10) {
                if (--occl_cam_state.a2C < 0) {
                    occl_cam_state.a2C = 2;
                }
            }
            break;
        case 7:
            if (g_controller_state.pressed & 0x50) {
                occl_cam_state.a30 = !occl_cam_state.a30;
            }
            break;
        case 8:
            if (g_controller_state.pressed & 0x50) {
                if ((occl_cam_state.a34 = !occl_cam_state.a34) != 0) {
                    float *ctx = (float *)view_context;
                    ctx[0x80] = IntToFloat(fs_aa_buffer[0xA8]) * 0.125f;
                    ctx[0x81] = IntToFloat(fs_aa_buffer[0xA9]) * 0.125f;
                } else {
                    float *ctx = (float *)view_context;
                    ctx[0x80] = IntToFloat(fs_aa_buffer[0xA8]) * 0.5f;
                    ctx[0x81] = IntToFloat(fs_aa_buffer[0xA9]) * 0.5f;
                }
                {
                    float *v = (float *)view_context;
                    v[0x82] = v[0x80] * 4.0f;
                    v[0x83] = v[0x81] * 4.0f;
                }
                UpdateViewContext();
            }
            break;
        case 9:
            if (g_controller_state.pressed & 0x50) {
                if (g_controller_state.pressed & 0x40) {
                    occl_cam_state.a38++;
                    while (occl_cam_state.a38 == 0x24) {
                        occl_cam_state.a38 = -1;
                    }
                } else {
                    occl_cam_state.a38--;
                    while (occl_cam_state.a38 < -1) {
                        occl_cam_state.a38 = 0x23;
                    }
                }
            }
            if (occl_cam_state.a38 >= 0) {
                occl_cam_state.uA4 = 2;
            } else {
                occl_cam_state.uA4 = 0;
            }
            break;
        case 10:
            if (g_controller_state.pressed & 0x50) {
                if (g_controller_state.pressed & 0x40) {
                    occl_cam_state.a3C++;
                    while (occl_cam_state.a3C == 0x10) {
                        occl_cam_state.a3C = 0;
                    }
                } else {
                    occl_cam_state.a3C--;
                    while (occl_cam_state.a3C < 0) {
                        occl_cam_state.a3C = 0xF;
                    }
                }
            } else if (g_controller_state.pressed & 0x20) {
                int i = occl_cam_state.a3C;
                unsigned char *e = &saved_mission_state[i + g_current_level_index * 16];
                *e = (*e == 0) ? 0xFF : 0;
                debug_mission_state_mirror[i] = *e;
            }
            break;
        }
        break;
    case 2:
        if (g_controller_state.pressed & 0x40) {
            occl_cam_state.flags4 ^= 1 << occl_cam_state.cursor;
        }
        break;
    case 3: {
        int max = 999999;
        int *val;
        if (occl_cam_state.cursor < 13) {
            unsigned char k = ((unsigned char *)&debug_weapon_item_ids)[occl_cam_state.cursor];
            max = ((Item1088 *)(k * sizeof(Item1088) + (unsigned int)vendor_item_prices))->max;
            val = &weapon_ammo_counts[k];
        } else {
            val = &gBolts;
        }
        if (g_controller_state.held & 8) {
            if (g_controller_state.pressed & 0x40) {
                *val += 60;
            }
            if (g_controller_state.pressed & 0x20) {
                if (*val > 60) {
                    *val -= 60;
                } else {
                    *val = 0;
                }
            }
        } else if (g_controller_state.held & 2) {
            if (g_controller_state.pressed & 0x40) {
                *val += 600;
            }
            if (g_controller_state.pressed & 0x20) {
                if (*val > 600) {
                    *val -= 600;
                } else {
                    *val = 0;
                }
            }
        } else {
            if (g_controller_state.pressed & 0x40) {
                *val += 1;
            }
            if (g_controller_state.pressed & 0x20) {
                if (*val > 0) {
                    *val -= 1;
                }
            }
        }
        if (*val > max) {
            *val = max;
        }
        if (g_controller_state.pressed & 0x10) {
            gold_weapon_purchased[((unsigned char *)&debug_weapon_item_ids)[occl_cam_state.cursor]]++;
        }
        if (g_controller_state.pressed & 0x80) {
            gold_weapon_purchased[((unsigned char *)&debug_weapon_item_ids)[occl_cam_state.cursor]]--;
        }
        {
            unsigned char k = ((unsigned char *)&debug_weapon_item_ids)[occl_cam_state.cursor];
            unsigned char *c = &gold_weapon_purchased[k];
            Gadget1088 *g = &item_info_table[k];
            if (g->cap < *c) {
                *c = g->cap;
            }
        }
        return;
    }
    case 4:
        if (occl_cam_state.cursor == 0) {
            if ((g_controller_state.held & 0x40) || (g_controller_state.pressed & 8)) {
                fog_near_distance += 1024.0f;
            }
            if ((g_controller_state.held & 0x10) || (g_controller_state.pressed & 4)) {
                fog_near_distance -= 1024.0f;
            }
            if (fog_near_distance > fog_far_distance) {
                fog_near_distance = fog_far_distance;
            } else if (fog_near_distance < 0.0f) {
                fog_near_distance = 0.0f;
            }
        }
        if (occl_cam_state.cursor == 2) {
            if ((g_controller_state.held & 0x40) || (g_controller_state.pressed & 8)) {
                fog_far_distance += 1024.0f;
            }
            if ((g_controller_state.held & 0x10) || (g_controller_state.pressed & 4)) {
                fog_far_distance -= 1024.0f;
            }
            if (524288.0f < fog_far_distance) {
                fog_far_distance = 524288.0f;
            } else if (fog_far_distance < fog_near_distance) {
                fog_far_distance = fog_near_distance;
            }
        }
        if (occl_cam_state.cursor == 1) {
            if ((g_controller_state.held & 0x40) || (g_controller_state.pressed & 8)) {
                fog_near_intensity -= 2.55f;
            }
            if ((g_controller_state.held & 0x10) || (g_controller_state.pressed & 4)) {
                fog_near_intensity += 2.55f;
            }
            if (255.0f < fog_near_intensity) {
                fog_near_intensity = 255.0f;
            } else if (fog_near_intensity < 0.0f) {
                fog_near_intensity = 0.0f;
            }
        }
        if (occl_cam_state.cursor == 3) {
            if ((g_controller_state.held & 0x40) || (g_controller_state.pressed & 8)) {
                fog_far_intensity -= 2.55f;
            }
            if ((g_controller_state.held & 0x10) || (g_controller_state.pressed & 4)) {
                fog_far_intensity += 2.55f;
            }
            if (255.0f < fog_far_intensity) {
                fog_far_intensity = 255.0f;
            } else if (fog_far_intensity < 0.0f) {
                fog_far_intensity = 0.0f;
            }
        }
        if (occl_cam_state.cursor == 4) {
            if (fog_color_red.r < 0xFF && ((g_controller_state.held & 0x40) || (g_controller_state.pressed & 8))) {
                fog_color_red.r++;
            }
            if (fog_color_red.r != 0 && ((g_controller_state.held & 0x10) || (g_controller_state.pressed & 4))) {
                fog_color_red.r--;
            }
        }
        if (occl_cam_state.cursor == 5) {
            if (fog_color_green[0] < 0xFF && ((g_controller_state.held & 0x40) || (g_controller_state.pressed & 8))) {
                fog_color_green[0]++;
            }
            if (fog_color_green[0] != 0 && ((g_controller_state.held & 0x10) || (g_controller_state.pressed & 4))) {
                fog_color_green[0]--;
            }
        }
        if (occl_cam_state.cursor == 6) {
            if (fog_color_blue[0] < 0xFF && ((g_controller_state.held & 0x40) || (g_controller_state.pressed & 8))) {
                fog_color_blue[0]++;
            }
            if (fog_color_blue[0] != 0 && ((g_controller_state.held & 0x10) || (g_controller_state.pressed & 4))) {
                fog_color_blue[0]--;
            }
        }
        ((void (*)(void))UpdateFog)();
        return;
    case 5:
        if (occl_cam_state.cursor == 0 && (g_controller_state.pressed & 0x50)) {
            occl_cam_state.aF0 = !occl_cam_state.aF0;
        }
        if (occl_cam_state.cursor == 1 && (g_controller_state.pressed & 0x50)) {
            occl_cam_state.aF4 = !occl_cam_state.aF4;
        }
        if (occl_cam_state.cursor == 2 && (g_controller_state.pressed & 0xF0)) {
            extern char camera_sample_path_format;
            char *fmt = camera_sample_dir;
            char *dev = &camera_sample_path_format;
            int i, found;

            found = 0;
            for (i = 0; i < camera_sample_point_count; i++) {
                float *p = camera_sample_points + i;
                if (p[0] < gCamera.pos[0] + 0.5f && gCamera.pos[0] - 0.5f < p[0]
                    && p[1] < gCamera.pos[1] + 0.5f && gCamera.pos[1] - 0.5f < p[1]
                    && p[2] < gCamera.pos[2] + 0.5f && gCamera.pos[2] - 0.5f < p[2]) {
                    found = 1;
                    break;
                }
            }
            if (found) {
                return;
            }
            {
                char path[64];
                char line[48];
                float *samples;
                int fd, size;

                sprintf(path, dev, fmt, g_current_level_index, camera_sample_text_name, camera_sample_path_suffix);
                sprintf(line, camera_sample_line_format, func_00120778(gCamera.pos[0]),
                              func_00120778(gCamera.pos[1]), func_00120778(gCamera.pos[2]));
                fd = sceOpen(path, 0x303);
                if (fd >= 0) {
                    sceLseek(fd, 0, 2);
                    sceWrite(fd, line, libc_strlen(line));
                    sceClose(fd);
                    occl_cam_state.bF9 = 0x79;
                }
                samples = camera_sample_points;
                sprintf(path, dev, fmt, g_current_level_index, camera_sample_bin_name, camera_sample_path_suffix);
                fd = sceOpen(path, 0x303);
                if (fd >= 0) {
                    sceLseek(fd, 0, 2);
                    sceWrite(fd, &gCamera.pos[0], 4);
                    sceWrite(fd, &gCamera.pos[1], 4);
                    sceWrite(fd, &gCamera.pos[2], 4);
                    sceLseek(fd, 0, 0);
                    size = sceLseek(fd, 0, 2);
                    if (size <= 0x100000) {
                        sceLseek(fd, 0, 0);
                        sceRead(fd, samples, size);
                        camera_sample_point_count = size / 12;
                    } else {
                        camera_sample_point_count = 0;
                    }
                    sceClose(fd);
                }
            }
        }
        if (occl_cam_state.cursor == 3 && (g_controller_state.pressed & 0xF0)) {
            extern char camera_sample_path_format[];
            char path[64];
            char line[64];
            int fd;

            sprintf(path, camera_sample_path_format, camera_sample_dir, g_current_level_index, camera_sample_text_name, camera_sample_path_suffix);
            sprintf(line, camera_sample_alt_line_format, func_00120778(gCamera.pos[0]),
                          func_00120778(gCamera.pos[1]), func_00120778(gCamera.pos[2]));
            fd = sceOpen(path, 0x303);
            if (fd >= 0) {
                sceLseek(fd, 0, 2);
                sceWrite(fd, line, libc_strlen(line));
                sceClose(fd);
                occl_cam_state.bF8 = 0x79;
            }
        }
        break;
    }
}

LINKER_REMNANT("asm/remnants/text", func_001F2410);

extern void set_identity_matrix(void *);
extern void FastVecScale(void *, void *, float);
extern void transform_vector(void *, void *, void *);
extern float screen_projection_focal;

/* The camera block as a struct: all four accesses then share one base
   register with field offsets, as in retail. */
extern CameraSystem gCamera_cam __asm__("D_00187040");


/* Projects camera-space point a1 through the camera matrix, divides by
   depth (D_0018D010 / w) and scales to x16 screen units. func_002346C0
   in tfragfunc.c builds the same matrix. */
void project_to_screen(Vec3f *a0, float *a1) {
    float m[4][4];
    float m2[4][4];
    float v[4];
    float out[4];
    float invw;

    set_identity_matrix(m);
    m[3][0] = -gCamera_cam.pos.f[0] * 1024.0f;
    m[3][1] = -gCamera_cam.pos.f[1] * 1024.0f;
    m[3][2] = -gCamera_cam.pos.f[2] * 1024.0f;
    sce_vu0_mul_matrix(m2, gCamera_cam.view_matrix, m);

    FastVecScale(v, a1, 1024.0f);
    v[3] = 1.0f;
    transform_vector(out, v, m2);

    invw = screen_projection_focal / out[3];
    a0->z = out[2] * 0.0009765625f;
    out[0] = out[0] * invw + 2048.0f;
    a0->x = out[0] * 16.0f;
    out[1] = out[1] * invw + 2048.0f;
    a0->y = out[1] * 16.0f;
}

LINKER_REMNANT("asm/remnants/text", func_001F2550);

/* Empty stub (returns immediately). */
void ProfileMarker_End(void) {
}

/* Empty stub (returns immediately). */
void ProfileMarker_Begin(void) {
}

extern int space_load_in_progress_m __asm__("D_0015F6FC") MACRO_ADDR;
extern int pal_mode MACRO_ADDR;
extern int g_gs_texture_allocation_start MACRO_ADDR;
extern int *g_render_packet_cursor MACRO_ADDR;

extern void DMAC_VIF1_Disable(void);
extern int sceDmaReset(int);
extern void InitDma(void);
extern void sceGsResetGraph(int, int, int, int);
extern void reset_vif1_gif(void);
extern void VU1_initChain(void);
extern void SetPalMode(void);
extern void DMAC_VIF1_Enable(void);
extern volatile int vif_chain_flags_v __asm__("D_00160FE0") MACRO_ADDR;

/* Render setup: raises D_0015F6FC, clears D_00160FE0, runs the setup calls,
   picks func_00121B78's mode from D_0015EE80 (retail's movz), then runs
   func_001F3890 with D_0015EF78 saved around it and D_00161000 cleared.
   The D_00160FE0 flag is volatile: retail keeps its store out of
   func_002350A8's delay slot, where the plain MACRO_ADDR store would go.
   The two stores that do sit in delay slots (D_00161000, D_0015EF78) are
   MACRO_ADDR and come out $gp-relative there. */
void reset_graphics(void) {
    int flag;
    int pal;

    space_load_in_progress_m = 1;
    vif_chain_flags_v = 0;
    DMAC_VIF1_Disable();
    sceDmaReset(1);
    InitDma();
    flag = pal_mode;
    sceGsResetGraph(0, 1, flag ? 3 : 2, 0);
    reset_vif1_gif();
    VU1_initChain();
    pal = g_gs_texture_allocation_start;
    g_render_packet_cursor = 0;
    SetPalMode();
    g_gs_texture_allocation_start = pal;
    VU1_initChain();
    DMAC_VIF1_Enable();
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

extern void scale_vector(void *, void *, float);
extern int camera_position_publication_suppressed;
extern char camera_rows[];
extern char view_context_field_c0[];
extern char sky_view_mtx[];

typedef struct {
    float f[0x50];
} SceneG;
extern SceneG gCamera_g __asm__("D_00187040");

/* Builds the view matrices in the camera block from the rotation set up in D_00187390 or the three rotation angles, then copies the result to D_0018D080. Adapted from Lombyte's project (GPL-3.0) for PAL: src/rendering/view/fun_001f2260.c, FUN_001f2260. */
void BuildViewMatrices(void) {
    float m[4][4];
    SceneG *g;
    char *x;
    float *xf;
    float a;
    float b;
    float c;

    if (camera_position_publication_suppressed == 0) {
        qcopy(m[0], camera_rows);
        qcopy(m[1], camera_rows + 0x10);
        qcopy(m[2], camera_rows + 0x20);
    } else {
        sceVu0UnitMatrix(m[0]);
        sceVu0RotMatrixX(m[0], m[0], gCamera_g.f[0x150 / 4]);
        sceVu0RotMatrixY(m[0], m[0], gCamera_g.f[0x154 / 4]);
        sceVu0RotMatrixZ(m[0], m[0], gCamera_g.f[0x158 / 4]);
    }
    g = &gCamera_g;
    x = view_context_field_c0;
    xf = (float *)(x - 0xC0);
    g->f[0] = -m[1][0];
    g->f[0x3C / 4] = 1.0f;
    *(int *)&g->f[0x30 / 4] = 0;
    g->f[0x10 / 4] = -m[1][1];
    g->f[0x20 / 4] = -m[1][2];
    g->f[4 / 4] = -m[2][0];
    g->f[0x14 / 4] = -m[2][1];
    g->f[0x24 / 4] = -m[2][2];
    g->f[8 / 4] = m[0][0];
    g->f[0x18 / 4] = m[0][1];
    g->f[0x28 / 4] = m[0][2];
    *(int *)&g->f[0x34 / 4] = 0;
    *(int *)&g->f[0x38 / 4] = 0;
    *(int *)&g->f[0xC / 4] = 0;
    *(int *)&g->f[0x1C / 4] = 0;
    *(int *)&g->f[0x2C / 4] = 0;
    sce_vu0_mul_matrix(&g->f[0x40 / 4], x, g);
    sce_vu0_mul_matrix(&g->f[0x80 / 4], x + 0x40, g);
    a = xf[0x1A0 / 4];
    b = xf[0x1A4 / 4];
    c = xf[0x1A8 / 4];
    g->f[0x80 / 4] += g->f[0x8C / 4] * a;
    g->f[0x84 / 4] += g->f[0x8C / 4] * b;
    g->f[0x88 / 4] += g->f[0x8C / 4] * c;
    g->f[0x90 / 4] += g->f[0x9C / 4] * a;
    g->f[0x94 / 4] += g->f[0x9C / 4] * b;
    g->f[0x98 / 4] += g->f[0x9C / 4] * c;
    g->f[0xA0 / 4] += g->f[0xAC / 4] * a;
    g->f[0xA4 / 4] += g->f[0xAC / 4] * b;
    g->f[0xA8 / 4] += g->f[0xAC / 4] * c;
    g->f[0xB0 / 4] += g->f[0xBC / 4] * a;
    g->f[0xB4 / 4] += g->f[0xBC / 4] * b;
    g->f[0xB8 / 4] += g->f[0xBC / 4] * c;
    sce_vu0_mul_matrix(&g->f[0xC0 / 4], x + 0x80, g);
    scale_vector(&g->f[0x100 / 4], x + 0x80, xf[0x1C0 / 4]);
    scale_vector(&g->f[0x110 / 4], x + 0x90, xf[0x1C0 / 4]);
    qcopy(&g->f[0x120 / 4], x + 0xA0);
    qcopy(&g->f[0x130 / 4], x + 0xB0);
    sce_vu0_mul_matrix(&g->f[0x100 / 4], &g->f[0x100 / 4], g);
    qcopy(sky_view_mtx, g);
    qcopy(sky_view_mtx + 0x10, &g->f[0x10 / 4]);
    qcopy(sky_view_mtx + 0x20, &g->f[0x20 / 4]);
    FastVecScale(sky_view_mtx + 0x30, &g->f[0x140 / 4], 1024.0f);
    *(float *)(sky_view_mtx + 0x3C) = 1024.0f;
}

extern int use_alt_fog;
extern FogPreset water_fog_color_red MACRO_ADDR;
extern FogPreset fog_color_red MACRO_ADDR;
extern int view_context[];
extern int part_clip_dist MACRO_ADDR;
extern int level_fog_mode MACRO_ADDR;
extern void UpdateViewContext(void);

/* UpdateFog(void): copies the fog preset (the fixed D_001611C4 when
   D_001873D4 is set, else the current D_0015F584) into the draw context
   at D_0018CE00+0x218, sets the mode word D_001601BC (0x40000 or
   0x1F4000), runs UpdateViewContext (func_001F3140) and clears
   D_0015F598. The presets and the two words it writes are MACRO_ADDR:
   retail reads each field with the one-register macro, and the mode
   word's store fits the branch delay slot ($gp-relative there). */
void UpdateFog(void) {
    if (use_alt_fog != 0) {
        view_context[0x8C] = water_fog_color_red.r;
        view_context[0x8D] = water_fog_color_red.g;
        view_context[0x8E] = water_fog_color_red.b;
        *(float *)&view_context[0x86] = water_fog_color_red.f[0];
        *(float *)&view_context[0x87] = water_fog_color_red.f[1];
        *(float *)&view_context[0x8A] = water_fog_color_red.f[2];
        *(float *)&view_context[0x8B] = water_fog_color_red.f[3];
        part_clip_dist = 0x40000;
    } else {
        view_context[0x8C] = fog_color_red.r;
        view_context[0x8D] = fog_color_red.g;
        view_context[0x8E] = fog_color_red.b;
        *(float *)&view_context[0x86] = fog_color_red.f[0];
        *(float *)&view_context[0x87] = fog_color_red.f[1];
        *(float *)&view_context[0x8A] = fog_color_red.f[2];
        *(float *)&view_context[0x8B] = fog_color_red.f[3];
        part_clip_dist = 0x1F4000;
    }
    UpdateViewContext();
    level_fog_mode = 0;
}
__asm__(".section .text\n\tnop\n");

extern char *occl_grid_data MACRO_ADDR;

/* ParseOcclGrid(x, y, z): walks the three-level occlusion grid at
   D_0015F720. Each level is {u16 start, u16 count, u16 entry[count]};
   the coordinate minus start must fall in [0, count), and its entry is
   the next level's offset in words from the grid (levels 1 and 2, 0 =
   empty) or, at the last level, a 128-byte cell index from the root
   (grid + grid[0]), 0xFFFF = empty. Returns the cell or 0. Each level's
   coordinate goes in its own local: that keeps y and x in their argument
   registers and the level pointer in $a3. The last level's bounds share
   one `if`, which is what leaves retail's shared failure return after
   level 2 and a separate one for the 0xFFFF test. */
int ParseOcclGrid(int x, int y, int z) {
    char *grid = occl_grid_data;
    char *base = grid + *(int *)grid;
    unsigned short *l = (unsigned short *)(grid + 4);
    int cz, cy, cx;

    cz = z - l[0];
    if (cz < 0) return 0;
    if (cz >= l[1]) return 0;
    if (l[cz + 2] == 0) return 0;
    l = (unsigned short *)(grid + l[cz + 2] * 4);
    cy = y - l[0];
    if (cy < 0) return 0;
    if (cy >= l[1]) return 0;
    if (l[cy + 2] == 0) return 0;
    l = (unsigned short *)(grid + l[cy + 2] * 4);
    cx = x - l[0];
    if (cx < 0 || cx >= l[1]) return 0;
    if (l[cx + 2] == 0xFFFF) return 0;
    return (int)(base + l[cx + 2] * 128);
}

__asm__(".section .text\n\tnop\n");

extern int ParseOcclGrid(int, int, int);

/* GetOcclGridFromPair(int, int, int, int, int, int, float) */
int GetOcclGridFromPair(int a0, int a1, int a2, int a3, int a4, int a5, float t) {
    int r;
    int b0;
    int b1;
    int b2;

    if (t < 0.5f) {
        r = ParseOcclGrid(a0, a1, a2);
        if (r != 0) {
            return r;
        }
        b0 = a3;
        b1 = a4;
        b2 = a5;
    } else {
        r = ParseOcclGrid(a3, a4, a5);
        if (r != 0) {
            return r;
        }
        b0 = a0;
        b1 = a1;
        b2 = a2;
    }
    return ParseOcclGrid(b0, b1, b2);
}

extern int truncate_float_to_s32_r(float) __asm__("func_001FA898");
extern float IntToFloat(int);
extern int GetOcclGridFromPair(int, int, int, int, int, int, float);
extern void fast_mem_or16(void *, void *, void *, int);
extern void FastMemSet();
extern char occl_visibility[];
extern char occl_visibility_merged[];
extern int occl_invalid_grid MACRO_ADDR;
extern int occl_fallback_mode MACRO_ADDR;
extern char *occl_previous_grid MACRO_ADDR;
extern float *occl_oct MACRO_ADDR;

/* Build the occlusion visibility bitmap for the camera's grid cell.
   Adapted from Lombyte's project (GPL-3.0) for PAL: src/rendering/build_occlusion_visibility.c, build_occlusion_visibility. */
void BuildOcclVisibility(void) {
    float scale = 0.25f;
    int x, y, z;
    char *vis;
    char *gx, *gy, *gz;
    float *p;
    int bx, by, bz;

    x = truncate_float_to_s32_r(gCamera_cam.pos.f[0] * scale);
    y = truncate_float_to_s32_r(gCamera_cam.pos.f[1] * scale);
    z = truncate_float_to_s32_r(gCamera_cam.pos.f[2] * scale);
    vis = (char *)ParseOcclGrid(x, y, z);
    if (vis != 0) {
        occl_invalid_grid = 0;
        FastMemCopy(occl_visibility, vis, 0x80);
        occl_previous_grid = vis;
    } else {
        occl_invalid_grid = 1;
        if (occl_fallback_mode == 0) {
            gx = (char *)GetOcclGridFromPair(x - 1, y, z, x + 1, y, z,
                                       gCamera_cam.pos.f[0] * scale - IntToFloat(x));
            gy = (char *)GetOcclGridFromPair(x, y - 1, z, x, y + 1, z,
                                       gCamera_cam.pos.f[1] * scale - IntToFloat(y));
            gz = (char *)GetOcclGridFromPair(x, y, z - 1, x, y, z + 1,
                                       gCamera_cam.pos.f[2] * scale - IntToFloat(z));
            if (gx != 0 || gy != 0 || gz != 0) {
                FastMemZero16(occl_visibility_merged, 0x80);
                if (gx != 0) {
                    fast_mem_or16(occl_visibility_merged, occl_visibility_merged, gx, 0x80);
                }
                if (gy != 0) {
                    fast_mem_or16(occl_visibility_merged, occl_visibility_merged, gy, 0x80);
                }
                if (gz != 0) {
                    fast_mem_or16(occl_visibility_merged, occl_visibility_merged, gz, 0x80);
                }
                vis = occl_visibility_merged;
                occl_previous_grid = vis;
                FastMemCopy(occl_visibility, vis, 0x80);
            }
        }
        if (vis == 0) {
            switch (occl_fallback_mode) {
            case 0:
                if (camera_position_publication_suppressed == 0 && occl_previous_grid != 0) {
                    FastMemCopy(occl_visibility, occl_previous_grid, 0x80);
                } else {
                    FastMemSet(occl_visibility, -1, 0x80);
                }
                break;
            case 1:
                FastMemSet(occl_visibility, -1, 0x80);
                break;
            case 2:
                p = occl_oct;
                if (p != 0) {
                    bx = 0.0f < gCamera_cam.pos.f[0] - p[0];
                    by = 0.0f < gCamera_cam.pos.f[1] - p[1];
                    bz = 0.0f < gCamera_cam.pos.f[2] - p[2];
                    FastMemCopy(occl_visibility, (char *)p + ((bz + by * 2 + bx * 4) * 0x80 + 0x10), 0x80);
                } else if (camera_position_publication_suppressed == 0 && occl_previous_grid != 0) {
                    FastMemCopy(occl_visibility, occl_previous_grid, 0x80);
                } else {
                    FastMemSet(occl_visibility, -1, 0x80);
                }
                break;
            }
        }
    }
    {
        unsigned char *vb = (unsigned char *)occl_visibility;
        vb[0x7F] |= 0x80;
    }
}

/* Unprototyped deliberately: two call sites need incompatible arg1
   types (-1 and a pointer) and both callers are byte-exact, so
   neither may be edited. Codegen is identical either way -- int and
   pointer are both 32-bit in the same arg register. */
extern void BuildOcclVisibility(void);
extern int occl_update NOT_SDA;

/* UpdateOcclusion(void) */
void UpdateOcclusion(void) {
    int state = occl_update;
    if (state == 0) {
        FastMemSet(occl_visibility, -1, 0x80);
    } else if (state == 2) {
        BuildOcclVisibility();
    }
}

extern short fs_aa_buffer[];
extern int g_screen_extent[];
extern float IntToFloat(int);

/* InitViewContext: the same set-up as configure_graphics_projection (func_001F3760) for
   the display's size, D_00151880[0xA8]/[0xA9], with fixed extras. The
   GS viewport record D_0013E600 gets width, height, their halves and the
   four <<4 edges around the 0x800 centre. The draw context D_0018CE00
   gets 32, 745472 and 0.63 at +0xA0/+0xA4/+0xB0, the half extents as
   floats (func_001FA888 is int to float) at +0x200/+0x204, four times
   each at +0x208/+0x20C, and 0, 524288, 255, 0 at +0x218/+0x21C/+0x228/
   +0x22C. The size is read into `short` locals (lhu, then sll/sra) through
   a base pointer that stays in $s1, and the height is read again from
   memory for the second conversion. */
void InitViewContext(void) {
    short *res = fs_aa_buffer;
    float *ctx = (float *)view_context;
    short w = res[0xA8];
    short h = res[0xA9];
    int hw = w >> 1;
    int hh = h >> 1;
    float fh;

    g_screen_extent[0] = w;
    g_screen_extent[1] = h;
    g_screen_extent[2] = hw;
    g_screen_extent[3] = hh;
    g_screen_extent[4] = (0x800 - hw) << 4;
    g_screen_extent[5] = (0x800 - hh) << 4;
    g_screen_extent[6] = (hw + 0x800) << 4;
    g_screen_extent[7] = (hh + 0x800) << 4;
    ctx[0x28] = 32.0f;
    ctx[0x29] = 745472.0f;
    ctx[0x2C] = 0.63f;
    ctx[0x80] = IntToFloat(w) * 0.5f;
    fh = IntToFloat(res[0xA9]) * 0.5f;
    ctx[0x81] = fh;
    ctx[0x82] = ctx[0x80] * 4.0f;
    ctx[0x83] = fh * 4.0f;
    ctx[0x86] = 0.0f;
    ctx[0x87] = 524288.0f;
    ctx[0x8A] = 255.0f;
    ctx[0x8B] = 0.0f;
}

__asm__(".section .text\n\tnop\n");

typedef u32 VecQuadP __attribute__((mode(TI), aligned(16)));
struct ViewP {
    s32 fog_color; s32 unk4; s32 unk8; s32 unkC;
    Vec4 regs[5]; Vec4 gif[4];
    f32 near_clip; f32 far_clip; f32 aspect_x; f32 aspect_y;
    Vec4 fov; f32 proj[16]; f32 proj2[16]; f32 proj3[16];
    Vec4 inverse_screen_scale, screen_scale, screen_bias, inverse_projection_scale, viewport_aspect, aspect_scale, clip_scale, clip_distances;
    f32 half_width, half_height, scr_x, scr_y, fog_mul, fog_add, fog_near_dist, fog_far_dist, fog_slope, fog_base, fog_near_int, fog_far_int;
    s32 fog_r, fog_g, fog_b;
};
#define view_context_p (*(struct ViewP *)&D_0018CE00)
extern s32 pal_mode MACRO_ADDR;
#define pal_mode_p D_0015EE80
struct FogRegs2P {
    u8 pad0[0x28];
    s32 mul;
    s32 max;
    s32 far_int;
    s32 near_int;
};
struct ClipP {
    Vec4 constants[3];
};

extern Vec4 view_clip_constants[4];
extern f32 view_depth_constants[4];
extern f32 view_fog_multiplier MACRO_ADDR;
extern f32 view_fog_multiplier_copy[];
extern f32 view_fog_multiplier_copy2[];
extern struct FogRegs2P shrub_vu1_constants;
extern struct FogRegs2P shrub_vu1_constants_alt;
extern Vec4 shrub_vu1_constants_alt_screen[2];
extern Vec4 vu1_view_screen_consts[3];
extern struct {
    u8 pad0[0x10];
    s32 mul;
    s32 far_int;
    s32 near_int;
    u8 pad1c[0x44];
    Vec4 constants[3];
    u8 pad90[0x10];
    s32 x;
    s32 y;
} D_001DEAC0;

extern f32 fast_arc_tan(f32, f32);
extern f32 FastCos(f32);
#define fast_cos func_001F9F90
extern void copy_memory_aligned(void *, void *, s32);
extern void SetTfragDists(void);
#define set_tfrag_dists func_00234380

/* Rebuilds the view state from the field of view, screen size and fog
   endpoints: the clip constants at D_0018D0C0, the fog multiplier and the
   fog packet registers, the projection matrices and the screen scale and
   bias vectors that the renderers copy into their packets. */
void UpdateViewContext(void) {
    struct ViewP *view;
    struct ViewP *packet_view;
    f32 depth_scale;
    f32 depth_bias;
    f32 horizontal_clip_scale;
    f32 vertical_clip_scale;
    f32 fog_distance_range;
    f32 fog_intensity_range;
    f32 fog_distance_slope;
    f32 near_clip;
    f32 projection_near_clip;
    f32 far_clip;
    f32 fog_multiplier;
    f32 projection_fog_multiplier;
    f32 horizontal_fov;
    f32 vertical_fov;
    f32 viewport_width;
    f32 viewport_height;
    s32 near_bits;
    s32 far_bits;
    s32 mul_bits;

    depth_scale = -8388080.0f;
    depth_bias = 8388112.0f;
    if (pal_mode_p != 0) {
        view_context_p.fov.f[1] = view_context_p.fov.f[0] * 0.756f;
    } else {
        view_context_p.fov.f[1] = view_context_p.fov.f[0] * 0.775f;
    }
    view = &view_context_p;
    view->aspect_x = view->scr_x / view->half_width;
    view->aspect_y = view->scr_y / view->half_height;
    view->fov.f[2] = 1.0f / fast_cos(fast_arc_tan(1.0f, view->fov.f[0]));
    view->fov.f[3] = 1.0f / fast_cos(fast_arc_tan(1.0f, view->fov.f[1]));
    horizontal_clip_scale = 1.0f / fast_cos(fast_arc_tan(1.0f, view->fov.f[0] * view->aspect_x));
    vertical_clip_scale = 1.0f / fast_cos(fast_arc_tan(1.0f, view->fov.f[1] * view->aspect_y));
    horizontal_clip_scale /= view->fov.f[2];
    vertical_clip_scale /= view->fov.f[3];
    view->aspect_scale.f[2] = 1.0f / view->aspect_x;
    view->aspect_scale.f[3] = 1.0f / view->aspect_y;
    view->clip_scale.f[0] = view->fov.f[2];
    view->clip_scale.f[1] = view->fov.f[3];
    view->clip_distances.f[0] = view->far_clip;
    view->aspect_scale.f[0] = view->aspect_x;
    view->aspect_scale.f[1] = view->aspect_y;
    near_clip = view->near_clip;
    view->clip_distances.f[1] = near_clip;
    view->clip_scale.f[2] = (horizontal_clip_scale + vertical_clip_scale) * 0.5f;
    qcopy(&view_clip_constants[0], &view->fov);
    qcopy(&view_clip_constants[1], &view->aspect_scale);
    qcopy(&view_clip_constants[2], &view->clip_scale);
    qcopy(&view_clip_constants[3], &view->clip_distances);

    /* Retail lw instructions carry the fog endpoint float bits into packet fields. */
    far_bits = *(s32 *)&view->fog_far_int;
    near_bits = *(s32 *)&view->fog_near_int;
    fog_distance_range = view->fog_far_dist - view->fog_near_dist;
    fog_intensity_range = view->fog_far_int - view->fog_near_int;
    fog_multiplier = fog_intensity_range * near_clip / fog_distance_range;
    fog_distance_slope = fog_intensity_range / (fog_distance_range / 1024.0f);
    view->fog_add =
        (view->fog_near_int * view->fog_far_dist - view->fog_far_int * view->fog_near_dist) /
        fog_distance_range;
    view->fog_slope = fog_distance_slope;
    view->fog_base = view->fog_near_int - view->fog_near_dist / 1024.0f * fog_distance_slope;
    view->fog_mul = fog_multiplier;
    view_fog_multiplier = fog_multiplier;
    view_fog_multiplier_copy[0] = fog_multiplier;
    view_fog_multiplier_copy2[0] = fog_multiplier;
    mul_bits = *(s32 *)&view->fog_mul;
    tfrag_vu_const_block.mul = mul_bits;
    tfrag_vu_const_block.far_int = far_bits;
    tfrag_vu_const_block.near_int = near_bits;
    shrub_vu1_constants.mul = mul_bits;
    shrub_vu1_constants_alt.mul = mul_bits;
    shrub_vu1_constants.far_int = far_bits;
    shrub_vu1_constants.near_int = near_bits;
    shrub_vu1_constants_alt.far_int = far_bits;
    shrub_vu1_constants_alt.near_int = near_bits;
    shrub_vu1_constants.max = 0x437EFFFF;
    shrub_vu1_constants_alt.max = 0x437EFFFF;
    set_tfrag_dists();

    projection_near_clip = view->near_clip;
    far_clip = view->far_clip;
    horizontal_fov = view->fov.f[0];
    vertical_fov = view->fov.f[1];
    viewport_width = view->half_width;
    viewport_height = view->half_height;
    projection_fog_multiplier = view->fog_mul;
    view_depth_constants[0] = 0.5f / 210000.0f;
    view_depth_constants[1] = -0.5f / 210000.0f;
    view_depth_constants[2] = 0.5f;
    view_depth_constants[3] = viewport_width / (horizontal_fov * 210000.0f);
    view->proj[0] = viewport_width / (horizontal_fov * projection_near_clip);
    view->proj[1] = 0;
    view->proj[2] = 0;
    view->proj[3] = 0;
    view->proj[4] = 0;
    view->proj[5] = viewport_height / (vertical_fov * projection_near_clip);
    view->proj[6] = 0;
    view->proj[7] = 0;
    view->proj[8] = 0;
    view->proj[9] = 0;
    view->proj[10] = (far_clip + projection_near_clip) /
                     (projection_near_clip * (far_clip - projection_near_clip)) * depth_scale;
    view->proj[11] = 1.0f / projection_near_clip * projection_fog_multiplier;
    view->proj[12] = 0;
    view->proj[13] = 0;
    view->proj[14] = -2.0f * projection_near_clip * far_clip /
                     (projection_near_clip * (far_clip - projection_near_clip)) * depth_scale;
    view->proj[15] = 0;
    copy_memory_aligned(view->proj2, view->proj, 0x40);
    view->proj2[11] = 1.0f / view->near_clip;
    copy_memory_aligned(view->proj3, view->proj2, 0x40);
    view->proj3[0] /= view->scr_x;
    view->proj3[5] /= view->scr_y;
    view->proj3[10] /= depth_scale;
    view->proj3[14] /= depth_scale;
    view->inverse_screen_scale.f[0] = 1.0f / view->scr_x;
    view->inverse_screen_scale.f[1] = 1.0f / view->scr_y;
    view->inverse_screen_scale.f[2] = 1.0f / depth_scale;
    view->inverse_screen_scale.f[3] = 1.0f / view->fog_mul;
    view->screen_scale.f[0] = view->scr_x;
    view->screen_scale.f[1] = view->scr_y;
    view->screen_scale.f[2] = depth_scale;
    view->screen_scale.f[3] = view->fog_mul;
    view->inverse_projection_scale.f[0] = view->scr_x / view->proj[0];
    view->inverse_projection_scale.f[1] = view->scr_y / view->proj[5];
    view->inverse_projection_scale.f[2] = depth_scale / view->proj[10];
    view->inverse_projection_scale.f[3] = view->fog_mul;
    view->screen_bias.f[0] = 2048.0f;
    view->screen_bias.f[1] = 2048.0f;
    view->screen_bias.f[2] = depth_bias;
    view->screen_bias.f[3] = view->fog_add;
    view->viewport_aspect.f[0] = view->scr_x / view->half_width;
    view->viewport_aspect.f[1] = view->scr_y / view->half_height;
    view->viewport_aspect.f[2] = 1.0f;
    view->viewport_aspect.f[3] = 1.0f;
    view->fog_color = *(s32 *)&view->fog_far_int;
    view->unk4 = 0x103E4000;
    view->unk8 = 0xE;
    view->unkC = 0;
    qzero(&view->regs[0]);

    packet_view = &view_context_p;
    packet_view->gif[0].i[0] = *(s32 *)&packet_view->fog_mul;
    packet_view->gif[0].i[1] = 0x303E4000;
    packet_view->gif[0].i[2] = 0x412;
    packet_view->gif[0].i[3] = 0x303EC000;
    packet_view->gif[1].i[0] = *(s32 *)&packet_view->fog_mul;
    packet_view->gif[1].i[1] = 0x302E4000;
    packet_view->gif[1].i[2] = 0x412;
    packet_view->gif[1].i[3] = 0x302EC000;
    packet_view->gif[2].i[0] = *(s32 *)&packet_view->fog_mul;
    packet_view->gif[2].i[1] = 0x20364000;
    packet_view->gif[2].i[2] = 0x41;
    packet_view->gif[2].i[3] = 0;
    packet_view->gif[3].i[0] = *(s32 *)&packet_view->fog_mul;
    packet_view->gif[3].i[1] = 0x20264000;
    packet_view->gif[3].i[2] = 0x41;
    packet_view->gif[3].i[3] = 0;
    qcopy(&packet_view->regs[0], &packet_view->gif[0]);
    qcopy(&packet_view->regs[1], &packet_view->viewport_aspect);
    qcopy(&packet_view->regs[2], &packet_view->inverse_screen_scale);
    qcopy(&packet_view->regs[3], &packet_view->screen_scale);
    qcopy(&packet_view->regs[4], &packet_view->screen_bias);
    qcopy(&vu1_view_screen_consts[0], &packet_view->screen_bias);
    qcopy(&vu1_view_screen_consts[1], &packet_view->screen_scale);
    qcopy(&vu1_view_screen_consts[2], &packet_view->viewport_aspect);
    qcopy(&tfrag_vu_const_block.constants[0], &packet_view->screen_bias);
    qcopy(&tfrag_vu_const_block.constants[1], &packet_view->inverse_screen_scale);
    qcopy(&tfrag_vu_const_block.constants[2], &packet_view->screen_scale);
    tfrag_vu_const_block.x = packet_view->viewport_aspect.i[0];
    tfrag_vu_const_block.y = packet_view->viewport_aspect.i[1];
    qcopy(&shrub_vu1_constants_alt_screen[0], &packet_view->screen_scale);
    qcopy(&shrub_vu1_constants_alt_screen[1], &packet_view->screen_bias);
}

#undef view_context_p
#undef pal_mode_p
#undef fast_cos
#undef set_tfrag_dists

extern int g_screen_extent[];
extern float IntToFloat(int);

/* Sets the screen size: the GS viewport record at D_0013E600 gets width,
   height, their halves and the four <<4 edges around the 0x800 centre;
   the draw context at D_0018CE00 gets a2 at +0xB0, 32 and 524288 at
   +0xA0/+0xA4, the half extents as floats (func_001FA888 is int to
   float) at +0x200/+0x204, four times each at +0x208/+0x20C, and a3..a6
   at +0x218/+0x21C/+0x228/+0x22C; then UpdateViewContext (func_001F3140).
   The context is reached through one local pointer, which keeps its
   full address in $s0 as retail does (indexing the global directly
   folds +0xB0 into the base and spends $a0 on the %hi). */
void configure_graphics_projection(int w, int h, float a2, float a3, float a4, float a5, float a6) {
    float *ctx = (float *)view_context;
    int hw = w >> 1;
    int hh = h >> 1;
    float fh;

    g_screen_extent[0] = w;
    g_screen_extent[1] = h;
    g_screen_extent[2] = hw;
    g_screen_extent[3] = hh;
    g_screen_extent[4] = (0x800 - hw) << 4;
    g_screen_extent[5] = (0x800 - hh) << 4;
    g_screen_extent[6] = (hw + 0x800) << 4;
    g_screen_extent[7] = (hh + 0x800) << 4;
    ctx[0x2C] = a2;
    ctx[0x28] = 32.0f;
    ctx[0x29] = 524288.0f;
    ctx[0x80] = IntToFloat(w) * 0.5f;
    fh = IntToFloat(h) * 0.5f;
    ctx[0x81] = fh;
    ctx[0x82] = ctx[0x80] * 4.0f;
    ctx[0x83] = fh * 4.0f;
    ctx[0x86] = a3;
    ctx[0x87] = a4;
    ctx[0x8A] = a5;
    ctx[0x8B] = a6;
    UpdateViewContext();
}

typedef struct {
    char disp_and_contexts[0x150];
    s16 display_width;
    s16 display_height;
    s16 psm;
    s16 fbp0;
    s16 storage_width;
    s16 storage_height;
} FsAaBuf;

typedef struct {
    int width;
    int height;
    int half_width;
    int half_height;
    int left;
    int top;
    int right;
    int bottom;
} ScreenExtent;

typedef struct {
    u64 pad0[2];
    u64 frame1;
    u64 pad18;
    u64 frame2;
    u64 pad28;
    u64 zbuf1;
    u64 pad38;
    u64 zbuf2;
    u64 pad48;
    u64 xyoffset1;
    u64 pad58;
    u64 xyoffset2;
    u64 pad68;
    u64 scissor1;
    u64 pad78;
    u64 scissor2;
} GsDrawEnvRegs;

typedef struct {
    char b[0x60];
} GsLoadImage;

extern FsAaBuf fs_aa_buffer_3890 __asm__("D_00151880");
extern ScreenExtent screen_extent_3890 __asm__("D_0013E600");
extern GsDrawEnvRegs draw_env_3890 __asm__("D_0013D010");
extern u64 depth_reg_3890 __asm__("D_0013D200");
extern u64 masked_depth_reg_3890 __asm__("D_0013D270");
extern unsigned char clear_buf_3890[] __asm__("D_001942C0");
extern int g_gs_texture_allocation_cursor MACRO_ADDR;
extern int display_buffer_address MACRO_ADDR;
extern int draw_buffer_address MACRO_ADDR;
extern int depth_buffer_address MACRO_ADDR;
extern int g_gs_texture_allocation_base MACRO_ADDR;
extern void flush_cache(int);
extern int wait_for_graphics_pipeline_idle(int, int);
extern void SetupFS_AA_buffer(int, int, int, int, int, int);
extern void PutDrawBufferLarge(void);
extern void append_gif_transfer_packet(void);
extern void PutDispBuffer(void);
extern int sceGsSetDefLoadImage(GsLoadImage *, s16, s16, s16, s16, s16, s16, s16);
extern int sceGsExecLoadImage(GsLoadImage *, unsigned char *);

/* Adapted from Lombyte's project (GPL-3.0): src/video/display/set_pal_mode.c, set_pal_mode. */
void SetPalMode(void) {
    GsLoadImage image;
    int n;
    int i;
    u64 zbuf;
    u64 frame;
    u64 scissor;
    s16 w;

    flush_cache(0);
    if (pal_mode != 0) {
        draw_buffer_address = 0x100000;
        depth_buffer_address = 0x1E0000;
        display_buffer_address = 0;
        g_gs_texture_allocation_base = 0x2C0000;
        SetupFS_AA_buffer(0x200, 0x1C0, 0x200, 0x200, 4, 0);
    } else {
        draw_buffer_address = 0xE0000;
        depth_buffer_address = 0x1B0000;
        display_buffer_address = 0;
        g_gs_texture_allocation_base = 0x280000;
        SetupFS_AA_buffer(0x200, 0x1A0, 0x200, 0x1C0, 0, 0);
    }
    w = fs_aa_buffer_3890.display_width;
    screen_extent_3890.width = w;
    screen_extent_3890.half_width = w >> 1;
    screen_extent_3890.half_height = (screen_extent_3890.height = fs_aa_buffer_3890.display_height) >> 1;
    screen_extent_3890.left = (0x800 - screen_extent_3890.half_width) << 4;
    screen_extent_3890.top = (0x800 - screen_extent_3890.half_height) << 4;
    screen_extent_3890.right = (screen_extent_3890.half_width + 0x800) << 4;
    screen_extent_3890.bottom = (screen_extent_3890.half_height + 0x800) << 4;
    flush_cache(0);
    wait_for_graphics_pipeline_idle(0, 0);
    frame = (draw_buffer_address >> 13) | ((u64)(screen_extent_3890.width >> 6) << 16);
    g_gs_texture_allocation_cursor = g_gs_texture_allocation_base;
    draw_env_3890.frame1 = frame;
    g_gs_texture_allocation_start = g_gs_texture_allocation_base;
    zbuf = (depth_buffer_address >> 13) | 0x1000000;
    draw_env_3890.frame2 = frame;
    draw_env_3890.xyoffset1 = screen_extent_3890.left | ((u64)screen_extent_3890.top << 32);
    draw_env_3890.zbuf1 = zbuf;
    draw_env_3890.zbuf2 = zbuf;
    draw_env_3890.xyoffset2 = screen_extent_3890.left | ((u64)screen_extent_3890.top << 32);
    scissor = ((u64)(screen_extent_3890.width - 1) << 16) | ((u64)(screen_extent_3890.height - 1) << 48);
    depth_reg_3890 = zbuf;
    draw_env_3890.scissor1 = scissor;
    draw_env_3890.scissor2 = scissor;
    masked_depth_reg_3890 = zbuf | ((u64)0x8000 << 17);
    flush_cache(0);
    PutDrawBufferLarge();
    append_gif_transfer_packet();
    flush_cache(0);
    wait_for_graphics_pipeline_idle(0, 0);
    PutDispBuffer();
    FastMemSet(clear_buf_3890, 0, 0x1000);
    n = (fs_aa_buffer_3890.storage_width * fs_aa_buffer_3890.storage_height) >> 10;
    for (i = 0; i < n; i++) {
        sceGsSetDefLoadImage(&image, i << 4, 1, 0, 0, 0, 32, 32);
        flush_cache(0);
        sceGsExecLoadImage(&image, clear_buf_3890);
        wait_for_graphics_pipeline_idle(0, 0);
    }
}

/*
 * ResetDrawGlobals. Seventeen zero stores in a row, in three addressing
 * forms that are all one assembler macro: lui/$at for most, plain $gp
 * for the three that really are small-data, and $gp again for the last
 * one because it lands in the jr delay slot where a two-instruction
 * expansion will not fit.
 */
extern int screen_tint_quad_enabled MACRO_ADDR;
extern int screen_tint_quad_rgba MACRO_ADDR;
extern short screen_overlay_enabled;              /* SDA, gp -0x78B4 */
extern short draw_unused_reset_flag;              /* SDA, gp -0x78A0 */
extern short occl_debug_overlay_enabled;              /* SDA, gp -0x7890 */
extern int letterbox_enabled MACRO_ADDR;
extern int letterbox_bar_height MACRO_ADDR;
extern int draw_callback_count MACRO_ADDR;
extern int draw_callback2_count MACRO_ADDR;
extern int draw_callback3_count MACRO_ADDR;
extern int draw_callback4_count MACRO_ADDR;
extern int effect_quad_count MACRO_ADDR;
extern int surface_height_tiles MACRO_ADDR;
extern int occl_cell_table MACRO_ADDR;
extern int occl_cell_count MACRO_ADDR;
extern int local_surface_region_enabled MACRO_ADDR;

/* ResetDrawGlobals: clears the draw-callback counts and related draw counters. */
void ResetDrawGlobals(void) {
    draw_callback_count = 0;
    draw_callback3_count = 0;
    draw_callback4_count = 0;
    draw_callback2_count = 0;
    effect_quad_count = 0;
    screen_tint_quad_enabled = 0;
    screen_tint_quad_rgba = 0;
    letterbox_enabled = 0;
    letterbox_bar_height = 0;
    *(int *)&screen_overlay_enabled = 0;
    *(int *)&draw_unused_reset_flag = 0;
    *(int *)&occl_debug_overlay_enabled = 0;
    occl_fallback_mode = 0;
    surface_height_tiles = 0;
    occl_cell_table = 0;
    occl_cell_count = 0;
    local_surface_region_enabled = 0;
}

extern int *g_render_packet_cursor MACRO_ADDR;
extern void VU1_addGSregister(int, long);
extern char reset_gs_regs_fixed[];
extern char draw_environment[];

/* ResetGsRegisters(void) */
void ResetGsRegisters(void) {
    g_render_packet_cursor[0] = 0x30000013;
    g_render_packet_cursor[1] = (int)reset_gs_regs_fixed;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000013;
    g_render_packet_cursor += 4;
    g_render_packet_cursor[0] = 0x3000000B;
    g_render_packet_cursor[1] = (int)draw_environment;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x5000000B;
    g_render_packet_cursor += 4;
    VU1_addGSregister(0x3D, (long)view_context[0x8C] | ((long)view_context[0x8D] << 8) |
                        ((long)view_context[0x8E] << 16));
}

extern long gs_display_settings[3];

/* GS privileged-register writes (0x1200_00XX = the GS's memory-mapped
   register block): CSR ack, PMODE, then SMODE2/DISPFB1/DISPFB2/DISPLAY1/
   DISPLAY2/BGCOLOR set from a 3-entry table. */
/* ResetGsRegistersPr(void) */
void ResetGsRegistersPr(void) {
    *(volatile long *)0x120000E0 = 0;
    *(volatile long *)0x12000000 = 0xFFA1;
    *(volatile long *)0x12000020 = gs_display_settings[0];
    *(volatile long *)0x12000070 = gs_display_settings[1];
    *(volatile long *)0x12000090 = gs_display_settings[1];
    *(volatile long *)0x12000080 = gs_display_settings[2];
    *(volatile long *)0x120000A0 = gs_display_settings[2];
    *(volatile long *)0x120000D0 = 0;
}

extern short screen_color_effect_now;             /* SDA, gp -0x78B0: Stripes * */
extern char *level_sky_effect_data MACRO_ADDR;
extern int draw_force_gif_transfer[];
extern int draw_enable_mask_f __asm__("D_0015F534") MACRO_ADDR;
extern int font_state MACRO_ADDR;
extern int g_game_mode MACRO_ADDR;
extern unsigned char subtitles_enabled_r[] __asm__("D_0015EF40") MACRO_ADDR;
extern float gFade MACRO_ADDR;
extern float white_fade_level MACRO_ADDR;
extern float draw_frame_time_ratio MACRO_ADDR;
extern unsigned char underwater_screen_tint_r_r[] __asm__("D_001611C0") MACRO_ADDR;
extern unsigned char underwater_screen_tint_g_r[] __asm__("D_001611C1") MACRO_ADDR;
extern unsigned char underwater_screen_tint_b_r[] __asm__("D_001611C2") MACRO_ADDR;
extern unsigned char underwater_screen_tint_a_r[] __asm__("D_001611C3") MACRO_ADDR;
extern unsigned short moby_vu1_data_qwc NOT_SDA;
extern char moby_vu1_data[];
extern char profile_mark_frame_start[];
extern char profile_mark_after_sky[];
extern char profile_mark_after_ties[];
extern char profile_mark_after_mobys[];
extern char profile_mark_after_lights[];
extern char profile_mark_after_particles[];
extern char profile_mark_after_sprites[];
extern char profile_mark_after_blur[];
extern char profile_mark_after_hud[];
extern char profile_mark_after_fades[];
extern char profile_mark_vu1_sync[];
extern char profile_mark_tie_lighting[];
extern char profile_mark_moby_patch[];
extern char profile_mark_tfrag_lighting[];
extern char profile_mark_shrub_lighting[];
extern char frame_vu0_micro_program[];
extern char tfrag_dma_block[];
extern char tie_light_data[];
extern char shrub_scratchpad_copy[];

extern void ProfileMarker_End_a(void *, int) __asm__("func_001F2558");
extern void ProfileMarker_Begin_a(void *, int) __asm__("func_001F2560");
extern void UpdateFog_v(void) __asm__("func_001F2930");
extern void append_gif_transfer_packet(void);
extern void BuildViewMatrices(void);
extern void prune_moby_references(void);
extern void Transition_DrawSky(void);
extern void DrawTfrag(void);
extern void append_dma_tag(int);
extern void DrawTies_2(void);
extern void DrawTies_1(void);
extern void vu1_gs_regs_alt(void);
extern void SetupGifPaging(int);
extern void dispatch_callback_list_2(void);
extern void DoGifPaging(void);
extern void VU1_gsRegsNormal(void);
extern void DrawShrubs(void);
extern void draw_fogged_fullscreen_sprite(char *);
extern void draw_debug_font(void);
extern void dispatch_callback_list_3(void);
extern void DrawMobys(void);
extern void VU1_addDataRef(void *, int);
extern void dispatch_callback_list_1(void);
extern void draw_lens_flare(void);
extern void draw_light_quads(void);
extern void flush_cache(int);
extern void PartProc(void);
extern void dispatch_callback_list_4(void);
extern void append_billboard_batch(void);
extern void render_queued_rotated_sprites(void);
extern void AA_BlurPass(void);
extern void DebugProfiler_Hook(void);
extern void update_hud(void);
extern void draw_help(void);
extern void draw_letterbox_bars(void);
extern void draw_subtitles(void);
extern void emit_rgba_draw_packet(int, int, int, int);
extern void draw_screen_effect(void);
extern void VU0_loadMicroProgram(void *);
extern void VU1_syncChain(int mask);
extern void LightTfrags(void *);
extern void PatchTfragGifs(void);
extern void register_entity_render_resources(void);
extern void copy_render_buffer_pair(void);
extern void LightTies(void *);
extern void PatchTieGifs(void);
extern void LightShrubs(void *);
extern void patch_shrub_gifs(void);
extern void PatchMobyGifs(void);

/* Draws the whole frame: each world layer's DMA chain in order, then the fades and the per-layer profiler bars. Adapted from Lombyte's project (GPL-3.0) for PAL: rendering/draw_debug_profiler.c, draw_debug_profiler. */
void DrawDebugProfiler(void) {
    float t;
    float div;

    if (level_sky_effect_data == 0 || *(short *)(level_sky_effect_data + 4) != 0 || ((draw_enable_mask_f ^ 1) & 1) || draw_config[2] == 0 ||
        draw_force_gif_transfer[0] != 0) {
        append_gif_transfer_packet();
    }
    BuildViewMatrices();
    prune_moby_references();
    UpdateOcclusion();
    ResetGsRegisters();
    font_state = -1;
    ProfileMarker_End_a(profile_mark_frame_start, 0xF);
    ProfileMarker_Begin_a(profile_mark_frame_start, 0xF);
    if (level_sky_effect_data != 0 && (draw_enable_mask_f & 1)) {
        if (draw_config[2] != 0) {
            Transition_DrawSky();
        }
        ProfileMarker_Begin_a(profile_mark_after_sky, 0xE);
        ProfileMarker_End_a(profile_mark_after_sky, 0xE);
    }
    if (draw_enable_mask_f & 2) {
        DrawTfrag();
    }
    append_dma_tag(0x02010000);
    if (draw_enable_mask_f & 4) {
        if (pal_mode != 0) {
            DrawTies_2();
        } else {
            DrawTies_1();
        }
    }
    append_dma_tag(0x02020000);
    if (draw_config[0x34 / 4] != 0 && draw_callback3_count != 0) {
        vu1_gs_regs_alt();
        SetupGifPaging(1);
        dispatch_callback_list_2();
        DoGifPaging();
        VU1_gsRegsNormal();
    }
    ProfileMarker_End_a(profile_mark_after_ties, 6);
    ProfileMarker_Begin_a(profile_mark_after_ties, 6);
    if (draw_enable_mask_f & 8) {
        DrawShrubs();
    }
    append_dma_tag(0x02040000);
    if (draw_config[0x44 / 4] != 0 && *(int *)&occl_debug_overlay_enabled != 0) {
        draw_fogged_fullscreen_sprite((char *)&occl_debug_overlay_enabled);
    }
    if (draw_enable_mask_f & 0x20) {
        SetupGifPaging(1);
        draw_debug_font();
        DoGifPaging();
    }
    if (draw_config[0x34 / 4] != 0 && draw_callback4_count != 0) {
        vu1_gs_regs_alt();
        SetupGifPaging(1);
        dispatch_callback_list_3();
        DoGifPaging();
        VU1_gsRegsNormal();
    }
    if (draw_enable_mask_f & 0x10) {
        DrawMobys();
    }
    append_dma_tag(0x02080000);
    SetupGifPaging(0);
    if ((draw_enable_mask_f & 0x20) && draw_config[0x30 / 4] != 0 && font_state != 6) {
        VU1_addDataRef(moby_vu1_data, moby_vu1_data_qwc);
        font_state = 6;
    }
    ProfileMarker_Begin_a(profile_mark_after_mobys, 4);
    ProfileMarker_End_a(profile_mark_after_mobys, 4);
    if (draw_enable_mask_f & 0x20) {
        vu1_gs_regs_alt();
        if (draw_config[0x34 / 4] != 0) {
            if (draw_callback_count != 0) {
                dispatch_callback_list_1();
            }
            VU1_addGSregister(0x42, 0x8000000048L);
            draw_lens_flare();
            vu1_gs_regs_alt();
            draw_light_quads();
        }
        ProfileMarker_End_a(profile_mark_after_lights, 6);
        ProfileMarker_Begin_a(profile_mark_after_lights, 6);
        if (draw_config[0x38 / 4] != 0) {
            VU1_addGSregister(8, 5);
            vu1_gs_regs_alt();
            flush_cache(0);
            PartProc();
            font_state = 8;
        }
        ProfileMarker_End_a(profile_mark_after_particles, 8);
        ProfileMarker_Begin_a(profile_mark_after_particles, 8);
        if (draw_config[0x3C / 4] != 0) {
            if (draw_callback2_count != 0) {
                vu1_gs_regs_alt();
                dispatch_callback_list_4();
            }
            if (g_game_mode == 0) {
                append_billboard_batch();
            }
            VU1_addGSregister(0x42, 0x8000000044L);
            render_queued_rotated_sprites();
        }
        ProfileMarker_End_a(profile_mark_after_sprites, 6);
        ProfileMarker_Begin_a(profile_mark_after_sprites, 6);
    }
    if (draw_config[0x48 / 4] != 0) {
        AA_BlurPass();
    }
    ProfileMarker_Begin_a(profile_mark_after_blur, 0xF);
    vu1_gs_regs_alt();
    if (draw_enable_mask_f & 0x10000) {
        DebugProfiler_Hook();
    }
    if ((draw_enable_mask_f & 0x80) && draw_config[0x40 / 4] != 0) {
        update_hud();
        draw_help();
        draw_letterbox_bars();
    }
    if (g_game_mode == 2 && subtitles_enabled_r[0] != 0) {
        draw_subtitles();
    }
    ProfileMarker_Begin_a(profile_mark_after_hud, 0xE);
    ProfileMarker_End_a(profile_mark_after_hud, 0xE);
    DoGifPaging();
    if (draw_enable_mask_f & 0x40) {
        if (draw_config[0x44 / 4] != 0) {
            VU1_addGSregister(0x42, 0x8000000044L);
            if (use_alt_fog != 0) {
                emit_rgba_draw_packet(underwater_screen_tint_r_r[0], underwater_screen_tint_g_r[0], underwater_screen_tint_b_r[0], underwater_screen_tint_a_r[0]);
            }
            if (gFade > 0.0f) {
                if (gFade > 1.0f) {
                    gFade = 1.0f;
                }
                emit_rgba_draw_packet(0, 0, 0, truncate_float_to_s32_r(gFade * 128.0f));
            }
            if (white_fade_level > 0.0f) {
                if (white_fade_level > 1.0f) {
                    white_fade_level = 1.0f;
                }
                emit_rgba_draw_packet(0xFF, 0xFF, 0xFF, truncate_float_to_s32_r(white_fade_level * 128.0f));
            }
            if (*(int *)&screen_overlay_enabled != 0 && *(int *)&screen_color_effect_now != 0) {
                draw_screen_effect();
            }
        }
        ProfileMarker_Begin_a(profile_mark_after_fades, 0xA);
    }
    VU0_loadMicroProgram(frame_vu0_micro_program);
    flush_cache(0);
    t = IntToFloat(*(volatile int *)0x10000800);
    draw_frame_time_ratio = t / (pal_mode != 0 ? 11520.0f : 9600.0f);
    VU1_syncChain(2);
    ProfileMarker_End_a(profile_mark_vu1_sync, 0x11);
    if (draw_enable_mask_f & 2) {
        if (draw_config[0x10 / 4] != 0) {
            LightTfrags(tfrag_dma_block);
            PatchTfragGifs();
        }
        ProfileMarker_End_a(profile_mark_tfrag_lighting, 2);
    }
    VU1_syncChain(4);
    ProfileMarker_End_a(profile_mark_vu1_sync, 0x11);
    if (draw_enable_mask_f & 4) {
        if (draw_config[0x18 / 4] != 0) {
            if (pal_mode != 0) {
                register_entity_render_resources();
                copy_render_buffer_pair();
            } else {
                LightTies(tie_light_data);
                PatchTieGifs();
            }
        }
        ProfileMarker_End_a(profile_mark_tie_lighting, 5);
    }
    VU1_syncChain(8);
    ProfileMarker_End_a(profile_mark_vu1_sync, 0x11);
    if (draw_enable_mask_f & 8) {
        if (draw_config[0x20 / 4] != 0) {
            LightShrubs(shrub_scratchpad_copy);
            patch_shrub_gifs();
        }
        ProfileMarker_End_a(profile_mark_shrub_lighting, 7);
    }
    VU1_syncChain(0x10);
    ProfileMarker_End_a(profile_mark_vu1_sync, 0x11);
    if (draw_enable_mask_f & 0x10) {
        if (draw_config[0x28 / 4] != 0) {
            PatchMobyGifs();
        }
        ProfileMarker_End_a(profile_mark_moby_patch, 3);
    }
    UpdateFog_v();
    occl_fallback_mode = 0;
}

extern int space_load_in_progress;
extern short draw_enable_mask;              /* SDA, gp -0x77CC */
extern void DrawDebugProfiler(void);


/* D_0015F6FC is read through a MACRO_ADDR alias: retail's one-register
   lui $2 / lw $2 (an older note filed it as an allocator question). */
void prepare_debug_profiler_render(void) {
    if (space_load_in_progress_m == 0) {
        append_gif_transfer_packet();
        *(int *)&draw_enable_mask = 0x7F;
        DrawDebugProfiler();
    }
}

LINKER_REMNANT("asm/remnants/text", func_001F4628);

extern int *g_render_packet_cursor MACRO_ADDR;
extern int *gif_paging_head_tag MACRO_ADDR;
extern int g_gs_texture_allocation_cursor MACRO_ADDR;
extern int effect_tex_cnt MACRO_ADDR;
extern int g_gs_texture_allocation_base MACRO_ADDR;
extern int pending_texture_upload_count MACRO_ADDR;
typedef struct {
    long unk0;
    long unk8;
} PageSlot;
extern PageSlot effect_texture_definitions[];
extern char hud_state[];

/* Returns the HUD state block that serves as the GIF paging arena. */
static inline char *PagingArena(void) {
    return hud_state;
}

/* SetupGifPaging(int): marks the D_00161000 packet in D_0015F550 and
   reserves 0x10 bytes, copies D_0015EF78 to D_0015EF74, clears
   D_0015F558 and the first dword of the D_0015F55C paging slots, then,
   when arg0 is 0, clears the +4 half of the arena's 8-byte list entries:
   the list at D_0019A4E8 + 0x24 (count at +0x44 of the record at +0x18)
   where it is at least D_0015EF8C >> 8, and all of the list at + 0x28
   (count at +0x24). The slot loop has its own counter, which loop
   reversal copies from the count (retail's $v1). The first list reads
   the arena through a static inline accessor (a fresh pseudo per read
   gives retail's copy for the loop) with the element offset first
   (`i * 8 + base`); the second through a block-local pointer, whose
   %hi retail keeps. */
void SetupGifPaging(int arg0) {
    int *p = g_render_packet_cursor;
    int i;

    gif_paging_head_tag = p;
    p += 4;
    g_render_packet_cursor = p;
    g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
    pending_texture_upload_count = 0;
    {
        int k;

        for (k = 0; k < effect_tex_cnt; k++) {
            effect_texture_definitions[k].unk0 = 0;
        }
    }
    if (arg0 == 0) {
        for (i = 0; i < *(int *)(*(char **)(PagingArena() + 0x18) + 0x44); i++) {
            unsigned short *el = (unsigned short *)(i * 8
                + *(int *)(PagingArena() + 0x24) + 4);

            if (*el >= (g_gs_texture_allocation_base >> 8)) {
                *el = 0;
            }
        }
        {
            char *b = hud_state;
            int j;

            for (j = 0; j < *(int *)(*(char **)(b + 0x18) + 0x24); j++) {
                *(short *)(*(char **)(b + 0x28) + j * 8 + 4) = 0;
            }
        }
    }
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

extern int *g_render_packet_cursor MACRO_ADDR;
extern int *gif_paging_tail_tag MACRO_ADDR;
extern int gif_paging_texture_flush_enabled;
extern void flush_pending_texture_uploads(void);
extern void VU1_texFlush(void);

/* DoGifPaging: pushes two 4-word GIF tags (0x20000000 in the first word)
   onto the D_00161000 packet, D_0015F554 marking where it started and
   D_0015F550's tag pointing at the second one; between the two, when
   D_0018A3DC is set, func_0020C2F8 and func_00234E80 add their own.
   The first advance goes through a local advanced in place, which keeps
   the old and new pointer in one register as retail does. */
void DoGifPaging(void) {
    int *p = g_render_packet_cursor;

    gif_paging_tail_tag = p;
    p += 4;
    g_render_packet_cursor = p;
    gif_paging_head_tag[0] = 0x20000000;
    gif_paging_head_tag[1] = (int)g_render_packet_cursor;
    gif_paging_head_tag[2] = 0;
    gif_paging_head_tag[3] = 0;
    if (gif_paging_texture_flush_enabled != 0) {
        flush_pending_texture_uploads();
        VU1_texFlush();
    }
    g_render_packet_cursor[0] = 0x20000000;
    g_render_packet_cursor[1] = (int)(gif_paging_head_tag + 4);
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0;
    g_render_packet_cursor += 4;
    gif_paging_tail_tag[0] = 0x20000000;
    gif_paging_tail_tag[1] = (int)g_render_packet_cursor;
    gif_paging_tail_tag[2] = 0;
    gif_paging_tail_tag[3] = 0;
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

typedef struct EffectTexDef {
    long tex0;
    unsigned short texel_off;
    unsigned short pal_off;
    short wlog2;
    short hlog2;
} EffectTexDef;
typedef struct TexUpload {
    int clut_data;
    short unk4;
    short cbp;
    int image_data;
    unsigned char tw;
    unsigned char th;
    short tbp;
} TexUpload;
struct EffectTextureDefinition_et {
    u64 tex0;
    u16 texel_offset_quadwords;
    u16 palette_offset_quadwords;
    s16 width_log2;
    s16 height_log2;
};
struct TextureUpload_et {
    s32 clut_data;
    s16 unk4;
    s16 cbp;
    s32 image_data;
    u8 tw;
    u8 th;
    s16 tbp;
};

extern struct EffectTextureDefinition_et effect_texture_definitions_et[] __asm__("D_0018D540");
extern struct TextureUpload_et pending_texture_uploads_et[0x40] __asm__("D_0018D140");
extern s32 pending_texture_upload_count_et __asm__("D_0015F558") MACRO_ADDR;
extern s32 level_texture_payload_address_et __asm__("D_0015F560") MACRO_ADDR;
extern s32 gs_texture_allocation_cursor_et __asm__("D_0015EF74") MACRO_ADDR;

/* GetEffectTex: the GS TEX0 word of effect texture slot index. The first
   call for a slot allocates its CLUT and texels from the GS texture cursor,
   builds and caches the word, and, while fewer than 64 uploads are
   pending, queues the slot's CLUT and pixels for upload.
   Adapted from Lombyte's project (GPL-3.0): src/rendering/effects/get_effect_texture.c, get_effect_texture. */
u64 func_001F4868_r(s32 index) __asm__("func_001F4868");

/* GetEffectTex with its real 64-bit return: builds and returns the GS TEX0 word for an effect texture. */
u64 func_001F4868_r(s32 index) {
    struct EffectTextureDefinition_et *texture;
    struct TextureUpload_et *upload;
    s32 width_log2;
    s32 buffer_width_shift;
    s32 palette_block_offset;
    s32 texel_address;
    s32 texel_block_offset;
    u64 tex0_word;
    s64 width_bits;
    s64 palette_bits;
    s32 pending_texture_upload_count_snapshot;

    if (effect_texture_definitions_et[index].tex0 == 0) {
        texture = &effect_texture_definitions_et[index];
        width_log2 = texture->width_log2;
        buffer_width_shift = width_log2 - 6;
        palette_block_offset = gs_texture_allocation_cursor_et >> 8;
        texel_address = gs_texture_allocation_cursor_et + 0x400;
        texel_block_offset = texel_address >> 8;
        pending_texture_upload_count_snapshot = pending_texture_upload_count_et;
        tex0_word = ((u64)(1 << ((buffer_width_shift <= -1) ? 0 : buffer_width_shift)) << 14);
        tex0_word |= texel_block_offset;
        width_bits = ((s64)((u64)(u16)texture->width_log2 << 48) >> 22) | 0x1300000;
        tex0_word |= width_bits;
        tex0_word |= (s64)((u64)(u16)texture->height_log2 << 48) >> 18;
        palette_bits = (u64)palette_block_offset << 37;
        palette_bits |= (u64)1 << 34;
        width_log2 += texture->height_log2;
        tex0_word |= palette_bits;
        tex0_word |= (u64)1 << 63;
        gs_texture_allocation_cursor_et = texel_address + (1 << width_log2);
        texture->tex0 = tex0_word;
        if (pending_texture_upload_count_snapshot < 0x40) {
            upload = &pending_texture_uploads_et[pending_texture_upload_count_snapshot];
            upload->clut_data =
                level_texture_payload_address_et + texture->palette_offset_quadwords * 0x10;
            upload->cbp = palette_block_offset;
            upload->unk4 = 0;
            *(s32 *)((u8 *)pending_texture_uploads_et + pending_texture_upload_count_snapshot * 0x10 +
                     8) = level_texture_payload_address_et + texture->texel_offset_quadwords * 0x10;
            upload->tw = (u8)texture->width_log2;
            upload->th = (u8)texture->height_log2;
            upload->tbp = texel_block_offset;
            pending_texture_upload_count_et = pending_texture_upload_count_snapshot + 1;
        }
    }
    return effect_texture_definitions_et[index].tex0;
}

/*
 * Four parallel callback lists, each a (function, argument) pair of
 * arrays with its own count, plus a "register" and a "run them all"
 * function per list. The counts are reached with retail's one-register
 * macro form, so they are MACRO_ADDR.
 */
typedef void (*DrawCallback)(void *);
extern DrawCallback draw_callback_funcs[];
extern void *draw_callback_args[];
extern DrawCallback draw_callback2_funcs[];
extern void *draw_callback2_args[];
extern DrawCallback draw_callback3_funcs[];
extern void *draw_callback3_args[];
extern DrawCallback draw_callback4_funcs[];
extern void *draw_callback4_args[];

/* AddDrawCallback: appends fn(arg) to the first draw-callback list. */
void AddDrawCallback(DrawCallback fn, void *arg) {
    int count = draw_callback_count;
    if (count < 0x40) {
        draw_callback_funcs[count] = fn;
        draw_callback_args[count] = arg;
        draw_callback_count = count + 1;
    }
}

/* dispatch_callback_list_1: calls every callback in the first list. */
void dispatch_callback_list_1(void) {
    int i;
    for (i = 0; i < draw_callback_count; i++) {
        draw_callback_funcs[i](draw_callback_args[i]);
    }
}

/* Calls every callback in the second draw-callback list. */
void dispatch_callback_list_2(void) {
    int i;
    for (i = 0; i < draw_callback3_count; i++) {
        draw_callback3_funcs[i](draw_callback3_args[i]);
    }
}

/* Calls every callback in the third draw-callback list. */
void dispatch_callback_list_3(void) {
    int i;
    for (i = 0; i < draw_callback4_count; i++) {
        draw_callback4_funcs[i](draw_callback4_args[i]);
    }
}

/* Appends fn(arg) to the fourth draw-callback list. */
void enqueue_callback_list_4(DrawCallback fn, void *arg) {
    int count = draw_callback2_count;
    if (count < 0x40) {
        draw_callback2_funcs[count] = fn;
        draw_callback2_args[count] = arg;
        draw_callback2_count = count + 1;
    }
}

/* Calls every callback in the fourth draw-callback list. */
void dispatch_callback_list_4(void) {
    int i;
    for (i = 0; i < draw_callback2_count; i++) {
        draw_callback2_funcs[i](draw_callback2_args[i]);
    }
}

typedef float FVec4[4] __attribute__((aligned(16)));
typedef struct {
    FVec4 v;
} FRow;
typedef struct {
    FRow pos;
    FRow dir;
} LightRec;
extern FRow effect_quad_corners[4];
extern LightRec effect_quad_entries[];
extern int GetEffectTex(int);
/* GetEffectTex as it really returns: the 64-bit TEX0 (for the callers that keep it whole). */
extern long GetEffectTex64(int) __asm__("func_001F4868");
extern void fast_vec_normalize(void *dst, void *src, float len);
extern void draw_world_quad_packet(void *, int, int);

/* Builds a 4-row matrix per light in D_0018E440 (count D_0015F574):
   each row starts as the light's position and is pushed along the
   corner D_0018CBA0[k], projected off the normalised light direction,
   scaled by the position's w; func_001F7EF8 draws it. The packet header,
   colours and UVs are filled on the stack but never sent. Both copies
   are qcopy (retail's lq/sq stay inside the loop), and the inner loop
   needs its own counter, not the one the setup loop used. */
void draw_light_quads(void) {
    FRow m[4];
    int colors[4];
    float uv[4][2];
    unsigned long pkt[4];
    FRow dir;
    int i;
    int j;

    pkt[1] = GetEffectTex(0);
    pkt[2] = 0xFF9000000260;
    pkt[0] = 5;
    pkt[3] = 0x8000000044;
    for (j = 0; j < 4; j++) {
        uv[j][0] = effect_quad_corners[j].v[2];
        uv[j][1] = effect_quad_corners[j].v[3];
        colors[j] = 0x40808080;
    }
    for (i = 0; i < effect_quad_count; i++) {
        float s;
        int k;

        qcopy(&dir, &effect_quad_entries[i].dir);
        fast_vec_normalize(&dir, &dir, 1.0f);
        s = effect_quad_entries[i].pos.v[3];
        for (k = 0; k < 4; k++) {
            float cx = effect_quad_corners[k].v[0];
            float vx = dir.v[0];
            float cy = effect_quad_corners[k].v[1];
            float d = cx * vx + cy * dir.v[1];

            qcopy(&m[k], &effect_quad_entries[i].pos);
            m[k].v[0] += (cx - vx * d) * s;
            m[k].v[1] += (cy - dir.v[1] * d) * s;
            m[k].v[2] -= dir.v[2] * d * s;
        }
        draw_world_quad_packet(m, 0, 0);
    }
}

extern int sce_gs_sync_v_i(int) __asm__("func_00122598");
extern void VU1_swapChain(void);
extern void VU1_sendChain(void);
extern void PutDrawBufferLarge(void);
extern void PutDrawBufferSmall(void);
extern int *g_render_packet_cursor MACRO_ADDR;
extern int game_frame_counter MACRO_ADDR;
extern char second_clear_packet[];

/* FadeToBlack(frames): `frames` VU1 chains, each drawing a
   full-screen quad (D_0013CED0's GS packet) with its alpha ramped down
   from 0x80 by (n << 7) / (n + 1) of the count still to go. It takes no
   colour: retail writes $a1 before it reads it. Each chain is bracketed by func_00234AC8/func_00122598 and a
   D_0015F538 bump. func_00122598 (sceGsSyncV) returns int: declared that
   way, the bump's temporary moves to $v1 as in retail. D_0015F538 is
   MACRO_ADDR so each bump reloads it in one register. */
void FadeToBlack(int frames) {
    int n;
    int q;

    VU1_syncChain(1);
    sce_gs_sync_v_i(0);
    game_frame_counter = game_frame_counter + 1;
    VU1_initChain();
    n = frames - 1;

    if (n >= 0) {
        do {
            PutDrawBufferLarge();
            append_gif_transfer_packet();
            emit_rgba_draw_packet(0, 0, 0, 0x80);
            PutDrawBufferSmall();

            q = (n << 7) / (n + 1);
            n--;

            VU1_addGSregister(1, (long)(0x80 - q) << 24);

            g_render_packet_cursor[0] = 0x30000014;
            g_render_packet_cursor[1] = (int)second_clear_packet;
            g_render_packet_cursor[2] = 0;
            g_render_packet_cursor[3] = 0x50000014;
            g_render_packet_cursor += 4;

            VU1_syncChain(1);
            sce_gs_sync_v_i(0);
            game_frame_counter = game_frame_counter + 1;
            VU1_sendChain();
            VU1_swapChain();
        } while (n >= 0);
    }

    VU1_syncChain(1);
    sce_gs_sync_v_i(0);
    game_frame_counter = game_frame_counter + 1;
    VU1_initChain();
    PutDrawBufferLarge();
    append_gif_transfer_packet();
}

typedef struct {
    short start;   /* 0x0 */
    short end;     /* 0x2 */
    short text[6]; /* 0x4: string offsets, one per language */
} Subtitle;
typedef struct {
    char pad00[0x34];
    int time;      /* 0x34 */
    char pad38[0x14];
    char *subs;    /* 0x4C */
} SubState;
extern SubState render_sequence_s __asm__("D_0018CC20");
extern int g_game_language MACRO_ADDR;
extern int g_screen_extent[];
extern void FontSetWindow(void *arg0, int a1, int a2, int a3, int a4, int a5,
                          int a6, int a7, int a8);
extern void font_print_window_regular_l(void *, long, char *, int) __asm__("func_001F7560");
extern void DrawUIFrame(int, int, int, int, int);

/* Draws the subtitle showing at the current time (D_0018CC20+0x34): the
   list at +0x4C holds 16-byte entries (start, end, and one string offset
   into the list per language; a negative start ends it). The language
   D_0015EE88 picks the string (2..5 map to 1..4, anything else to 0). The
   text is measured in a FontSetWindow buffer (func_001F7648/7560), its
   box is kept 0x14 above the bottom of the screen (D_0013E600[1]), the
   frame is drawn (func_001F62C8) and the text printed with the measure
   flag (4) cleared. The list is tested and then read again for the
   loop, which gives retail's copy of it; the clamp test is written
   bottom-first, which gives retail's registers. */
void draw_subtitles(void) {
    Subtitle *p;
    int lang;
    int idx;

    if (render_sequence_s.subs == 0) {
        return;
    }
    lang = g_game_language;
    idx = (lang >= 2 && lang <= 5) ? lang - 1 : 0;
    for (p = (Subtitle *)render_sequence_s.subs; p->start >= 0; p++) {
        if (render_sequence_s.time < p->start) {
            continue;
        }
        if (p->end < render_sequence_s.time) {
            continue;
        }
        {
            short win[16];
            short w, h;
            int hw, hh;

            FontSetWindow(win, 0xC8, 0x208, 0x28, 0x1D8, 0x100, g_screen_extent[1] - 0x38, 0x12, 7);
            font_print_window_regular_l(win, 0x80B0B0B0, render_sequence_s.subs + p->text[idx], -1);
            win[5] = g_screen_extent[1] - 0x3C;
            w = win[6];
            h = win[7];
            hh = (h >> 1) + 5;
            hw = (w >> 1) + 10;
            if (win[5] + hh > g_screen_extent[1] - 0x14) {
                win[5] = g_screen_extent[1] - 0x14 - hh;
            }
            DrawUIFrame(win[5] - hh, win[5] + hh, 0x100 - hw, 0x100 + hw, 0x60);
            win[9] &= ~4;
            font_print_window_regular_l(win, 0x80B0B0B0, render_sequence_s.subs + p->text[idx], -1);
            return;
        }
    }
}

extern char rect_overlay_gif_regs_a[];
extern char rect_overlay_gif_regs_b[];

/* Letterbox bars: while D_0015F544 is set the bar height D_0015F548
   grows to 24, otherwise it shrinks to 0. While it is non-zero, append
   a GIF packet (the D_00160920/D_00160930 register descriptors, PRIM
   0x104) drawing two full-width strips, the height in 16ths reaching in
   from the top and bottom of the D_0013E600 viewport, the same packet
   steps as func_001F5650. */
void draw_letterbox_bars(void) {
    int h;

    if (letterbox_enabled != 0) {
        if (letterbox_bar_height < 24) {
            letterbox_bar_height++;
        }
    } else {
        if (letterbox_bar_height == 0) {
            return;
        }
        letterbox_bar_height--;
    }
    h = letterbox_bar_height;
    if (h == 0) {
        return;
    }
    h <<= 4;
    g_render_packet_cursor[0] = 0x10000007;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000007;
    {
        int *base = g_render_packet_cursor;
        g_render_packet_cursor = base + 4;
        qcopy(g_render_packet_cursor, rect_overlay_gif_regs_a);
        *(short *)(base + 4) = 0x8001;
    }
    {
        int *base = g_render_packet_cursor;
        long *p;
        g_render_packet_cursor = base + 4;
        p = (long *)g_render_packet_cursor;
        p[0] = 0x104;
        p[1] = 0x80000000;
    }
    {
        int *base = g_render_packet_cursor;
        g_render_packet_cursor = base + 4;
        qcopy(g_render_packet_cursor, rect_overlay_gif_regs_b);
        *(short *)(base + 4) = 0x8008;
    }
    {
        int *base = g_render_packet_cursor;
        long *p;
        g_render_packet_cursor = base + 4;
        p = (long *)g_render_packet_cursor;
        p[0] = g_screen_extent[4] | ((long)g_screen_extent[5] << 16) | ((long)0xFFFFF3 << 32);
        p[1] = g_screen_extent[4] | ((long)(g_screen_extent[5] + h) << 16) | ((long)0xFFFFF3 << 32);
        p[2] = g_screen_extent[6] | ((long)g_screen_extent[5] << 16) | ((long)0xFFFFF3 << 32);
        p[3] = g_screen_extent[6] | ((long)(g_screen_extent[5] + h) << 16) | ((long)0xFFFFF3 << 32);
        p[4] = g_screen_extent[6] | ((long)g_screen_extent[7] << 16) | ((long)0xFFFFF3 << 32);
        p[5] = g_screen_extent[6] | ((long)(g_screen_extent[7] - h) << 16) | ((long)0xFFFFF3 << 32);
        p[6] = g_screen_extent[4] | ((long)g_screen_extent[7] << 16) | ((long)0xFFFFF3 << 32);
        p[7] = g_screen_extent[4] | ((long)(g_screen_extent[7] - h) << 16) | ((long)0xFFFFF3 << 32);
    }
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x40);
}

typedef struct {
    char pad0[4];
    int color;
    long enable;
    int step0;
    int color0;
    long enable0;
    int step1;
    int color1;
    long enable1;
} Stripes;
extern short screen_color_effect_now;             /* SDA, gp -0x78B0: Stripes * */
extern void DrawRectOverlay_FiiiiUl(int, int, int, int, unsigned long);

#define STRIPES (*(Stripes **)&D_0015F450)

/* Fills the screen in vertical stripes: an optional full-screen colour
   first, then alternating stripes of widths step0/step1 in colours
   color0/color1, each with its own GS register 0x42 blend word when set.
   Adapted from Lombyte's project (GPL-3.0) for PAL. */
void draw_screen_effect(void) {
    int i;
    short w;
    long mask;
    int c;
    long e;
    int v14;
    long e18;
    int v24;
    long e28;

    i = 0;
    w = fs_aa_buffer[0xA9];
    e = STRIPES->enable;
    if (e != 0) {
        VU1_addGSregister(0x42, e & 0xFF000000FFL);
    }
    c = STRIPES->color;
    if (c & 0xFF000000) {
        DrawRectOverlay_FiiiiUl(0, w, 0, fs_aa_buffer[0xA8], (unsigned long)((long)c << 0x20) >> 0x20);
    }
    if (w > 0) {
        mask = 0xFF000000FFL;
        do {
            e18 = STRIPES->enable0;
            if (e18 != 0) {
                VU1_addGSregister(0x42, e18 & mask);
            }
            v14 = STRIPES->color0;
            if (v14 & 0xFF000000) {
                DrawRectOverlay_FiiiiUl(i, (i + STRIPES->step0 < w - 1) ? i + STRIPES->step0 : w - 1, 0,
                              fs_aa_buffer[0xA8], (unsigned long)((long)v14 << 0x20) >> 0x20);
            }
            i = i + STRIPES->step0;
            e28 = STRIPES->enable1;
            if (e28 != 0) {
                VU1_addGSregister(0x42, e28 & mask);
            }
            v24 = STRIPES->color1;
            if (v24 & 0xFF000000) {
                DrawRectOverlay_FiiiiUl(i, (i + STRIPES->step1 < w - 1) ? i + STRIPES->step1 : w - 1, 0,
                              fs_aa_buffer[0xA8], (unsigned long)((long)v24 << 0x20) >> 0x20);
            }
            i = i + STRIPES->step1;
        } while (i < w);
    }
}

#undef STRIPES

extern int depth_buffer_address MACRO_ADDR;

/* GS register writes around an overlay: blend register 0x42 from the
   64-bit word at +8 while it is set, and when the colour at +4 has an
   alpha byte, register 0x4E switched around a full-screen
   func_001F5650 draw. The 64-bit constants are ps2eeas's dli
   sequences (tools/ps2eeas_dli.py). */
void draw_fogged_fullscreen_sprite(char *arg0) {
    long v = *(long *)(arg0 + 8);

    if (v != 0) {
        VU1_addGSregister(0x42, v & 0xFF000000FFL);
    }
    if ((*(int *)(arg0 + 4) & 0xFF000000) != 0) {
        VU1_addGSregister(0x4E, (depth_buffer_address >> 13) | 0x1000000 | 0x100000000L);
        DrawRectOverlay_FiiiiUl(0, fs_aa_buffer[0xA9], 0, fs_aa_buffer[0xA8],
                      *(unsigned int *)(arg0 + 4));
        VU1_addGSregister(0x4E, 0x1000000 | (depth_buffer_address >> 13));
    }
    if (*(long *)(arg0 + 8) != 0) {
        VU1_addGSregister(0x42, 0x8000000044L);
    }
}

/* Sets GS register 1 from four bytes packed into one 64-bit value, then
   restores the default register set. The parameters are int, widened in
   the expression: with long parameters the scheduler hoists the last
   dsll one slot early (it was a 6/144 near-miss that way). */
extern int *g_render_packet_cursor MACRO_ADDR;
extern char first_clear_packet[];

/* Appends a GIF packet that sets GS register 1 from the four bytes a, b, c, d. */
void emit_rgba_draw_packet(int a, int b, int c, int d) {
    VU1_addGSregister(1, (long)a | ((long)b << 8) | ((long)c << 16) | ((long)d << 24));
    g_render_packet_cursor[0] = 0x30000014;
    g_render_packet_cursor[1] = (int)first_clear_packet;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000014;
    g_render_packet_cursor += 4;
}


/* DrawRectOverlay: append a GIF packet drawing the rectangle x0..x1,
   y0..y1 (in 16ths, offset by the viewport origin D_0013E600[4]/[5]
   - 8) as a PRIM 0x144 sprite pair in colour rgba: the tag, the
   D_00160920 and D_00160930 register descriptors (ids 0x8001/0x8004),
   then four XYZ values at Z 0xFFFFF0. Each packet step has its own
   block-scoped base pointer, which gives retail's registers. */
void DrawRectOverlay_FiiiiUl(int y0, int y1, int x0, int x1, unsigned long rgba) {
    g_render_packet_cursor[0] = 0x10000005;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000005;
    {
        int *base = g_render_packet_cursor;
        g_render_packet_cursor = base + 4;
        qcopy(g_render_packet_cursor, rect_overlay_gif_regs_a);
        *(short *)(base + 4) = 0x8001;
    }
    {
        int *base = g_render_packet_cursor;
        long *p;
        g_render_packet_cursor = base + 4;
        p = (long *)g_render_packet_cursor;
        p[0] = 0x144;
        p[1] = rgba;
    }
    {
        int *base = g_render_packet_cursor;
        g_render_packet_cursor = base + 4;
        qcopy(g_render_packet_cursor, rect_overlay_gif_regs_b);
        *(short *)(base + 4) = 0x8004;
    }
    {
        int *base = g_render_packet_cursor;
        long *p;
        g_render_packet_cursor = base + 4;
        p = (long *)g_render_packet_cursor;
        p[0] = (x0 * 16 + g_screen_extent[4] - 8)
             | ((long)(y0 * 16 + g_screen_extent[5] - 8) << 16)
             | ((long)0xFFFFF0 << 32);
        p[1] = (x1 * 16 + g_screen_extent[4] - 8)
             | ((long)(y0 * 16 + g_screen_extent[5] - 8) << 16)
             | ((long)0xFFFFF0 << 32);
        p[2] = (x0 * 16 + g_screen_extent[4] - 8)
             | ((long)(y1 * 16 + g_screen_extent[5] - 8) << 16)
             | ((long)0xFFFFF0 << 32);
        p[3] = (x1 * 16 + g_screen_extent[4] - 8)
             | ((long)(y1 * 16 + g_screen_extent[5] - 8) << 16)
             | ((long)0xFFFFF0 << 32);
    }
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x20);
}

LINKER_REMNANT("asm/remnants/text", func_001F57F8);

extern char textured_quad_header[];

/* Append a textured sprite as a four-vertex strip (PRIM 0x154): screen
   corners in 12.4 fixed point relative to the viewport origin, UVs from
   (u, v) to (u + uw, v + vh) in 16ths. */
void DrawTexturedQuad(int x, int y, int w, int h, int u, int v, int uw, int vh,
                   unsigned long rgba, unsigned long tex) {
    int x0 = x * 16 + g_screen_extent[4] - 8;
    int x1 = (x + w) * 16 + g_screen_extent[4] - 8;
    int y0 = y * 16 + g_screen_extent[5] - 8;
    int y1 = (y + h) * 16 + g_screen_extent[5] - 8;
    int s1 = (u + uw) * 16;
    int s0 = u * 16;
    int vb = v + vh;
    long *p;
    int *base;

    g_render_packet_cursor[0] = 0x10000007;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000007;
    base = g_render_packet_cursor;
    g_render_packet_cursor = base + 4;
    qcopy(base + 4, textured_quad_header);
    p = (long *)(base + 8);
    g_render_packet_cursor = base + 8;
    p[0] = tex;
    p[1] = 0x154;
    p[2] = rgba;
    p[3] = (v << 20) + s0;
    p[4] = x0 | ((long)y0 << 16) | ((long)0xFFFFF0 << 32);
    p[5] = (v << 20) + s1;
    p[6] = x1 | ((long)y0 << 16) | ((long)0xFFFFF0 << 32);
    p[7] = (vb << 20) + s0;
    p[8] = x0 | ((long)y1 << 16) | ((long)0xFFFFF0 << 32);
    p[9] = (vb << 20) + s1;
    p[10] = x1 | ((long)y1 << 16) | ((long)0xFFFFF0 << 32);
    p[11] = 0;
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x60);
}

extern int truncate_float_to_s32_r(float) __asm__("func_001FA898");

/* func_001F5800 with a float screen rectangle: each corner is rounded
   (func_001FA898) from 16ths before the viewport offset. */
void append_subpixel_textured_screen_quad(float x, float y, float w, float h, int u, int v, int uw, int vh,
                   unsigned long rgba, unsigned long tex) {
    int x0 = truncate_float_to_s32_r(x * 16.0f) + g_screen_extent[4] - 8;
    int x1 = truncate_float_to_s32_r((x + w) * 16.0f) + g_screen_extent[4] - 8;
    int y0 = truncate_float_to_s32_r(y * 16.0f) + g_screen_extent[5] - 8;
    int y1 = truncate_float_to_s32_r((y + h) * 16.0f) + g_screen_extent[5] - 8;
    int s1 = (u + uw) * 16;
    int s0 = u * 16;
    int vb = v + vh;
    long *p;
    int *base;

    g_render_packet_cursor[0] = 0x10000007;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000007;
    base = g_render_packet_cursor;
    g_render_packet_cursor = base + 4;
    qcopy(base + 4, textured_quad_header);
    p = (long *)(base + 8);
    g_render_packet_cursor = base + 8;
    p[0] = tex;
    p[1] = 0x154;
    p[2] = rgba;
    p[3] = (v << 20) + s0;
    p[4] = x0 | ((long)y0 << 16) | ((long)0xFFFFF0 << 32);
    p[5] = (v << 20) + s1;
    p[6] = x1 | ((long)y0 << 16) | ((long)0xFFFFF0 << 32);
    p[7] = (vb << 20) + s0;
    p[8] = x0 | ((long)y1 << 16) | ((long)0xFFFFF0 << 32);
    p[9] = (vb << 20) + s1;
    p[10] = x1 | ((long)y1 << 16) | ((long)0xFFFFF0 << 32);
    p[11] = 0;
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x60);
}

extern char textured_rect_gif_regs[];

/* func_001F5988, skipped when the rectangle lies outside 0x7000..0x9000
   (12.4), with a CLAMP register (REGION_CLAMP to the texel rectangle)
   after the PRIM and a closing (5, 0) pair. */
void emit_textured_rectangle(float x, float y, float w, float h, int u, int v, int uw, int vh,
                   unsigned long rgba, unsigned long tex) {
    int x0 = truncate_float_to_s32_r(x * 16.0f) + g_screen_extent[4] - 8;
    int x1 = truncate_float_to_s32_r((x + w) * 16.0f) + g_screen_extent[4] - 8;
    int y0 = truncate_float_to_s32_r(y * 16.0f) + g_screen_extent[5] - 8;
    int y1 = truncate_float_to_s32_r((y + h) * 16.0f) + g_screen_extent[5] - 8;
    int s1;
    int s0;
    int vb;
    int ub;
    long *p;
    int *base;

    if (x0 > 0x9000 || x1 < 0x7000 || y0 > 0x9000 || y1 < 0x7000) {
        return;
    }
    g_render_packet_cursor[0] = 0x10000008;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0;
    g_render_packet_cursor[3] = 0x50000008;
    ub = u + uw;
    vb = v + vh;
    s0 = u * 16;
    s1 = ub * 16;
    base = g_render_packet_cursor;
    g_render_packet_cursor = base + 4;
    qcopy(base + 4, textured_rect_gif_regs);
    p = (long *)(base + 8);
    g_render_packet_cursor = base + 8;
    p[0] = tex;
    p[1] = 0x154;
    p[2] = 0xA | ((long)u << 4) | ((long)ub << 14) | ((long)v << 24) | ((long)vb << 34);
    p[3] = rgba;
    p[4] = (v << 20) + s0;
    p[5] = x0 | ((long)y0 << 16) | ((long)0xFFFFF0 << 32);
    p[6] = (v << 20) + s1;
    p[7] = x1 | ((long)y0 << 16) | ((long)0xFFFFF0 << 32);
    p[8] = (vb << 20) + s0;
    p[9] = x0 | ((long)y1 << 16) | ((long)0xFFFFF0 << 32);
    p[10] = (vb << 20) + s1;
    p[11] = x1 | ((long)y1 << 16) | ((long)0xFFFFF0 << 32);
    p[12] = 5;
    p[13] = 0;
    g_render_packet_cursor = (int *)((char *)g_render_packet_cursor + 0x70);
}

struct Screen {
    s32 width; /* 0x0: display width (D_00151780 +0x150) */
    s32 height; /* 0x4: display height (D_00151780 +0x152) */
    s32 half_width; /* 0x8: width >> 1 */
    s32 half_height; /* 0xC: height >> 1 */
    s32 left; /* 0x10: (0x800 - half_width) << 4 */
    s32 top; /* 0x14: (0x800 - half_height) << 4 */
    s32 right; /* 0x18: (0x800 + half_width) << 4 */
    s32 bottom; /* 0x1C: (0x800 + half_height) << 4 */
};
extern struct Screen g_screen_extent_r __asm__("D_0013E600");
struct DmaTag {
    u32 tag;
    u32 addr;
    u32 vif0;
    u32 vif1;
};
struct GifTag;
union PacketCursor {
    struct DmaTag *tag;
    struct GifTag *gif;
    s32 *words;
    u8 *bytes;
    s32 addr;
};
extern union PacketCursor g_render_packet_cursor_r __asm__("D_00161000") MACRO_ADDR;
extern void FastVecAdd(void *, void *, void *);
extern void FastVecSub(void *, void *, void *);
extern void FastVecScale(void *, void *, f32);
extern f32 FastCos(f32);
extern f32 FastSin(f32);
extern s32 truncate_float_to_s32(f32);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/rendering/append_rotated_sprite_quad.c, append_rotated_sprite_quad. */
void append_rotated_sprite_quad(f32 center_x, f32 center_y, f32 quad_width, f32 quad_height,
                                f32 angle, s32 texture_width, s32 texture_height,
                                s64 texture_tex0, s64 z_and_fog, s32 color, u8 flip_u, u8 flip_v,
                                f32 pivot_u, f32 pivot_v) {
    Vec4f vertical_edge;
    Vec4f horizontal_edge;
    Vec4f center;
    Vec4f temporary;
    Vec4f top_left;
    Vec4f top_right;
    Vec4f bottom_left;
    Vec4f bottom_right;
    struct DmaTag *tag;
    u64 *packet_words;
    s32 texture_left;
    s32 texture_right;
    s32 texture_top;
    s32 texture_bottom;
    f32 inverse_pivot_u;
    f32 inverse_pivot_v;

    if (flip_u != 0) {
        texture_left = ((u32)texture_width << 4);
        texture_right = 0x10;
    } else {
        texture_right = ((u32)texture_width << 4);
        texture_left = 0x10;
    }
    if (flip_v != 0) {
        texture_top = ((u32)texture_height << 20);
        texture_bottom = 0x100000;
    } else {
        texture_bottom = ((u32)texture_height << 20);
        texture_top = 0x100000;
    }
    center.x = center_x;
    center.y = center_y;
    inverse_pivot_v = 1.0f - pivot_v;
    vertical_edge.x = quad_height * FastSin(angle);
    vertical_edge.y = quad_height * FastCos(angle);
    horizontal_edge.x = quad_width * FastCos(angle);
    horizontal_edge.y = -quad_width * FastSin(angle);
    inverse_pivot_u = 1.0f - pivot_u;
    FastVecScale(&temporary, &vertical_edge, inverse_pivot_v);
    FastVecAdd(&top_left, &center, &temporary);
    FastVecScale(&temporary, &horizontal_edge, inverse_pivot_u);
    FastVecSub(&top_left, &top_left, &temporary);
    FastVecScale(&temporary, &vertical_edge, inverse_pivot_v);
    FastVecAdd(&top_right, &center, &temporary);
    FastVecScale(&temporary, &horizontal_edge, pivot_u);
    FastVecAdd(&top_right, &top_right, &temporary);
    FastVecScale(&temporary, &vertical_edge, pivot_v);
    FastVecSub(&bottom_left, &center, &temporary);
    FastVecScale(&temporary, &horizontal_edge, inverse_pivot_u);
    FastVecSub(&bottom_left, &bottom_left, &temporary);
    FastVecScale(&temporary, &vertical_edge, pivot_v);
    FastVecSub(&bottom_right, &center, &temporary);
    FastVecScale(&temporary, &horizontal_edge, pivot_u);
    FastVecAdd(&bottom_right, &bottom_right, &temporary);
    g_render_packet_cursor_r.tag->tag = 0x10000007;
    g_render_packet_cursor_r.tag->addr = 0;
    g_render_packet_cursor_r.tag->vif0 = 0;
    g_render_packet_cursor_r.tag->vif1 = 0x50000007;
    tag = g_render_packet_cursor_r.tag;
    packet_words = (u64 *)(tag + 1);
    g_render_packet_cursor_r.tag = tag + 1;
    packet_words[0] = 0xB400000000008001;
    packet_words[1] = 0x53535353106;
    packet_words[2] = texture_tex0;
    packet_words[3] = 0x154;
    packet_words[4] = color;
    packet_words[5] = texture_left | texture_top;
    packet_words[6] =
        (truncate_float_to_s32(top_left.x * 16.0f) + g_screen_extent_r.left - 8) |
        ((u64)(truncate_float_to_s32(top_left.y * 16.0f) + g_screen_extent_r.top - 8) << 16) |
        ((u64)z_and_fog << 32);
    packet_words[7] = texture_right | texture_top;
    packet_words[8] =
        (truncate_float_to_s32(top_right.x * 16.0f) + g_screen_extent_r.left - 8) |
        ((u64)(truncate_float_to_s32(top_right.y * 16.0f) + g_screen_extent_r.top - 8) << 16) |
        ((u64)z_and_fog << 32);
    packet_words[9] = texture_left | texture_bottom;
    packet_words[10] =
        (truncate_float_to_s32(bottom_left.x * 16.0f) + g_screen_extent_r.left - 8) |
        ((u64)(truncate_float_to_s32(bottom_left.y * 16.0f) + g_screen_extent_r.top - 8) << 16) |
        ((u64)z_and_fog << 32);
    packet_words[11] = texture_right | texture_bottom;
    packet_words[12] =
        (truncate_float_to_s32(bottom_right.x * 16.0f) + g_screen_extent_r.left - 8) |
        ((u64)(truncate_float_to_s32(bottom_right.y * 16.0f) + g_screen_extent_r.top - 8) << 16) |
        ((u64)z_and_fog << 32);
    packet_words[13] = 0;
    g_render_packet_cursor_r.tag = (struct DmaTag *)((u8 *)g_render_packet_cursor_r.tag + 0x70);
}

LINKER_REMNANT("asm/remnants/text", func_001F62C0);

/* Draws a bevelled frame: the box itself, then three shrinking bars
   above and three below it, all in grey 0x040404 with alpha a. */
void DrawUIFrame(int x0, int x1, int y0, int y1, int a) {
    int col = (a << 24) | 0x40404;

    DrawRectOverlay_FiiiiUl(x0, x1, y0, y1, col);
    DrawRectOverlay_FiiiiUl(x0 + 1, x1 - 1, y0 - 2, y0, col);
    DrawRectOverlay_FiiiiUl(x0 + 2, x1 - 2, y0 - 3, y0 - 2, col);
    DrawRectOverlay_FiiiiUl(x0 + 4, x1 - 4, y0 - 4, y0 - 3, col);
    DrawRectOverlay_FiiiiUl(x0 + 1, x1 - 1, y1, y1 + 2, col);
    DrawRectOverlay_FiiiiUl(x0 + 2, x1 - 2, y1 + 2, y1 + 3, col);
    DrawRectOverlay_FiiiiUl(x0 + 4, x1 - 4, y1 + 3, y1 + 4, col);
}

/* Emits a 9-point cross/star pattern of func_001F5650 draws around
   (a0, a1, a2, a3), offset by +-1/3/5 along each axis, all sharing the
   colour/flags word a4. The first call passes a4 with only its top byte
   kept and the low nibble forced to 4; the rest pass a4 unchanged. */
void draw_outlined_rect(int a0, int a1, int a2, int a3, int a4) {
    int color;
    int t0, t1, t2;
    int t_a0m5, t4, t5;
    int t_a0p3, t6, t_a2m3;
    int t_a1p3, t3, t_a3p1;

    color = (a4 & (int)0xFF000000) | 4;
    DrawRectOverlay_FiiiiUl(a0, a1, a2, a3, color);

    t0 = a0 + 1;
    t1 = a2 + 3;
    t2 = a3 + 5;
    DrawRectOverlay_FiiiiUl(a0 - 1, t0, t1, t2, a4);

    t_a0m5 = a0 - 5;
    t4 = a2 - 1;
    t5 = a3 - 3;
    DrawRectOverlay_FiiiiUl(a0 - 3, t_a0m5, t4, t5, a4);

    t3 = a1 - 3;
    DrawRectOverlay_FiiiiUl(t_a0m5, t3, t4, a2 + 1, a4);

    t_a0p3 = a0 + 3;
    t6 = a1 + 1;
    t_a2m3 = a2 - 3;
    DrawRectOverlay_FiiiiUl(t_a0p3, t6, t_a2m3, a2 - 5, a4);

    DrawRectOverlay_FiiiiUl(a1 - 1, t6, t_a2m3, t5, a4);

    t_a1p3 = a1 + 3;
    t_a3p1 = a3 + 1;
    DrawRectOverlay_FiiiiUl(t_a1p3, a1 + 5, t1, t_a3p1, a4);

    DrawRectOverlay_FiiiiUl(t_a0p3, t_a1p3, a3 - 1, t_a3p1, a4);

    DrawRectOverlay_FiiiiUl(t0, t3, a3 + 3, t2, a4);
}

/* gp-relative: declared as a 2-byte type purely so -G2 places it in the
   small-data area (placement is decided by DECLARED size), then accessed
   as the 4-byte word it really is. gp base 0x166D00 - 0x7764 = 0x15F59C. */
extern int GlobalStateFlag MACRO_ADDR;

/* Sets the word flag at D_0015F59C to 1. */
void Font_EnableColorCodes(void) {
    *(int *)&GlobalStateFlag = 1;
}

/* Clears the word flag at D_0015F59C. */
void Font_DisableColorCodes(void) {
    *(int *)&GlobalStateFlag = 0;
}

/* String width: sums the signed width byte (+3 of each 4-byte entry of
   `table`, indexed by character) over at most `count` characters of `str`,
   stopping at the NUL. Called by func_001F6600/20/40 with the three font
   tables. A do-while behind an entry test, with `p = str` set inside the
   if: the body's `*p` then sits after the loop label, so CSE keeps it
   apart from the entry test's `*str`, and `i = 0` is not folded into the
   first `i++`, both as in retail. */
int measure_text_width(unsigned char *str, int count, void *table) {
    int width = 0;
    int i = 0;
    unsigned char *p;

    if (count != 0 && *str != 0) {
        p = str;
        do {
            int c = *p;
            int w;

            i++;
            p++;
            w = ((signed char *)table)[c * 4 + 3];
            if (w != 0) {
                width += w;
            }
        } while (i != count && *p != 0);
    }
    return width;
}

extern int measure_text_width(unsigned char *arg0, int arg1, void *arg2);
extern unsigned char normal_font_metrics[];
extern unsigned char small_font_metrics[];
extern unsigned char large_font_metrics[];

/* measure_text_width_regular: width of the first arg1 characters of arg0 in the regular font table. */
int measure_text_width_regular(unsigned char *arg0, int arg1) {
    return measure_text_width(arg0, arg1, normal_font_metrics);
}

/* measure_text_width_small: width of the first arg1 characters of arg0 in the small font table. */
int measure_text_width_small(unsigned char *arg0, int arg1) {
    return measure_text_width(arg0, arg1, small_font_metrics);
}

/* measure_text_width_large: width of the first arg1 characters of arg0 in the large font table. */
int measure_text_width_large(unsigned char *arg0, int arg1) {
    return measure_text_width(arg0, arg1, large_font_metrics);
}

/*
 * Retail has 8 bytes of nop padding between func_001F6640 and
 * func_001F6668, and it lives *after* `endlabel` in
 * asm/nonmatchings/text/func_001F6640.s -- so the INCLUDE_ASM stub was
 * supplying it, and replacing that stub with C silently dropped it,
 * shifting every later function in the segment by -8 and corrupting
 * their `jal` targets (func_001F7B40 read 1/44 while being
 * instruction-for-instruction identical to retail). Emitted explicitly
 * to preserve the layout.
 *
 * Check for this whenever converting a stub: content after a .s file's
 * `endlabel` is inter-function padding the stub was carrying, and it has
 * to be reproduced or everything downstream drifts. Alignment directives
 * do not cover it -- both boundaries here are already 8-byte aligned.
 */
__asm__(".section .text\n\tnop\n\tnop\n");

struct Glyph_F6668 {
    u8 u;
    u8 v;
    s8 top;
    s8 adv;
};
extern s32 font_window_active_r __asm__("D_0015F5A0") MACRO_ADDR;
extern s32 D_0015F59C_F6668 SDATA(D_0015F59C);
extern s32 font_palette_colors_r[] __asm__("D_0018CBF8");
extern void DrawTexturedQuad_r(s32, s32, s32, s32, s32, s32, s32, s32, s64, s64) __asm__("func_001F5800");

/* FontPrint: draws a string glyph by glyph; bytes 8..15 switch the colour, 0x80..0xA7 add an accent glyph,
   bytes below 0x20 are 24-pixel grey button glyphs.
   Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/ui/text/font_print.c, font_print. */
void func_001F6668_r(s32 x, s32 y, u64 color, u8 *text, s32 character_limit, s64 texture,
                      struct Glyph_F6668 *glyphs) __asm__("func_001F6668");
void func_001F6668_r(s32 x, s32 y, u64 color, u8 *text, s32 character_limit, s64 texture,
                      struct Glyph_F6668 *glyphs) {
    s32 character_count;
    u8 *cursor;
    u8 character;
    struct Glyph_F6668 *overlay_glyph;
    s32 gray_value;
    s32 gray_color;

    if (font_window_active_r == 0) {
        font_palette_colors_r[0] = color;
    }
    character_count = 0;
    if (character_limit == 0 || *text == 0) {
        return;
    }
    cursor = text;
    do {
        /* Bytes 8..15 select a palette color, preserving the current alpha. */
        if ((u8)(*cursor - 8) < 8) {
            if (D_0015F59C_F6668 != 0) {
                color &= 0xFF000000;
                color |= font_palette_colors_r[*cursor - 8] & 0xFFFFFF;
            }
        } else {
            character = *cursor;
            if (glyphs[character].adv != 0) {
                /* Characters 0x80..0xa7 also draw the glyph 0x40 entries later. */
                if ((u8)(character + 0x80) < 0x28) {
                    overlay_glyph = (struct Glyph_F6668 *)(((character + 0x40) << 2) + (s32)glyphs);
                    DrawTexturedQuad_r(x + overlay_glyph->adv, y + overlay_glyph->top, 16, 16,
                                       overlay_glyph->u, overlay_glyph->v, 16, 16, color, texture);
                }
                /* Control glyphs use 24-pixel grayscale sprites. */
                if (*cursor < 0x20) {
                    gray_value =
                        (s32)((color & 0xFF) + ((color >> 8) & 0xFF) + ((color >> 16) & 0xFF)) / 3;
                    gray_color = (s32)(color & 0xFF000000);
                    gray_color += gray_value << 16;
                    gray_color += gray_value << 8;
                    gray_value += gray_color;
                    DrawTexturedQuad_r(x, y + glyphs[*cursor].top, 24, 16, glyphs[*cursor].u,
                                       glyphs[*cursor].v, 24, 16, gray_value, texture);
                } else if (*cursor > 0x20) {
                    DrawTexturedQuad_r(x, y + glyphs[*cursor].top, 16, 16, glyphs[*cursor].u,
                                       glyphs[*cursor].v, 16, 16, color, texture);
                }
                x += glyphs[*cursor].adv;
            }
        }
        character_count++;
        if (character_count == character_limit) {
            break;
        }
        cursor++;
    } while (*cursor != 0);
}

extern void FontPrint(void *, void *, void *, void *, void *, long,
                          unsigned char *);

/* Same shape as func_001F7560/func_001F75D0 below, one argument wider:
   mode 1 vs 2, D_001DF3D0 vs D_001DF770. Seven arguments, so EABI puts
   the fifth through seventh in $8/$9/$10. */
/* FontPrintLarge */
void FontPrintLarge(void *a, void *b, void *c, void *d, void *e) {
    long mode = GetEffectTex64(1);

    FontPrint(a, b, c, d, e, mode, normal_font_metrics);
}

/* FontPrintSmall */
void FontPrintSmall(void *a, void *b, void *c, void *d, void *e) {
    long mode = GetEffectTex64(2);

    FontPrint(a, b, c, d, e, mode, small_font_metrics);
}

LINKER_REMNANT("asm/remnants/text", func_001F69E8);

struct Glyph {
    u8 u;
    u8 v;
    s8 top;
    s8 adv;
};
extern s32 font_window_active MACRO_ADDR;
extern s32 font_palette_colors[];
extern f32 IntToFloat(s32);
extern void emit_textured_rectangle_r(f32, f32, f32, f32, s32, s32, s32, s32, u64, s64) __asm__("func_001F5BB8");
void process_bgm_display_text_event(u64 color, u8 *s, f32 x, s32 n, s64 tex, struct Glyph *g, f32 y, f32 scale);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/audio/music/process_bgm_display_text_event.c, process_bgm_display_text_event. */
void process_bgm_display_text_event(u64 color, u8 *str, f32 x, s32 n, s64 tex, struct Glyph *g, f32 y, f32 scale) {
    u8 *s;
    s32 i;
    f32 size;
    f32 top;
    f32 dx;
    f32 dy;
    struct Glyph *e;
    s32 avg;
    s32 mk;

    if (font_window_active == 0) {
        font_palette_colors[0] = color;
    }
    size = scale * 16.0f;
    i = 0;
    if (n == 0 || *str == 0) {
        return;
    }
    s = str;
    do {
        if ((u8)(*s - 8) < 8) {
            if ((*(s32 *)&GlobalStateFlag) != 0) {
                color &= 0xFF000000;
                color |= font_palette_colors[*s - 8] & 0xFFFFFF;
            }
        } else if (g[*s].adv != 0) {
            top = IntToFloat(g[*s].top) * scale;
            if ((u8)(*s + 0x80) < 0x28) {
                e = (struct Glyph *)(((*s + 0x40) << 2) + (s32)g);
                dx = IntToFloat(e->adv) * scale;
                dy = IntToFloat(e->top) * scale;
                emit_textured_rectangle_r(x + dx, y + dy, size, size, e->u, e->v, 16, 16, color, tex);
            }
            if (*s < 0x20) {
                avg = (s32)((color & 0xFF) + ((color >> 8) & 0xFF) + ((color >> 16) & 0xFF)) / 3;
                mk = (s32)(color & 0xFF000000);
                mk += avg << 16;
                mk += avg << 8;
                avg += mk;
                emit_textured_rectangle_r(x, y + top, scale * 24.0f, scale * 16.0f, g[*s].u, g[*s].v, 24, 16, avg, tex);
            } else if (*s > 0x20) {
                emit_textured_rectangle_r(x, y + top, size, size, g[*s].u, g[*s].v, 16, 16, color, tex);
            }
            x += IntToFloat(g[*s].adv) * scale;
        }
        i++;
        if (i == n) {
            break;
        }
        s++;
    } while (*s != 0);
}

LINKER_REMNANT("asm/remnants/text", func_001F6CE0);

extern int measure_text_width_regular(unsigned char *, int);
extern int measure_text_width_small(unsigned char *, int);

/* func_001F6CF8/func_001F6D88/func_001F6E18 are func_001F68E8's family
   with a leading measure call: the same mode/table triple (1, 2, 3 and
   D_001DF3D0, D_001DF770, D_001DFB10), each paired with its own
   measuring helper, and the first argument stepped back by whatever that
   helper returns. */
void font_print_right(char *a, void *b, void *c, unsigned char *d, int e) {
    char *p = a - measure_text_width_regular(d, e);
    long mode = GetEffectTex64(1);

    FontPrint(p, b, c, d, (void *)e, mode, normal_font_metrics);
}

/* Right-aligned FontPrint with the second font. */
void font_print_right_small(char *a, void *b, void *c, unsigned char *d, int e) {
    char *p = a - measure_text_width_small(d, e);
    long mode = GetEffectTex64(2);

    FontPrint(p, b, c, d, (void *)e, mode, small_font_metrics);
}

/* Right-aligned FontPrint with the third font. */
void font_print_right_large(char *a, void *b, void *c, unsigned char *d, int e) {
    char *p = a - measure_text_width_large(d, e);
    long mode = GetEffectTex64(3);

    FontPrint(p, b, c, d, (void *)e, mode, large_font_metrics);
}

/* func_001F6EA8/func_001F6F40/func_001F6FD8 are the func_001F6CF8 triple
   centred instead of left-aligned: the step-back is half the measured
   value, and the adjusted position is returned. Typed all-int to match
   the extern func_001F7288 already declares for func_001F6FD8. */
/* FontPrintCenter */
int FontPrintCenter(int a, int b, int c, int d, int e) {
    int p = a - (measure_text_width_regular((unsigned char *)d, e) >> 1);
    long mode = GetEffectTex64(1);

    FontPrint((void *)p, (void *)b, (void *)c, (void *)d, (void *)e,
                  mode, normal_font_metrics);
    return p;
}

/* FontPrintCenterSmall */
int FontPrintCenterSmall(int a, int b, int c, int d, int e) {
    int p = a - (measure_text_width_small((unsigned char *)d, e) >> 1);
    long mode = GetEffectTex64(2);

    FontPrint((void *)p, (void *)b, (void *)c, (void *)d, (void *)e,
                  mode, small_font_metrics);
    return p;
}

/* FontPrintCenterLarge */
int FontPrintCenterLarge(int a, int b, int c, int d, int e) {
    int p = a - (measure_text_width_large((unsigned char *)d, e) >> 1);
    long mode = GetEffectTex64(3);

    FontPrint((void *)p, (void *)b, (void *)c, (void *)d, (void *)e,
                  mode, large_font_metrics);
    return p;
}

extern void VU1_setScissor(int, int, int, int);
struct FontWindow {
    short y_min, y_max, x_min, x_max, x_anchor, y_start;
    short out_w, out_h, line_h;
    unsigned short flags;
    short x_sub, y_sub;
};

/* FontPrintWindow */
void FontPrintWindow(void *win_p, long a1, void *text_p, int len, long a4, unsigned char *glyphs)
{
    struct FontWindow *win = (struct FontWindow *)win_p;
    unsigned char *text = (unsigned char *)text_p;
    short start[32];
    short endi[32];
    short colr_arr[32];
    int W, Wsave, line, i, brk, width, colr, n0, last, balanced;
    int k, y, flags, adv, c;

    VU1_setScissor(win->x_min, win->x_max - 1, win->y_min, win->y_max - 1);
    font_window_active = 1;
    if ((win->flags ^ 1) & 1) {
        W = win->x_max - win->x_anchor;
    } else {
        int l = win->x_anchor;
        int a = win->x_max - l;
        W = l - win->x_min;
        if (a < W)
            W = a;
        W = W * 2;
    }
    Wsave = W;
    colr = 0;
    balanced = 0;
    n0 = 0;
    last = 0;

    for (;;) {
        line = 0;
        i = 0;
        if (len != 0 && text[0] != 0) {
            for (;;) {
                start[line] = i;
                colr_arr[line] = colr;
                width = 0;
                brk = i;
                {
                    while (width < W) {
                        if (text[i] == 0x20 || text[i] < 0x10)
                            brk = i;
                        if (*(int *)&GlobalStateFlag != 0 && (unsigned char)(text[i] - 8) < 8)
                            colr = text[i] - 8;
                        if (text[i] < 2)
                            break;
                        adv = ((signed char *)glyphs)[text[i] * 4 + 3];
                        i++;
                        if (adv != 0)
                            width += adv;
                    }
                }
                endi[line] = brk;
                if ((short)brk == start[line])
                    endi[line] = i;
                i = endi[line];
                if (text[i] == 0x20 || text[i] < 0x10)
                    endi[line] = endi[line] - 1;
                line++;
                if (text[i] == 0) {
                    last = width;
                    break;
                }
                i++;
                if (i == len || text[i] == 0)
                    break;
            }
        }
        if (balanced || line < 2)
            break;
        if (n0 == 0)
            n0 = line;
        if (n0 < line) {
            W = Wsave;
            balanced = 1;
            continue;
        }
        if (last < W / 3) {
            W -= 16;
            continue;
        }
        break;
    }

    {
        struct FontWindow *dw = win;
        int cnt, w;
        dw->out_w = 0;
        y = dw->y_start;
        dw->out_h = line * dw->line_h;
        if (dw->flags & 2)
            y -= (line * dw->line_h) >> 1;
        for (k = 0; k < line; k++, y += dw->line_h) {
            if (y + dw->line_h < dw->y_min)
                continue;
            if (dw->y_max < y)
                continue;
            cnt = endi[k] - start[k] + 1;
            w = measure_text_width(text + start[k], cnt, glyphs);
            if (dw->out_w < w)
                dw->out_w = w;
            if (dw->flags & 4)
                continue;
            font_palette_colors[0] = a1;
            if (dw->flags & 8) {
                float fx;
                if (dw->flags & 1)
                    fx = (dw->x_anchor - (w >> 1)) + dw->x_sub * 0.0625f;
                else
                    fx = dw->x_anchor + dw->x_sub * 0.0625f;
                process_bgm_display_text_event(font_palette_colors[colr_arr[k]], text + start[k], fx, cnt, a4, (struct Glyph *)glyphs,
                              y + dw->y_sub * 0.0625f, 1.0f);
            } else if (dw->flags & 1) {
                FontPrint((void *)(dw->x_anchor - (w >> 1)), (void *)y, (void *)font_palette_colors[colr_arr[k]],
                              text + start[k], (void *)cnt, a4, glyphs);
            } else {
                FontPrint((void *)(int)dw->x_anchor, (void *)y, (void *)font_palette_colors[colr_arr[k]],
                              text + start[k], (void *)cnt, a4, glyphs);
            }
        }
    }
    font_window_active = 0;
    VU1_setScissor(0, g_screen_extent[0] - 1, 0, g_screen_extent[1] - 1);
}

extern int GetEffectTex(int);

/* func_001F7560 and func_001F75D0 are the same call with a different
   mode (1 vs 2) and a different table. Six arguments: EABI passes the
   fifth and sixth in $8/$9, which is why they appear alongside $4-$7
   rather than on the stack. */
void font_print_window_regular(void *a, void *b, void *c, void *d) {
    int mode = GetEffectTex(1);

    FontPrintWindow(a, b, c, d, mode, normal_font_metrics);
}

/* FontPrintWindow with the second font. */
void font_print_window_small(void *a, void *b, void *c, void *d) {
    int mode = GetEffectTex(2);

    FontPrintWindow(a, b, c, d, mode, small_font_metrics);
}

LINKER_REMNANT("asm/remnants/text", func_001F7640);

/* FontSetWindow */
void FontSetWindow(void *arg0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) {
    short *s = (short *)arg0;
    s[0] = a1;
    s[1] = a2;
    s[2] = a3;
    s[3] = a4;
    s[4] = a5;
    s[5] = a6;
    s[8] = a7;
    s[9] = a8;
    s[6] = 0;
    s[7] = 0;
    s[10] = 0;
    s[11] = 0;
}

extern int draw_buffer_address MACRO_ADDR;
extern int frame_size[];
#define PK ((unsigned char *)D_00161000)

/* Uploads a block to the GS in DMA packets. */
void queue_framebuffer_upload(unsigned int source) {
    int remaining = frame_size[0];
    int destination = draw_buffer_address >> 8;
    unsigned long count;
    unsigned long transfer;
    unsigned char *packet;
    unsigned long *words;

    do {
        count = (long)remaining;
        if (count > 0x80)
            count = 0x80;
        *(unsigned int *)(PK + 0) = 0x10000006;
        remaining -= 0x80;
        *(unsigned int *)(PK + 4) = 0;
        *(unsigned int *)(PK + 8) = 0;
        *(unsigned int *)(PK + 12) = 0x50000006;
        packet = PK;
        g_render_packet_cursor = (int *)(packet + 0x10);
        transfer = (unsigned long)count << 7;
        words = (unsigned long *)(packet + 0x10);
        words[0] = 0x4000000000000001UL;
        words[1] = 0x0eeeeeee;
        words[2] = ((unsigned long)(unsigned int)destination << 32) | 0x8000000000000UL;
        words[3] = 0x50;
        words[4] = 0;
        words[5] = 0x51;
        words[6] = ((unsigned long)count << 32) | 0x200;
        words[7] = 0x52;
        words[8] = 0;
        words[9] = 0x53;
        words[10] = transfer | 0x0800000000008000UL;
        words[11] = 0;
        g_render_packet_cursor = (int *)(packet + 0x70);
        *(unsigned int *)(packet + 0x70) = (unsigned int)(long)(int)(transfer | 0x30000000);
        *(unsigned int *)(PK + 4) = source;
        source += (int)(count * 0x800);
        destination += (int)(count * 8);
        *(unsigned int *)(PK + 8) = 0;
        *(unsigned int *)(PK + 12) = (unsigned int)transfer | 0x50000000;
        g_render_packet_cursor = (int *)(PK + 0x10);
    } while (remaining > 0);
    VU1_texFlush();
}

extern short font_depth_bias;              /* SDA, gp -0x78B8 */
extern unsigned short font_vu_program_tag NOT_SDA;
extern char font_vu_program[];
extern char camera_position[];
extern void set_scale_matrix(float *, float);
extern void vu1_gs_regs_font(void);

/* Build the VU1 setup packet: load microprogram 7 if another is
   resident, then a DIRECT/UNPACK chain with the two camera matrices
   (each scaled to 1024 and biased in z by D_0015F448), the screen scale
   and offsets from D_0018CE00, and the GIF register setup; the DMA tag's
   qword count is patched in at the end. */
void font_queue_vu_state(void) {
    float m[16];
    char *cam;
    char *g;
    int *base;
    int *p;

    set_scale_matrix(m, 1024.0f);
    cam = camera_position;
    FastVecScale(&m[12], cam, -1024.0f);
    m[15] = 1.0f;
    if (font_state != 7) {
        VU1_addDataRef(font_vu_program, font_vu_program_tag);
        font_state = 7;
    }
    g_render_packet_cursor[0] = 0x10000000;
    g_render_packet_cursor[1] = 0;
    g_render_packet_cursor[2] = 0x11000000;
    g_render_packet_cursor[3] = 0x01000404;
    base = g_render_packet_cursor;
    base[4] = 0;
    base[5] = 0;
    base[6] = 0;
    base[7] = 0x6C0C43A4;
    p = base + 8;
    sce_vu0_mul_matrix(p, cam - 0x100, m);
    *(float *)(p + 14) += *(float *)&font_depth_bias;
    p = base + 0x18;
    sce_vu0_mul_matrix(p, cam - 0x80, m);
    *(float *)(p + 14) += *(float *)&font_depth_bias;
    g = (char *)view_context;
    base[0x28] = 0x8000;
    base[0x29] = 0x303EC000;
    base[0x2A] = 0x412;
    *(float *)(base + 0x2B) = *(float *)(g + 0x210);
    p = base + 0x2C;
    qcopy(p, g + 0x190);
    p = base + 0x30;
    qcopy(p, g + 0x1A0);
    *(float *)(base + 0x34) = *(float *)(g + 0x22C);
    *(float *)(base + 0x35) = *(float *)(g + 0x228);
    base[0x36] = 0;
    base[0x37] = 0;
    base[0x38] = 0x03000000;
    base[0x39] = 0x020001D2;
    base[0x3A] = 0x15000000;
    base[0x3B] = 0;
    p = base + 0x3C;
    g_render_packet_cursor[0] |= ((char *)p - (char *)g_render_packet_cursor >> 4) - 1;
    g_render_packet_cursor = p;
    vu1_gs_regs_font();
}

extern void setup_render_target(int, int, int);
extern short render_target_base_page NOT_SDA;

/* Sets up a (1 << a) x (1 << b) area, as vendor.c's func_0023A948 does:
   func_001FB608 gets the sizes and a base address, which is D_001519EE
   pages when `flag` is set, else D_0015EF8C less 4 << min(a + b, 16)
   bytes rounded down to a page (8 KB); func_001F3760 gets the sizes and
   the float setup (f, 0, 524288, 255, 0); GS registers 0x47 (0 with the
   flag, 0x30000 without) and 0x42 are then written. Each arm makes its
   own page-aligned base, so the two trailing `<< 13`s are cross-jumped
   into the one retail has ahead of the argument moves. */
void configure_graphics_render_target_and_projection(int a, int b, int flag, float f) {
    int base;

    if (flag != 0) {
        base = render_target_base_page << 13;
    } else {
        int t = a + b;

        if (t > 16) {
            t = 16;
        }
        base = g_gs_texture_allocation_base - (4 << t);
        base = (base >> 13) << 13;
    }
    setup_render_target(a, b, base);
    configure_graphics_projection(1 << a, 1 << b, f, 0.0f, 524288.0f, 255.0f, 0.0f);
    if (flag != 0) {
        VU1_addGSregister(0x47, 0);
    } else {
        VU1_addGSregister(0x47, 0x30000);
    }
    VU1_addGSregister(0x42, 0x8000000044L);
}

__asm__(".section .text\n\tnop\n");

extern void InitViewContext(void);

/* Frame start for the large draw buffer. */
void begin_draw_frame(void) {
    PutDrawBufferLarge();
    InitViewContext();
    UpdateViewContext();
}

/* Two prototypes for one symbol: func_001F55C0 passes a 64-bit value
   (retail shifts it with dsll), func_001F7B70 passes plain ints
   (addiu, not daddiu). */
extern int font_quad_count MACRO_ADDR;
extern short font_depth_bias;              /* SDA, gp -0x78B8 */
extern void prepare_font_quads(void);
extern void font_queue_vu_state(void);
extern void draw_font_quad_packets(void);

/* Optional extra draw pass, enabled by D_0015F578. */
void draw_debug_font(void) {
    if (font_quad_count != 0) {
        VU1_addGSregister(8, 5);
        VU1_addGSregister(0x14, 0x61);
        VU1_addGSregister(0x47, 0x513F1);
        VU1_addGSregister(0x4A, 1);
        prepare_font_quads();
        *(float *)&font_depth_bias = -0.04f;
        font_queue_vu_state();
        draw_font_quad_packets();
        *(int *)&font_depth_bias = 0;
        VU1_addGSregister(0x4A, 0);
    }
}

extern int bit_swap_lut[];

/* initialize_alpha_lookup_table: fills the alpha lookup table D_0018E840. */
void initialize_alpha_lookup_table(void) {
    int i;
    for (i = 0; i < 0x100; i++) {
        int v = i & 0xE7;
        if (i & 0x8)  v |= 0x10;
        if (i & 0x10) v |= 0x8;
        bit_swap_lut[v] = (i >> 1) << 24;
    }
}

LINKER_REMNANT("asm/remnants/text", func_001F7C50);
