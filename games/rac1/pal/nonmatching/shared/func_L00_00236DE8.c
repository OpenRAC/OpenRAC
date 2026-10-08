/* NON_MATCHING func_L00_00236DE8 -- src/overlays/shared/hud_00235960.c
 * Best so far: SIZE ours 328 / retail 332, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Bounces 100 HUD particles (16-byte records x,y,vx,vy; every other one, parity from frame counter D_L00_0015F4F
 *   Difference: retail has `jal; nop; lui $3; lw $3,%lo($3)` (delay slot unfilled, cnt in $3, tmp reuse `sll $2,$2
 *   Unblock: something makes the counter load ineligible for the call's delay slot (length-8 macro load) without M
 *   Round q30/w04: src has func_L00_00236830(HudElem *), so a no-arg call needs an alias decl (extern void f_v() _
 */
extern void func_L00_00236830_v() __asm__("func_L00_00236830");
extern int D_L00_0015F4F8;
extern float D_L00_0017E760[][4];
extern short D_L00_0015F830;
extern short D_L00_0015F834;
extern short D_L00_0015F838;

/* bounces the drifting particles around a box that depends on their index */
void func_L00_00236DE8(void) {
    int i;
    int par;
    float *p;

    func_L00_00236830_v();
    par = D_L00_0015F4F8 % 2;
    p = D_L00_0017E760[par];
    for (i = par; i < 100; i += 2) {
        float lim;
        float a, b;

        if (i < 10) {
            lim = *(float *)&D_L00_0015F830;
        } else if (i < 30) {
            lim = *(float *)&D_L00_0015F834;
        } else {
            lim = *(float *)&D_L00_0015F838;
        }
        a = lim - 17.0f;
        b = lim - 116.0f;
        p[0] += p[2];
        p[1] += p[3];
        if (p[0] < a) {
            p[0] = a;
            p[2] = -p[2];
        }
        if (-a < p[0]) {
            p[0] = -a;
            p[2] = -p[2];
        }
        if (p[1] < b) {
            p[1] = b;
            p[3] = -p[3];
        }
        if (-b < p[1]) {
            p[1] = -b;
            p[3] = -p[3];
        }
        p += 8;
    }
}
