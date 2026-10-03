/* NON_MATCHING func_L00_0026B890 -- src/overlays/shared/partupd_0026A130.c
 * Best so far: BYTES 19/644 (97.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a type-11 particle (rate-limited by load globals). Best is p1.c (19 bytes differ, same size): the store
 *   to e[0x18..0x1B] come out in another order and s1/s2 (t1/t2) land in swapped saved regs ($s6/$s7); also
 *   $a3 vs $t2 saved into $fp/$s7. Moving the e[0x1A]=s2 store (p2, p3) did not help. Signature uses the
 *   partupd file's later extern (float f in slot 8, int s3, float g). Needs a different store order/arg-use trick.
 */
extern int func_002140B0(int);
extern void *func_00218928(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_001160D8(void);
extern float func_002140F8(float, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float D_L00_0015F6B4 MACRO_ADDR;
extern float D_L00_0015F6B8 MACRO_ADDR;
extern unsigned char *D_L00_001B242C;
typedef struct { float x, y, z, w; } __attribute__((aligned(16))) W4;

// Spawns a type-11 particle at a position with a random chance of being skipped under load.
void func_L00_0026B890(void *a, void *b, int c, int d, int flag, int s1, int s2, float f, int s3, float g) {
    unsigned char *m;
    char *e;
    W4 q;
    if (flag) {
        if (D_L00_0015F6B4 > 0.85f || D_L00_0015F6B8 > 0.85f) {
            if (!func_002140B0(3)) return;
        }
        if (D_L00_0015F6B4 > 0.9f || D_L00_0015F6B8 > 0.9f) {
            if (!func_002140B0(2)) return;
        }
        if (D_L00_0015F6B4 > 1.0f || D_L00_0015F6B8 > 1.0f) {
            if (!func_002140B0(1)) return;
        }
    }
    m = func_00218928(0xB);
    if (m) {
        qcopy(m + 0x10, a);
        *(int *)(m + 4) = c;
        m[9] = func_001FA898_r(4.0f) - 0x60;
        m[3] = func_002140B0(100) < 0x14 ? 0x44 : 0x48;
        m[1] = 0;
        e = (char *)m + 0x20;
        *(int *)(m + 0xc) = 0;
        m[8] = func_001160D8();
        m[2] = *D_L00_001B242C;
        {
        W4 r = {0};
        r.x = func_002140F8(-1.0f, 1.0f);
        r.y = func_002140F8(-1.0f, 1.0f);
        r.z = func_002140F8(-1.0f, 1.0f);
        q = r;
        func_L00_001FF4B0(&q, &q, g);
        func_001F9BD8(e, b, &q);
        *(float *)(e + 0xC) = g * 0.75f;
        *(short *)(m + 0xA) = flag;
        e[0x19] = s3;
        e[0x1B] = flag;
        e[0x18] = s1;
        *(float *)(e + 0x1C) = f;
        *(int *)(e + 0x10) = c;
        *(int *)(e + 0x14) = d;
        e[0x1A] = s2;
        }
    }
}
