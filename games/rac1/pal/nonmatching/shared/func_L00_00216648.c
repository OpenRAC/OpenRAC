/* NON_MATCHING func_L00_00216648 -- src/overlays/shared/help_00214D60.c
 * Best so far: SIZE ours 376 / retail 384, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_00216648: camera tuning per frame; when the cheat byte is set copies a float to D_L00_0017B48C, then 
 *   Best candidate p7.c: 376 bytes vs retail 384. Register allocation (f20/f21, the two addiu of D_L00_0017A780 vi
 *   Remaining difference: retail reloads *(int*)(p+0x1184) after the store to D_L00_0017B48C (gcc treats the store
 */
extern char D_0013E633[];
extern unsigned char D_0015EEB0[];
extern short D_0015EF18;
extern char D_L00_0017B48C[];
extern char D_L00_0017A780[];
extern float func_001F9B88(float);

/* Per-frame camera tuning: derives the tilt and bob values from the clamped look input. */
void func_L00_00216648(void) {
    char *p = D_0013E633 + 0xE1D;
    float inv, a, b, k;

    if (*(int *)(p + 0x1184) != 0) {
        if (D_0015EEB0[3] != 0) {
            *(float *)D_L00_0017B48C = *(float *)&D_0015EF18;
        }
        inv = 1024.0f / *(float *)(*(int *)(p + 0x1184) + 0x2C);
        if (*(int *)(p + 0x2084) == 2) {
            char *q = D_L00_0017A780, *r;
            p = D_0013E633 + 0xE1D;
            a = *(float *)(p + 0x188);
            *(float *)(q + 0xD04) = 0.02f;
            *(float *)(q + 0xD08) = 0.35f;
            if (a > 1.4f) a = 1.4f;
            else if (a < -1.4f) a = -1.4f;
            b = -a * 1.2f;
            if (b > 1.3155f) b = 1.3155f;
            else if (b < -1.3155f) b = -1.3155f;
            r = D_L00_0017A780;
            *(float *)(r + 0xCC8) = b;
            k = 0.1f;
            k = func_001F9B88(a) * k + k;
            if (k > 0.07f) k = 0.07f;
            *(float *)(r + 0xCF8) = k * inv;
        }
    }
}
