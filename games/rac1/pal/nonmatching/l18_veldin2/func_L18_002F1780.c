/* NON_MATCHING func_L18_002F1780 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: SIZE ours 824 / retail 816, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   Used ~11 of 14 runs.
 *   # Round 2 (11 of 14 runs)
 *   Finding: retail's first arg of func_001F9BD8 is &rows[k].v[j] (s1 += 0x10 per j), not &rows[k]; and `n = (data
 *   Results: p7 (int t=(j>>1)-1 to stop folding i+1) 824, p8 (t+v[j]) 836, p9 (p4+v[j]) 820, p10 (p9+n before if) 
 *   Remaining: retail recomputes the row address after func_001FA888 (second mult, sd 0 in delay slot), spills v2 
 *   # Round 3 (14 of 16 runs; p14-p18 of the cut-off worker were never written up)
 *   p17 had already reached size 816 (BYTES 508): v2 used directly (no v2p pointer) + `((float *)rows[i].pt)[j*2+1
 *   Remaining difference: (1) the moby search loop: retail keeps `lhu *p` reloaded in a bnel delay slot (loop body
 */
extern int D_L18_0015F6B0 MACRO_ADDR;
extern int *D_L18_001AC540[];
extern short D_L18_00160058;
extern float D_L18_001DA5C0[];
extern short D_L18_001622C8;
extern short D_L18_001622CC;
extern short D_L18_001622D0;
extern short D_L18_001622D4;
extern short D_L18_001622D8;
extern short D_L18_001622DC;
extern short D_L18_001622E0;
extern short D_L18_001622E4;
extern int func_001F4868(int);
extern float func_001FA888(int);
extern int func_001FA8A8(int, int, float);
extern void func_001153FC(void *, int, int);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
#define W(x) (*(int *)&(x))

typedef struct {
    float v[4][4];
    int col[4];
    float pt[4][2];
    long z;
    long tex;
    long k;
    long packed;
} Row;

extern void func_L18_002F1780_c(char *) __asm__("func_L18_002F1780");
void func_L18_002F1780_c(char *arg) {
    Row rows[16];
    float v2[8];
    int i, j;
    if (W(D_L18_001622C8) == 0) {
        for (i = 0; i < 16; i++) {
            float base;
            Row *r = &rows[i];
            r->tex = func_001F4868(0x13);
            r->k = 0xFF90L << 32 | 0x260;
            r->packed = (long)W(D_L18_001622D0) | (long)W(D_L18_001622D4) << 2 |
                             (long)W(D_L18_001622D8) << 4 | (long)W(D_L18_001622DC) << 6 | 0x8000L << 24;
            base = func_001FA888((D_L18_0015F6B0 + 3) & 3) * 0.0625f;
            r->z = 0;
            for (j = 0; j < 4; j++) {
                float f;
                ((float *)rows[i].pt)[j * 2 + 1] = D_L18_001DA5C0[j * 2 + 1];
                rows[i].pt[j][0] = D_L18_001DA5C0[j * 2];
                f = func_001FA888(i - ((j >> 1) - 1)) * 0.25f + base;
                if (f > 1.0f) {
                    f = 1.0f;
                } else if (f < 0.0f) {
                    f = 0.0f;
                }
                rows[i].col[j] = func_001FA8A8(W(D_L18_001622E0), W(D_L18_001622E4), f);
            }
        }
        func_001153FC(v2, 0, 0x20);
        v2[2] = 0.15f;
        v2[6] = -0.15f;
        {
            short *p = (short *)D_L18_001AC540[(unsigned char)arg[0x21]];
            do {
                char *moby;
                char *data;
                int k;
                do {
                    moby = (char *)((*p & 0x7FFF) << 8) + W(D_L18_00160058);
                } while (*(short *)(moby + 0xA6) != 0x54B);
                data = *(char **)(moby + 0x78);
                for (k = 0; k < *(short *)(data + 0x164); k++) {
                    int n = (*(short *)(data + 0x166) - k + 15) & 15;
                    if (W(D_L18_001622CC) == 0) {
                        for (j = 0; j < 4; j++) {
                            func_001F9BD8(rows[k].v[j], data + (((n + (j >> 1)) & 15) << 4) + 0x10, (char *)v2 + ((j & 1) << 4));
                        }
                        func_L00_001FD1D8(&rows[k], 0, 0);
                    }
                }
            } while (*p++ >= 0);
        }
    }
}
