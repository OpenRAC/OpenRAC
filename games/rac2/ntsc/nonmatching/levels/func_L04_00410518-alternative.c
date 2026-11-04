/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L04_00410518: 620 of 620 bytes, instruction mix identical (same loops, same constants,
 * 9999.0f seed, same stack slots 0x10/0x14/0x18/0x1C/0x20), but the register assignment differs:
 * retail keeps the loop counter i in $s3 and the route pointer in $s2 (pt $s0, inner j $s1),
 * mine allocates i=$s2, route=$s3, pt=$s1, j=$s0, so every instruction touching them differs.
 * Tried: swapping local declaration order (four orders), a separate counter for the first
 * loop (moves it to $s1), a separate route pointer per loop, no reload of route at the end.
 * None changed which of i/route gets the higher callee-saved number.
 */
#include "common.h"
extern float func_L04_0040F8F0(u8 *, u8 *, int, int, int, int);
extern void func_0030EDC8(u8 *, u8 *, u8 *);
extern f32 func_0030EF50(u8 *);
extern u8 *D_L04_001FE340[];

/* Finds the nearest point on the current route or one of its four links. */
void func_L04_00410518(u8 *a0, u8 *a1, s32 *a2, s32 *a3, f32 *a4, s32 a5) {
    f32 d[4] __attribute__((aligned(16)));
    f32 best = 9999.0f;
    s32 prev3 = *a3;
    s32 prev2 = *a2;
    s32 *link;
    u8 *pt;
    s32 j;
    u8 *route1;
    u8 *route;
    s32 i;
    s32 half;
    f32 dist;

    *a2 = -1;
    *a3 = 0;
    *a4 = 0.0f;
    route1 = D_L04_001FE340[*(s32 *)(a1 + 0x1A8)];
    pt = route1 + 0x10;
    for (i = 0; i < *(s32 *)route1 - 1; i++) {
        func_0030EDC8((u8 *)d, a0 + 0x10, pt);
        dist = func_0030EF50((u8 *)d);
        if (dist < best) {
            *a3 = i;
            best = dist;
        }
        pt += 0x10;
    }
    link = (s32 *)(a1 + 0x1AC);
    for (i = 0; i < 4; i++) {
        if (link[i] == -1) {
            continue;
        }
        route = D_L04_001FE340[link[i]];
        pt = route + 0x10;
        for (j = 0; j < *(s32 *)route - 1; j++) {
            func_0030EDC8((u8 *)d, a0 + 0x10, pt);
            dist = func_0030EF50((u8 *)d);
            if (dist < best) {
                *a3 = j;
                *a2 = i;
                best = dist;
            }
            pt += 0x10;
        }
    }
    half = (s32)((f32)*(s32 *)D_L04_001FE340[*(s32 *)(a1 + 0x1A8)] * 0.5f);
    if (prev2 == -1 && prev3 == 0 && *a2 == prev2) {
        if (half < *a3) {
            *a3 = 0;
        }
    }
    *a4 = func_L04_0040F8F0(a0, a1, *a2, *a3, a5, *(s16 *)(a0 + 0xAA) == 0xBBC);
}
