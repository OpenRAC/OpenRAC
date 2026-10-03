/* NON_MATCHING func_L00_0023B610 -- src/overlays/shared/hud_00235960.c
 * Best so far: SIZE ours 312 / retail 316, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   steps +0x6C between -6 and 30, calls func_001FFC48 at -6, and calls the record's hook (+0x14) when arg is set;
 *   the live count. Best: p3.c (index loop, e = (int *)D_L00_0017E5D8; e[1]) is 24 bytes off, all register allocat
 *   retail keeps n in $s1 and arg in $s2, ours swaps them (prologue store order follows). Reordering declarations,
 *   before the call did not move it (p4, p5); the function-pointer-local variant p6 was not run (budget spent by m
 *   on parallel runs). Unblock: a wording that changes the priority of arg vs n.
 *   Second session (lb1/p02): Lombyte-style pointer loop with do/while (p7), n declared/assigned at other places (
 *   all still give the same 24-byte diff (n in $s2, arg in $s1 instead of the reverse; p10 also flips slt to sltu)
 *   outranks n. Stopped; a wording that raises n's priority or lowers arg's would fix the whole diff.
 */
extern int func_001F9850(int);
extern void func_001FFC48(void *);
extern char D_L00_0017E5D8[];

/* Per-frame HUD bank update: tick each record's timer and slide, call its hook; returns how many are live. */
int func_L00_0023B610(int arg) {
    HudBank *p;
    int n;

    func_L00_0023B750(arg);
    n = 0;
    p = D_L00_0017DD50;
    do {
        if ((p->unk04 & 0x10) || *(int *)(D_L00_0017E5D8 + 4) != 0) {
            if (p->unk7C < func_001F9850(10)) {
                p->unk7C = func_001F9850(10);
            }
        }
        if (p->unk7C > 0 && --p->unk7C > 0) {
            n++;
            if (p->unk6C < 30) p->unk6C = p->unk6C + 1;
        } else {
            if (p->unk6C >= -5) p->unk6C = p->unk6C - 1;
        }
        if (p->unk68 != 0 && p->unk6C == -6) {
            func_001FFC48(p);
        }
        if (arg != 0 && *(void **)((char *)p + 0x14) != 0) {
            (*(void (**)(void *))((char *)p + 0x14))(p);
        }
        p++;
    } while (p < D_L00_0017DD50 + 13);
    return n;
}
