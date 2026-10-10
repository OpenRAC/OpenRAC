/* NON_MATCHING func_L06_002EAF90 -- src/overlays/shared/vendor_002D9548.c
 * Best so far: SIZE ours 424 / retail 428, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Pushes a moby out of walls: builds a step vector from moby+0xE0, applies to vel and position, then collision-t
 *   All p*.c compile to one fewer saved register (416 vs 428 bytes): retail keeps the scratch vector b (sp+0x10) i
 *   Round q30/x05: the old best.c did not compile (func_L00_00262DF0 takes (float, int, ...); u128 typedef clashes
 *   Left: retail does not rotate the final loop (slti/beqz at top, `b` back with addiu in the delay slot) and keep
 *   hq3 s06: best.c with the `while` loop as `if (f()) {...; i++} else break;` (p14) is 424/428, the best so far. 
 */
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001F9CB8(void *);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern void func_001F9BF0(void *, void *, void *);
extern int func_L00_001F10E0(float, void *, int, void *);
extern void func_L00_00262DF0(float, int, void *, void *);
extern float D_0015EE6C MACRO_ADDR;
extern unsigned char D_L06_001746E0[];
extern char D_L06_001746F0[];

/* Pushes a moby out of the walls along a velocity step, then optionally notifies its owner. */
void func_L06_002EAF90(char *moby, char *vel, int flag, float s, float r) {
    float v[16];
    float *a = v;
    float *b;
    float *c = v + 8;
    float *d = v + 12;
    int i;
    func_L00_001FF4B0(c, moby + 0xE0, s);
    func_L00_001FF4B0(d, c, -D_0015EE6C);
    func_001F9BD8(vel, vel, d);
    qcopy(a, (moby + 0x10));
    func_001F9BD8(a, a, c);
    func_001F9BD8((moby + 0x10), (moby + 0x10), vel);
    b = v + 4;
    func_001F9BD8(b, (moby + 0x10), c);
    if (r < func_001F9CB8(vel)) {
        if (func_L00_001EFFF0(a, b, 0, (int)moby, 0)) {
            qcopy(b, D_L06_001746E0);
            func_001F9BF0((moby + 0x10), b, c);
        }
    }
    i = 0;
    while (i < 6) {
        if (func_L00_001F10E0(r, b, 4, moby)) {
            qcopy(b, D_L06_001746F0);
            func_001F9BF0((moby + 0x10), b, c);
            i++;
        } else {
            break;
        }
    }
    if (flag != 0) {
        func_L00_00262DF0(0.5f, *(int *)(*(char **)(moby + 0x78) + 0x1EC), moby + 0x10, moby + 0x10);
    }
}
