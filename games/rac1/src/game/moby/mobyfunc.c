#include "common.h"
#include "structs/moby.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"
#include "game/render/draw.h"
#include "game/render/vuchain.h"
#include "data/executable.h"
#include "asm_functions/executable.h"
#include "game/math/fastfunc.h"
#include "game/audio/sound.h"
#include "game/system/stub.h"
#include "game/ui/pause.h"

/*
 * mobyfunc.cpp in the original source; text 0x20D348-0x20E6B8.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

extern char voice_pool[];

/* A moby instance: 0x100 bytes, in one array from moby_instance_end to
   moby_instance_perm_end. state 0xFE is a free slot and 0xFF marks the free tail
   (taking that slot moves the mark to the next one); a freed slot is not
   reused before frame unk38 (DeleteMoby sets it two frames ahead). */
typedef struct {
    char _pad00[0x11];
    unsigned char unk11; /* 0x11: copied to a moby's +0x7C */
    unsigned char unk12; /* 0x12: copied to a moby's +0x7E */
    char _pad13[0x1C - 0x13];
    int frames[1]; /* 0x1C */
} MobySeq;
extern char *moby_instance_end MACRO_ADDR;
extern char *moby_instance_perm_end MACRO_ADDR;
extern char *g_moby_vars MACRO_ADDR;
extern int world_update_time MACRO_ADDR;
extern int num_spawnable_mobys MACRO_ADDR;
extern char s_tried_to_create_moby[];
extern void InitMobyInstance(void *, int);

/* CreateMoby. The failure message ("... Time: %d, oClass: %d") takes
   oClass as its third argument, which is why retail keeps it in $a2. */
Moby *CreateMoby(int oClass) {
    Moby *m;

    for (m = (Moby *)moby_instance_end; m < (Moby *)moby_instance_perm_end; m++) {
        if (m->state >= 0xFE && (unsigned int)world_update_time >= m->spawn_frame) {
            if (m->state == 0xFF) {
                m[1].state = 0xFF;
            }
            InitMobyInstance(m, oClass);
            m->pvars = (u8 *)(g_moby_vars + (m - (Moby *)moby_instance_end) * 0x80);
            FastMemSet((char *)m->pvars, 0, 0x80);
            if (num_spawnable_mobys != 0) {
                num_spawnable_mobys--;
            }
            return m;
        }
    }
    STUB_printf(s_tried_to_create_moby, world_update_time, oClass);
    return 0;
}

extern unsigned char resident_class_slot_by_id[] NOT_SDA;

extern void *moby_class_resources[];
extern int resident_object_pool MACRO_ADDR;
extern void update_moby_animation_state_p(void *) __asm__("func_0020D6D0");

typedef struct {
    char _pad00[0x10];
    unsigned char nframes; /* 0x10 */
    signed char unk11;     /* 0x11 */
} MobyISeq;

typedef struct {
    char _pad00[6];
    unsigned char unk06; /* 0x06 */
    char _pad07[5];
    unsigned char unk0C; /* 0x0C */
    char _pad0D;
    unsigned char unk0E; /* 0x0E */
    unsigned char unk0F; /* 0x0F */
    int unk10;           /* 0x10 */
    char _pad14[0x10];
    float unk24;         /* 0x24 */
    char _pad28[0x18];
    int unk40;           /* 0x40 */
    unsigned short unk44; /* 0x44 */
    char _pad46[2];
    MobyISeq *seq;       /* 0x48 */
} MobyIClass;

typedef struct {
    char _pad00[0x21];
    unsigned char unk21;   /* 0x21 */
    unsigned char oClass;  /* 0x22 */
    unsigned char unk23;   /* 0x23 */
    MobyIClass *pClass;    /* 0x24 */
    char _pad28[4];
    float unk2C;           /* 0x2C */
    char _pad30[4];
    unsigned short flags;  /* 0x34 */
    unsigned short unk36;  /* 0x36 */
    unsigned long unk38;   /* 0x38 */
    char _pad40[0x18];
    float unk58;           /* 0x58 */
    float unk5C;           /* 0x5C */
    char _pad60[0x11];
    unsigned char unk71;   /* 0x71 */
    unsigned char unk72;   /* 0x72 */
    unsigned char unk73;   /* 0x73 */
    int unk74;             /* 0x74 */
    char _pad78[4];
    unsigned char unk7C;   /* 0x7C */
    unsigned char unk7D;   /* 0x7D */
    unsigned char unk7E;   /* 0x7E */
    unsigned char unk7F;   /* 0x7F */
    char _pad80[4];
    int unk84;             /* 0x84 */
    int unk88;             /* 0x88 */
    char _pad8C[4];
    int unk90;             /* 0x90 */
    int unk94;             /* 0x94 */
    char _pad98[8];
    unsigned char unkA0;   /* 0xA0 */
    unsigned char unkA1;   /* 0xA1 */
    unsigned char unkA2;   /* 0xA2 */
    unsigned char unkA3;   /* 0xA3 */
    unsigned char unkA4;   /* 0xA4 */
    char _padA5;
    short unkA6;           /* 0xA6 */
    int unkA8;             /* 0xA8 */
    int unkAC;             /* 0xAC */
    char _padB0[0xD];
    unsigned char unkBD;   /* 0xBD */
    char _padBE[0x42];
} MobyI;

/* InitMobyInstance: clears the 0x100-byte moby, fills its defaults (class
   byte from resident_class_slot_by_id[oClass], colours, its slot index from the moby
   array base resident_object_pool), flags a class with no moby_class_updates entry, then
   copies the class record moby_class_resources[class] (or marks the moby dead when
   there is none) and applies the animation-sequence rules after
   update_moby_animation_state. The moby is a real struct so its non-byte stores are "in
   struct" and resident_object_pool's load can move above them; the class load comes
   first, and the default stores are ordered so that sched1's
   register-pressure tie-break (stores that free a register go first)
   reproduces retail's store order. */
void InitMobyInstance(void *arg0, int oClass) {
    MobyI *m = (MobyI *)arg0;
    unsigned char c;
    int idx;
    MobyIClass *pClass;

    FastMemSet(m, 0, 0x100);
    c = resident_class_slot_by_id[oClass];
    m->unk23 = 0x80;
    m->oClass = c;
    m->unkA4 = 0xFF;
    m->unk21 = 0xFF;
    m->unk71 = 0xFF;
    m->unk72 = 0xFF;
    m->unkA6 = oClass;
    m->unk38 = 0x40404000000000L;
    m->unk36 = 0x7F80;
    idx = ((char *)m - (char *)resident_object_pool) >> 8;
    m->unkA8 = idx << 16;
    m->unkAC = idx;
    m->unk7E = 0;
    m->unk7C = 0xFF;
    m->unkA0 = 0x7F;
    m->unkA2 = 0x80;
    m->unk7D = 0xFF;
    m->unkA1 = 0x7F;
    m->unkA3 = 0x80;
    m->unk74 = moby_class_updates[m->oClass];
    if (m->unk74 == 0) {
        m->flags |= 2;
    }
    pClass = (MobyIClass *)moby_class_resources[m->oClass];
    if (pClass != 0) {
        MobyIClass *p;

        m->pClass = pClass;
        m->unk72 = pClass->unk0E;
        m->flags |= pClass->unk44;
        m->unk94 = pClass->unk10;
        m->unk2C = pClass->unk24;
        m->unk58 = 1.0f;
        m->unk5C = 1.0f;
        if (pClass->unk40 != 0) {
            m->flags |= 0x10;
            m->unk90 = pClass->unk40;
        }
        if (m->pClass->unk0F != 0) {
            m->unk7F = 0x18;
            m->flags |= 0x400;
            m->unk84 = 0;
            m->unk88 = 0;
            m->unkBD = 0;
        }
        if (m->pClass->unk06 != 0) {
            m->unk73 = 0x18;
        }
        if (m->pClass->seq == 0) {
            return;
        }
        update_moby_animation_state_p(m);
        if (m->pClass->seq->nframes >= 2) {
            m->flags &= 0xFFFD;
        }
        p = m->pClass;
        if (p->unk0C == 1) {
            if (p->seq->nframes < 2) {
                m->unk58 = 0.0f;
                if (p->seq->unk11 < 0) {
                    m->flags |= 0x40;
                }
            }
        }
        return;
    }
    m->pClass = 0;
    m->flags |= 5;
    m->unk94 = 0;
}

typedef struct {
    char _pad00[0x20];
    unsigned char state; /* 0x20 */
    char _pad21[0x38 - 0x21];
    long unk38; /* 0x38 */
} MobyDel;

/* DeleteMoby */
void DeleteMoby(MobyDel *m) {
    if ((char *)m < moby_instance_end) {
        m->state = 0xFD;
    } else {
        m->state = 0xFE;
    }
    m->unk38 = world_update_time + 2;
    update_moby_grids(m, 0x80807F7F);
}

/* Refresh a moby's animation frame pointers from its class's sequence
   table: seq 0xFF (none) points frameData into debug_font_buffer instead.
   Reaching the table as mc->seqs[i] (an array member behind a
   class-pointer local) is load-bearing: CSE shares mc + 0x48 across the
   three uses, the adds come out base-first, and local-alloc ties the
   last one to that base, which puts it in $a0 and overwrites the
   compare's copy of m->seq. With the value gone, reload_cse cannot turn
   the three index loads into `andi`s of the compare register. */
void update_moby_animation_state(Moby *m) {
    MobyClass *mc;
    if (m->seq != 0xFF) {
        mc = m->pclass;
        m->cur_frame_data = (void *)(((MobySeq *)mc->seqs[m->seq])->frames[m->frame]);
        m->anim_trigger_count = ((MobySeq *)mc->seqs[m->seq])->unk12;
        m->unk7C = ((MobySeq *)mc->seqs[m->seq])->unk11;
    } else {
        m->unk7C = m->seq;
        m->anim_trigger_count = 0;
        m->cur_frame_data = (void *)((int)&debug_font_buffer[m->frame << 11]);
    }
    m->prev_frame_data = (void *)(((MobySeq *)m->pclass->seqs[m->prev_seq])->frames[m->prev_frame]);
}

/* Drop this moby's sound handle (byte 0x7D) when its sound slot no longer
   belongs to it or plays another sound than the wanted one (byte 0x7C);
   with no handle, start the wanted one. Re-reading s[0x7D] inside the
   branch is what gives retail's two registers: the compare keeps the
   first load, the copy (live across blocks) becomes CSE's canonical
   register for the index and the call. */
void update_moby_voice(unsigned char *s) {
    if (s[0x7D] != 0xFF) {
        int id = s[0x7D];
        char *e = voice_pool + id * 0x70;
        if (*(unsigned char **)(e + 0x88) != s) {
            s[0x7D] = 0xFF;
        } else if (*(short *)(e + 0x7E) != s[0x7C]) {
            release_voice_slot(id);
            s[0x7D] = 0xFF;
        }
    } else if (s[0x7C] != 0xFF) {
        s[0x7D] = Moby_PlaySound(s[0x7C], 4, (int)s);
    }
}

LINKER_REMNANT("asm/remnants/text", func_0020D828);

/* Animation-frame value of a moby (the short at +4 of its frame data,
   1/16 units): the current frame's (the previous one's when no sequence
   is set), plus the blend factor at +0x54 while a new sequence or frame
   is starting, else blended between the current and previous frames by
   that factor. Written `frame > prevFrame` so the two bytes load in
   retail's order; prevFrameData is read before the first call. */
float compute_interpolated_record_value(Moby *m) {
    int fd;

    if (m->seq != 0xFF) {
        fd = (int)m->cur_frame_data;
    } else {
        fd = (int)m->prev_frame_data;
    }
    if (*(float *)((char *)m + 0x54) == 0.0f) {
        return IntToFloat(*(short *)(fd + 4)) * 0.0625f;
    }
    if (m->seq != m->prev_seq || m->frame > m->prev_frame) {
        return IntToFloat(*(short *)(fd + 4)) * 0.0625f + *(float *)((char *)m + 0x54);
    } else {
        int prev = (int)m->prev_frame_data;
        float a = IntToFloat(*(short *)(fd + 4)) * (1.0f - *(float *)((char *)m + 0x54));
        return (a + IntToFloat(*(short *)(prev + 4)) * *(float *)((char *)m + 0x54)) * 0.0625f;
    }
}
__asm__(".section .text\n\tnop\n");

LINKER_REMNANT("asm/remnants/text", func_0020D928);

/* Attach a fresh node to arg0's list at +0x64, seeded with 1.0f scales. */
/* AttachManipulator */
void AttachManipulator(char *arg0, int arg1, unsigned char *arg2) {
    unsigned char *e;

    if (arg2[1] != 0) {
        return;
    }
    arg2[0] = (char)arg1;
    arg2[1] = 1;
    *(float *)(arg2 + 0x1C) = 1.0f;
    *(float *)(arg2 + 0x20) = 1.0f;
    *(float *)(arg2 + 0x24) = 1.0f;
    *(float *)(arg2 + 0x28) = 1.0f;

    e = *(unsigned char **)(*(char **)(*(char **)(arg0 + 0x24) + 0x1C) + arg2[0] * 4 + 4);
    *(int *)(arg2 + 4) = e[*e + 4] * 0x40 + 0x70000000;

    *(int *)(arg2 + 8) = *(int *)(arg0 + 0x64);
    *(int *)(arg0 + 0x64) = (int)arg2;
}

/*
 * DetachManipulator: unlink `node` from the list at arg0+0x64 (next
 * pointer at +8), then clear it with FastMemSet(node, 0, 0x40).
 * The head is read twice, for the test and again for `cur`: the copy
 * that makes lands in the bnel's slot and leaves retail's two nops.
 */
void DetachManipulator(void *arg0, void *arg1) {
    char *base = (char *)arg0;
    char *node = (char *)arg1;
    char *cur;

    if (node == 0) {
        return;
    }
    if (*(char **)(base + 0x64) == node) {
        *(char **)(base + 0x64) = *(char **)(node + 8);
    } else {
        cur = *(char **)(base + 0x64);
        while (*(char **)(cur + 8) != 0 && *(char **)(cur + 8) != node) {
            cur = *(char **)(cur + 8);
        }
        if (*(char **)(cur + 8) == node) {
            *(char **)(cur + 8) = *(char **)(node + 8);
        }
    }
    FastMemSet(node, 0, 0x40);
}

extern int moby_render_slots[];

/* Returns the render slot holding moby v, or claims a free one; -1 if all 16 are taken. */
int find_or_allocate_moby_render_slot(int v) {
    int i = 0;
    int *p = moby_render_slots;
    do {
        if (*p == 0 || *p == v) {
            *p = v;
            return i;
        }
        i++;
        p++;
    } while (i < 0x10);
    return -1;
}

/* Clears render slots whose moby is dead or no longer in its expected state. */
void prune_moby_references(void) {
    int sentinel = 0xFF;
    char **p = (char **)moby_render_slots;
    int i = 0xF;
    do {
        char *e = *p;
        i--;
        if (e != 0) {
            if ((*(unsigned char *)(e + 0x20) & 0x80) != 0 || *(unsigned char *)(e + 0x52) != sentinel) {
                *p = 0;
            }
        }
        p++;
    } while (i >= 0);
}

/* copy_matrix_rotation is declared above with a single argument, for the
   extract_matrix_rotation_quaternion site; this one passes a source as well. */

/* Scale the single vertex block at arg2+0x30 by the object's 0x2C field
   in 1/1024 units, then run it through the two per-object transforms at
   arg0+0xC0 and arg0+0x10. 0x3A800000 is exactly 2^-10. */
void build_moby_bone_transform(char *arg0, int arg1, char *arg2) {
    char buf[0x40];
    int n;
    char *v;
    float s;

    s = *(float *)(arg0 + 0x2C) * 0.0009765625f;
    n = arg1;
    get_moby_joint_matrices(arg0, 1, &n, arg2);
    v = arg2 + 0x30;
    FastVecScale(v, v, s);
    copy_matrix_rotation(buf, arg0 + 0xC0);
    multiply_matrix(arg2, buf, arg2);
    FastVecAdd(v, v, arg0 + 0x10);
}

/* The many-vertex form of the same thing: arg1 blocks of 0x10 bytes
   starting at arg3, each scaled and transformed in place. */
void transform_scaled_vertex_batch(char *arg0, int arg1, void *arg2, char *arg3) {
    char *v = arg3;
    int n = arg1;
    float s;

    s = *(float *)(arg0 + 0x2C) * 0.0009765625f;
    get_moby_joint_matrices_batch(arg0, arg1, arg2, arg3);
    if (n > 0) {
        do {
            FastVecScale(v, v, s);
            n--;
            transform_vector_by_basis(v, v, arg0 + 0xC0);
            FastVecAdd(v, v, arg0 + 0x10);
            v += 0x10;
        } while (n != 0);
    }
}

LINKER_REMNANT("asm/remnants/text", func_0020DC38);

extern int moby_texture_tag_slot MACRO_ADDR;
extern int g_render_packet_cursor MACRO_ADDR;
extern int g_gs_texture_allocation_cursor MACRO_ADDR;

/* DmaMobyTextures: splices the texture uploads into the DMA chain with
   "next" tags (0x20000000). g_render_packet_cursor is the packet write pointer and
   moby_texture_tag_slot the tag slot DrawMobysSetup reserved. Every access goes
   back to the globals, because each store through them could alias. */
void DmaMobyTextures(void) {
    int *p = (int *)g_render_packet_cursor;

    g_render_packet_cursor += 0x10;
    ((int *)moby_texture_tag_slot)[0] = 0x20000000;
    ((int *)moby_texture_tag_slot)[1] = g_render_packet_cursor;
    ((int *)moby_texture_tag_slot)[2] = 0;
    ((int *)moby_texture_tag_slot)[3] = 0;
    if (draw_config[10] != 0 && draw_config[9] != 0) {
        upload_used_moby_textures(g_gs_texture_allocation_cursor);
        VU1_texFlush();
    }
    ((int *)g_render_packet_cursor)[0] = 0x20000000;
    ((int *)g_render_packet_cursor)[1] = moby_texture_tag_slot + 0x10;
    ((int *)g_render_packet_cursor)[2] = 0;
    ((int *)g_render_packet_cursor)[3] = 0;
    g_render_packet_cursor += 0x10;
    p[0] = 0x20000000;
    p[1] = g_render_packet_cursor;
    p[2] = 0;
    p[3] = 0;
}

extern int moby_class_rectify[];
extern short moby_gs_loc_stash[];

/* A class's joint records (the list at class+0x20): the joint's tag
   bytes, 0xFF-terminated, then the address of its first 0x40-byte GIF
   entry, whose sign bit marks the last joint. */
typedef struct {
    unsigned char tags[0xC];
    int entry;
} MobyJoint;

/* PatchMobyGifs: for each class in the negative-terminated moby_class_rectify
   list, walk its joints; each tag looks up two 14-bit values in
   moby_gs_loc_stash and ORs the non-zero ones into the low bits of the +0x30 /
   +0x40 words of successive GIF entries. The outer pointer is advanced
   through a `next` named at the top of the body (retail computes p + 1
   there), and the joint loop tests and breaks before its increment: in
   a do/while the increment is an expression GCSE hoists to the loop top. */
/* The class header's GIF table pointer (MobyClass.gifs at +0x20). The rest of the class is not read here. */
typedef struct {
    u8 pad_00[0x20];
    MobyJoint *gifs;
} MobyClassGifs;

/* Texture index range used when patching GIF packets. */
struct TexRemap {
    s16 lo;
    s16 hi;
};

/* Patches the texture addresses in the GIF packets of moby classes listed for remapping. */
void PatchMobyGifs(void) {
    s32 *id;
    s32 *next;
    MobyJoint *gif_entry;
    u8 *tex;
    u32 *gif;
    struct TexRemap *texture_remap;

    for (id = moby_class_rectify; *id >= 0; id = next) {
        MobyClassGifs *cls;

        next = id + 1;
        cls = ((MobyClassGifs **)moby_class_resources)[*id];
        gif_entry = cls->gifs;
        for (;; gif_entry++) {
            gif = (u32 *)(gif_entry->entry & 0x7FFFFFFF);
            tex = gif_entry->tags;
            if (*tex != 0xFF) {
                do {
                    texture_remap = &((struct TexRemap *)moby_gs_loc_stash)[*tex];
                    if (texture_remap->lo != 0) {
                        gif[12] = (gif[12] & 0xFFFFC000) | texture_remap->lo;
                    }
                    if (texture_remap->hi != 0) {
                        gif[16] = (gif[16] & 0xFFFFC000) | texture_remap->hi;
                    }
                    tex++;
                    gif += 16;
                } while (*tex != 0xFF);
            }
            if (gif_entry->entry < 0) {
                break;
            }
        }
    }
}

extern int g_hero_moby NOT_SDA;
extern float resident_packed_direction[] NOT_SDA;

/* Sets the shadow light direction from the light direction at the hero. */
void update_resident_packed_direction(void) {
    float buf[4];
    float a;

    get_moby_light_direction((void *)g_hero_moby, buf);
    a = fast_arc_tan(buf[0], buf[1]);
    resident_packed_direction[0] = FastCos(a) * 0.14f;
    resident_packed_direction[1] = FastSin(a) * 0.14f;
    resident_packed_direction[2] = -0.99f;
}

extern int graphics_setup_dma_chain_enabled MACRO_ADDR;
extern int graphics_setup_tag_slot MACRO_ADDR;
extern char moby_shadow_callback_label[];
extern char graphics_setup_command_stream[];

/* The same splice as DmaMobyTextures, around build_shadow_command_stream/submit_graphics_setup_command_stream,
   at the tag slot graphics_setup_tag_slot; with graphics_setup_dma_chain_enabled clear the slot becomes a
   plain "cnt" tag (0x10000000) instead. The write pointer goes through
   a temporary that is advanced in place (t += 0x10): that breaks CSE's
   p == t equivalence, which is what keeps retail's `daddu $16,$2,$0`
   copy. `p = g_render_packet_cursor; g_render_packet_cursor += 0x10;` is 4 bytes short. */
void configure_graphics_setup_dma_chain(void) {
    int *p;
    int t;

    if (graphics_setup_dma_chain_enabled == 0) {
        ((int *)graphics_setup_tag_slot)[0] = 0x10000000;
        ((int *)graphics_setup_tag_slot)[1] = 0;
        ((int *)graphics_setup_tag_slot)[2] = 0;
        ((int *)graphics_setup_tag_slot)[3] = 0;
        return;
    }
    t = g_render_packet_cursor;
    p = (int *)t;
    t += 0x10;
    g_render_packet_cursor = t;
    ((int *)graphics_setup_tag_slot)[0] = 0x20000000;
    ((int *)graphics_setup_tag_slot)[1] = g_render_packet_cursor;
    ((int *)graphics_setup_tag_slot)[2] = 0;
    ((int *)graphics_setup_tag_slot)[3] = 0;
    build_shadow_command_stream();
    submit_graphics_setup_command_stream(graphics_setup_command_stream);
    ((int *)g_render_packet_cursor)[0] = 0x20000000;
    ((int *)g_render_packet_cursor)[1] = graphics_setup_tag_slot + 0x10;
    ((int *)g_render_packet_cursor)[2] = 0;
    ((int *)g_render_packet_cursor)[3] = 0;
    g_render_packet_cursor += 0x10;
    p[0] = 0x20000000;
    p[1] = g_render_packet_cursor;
    p[2] = 0;
    p[3] = 0;
    noop_core_callback(moby_shadow_callback_label, 8);
}

extern char normal_lut[];
extern int render_packet_end MACRO_ADDR;
extern int vu1_chain_safe_end MACRO_ADDR;

/* ProcessMobyAnimData(void). Both globals are MACRO_ADDR: render_packet_end
   loads with retail's one-register lui $4 / lw $4, and vu1_chain_safe_end's
   load, scheduled into the jal delay slot, becomes $gp-relative. */
void ProcessMobyAnimData(void) {
    flush_cache(0);
    FastMemCopy((void *)0x70003800, normal_lut, 0x800);
    MobyAnimProc(render_packet_end, vu1_chain_safe_end);
}

/* InitMobyClassDists(void) */
void InitMobyClassDists(void) {
    FastMemSet((void *)0x70003A00, (void *)0x40000000, 0x380);
}

extern char moby_backup_buffer[];

/* StashMobyClassDists(void) */
void StashMobyClassDists(void) {
    FastMemCopy(moby_backup_buffer, (void *)0x70003A00, 0x380);
}

/* RestoreMobyClassDists(void) */
void RestoreMobyClassDists(void) {
    FastMemCopy((void *)0x70003A00, moby_backup_buffer, 0x380);
}

extern unsigned short moby_vu1_data_qwc NOT_SDA;

extern int font_state MACRO_ADDR;
extern char moby_draw_vu0_micro_program[];
extern int g_gs_texture_allocation_start MACRO_ADDR;
extern char moby_texture_label[];
extern int moby_anim_head MACRO_ADDR;
extern int vu1_chain_limit MACRO_ADDR;

/* DrawMobysSetup(void). What an older note here called a dead
   `&moby_texture_label` and an unexplained `li $5,1` are the arguments of the
   profiling marker noop_core_callback_secondary. The packet pointer goes through a
   local advanced in place, as in DrawShrubs. VU1_addGSregister's second
   parameter being `long` is what orders its arguments like retail. */
void DrawMobysSetup(void) {
    int p;

    VU1_addDataRef(moby_vu1_data, moby_vu1_data_qwc);
    font_state = 6;
    VU0_loadMicroProgram(moby_draw_vu0_micro_program);
    VU1_addGSregister(0x47, 0x5360B);
    p = g_render_packet_cursor;
    moby_texture_tag_slot = p;
    g_gs_texture_allocation_cursor = g_gs_texture_allocation_start;
    p += 0x10;
    g_render_packet_cursor = p;
    noop_core_callback_secondary(moby_texture_label, 1);
    graphics_setup_tag_slot = 0;
    vu1_chain_limit = vu1_chain_safe_end - 0x10000;
    moby_anim_head = render_packet_end;
}

/* DrawMobyList */
void DrawMobyList(int arg0, int arg1) {
    VU1_addGSregister(0x47, 0x5360B);
    flush_cache(0);
    RestoreMobyClassDists();
    moby_anim_head = moby_proc(arg0, moby_anim_head, arg1, 0);
    StashMobyClassDists();
    moby_anim_head -= 0x10;
}

extern void DmaMobyTextures(void);
extern void ProcessMobyAnimData(void);

extern char moby_draw_callback_label[];
extern char moby_skin_callback_label[];
extern int moby_word_fill_table_bytes MACRO_ADDR;

/* DrawMobysCleanUp. The empty profiling markers take (void *, int);
   DmaMobyTextures and apply_moby_word_fill_table take nothing. With the arguments
   right, moby_skin_callback_label is a plain array, not MACRO_ADDR. */
void DrawMobysCleanUp(void) {
    noop_core_callback_secondary(moby_draw_callback_label, 3);
    DmaMobyTextures();
    noop_core_callback(moby_draw_callback_label, 5);
    if (draw_config[10] != 0) {
        ProcessMobyAnimData();
        if (moby_word_fill_table_bytes != 0) {
            apply_moby_word_fill_table();
        }
    }
    noop_core_callback(moby_skin_callback_label, 3);
    if (draw_config[10] != 0 && graphics_setup_tag_slot != 0) {
        configure_graphics_setup_dma_chain();
    }
}

extern int moby_draw_enabled;
extern char vu_chain_overflow_message[];
extern void DrawMobysSetup(void);
extern void DrawMobysCleanUp(void);

/* DrawMobys */
void DrawMobys(void) {
    DrawMobysSetup();
    if (moby_draw_enabled != 0) {
        InitMobyClassDists();
        moby_anim_head = moby_proc(resident_object_pool, moby_anim_head, -1, 1);
        if (g_render_packet_cursor > vu1_chain_limit) {
            STUB_printf(vu_chain_overflow_message);
        }
    }
    DrawMobysCleanUp();
}

LINKER_REMNANT("asm/remnants/text", func_0020E330);

void pack_render_command_fields(u64 *command_words, u64 upper_field, u64 middle_field, u64 low_field, u64 tail_field);

/* Adapted from Lombyte's project (GPL-3.0) for PAL by OpenRAC's tools/port.py: src/rendering/commands/pack_render_command_fields.c, PackRenderCommandFields. */
void pack_render_command_fields(u64 *command_words, u64 upper_field, u64 middle_field, u64 low_field, u64 tail_field) {
    int command_word_index;
    if (middle_field) {
        upper_field <<= 32;
        low_field <<= 8;
        tail_field <<= 16;
    } else {
        upper_field <<= 32;
        low_field <<= 8;
        tail_field <<= 16;
    }
    {
        u64 packed_command;
        packed_command = upper_field | middle_field;
        packed_command |= low_field;
        packed_command |= tail_field;
        command_word_index = 7;
        command_words[command_word_index] = packed_command;
    }
}

ASM_FUNC("asm/handwritten/text", func_0020E360);

ASM_FUNC("asm/handwritten/text", func_0020E3D0);
