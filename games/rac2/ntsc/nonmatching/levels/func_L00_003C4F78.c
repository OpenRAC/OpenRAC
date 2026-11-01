/* GPT-6.1 Sol high, v0923, 2026-10-29; not linked.
 * Best check: USA v1.01, -O2 -G8 -ffunction-sections, 912/916 bytes,
 * 0/27 places. Thirteen bounded check.py trials; declarations are included below.
 * Recomputing the history-head argument after the clamp restores the outer/last
 * counters to s5/s4. This draft uses split addresses for the history table and
 * MACRO_ADDR for the axis/count tables. Measured state/particle fields restore most
 * effect-field scheduling. A separate endpoint index preserves subtract-before-
 * shift, matching retail rather than folding it to a -16 byte displacement.
 * Residual: inner-copy pointer registers s1/s2 and row-address association,
 * i+1 temporary, first effect's context/scale load order, texture-table address
 * materialization (one instruction short), and final context-pointer register.
 * Explicit loop pointers, flat/decomposed row arithmetic and grouped TI copies
 * did not improve it; struct/union point and array/row-struct variants tie.
 * Reopen with independently evidenced row/texture pointer lifetimes. Both
 * allocations reuse the endpoint; all three draw-call stack tails are retained.
 */
#include "common.h"


typedef struct { float f[4]; } __attribute__((aligned(16))) UiHistoryPoint;
extern int D_L00_001B2940[] MACRO_ADDR;
extern UiHistoryPoint D_L00_0023FB80[][20];
extern UiHistoryPoint D_L00_001B2900[] MACRO_ADDR;
extern float D_L00_001AFF4C __attribute__((sda));
extern int D_L00_001AFF50 __attribute__((sda));
extern int D_L00_001AFF54 __attribute__((sda));
extern int D_L00_001AFF58 __attribute__((sda));
extern float D_L00_001AFF5C __attribute__((sda));
extern float D_L00_001AFF60 __attribute__((sda));
extern char D_L00_001FE940[] MACRO_ADDR;
extern void func_00282CC0(void *, void *, void *);
/* The second integer-bank input is forwarded as the axis vector pointer. */
extern void func_002AB668(void *, void *, float);
extern void func_002ACDE8(void *, void *, void *);
extern void func_002ACB48(float *, float);
extern float func_00282DF0(void *);
extern void func_00282D30(void *, void *, float);
typedef struct {
    char prefix[8];
    int flags;
    char pad[0x24];
    UiHistoryPoint origin;
} UiTrailState;
typedef struct {
    unsigned char prefix[2];
    unsigned char texture;
    unsigned char flags;
    int color;
    int unused;
    float size;
} UiTrailParticle;
extern UiTrailParticle *func_002C36D0(void *, int, int, int);
extern void func_L00_003C5310(void *, int, int, int, int, int, int, int,
                              int, int, int, float, float, float);

void func_L00_003C4F78(char *owner)
{
    UiHistoryPoint vertices[20], origin, change;
    UiTrailState *state = *(UiTrailState **)(owner + 0x68);
    int i;
    qcopy(&origin, &state->origin);
    for (i = 0; i < 4; i++) {
        int last, j;
        UiHistoryPoint *head, *tip;
        float length, limit;
        UiTrailParticle *effect;
        D_L00_001B2940[i]++;
        if (D_L00_001B2940[i] >= 20)
            D_L00_001B2940[i] = 20;
        last = D_L00_001B2940[i] - 1;
        for (j = last; j > 0; j--) {
            int index = last - j;
            qcopy(&D_L00_0023FB80[i][j], &D_L00_0023FB80[i][j - 1]);
            func_00282CC0(&vertices[index], &origin, &D_L00_0023FB80[i][j]);
            vertices[index].f[3] = 0.05f;
        }
        head = &D_L00_0023FB80[i][0];
        func_002AB668(&change, &D_L00_001B2900[i], 0.10471976548433304f);
        func_002ACDE8(head, head, &change);
        func_002ACB48(head->f, 0.025f);
        length = func_00282DF0(head);
        limit = D_L00_001AFF4C * 1.1f;
        if (length > limit)
            func_00282D30(head, head, limit / length);
        else {
            limit = D_L00_001AFF4C * 0.9f;
            if (length < limit)
                func_00282D30(head, head, limit / length);
        }
        func_00282CC0(&vertices[last], &origin, &D_L00_0023FB80[i][0]);
        vertices[last].f[3] = 0.05f;
        {
            int endpoint = D_L00_001B2940[i] - 1;
            tip = vertices + endpoint;
        }
        effect = func_002C36D0(tip, 2, 0, 1);
        if (effect != 0) {
            effect->color = D_L00_001AFF50 | state->flags;
            effect->size = D_L00_001AFF5C * 210000.0f;
            effect->texture = (*(char **)(D_L00_001FE940 + 0xF8))[1];
        }
        effect = func_002C36D0(tip, 2, 0, 1);
        if (effect != 0) {
            effect->color = 0x407F7F7F;
            effect->size = D_L00_001AFF5C * 31500.001953125f;
        }
        func_L00_003C5310(vertices, 20, 14, 14,
                          D_L00_001AFF54, D_L00_001AFF54 & 0xFFFFFF,
                          D_L00_001AFF58 | state->flags, D_L00_001AFF58,
                          0, D_L00_001B2940[i] - 1, D_L00_001B2940[i],
                          0.02f, 0.07f, D_L00_001AFF60);
    }
}
