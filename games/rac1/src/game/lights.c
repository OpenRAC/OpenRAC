#include "common.h"
#include "callees.h"
#include "records.h"
#include "structs.h"

/*
 * lights.cpp in the original source; text 0x202260-0x202AA8.
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

extern char dir_lights[];
extern float gCamera_yaw;
extern char point_lights[];
extern char point_light_data[];
extern float FastAddRots(float, float);
extern float FastCos(float);
extern float FastSin(float);
extern float FastVecDist(void *, void *);
extern void CreatePointLight(int);
extern void RefreshPointLight(int);

/* Per-frame light update: the ambient colour (0.8 grey, -0.3) and a key
   light 0.8 rad behind the camera heading, tilted down; then every
   active point light (0x30-byte records at D_0019C4C0, positions from
   D_0019C2C0) that has moved more than 8 units is re-placed and either
   created (state 1 -> 2) or refreshed (state 2). */
void UpdateAllPointLights(void) {
    char *l = dir_lights;
    float ang;
    int i;

    *(float *)(l + 0x34C) = -0.3f;
    *(float *)(l + 0x340) = 0.8f;
    *(float *)(l + 0x344) = 0.8f;
    *(float *)(l + 0x348) = 0.8f;
    ang = FastAddRots(gCamera_yaw, -0.8f);
    *(float *)(l + 0x350) = FastCos(ang) * 0.866f;
    *(float *)(l + 0x354) = FastSin(ang) * 0.866f;
    *(float *)(l + 0x358) = -0.5f;
    *(int *)(l + 0x35C) = 0;
    for (i = 0; i < 8; i++) {
        char *src = point_lights + i * 0x20;
        char *dst = point_light_data + i * 0x30;

        if (*(int *)(dst + 0x10) != 0 && FastVecDist(src + 0x10, dst + 0x20) > 8.0f) {
            qcopy(dst + 0x20, src + 0x10);
            if (*(int *)(dst + 0x10) == 1) {
                CreatePointLight(i);
                *(int *)(dst + 0x10) = 2;
            } else if (*(int *)(dst + 0x10) == 2) {
                RefreshPointLight(i);
            }
        }
    }
}

typedef struct {
    float v[4];
} __attribute__((aligned(16))) LtVec;

typedef struct {
    short n0;
    short c0;
    short n2;
    short c2;
    short n1;
    short c1;
    short *buf;
    unsigned char pad[0x20];
} LtSlot;

typedef struct {
    unsigned char pad0[0x10];
    LtVec sphere;
} LtDef;

typedef struct {
    unsigned char pad[0x1E];
    unsigned short lights;
} LtObj20;

typedef struct {
    unsigned char pad[0x36];
    unsigned short lights;
    unsigned char pad38[8];
} LtObj40;

extern LtObj20 *ties_o __asm__("D_00161050") MACRO_ADDR;
extern LtObj20 *ties_end_o __asm__("D_00161054") MACRO_ADDR;
extern LtObj40 *tfrag_render_records_o __asm__("D_00160F8C") MACRO_ADDR;
extern int tfrag_render_record_count MACRO_ADDR;
extern LtObj20 *shrubs_o __asm__("D_001604D4") MACRO_ADDR;
extern LtObj20 *shrubs_end_o __asm__("D_001604D8") MACRO_ADDR;
extern int spheres_intersect(void *, void *);
extern void scale_vector(void *, void *, float);

/* CreatePointLight: lists which objects of the three lists the light's sphere touches and records the light's index in their packed nibble field. Adapted from Lombyte's project (GPL-3.0) for PAL: src/rendering/create_point_light.c, create_point_light. */
void CreatePointLight(int i) {
    LtSlot *slot;
    LtDef *def;
    short *p;
    short *end;
    LtVec sphere;
    LtObj20 *o;
    LtObj40 *m;
    int n;
    int n2;
    int n3;
    unsigned int v;

    slot = &((LtSlot *)point_light_data)[i];
    p = slot->buf;
    end = p + 0x200;
    def = &((LtDef *)point_lights)[i];
    qcopy(&sphere, &def->sphere);
    sphere.v[3] += 8.0f;
    if (p < end) {
        slot->n0 = 0;
        n = 0;
        for (o = ties_o; o != ties_end_o; o++, n++) {
            if (spheres_intersect(&sphere, o)) {
                *p++ = n;
                v = o->lights;
                if (v == 0xFFFF) {
                    o->lights = i | 0xFFF0;
                } else if ((v & 0xFFF0) == 0xFFF0) {
                    o->lights = (v & 0xFF0F) | (i << 4);
                } else if ((v & 0xFF00) == 0xFF00) {
                    o->lights = (v & 0xF0FF) | (i << 8);
                } else if ((v & 0xF000) == 0xF000) {
                    o->lights = (v & 0x0FFF) | (i << 12);
                } else {
                    p--;
                }
                if (p >= end) {
                    break;
                }
            }
        }
        slot->c0 = p - slot->buf;
        if (p < end) {
            slot->n1 = slot->c0;
            scale_vector(&sphere, &sphere, 1024.0f);
            m = tfrag_render_records_o;
            for (n2 = 0; n2 < tfrag_render_record_count; n2++, m++) {
                if (spheres_intersect(&sphere, m)) {
                    *p++ = n2;
                    v = m->lights;
                    if (v == 0xFFFF) {
                        m->lights = i | 0xFFF0;
                    } else if ((v & 0xFFF0) == 0xFFF0) {
                        m->lights = (v & 0xFF0F) | (i << 4);
                    } else if ((v & 0xFF00) == 0xFF00) {
                        m->lights = (v & 0xF0FF) | (i << 8);
                    } else if ((v & 0xF000) == 0xF000) {
                        m->lights = (v & 0x0FFF) | (i << 12);
                    } else {
                        p--;
                    }
                    if (p >= end) {
                        break;
                    }
                }
            }
            slot->c1 = (p - slot->buf) - slot->n1;
            if (p < end) {
                slot->n2 = p - slot->buf;
                n3 = 0;
                for (o = shrubs_o; o != shrubs_end_o; o++, n3++) {
                    if (spheres_intersect(&sphere, o)) {
                        *p++ = n3;
                        v = o->lights;
                        if (v == 0xFFFF) {
                            o->lights = i | 0xFFF0;
                        } else if ((v & 0xFFF0) == 0xFFF0) {
                            o->lights = (v & 0xFF0F) | (i << 4);
                        } else if ((v & 0xFF00) == 0xFF00) {
                            o->lights = (v & 0xF0FF) | (i << 8);
                        } else if ((v & 0xF000) == 0xF000) {
                            o->lights = (v & 0x0FFF) | (i << 12);
                        } else {
                            p--;
                        }
                        if (p >= end) {
                            break;
                        }
                    }
                }
                slot->c2 = (p - slot->buf) - slot->n2;
            }
        }
    }
}

extern void DetachPointLight(int);

/* RefreshPointLight */
void RefreshPointLight(int arg0) {
    DetachPointLight(arg0);
    CreatePointLight(arg0);
}
/* Retail aligns the next function to 16 bytes, and this function's .s
   stub carried one padding word to do it. Decompiling to C drops that
   padding, shifting every later function in the object by -4 and
   producing spurious `jal` diffs far from the cause -- so restore it
   explicitly. */
__asm__(".align 4");

struct LightRecord32C {
    u8 pad_0[0x1B];
    u8 unk1B;
    u8 pad_1C[0x2];
    u16 unk1E;
};

struct LightRecord64 {
    u8 pad_0[0x35];
    u8 unk35;
    u16 unk36;
};

struct LightRecord32A {
    u8 pad_0[0x1B];
    u8 unk1B;
    u8 pad_1C[0x2];
    u16 unk1E;
};

struct PointLightLinks {
    s16 unk0;
    s16 unk2;
    s16 unk4;
    s16 unk6;
    s16 unk8;
    s16 unkA;
    s32 unkC;
};

extern u8 *shrubs MACRO_ADDR;
extern u8 *tfrag_render_records MACRO_ADDR;
extern u8 *ties MACRO_ADDR;

extern u8 str_bad_point_light_tie[];
extern u8 str_bad_point_light_tfrag[];
extern u8 str_bad_point_light_shrub[];
extern s32 STUB_printf();

/* Remove one point light from three packed attachment lists. Each 16-bit
   reference stores up to four light IDs in nibbles; deleting one shifts
   the higher IDs down and marks the record free when all are 0xF. */
void DetachPointLight(s32 arg0) {
    s16 *firstLinks;
    s16 *thirdLinks;
    s16 *secondLinks;
    s32 thirdEnd;
    s32 firstEnd;
    s32 secondEnd;
    s32 thirdNibble;
    s32 secondNibble;
    s32 fourthNibble;
    s32 firstNibble;
    s32 secondLowNibble;
    s32 thirdLowNibble;
    s32 firstLowNibble;
    u32 thirdPacked;
    u32 firstPacked;
    u32 secondPacked;
    struct LightRecord32C *thirdRecord;
    struct LightRecord64 *secondRecord;
    struct LightRecord32A *firstRecord;
    struct PointLightLinks *links;

    firstNibble = arg0 & 0xFFFF;
    links = (arg0 * 0x30) + point_light_data;
    secondNibble = (arg0 * 0x10) & 0xFFFF;
    thirdNibble = (arg0 << 8) & 0xFFFF;
    firstLinks = links->unkC + (links->unk0 * 2);
    firstEnd = firstLinks + links->unk2;
    fourthNibble = (arg0 << 0xC) & 0xFFFF;
    if (firstLinks != firstEnd) {
        do {
            firstRecord = ties + (*firstLinks << 5);
            firstPacked = firstRecord->unk1E;
            firstLowNibble = firstPacked & 0xF;
            if (firstLowNibble == firstNibble) {
                firstPacked = (firstPacked >> 4) | 0xF000;
            } else if ((firstPacked & 0xF0) == secondNibble) {
                firstPacked = firstLowNibble | ((firstPacked >> 4) & 0xFF0) | 0xF000;
            } else if ((firstPacked & 0xF00) == thirdNibble) {
                firstPacked = (firstPacked & 0xFF) | ((firstPacked >> 4) & 0xF00) | 0xF000;
            } else if ((firstPacked & 0xF000) == fourthNibble) {
                firstPacked |= 0xF000;
            } else {
                STUB_printf(str_bad_point_light_tie);
            }
            firstLinks += 1;
            if (firstPacked == 0xFFFF) {
                firstRecord->unk1B = 1;
            }
            firstRecord->unk1E = firstPacked;
        } while (firstLinks != firstEnd);
    }
    secondLinks = links->unkC + (links->unk8 * 2);
    links->unk0 = 0;
    secondEnd = secondLinks + links->unkA;
    links->unk2 = 0;
    if (secondLinks != secondEnd) {
        do {
            secondRecord = tfrag_render_records + (*secondLinks << 6);
            secondPacked = secondRecord->unk36;
            secondLowNibble = secondPacked & 0xF;
            if (secondLowNibble == firstNibble) {
                secondPacked = (secondPacked >> 4) | 0xF000;
            } else if ((secondPacked & 0xF0) == secondNibble) {
                secondPacked = secondLowNibble | ((secondPacked >> 4) & 0xFF0) | 0xF000;
            } else if ((secondPacked & 0xF00) == thirdNibble) {
                secondPacked = (secondPacked & 0xFF) | ((secondPacked >> 4) & 0xF00) | 0xF000;
            } else if ((secondPacked & 0xF000) == fourthNibble) {
                secondPacked |= 0xF000;
            } else {
                STUB_printf(str_bad_point_light_tfrag);
            }
            secondLinks += 1;
            if (secondPacked == 0xFFFF) {
                secondRecord->unk35 = 1;
            }
            secondRecord->unk36 = secondPacked;
        } while (secondLinks != secondEnd);
    }
    thirdLinks = links->unkC + (links->unk4 * 2);
    links->unk8 = 0;
    thirdEnd = thirdLinks + links->unk6;
    links->unkA = 0;
    if (thirdLinks != thirdEnd) {
        do {
            thirdRecord = shrubs + (*thirdLinks << 5);
            thirdPacked = thirdRecord->unk1E;
            thirdLowNibble = thirdPacked & 0xF;
            if (thirdLowNibble == firstNibble) {
                thirdPacked = (thirdPacked >> 4) | 0xF000;
            } else if ((thirdPacked & 0xF0) == secondNibble) {
                thirdPacked = thirdLowNibble | ((thirdPacked >> 4) & 0xFF0) | 0xF000;
            } else if ((thirdPacked & 0xF00) == thirdNibble) {
                thirdPacked = (thirdPacked & 0xFF) | ((thirdPacked >> 4) & 0xF00) | 0xF000;
            } else if ((thirdPacked & 0xF000) == fourthNibble) {
                thirdPacked |= 0xF000;
            } else {
                STUB_printf(str_bad_point_light_shrub);
            }
            thirdLinks += 1;
            if (thirdPacked == 0xFFFF) {
                thirdRecord->unk1B = 1;
            }
            thirdRecord->unk1E = thirdPacked;
        } while (thirdLinks != thirdEnd);
    }
    links->unk4 = 0;
    links->unk6 = 0;
}
