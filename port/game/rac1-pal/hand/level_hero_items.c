/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Native-only HeroItemsCreate, PAL 0020F118..0020F750. Reviewed against
 * the full retail body and nonmatching/shared/func_L00_0020F118.c.
 * Explicit pointer fields retain EE layout in hostgen; no PS2 match claimed.
 */
typedef struct HeroItemMoby {
    unsigned char pad00[0x24];
    unsigned char* model;
    unsigned char pad28[9], draw;
    short distance;
    unsigned short flags;
    unsigned char pad36[2];
    unsigned int lighting[2];
    unsigned char pad40[0x33], joints;
    int auxiliary;
} HeroItemMoby;

typedef struct HeroCreateSlot {
    HeroItemMoby *primary, *secondary;
    unsigned char pad08[8];
    int flags, available;
    unsigned char pad18[12];
    int status, item;
    unsigned char pad2C[0x24];
} HeroCreateSlot;

typedef struct HeroItems {
    unsigned char pad00[0x1090];
    HeroCreateSlot slots[6];
    unsigned char pad1270[0x680];
    unsigned char scratch[3][0xB0];
    unsigned char pad1B00[0x580];
    HeroItemMoby* hero;
    unsigned char pad2084[0x27], selector;
    unsigned char pad20AC[12];
    int state;
    unsigned char pad20BC[0x18];
    int primary_override, pad20D8, optional_override, back_override;
    unsigned char pad20E4[0x1F4];
    short hidden;
} HeroItems;

typedef struct HeroItemDefinition {
    unsigned char pad00[8];
    int slot;
    unsigned char pad0C[4];
    int primary, secondary;
    unsigned char selector, pad19[0x33];
} HeroItemDefinition;

extern unsigned char hero_items_state[] __asm__("D_0013E633");
extern unsigned char hero_items_options[] __asm__("D_0015EE88");
extern unsigned char hero_items_saved[] __asm__("D_0014171B");
extern unsigned char hero_items_owned[] __asm__("D_0013D5E9");
extern int hero_items_clock __asm__("D_L00_0015F4F8");
extern HeroItemDefinition hero_items_definitions[] __asm__("D_L00_00179BC0");
extern void hero_items_request(int, int) __asm__("func_00205270");
extern void hero_items_clear(void*, int, int) __asm__("func_001F99B0");
extern HeroItemMoby* hero_items_spawn(int) __asm__("func_0020D348");
extern void hero_items_animate(void*, int, int, int) __asm__("func_00213DE0");
extern void* hero_items_back_part(int) __asm__("func_L00_002B5428");

static void hero_item_init(HeroItemMoby* m, HeroItemMoby* hero, int check_joints) {
    m->distance = 0x20;
    m->draw = 1;
    m->lighting[0] = hero->lighting[0];
    m->lighting[1] = hero->lighting[1];
    if (check_joints && m->model[6] != 0) {
        m->joints = 0x18;
    }
}

void func_L00_0020F118(void) {
    HeroItems* g = (HeroItems*)(hero_items_state + 0xE1D);
    HeroCreateSlot* slot;
    HeroItemMoby* m;
    int item, which, i;

    if (!g->slots[0].primary && g->slots[0].available < hero_items_clock
        && (g->state == 0 || g->state == 0x24)) {
        item = g->primary_override;
        if (item == 0) {
            item = *(int*)(hero_items_saved + 0x45);
            if (*(int*)(hero_items_options + 8) != 0 || item == 0) {
                item = 8;
            }
        }
        hero_items_request(hero_items_definitions[item].primary, -1);
        for (i = 0; i < 3; ++i) {
            hero_items_clear(g->scratch[i], 0, 0xB0);
            *(short*)(g->scratch[i] + 0xA0) = -1;
        }
        g->slots[0].item = item;
        which = hero_items_definitions[item].slot;
        slot = &g->slots[which];
        m = hero_items_spawn(hero_items_definitions[slot->item].primary);
        if (m) {
            hero_item_init(m, g->hero, 1);
            slot->primary = m;
            slot->status = 2;
            if (item == 8) {
                slot->flags = 0x80;
                hero_items_animate(m, 1, 0, 1);
            } else {
                slot->flags = 0x20;
            }
            g->selector = hero_items_definitions[g->slots[which].item].selector;
        }
    }
    if (!g->slots[3].primary) {
        item = g->back_override;
        if (item == 0) {
            item = *(int*)(hero_items_saved + 0x51);
            if (item == 0) {
                item = *(int*)(hero_items_options + 0xC) != 0 ? 3 : 2;
            }
        }
        g->slots[3].item = item;
        m = hero_items_spawn(hero_items_definitions[item].primary);
        if (m) {
            hero_item_init(m, g->hero, 1);
            g->slots[3].status = 2;
            g->slots[3].primary = m;
            if (g->hidden != 0) {
                m->flags |= 0x41;
            }
            if (g->hidden == 0 && g->slots[3].item == 3) {
                hero_items_back_part(0);
                hero_items_back_part(1);
            }
        }
    }
    if (!g->slots[3].secondary) {
        m = hero_items_spawn(hero_items_definitions[1].primary);
        if (m) {
            hero_item_init(m, g->hero, 1);
            g->slots[3].status = 2;
            g->slots[3].secondary = m;
            if (g->hidden != 0) {
                m->flags |= 0x41;
            }
        }
    }
    if (!g->slots[2].primary) {
        item = *(int*)(hero_items_saved + 0x4D);
        if (item != 0 || g->optional_override != 0) {
            g->slots[2].item = g->optional_override != 0 ? g->optional_override : item;
            m = hero_items_spawn(hero_items_definitions[g->slots[2].item].primary);
            if (m) {
                hero_item_init(m, g->hero, 1);
                m->flags |= 0x802;
                m->auxiliary = 0;
                g->slots[2].status = 2;
                g->slots[2].primary = m;
            }
        }
    }
    if (!g->slots[1].primary) {
        item = *(int*)(hero_items_saved + 0x49);
        if (item != 0) {
            g->slots[1].item = item;
            m = hero_items_spawn(hero_items_definitions[item].primary);
            if (m) {
                hero_item_init(m, g->hero, 1);
                g->slots[1].primary = m;
                g->slots[1].status = 2;
            }
            /* Retail attempts the secondary even if the primary allocation fails. */
            item = *(int*)(hero_items_saved + 0x49);
            m = hero_items_spawn(hero_items_definitions[item].secondary);
            if (m) {
                hero_item_init(m, g->hero, 1);
                g->slots[1].secondary = m;
                g->slots[1].status = 2;
            }
        }
    }
    if (!g->slots[4].primary && hero_items_owned[0] != 0) {
        m = hero_items_spawn(hero_items_definitions[33].primary);
        g->slots[4].item = 0x21;
        if (m) {
            hero_item_init(m, g->hero, 1);
            g->slots[4].primary = m;
            g->slots[4].status = 2;
        }
    }
    if (!g->slots[4].secondary && hero_items_owned[1] != 0) {
        m = hero_items_spawn(hero_items_definitions[34].primary);
        g->slots[4].item = 0x22;
        if (m) {
            hero_item_init(m, g->hero, 0);
            g->slots[4].status = 2;
            g->slots[4].secondary = m;
        }
    }
    if (!g->slots[5].primary && hero_items_owned[2] != 0) {
        m = hero_items_spawn(hero_items_definitions[35].primary);
        g->slots[5].item = 0x23;
        if (m) {
            hero_item_init(m, g->hero, 0);
            g->slots[5].status = 2;
            g->slots[5].primary = m;
        }
    }
}
