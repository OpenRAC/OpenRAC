/* NON_MATCHING func_L00_001F92E8 -- src/overlays/shared/draw_001F3A78.c
 * Best so far: SIZE ours 252 / retail 248, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Draws the screen-fade overlay (clamp float D_L00_0015F4FC to 1.0, scale by 128, func_001FA898 -> func_001F55C0
 *   Matched: fade part (float loaded via lui decl, clamp store via a `short` alias of the same asm name = $gp stor
 *   Difference: retail keeps `lui $5,%hi(D_L00_0015F706); lh` inside the loop top with a nop in the bgez delay slo
 */
#include "common.h"
typedef struct { int a, b, c, d; } Ent;
extern int D_L00_0015F6BC MACRO_ADDR;
extern int D_L00_0015F708;
extern float D_L00_0015F4FC;
extern short D_gp __asm__("D_L00_0015F4FC");
extern float D_f2 __asm__("D_L00_0015F4FC");
extern Ent D_L00_00173180[];
extern short D_L00_0015F706[];
extern void func_001F7680(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F55C0(int, int, int, int);
extern void func_001F3C10(void);
extern void func_001F4630(int);
extern void func_001F4748(void);
extern void func_L00_001FCC50(Ent *, int);

/* Draws the screen fade and the queued entries when the debug flag is clear. */
void func_L00_001F92E8(void) {
    int i;
    if (D_L00_0015F6BC == 0) {
        func_001F7680(D_L00_0015F708);
        if (D_L00_0015F4FC > 0.0f) {
            if (D_L00_0015F4FC > 1.0f) *(float *)&D_gp = 1.0f;
            func_001F55C0(0, 0, 0, func_001FA898_r(D_f2 * 128.0f));
        }
        func_001F3C10();
        func_001F4630(0);
        i = 0;
        if (D_L00_00173180[0].a >= 0) {
            do {
                func_L00_001FCC50(&D_L00_00173180[i], D_L00_0015F706[0]);
                i++;
            } while (D_L00_00173180[i].a >= 0);
        }
        func_001F4748();
    }
}
