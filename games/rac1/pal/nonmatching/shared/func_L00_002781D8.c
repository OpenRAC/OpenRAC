/* NON_MATCHING func_L00_002781D8 -- src/overlays/shared/pause_00277208.c
 * Best so far: BYTES 64/1772 (96.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Stop: best p7 = BYTES 64/1772 (p9, p11 identical).
 *   Summary: draws pause-menu pages (rects via func_00238D90, then per-page texture draw in 2 passes).
 *   Key matches found: block-local 'char *p = D' per use; separate index var per loop (j in loop3 -> gcse PRE T=j+
 *   'int ww = w' before the pp check and 'col = gp value' before the d stores (sched1 is single-block only); natur
 *   Left (~45 words): loop1 giv regs s2/s3 swapped; func_00238D90/func_001F7A50 set \$5 before \$4 in retail; loop
 *   reload-register round-robin (choose_reload_regs) out of phase at several spots (T_pass, pp reloads, 0x80/1 con
 *   Unblock: reproduce retail's exact reload sequence (count of reloads before loop1 differs by one; something bet
 *   Dumps (TRY_CFLAGS=-dLSlgRG) are in opus/dumps/ and *_f.txt extracts.
 */
typedef int u128 __attribute__((mode(TI)));
extern void func_001FB530(void);
extern void func_L00_002A2258(int a, int b, int n);
extern void func_001F3C10(void);
extern void func_001F55C0(int, int, int, int);
extern void func_00234C98(int, long);
extern void func_0020E0C8(void);
extern void func_0020E040(void);
extern void func_0020E068(void);
extern void func_0020E180(int, int);
extern void func_00238D90(void *, void *, int *, int *, int *, int *);
extern void func_00201640(int, int, int, int, long, long);
extern void func_001F7A50(int a, int b, int flag, float f);
extern void func_002017C8(int arg0, int arg1, int arg2, int arg3, long arg4, int arg5, int arg6);
extern void func_001F7B40(void);
extern void func_001F5800(int, int, int, int, int, int, int, int, long, long);
extern void func_0020E098(void);
extern void func_0020E200(void);
extern void func_00234AC8(int);
extern void func_0020DD48(void);
extern void func_001F4630(int);
extern void func_002250B8(int);
extern void func_001F4748(void);
extern char D_L00_001BA070[] NOT_SDA;
extern int D_0015EF84 MACRO_ADDR;
extern int D_0013E15A[];
extern int D_L00_001BA220[];
extern int D_L00_001B2740[];
extern long D_0015EFD0 MACRO_ADDR;
extern short D_L00_00160330;

/* draws the pause menu pages: lays out each page moby's screen rect, then renders each page's texture */
void func_L00_002781D8(int arg0) {
    char *pm = D_L00_001BA070;
    u128 c[4];
    u128 pad[5];
    u128 a, b;
    int w, h, x, y;
    char **pp;
    int i;
    int k;
    int j;
    int n;
    int pass;

    func_001FB530();
    pp = 0;
    func_L00_002A2258(*(int *)(pm + 0x10), D_0015EF84, *(int *)((char *)D_0013E15A + 0x4AA) << 11);
    if (arg0 != 0) {
        return;
    }
    func_001F3C10();
    func_001F55C0(0, 0, 0, 0x30);
    if (*(int *)D_L00_001BA070 == 0x14) {
        return;
    }
    func_00234C98(0x47, 0x5360B);
    func_0020E0C8();
    func_0020E040();
    func_0020E068();
    for (i = 0; i < 14; i++) {
        if (D_L00_001B2740[i] != 0 && D_L00_001BA220[i] != 0 && (i != 6 || *(int *)(pm + 0xD8) != 0)) {
            func_0020E180(D_L00_001BA220[i], 1);
        }
    }
    {
        char *p = D_L00_001BA070;
        if (*(char **)(p + 4) != 0) {
            pp = (char **)(*(char **)(p + 4) + 0x44);
        }
    }
    for (k = 0; k < 14; k++) {
        char *d;
        if (D_L00_001BA220[k] == 0 || D_L00_001B2740[k] == 0) {
            continue;
        }
        if (k == 6) {
            char *p = D_L00_001BA070;
            if (*(int *)(p + 0xD8) == 0) {
                continue;
            }
        }
        d = *(char **)(D_L00_001BA220[k] + 0x78);
        qcopy(&c[0], d);
        qcopy(&c[1], d + 0x10);
        qcopy(&c[2], d + 0x20);
        qcopy(&c[3], d + 0x30);
        a = c[0];
        b = c[3];
        func_00238D90(&a, &b, &w, &h, &x, &y);
        {
            int ww = w;
            int col;
            x++;
            y++;
            if (pp != 0 && pp[k] != 0) {
                *(int *)(pp[k] + 0x20) = ww;
                *(int *)(pp[k] + 0x24) = h;
                *(int *)(pp[k] + 0x18) = x;
                *(int *)(pp[k] + 0x1C) = y;
            }
            col = *(int *)&D_L00_00160330;
            *(int *)(d + 0x50) = x;
            *(int *)(d + 0x54) = y;
            *(int *)(d + 0x58) = w;
            *(int *)(d + 0x5C) = h;
            func_00201640(x + 1, y + 1, x + w - 1, y + h - 1, col, 0);
        }
    }
    for (pass = 0; pass < 2; pass++) {
        for (j = 0; j < 14; j++) {
            char *m = (char *)D_L00_001BA220[j];
            int flags;
            int pw, ph, px, py;
            int tw, th, su, sv;
            int r, u, v, u1, v1;
            char *d;
            if (m == 0 || pp == 0 || pp[j] == 0) {
                continue;
            }
            flags = *(int *)(pp[j] + 0x10);
            if (flags & 4) {
                continue;
            }
            if (D_L00_001B2740[j] == 0 || *(int *)(pp[j] + 4) == 0) {
                continue;
            }
            if (j == 6) {
                char *p = D_L00_001BA070;
                if (*(int *)(p + 0xD8) == 0) {
                    continue;
                }
            }
            if (pass == 0 && !(flags & 2)) {
                continue;
            }
            if (pass == 1 && (flags & 2)) {
                continue;
            }
            if (flags & 1) {
                (*(int (**)(char *))(pp[j] + 4))(pp[j]);
                continue;
            }
            d = *(char **)(m + 0x78);
            px = *(int *)(d + 0x50);
            py = *(int *)(d + 0x54);
            pw = *(int *)(d + 0x58);
            ph = *(int *)(d + 0x5C);
            tw = 7;
            while ((1 << tw) < pw) {
                tw++;
            }
            th = 7;
            while ((1 << th) < ph) {
                th++;
            }
            while (tw + th >= 18) {
                th--;
            }
            func_001F7A50(tw, th, pass != 0, 1.0f);
            su = 1 << tw;
            sv = 1 << th;
            func_002017C8(0, 0, su, sv, *(int *)&D_L00_00160330, 0, 0);
            r = (*(int (**)(char *))(pp[j] + 4))(pp[j]);
            func_001F7B40();
            func_00234C98(0x42, 0x8000000064L);
            func_00234C98(0x47, 0x43);
            if (r & 1) {
                continue;
            }
            u = 0;
            u1 = su;
            v = 0;
            v1 = sv;
            if (r & 2) {
                u1 = pw;
                v1 = ph;
            } else if (r & 8) {
                u = (u1 - pw) / 2;
                v = (v1 - ph) / 2;
                u1 -= u;
                v1 -= v;
                if (u < 0) {
                    u = 0;
                }
                if (v < 0) {
                    v = 0;
                }
                if (u1 > su) {
                    u1 = su;
                }
                if (v1 > sv) {
                    v1 = sv;
                }
            } else if (r & 4) {
                if (pw < ph) {
                    u = u1 / 2 - (pw * u1) / (ph * 2);
                    u1 -= u;
                } else {
                    v = v1 / 2 - (ph * v1) / (pw * 2);
                    v1 -= v;
                }
            } else if (!(r & 0x10)) {
                continue;
            }
            func_001F5800(px, py, pw, ph, u, v, u1 - u, v1 - v, 0x80808080L, D_0015EFD0);
        }
        if (pass == 0) {
            func_0020E098();
            func_0020E200();
            func_00234AC8(0x10);
            func_0020DD48();
        }
    }
    func_001F4630(0);
    for (n = 0; n < 14; n++) {
        char *p = D_L00_001BA070;
        if (D_L00_001B2740[n] != 0 && (n != 6 || *(int *)(p + 0xD8) != 0)) {
            func_002250B8(D_L00_001BA220[n]);
        }
    }
    func_001F4748();
}
