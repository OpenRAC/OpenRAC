/* NON_MATCHING func_L00_002AA998 -- src/overlays/shared/vendor_002A5138.c
 * Best so far: BYTES 39/688 (94.3% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char D_0013E633_f5d[] __asm__("D_0013E633");
typedef int Q_2aa998 __attribute__((mode(TI)));
extern short D_L00_0016148C;
extern char D_L00_00173F60_c[] __asm__("D_L00_00173F60");
extern short D_L00_001B0AF0[];
extern void func_00215C00(void *, float, float, float);
extern char *func_L00_00264140(void *, void *, void *, float *, float *, float *, float);
extern int func_001F9850(int);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern void func_L00_002607F8_c(void *, short *, int) __asm__("func_L00_002607F8");

/* Spawns a homing bolt (class 0x100) from owner at pos, aimed at tgt or at whatever lies along its heading. */
char *func_L00_002AA998(void *owner, void *pos, float a, float b, float c, char *tgt, float rz, float ry) {
    char *m;
    char *d;
    float t[4];
    float p[4];
    m = (char *)func_0020D348(0x100);
    if (m != 0) {
        ((unsigned char *)m)[0x30] = 0xFF;
        m[0x31] = 1;
        m[0x20] = 0;
        *(float *)(m + 0x2C) *= *(float *)&D_L00_0016148C;
        *(short *)(m + 0x32) = 0xFF;
        d = *(char **)(m + 0x78);
        *(float *)(m + 0x48) = rz;
        *(float *)(m + 0x44) = ry;
        if (tgt == 0) {
            func_00215C00(t, D_0015EE6C * 20.0f, rz, ry);
            *(Q_2aa998 *)p = *(Q_2aa998 *)pos;
            tgt = func_L00_00264140(owner, p, t, &a, &b, &c, 0.34906584f);
        }
        *(void **)(d + 0x10) = owner;
        qcopy(m + 0x10, pos);
        *(float *)(d + 0x34) = D_0015EE6C * 100.0f;
        *(int *)(d + 0x38) = 1;
        *(int *)(d + 0x24) = 0;
        *(int *)(d + 0x28) = 0;
        *(int *)(d + 0x20) = 0;
        *(char **)(d + 0x18) = tgt;
        if (tgt != 0) {
            unsigned char *e = (unsigned char *)func_L00_0025D390(tgt);
            if (e != 0) {
                *(unsigned short *)(e + 0x1E) |= 0x80;
                if (e[0xB] != 0) *(int *)(d + 0x38) = 0;
                if (e[0xC] != 0) *(float *)(d + 0x34) = (float)e[0xC] * D_0015EE6C;
            }
        }
        *(float *)(d + 0x2C) = a;
        *(float *)(d + 0x30) = b;
        *(int *)(d + 0x14) = func_001F9850(0x96);
        if (tgt != 0) {
            qcopy(d, tgt + 0x10);
            *(float *)(d + 8) += c;
            *(float *)(d + 0x1C) = c;
        }
        {
            int n;
            int r;
            if (func_001F9CB8((float *)(D_0013E633_f5d + 0xF5D)) > D_0015EE6C * 0.1f) n = 0x3C;
            else n = 1;
            r = func_001F9850(n);
            *(short *)(d + 0x3E) = r;
            *(short *)(d + 0x3C) = r;
        }
        *(Q_2aa998 *)t = *(Q_2aa998 *)((char *)owner + 0x10);
        t[2] = ((float *)pos)[2];
        if (func_L00_001EFFF0(t, pos, 0, *(int *)(D_0013E633 + 0x2E9D), 0) != 0) {
            m[0xBC] = 1;
            qcopy(m + 0x10, D_L00_00173F60_c);
        }
        func_L00_00251E30(m);
        func_L00_002607F8_c(m, D_L00_001B0AF0, 0x1F);
        *(float *)(m + 0x2C) += *(float *)(m + 0x2C);
    }
    return m;
}
