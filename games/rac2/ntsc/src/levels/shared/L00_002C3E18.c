#include "include_asm.h"
typedef int Vector __attribute__((mode(TI)));
typedef struct {
    unsigned char pad0[0x80]; float position[4];
    unsigned char pad1[8]; float heading;
    unsigned char pad2[0x64C - 0x9C]; float alternate_heading;
    unsigned char pad3[0xAA4 - 0x650]; void *selected_aa4; int ready_aa8;
    unsigned char pad4[12]; int value_ab8;
    unsigned char pad5[4]; float value_ac0; float score_ac4;
    unsigned char pad6[8]; void *selected_ad0;
    unsigned char pad7[4]; float score_ad8;
    unsigned char pad8[2]; short ready_ade;
    unsigned char pad9[0xB10 - 0xAE0]; void *selected_b10; int ready_b14;
    unsigned char pad10[0x2294 - 0xB18]; int mode, field_2298, state;
} HeroSelectionState;
extern HeroSelectionState D_00189E20;

extern unsigned char *D_L00_001B1EA8 __attribute__((sda));
extern float func_L00_002AE388(void *, int *, float, float, float, float);
extern float func_00282E48(void *, void *);
extern float func_00282E80(void *, void *);
extern int func_002ACBC8(void *, int);
extern int func_002761F8(void *, void *, int, void *, int);

int func_L00_002C3E18(void)
{
    unsigned char *node, *best;
    float closest;
    float first[4], second[4];
    int blocked[4];
    if (D_00189E20.state == 0xD) return D_00189E20.ready_aa8;
    {
        best = 0;
        closest = 100000000.0f;
        D_00189E20.ready_aa8 = 0;
        for (node = D_L00_001B1EA8; node; node = *(unsigned char **)(node + 0x28)) {
            char *data;
            float horizontal, vertical, heading, range, score;
            if (*(short *)(node + 0xAA) != 0x2F6) continue;
            if (*(unsigned short *)(node + 0x34) & 1) continue;
            if ((*(unsigned short *)(node + 0x34) & 0x1000) == 0) continue;
            data = *(char **)(node + 0x68);
            if (*(int *)data != 0) continue;
            if (*(float *)(data + 0x20) != 0.0f &&
                *(float *)(data + 0x20) < func_00282E48(D_00189E20.position, node + 0x10)) continue;
            horizontal = 0.5235988497734069824f;
            vertical = -1.0f;
            if (D_00189E20.mode == 1 || D_00189E20.mode == 0x1D)
                horizontal = vertical = 0.087266474962234497f;
            heading = D_00189E20.heading;
            if (D_00189E20.state == 0xF) heading = D_00189E20.alternate_heading;
            range = 30.0f;
            if (*(float *)(data + 0x20) != 0.0f) range = *(float *)(data + 0x20);
            score = func_L00_002AE388(node, blocked, heading, range, horizontal, vertical);
            if (blocked[0] != 0) continue;
            if (func_00282E80(D_00189E20.position, node + 0x10) < 4.0f) continue;
            if ((unsigned char)node[0x31] == 0) score += 10.0f;
            if (*(int *)(data + 0x1C) >= 0 && func_002ACBC8(D_00189E20.position, *(int *)(data + 0x1C))) continue;
            if (score < closest) { closest = score; best = node; }
        }
        if (best) {
            unsigned char *data;
            float value;
            D_00189E20.selected_aa4 = best;
            *(Vector *)first = *(Vector *)D_00189E20.position;
            first[2] += 0.5f;
            *(Vector *)second = *(Vector *)(best + 0x10);
            data = *(unsigned char **)(best + 0x68);
            D_00189E20.value_ab8 = *(int *)(data + 8);
            value = *(float *)(data + 4);
            D_00189E20.score_ac4 = closest;
            D_00189E20.value_ac0 = value;
            if (!func_002761F8(first, second, 2, 0, 0)) D_00189E20.ready_aa8 = 1;
        }
    }
    return D_00189E20.ready_aa8;
}
