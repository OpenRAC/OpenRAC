/* NON_MATCHING func_L00_002465F8 -- src/overlays/shared/map_002465F8.c
 * Best so far: SIZE ours 2196 / retail 2248, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Level entry (2248 bytes): clears D_L00_00161F00..D_L00_001660B0 (aligned: one func_001F99D8 call, else a debug
 *   Left: retail keeps symbol high halves in saved registers (lui $s1,0x1C for D_L00_001BA070, $s5/$s6 bases) and 
 *   Unblock: a candidate that keeps the high halves in pointer locals matched to retail's saved registers (regallo
 */
extern char D_L00_001660B0[];
extern char D_L00_00161F00[];
extern char D_L00_0015FD70[];
extern char D_L00_0015FD78[];
extern char D_0014171B[] NOT_SDA;
extern unsigned char D_0013DE55[] NOT_SDA;
extern unsigned char D_0013DE4B[] NOT_SDA;
extern unsigned char D_0013D355[] NOT_SDA;
extern char D_0013E633[] NOT_SDA;
extern char D_L00_0016C158[];
extern char D_L00_001BA070[];
extern char D_L00_0016C9B0[];
extern int D_0015EE80 MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern int D_0015EFA4 MACRO_ADDR;
extern int D_L00_0015F6A8 MACRO_ADDR;
extern int D_L00_0015F6AC MACRO_ADDR;
extern int D_L00_0015F6BC MACRO_ADDR;
extern int D_L00_0015F4F8 MACRO_ADDR;
extern int D_L00_0015F650 MACRO_ADDR;
extern int D_L00_0015F690 MACRO_ADDR;
extern float D_L00_0015F6B4 MACRO_ADDR;
extern int D_L00_0015F690_n __asm__("D_L00_0015F690") NOT_SDA;
extern long D_0015EE40 MACRO_ADDR;
extern void func_001F99D8(void *, int);
extern int func_001E9730();
extern void func_00234018(void);
extern void func_L00_0023D9C0(void);
extern void func_L00_00244AE0(int, int);
extern void func_001F0F30(void);
extern void func_L00_00246EC0(void);
extern void func_001FF6B8(void);
extern void func_L00_00276F18(void);
extern void func_L00_002901A8(void);
extern void func_00122598(int);
extern void func_001FB470(void);
extern void func_00216A90(int, int, int);
extern void func_001E9768(void *, int);
extern void func_001E9768_v() __asm__("func_001E9768");
extern void func_00216270(void);
extern void func_L00_001F4918(void);
extern void func_L00_001F5C60(void);
extern void func_L00_00299250(void);
extern void func_L00_001F91B0(void);
extern void func_L00_0029AD18(void);
extern void func_L00_0029A300(void);
extern void func_001F45F0(void);
extern void func_L00_00277A88(void);
extern void func_L00_001F92C0(void);
extern void func_001FD3E8(void);
extern void func_L00_001F9248(void);
extern void func_L00_0029D988(void);
extern void func_L00_002A1540(void);
extern void func_00230A90(void);
extern void func_00232200(void);
extern void func_L00_0029AFB8(void);
extern void func_L00_001F92E8(void);
extern void func_L00_001F70F0(void);
extern void func_002349B8(void);
extern void func_00234948(void);
extern void func_001FB598(void);
extern void func_001FB848(void);
extern void func_001FB498(void);
extern int func_0012DDC0(void);
extern void func_00234AC8(int);
extern void func_002348E8(void);
extern void func_0012EC30(void);
extern void func_0012EC40(void);
extern void func_0022EFE8(void);
extern void func_00216D88(void);
extern void func_00209E68(void);
extern void func_00209070(void);
extern void func_001F4630(int);
extern void func_00234C98(int, long);
extern int func_00200198(int, int);
extern void func_00200468(int, int, int, int, int, int);
extern long func_00200248(int);
extern void func_00200E38_f(float, float, int, int, int, float, float, float) __asm__("func_00200E38");
extern void func_001F4748(void);
extern void func_L00_001F3A78(void);
extern float func_001FA888(int);
extern int func_001F9850(int);
extern void func_001F3D00(void);
extern void func_00228110(void);
extern void func_00233308(void);
extern void func_00216F28(void);
extern void func_00216EF0(int);

typedef struct {
    unsigned short a;
    unsigned short b;
    unsigned int c;
} Ent;

/* Level entry: clears the level buffer, sets up the level state, then runs the main loop that dispatches on it. */
void func_L00_002465F8(void) {
    int size = D_L00_001660B0 - D_L00_00161F00;
    int i;
    char *q;
    char *B;
    Ent *arr;
    int v;
    int sel;
    int thr;
    int hw;
    int w;
    int k;
    long tex;
    int hwv;
    int one;
    int d;
    int a18 = 0;
    int a19 = 0;
    float f0;
    float f1;
    float f20;
    float f21;

    if (!((size & 0xF) != 0 || ((unsigned int)D_L00_00161F00 & 0xF) != 0)) {
        func_001F99D8(D_L00_00161F00, size);
    } else {
        func_001E9730(D_L00_0015FD70);
        {
            char *dst = D_L00_00161F00;
            int n = D_L00_001660B0 - D_L00_00161F00;
            for (i = 0; i < n; i++) dst[i] = 0;
        }
    }

    func_00234018();
    D_L00_0015F6AC = 0;
    D_L00_0015F6A8 = 6;
    one = 1;
    func_L00_0023D9C0();
    *(short *)(D_0014171B + 0x100B5 + 0x38) = 0;
    func_L00_00244AE0(1, 0);
    q = D_L00_0016C158;
    *(int *)(q + 0x8) = 15;
    *(int *)(q + 0x30) = D_0015EE80;
    *(int *)(q + 0x38) = -1;
    *(int *)(q + 0xF4) = one;
    *(int *)(q + 0xC) = one;
    func_001F0F30();
    func_L00_00246EC0();
    func_001FF6B8();
    func_L00_00276F18();
    func_L00_002901A8();
    D_L00_0015F4F8 = 0;
    func_00122598(0);
    func_001FB470();
    func_00216A90(*(short *)(D_0014171B + 0x100B5 + 0x38), 1, 0x400);

    v = D_0015EE84;
    if ((v == one && D_0013DE4B[0] == 0) || (v == 0 && D_0013D355[0x143] != 0)) {
        func_00216F28();
    } else {
        func_00216EF0(0);
    }
    v = D_0015EE84;
    q = D_0013DE55 + 0xB + v;
    if (*(unsigned char *)q == 0) {
        *(unsigned char *)q = 1;
    }
    v = D_0015EE84;
    arr = (Ent *)(D_0014171B + 0x65);
    if (arr[v].a <= 0xFFFE) {
        arr[v].a = arr[v].a + 1;
    }
    d = 600;
    if (arr[D_0015EE84].b < func_001F9850(D_0015EFA4) / d) {
        arr[D_0015EE84].b = func_001F9850(D_0015EFA4) / d;
    }
    arr[v].c = (arr[v].c | (1u << v)) | 0x80000000u;
    func_0012EC30();

    while (D_L00_0015F650 == 0) {
        a18 = 0;
        a19 = 0;
        D_0015EE40 += (unsigned int)*(volatile unsigned int *)0x10000800;
        *(volatile int *)0x10000800 = 0;
        func_L00_001F70F0();
        D_L00_0015F6BC = 0;
        func_002349B8();
        func_00234948();
        func_001FB598();
        func_001FB848();
        func_001FB498();
        func_0012DDC0();

        v = D_L00_0015F6A8;
        switch (v) {
        case -1:
            func_00216270();
            func_L00_001F4918();
            func_L00_001F5C60();
            break;
        case 0:
            func_00216270();
            func_L00_00299250();
            func_L00_001F91B0();
            break;
        case 1:
            func_L00_0029AD18();
            break;
        case 2:
            func_00216270();
            func_L00_0029A300();
            func_001F45F0();
            break;
        case 3:
            func_00216270();
            func_L00_00277A88();
            func_L00_001F92C0();
            break;
        case 4:
            func_00216270();
            func_001FD3E8();
            func_L00_001F9248();
            break;
        case 5:
            func_00216270();
            func_L00_0029D988();
            func_L00_002A1540();
            break;
        case 6:
            func_00216270();
            func_00230A90();
            func_00232200();
            break;
        case 7:
            func_00216270();
            func_L00_0029AFB8();
            func_L00_001F92E8();
            break;
        }
        if (D_L00_0015F6A8 == v) {
            D_L00_0015F6AC = D_L00_0015F6AC + 1;
        } else {
            D_L00_0015F6AC = 0;
        }

        if (D_L00_0015F6BC != 0) {
            func_00234AC8(1);
            func_002348E8();
            continue;
        }

        func_00209E68();
        func_00209070();
        sel = *(int *)(D_0013D355 + 0x117);
            if ((unsigned int)(sel - 3) < 4 || sel == 9 || sel == 10 || sel == 11 || sel == 12
            || sel == 15 || sel == 16 || sel == 17 || sel == 18 || sel == 19 || sel == 20) {
            if (*(int *)(D_L00_001BA070 + 0x13C) == 0) {
                k = 55;
                func_001F4630(0);
                f21 = 272.0f;
                func_00234C98(0x47, 0x3004B);
                w = func_00200198(0x755D, 0);
                func_00200468(w, 0x32, 0x12C, 0x40, 0x40, 0x80);
                f1 = (float)k;
                f20 = (float)(D_L00_0015F4F8 % k) * -6.2831855f / f1;
                w = func_00200198(0x755D, 1);
                tex = func_00200248(w);
                func_00200E38_f(1312.0f, 5324.0f, 0x40, 0x40, tex, f21, f21, f20);
                func_001F4748();
            }
        }
        *(int *)(D_L00_001BA070 + 0x13C) = 0;
        func_L00_001F3A78();

        q = D_L00_0015FD78;
        func_001E9768(q, 0x10);
        func_001E9768(q, 0x10);
        func_00234AC8(1);
        hwv = *(volatile int *)0x10000800;
        f1 = func_001FA888(hwv);
        f0 = 9600.0f;
        if (D_0015EE80 != 0) {
            f0 = 11328.0f;
        }
        D_L00_0015F6B4 = f1 / f0;
        if (D_L00_0015F6A8 == 0 && *(int *)(D_L00_0016C158 + 0x18) != 0) {
            func_001E9768_v();
        }

        B = D_0013E633 + 0xE1D;
        if (*(unsigned char *)(B + 0x20B1) == 0) {
            v = D_L00_0015F6A8;
            if (v == 0 || v == 2) {
                hw = *(volatile int *)0x10000800;
                thr = 9600;
                if (D_0015EE80 != 0) {
                    thr = 11520;
                }
                if (thr < hw) {
                    if (v == 0) {
                        func_00216270();
                        func_L00_00299250();
                    } else if (*(short *)D_L00_0016C9B0 == 0) {
                        func_00216270();
                        func_L00_0029A300();
                    }
                }
                func_00122598(0);
                D_L00_0015F4F8 = D_L00_0015F4F8 + 1;
                func_00228110();
                D_L00_0015F690 = 0;
            } else {
                func_00122598(0);
                D_L00_0015F4F8 = D_L00_0015F4F8 + 1;
                func_00228110();
                D_L00_0015F690_n = 0;
            }
        }

        func_001F3D00();
        if (D_L00_0015F6A8 == 0 && *(unsigned char *)(B + 0x20B1) != 0) {
            a18 = 1;
            a19 = 1;
        }
        if (a18 != 0) {
            func_002348E8();
            func_0012EC40();
            func_0012DDC0();
            func_0022EFE8();
            func_00216D88();
            func_L00_00244AE0(0, a19);
                    D_L00_0015F6A8 = 0;
            func_00216A90(*(short *)(D_0014171B + 0x100B5 + 0x38), 1, 0x400);
            func_001FF6B8();
        }

        arr = (Ent *)(D_0014171B + 0x65);
        d = 600;
        if (arr[D_0015EE84].b < func_001F9850(D_0015EFA4) / d) {
            arr[D_0015EE84].b = func_001F9850(D_0015EFA4) / d;
        }
    }
    func_00233308();
}
