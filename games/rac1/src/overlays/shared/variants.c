/* Functions of common level code added after the first split; stubs added by tools/overlay_variants.py, replaced by C
   as functions are matched. */
#include "common.h"
#include "include_asm.h"
#include "structs/hero.h"
#include "overlays/shared/hero_animation_slots.h"
extern Hero g_hero;

typedef struct {
    char pad[0x24];
    int v;
    char pad2[0x24];
} Rec4C;

extern Rec4C quartu_item_defs[];

/* Returns 0 or 0x54 in the early-out cases below, otherwise the v field of quartu_item_defs[hero_slot_item_id(0)]. */
int HelpL15_GetRecordValue(int a) {
    struct Hero *g = &g_hero;
    switch (g->vehicle_mode) {
        case 2:
            return 0;
        case 3:
            return 0;
    }
    {
        struct Hero *g2 = &g_hero;
        if (g2->health.hp == 1) {
            return 0x54;
        }
        if (a == 0) {
            return 0;
        }
        if (g2->item_anim_active == 0) {
            return 0;
        }
        if (g2->unk20AA == 0) {
            return 0;
        }
        if (g2->unk22C8 != 0) {
            return 0;
        }
    }
    {
        int i = hero_slot_item_id(0);
        return quartu_item_defs[i].v;
    }
}
