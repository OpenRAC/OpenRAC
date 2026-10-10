/* NON_MATCHING func_L00_0029ADD8 -- src/overlays/shared/tieproc_00299108.c
 * Best so far: SIZE ours 400 / retail 404, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   EnterSlideshowMode: starts the slideshow (func_00217748 x2, rebases the level pointers D_L00_0016128C/F708/F70
 *   Best try p7.c (400/404 bytes, SIZE mismatch): the code and the load/store order match retail, but the compiler
 *   Unblock: a declaration form for D_L00_0015F700+{2,8,0xC} that makes the store length 8 to the compiler (not sl
 */
extern void func_001F4E08(int);
extern void func_001F99B0(void *, int, int);
extern int func_00217628_w(int, int, int) __asm__("func_00217628");
extern int D_L00_0015F6BC MACRO_ADDR;
extern int D_L00_0015F6A8 MACRO_ADDR;
extern short D_L00_0015F4FC_g __asm__("D_L00_0015F4FC");
extern int D_L00_0015F700[];
extern int D_L00_00173F00[];
extern int D_L00_0016128C MACRO_ADDR;
extern int D_L00_0015F70C MACRO_ADDR;
extern int D_L00_0015F708 MACRO_ADDR;
extern short D_L00_0015F702[2] MACRO_ADDR;
extern int D_0015EE80 MACRO_ADDR;
typedef struct { int b; int a; } LL9;
typedef struct { char pad[0x16A8]; LL9 normal[20]; LL9 alt[20]; } G9;
extern G9 D_00137C80_h __asm__("D_00137C80");

/* starts the slideshow: resets state, rebases the level data pointers, and starts the first two streams */
void func_L00_0029ADD8(void) {
    int t;
    func_00217748(0);
    func_001F4E08(8);
    D_L00_0015F6BC = 1;
    D_L00_0015F6A8 = 7;
    *(float *)&D_L00_0015F4FC_g = 1.0f;
    func_001F99B0(D_L00_0015F700, 0, 16);
    t = D_L00_0016128C - 0xE0000;
    D_L00_0015F708 = D_L00_00173F00[1] + t;
    D_L00_0015F70C = D_L00_00173F00[2] + t;
    D_L00_0016128C = t;
    if (D_0015EE80 != 0) {
        func_00217628_w(D_L00_0015F708, D_00137C80_h.alt[D_L00_0015F702[0]].a, D_00137C80_h.alt[D_L00_0015F702[0]].b);
    } else {
        func_00217628_w(D_L00_0015F708, D_00137C80_h.normal[D_L00_0015F702[0]].a, D_00137C80_h.normal[D_L00_0015F702[0]].b);
    }
    func_00217748(1);
    D_L00_0015F702[0]++;
    if (D_0015EE80 != 0) {
        func_00217628_w(D_L00_0015F70C, D_00137C80_h.alt[D_L00_0015F702[0]].a, D_00137C80_h.alt[D_L00_0015F702[0]].b);
    } else {
        func_00217628_w(D_L00_0015F70C, D_00137C80_h.normal[D_L00_0015F702[0]].a, D_00137C80_h.normal[D_L00_0015F702[0]].b);
    }
}
