/* NON_MATCHING func_L07_0029C6A8 -- src/overlays/shared/partupd_0029B070.c
 * Best so far: BYTES 6/376 (98.4% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L07_0029C6A8: spawns a particle moby (class 0x3D), fills its fields, then builds an orientation vector fr
 *   Only difference: two adjacent stores are swapped. Retail stores p[3], p[1], then the float x at p+0xC; ours al
 *   A scheduler tie on independent stores; would need a source form that changes the priority of the p[1]=0 store 
 */
extern int func_001F9850(int);
extern float func_002140F8(float, float);
extern int func_001160D8(void);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_00250800(void *, int, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern unsigned char *D_L07_001B24F4;
extern float D_0015EE60 MACRO_ADDR;
extern char D_L07_00166EC0[];

// Spawns a particle moby and orients it from a point relative to the camera.
char *func_L07_0029C6A8(int a, short b, float x, float y) {
    float va[4];
    float vb[4];
    char *p = func_00218928(0x3D);
    char *q;
    if (p) {
        *(int *)(p + 0x20) = a;
        q = p + 0x20;
        *(short *)(p + 0xA) = func_001F9850(0x50);
        *(int *)(q + 4) = 0;
        *(float *)(q + 8) = func_002140F8(*(float *)(q + 4), 1.0f);
        *(short *)(q + 0xC) = func_001160D8() & 0xF;
        *(float *)(q + 0x14) = y;
        p[8] = func_001160D8();
        *(int *)(p + 4) = 0x7F7F7F;
        p[9] = func_001FA898_r(4.0f) + 0x40;
        p[3] = 0x48;
        p[1] = 0;
        *(float *)(p + 0xC) = x;
        p[2] = D_L07_001B24F4[*(short *)(q + 0xC)];
        *(float *)(q + 0x10) = D_0015EE60 * 0.015f;
        *(short *)(q + 0xE) = b;
        p[8] = func_001FA898_r(*(float *)(q + 8) * 255.0f);
        func_L00_00250800(*(void **)(p + 0x20), *(short *)(q + 0xE), va);
        func_001F9BF0(vb, D_L07_00166EC0, va);
        func_L00_001FF4B0(vb, vb, *(float *)(q + 0x14));
        func_001F9BD8(p + 0x10, va, vb);
    }
    return p;
}
