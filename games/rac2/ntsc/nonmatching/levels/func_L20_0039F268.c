/* Readable C reconstruction, measured2026-10-23.
 * Profile: -O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier -frac2-zero-ti-por.
 * us: 0/1, us2: 0/1, pal: 0/1.
 * 728 versus756 bytes; local frame120 versus130, history traversal allocation and GP/data forms differ.
 * Tried: Quadword/float union and integer alpha result; explicit assembler G8 did not help.
 * Reopen: A verified source buffer layout or register-lifetime explanation; do not add padding to force the frame.
 * Historical notes below are preserved; their copy-wall claims are superseded
 * by the current qualified copy profiles. No handwritten-assembly inference.
 */

typedef int Quadword __attribute__((mode(TI)));
typedef union { Quadword q; float f[4]; } Vector;
extern void func_00282CC0(void *, const void *, const void *);
extern void func_00282CF0(void *, const void *, const void *);
extern float func_00283CE0(int);
extern int func_00283CF0(float);
extern void func_L00_002EBE88(void *, int, int, int, unsigned int, unsigned int, float, float, float);
extern short D_L20_001B38B0[10];
extern short D_L20_001B38C8[10];
extern short D_L20_001B38E0[10];
extern float D_L20_001B3904 __attribute__((nosda));
extern Vector D_L20_00235580[10][10];
extern float D_L20_001AED84;
extern unsigned int D_L20_001AED18;
extern unsigned int D_L20_001AED1C;

void func_L20_0039F268(unsigned char *object) {
    Vector offset;
    Vector origin;
    Vector differences[9];
    unsigned char *state = *(unsigned char **)(object + 0x68);
    int i, j, alphaValue;
    float numerator, denominator, alpha;
    unsigned int colour;
    offset.q = 0;
    offset.f[2] = 0.75f;
    func_00282CC0(&origin, object + 0x10, &offset);
    for (i = 0; i < 10; ++i) {
        if (D_L20_001B38B0[i]) {
            for (j = 0; j < 9; ++j)
                func_00282CF0(&differences[j], &D_L20_00235580[i][j + 1], &D_L20_00235580[i][j]);
            D_L20_00235580[i][0].q = origin.q;
            D_L20_00235580[i][0].f[3] = 0.04f;
            for (j = 0; j < 9; ++j) {
                func_00282CC0(&D_L20_00235580[i][j + 1], &D_L20_00235580[i][j], &differences[j]);
                D_L20_00235580[i][j + 1].f[3] = 0.04f;
            }
            if (D_L20_001B38B0[i] > 0) {
                numerator = func_00283CE0(D_L20_001B38B0[i]);
                denominator = func_00283CE0(D_L20_001B38E0[i]);
                D_L20_001B38C8[i] = func_00283CF0(D_L20_001AED84 * *(float *)(state + 0x14) * (numerator / denominator));
            } else D_L20_001B38C8[i] = 0;
        }
    }
    i = 9;
    do {
        j = 9 - i;
        if (D_L20_001B38B0[j]) {
            alpha = func_00283CE0(D_L20_001B38C8[j]);
            alphaValue = func_00283CF0(alpha * D_L20_001B3904);
            colour = D_L20_001AED18;
            if (*(short *)(object + 0xAA) == 0x1192) colour = D_L20_001AED1C;
            func_L00_002EBE88(D_L20_00235580[j], 23, 14, 10,
                             ((unsigned int)alphaValue << 24) | 0x7F7F7F,
                             ((unsigned int)alphaValue << 24) | colour, 0.0f, 4.0f, 0.0f);
        }
        --i;
    } while (i >= 0);
}
