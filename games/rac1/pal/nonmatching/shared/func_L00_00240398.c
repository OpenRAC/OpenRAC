/* NON_MATCHING func_L00_00240398 -- src/overlays/shared/loaders_00240398.c
 * Best so far: SIZE ours 1036 / retail 1040, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_00240398: reads a level's four packed GS images through func_00122630/func_00122958 (4-way if chain i
 *   Best is p5.c (size 1020 vs retail 1040; structure, regs of the loop, stack slots 0x60/0x64 and the struct-type
 *   Differences: (1) in the i==2 arm our delay slot takes `lw $5,D_0015EF8C` (gp form) where retail has `move $4,$
 */
typedef struct {
    char pad[0x12CC];
    struct {
        int a;
        int b;
    } e[1];
} G137C80;
extern G137C80 D_00137C80_c __asm__("D_00137C80");
extern char D_0013DE6E[];
extern char *D_0015EF54 MACRO_ADDR;
extern int D_0015EF8C MACRO_ADDR;
extern char D_L00_001AEF40[];
extern int D_L00_00160088 MACRO_ADDR;
extern short D_L00_0015FCC0;
extern short D_L00_0015FCD0;
extern char D_L00_0015FCE0;
extern short D_L00_0015FCCA MACRO_ADDR;
extern short D_L00_0015FCCC MACRO_ADDR;
extern short D_L00_0015FCCE MACRO_ADDR;
extern short D_L00_0015FCDA MACRO_ADDR;
extern unsigned char D_L00_00197F40[];
extern int D_L00_00199E00[];
extern int D_L00_00160608[];
extern int D_L00_00160618[];
extern int D_L00_00160628 MACRO_ADDR;
extern int D_L00_00160624 MACRO_ADDR;
extern int D_L00_0016062C MACRO_ADDR;
extern void func_001F9A98(void *, void *, int);
extern int func_00122630(void *, short, short, short, short, short, short, short);
extern void func_00118D80(int);
extern int func_00122958(void *, void *);
extern int func_00120858(int, unsigned short);
extern void func_00203E78(void *, void *, void *);

/* Loads a level's four packed images through the GS loader and registers the image records and their data blocks. */
char *func_L00_00240398(char *buf, int *tbl) {
    int img[24];
    int i = 0;
    int a;
    int off;
    int b;
    int c;
    char *p;
    char *src;
    char *dst;
    char *base;
    char *g2 = D_0013DE6E + 0x2C2;
    int idx;
    func_001F9A98(buf, D_0015EF54, D_00137C80_c.e[*(short *)(g2 + 0x26) + 1].a << 11);
    a = *(int *)buf;
    off = *(int *)(buf + 4);
    p = buf + off;
    b = *(int *)(buf + 8);
    c = *(int *)(buf + 0xC);
    do {
        if (i == 0) {
            func_00122630(img, (D_0015EF8C + tbl[0x2B]) >> 8, 1, 0x13, 0, 0, 0x40, 0x40);
            src = buf + off;
            dst = p + 0x14430;
        } else if (i == 1) {
            func_00122630(img, (D_0015EF8C + tbl[0x2C]) >> 8, 1, 0x13, 0, 0, 0x20, 0x20);
            src = buf + off;
            dst = p + 0x15430;
        } else if (i == 2) {
            func_00122630(img, (D_0015EF8C + tbl[0x2D]) >> 8, 1, 0, 0, 0, 0x10, 0x10);
            dst = p + 0x30;
            src = buf + off;
        } else {
            func_00122630(img, (D_0015EF8C + tbl[0x2E]) >> 8, 1, 0, 0, 0, 0x10, 0x10);
            src = buf + off;
            dst = buf + c + 0x30;
        }
        func_00118D80(0);
        i++;
        func_00122958(img, dst);
        func_00120858(0, 0);
    } while (i < 4);
    base = buf;
    buf = src;
    D_L00_0015FCCA = tbl[0x2D] >> 8;
    idx = D_L00_00160088;
    D_L00_0015FCCC = tbl[0x2B] >> 8;
    D_L00_0015FCCE = tbl[0x2C] >> 8;
    qcopy(D_L00_001AEF40 + idx * 16, &D_L00_0015FCC0);
    D_L00_00199E00[D_L00_00160088] = (int)buf + 0x80000000;
    func_001F9A98(buf, base + off + 0x430, 0x14000);
    buf += 0x14000;
    D_L00_0015FCE0 = *(unsigned char *)&D_L00_00160088;
    if (D_L00_00197F40[D_L00_00160608[*(short *)(g2 + 0x26)]] == 0xFF) {
        func_00203E78(base + a, (char *)&D_L00_0015FCC0 - D_L00_00160088 * 16, &D_L00_0015FCE0);
    }
    idx = D_L00_00160088 + 1;
    D_L00_0015FCDA = tbl[0x2E] >> 8;
    D_L00_00160088 = idx;
    qcopy(D_L00_001AEF40 + idx * 16, &D_L00_0015FCD0);
    D_L00_00199E00[idx] = (int)buf + 0x70000000;
    func_001F9A98(buf, base + c + 0x430, 0x4000);
    buf += 0x4000;
    D_L00_0015FCE0 = *(unsigned char *)&D_L00_00160088;
    if (D_L00_00197F40[D_L00_00160618[*(short *)(g2 + 0x26)]] == 0xFF) {
        func_00203E78(base + b, (char *)&D_L00_0015FCD0 - D_L00_00160088 * 16, &D_L00_0015FCE0);
    }
    D_L00_00160628 = 1;
    D_L00_00160624 = 1;
    D_L00_00160088 = D_L00_00160088 + 1;
    D_L00_0016062C = 0;
    return buf;
}
