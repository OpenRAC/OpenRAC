/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L03_003F6948 (L03, 604 bytes): NOT matched. Best attempt compiles to 588 bytes in a -G8 unit.
 * Structure matches (frame 0x130, both loops, call order, the 1/60 and 1.0 constants); the second
 * loop matches. Remaining differences: register numbers in the first loop (retail s0=index,
 * s1=pointer, s5=state, s6=0xA72; branch-likely delay slots filled differently), and the table
 * loop, where retail's preheader materialises 0x1B10E4 as an adjacent lui/addiu (mine splits
 * lui/addiu around other IV setup, which needs -mno-split-addresses that this file's flags lack)
 * and uses one counter register. Tried: switch/else-if/continue forms for the 0xFE/0xFD test
 * (only the nested "!= 0xFE then != 0xFD continue" form gives retail's beq/bnel shape),
 * separate counters per loop, f32[4] locals (needed for frame layout), qcopy for the vec copy,
 * SDATA incomplete arrays for gp access, sized/nosda variants of the 0x1B10E4 symbol.
 * Place in src/levels/03_endako/L03_003F09C0-g8.c. */
#include "common.h"

typedef int L03_6948Quad __attribute__((mode(TI)));

typedef struct {
    f32 x;
    f32 y;
} L03_6948Pair;

typedef struct {
    L03_6948Quad v[4];
    s32 w[4];
    L03_6948Pair p[4];
    u8 tail[0x20];
} L03_6948Table;

typedef struct {
    u8 pad0[0x340];
    u32 entries[30];
    u32 unknown3B8;
    s32 count;
} L03_6948State;

extern L03_6948Quad D_L03_001B10A0[] SDATA(D_L03_001B10A0);
extern f32 D_L03_001B10E0[] SDATA(D_L03_001B10E0);
extern f32 D_L03_001B10E4[7];
extern s32 D_L03_001B1100[] SDATA(D_L03_001B1100);
extern f32 D_L03_001B1064 SDATA(D_L03_001B1064);
extern u8 D_L03_001B9980[];

extern void func_L03_003F6918(u8 *entity, s32 index);
extern void func_2ECA50(void *table, s32 a, s32 b, s32 c);
extern void func_2EEFE0(void *, void *, void *);
extern void func_2EF298(void *, void *, f32);
extern void func_2EF0C8(void *, void *, void *);
extern void func_2EB690(void *, void *, s32);

void func_L03_003F6948(u8 *entity) {
    L03_6948State *state = *(L03_6948State **)(entity + 0x68);
    f32 a[4] __attribute__((aligned(16)));
    f32 b[4] __attribute__((aligned(16)));
    f32 c[4] __attribute__((aligned(16)));
    f32 d[4] __attribute__((aligned(16)));
    L03_6948Table table;
    s32 i;
    s32 k;
    s32 j;
    f32 scale;

    for (i = 0; i < state->count; i++) {
        u8 *e = (u8 *)state->entries[i];

        if (e != 0 && *(s16 *)(e + 0xAA) == 0xA72) {
            if (e[0x20] != 0xFE) {
                if (e[0x20] != 0xFD) {
                    continue;
                }
            }
            func_L03_003F6918(entity, i);
        }
    }
    if (state->count == 0) {
        return;
    }
    func_2ECA50(&table, 5, 1, 0x80);
    for (k = 0; k < 4; k++) {
        table.v[k] = D_L03_001B10A0[k];
        table.w[k] = D_L03_001B1100[k >> 1];
        table.p[k].x = D_L03_001B10E0[k * 2];
        table.p[k].y = D_L03_001B10E4[k * 2];
    }
    scale = D_L03_001B1064 * 0.016666668f;
    for (j = 0; j < state->count; j++) {
        u8 *e = (u8 *)state->entries[j];
        u8 *es = *(u8 **)(e + 0x68);

        qcopy(d, e + 0x10);
        d[3] = 1.0f;
        func_2EEFE0(a, D_L03_001B9980, d);
        func_2EF298(b, es, (f32)*(s16 *)(es + 0x12) * scale);
        func_2EF0C8(c, b, a);
        func_2EF298(c, c, 1.0f);
        func_2EF0C8(a, b, c);
        func_2EB690(&table, a, 0);
    }
}
