/* func_00328B68 - retained near miss, USA v1.01.
 *
 * Current-master check: temporarily compiled in src/game/00327038.c using
 * `python3 tools/check.py func_00328B68 --diff`: 312 bytes against 316 retail,
 * 0 of 28 places. Expressing the call as func_00283180(a - pi) correctly keeps
 * retail's subtraction in the call delay slot and restores the pi constant.
 * The remaining mismatch starts in the prologue: retail saves f26 and uses it
 * for pi, with a 0x70-byte frame; this cc1 allocates pi to f25 and a 0x60-byte
 * frame. Later FPU register choices and call setup also differ. 29 Oct 2026 probes keeping pi in a const local outside the loop and inlining
 * the angle load into the call both remained 312/316, 0/28; neither changed
 * the prologue allocation.
 *
 * Retail body: 28 placements (the boot and every level).
   Retail at 0x00328B68: the hundred resident entries at 0x247900 are walked
   (the entry pointer steps by 0x40); for the entry whose +0x24 word is the
   argument, the +0x20 angle is turned by func_00283180, halved between 0 and
   1, scaled by 128 and drawn twice (func_L00_003E0F58 with the colours
   0x103080 and 0x7F7F7F).
   Retail materialises the table symbol and adds 0x10 to it (lui $v0,0x23 ;
   addiu $v0,$v0,-0x4F60 ; addiu $s1,$v0,0x10), while this candidate folds the
   +0x10 into the pointer. The main remaining obstacle is the FPU register
   allocation/prologue; later call setup follows from that allocation. */
extern float func_00283180(float);
extern int func_00283CF0(float);
extern void ref_L00_00328B68_00(void *, int, int, float, float, float, int, int);
extern char ref_L00_00328B68_01[];

void func_00328B68(char *a0)
{
    char *p = ref_L00_00328B68_01 + 0x10;
    float a;
    float s;
    float u;
    int n;
    int i;

    for (i = 0x63; i >= 0; i--) {
        if (*(int *)(p + 0x24) == (int)a0) {
            a = *(float *)(p + 0x20);
            s = func_00283180(a - 3.1415927f);
            s = (s + 1.0f) * 0.5f;
            n = func_00283CF0(s);
            u = s * 128.0f;
            ref_L00_00328B68_00(p, 0x103080, n, 0.0f, u, 0.01099406534147262573f, 0x2E, 2);
            ref_L00_00328B68_00(p, 0x7F7F7F, n, 0.0f, 0.0625f, 0.01099406534147262573f, 0x2E, 2);
        }
        p += 0x40;
    }
}
