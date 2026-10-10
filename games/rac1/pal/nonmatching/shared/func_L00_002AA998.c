/* NON_MATCHING func_L00_002AA998 -- src/overlays/shared/vendor_002A5138.c
 * Best so far: SIZE ours 692 / retail 688, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns class0x100, resolves a target, initializes projectile data and collision, registers and scales the moby
 *   mini12 a01 named wall: retail loads D_0015EE6C via repeated lui at +0x90, +0x100, +0x1dc (three distinct acces
 *   No run; unblock needs matching per-function compiler flags.
 *   Offset correction: D_0015EE6C lui instructions are +0x94, +0xfc and +0x1d0; all are explicit repeated symbol l
 *   mini42 main-only: current staged candidate confirms39/688. p1 union vectors plus owner layout and real float d
 */
extern char D_0013E633_f5d[] __asm__("D_0013E633");
typedef int Q_2aa998 __attribute__((mode(TI)));
typedef union { Q_2aa998 q; float f[4]; } BoltVector_2AA998;
typedef struct { char pad[0x10]; BoltVector_2AA998 position; } BoltOwner_2AA998;
extern float D_L00_0016148C;
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
    BoltVector_2AA998 t;
    BoltVector_2AA998 p;
    m = (char *)func_0020D348(0x100);
    if (m != 0) {
        ((unsigned char *)m)[0x30] = 0xFF;
        m[0x31] = 1;
        m[0x20] = 0;
        *(float *)(m + 0x2C) *= D_L00_0016148C;
        *(short *)(m + 0x32) = 0xFF;
        d = *(char **)(m + 0x78);
        *(float *)(m + 0x48) = rz;
        *(float *)(m + 0x44) = ry;
        if (tgt == 0) {
            func_00215C00(&t, D_0015EE6C * 20.0f, rz, ry);
            p.q = *(Q_2aa998 *)pos;
            tgt = func_L00_00264140(owner, &p, &t, &a, &b, &c, 0.34906584f);
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
        t.q = ((BoltOwner_2AA998 *)owner)->position.q;
        t.f[2] = ((float *)pos)[2];
        if (func_L00_001EFFF0(&t, pos, 0, *(int *)(D_0013E633 + 0x2E9D), 0) != 0) {
            m[0xBC] = 1;
            qcopy(m + 0x10, D_L00_00173F60_c);
        }
        func_L00_00251E30(m);
        func_L00_002607F8_c(m, D_L00_001B0AF0, 0x1F);
        *(float *)(m + 0x2C) += *(float *)(m + 0x2C);
    }
    return m;
}
