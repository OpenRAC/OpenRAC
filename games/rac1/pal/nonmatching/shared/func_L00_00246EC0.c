/* NON_MATCHING func_L00_00246EC0 -- src/overlays/shared/map_002465F8.c
 * Best so far: SIZE ours 1028 / retail 1040, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_00246EC0: map setup. Fills tables from D_L00_00183FF0/00182910, picks a level entry from D_00137C80, 
 *   Best: p6.c (SIZE 1028 vs 1040, 9 of 10 runs used). Structure and all instructions are right; differences are r
 *   (ours data=$17,b=$18,lo=$19,hi=$20,g=$21), loop counters $6/$3 in retail vs $7 in ours, and retail hoists li 0
 *   Unblock: find the source spelling that changes pseudo creation order (struct-typed D_L00_001842F0 with named f
 */
extern void func_002176C8(void *, int, int);
int func_00234158(int arg0, int arg1, int arg2, int arg3);
extern int func_0020C468_2(int, int) __asm__("func_0020C468");
extern char *func_001FFAB8_d(int, int, char *, int) __asm__("func_001FFAB8");
extern void func_001F9A98(void *, void *, int);
extern void func_002083E0(void *arg0, unsigned char *arg1, int arg2);
extern void func_00206F40(int *dst, unsigned char *src, short *offs);
extern int D_L00_001842F0[];
extern int D_L00_00183FF0[];
extern int D_0015EE84_far __asm__("D_0015EE84") NOT_SDA;
extern float D_L00_00182910[];
extern float D_L00_00182B70[];
extern char D_L00_0015FEC0[];
extern int *D_L00_00173F08;
extern char D_L00_001E8E80[];
extern unsigned char D_L00_00184494[];
extern unsigned char D_0013D50F[] NOT_SDA;
extern char D_00137C80[] NOT_SDA;
extern unsigned char D_0014171B[] NOT_SDA;

/* Sets up the map's tables, loads its data from disc and builds the circular mask. */
void func_L00_00246EC0(void) {
    int i, j;
    int *gl;
    int *q;
    int v;
    int *e;
    int *data;
    int n, b, len;
    unsigned char *pp;

    D_L00_001842F0[4] = 1;
    for (i = 0; i < 20; i++) {
        D_L00_001842F0[0x41 + i] = D_L00_00183FF0[i * 2];
        D_L00_001842F0[0x55 + i] = D_L00_00183FF0[i * 2 + 1];
        ((float *)D_L00_001842F0)[0x2D + i] = 0.65f;
    }
    v = D_0015EE84_far;
    D_L00_001842F0[0x8B] = -1;
    D_L00_001842F0[0x89] = v > 18 ? 0 : v;
    for (i = 0; i < 19; i++) {
        float a = D_L00_00182910[i * 8], b = D_L00_00182910[i * 8 + 1], c = D_L00_00182910[i * 8 + 2], dd = D_L00_00182910[i * 8 + 3];
        float ee = D_L00_00182910[i * 8 + 4], f = D_L00_00182910[i * 8 + 5], gg = D_L00_00182910[i * 8 + 6], h = D_L00_00182910[i * 8 + 7];
        float p = (ee - gg) / (a - c);
        float r = (f - h) / (b - dd);
        D_L00_00182B70[i * 4 + 1] = p;
        D_L00_00182B70[i * 4 + 3] = r;
        D_L00_00182B70[i * 4] = ee - p * a;
        D_L00_00182B70[i * 4 + 2] = f - r * b;
    }
    pp = (unsigned char *)D_0013D50F + 0xB9;
    if (pp[0x21] != 0) {
        D_L00_001842F0[0x8C] = 1;
        e = (int *)(D_00137C80 + 0x8B8) + D_L00_001842F0[0x89] * 2;
    } else {
        D_L00_001842F0[0x8C] = 0;
        e = (int *)(D_00137C80 + 0x820) + D_L00_001842F0[0x89] * 2;
    }
    if (e[1] != 0) {
        b = e[1];
        data = D_L00_00173F08;
        gl = D_L00_001842F0;
        gl[10] = 1;
        func_002176C8((char *)data + 0x9A800, e[0], b);
        n = b << 7;
        gl[0x8B] = func_00234158((int)((char *)data + 0x9A800), n, 0x4800, (int)D_L00_001E8E80);
        func_0020C468_2((int)((char *)data + 0x9A800), (int)data);
        gl[0x8D] = n;
        len = data[1] - data[0];
        gl[0] = (int)func_001FFAB8_d(len, 0, D_L00_0015FEC0, 0x2DB);
        func_001F9A98((void *)gl[0], (char *)data + data[0], len);
        gl[0] = gl[0] + 8;
        gl[1] = gl[0];
        len = data[2] - data[1];
        gl[5] = (int)func_001FFAB8_d(len, 0, D_L00_0015FEC0, 0x2EC);
        func_001F9A98((void *)gl[5], (char *)data + data[1], len);
        gl[2] = (int)func_001FFAB8_d(0x1000, 0, D_L00_0015FEC0, 0x2F3);
        for (i = 0; i < 8; i++) {
            gl[12 + i * 4] = -1;
            gl[13 + i * 4] = -1;
            gl[14 + i * 4] = gl[2] + (i << 9);
        }
        D_L00_001842F0[3] = (int)func_001FFAB8_d(0x8000, 0, D_L00_0015FEC0, 0x2FC);
        q = (int *)(D_0014171B + 0x8A5 + D_L00_001842F0[0x89] * 0x800);
        if (*(unsigned char *)q != 0) {
            func_002083E0((void *)D_L00_001842F0[3], (unsigned char *)q, D_L00_001842F0[5]);
        } else {
            func_00206F40((int *)D_L00_001842F0[3], (unsigned char *)D_L00_001842F0[0], (short *)D_L00_001842F0[1]);
        }
        for (i = 0; i < 32; i++) {
            float di = 15.5f - (float)i;
            float di2 = di * di;
            for (j = 0; j < 32; j++) {
                float dj = 15.5f - (float)j;
                if (dj * dj + di2 > 256.0f) {
                    D_L00_00184494[i * 4 + j / 8] |= 1 << (j % 8);
                }
            }
        }
    } else {
        D_L00_001842F0[0] = 0;
        D_L00_001842F0[10] = 0;
        D_L00_001842F0[2] = 0;
        D_L00_001842F0[3] = 0;
        D_L00_001842F0[1] = 0;
    }
}
