/* NON_MATCHING func_L06_00223630 -- src/overlays/l06_blarg/help_00223630.c
 * Best so far: BYTES 9/612 (98.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Resolves pending hero action id w (g[0x12E0], cleared via func_001F99D8) into flag bytes, then fires func_L06_
 *   Best p3.c (BYTES 9/612, i.e. 4 instructions differ at +124..+134): fresh `char *gN = D_0013F450;` per if-block
 *   Left: retail's first `lbu g[0x12E5]` test branches (beql) straight past the second identical test to the final
 */
extern unsigned char D_0013F450[] NOT_SDA;
extern void func_001F99D8(void *, int);
extern void func_L00_0020BFA8(void);
extern void func_L06_00235E08(int, int);
extern float func_001F9B88(float);

// Resolves the pending hero action id into input flags, then triggers a state change when its conditions hold.
void func_L06_00223630(void) {
    char *g = (char *)D_0013F450;
    char *g1;
    char *g2;
    char *g3;
    int u = *(unsigned char *)(g + 0x12ED);
    int w = *(short *)(g + 0x12E0);
    func_001F99D8(g + 0x12E0, 0x10);
    *(unsigned char *)(g + 0x12ED) = u;
    *(short *)(g + 0x12E0) = -1;
    *(unsigned char *)(g + 0x20A9) = 0;
    *(short *)(g + 0x308) = 0;
    if (w == -1) return;
    if (w == 2) {
        if (*(short *)(g + 0x30C) == 0 || *(float *)(g + 0x2DC) < 0.3f) {
            *(unsigned char *)(g + 0x12E7) = 1;
            if (*(unsigned char *)(g + 0x20A4) == 0) {
                char *m = *(char **)(g + 0x10E0);
                if (m != 0 && *(short *)(m + 0xA6) == 0xAD) *(short *)(g + 0x308) = 1;
            }
        }
    }
    if (w == 0xD) {
        char *gx = (char *)D_0013F450;
        *(unsigned char *)(gx + 0x12EC) = 1;
    }
    if (w == 1) {
        char *gx = (char *)D_0013F450;
        *(unsigned char *)(gx + 0x12E5) = 1;
    }
    if (w == 8) {
        char *gx = (char *)D_0013F450;
        *(unsigned char *)(gx + 0x12EA) = 1;
    }
    if (w == 9) {
        char *gx = (char *)D_0013F450;
        *(unsigned char *)(gx + 0x12EE) = 1;
    }
    if (w == 0xC) {
        char *gx = (char *)D_0013F450;
        *(unsigned char *)(gx + 0x12EA) = 1;
    }
    g1 = (char *)D_0013F450;
    if (*(unsigned char *)(g1 + 0x12E5) != 0 && *(int *)(g1 + 0x300) != 0 && *(unsigned char *)(g1 + 0x20A4) == 1 && *(int *)(g1 + 0x2084) != 0x7D) {
        func_L06_00235E08(0x7D, 1);
        return;
    }
    g2 = (char *)D_0013F450;
    if (*(unsigned char *)(g2 + 0x12E5) != 0) {
        if (*(int *)(g2 + 0x300) != 0) {
            if (*(int *)(g2 + 0x2084) != 0x3C || *(short *)(g2 + 0x41E) != 0) {
                func_L00_0020BFA8();
                if (*(int *)(g2 + 0x22A8) != 0) {
                    func_L06_00235E08(0x3C, 1);
                } else {
                    func_L06_00235E08(0x7C, 1);
                }
                return;
            }
        }
    }
    g3 = (char *)D_0013F450;
    if (*(unsigned char *)(g3 + 0x12EC) == 0) return;
    if (*(int *)(g3 + 0x2084) == 0x7F) return;
    if (!(func_001F9B88(*(float *)(g3 + 0x2F0) - (*(float *)(g3 + 0x88) + 0.25f)) < 1.0f)) return;
    if (!(0.0f < *(float *)(g3 + 0x2F0) - *(float *)(g3 + 0x88))) return;
    if (!(*(float *)(g3 + 0x108) < 0.0f)) return;
    func_L00_0020BFA8();
    func_L06_00235E08(0x7F, 1);
}
