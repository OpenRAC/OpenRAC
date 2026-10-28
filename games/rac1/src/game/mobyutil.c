#include "common.h"
#include "moby.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * mobyutil.cpp in the original source; text 0x213A78-0x2161E0.
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
extern void CreatePointLight(int);
extern void DetachPointLight(int);
extern void update_world_object_animation(void *);
extern int render_sequence NOT_SDA;
extern int vu1_chain_buffer_2 NOT_SDA;
extern int current_vu_chain;
extern int map_slot_use[];
extern void move_map_entry_slot(int a, int b);
extern PadSlots level_map_selection_slots __asm__("D_001A01F0");
extern int level_map_selection[];
extern int *menu_level_indices;
extern unsigned char menu_item_enabled_0 NOT_SDA;
extern unsigned char g_menu_item_enabled_1 NOT_SDA;
extern unsigned char menu_item_enabled_9 NOT_SDA;
extern short menu_selection_count;
extern unsigned char menu_item_enabled_16 NOT_SDA;
extern unsigned char menu_item_enabled_17 NOT_SDA;
extern unsigned char menu_item_enabled_18 NOT_SDA;
extern unsigned char menu_item_enabled_19 NOT_SDA;
extern unsigned char menu_item_enabled_25 NOT_SDA;
extern int map_region_enabled_3 NOT_SDA;
extern unsigned char menu_item_enabled_41 NOT_SDA;
extern int hero_control_mode NOT_SDA;
extern unsigned char menu_item_enabled_36 NOT_SDA;
extern unsigned char menu_item_enabled_37 NOT_SDA;
extern unsigned char menu_item_enabled_38 NOT_SDA;
extern unsigned char menu_item_enabled_55 NOT_SDA;
extern unsigned char menu_item_enabled_56 NOT_SDA;
extern unsigned char menu_item_enabled_57 NOT_SDA;
extern unsigned char menu_item_enabled_68;
extern unsigned char menu_item_enabled_64 NOT_SDA;
extern unsigned char menu_item_enabled_65 NOT_SDA;
extern unsigned char menu_item_enabled_66 NOT_SDA;
extern unsigned char menu_item_enabled_67 NOT_SDA;
extern unsigned char menu_item_enabled_69 NOT_SDA;
extern unsigned char menu_item_enabled_77 NOT_SDA;
extern unsigned char menu_item_enabled_102 NOT_SDA;
extern unsigned char menu_item_enabled_103 NOT_SDA;
extern unsigned char menu_item_enabled_104 NOT_SDA;
extern unsigned char menu_item_enabled_105 NOT_SDA;
extern unsigned char menu_item_enabled_115 NOT_SDA;
extern int max_map_coverage_size[];
extern void DebugEnter_Nop(void);
extern int compress_map_coverage(void *dst, int size, int a, int b);
extern void downsample_map_coverage(void *dst);
extern short g_current_level_index;
extern int g_current_level_index_far __asm__("D_0015EE84") NOT_SDA;
extern int map_coverage_active[] NOT_SDA;
extern void decode_compressed_occlusion_map(void *, unsigned char *, int);
extern void expand_map_coverage(void *, unsigned char *);
extern char memory_card_state[];
extern short mode_freeze_state;
extern int mode_freeze_flags;
extern int mc_gamedata[];
extern int mc_leveldata[];
extern int memcard_GetDataSize(int *p);
extern int memcard_PrepData(void *dst, int i, int *table);
extern int sce_mc_init(void);
extern int STUB_printf();
extern char memcard_error_message[];
extern int race_gates_24_status NOT_SDA;
extern unsigned char menu_item_enabled_12 NOT_SDA;
extern int race_gates_47_flag NOT_SDA;
extern unsigned char gSpecialItems[];
extern unsigned char gHaveHeliPack NOT_SDA;
extern int race_gates NOT_SDA;
extern int race_gates_66_status NOT_SDA;
extern unsigned char menu_item_enabled_73 NOT_SDA;
extern int race_gates_70_status NOT_SDA;
extern unsigned char menu_item_enabled_85 NOT_SDA;
extern int race_gates_87_status NOT_SDA;
extern unsigned char race_gates_base NOT_SDA;
extern int item_available NOT_SDA;
extern unsigned char menu_item_enabled_20 NOT_SDA;
extern unsigned char gemlik_available NOT_SDA;
extern unsigned char gHaveMorphORay NOT_SDA;
extern unsigned char gHaveHologuise NOT_SDA;
extern int moby_render_slots[];
extern void copy_matrix_rotation_2(void *, void *) __asm__("func_001FA460");
extern void FastVecScale(void *, void *, float);
extern void FastVecAdd(void *, void *, void *);
extern void matrix_mul_vec3(void *, void *, void *);
extern int g_hero_moby NOT_SDA;
extern float shadow_light_dir[] NOT_SDA;
extern float FastCos(float);
extern float FastSin(float);
extern void flush_cache(int);
extern char normal_lut[];
extern int render_packet_end;
extern short vu1_chain_safe_end;
extern char moby_backup_buffer[];
extern float FastAbsF(float arg0);
extern int find_or_allocate_id_slot(int v);
extern void prune_moby_references(void);

ASM_FUNC("asm/handwritten/text", func_00213A78);

LINKER_REMNANT("asm/remnants/text", func_00213BAC);

typedef struct {
    int key;
    int a;
    int b;
} Rec0C;
extern Rec0C moby_class_data_table[];
extern int resident_moby_class_data[];
extern char *moby_class_resources[];
extern int resident_moby_class_count MACRO_ADDR;

/* Looks arg0 up in the {key, a, b} table D_001E8F00 (ended by key -1).
   D_00160000 is read at every use, not cached in a local: CSE then
   reproduces retail's load order and its surviving register copy. */
struct Obj {
    u8 pad_00[0x2C];
    s32 extra;                       /* 0x2C */
};

/* Looks id up in the {key, a, b} table D_001E8F00 (ended by key -1).
   D_00160000 is read at every use, not cached in a local: CSE then
   reproduces retail's load order and its surviving register copy. */

void assign_moby_class_data(int id) {
    struct Obj *class_resource;
    int i = 0;

    class_resource = moby_class_resources[resident_moby_class_count];
    while (moby_class_data_table[i].key != -1 && moby_class_data_table[i].key != id) {
        i++;
    }
    resident_moby_class_data[resident_moby_class_count] = moby_class_data_table[i].a;
    if (class_resource != 0) {
        ((struct Obj *)moby_class_resources[resident_moby_class_count])->extra = moby_class_data_table[i].b;
    }
}

LINKER_REMNANT("asm/remnants/text", func_00213C70);

extern void *build_resident_visibility_list(void);
extern void advance_moby_animation(void *);
extern void refresh_moby_spatial_bounds(void *);
extern void *visible_moby_list MACRO_ADDR;

void update_visible_resident_objects(void) {
    char *m;

    visible_moby_list = build_resident_visibility_list();
    m = visible_moby_list;
    if (m != 0) {
        while (1) {
            if ((signed char)*(char *)&((Moby *)m)->state < 0) {
                m = *(char **)&((Moby *)m)->next;
            } else {
                if (!(((Moby *)m)->flags & 0x40)) {
                    advance_moby_animation(m);
                }
                /* inline asm: hook call (beq/move/jal with empty slots). The original used inline asm here too: GCC 2.95 reorg leaves both slots empty only next to asm (see reorg.c stop_search_p). */
                { int r; __asm__ volatile("beq %1,$0,1f\n\tmove $4,%2\n\tjal $31,%1\n1:" : "=r"(r) : "r"(*(void (**)(void *))(m + 0x74)), "r"(m) : "$4", "$31", "memory"); }
                if (((Moby *)m)->flags & 0x4) {
                    m = *(char **)&((Moby *)m)->next;
                } else {
                    refresh_moby_spatial_bounds(m);
                    m = *(char **)&((Moby *)m)->next;
                }
            }
            if (m == 0) break;
        }
    }
}

LINKER_REMNANT("asm/remnants/text", func_00213D10);

typedef struct {
    char _pad00[0x48];
    AnimSeq *seqs[1]; /* 0x48 */
} AnimClass;
typedef struct {
    char _pad00[0x24];
    AnimClass *pClass;       /* 0x24 */
    char _pad28[0x50 - 0x28];
    unsigned char frame;     /* 0x50 */
    unsigned char nextFrame; /* 0x51 */
    unsigned char seq;       /* 0x52 */
    unsigned char prevSeq;   /* 0x53 */
    char _pad54[0x5C - 0x54];
    float unk5C;             /* 0x5C */
    char _pad60[0x68 - 0x60];
    float *frameData;        /* 0x68 */
    char _pad6C[4];
    unsigned char unk70;     /* 0x70 */
} MobyAnim;
extern void update_moby_animation_state_a(void *) __asm__("func_0020D6D0");

/* Sets moby m's animation to sequence seq at frame (clamped to the
   sequence's last frame), the next frame to frame + 1 (clamped the same
   way, 0 if still out of range), refreshes the frame pointers
   (func_0020D6D0) and copies the first float of the new frame to +0x5C.
   The frame count is re-read through the class at each test, as retail
   reloads it after the byte stores. */
void MobySetAnimation(MobyAnim *m, int seq, int frame) {
    int n = m->pClass->seqs[seq]->nframes;

    m->seq = seq;
    if (frame >= n) {
        frame = n - 1;
    }
    m->frame = frame;
    m->nextFrame = frame + 1;
    if (m->nextFrame > m->pClass->seqs[seq]->nframes - 1) {
        m->nextFrame = m->pClass->seqs[seq]->nframes - 1;
    }
    m->prevSeq = seq;
    if (m->nextFrame >= m->pClass->seqs[seq]->nframes) {
        m->nextFrame = 0;
    }
    update_moby_animation_state_a(m);
    m->unk5C = *m->frameData;
    m->unk70 &= ~2;
}

extern float IntToFloat(int arg0);
extern void capture_moby_blend_pose(void *, int);
extern char moby_blend_slot_poses[];

/* As func_00213F28 below, for a plain sequence change: clamp the frame
   to the sequence's last one; if the moby hasn't settled, park the
   current pose in a D_001B2F80 blend slot (seq 0xFF, frame = slot). Then
   set the new sequence and frame and arm the blend timer from arg3. */
void MobyBlendAnimation(MobyAnim *arg0, int arg1, int arg2, int arg3) {
    int n = arg0->pClass->seqs[arg1]->nframes;
    int slot;
    unsigned char oldSeq;
    float scale;

    if (arg2 >= n) {
        arg2 = n - 1;
    }
    if (*(float *)((char *)arg0 + 0x54) > 0.025f ||
        *(int *)((char *)arg0 + 0x60) != 0 ||
        *(int *)((char *)arg0 + 0x64) != 0) {
        slot = find_or_allocate_id_slot((int)arg0);
        if (slot >= 0) {
            capture_moby_blend_pose(arg0, slot | 0x300);
            qcopy(moby_blend_slot_poses + slot * 0x10, (char *)arg0 + 0xF0);
            oldSeq = arg0->seq;
            if (oldSeq != 0xFF) {
                *(unsigned char *)((char *)arg0 + 0xA5) = oldSeq;
            }
            arg0->seq = 0xFF;
            arg0->frame = slot;
        }
    }
    arg0->nextFrame = arg2;
    arg0->prevSeq = arg1;
    update_moby_animation_state_a(arg0);
    *(float *)((char *)arg0 + 0x58) = 1.0f;
    scale = 1.0f / IntToFloat(arg3);
    *(float *)((char *)arg0 + 0x54) = 0.0f;
    arg0->unk70 = (unsigned char)(arg0->unk70 & 0xFD);
    arg0->unk5C = scale;
    *(unsigned char *)((char *)arg0 + 0x7C) =
        *((unsigned char *)arg0->pClass->seqs[arg1] + 0x11);
}

/* Re-registers this MobyAnim in the shared slot table (D_001B2F40, via
   func_0020DA68) when it hasn't settled yet (unk54 > 0.025, or unk60/unk64
   nonzero) or the caller forces it (arg4 & 4): builds a flags byte from
   arg4 bits 0/1 (0x100/0x200), hands it to func_0020FC38, snapshots this
   moby's unkF0 vector into the matching D_001B2F80 slot, remembers the
   old seq in unkA5 (unless it was already 0xFF), then marks seq 0xFF and
   frame = slot. Either way it then sets nextFrame/prevSeq from arg2/arg1,
   refreshes frame pointers (func_0020D6D0), arms the timer (unk58 = 1),
   resets unk54, clears unk70 bit 1, stores 1/func_001FA888(arg3) into
   unk5C, and copies a not-yet-named byte (offset 0x11) out of
   pClass->seqs[arg1] into unk7C. */
void blend_moby_animation_ex(MobyAnim *arg0, int arg1, int arg2, int arg3, int arg4) {
    int slot;
    int flags;
    unsigned char oldSeq;
    float scale;

    if (*(float *)((char *)arg0 + 0x54) > 0.025f ||
        *(int *)((char *)arg0 + 0x60) != 0 ||
        *(int *)((char *)arg0 + 0x64) != 0 || (arg4 & 4)) {
        slot = find_or_allocate_id_slot((int)arg0);
        if (slot >= 0) {
            flags = (arg4 & 1) ? (slot | 0x100) : slot;
            capture_moby_blend_pose(arg0, (arg4 & 2) ? (flags | 0x200) : flags);
            qcopy(moby_blend_slot_poses + slot * 0x10, (char *)arg0 + 0xF0);
            oldSeq = arg0->seq;
            if (oldSeq != 0xFF) {
                *(unsigned char *)((char *)arg0 + 0xA5) = oldSeq;
            }
            arg0->seq = 0xFF;
            arg0->frame = slot;
        }
    }
    arg0->nextFrame = arg2;
    arg0->prevSeq = arg1;
    update_moby_animation_state_a(arg0);
    *(float *)((char *)arg0 + 0x58) = 1.0f;
    scale = 1.0f / IntToFloat(arg3);
    *(float *)((char *)arg0 + 0x54) = 0.0f;
    arg0->unk70 = (unsigned char)(arg0->unk70 & 0xFD);
    arg0->unk5C = scale;
    *(unsigned char *)((char *)arg0 + 0x7C) =
        *((unsigned char *)arg0->pClass->seqs[arg1] + 0x11);
}

LINKER_REMNANT("asm/remnants/text", func_00214080);

extern int func_001160D8(void);

/* Returns a random integer from 0 to arg0 - 1. */
int random_integer_below(int arg0) {
    return ((func_001160D8() >> 16) & 0x7FFF) % arg0;
}

LINKER_REMNANT("asm/remnants/text", func_002140F0);

/* As func_00214158: the old note's C, with the mtc1 nop added by the
   pipeline. */
float random_float_between(float a, float b) {
    int v = func_001160D8();
    float delta = b - a;
    v = (v >> 16) & 0x7FFF;
    return a + (float)v * delta * 3.0517578125e-05f;
}

/* The C of the old revert note: the nop it lacked is ps2eeas's (after
   an mtc1), which tools/ps2eeas_nops.py now adds. */
float random_angle_radians(void) {
    int v = ((func_001160D8() >> 16) & 0xFFF) - 0x800;
    return (float)v * 3.14159274f * 0.00048828125f;
}

extern float random_angle_radians(void);
extern float random_float_between(float, float);
extern void BuildSphericalOffset(void *, float, float, float);

/* Writes a random offset with length between arg1 and arg2 in a random direction. */
void random_spherical_offset(void *arg0, float arg1, float arg2) {
    float r1 = random_angle_radians();
    float r2 = random_angle_radians();

    BuildSphericalOffset(arg0, random_float_between(arg1, arg2), r1, r2);
}

/* Cosine interpolation: a + (b - a) * ((1 - cos(t * pi)) * 0.5). */
float CosineInterpolate(float a, float b, float t) {
    if (t == 0.0f) {
        return a;
    }
    if (t == 1.0f) {
        return b;
    }
    return a + (b - a) * ((1.0f - FastCos(t * 3.14159274f)) * 0.5f);
}

LINKER_REMNANT("asm/remnants/text", func_002142B8);

extern int coll_line_fix_a(void *, void *, int, int, int) __asm__("func_001EFE10");
extern char coll_hit_pos[];

/* Builds two 16-byte copies of *arg0: one with byte offset 8 (a float)
   forced to 0.01f, the other with its offset-8 float bumped by arg2.
   Passes both to func_001EFE10 (a collision/line test elsewhere in the
   file's neighbours); returns D_00194220's float at +8 on success, else
   0.0f. */
f32 ProbeGroundHeight(void *arg0, s32 arg1, f32 arg2) {
    char sp0[16];
    char sp1[16];

    qcopy(sp0, arg0);
    *(f32 *)(sp0 + 8) = 0.01f;
    qcopy(sp1, arg0);
    *(f32 *)(sp1 + 8) = *(f32 *)(sp1 + 8) + arg2;
    if (coll_line_fix_a(sp1, sp0, arg1 | 2, 0, 0) != 0) {
        return *(f32 *)(coll_hit_pos + 8);
    }
    return 0.0f;
}

LINKER_REMNANT("asm/remnants/text", func_002143D0);

extern int occl_cell_count MACRO_ADDR;
extern int local_surface_region_enabled MACRO_ADDR;
extern float local_surface_region_center_x[4] MACRO_ADDR;
extern int sample_surface_height_map(float *, void *, float, float, float);
extern float vec_distance_xy(void *, void *);
extern void set_vector_unit_z(void *);

/* Ground height under pos: func_0023B210's answer when D_00161298 is on
   and it finds one (it writes the height through its first argument and
   gets `out` as well); else, when D_0016129C is on and pos lies within
   0.5 of the height of the disc D_001612A0 (x, y, height, radius) and
   inside its radius (func_001F9D48 is an XY distance), the disc's
   height; else pos's own z. Whenever the answer is not func_0023B210's,
   a non-null `out` is reset by func_001F9BC8. The disc is one MACRO_ADDR
   array: its fields are symbol+offset accesses, which the compiler
   counts as two instructions and so keeps out of delay slots, as retail
   has them. */
float resolve_camera_surface_height(float *pos, void *out) {
    float h;

    if (occl_cell_count != 0) {
        h = pos[2];
        if (sample_surface_height_map(&h, out, pos[0], pos[1], h) != 0) {
            return h;
        }
    }
    if (local_surface_region_enabled != 0 && FastAbsF(pos[2] - local_surface_region_center_x[2]) < 0.5f
        && vec_distance_xy(pos, local_surface_region_center_x) < local_surface_region_center_x[3]) {
        if (out != 0) {
            set_vector_unit_z(out);
        }
        return local_surface_region_center_x[2];
    }
    if (out != 0) {
        set_vector_unit_z(out);
    }
    return pos[2];
}

LINKER_REMNANT("asm/remnants/text", func_00214538);

extern char coll_output[];
extern void vec_scale_xy_to_length(float *, float *, float);
extern void fast_vec_normalize(void *, void *, float);
extern float min_float(float, float);
extern float max_float(float, float);

/* Shadow range probe: cast a ray down (8 units) from the moby position
   shifted against the light direction D_001CAE00 by its size, then a
   second one along the light from just above; +0x84/+0x88 get the lower
   and upper hit height (at most 4 apart), or 0 when nothing is below. */
void update_moby_shadow_range(char *m) {
    float dir[4];
    float p[4];
    float a[4];
    float b[4];
    float h2;
    float h1;
    float t;
    char *hit;

    qcopy(dir, shadow_light_dir);
    vec_scale_xy_to_length(dir, dir, 1.0f);
    FastVecScale(p, m, 0.0009765625f);
    qcopy(a, p);
    a[0] -= dir[0] * *(float *)(m + 0xC) * 0.000732421875f;
    a[1] -= dir[1] * *(float *)(m + 0xC) * 0.000732421875f;
    qcopy(b, a);
    b[2] -= 8.0f;
    if (coll_line_fix_a(a, b, 0x22, 0, 0) != 0) {
        qcopy(a, p);
        hit = coll_output;
        h1 = *(float *)(hit + 0x28);
        a[2] = a[2] + *(float *)(m + 0xC) * 0.00048828125f;
        a[0] = a[0] + dir[0] * *(float *)(m + 0xC) * 0.000732421875f;
        a[1] = a[1] + dir[1] * *(float *)(m + 0xC) * 0.000732421875f;
        fast_vec_normalize(dir, dir, (a[2] - h1) / -dir[2]);
        FastVecAdd(b, a, dir);
        h2 = h1;
        if (coll_line_fix_a(a, b, 0x22, 0, 0) != 0) {
            h2 = *(float *)(hit + 0x28);
        }
        *(float *)(m + 0x84) = min_float(h1, h2) - 0.25f;
        t = max_float(h1, h2) + 0.25f;
        *(float *)(m + 0x88) = t;
        if (*(float *)(m + 0x84) + 4.0f < t) {
            *(float *)(m + 0x88) = *(float *)(m + 0x84) + 4.0f;
        }
    } else {
        *(float *)(m + 0x84) = 0.0f;
        *(float *)(m + 0x88) = 0.0f;
    }
}

LINKER_REMNANT("asm/remnants/text", func_00214770);

extern short damage_class_131_a8_float_table;
extern unsigned char damage_reaction_code_table[];
extern char damage_reaction_scale_table[];
extern unsigned char hero_globals_base[];
extern char g_hero[];
extern int suck_get_aim_height(char *);
extern int read_state_field(char *);
extern int FastDecTimer(void *);
extern int scale_game_frame_count(int);
extern float FastVecLength(void *);

typedef struct { float v[4]; } __attribute__((aligned(16))) Q_214778;

/* Picks a moby's hit reaction from the damage record, scales the knockback; returns the reaction. */
int select_damage_reaction(char *a, char *B, char *an, unsigned int btn, int *outA, float *outF, int *outI, int k) {
    float v[4] __attribute__((aligned(16)));
    float w[4] __attribute__((aligned(16)));
    int s21;
    int a22;
    int u17;
    unsigned int c;

    *outA = 0;
    if (outI != 0)
        *outI = -1;

    if (B != 0 && *(char **)(B + 0x20) != 0) {
        int cls = *(short *)(*(char **)(B + 0x20) + 0xA6);
        if ((cls == 0x131 || cls == 0xA8) && an != 0)
            *(float *)(B + 0x2C) = ((float *)&damage_class_131_a8_float_table)[(unsigned char)an[8]];
    }

    if (a == 0) {
        if (an != 0) {
            *(short *)(an + 0x1C) = 0;
            *(short *)(an + 0x14) = -1;
        }
        *(short *)(an + 0x16) = -1;
        *outA = 0;
        return 0;
    }
    if (an == 0) {
        an = (char *)suck_get_aim_height(a);
        if (an == 0) {
            *outA = 0;
            return 0;
        }
    }
    if (k == 4)
        k = (unsigned char)an[8];

    if (B == 0) {
        if (an != 0) {
            FastDecTimer(an + 0x1C);
            *(short *)(an + 0x16) = -1;
        }
        *outA = 1;
        if (outF != 0)
            *outF = 0;
        return 0xB;
    }

    if (*(float *)(B + 0x1C) == 5627.9248f) {
        FastVecScale(B + 0x10, B + 0x10, *(float *)(damage_reaction_scale_table + (unsigned char)B[0x28] * 4 + k * 32));
        a22 = *(unsigned short *)(B + 0x2A);
        *(float *)(B + 0x1C) = 5627.9248f;
        if (a22 == 0x47) {
            char *h = g_hero;

            v[0] = FastCos(fast_arc_tan(*(float *)(a + 0x10) - *(float *)(h + 0x80),
                                               *(float *)(a + 0x14) - *(float *)(h + 0x84)));
            v[1] = FastSin(fast_arc_tan(*(float *)(a + 0x10) - *(float *)(h + 0x80),
                                               *(float *)(a + 0x14) - *(float *)(h + 0x84)));
            v[2] = 1.0f;
            fast_vec_normalize(v, v, FastVecLength(B + 0x10) * 0.7f);
            FastVecScale(B + 0x10, B + 0x10, 0.3f);
            FastVecAdd(w, B + 0x10, v);
            *(Q_214778 *)(B + 0x10) = *(Q_214778 *)w;
            vec_scale_xy_to_length((float *)(B + 0x10), (float *)(B + 0x10), 1.0f);
        }
    } else {
        if (*(char **)(B + 0x20) == 0) {
            a22 = 0x47;
            B[0x28] = 8;
        } else {
            a22 = *(short *)(*(char **)(B + 0x20) + 0xA6);
            B[0x28] = 8;
        }
    }

    switch ((unsigned char)B[0x28]) {
    case 0:
        if ((unsigned int)(*(unsigned short *)(B + 0x2A) - 1) < 2) {
            *outA = 2;
            s21 = 0;
        } else {
            *outA = 2;
            s21 = 1;
        }
        break;
    case 1:
        if ((unsigned char)B[0x29] < 2) {
            *outA = 2;
            s21 = 0xB;
        } else if ((unsigned char)B[0x29] == 2) {
            *outA = 2;
            s21 = 5;
        } else {
            *outA = 2;
            s21 = 9;
        }
        break;
    case 2:
        if ((unsigned char)B[0x29] < 2) {
            *outA = 2;
            s21 = 8;
        } else {
            *outA = 2;
            s21 = 0xD;
        }
        break;
    case 3:
        c = (unsigned char)B[0x29];
        if (c < 2) {
            *outA = 2;
            s21 = 0xA;
        } else if (c < 3) {
            *outA = 2;
            s21 = 0xF;
        } else {
            *outA = 2;
            s21 = 0x10;
            *(short *)(an + 0x14) = -1;
            *(short *)(an + 0x1C) = 0;
        }
        break;
    case 4:
        *outA = 2;
        s21 = 0xE;
        break;
    case 5:
        if ((unsigned char)B[0x29] < 2) {
            *outA = 3;
            s21 = 4;
        } else {
            *outA = 4;
            s21 = 7;
        }
        break;
    case 7:
        if ((unsigned char)B[0x29] < 2) {
            *outA = 2;
            s21 = 0xC;
        } else {
            *outA = 2;
            s21 = 0x11;
        }
        break;
    default:
        *outA = 2;
        s21 = 1;
        break;
    }

    if (outI != 0)
        *outI = a22;

    u17 = damage_reaction_code_table[(unsigned char)an[8] + s21 * 4];
    if (*(short *)(an + 0x14) != a22)
        *(short *)(an + 0x1C) = 0;

    if (FastDecTimer(an + 0x1C) != 0) {
        switch (s21) {
        case 0:
            *(short *)(an + 0x1C) = scale_game_frame_count(0x25);
            break;
        case 1:
            *(short *)(an + 0x1C) = scale_game_frame_count(0xF);
            break;
        case 4:
            *(short *)(an + 0x1C) = scale_game_frame_count(0x3C);
            break;
        case 7:
            if (hero_globals_base[0] != 0)
                *(short *)(an + 0x1C) = scale_game_frame_count(0x2D);
            else
                *(short *)(an + 0x1C) = scale_game_frame_count(0x3C);
            break;
        case 11:
            *(short *)(an + 0x1C) = scale_game_frame_count(0x3C);
            break;
        default:
            *(short *)(an + 0x1C) = scale_game_frame_count(0x1E);
            break;
        }
    } else {
        switch (s21) {
        case 0:
        case 1:
            if ((btn & 0x10) == 0)
                u17 = 0xB;
            if ((btn & 0x20) == 0)
                *outA = 1;
            *(float *)(B + 0x2C) = 0.0f;
            break;
        case 4:
            if ((btn & 1) == 0)
                u17 = 0xB;
            if ((btn & 2) == 0)
                *outA = 1;
            *(float *)(B + 0x2C) = 0.0f;
            break;
        case 7:
            if ((btn & 4) == 0)
                u17 = 0xB;
            if ((btn & 8) == 0)
                *outA = 1;
            *(float *)(B + 0x2C) = 0.0f;
            break;
        case 11:
            u17 = 0xC;
            break;
        default:
            *(float *)(B + 0x2C) = 0.0f;
            u17 = 0xD;
            break;
        }
    }

    *(short *)(an + 0x14) = a22;
    *(short *)(an + 0x16) = a22;
    if (outF != 0)
        *outF = *(float *)(B + 0x2C);

    if (s21 == 4) {
        char *p = (char *)read_state_field(a);
        if (p != 0)
            p[0x2E] = 1;
    } else {
        char *p = (char *)read_state_field(a);
        if (p != 0)
            p[0x2E] = 0;
    }
    return u17;
}

LINKER_REMNANT("asm/remnants/text", func_00214CF8);

/* Steps *p toward target by at most maxstep; returns the distance still left. */
float ApproachValue(float *p, float target, float maxstep) {
    float d = target - *p;
    if (maxstep < d) {
        d = maxstep;
    } else if (d < -maxstep) {
        d = -maxstep;
    }
    *p = *p + d;
    return FastAbsF(target - *p);
}

LINKER_REMNANT("asm/remnants/text", func_00214D80);


/* Moves *p1 toward `a` with velocity *p2: when *p2 heads away from the
   target (or it is reached), the velocity eases to 0 by c
   (func_00214D28) and is added. Otherwise, inside the braking distance
   (*p2^2 / c / 2) it eases to 0 by c, or by 1.1c once |diff| + |*p2| no
   longer exceeds that distance; outside it, it eases by b toward
   +-min(func_001F9B50(2c * diff), d) (func_001F9B50 is a square root).
   Then, if |*p2| < |diff| the velocity is added and returned, else *p1
   snaps to `a` and diff is returned. func_001F9B88 is fabsf; the fabs
   calls are made in retail's order through temporaries. */
float ApproachWithVelocity(float *p1, float *p2, float a, float b, float c, float d) {
    float diff = a - *p1;

    if (*p2 * diff >= 0.0f && diff != 0.0f) {
        float half = *p2 * *p2 / c * 0.5f;

        if (FastAbsF(diff) < half) {
            float s = FastAbsF(diff);
            s += FastAbsF(*p2);
            if (half < s) {
                ApproachValue(p2, 0.0f, c);
            } else {
                ApproachValue(p2, 0.0f, c * 1.1f);
            }
        } else {
            float speed = vec_length((c + c) * diff);

            if (d < speed) {
                speed = d;
            }
            if (diff < 0.0f) {
                ApproachValue(p2, -speed, b);
            } else {
                ApproachValue(p2, speed, b);
            }
        }
        {
            float m1 = FastAbsF(diff);
            float m2 = FastAbsF(*p2);

            if (m2 < m1) {
                *p1 = *p1 + *p2;
                return *p2;
            }
            *p1 = a;
            return diff;
        }
    }
    ApproachValue(p2, 0.0f, c);
    *p1 += *p2;
    return *p2;
}

LINKER_REMNANT("asm/remnants/text", func_00214F50);

extern void fast_vec_normalize(void *, void *, float);

struct M2c_temp3 {
    f32 values[3];
    s32 pad;
};
extern void fast_vec_normalize_r(f32 *, f32 *, f32) __asm__("func_001F9DC0");
void normalize_vector_triplet(f32 *arg0);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/math/vectors/normalize_vector_triplet.c, normalize_vector_triplet. */
void normalize_vector_triplet(f32 *arg0) {
    struct M2c_temp3 scratch;
    f32 *base;
    f32 *src;
    f32 *dst;
    f32 value;
    s32 offset;
    s32 i;
    s32 next;
    s32 count;

    base = arg0;
    i = 0;
    do {
        next = i + 1;
        offset = i * 4;
        scratch.pad = 0;
        dst = scratch.values;
        src = (f32 *)((u8 *)base + offset);
        count = 2;
        do {
            value = *src;
            count--;
            src += 4;
            *dst = value;
            dst++;
        } while (count >= 0);
        fast_vec_normalize_r(scratch.values, scratch.values, 1.0f);
        dst = (f32 *)((u8 *)base + offset);
        src = scratch.values;
        count = 2;
        do {
            value = *src;
            count--;
            src++;
            *dst = value;
            dst += 4;
        } while (count >= 0);
        i = next;
    } while (i < 3);
}

LINKER_REMNANT("asm/remnants/text", func_00215038);

/* A goto into the first `return 0` gives retail's backward beqz; the two
   nops before it are short-loop padding (tools/ps2eeas_nops.py). */
int suck_get_aim_height(char *arg0) {
    if (arg0 == 0) {
    ret0:
        return 0;
    }
    if ((((Moby *)arg0)->flags & 0x20) == 0) {
        goto ret0;
    }
    return *((int *)((Moby *)arg0)->pvars);
}

/* func_00215048 for the +0x10 field. */
int read_state_field(char *arg0) {
    if (arg0 == 0) {
    ret0:
        return 0;
    }
    if ((((Moby *)arg0)->flags & 0x20) == 0) {
        goto ret0;
    }
    return *(int *)(((char *)((Moby *)arg0)->pvars) + 0x10);
}

LINKER_REMNANT("asm/remnants/text", func_002150A8);

/* The {1, 2, 0} successor table of the quaternion extraction. */
typedef struct {
    int next[3];
} QuatNext;

extern QuatNext quat_next_axis_table;

/* Rotation matrix (rows of four floats) to quaternion (x, y, z, w), by
   Shoemake's method: from the trace when it is positive, else from the
   largest diagonal element, working on a packed 3x3 copy. */
void MatrixToQuaternion(void *arg0, void *arg1) {
    float *q = arg0;
    float (*m)[4] = arg1;
    QuatNext n = quat_next_axis_table;
    float mat[3][3];
    float trace;
    float s;
    int i, j, k;

    trace = m[0][0] + m[1][1] + m[2][2];
    if (trace > 0.0f) {
        s = vec_length(trace + 1.0f);
        q[3] = s * 0.5f;
        s = 0.5f / s;
        q[0] = (m[2][1] - m[1][2]) * s;
        q[1] = (m[0][2] - m[2][0]) * s;
        q[2] = (m[1][0] - m[0][1]) * s;
    } else {
        mat[0][0] = m[0][0];
        mat[0][1] = m[0][1];
        mat[0][2] = m[0][2];
        mat[1][0] = m[1][0];
        mat[1][1] = m[1][1];
        mat[1][2] = m[1][2];
        mat[2][0] = m[2][0];
        mat[2][1] = m[2][1];
        mat[2][2] = m[2][2];
        i = 0;
        if (mat[1][1] > mat[0][0]) {
            i = 1;
        }
        if (mat[2][2] > mat[i][i]) {
            i = 2;
        }
        j = n.next[i];
        k = n.next[j];
        s = vec_length(mat[i][i] - (mat[j][j] + mat[k][k]) + 1.0f);
        q[i] = s * 0.5f;
        if (s != 0.0f) {
            s = 0.5f / s;
        }
        q[3] = (mat[k][j] - mat[j][k]) * s;
        q[j] = (mat[j][i] + mat[i][j]) * s;
        q[k] = (mat[k][i] + mat[i][k]) * s;
    }
}

extern void copy_matrix_rotation(void *);
extern void MatrixToQuaternion(void *, void *);
extern void matrix_copy_rows(void *, void *);

/* func_001FA460 takes two arguments: arg1 is passed on to it untouched,
   which gives arg1 three references and retail's $s0. */
void extract_matrix_rotation_quaternion(void *arg0, void *arg1) {
    char buf[0x40];
    copy_matrix_rotation_2(buf, arg1);
    MatrixToQuaternion(arg0, buf);
    matrix_copy_rows(arg1, buf);
}

LINKER_REMNANT("asm/remnants/text", func_00215378);

extern float FastSin(float);  /* sin of a half-angle */
extern float FastCos(float);  /* cos of a half-angle */

/* Axis-angle -> quaternion: the xyz part is axis scaled by sin(angle/2)
   (done by func_001F9C30, which writes through arg0), and w at +0xC is
   cos(angle/2). Both trig calls take the same half-angle, which is why
   it lives in $f20 across all three calls. */
void build_quaternion_from_axis_angle(void *arg0, void *axis, float angle) {
    float half = angle * 0.5f;

    FastVecScale(arg0, axis, FastSin(half));
    *(float *)((char *)arg0 + 0xC) = FastCos(half);
}

typedef struct {
    float m[4][4];
} __attribute__((aligned(16))) Mtx44;
extern void clear_u64_value(float *);
extern void euler_to_matrix(float *, float *);
extern void euler_to_matrix_ee(float *, float *);

/* Matrix to Euler angles: on a copy of src with the translation cleared,
   read the Z angle from row 0, undo it (func_001FA218 builds the Z
   rotation, func_001FA540 multiplies), read and undo Y the same way,
   then read X from row 1; out = (x, y, z). The last undo vector is
   filled but never used. The 16-byte aligned struct copy is retail's
   four interleaved lq/sq. */
void matrix_to_euler(Mtx44 *src, float *out) {
    float rot[16];
    Mtx44 m;
    float v[4];
    float x;
    float y;
    float z;

    m = *src;
    clear_u64_value(m.m[3]);
    m.m[3][3] = 1.0f;
    z = fast_arc_tan(m.m[0][0], m.m[0][1]);
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = -z;
    euler_to_matrix(rot, v);
    sce_vu0_mul_matrix(&m, rot, &m);
    y = fast_arc_tan(m.m[0][0], -m.m[0][2]);
    v[0] = 0.0f;
    v[2] = 0.0f;
    v[1] = -y;
    euler_to_matrix_ee(rot, v);
    sce_vu0_mul_matrix(&m, rot, &m);
    x = fast_arc_tan(m.m[1][1], m.m[1][2]);
    v[1] = 0.0f;
    v[0] = -x;
    v[2] = 0.0f;
    out[2] = z;
    out[1] = y;
    out[0] = x;
}

/* 12 bytes of post-endlabel nop padding in retail -- see func_001F6668. */
__asm__(".section .text\n\tnop\n\tnop\n\tnop\n");

LINKER_REMNANT("asm/remnants/text", func_00215518);

typedef struct {
    char pad[0x30];
    float pos[4];     /* 0x30 */
    float mtx[4][4];  /* 0x40 */
} ViewBox;
extern ViewBox *clip_volume_table MACRO_ADDR;
extern void FastVecSub(void *, void *, void *);

/* Is point arg0 inside box arg1 of the table at D_00160134? The offset
   from the box's position, taken through its matrix, must lie in
   [-1, 1] on every axis. 0 for arg1 == -1. */
int is_point_inside_clip_volume(void *arg0, int arg1) {
    float d[4];
    float v[4];
    ViewBox *m;

    if (arg1 == -1) {
        return 0;
    }
    m = &clip_volume_table[arg1];
    FastVecSub(d, arg0, m->pos);
    d[3] = 0.0f;
    matrix_mul_vec3(v, d, m->mtx);
    if (v[0] >= -1.0f && v[0] <= 1.0f && v[1] >= -1.0f && v[1] <= 1.0f
        && v[2] >= -1.0f && v[2] <= 1.0f) {
        return 1;
    }
    return 0;
}

LINKER_REMNANT("asm/remnants/text", func_00215648);


/* Conjugate-style sandwich: arg0 = arg2 * (arg1 with w = 0) * a, where
   a is arg2 negated with its w kept (func_001FA588 is the quaternion
   multiply). The 16-byte copy of arg1 is qcopy's lq/sq. */
void rotate_vector_by_quaternion(void *arg0, void *arg1, void *arg2) {
    float a[4];
    float b[4];
    float c[4];

    FastVecScale(a, arg2, -1.0f);
    a[3] = ((float *)arg2)[3];
    qcopy(b, arg1);
    b[3] = 0.0f;
    multiply_quaternions(c, arg2, b);
    multiply_quaternions(arg0, c, a);
}

extern void build_quaternion_from_axis_angle(void *arg0, void *axis, float angle);
extern void rotate_vector_by_quaternion(void *arg0, void *arg1, void *arg2);

/* dst = vec rotated `angle` around axis. A tiny angle skips the rotation
   (dst = vec); otherwise the axis is normalised to unit length and turned
   into an axis-angle quaternion in a scratch buffer, which then rotates
   vec into dst. */
void build_look_at_matrix(void *dst, void *vec, void *axis, float angle) {
    float q[4];

    if (FastAbsF(angle) < 0.00001f) {
        qcopy(dst, vec);
        return;
    }
    fast_vec_normalize(q, axis, 1.0f);
    build_quaternion_from_axis_angle(q, q, angle);
    rotate_vector_by_quaternion(dst, vec, q);
}

LINKER_REMNANT("asm/remnants/text", func_00215788);

extern float frame_scale MACRO_ADDR;
extern float g_frame_scale_sq MACRO_ADDR;
extern float inv_frame_scale MACRO_ADDR;
extern float g_frame_time MACRO_ADDR;
extern float g_frame_time_sq MACRO_ADDR;
extern float g_frame_time_cube MACRO_ADDR;
extern int frame_scale_fifths MACRO_ADDR;
extern float frame_delta_time MACRO_ADDR;
extern int pal_mode MACRO_ADDR;

/* The EE80 flag is written first in each arm. Its float store lands in
   a delay slot, where check_macro_slots.py makes it $gp-relative. */
void set_video_timing(int pal) {
    if (pal == 0) {
        pal_mode = 0;
        frame_scale = 1.0f;
        g_frame_scale_sq = 1.0f;
        inv_frame_scale = 1.0f;
        g_frame_time = 0.01666666753590106964111328125f;
        g_frame_time_sq = 0.000277777784503996372222900390625f;
        g_frame_time_cube = 0.00000462962952951784245669841766357421875f;
        frame_scale_fifths = 5;
        frame_delta_time = 0.01666666753590106964111328125f;
    } else {
        pal_mode = 1;
        frame_scale = 1.2000000476837158203125f;
        g_frame_scale_sq = 1.440000057220458984375f;
        inv_frame_scale = 0.833333313465118408203125f;
        g_frame_time = 0.02000000141561031341552734375f;
        g_frame_time_sq = 0.00040000001899898052215576171875f;
        g_frame_time_cube = 0.0000079999999798019416630268096923828125f;
        frame_scale_fifths = 6;
        frame_delta_time = 0.0199999995529651641845703125f;
    }
}

LINKER_REMNANT("asm/remnants/text", func_002158E0);

extern int FastVectorToPackedChars(float *);
extern int truncate_float_to_s32_r(float) __asm__("func_001FA898");

/* Packs vector v into one RGBA word at *out (func_001F9F30 converts a
   float4 to bytes): its largest absolute component sets a scale n,
   rounded (func_001FA898) from max * 10000 / 63 and clamped to 1..255,
   which goes in w; x, y, z become v / (n / 10000) + 127. */
void pack_vector_xyz_with_shared_scale(float *v, int *out) {
    float buf[4];
    float a = FastAbsF(v[0]);
    float b = FastAbsF(v[1]);
    float c = FastAbsF(v[2]);
    float m;
    int n;
    float s;

    if (b < a) {
        b = a;
    }
    m = (c < b) ? b : c;
    n = truncate_float_to_s32_r(m * 10000.0f / 63.0f);
    n = (n < 0x100) ? n : 0xFF;
    if (n <= 0) {
        n = 1;
    }
    buf[3] = (float)n;
    s = 1.0f / (buf[3] * 0.0001f);
    buf[0] = v[0] * s + 127.0f;
    buf[1] = v[1] * s + 127.0f;
    buf[2] = v[2] * s + 127.0f;
    *out = FastVectorToPackedChars(buf);
}

typedef float FVec4[4] __attribute__((aligned(16)));
extern void FastVectorFromPackedChars(float *, int);

/* Unpack the packed colour *arg1 into a float vector, centre its
   channels on 127 and scale by alpha/10000 into arg0. The 127 vector is
   a 16-byte aligned (sceVu0FVECTOR-style) local cleared by its
   initializer (one por/sq), then filled. */
void load_display_text_resource_entry(float *arg0, int *arg1) {
    float v[4];
    FVec4 mid = { 0 };
    float scale;

    mid[0] = 127.0f;
    mid[1] = 127.0f;
    mid[2] = 127.0f;
    FastVectorFromPackedChars(v, *arg1);
    scale = v[3] * 0.0001f;
    FastVecSub(v, v, mid);
    FastVecScale(arg0, v, scale);
}


/* Rounds arg1 to arg0 decimal places: round(arg1 * 10^arg0) / 10^arg0.
   arg1 is updated in place throughout, so it stays in $f12 as retail
   has it. The nops after the two mtc1s are ps2eeas's
   (tools/ps2eeas_nops.py). */
float round_float_to_decimal_places(int arg0, float arg1) {
    int p = 1;
    float scale;

    if (arg0 > 0) {
        do {
            arg0--;
            p *= 10;
        } while (arg0 != 0);
    }
    scale = (float)p;
    arg1 += 1.0f / (scale + scale);
    arg1 *= scale;
    arg1 = (float)truncate_float_to_s32_r(arg1);
    return arg1 / scale;
}

extern float MobyGetFrameValue(char *);
extern float round_float_to_decimal_places(int, float);

/* Returns 1 if the moby's current anim frame lies inside the one-frame window starting at arg1. */
int IsFrameWithinWindow(char *arg0, float arg1) {
    float now = MobyGetFrameValue(arg0);
    float a = round_float_to_decimal_places(4, now - arg1);
    float b = round_float_to_decimal_places(4, *(float *)(arg0 + 0x58) * *(float *)(arg0 + 0x5C));

    if (arg1 <= now && a < b) {
        return 1;
    }
    return 0;
}

LINKER_REMNANT("asm/remnants/text", func_00215BA8);

/* Spherical-to-cartesian: x is the radius, y and z the two angles.
   func_001F9F90 is cos, func_001F9FA8 sin (named in the comments at
   func_00212xxx). Each product is spelled trig * r * trig so the two
   calls are issued before the multiplies, the way retail does. */
void BuildSphericalOffset(void *arg0, float r, float y, float z) {
    float *out = (float *)arg0;

    out[0] = FastCos(y) * r * FastCos(z);
    out[1] = FastSin(y) * r * FastCos(z);
    out[2] = FastSin(z) * r;
}

LINKER_REMNANT("asm/remnants/text", func_00215CA0);

typedef struct {
    float v[4];
} __attribute__((aligned(16))) PathPt;
typedef struct {
    int count;
    char pad[0xC];
    PathPt pts[1];        /* 0x10 */
} Path;
extern void lerp_vector(void *, void *, void *, float);
extern float vec_length_xy(void *);
extern float FastSubRots(float, float);
extern float FastAddRots(float, float);

/* Samples a path of points at arg t: lerps the position between point
   floor(t) and the next into pos and, unless flags bit 0 is set, writes
   the rotation interpolated between the two segments' directions (and
   the points' w as roll) into rot. Without wrap the last segment clamps. */
void SamplePathAt(Path *path, int wrap, void *pos, float *rot, int flags, float t) {
    PathPt a;
    PathPt b;
    PathPt c;
    PathPt d;
    int i;
    int j;
    int k;
    int clamp = 0;
    float f;
    float yaw;
    float pitch;
    float yaw2;
    float pitch2;
    float roll;
    float roll2;

    i = truncate_float_to_s32_r(t);
    if (!wrap && i >= path->count - 2) {
        i = path->count - 2;
        clamp = 1;
    }
    f = t - (float)i;
    if (clamp && f > 1.0f) {
        f = 1.0f;
    }
    j = i + 1;
    k = i + 2;
    if (k >= path->count) {
        j = j % path->count;
        k = k % path->count;
    }
    roll2 = 0.0f;
    qcopy(&a, &path->pts[i]);
    qcopy(&b, &path->pts[j]);
    lerp_vector(pos, &a, &b, f);
    if (flags & 1) {
        return;
    }
    FastVecSub(&d, &b, &a);
    yaw = fast_arc_tan(d.v[0], d.v[1]);
    pitch = fast_arc_tan(vec_length_xy(&d), d.v[2]);
    roll = a.v[3];
    if (clamp) {
        yaw2 = yaw;
        pitch2 = pitch;
        roll = 0.0f;
    } else {
        qcopy(&c, &path->pts[k]);
        FastVecSub(&d, &c, &b);
        yaw2 = fast_arc_tan(d.v[0], d.v[1]);
        pitch2 = fast_arc_tan(vec_length_xy(&d), d.v[2]);
        roll2 = b.v[3];
    }
    rot[0] = 0.0f;
    rot[1] = -FastAddRots(FastSubRots(pitch2, pitch) * f, pitch);
    rot[2] = FastAddRots(FastSubRots(yaw2, yaw) * f, yaw);
    rot[3] = -FastAddRots(FastSubRots(roll2, roll) * f, roll);
}

LINKER_REMNANT("asm/remnants/text", func_00215F20);

extern int msg_string(int);
extern void copy_text_to_shared_buffer(int);
extern int help_message_keepalive MACRO_ADDR;
extern int help_message_owner MACRO_ADDR;
extern int help_message_id MACRO_ADDR;

/* Shows help message arg1 (func_001FFE88 of its string) for requester
   arg0: 2 if arg0 already holds the slot, 1 if the slot was free and
   arg0 takes it, 0 if someone else holds it. The failure return goes
   last, which lets the scheduler put `li $2,1` before the last store. */
int try_set_help_message(int arg0, int arg1) {
    int cur = help_message_owner;

    if (cur == arg0) {
        if (arg1 != 0) {
            copy_text_to_shared_buffer(msg_string(arg1));
        }
        help_message_id = arg1;
        help_message_keepalive = 2;
        return 2;
    }
    if (cur == 0) {
        if (arg1 != 0) {
            copy_text_to_shared_buffer(msg_string(arg1));
        }
        help_message_owner = arg0;
        help_message_keepalive = 2;
        help_message_id = arg1;
        return 1;
    }
    return 0;
}

/* Sets a help message even when another one shows, and keeps it alive; returns 3 on success. */
int force_help_message(int arg0, int arg1) {
    int busy = try_set_help_message(arg0, arg1);
    if (busy != 0) {
        return busy;
    }
    if (arg1 != 0) {
        copy_text_to_shared_buffer(msg_string(arg1));
    }
    help_message_owner = arg0;
    help_message_keepalive = 2;
    help_message_id = arg1;
    return 3;
}

/* Returns gold bolts collected minus four per gold weapon bought, clamped to 0..40. */
int compute_clamped_count_difference(void) {
    int a = count_nonzero_entries_up_to_40();
    int b = count_nonzero_entries_up_to_10();
    int v = a - b * 4;
    if (v < 0) v = 0;
    return (v < 0x29) ? v : 0x28;
}

extern unsigned char gold_bolts_collected[];

/* Counts the gold bolts collected, capped at 40. */
int count_nonzero_entries_up_to_40(void) {
    int count = 0;
    int row;
    for (row = 0; row < 0x14; row++) {
        unsigned char *p = &gold_bolts_collected[row * 4];
        int k;
        for (k = 3; k >= 0; k--) {
            if (*p != 0) count = count + 1;
            p++;
        }
    }
    if (count < 0) count = 0;
    return (count < 0x29) ? count : 0x28;
}

extern unsigned char gold_weapon_purchased[];

/* Counts the gold weapons bought, capped at 10. */
int count_nonzero_entries_up_to_10(void) {
    int count = 0;
    int i;
    for (i = 0; i < 0x25; i++) {
        if (gold_weapon_purchased[i] != 0) count = count + 1;
    }
    if (count < 0) count = 0;
    return (count < 0xB) ? count : 0xA;
}

extern unsigned char gSkillPoints[];

/* Counts the skill points earned, capped at 30. */
int count_nonzero_entries_up_to_30(void) {
    int count = 0;
    int i;
    for (i = 0; i < 0x20; i++) {
        if (gSkillPoints[i] != 0) count = count + 1;
    }
    if (count < 0) count = 0;
    return (count < 0x1F) ? count : 0x1E;
}
