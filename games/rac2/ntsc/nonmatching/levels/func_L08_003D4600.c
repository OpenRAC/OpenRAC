/* Retained complete C reconstruction; not linked or counted.
 * Profile: -O2 -G0 -ffunction-sections
 * 144/144 bytes; 0/3 placements in us, us2 and pal.
 * Residual: The null-guard delay slot and inherited header-store scheduling still differ; the variant state has its owner pointer at offset zero.
 * Reopen: Independent inherited-field or copy-destination lifetime evidence beyond the tied typed header snapshots.
 * Trial-only source markers and profiles are restored after checking.
 */

#include "common.h"
typedef int SpawnVector __attribute__((mode(TI)));
typedef struct SpawnObject SpawnObject;
typedef struct { u8 unknown00[0x24]; float scale; } BlueprintPrefix;
typedef struct { SpawnObject *owner; } AttachedState;
/* Partial object view; all gaps and offsets are measured accesses, not a
 * complete allocation-size claim. */
struct SpawnObject {
    u8 unknown00[0x10];
    SpawnVector position;
    u8 mode;
    u8 unknown21[3];
    BlueprintPrefix *blueprint;
    u8 unknown28[4];
    float scale;
    u8 intensity;
    u8 enabled;
    u16 colour;
    u16 flags;
    u8 unknown36[0x32];
    AttachedState *state;
    u8 unknown6C[0x84];
    SpawnVector orientation;
};
extern SpawnObject *func_0029EFD0(s32);
extern void func_002A0F48(void *);

/* Inherits colour and orientation, while installing a separate position. */
SpawnObject *func_L08_003D4600(SpawnObject *owner, SpawnVector *position)
{
    SpawnObject *object = func_0029EFD0(0x9A1);
    if (object != 0) {
        SpawnVector *destination = &object->position;
        object->mode = 0;
        object->intensity = owner->intensity;
        object->enabled = 1;
        object->colour = owner->colour;
        object->state->owner = owner;
        qcopy(destination, position);
        qcopy(&object->orientation, &owner->orientation);
        func_002A0F48(object);
    }
    return object;
}
