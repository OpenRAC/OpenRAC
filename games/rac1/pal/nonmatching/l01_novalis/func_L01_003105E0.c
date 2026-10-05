/* NON_MATCHING func_L01_003105E0 -- src/overlays/l01_novalis/vendor_002FABE8.c
 * Best so far: SIZE ours 264 / retail 304, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UvScroller update (UpdateMoby_1848): state 0 zeroes the two UV offsets D_L01_001621A0/A4 and goes to state 1; 
 *   Natural source (p0.c) gives 280 bytes vs 304: same structure, but ours keeps the new B value in a register ($f
 *   Every wording of the same statements (locals, explicit temps, decl variants) compiles to the same bytes; an un
 */
void func_001F49B0(void *fn, void *arg);
extern f32 D_L01_001621A0_105E0[2] __asm__("D_L01_001621A0");
void func_L01_003104B8(void);
typedef struct {
    u8 pad0[0x20];
    u8 state;
} ScrollMoby;
extern f32 D_L01_001621A0_105E0b[2] __asm__("D_L01_001621A0");
extern f32 D_0015EE7C MACRO_ADDR;
void func_L01_003104B8_105E0b(void) __asm__("func_L01_003104B8");
void func_001F49B0_105E0b(void *fn, void *arg) __asm__("func_001F49B0");

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/overlays/l01/gameplay_vendor_00309bf8.c, FUN_L01_0030f208. */
void func_L01_003105E0(ScrollMoby *m) {
    f32 d;

    switch (m->state) {
    case 0:
        D_L01_001621A0_105E0b[0] = 0.0f;
        D_L01_001621A0_105E0b[1] = 0.0f;
        m->state = 1;
        break;
    case 1:
        d = D_0015EE7C * 0.025f;
        D_L01_001621A0_105E0b[0] += d;
        D_L01_001621A0_105E0b[1] += d;
        if (D_L01_001621A0_105E0b[0] > 1.0f) {
            D_L01_001621A0_105E0b[0] -= 1.0f;
        }
        if (D_L01_001621A0_105E0b[0] < -1.0f) {
            D_L01_001621A0_105E0b[0] += 1.0f;
        }
        if (D_L01_001621A0_105E0b[1] > 1.0f) {
            D_L01_001621A0_105E0b[1] -= 1.0f;
        }
        if (D_L01_001621A0_105E0b[1] < -1.0f) {
            D_L01_001621A0_105E0b[1] += 1.0f;
        }
        func_001F49B0_105E0b(func_L01_003104B8_105E0b, m);
        break;
    }
}
