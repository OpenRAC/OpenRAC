/* NON_MATCHING func_L00_002A7E10 -- src/overlays/shared/vendor_002A5138.c
 * Best so far: SIZE ours 624 / retail 632, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   CollectBolt: adds a bolt's value (5/1/20/50 by moby kind at +0xA6, else the a1 register) to the save-slot tabl
 *   Open differences: (1) retail reads D_0015EE98/D_0015EF2C via $gp in the branch delay slots but reloads D_0015E
 *   Unblock: the original declarations of D_0015EE98/D_0015EF24 (probably fields of a struct at 0x15EE84) and the 
 */
extern int func_001FFB38(int, int, int, int, int, int, int);
extern void func_L00_00258DB0(float *, float, float);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_002140B0(int);
extern float func_002140F8(float, float);
extern int func_001F9850(int);
extern void func_L00_00272F00(void *, int, int, int, int, void *, float, float, float);
extern void func_L00_0023A658(void);
extern void func_L00_0023A690(void);
extern void func_L00_0023A788(void);
extern short D_0015EE98_s __asm__("D_0015EE98");
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
#define BOLTS (*(int *)((char *)&D_0015EE84_m + 0x14))
extern float D_0015EE6C MACRO_ADDR;
extern short D_0015EF2C;
extern int D_0015EF24 MACRO_ADDR;
extern int D_L00_0015F678;
extern short D_L00_0016009C;
extern char D_0013DE6E[];
extern unsigned char D_0014171B[];

struct BoltExtra {
    char pad[0xB1];
    unsigned char idx;
};
struct BoltData {
    char pad[0x40];
    unsigned char flag;
};
struct BoltMoby {
    char pad0[0x10];
    float pos[4];
    char pad1[0x58];
    struct BoltData *data;
    char pad2[0x2A];
    short kind;
    char pad3[0x10];
    struct BoltExtra *extra;
};

/* adds a bolt's value to the counters and spawns the collect sparkle */
void func_L00_002A7E10(struct BoltMoby *m, int amt) {
    float a[4];
    float b[4];
    int n, k;
    float f;
    int v = m->kind;
    struct BoltExtra *p;

    if (v == 14) {
        amt = 5;
    } else if (v < 15) {
        amt = 1;
    } else {
        switch (v) {
        case 15:
            amt = 0x14;
            break;
        case 16:
            amt = 0x32;
            break;
        }
    }
    p = m->extra;
    if (p != 0 && !(p->idx & 0x80)) {
        *(unsigned short *)(D_0014171B + 0xBF75 + 2 + p->idx * 4 + D_0015EE84_m * 256) += amt;
    }
    BOLTS = *(int *)&D_0015EE98_s + amt;
    if (m->data->flag != 0 ||
        (D_L00_0015F678 != 0 && (unsigned)m < *(unsigned *)&D_L00_0016009C)) {
        *(int *)((char *)&D_0015EF24 + 8) = *(int *)&D_0015EF2C + amt;
    }
    ((int *)(D_0013DE6E + 0x1D2))[D_0015EE84_m] += amt;
    func_001FFB38(2, 0x754E, (int)func_L00_0023A658, (int)func_L00_0023A690, (int)func_L00_0023A788, (int)&BOLTS, 0x98967F);
    func_L00_00258DB0(a, D_0015EE6C * 0.7f, D_0015EE6C);
    func_001F9C30(b, a, 10.0f);
    func_001F9BD8(b, b, m->pos);
    n = func_002140B0(2);
    if (n == 0) n = n - 1;
    k = n;
    f = func_002140F8(0.4f, 0.5f);
    func_L00_00272F00(b, func_001F9850(0x19), 0x7F207F7F, 0, k, a, f * 0.2f, f, 0.0f);
    func_L00_00272F00(b, func_001F9850(0x19), 0x7F7F7F7F, 1, -k, a, f * 0.14f, f * 0.7f, 0.0f);
}
