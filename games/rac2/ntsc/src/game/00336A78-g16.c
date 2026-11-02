#include "include_asm.h"

/* func_00336F20's two colour tables are 4 and 2 words at $gp (0x001ADAF8, 0x001ADB08 in the
   boot; D_001ADB10 follows them), hence a unit of its own with -G16 and the assembler's -G16. */

extern unsigned char D_00139568[];
typedef struct {
    int enabled;
    unsigned char level;
    unsigned char pad05[0x67];
    int cost;
    unsigned char pad70[0x70];
} ItemRow;
extern ItemRow D_00239BA0[];
extern unsigned int D_001ADAF8[4] __attribute__((sda));
extern unsigned int D_001ADB08[2] __attribute__((sda));
extern int func_00287F60(int);
extern int func_00288D58(void);

/* Items 12, 14, 17, 18 and 53, which use the two-entry colour table. */
static __inline__ int itemInShortSet(int item)
{
    int found = 0;
    switch (item) { case 12: case 14: case 17: case 18: case 53: found = 1; }
    return found;
}

static __inline__ int itemCountReached(int item)
{
    if (D_00239BA0[D_00139568[item]].enabled) {
        int count = func_00287F60(item);
        if (count >= 2 || (count == 1 && itemInShortSet(item))) return 1;
    }
    return 0;
}

static __inline__ int itemLevelReached(int item)
{
    if (D_00239BA0[D_00139568[item]].enabled) {
        if (D_00239BA0[D_00139568[item]].level == 2
            || (D_00239BA0[D_00139568[item]].level == 1 && itemInShortSet(item))) return 1;
    }
    return 0;
}

/* Picks the colours for an item: *first and *second default to 0x55F0C070; *value is the
   colour of the item's level (item 16 from level 2 on uses the table from its start). */
void func_00336F20(int item, unsigned int *first, unsigned int *second, unsigned int *value)
{
    unsigned int colour;

    *first = 0x55F0C070;
    *second = 0x55F0C070;
    if (itemInShortSet(item)) {
        int index = D_00139568[item];
        colour = D_001ADB08[D_00239BA0[index].level];
    } else {
        int index = D_00139568[item];
        colour = D_001ADAF8[D_00239BA0[index].level];
    }
    *value = colour;
    if (item == 16) {
        if (D_00239BA0[D_00139568[16]].level >= 2)
            *value = D_001ADAF8[D_00239BA0[D_00139568[16]].level - 2];
    }
    if (D_00239BA0[D_00139568[item]].enabled && func_00287F60(item) > 0) {
        if (func_00288D58() && itemCountReached(item)) {
            if (itemLevelReached(item)) {
                *first = 0x60664088;
                *second = 0x606600D8;
            }
        } else {
            ItemRow *row = &D_00239BA0[D_00139568[item]];
            if (row->level == 0 && row->cost > 0) {
                *first = 0x6049C1FF;
                *second = 0x60001EFF;
            }
        }
    }
}
