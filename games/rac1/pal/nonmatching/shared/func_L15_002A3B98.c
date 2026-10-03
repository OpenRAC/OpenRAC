/* NON_MATCHING func_L15_002A3B98 -- src/overlays/shared/vendor_00298BB8.c
 * Best so far: SIZE ours 556 / retail 568, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Per-moby draw loop: builds a view matrix B (orient to camera at D_L15_00167300+0x140), then for 4 rows copies 
 *   Best p3.c (SIZE 556 vs 568): prologue, GIF-style sd stores (use long, not long long), first copy loop and the 
 *   Unblock: a wording that stops gcc reversing the inner loop (loop biv kept) -- not found in 6 runs.
 */
typedef int u128_2A3B98 __attribute__((mode(TI)));
extern int func_001F4868(int);
extern void func_001F9BC0(float *);
extern float func_L00_001FF860(float, float);
extern void func_001FA218(float *, float *);
extern void func_L00_001FD1D8(void *, void *, int);
extern float D_L15_001CE490[];
extern float D_L15_001CE390[];
extern int D_L15_001AC140[];
extern char D_L15_00167300[];
extern int D_L15_00160058_m __asm__("D_L15_00160058") MACRO_ADDR;
extern short D_L15_00161544;

// Draws the model matrices for each moby in the room's list, oriented toward the camera.
void func_L15_002A3B98(char *m) {
    float mtx[16];
    int cols[4];
    float t[8];
    long g[4];
    float A[4];
    float B[16];
    short *p;
    char *c;
    float *w;
    int i, j, k;
    g[1] = func_001F4868(8);
    g[2] = 0xFF9000000260L;
    g[3] = 0x8000000044L;
    g[0] = 0;
    for (i = 0; i < 4; i++) {
        t[i * 2] = D_L15_001CE490[i * 2];
        t[i * 2 + 1] = D_L15_001CE490[i * 2 + 1];
    }
    p = (short *)D_L15_001AC140[*(unsigned char *)(m + 0x21)];
    c = D_L15_00167300;
    w = B + 12;
    do {
        char *o = (char *)D_L15_00160058_m + ((*(unsigned short *)p & 0x7FFF) << 8);
        if (*(unsigned char *)(o + 0x20) != 1) {
            char *pos = o + 0x10;
            float d;
            func_001F9BC0(A);
            A[2] = func_L00_001FF860(*(float *)(c + 0x140) - *(float *)(o + 0x10), *(float *)(c + 0x144) - *(float *)(o + 0x14));
            d = func_001F9D48(pos, c + 0x140);
            A[1] = -func_L00_001FF860(d, *(float *)(c + 0x148) - *(float *)(o + 0x18));
            func_001FA218(B, A);
            qcopy(w, pos);
            B[14] += *(float *)&D_L15_00161544;
            B[15] = 1.0f;
            for (j = 0; j < 4; j++) {
                k = 0;
                do {
                    cols[k] = *(int *)(&D_L15_00161544 + 2 + j * 2);
                    *(u128_2A3B98 *)(mtx + k * 4) = *(u128_2A3B98 *)(D_L15_001CE390 + j * 16 + k * 4);
                    k++;
                } while (k < 4);
                func_L00_001FD1D8(mtx, B, 0);
            }
        }
    } while (*p++ >= 0);
}
