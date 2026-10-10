/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Native-only PAL 0020E3B8..0020E9F4, decoded from the complete retail
 * body. Maintains two hero pose manipulators and their blend transitions.
 * Explicit pointer fields preserve EE layout in hostgen; no PS2 match.
 */
typedef struct HeroPoseNode {
    unsigned char pad00[6], use_buffer[2];
    float weight;
    unsigned char pad0C[12];
    void* pose;
    unsigned char pad1C[4], frame[2], sequence[2];
    float blend, speed, rate;
    short transition;
    unsigned char special, releasing;
    int fading;
    void* buffers[2];
} HeroPoseNode;
typedef struct HeroPoseMoby {
    unsigned char pad00[0x50], frame[2], sequence[2];
    float blend;
} HeroPoseMoby;
typedef struct HeroPoseState {
    unsigned char pad00[0xA9C];
    int compare_categories;
    unsigned char padAA0[0x260];
    HeroPoseNode* nodes[2];
    void* extra;
    unsigned char padD0C[8];
    int allow_extra;
    unsigned char padD18[0x1368];
    HeroPoseMoby* hero;
    unsigned char pad2084[0x27], selector, pad20AC[3], force_fade;
} HeroPoseState;
extern unsigned char hp_state[] __asm__("D_0013E633");
extern float hp_step __asm__("D_0015EE60");
extern float hp_scale __asm__("D_0015EE64");
extern int hp_ids[] __asm__("D_L00_0015F780");
extern unsigned char hp_pose_index[] __asm__("D_L00_00197F40");
extern void* hp_poses[] __asm__("D_L00_00197680");
extern float hp_pose_data[] __asm__("D_L00_0017A780");
extern unsigned char hp_buffers[] __asm__("D_L00_0017CB80");
extern float hp_approach(float*, float, float) __asm__("func_00214D28");
extern HeroPoseNode* hp_create(void*, int) __asm__("func_L00_00250060");
extern void hp_remove(void*, HeroPoseNode**) __asm__("func_L00_00250120");
extern void hp_update(void*, void*) __asm__("func_L00_002501C8");
extern int hp_category(int) __asm__("func_L00_002056D0");
extern int hp_special(int, float*) __asm__("func_L00_0020DC68");
extern int hp_sequence(void*, int) __asm__("func_L00_0020DC50");
extern int hp_ticks(int) __asm__("func_001F9850");
extern void hp_trap(void) __asm__("func_001F9978");

void func_L00_0020E3B8(void) {
    HeroPoseState* g = (HeroPoseState*)(hp_state + 0xE1D);
    HeroPoseNode* n;
    int i, j, enabled, changed, special, category;
    float factor;
    for (i = 0; i < 2; ++i) {
        n = g->nodes[i];
        if (n && n->fading) {
            if (hp_approach(&n->weight, 0.0f, hp_step * 0.1f) == 0.0f)
                hp_remove(g->hero, &g->nodes[i]);
        } else {
            enabled = g->selector && (i == 0 || g->selector == 2);
            if (g->force_fade) {
                enabled = 0;
                if (n) n->fading = 1;
            }
            if (enabled) {
                if (!n) {
                    n = hp_create(g->hero, hp_ids[i]);
                    g->nodes[i] = n;
                    /* Retail immediately dereferences the allocation; do not
                     * silently skip a failed required manipulator on PC. */
                    if (!n) { hp_trap(); return; }
                    n->weight = 0.0f;
                    n->pose = hp_poses[hp_pose_index[i + 1]];
                }
            } else {
                if (!n) continue;
                if (hp_approach(&n->weight, 0.0f, hp_step * 0.1f) == 0.0f)
                    hp_remove(g->hero, &g->nodes[i]);
            }
        }
        n = g->nodes[i];
        if (!n) continue;
        changed = g->hero->sequence[0] != g->hero->sequence[1];
        if (g->extra && !g->allow_extra)
            hp_approach(&n->weight, 0.0f, hp_step * 0.1f);
        else if (!n->fading)
            hp_approach(&n->weight, 1.0f, hp_step * 0.1f);
        n->transition = 0;
        if (changed && g->compare_categories) {
            category = hp_category(n->sequence[1]);
            if (category != hp_category(g->hero->sequence[1]))
                n->transition = n->sequence[0] != n->sequence[1];
        }
        special = hp_special(g->hero->sequence[1], &factor);
        if (special || n->special) {
            if (factor != 0.0f) {
                hp_pose_data[0x5E4 / 4] = factor;
                hp_pose_data[0x694 / 4] = factor;
                hp_pose_data[0x624 / 4] = hp_scale * 0.05f;
                hp_pose_data[0x628 / 4] = hp_scale * 0.3f;
                hp_pose_data[0x6D4 / 4] = hp_scale * 0.05f;
                hp_pose_data[0x6D8 / 4] = hp_scale * 0.3f;
            }
            if (special) {
                if (!n->special || n->sequence[1] != 0) {
                    if (n->sequence[0] == n->sequence[1] || n->blend > 0.5f) {
                        n->sequence[0] = n->sequence[1];
                        n->frame[0] = n->frame[1];
                    }
                    n->blend = 0.0f;
                    n->sequence[1] = n->frame[1] = 0;
                    n->speed = 1.0f;
                    n->rate = 1.0f / (float)hp_ticks(15);
                    n->special = 1;
                    n->releasing = 0;
                }
                n->speed = 1.0f;
            } else if (!n->releasing) {
                n->sequence[1] = (unsigned char)hp_sequence(g->hero, g->hero->sequence[1]);
                n->frame[1] = 0;
                n->releasing = 1;
                n->blend = 0.0f;
                n->speed = 0.0f;
            } else {
                n->blend += hp_step * 0.1f;
                if (n->blend > 1.0f) n->blend = 1.0f;
                if (n->blend == 1.0f) n->special = 0;
            }
        } else {
            if (!n->transition) {
                for (j = 0; j < 2; ++j) {
                    if (!changed || j == 1)
                        n->sequence[j] = (unsigned char)hp_sequence(g->hero, g->hero->sequence[j]);
                }
            }
            if (n->transition) {
                n->blend += hp_step * 0.15f;
                if (n->blend >= 1.0f) {
                    n->sequence[0] = n->sequence[1];
                    n->frame[0] = n->frame[1];
                    n->transition = 0;
                }
            } else {
                if (!changed) n->frame[0] = g->hero->frame[0];
                n->frame[1] = g->hero->frame[1];
                n->blend = g->hero->blend;
            }
        }
        for (j = 0; j < 2; ++j) {
            n->use_buffer[j] = n->sequence[j] < 0x17;
            if (n->use_buffer[j]) n->buffers[j] = hp_buffers + i * 0x800 + j * 0x400;
        }
        hp_update(g->hero, n);
    }
}
