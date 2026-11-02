/* GPT-6.1 Sol (high), 29 October 2026; retained near miss.
 * Profile: -O2 -G8 -ffunction-sections (default split addresses).
 * USA v1.01: 740/740 bytes, 0/3 places (18, 19, 20).
 * Incomplete sda arrays for the HUD resource pointer and screen width retain
 * absolute loads without keeping their address halves across calls. Together
 * with the shared hero base this recovers retail's prologue and body.
 * Computing blink before the range comparison also recovers the first plain
 * branch and the success value in the second branch's delay slot.
 * Signed word colors and the signed 64-bit text-color parameter retain the
 * measured zero-extended constants and sign-extending third color load.
 * Remaining: +0x210 uses andi v1,a2,24 instead of v0; +0x214 uses beql v1,zero
 * instead of beq v0,zero. Other non-relocated instruction words agree.
 * Direct short-circuit/goto spellings tie with the older blink sequence;
 * nested early returns add 12 bytes. keep-block-order and unsplit-edges did
 * not close it; disabling initial scheduling disturbs the prologue. No linked
 * build or other-release checks. Reopen with a new lifetime/control-flow witness.
 *
 * Claude Opus 5.5 (opus-1, v1648), 30 October 2026: the blink test written
 * naturally after the range test, `remaining < 32 || remaining > 300 ||
 * (remaining & 24)`, is retail's RTL (andi after the first branch); reorg then
 * decides. Measured: with the exits as `goto done` / nested `return 0` at the
 * end (the right exits), the first branch steals `sll` from the print block
 * (beql) instead of taking the fallthrough andi, whenever the return value
 * lives in v0 in both the return-1 and return-0 blocks (small tests: a void
 * function, `done: return 1`, or a result variable in another register all
 * fill it like retail). Inline `return 0` exits give retail's blink code but
 * lay the second exit out as `bnez; b` (748 bytes). `s32 result = 0; if (mode
 * == 4 && ptr) {...; result = 1;} return result;` gives every retail branch and
 * delay slot, but result lands in a1 (sched1 puts result = 0 between the hero
 * lui and lo_sum) plus a final move: 744 bytes, 15 lines. Reopen with a way to
 * have v0 = 0 before the first branch after the hero lo_sum.
 */
#include "common.h"
extern u8 *D_L18_001A904C[] __attribute__((sda));
extern u8 *D_0018C0B0 NOT_SDA;
extern u8 D_00189E20[];
extern s32 D_001A7340[] __attribute__((sda));
extern u32 D_L18_001B12C4 __attribute__((sda)), D_L18_001B12C8 __attribute__((sda));
extern u32 D_L18_001B12CC __attribute__((sda)), D_L18_001B12D0 __attribute__((sda));
extern s32 D_L18_001B12D4 __attribute__((sda)), D_L18_001B12D8 __attribute__((sda));
extern s32 D_L18_001B12DC __attribute__((sda)), D_L18_001B12E0 __attribute__((sda));
extern s32 D_L18_001B12E4 __attribute__((sda)), D_L18_001B12E8 __attribute__((sda));
extern char D_L18_001B1300[];
extern s32 func_00336D50(u8 *, s32);
extern void func_00300A00(char *, s32, float, float, float, float, float);
extern float func_00283CE0(s32);
extern s32 func_00283D38(s32, s32, float);
extern void func_0027EAE8(s32, s32, s32, s32, s32, void *);
extern s32 func_00115DA8(char *, char *, ...);
extern void func_L18_003044B8(s32, s32, s64, char *, s32, float);
s32 func_L18_00426AB0(void)
{
    s32 colors[2];
    char text[16];
    u8 *state;
    u8 *hero = D_00189E20;
    s32 middle;
    float ratio;
    float amount;
    s32 handle;
    u16 remaining;
    s32 blink;
    if (hero[0x22B4] != 4) goto done;
    if (!D_L18_001A904C[0]) goto done;
    handle = func_00336D50(D_L18_001A904C[0] + 0x8710, 0xDB);
    func_00300A00((char *)handle, 0x60442D00, (float)(D_001A7340[0] / 2), (float)(D_L18_001B12D8 + D_L18_001B12E8), 1.0f, 1.0f, 0.0f);
    handle = func_00336D50(D_L18_001A904C[0] + 0x8710, 0xDC);
    func_00300A00((char *)handle, 0x55F0C070, (float)(D_001A7340[0] / 2), (float)(D_L18_001B12D8 + D_L18_001B12E8), 1.0f, 1.0f, 0.0f);
    state = *(u8 **)(*(u8 **)(hero + 0x2290) + 0x68);
    amount = func_00283CE0(*(s16 *)(state + 0x3BC));
    ratio = amount / func_00283CE0(*(s16 *)(state + 0x3BE));
    colors[0] = func_00283D38(D_L18_001B12C4, D_L18_001B12CC, ratio);
    colors[1] = func_00283D38(D_L18_001B12C8, D_L18_001B12D0, ratio);
    middle = D_001A7340[0] / 2;
    func_0027EAE8(middle + D_L18_001B12D4, D_L18_001B12D8,
                  (s32)((float)(middle + D_L18_001B12D4) + (float)D_L18_001B12DC * ratio),
                  D_L18_001B12D8 + D_L18_001B12E0, 0xFFFFF1, colors);
    func_0027EAE8(middle - D_L18_001B12D4, D_L18_001B12D8,
                  (s32)((float)(middle - D_L18_001B12D4) - (float)D_L18_001B12DC * ratio),
                  D_L18_001B12D8 + D_L18_001B12E0, 0xFFFFF1, colors);
    remaining = *(u16 *)(state + 0x3BC);
    blink = remaining & 24;
    if ((u32)(remaining - 32) > 268 || blink) {
        func_00115DA8(text, D_L18_001B1300, (s16)remaining / 60);
        func_L18_003044B8(middle, D_L18_001B12E4 + 1, 0xFF000000, text, 3, 0.96f);
        func_L18_003044B8(middle, D_L18_001B12E4 - 1, 0xFF000000, text, 3, 1.04f);
        func_L18_003044B8(middle, D_L18_001B12E4, colors[0], text, 3, 1.0f);
    }
    return 1;
done:
    return 0;
}
