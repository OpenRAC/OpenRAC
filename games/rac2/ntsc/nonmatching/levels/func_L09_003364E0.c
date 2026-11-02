/* GPT-6.1 Sol high, v0711, pinned upstream compiler, USA v1.01.
 * Best: 280/280 bytes, 0/5 placements; default -O2 -G0 profile.
 * Loading/storing class_id before scale leaves two final stores misplaced,
 * instead of the previous four-word scheduling disagreement. The object,
 * value and parameter still occupy S2/S0/S3 instead of retail S3/S2/S0.
 * Class-id snapshot, early return, block-scoped header values and -g did not
 * repair allocation. Reversed header stores put parameter in S0 but reverse
 * retail's first two header stores and leave object/value allocation wrong.
 * Reopen with actual allocator/lifetime evidence. Required types are below.
 * No linked build claimed.
 */
#include "common.h"

typedef union {
    u32 words[4];
    u8 bytes[16];
} ParticleHeader;

typedef struct {
    char *object;
    int index;
    int class_id;
    float scale;
} ParticleContext;

/* B9038 allocates at a 64-byte stride and clears all four quadwords. */
typedef struct {
    ParticleHeader header;
    float position[4];
    float previous_position[4];
    ParticleContext context;
} __attribute__((aligned(16))) ParticleSlot;

extern u8 *D_L09_001FF65C;
extern void *func_002B9030(int);
extern int func_00283CF0(float);
extern void func_0029FCC0(char *, int, char *);

char *func_L09_003364E0(char *object, int index, u32 value, int parameter,
                      float first_scale, float second_scale)
{
    ParticleSlot *slot = func_002B9030(0x37);
    float *position;

    if (slot != 0) {
        slot->header.words[1] = value;
        slot->header.words[3] = parameter;
        slot->header.bytes[9] = (u8)(func_00283CF0(0.4f) + 0x20);
        slot->header.bytes[3] = 0x48;
        slot->header.bytes[1] = 3;
        slot->header.bytes[2] = D_L09_001FF65C[0];
        if (index > 0) {
            position = slot->position;
            func_0029FCC0(object, index, (char *)position);
        } else {
            qcopy(slot->position, object + 0x10);
        }
        qcopy(slot->previous_position, slot->position);
        slot->previous_position[3] = second_scale;
        slot->position[3] = first_scale;
        {
        ParticleContext *context = &slot->context;
        context->object = object;
        context->class_id = *(short *)(object + 0xAA);
        context->scale = first_scale;
        context->index = index;
        }
    }
    return (char *)slot;
}
