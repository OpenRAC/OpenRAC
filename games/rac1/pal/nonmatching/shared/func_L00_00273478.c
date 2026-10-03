/* NON_MATCHING func_L00_00273478 -- src/overlays/shared/partupd_00272158.c
 * Best so far: BYTES 10/252 (96.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns particle type 0x37 tied to a moby (args: moby, int, int, int, float, float); fills fields, copies r+0x1
 *   Best is p5.c (10 bytes differ of 252): only the allocation of arg0 and arg3 differs (ours a=$s2,d=$s3; retail 
 *   Rewording the store order and hoisting the lh did not move the allocator; would need the original priority/liv
 */
extern char *D_L00_001B24DC;
extern unsigned char *func_00218928(int);
extern int func_001FA898(float);
extern void func_L00_00250800(void *, int, void *);

/* spawns a type-0x37 particle tied to a moby */
unsigned char *func_L00_00273478(char *a, int b, int c, int d, float f, float g) {
    unsigned char *r = func_00218928(0x37);
    if (r != 0) {
        char *p;
        *(int *)(r + 4) = c;
        *(int *)(r + 0xC) = d;
        r[9] = func_001FA898(0.4f) + 0x20;
        r[3] = 0x48;
        r[1] = 3;
        r[2] = *(unsigned char *)D_L00_001B24DC;
        func_L00_00250800(a, b, r + 0x10);
        qcopy(r + 0x20, r + 0x10);
        *(float *)(r + 0x2C) = g;
        *(float *)(r + 0x1C) = f;
        p = (char *)r + 0x30;
        *(char **)p = a;
        *(int *)(p + 8) = *(short *)(a + 0xA6);
        *(int *)(p + 4) = b;
        *(float *)(p + 0xC) = f;
    }
    return r;
}
