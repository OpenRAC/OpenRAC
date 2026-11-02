/* GPT-6.1 Sol (high), 29 October 2026: retained, unlinked near miss.
 * USA v1.01, -O2 -G8 -ffunction-sections: 1432/1432 bytes, 0/1 places.
 * check.py: first instruction difference +0x50; retail 0xC0 frame and all
 * seven saved-GPR roles recovered. No full linked build was run.
 *
 * Improvements over the 28 October draft: a real union expresses the disjoint
 * state/table-cursor phases without keeping a second state pointer live.
 * The volatile declaration of the flag preserves both observed byte reads
 * at 0041D3C8 and 0041D43C; there are no per-access casts. The true flag path
 * skips entry cleanup when its timer is still below the threshold. Loading
 * the old part only after testing the replacement reproduces the transfer
 * loop's branch-likely and all four qcopy sequences.
 *
 * Remaining: the first true-flag branch hoists object/A1 setup differently;
 * post-transfer constant scheduling differs. Both final effect sequences
 * hoist the table offset and index parity too early, choose ordinary rather
 * than likely exit branches, and form the old part's +0xD0 vector address
 * in the following vector call's slot instead of the random call's slot.
 * Their stack-argument setup also schedules differently. Parity temporaries,
 * assignment placement, unsigned indices and nested call spelling did not
 * recover the retail schedule. NOT_SDA on the flag keeps its high half in
 * an extra live register; disabling GCSE does not fix the function.
 * Reopen with independent evidence for the final phase's source/call shape.
 * All required declarations follow; the original INCLUDE_ASM stays in src/.
 */

#include "common.h"
typedef int Quad __attribute__((mode(TI)));
/* Two five-part windows share this ten-pointer table. */
typedef struct { u8 prefix[0x358]; char *parts[10]; } PartState;
extern char *D_L23_001B1484 __attribute__((sda));
extern int D_L23_001B1468 __attribute__((sda));
extern int D_L23_001B1434 __attribute__((sda));
extern char *D_L23_001B13D0 __attribute__((sda));
extern float D_L23_001B16D0 __attribute__((sda));
extern int D_L23_001B3720;
/* Retail reads this byte again at 0041D43C on the entry-cleanup path. */
extern volatile unsigned char D_001A7AFD;
extern int func_002F5D10(void);
extern void func_002A6C58(char *, int, int);
extern void func_L23_0041D9B0(void);
extern void func_L00_003294F0(unsigned char *, int);
extern void func_0029F338(void *);
extern void *func_0032C700(void *, int, int, int, int, int, int, int, float, float, int, int, int, void *);
extern int func_002E5CF8(int, int, char *);
extern void func_0029FC20(char *, int, char *);
extern void func_00282D30(char *, char *, float);
extern float func_002A7878(float, float);
extern void func_00282CC0(char *, char *, char *);
extern char *func_00273730(char *, int, int, int, char *, char *, float *, int, float, float);
extern void func_00310C88(u8 *);
extern int func_002A77E0(int);

/* Move through the encounter's damage phases, detaching pieces and spawning debris. */
void func_L23_0041D398(char *object) {
    struct { Quad velocity, matrix[4], random; } local;
    /* The encounter state becomes a cursor into its part table in each phase. */
    union { PartState *state; char **parts; } cursor;
    char *old, *particle;
    int index;
    unsigned short timer;
    float scale;
    cursor.state = *(PartState **)(D_L23_001B1484 + 0x68);
    if (D_001A7AFD) {
        if (*(unsigned short *)(object + 0x96) <= D_L23_001B1468 + 60) goto update_parts;
    } else {
        if (*(unsigned short *)(object + 0x96) <= D_L23_001B1468 + 200) goto entry;
        if (func_002F5D10()) goto entry;
    }
    func_002A6C58(object, 0, 3);
    func_L23_0041D9B0();
    return;
entry:
    if (!D_001A7AFD && (unsigned short)*(short *)(object + 0x96) > D_L23_001B1468 + 60) {
        char *entryState;
        if (!D_L23_001B13D0) return;
        entryState = *(char **)(D_L23_001B13D0 + 0x68);
        entryState[13] = 1;
        entryState[8] = 0;
        func_L00_003294F0((unsigned char *)D_L23_001B13D0, 1);
        D_L23_001B13D0 = 0;
        return;
    }
update_parts:
    if (D_L23_001B1434) {
        int i;
        cursor.parts = cursor.state->parts + 5;
        for (i = 5; i < 10; i++, cursor.parts++) {
            if (!D_L23_001B3720) {
                func_0029F338(*cursor.parts);
                *cursor.parts = 0;
                *(unsigned short *)(object + 0x34) |= 0x41;
                func_0032C700(object + 16, 1, 135, 120, 120, 115, 125, 1, 6.5f, 20.0f, 1, 1, 1, 0);
                func_002E5CF8(12, 0, object);
            } else if (*cursor.parts) {
                int debrisIndex = 10 - D_L23_001B3720;
                func_0029FC20(object, i - 4, (char *)local.matrix);
                func_00282D30((char *)&local.velocity, (char *)&local.matrix[1], D_L23_001B16D0);
                scale = func_002A7878(0.0f, D_L23_001B16D0 * 0.5f);
                func_00282D30((char *)&local.random, object + 0xE0, scale);
                func_00282CC0((char *)&local.velocity, (char *)&local.velocity, (char *)&local.random);
                {
                    char *oldPart = *cursor.parts;
                    particle = func_00273730(oldPart, 1, *(short *)(oldPart + 0xAA), debrisIndex, oldPart + 16, oldPart + 0xF0, (float *)&local.velocity, 255, 140.0f, 25.0f);
                    if (particle) *(void (**)(u8 *))(particle + 0x64) = func_00310C88;
                }
            }
        }
        D_L23_001B3720--;
        return;
    }
    timer = *(unsigned short *)(object + 0x96);
    if ((D_L23_001B1468 << 1) / 3 < timer) {
        int i;
        cursor.parts = cursor.state->parts;
        func_0032C700(object + 16, 1, 45, 30, 40, 25, 85, 1, 4.5f, 20.0f, 1, 1, 1, 0);
        func_002E5CF8(11, 0, object);
        for (i = 4; i >= 0; i--, cursor.parts++) {
            char *replacement = cursor.parts[5];
            if (replacement) {
                char *old = *cursor.parts;
                if (!old) continue;
                qcopy(replacement + 0xC0, old + 0xC0);
                qcopy(replacement + 0xD0, old + 0xD0);
                qcopy(replacement + 0xE0, old + 0xE0);
                qcopy(replacement + 0x10, old + 0x10);
                *(unsigned short *)(replacement + 0x34) &= 0xFFF9;
                *(unsigned short *)(cursor.parts[5] + 0x34) |= 0x4000;
                *(unsigned short *)(cursor.parts[5] + 0x34) &= 0xFFBE;
                func_0029F338(*cursor.parts);
                *cursor.parts = 0;
            }
        }
        D_L23_001B1434 = 1;
        D_L23_001B3720 = 10;
        return;
    }
    if (D_L23_001B1468 / 3 < timer) {
        if (timer & 15) return;
        index = func_002A77E0(5);
        cursor.parts = cursor.state->parts;
        cursor.parts += index;
        old = *cursor.parts;
        scale = func_002A7878(4.0f, 12.0f);
        old += 0xD0;
        func_00282D30((char *)&local.velocity, old, scale);
        func_00282CC0((char *)&local.velocity, (char *)&local.velocity, *cursor.parts + 16);
        scale = func_002A7878(2.3f, 3.7f);
        index &= 1;
        func_0032C700(&local.velocity, index, 20, 10, 14, 15, 25, 0, scale, 20.0f, index, 0, 0, 0);
        func_002E5CF8(func_002A77E0(3) + 13, 0, object);
        return;
    }
    if (timer & 31) return;
    index = func_002A77E0(5);
    cursor.parts = cursor.state->parts;
    cursor.parts += index;
    old = *cursor.parts;
    scale = func_002A7878(4.0f, 12.0f);
    old += 0xD0;
    func_00282D30((char *)&local.velocity, old, scale);
    func_00282CC0((char *)&local.velocity, (char *)&local.velocity, *cursor.parts + 16);
    scale = func_002A7878(2.3f, 3.7f);
    index &= 1;
    func_0032C700(&local.velocity, index, 20, 10, 14, 15, 25, 0, scale, 20.0f, index, 0, 0, 0);
    func_002E5CF8(func_002A77E0(3) + 13, 0, object);
}
