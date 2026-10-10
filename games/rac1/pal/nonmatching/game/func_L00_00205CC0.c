/* Clamps two float terms at B+0x8A8 and B+0x8A4 from B+0x278 and B+0x274; when the timer at
 * S+0xFF0 is running, calls func_0020D960 / func_0020D9D8 for the slots flagged in S+0xD31 and copies
 * three 16-byte tables into each slot, then fills S+0xD3C from the float table at D_L00_0017C290. */
typedef struct { unsigned int w[4]; } __attribute__((aligned(16))) Quad16c;

extern char D_L00_0017A780[];
extern char D_L00_0017C220[];
extern char D_L00_0017C140[];
extern char D_L00_0017C1B0[];
extern char D_L00_0017C2C0[];
extern char D_L00_0017C290[];
extern void func_L00_00205C18(void);
extern void func_0020D9D8(void *, void *);

void func_L00_00205CC0(void) {
    char *b = D_L00_0017A780;
    char *s = (char *)D_0013F450;
    float v, q, y;
    int k, cnt;
    char *p;

    v = *(float *)(b + 0x278) * 0.25f;
    if (0.148352981f < v) {
        *(float *)(b + 0x8A8) = v;
        *(float *)(b + 0x8A8) = 0.148352981f;
    } else {
        *(float *)(b + 0x8A8) = v;
        if (v < -0.148352981f) *(float *)(b + 0x8A8) = -0.148352981f;
    }

    q = *(float *)(b + 0x274) / 2.8f;
    *(float *)(b + 0x8A4) = q;
    if (0.296705961f < q) *(float *)(b + 0x8A4) = 0.296705961f;
    y = *(float *)(b + 0x8A4);
    if (y < -0.17453292f) *(float *)(b + 0x8A4) = -0.17453292f;

    p = *(char **)(s + 0x2080);
    if ((unsigned int)(*(unsigned char *)(p + 0x53) - 1) < 2u) {
        if (0.0349065848f < *(float *)(b + 0x8A8)) *(float *)(b + 0x8A8) = 0.0349065848f;
        if (*(float *)(b + 0x8A8) < -0.0349065848f) *(float *)(b + 0x8A8) = -0.0349065848f;
        if (0.122173049f < *(float *)(b + 0x8A4)) *(float *)(b + 0x8A4) = 0.122173049f;
        if (*(float *)(b + 0x8A4) < -0.122173049f) *(float *)(b + 0x8A4) = -0.122173049f;
    }

    func_L00_00205C18();

    cnt = *(int *)(s + 0xFF0);
    if (cnt == 0) return;
    cnt = cnt + 1;
    *(int *)(s + 0xFF0) = cnt;
    if (cnt >= 12) *(int *)(s + 0xFF0) = 0;

    if (*(int *)(s + 0xFF0) == 0) {
        for (k = 0; k < 7; k++) {
            if (*(unsigned char *)(s + 0xD31 + 0x40 * k) != 0)
                func_0020D9D8(*(void **)(s + 0x2080), s + 0xD30 + 0x40 * k);
        }
        return;
    }

    for (k = 0; k < 7; k++) {
        if (*(unsigned char *)(s + 0xD31 + 0x40 * k) == 0) {
            func_0020D960(*(char **)(s + 0x2080), *(int *)(D_L00_0017C2C0 + 4 * k),
                          s + 0xD30 + 0x40 * k);
            *(unsigned char *)(s + 0xD33 + 0x40 * k) = 1;
            p = s + 0xD40 + 0x40 * k;
            *(Quad16c *)p = *(Quad16c *)(D_L00_0017C140 + 0x10 * k);
            p = s + 0xD50 + 0x40 * k;
            *(Quad16c *)p = *(Quad16c *)(D_L00_0017C1B0 + 0x10 * k);
            p = s + 0xD60 + 0x40 * k;
            *(Quad16c *)p = *(Quad16c *)(D_L00_0017C220 + 0x10 * k);
        }
    }

    for (k = 0; k < 7; k++) {
        cnt = *(int *)(s + 0xFF0);
        *(float *)(s + 0xD3C + 0x40 * k) = ((float *)D_L00_0017C290)[cnt];
    }
}
