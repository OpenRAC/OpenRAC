/* NON_MATCHING func_L00_00211D28 -- src/overlays/shared/help_0020CDF0.c
 * Best so far: BYTES 5/448 (98.9% of the bytes match), checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Hero per-frame input update: stick from pad (D_0013A5E0+0x2460) into D_0013E633+0xE1D+0x1D20, dpad fallback if
 *   Best try p8.c: 7 of 448 words differ, all register allocation of the dpad expression: retail puts (b>>13)&1 in
 *   Budget spent. Unblock: a wording of x = ((b>>13)&1) - ((b>>15)&1), y = ((b>>14)&1) - ((b>>12)&1) that swaps th
 *   Round q27/s06: best.c V4 typedef now conflicts with the file (rename it). p9 (four t12..t15 locals) 12 bytes, 
 *   2026-10-07 mini22 main-only: p12 negated-right plus left axis form improves BYTES5/448 (old7); p13 negative di
 */
extern char D_0013A5E0[];
extern char D_0013E633[];
extern int func_L00_00233CC0(void);
extern float func_L00_00211EE8(void);
extern float func_001F9CE8(void *);
extern void func_L00_002180F8(void);
extern void func_001FA218(void *, void *);
extern void func_001FA190(void *);
extern void func_001FA4A0(void *, void *);
extern void func_L00_00211A38(void);
extern void func_L00_00221A98(void);
extern void func_L00_00221D50(void);

typedef struct { float v[4]; } InputHistoryVector_211D28 __attribute__((aligned(16)));
typedef struct {
    char pad0[0x2128];
    float ring[32];
    int head;
    int count;
    int head2;
    int count2;
} InputHistoryRings_211D28;

/* Per-frame update of the hero's stick input and motion history rings. */
void func_L00_00211D28(void) {
    int b;

    int r = func_L00_00233CC0();
    char *g = D_0013E633 + 0xE1D;
    char *pad;
    InputHistoryRings_211D28 *rb;

    g[0x20B3] = r;
    *(float *)(g + 0x229C) = func_L00_00211EE8();
    pad = D_0013A5E0 + 0x2460;
    rb = (InputHistoryRings_211D28 *)g;
    *(int *)(g + 0x240) = 0;
    *(float *)(g + 0x1D20) = *(float *)(pad + 0x108);
    *(float *)(g + 0x1D24) = *(float *)(pad + 0x10C);
    if (func_001F9CE8(g + 0x1D20) < 0.25f) {
        b = *(int *)(pad + 0x1B0);
        *(float *)(g + 0x1D20) = (float)(-((b >> 15) & 1) + ((b >> 13) & 1));
        *(float *)(g + 0x1D24) = (float)(((b >> 14) & 1) - ((b >> 12) & 1));
    }
    func_L00_002180F8();
    rb->ring[rb->head] = *(float *)(g + 0x98);
    rb->head = (rb->head + 1) % 32;
    rb->count = rb->count + 1;
    if (rb->count > 32) {
        rb->count = 32;
    }
    {
        InputHistoryVector_211D28 *ring2 = (InputHistoryVector_211D28 *)(g + 0x1B00);
        int h = rb->head2;
        qcopy(&ring2[h], g + 0x80);
        rb->head2 = (h + 1) % 32;
    }
    rb->count2 = rb->count2 + 1;
    if (rb->count2 > 32) {
        rb->count2 = 32;
    }
    func_001FA218(g, g + 0x90);
    func_001FA190(g + 0x40);
    func_001FA4A0(g + 0x40, g);
    func_L00_00211A38();
    func_L00_00221A98();
    func_L00_00221D50();
}
