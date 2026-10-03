/* NON_MATCHING func_L00_0029AFB8 -- src/overlays/shared/tieproc_00299108.c
 * Best so far: SIZE ours 748 / retail 756, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   Per-frame state machine for the level intro/stream swap: fades D_L00_0015F4FC, steps D_L00_0015F700 state 0 (w
 *   Structure and all gp/lui forms match except SIZE 748 vs 756: retail never fills the delay slots of the branche
 *   Would unblock: knowing why retail's scheduler does not thread the tail load into those slots (probably a diffe
 */
extern void func_00217748(int);
extern void func_L00_0029AF70(void);
extern void func_00234AC8(int);
extern int func_00217628_a(int *, int, int) __asm__("func_00217628");
extern void func_0022DD68(void);
extern int D_L00_0015F4FC MACRO_ADDR;
extern int D_L00_0015F702_m __asm__("D_L00_0015F702") MACRO_ADDR;
#define D_L00_0015F702 (*(short *)&D_L00_0015F702_m)
extern int D_L00_0015F704_m __asm__("D_L00_0015F704") MACRO_ADDR;
#define D_L00_0015F704 (*(short *)&D_L00_0015F704_m)
extern int D_L00_0015F706_m __asm__("D_L00_0015F706") MACRO_ADDR;
#define D_L00_0015F706 (*(short *)&D_L00_0015F706_m)
extern int D_L00_0015F700_mm __asm__("D_L00_0015F700") MACRO_ADDR;
extern int D_L00_00173D00[];
extern int *D_L00_0015F708 MACRO_ADDR;
extern int *D_L00_0015F70C MACRO_ADDR;
extern int D_0015EE80 MACRO_ADDR;
extern char D_00137C80[];
extern unsigned char D_0013A5E0[];
extern unsigned char D_0014171B[] NOT_SDA;

void func_L00_0029AFB8(void) {
    int s0, t, i, r;
    int *src, *dst;
    float f;
    func_00217748(0);
    f = *(float *)&D_L00_0015F4FC - 0.05f;
    *(float *)&D_L00_0015F4FC = f;
    if (f < 0.0f) {
        *(float *)&D_L00_0015F4FC = 0.0f;
    }
    s0 = D_0013A5E0[0x2CF2] == 0x41;
    if ((*(short *)&D_L00_0015F700_mm) == 0) {
        if (D_L00_0015F706 >= D_L00_00173D00[D_L00_0015F702] - 0x24) {
            if (*(short *)(D_0014171B + 0x100BD) != 0) {
                func_00217748(1);
            }
            if ((!s0 && D_L00_0015F702 == 0x29) || D_L00_0015F702 >= 0x2D) {
                func_L00_0029AF70();
                return;
            }
            D_L00_0015F704 = -1;
            (*(short *)&D_L00_0015F700_mm) = 1;
        }
    } else if ((*(short *)&D_L00_0015F700_mm) == 1) {
        func_00234AC8(1);
        t = D_L00_0015F704;
        if (t < 0x20) {
            int off = t * 4;
            src = (int *)((char *)D_L00_0015F70C + off);
            dst = (int *)((char *)D_L00_0015F708 + off);
            for (i = 0; i <= 0x37FFF; i += 0x20) {
                *dst = *src;
                dst += 0x20;
                src += 0x20;
            }
        } else if (t >= 0x24) {
            if (!s0) {
                if (D_L00_0015F702 == 8) {
                    D_L00_0015F702 = 0x11;
                    D_L00_0015F706 = 0x1590;
                }
            } else {
                if (D_L00_0015F702 == 0x11) {
                    D_L00_0015F702 = 0x1B;
                    D_L00_0015F706 = 0x2238;
                }
                if (D_L00_0015F702 == 0x27) {
                    D_L00_0015F702 = 0x29;
                    D_L00_0015F706 = 0x32B4;
                }
            }
            (*(short *)&D_L00_0015F700_mm) = 0;
            D_L00_0015F704 = -1;
            D_L00_0015F702 = D_L00_0015F702 + 1;
            r = D_L00_0015F702 % 20;
            if (D_0015EE80 != 0) {
                func_00217628_a(D_L00_0015F70C, *(int *)(D_00137C80 + 0x1748 + r * 8), *(int *)(D_00137C80 + 0x174C + r * 8));
            } else {
                func_00217628_a(D_L00_0015F70C, *(int *)(D_00137C80 + 0x16A8 + r * 8), *(int *)(D_00137C80 + 0x16AC + r * 8));
            }
        }
    }
    D_L00_0015F704 = D_L00_0015F704 + 1;
    D_L00_0015F706 = D_L00_0015F706 + 1;
    if (*(int *)(D_0013A5E0 + 0x2604) & 0x800) {
        func_L00_0029AF70();
    }
    func_0022DD68();
}
