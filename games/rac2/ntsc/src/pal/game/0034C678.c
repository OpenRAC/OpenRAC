#include "include_asm.h"

typedef struct Rac2Native_79744baad5ad7f65_ScalarChange48Owner {
    unsigned int field_00;
    int value_04;
    unsigned char gap_08[0x3ec];
    int counter_3f4;
    int counter_3f8;
    int previous_3fc;
} Rac2Native_79744baad5ad7f65_ScalarChange48Owner;
extern unsigned char D_0018C0D4[];

/* Replace the value at offset 0x04 and, when it changes while the bound debug byte is zero, retain the previous value and set the two counters to 180 and 300; original game role is unknown. */
void func_PAL_0034C678(Rac2Native_79744baad5ad7f65_ScalarChange48Owner *owner, int value)
{
    int previous = owner->value_04;
    if (previous != value && D_0018C0D4[0] == 0) {
        owner->previous_3fc = previous;
        owner->counter_3f8 = 150;
        owner->counter_3f4 = 250;
    }
    owner->value_04 = value;
}

