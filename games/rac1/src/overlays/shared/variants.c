/* Functions of common level code added after the first split; stubs added by tools/overlay_variants.py, replaced by C as functions are matched. */
#include "common.h"
#include "include_asm.h"

typedef struct {
    char pad[0x24];
    int v;
    char pad2[0x24];
} Rec4C;
extern unsigned char hero_globals_base[];
extern int hero_slot_item_id(int);
extern Rec4C quartu_item_defs[];

/* Returns 0 or 0x54 in the early-out cases below, otherwise the v field of D_L15_0017A140[func_L00_0020DB30(0)]. */
int HelpL15_GetRecordValue(int a) {
    char *g = (char *)hero_globals_base + 0xE1D;
    switch (*(unsigned char *)(g + 0x20A4)) {
    case 2:
        return 0;
    case 3:
        return 0;
    }
    {
        char *g2 = (char *)hero_globals_base + 0xE1D;
        if (*(int *)(g2 + 0x22A8) == 1) {
            return 0x54;
        }
        if (a == 0) {
            return 0;
        }
        if (*(unsigned char *)(g2 + 0x20A8) == 0) {
            return 0;
        }
        if (*(unsigned char *)(g2 + 0x20AA) == 0) {
            return 0;
        }
        if (*(short *)(g2 + 0x22C8) != 0) {
            return 0;
        }
    }
    {
        int i = hero_slot_item_id(0);
        return quartu_item_defs[i].v;
    }
}
