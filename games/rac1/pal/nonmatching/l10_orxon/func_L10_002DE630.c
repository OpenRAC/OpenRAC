/* NON_MATCHING func_L10_002DE630 -- src/overlays/l10_orxon/vendor_00296BD8.c
 * Best so far: SIZE ours 364 / retail 360, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Creates moby 0x442 and fills data (arg, 3 floats, owner, func_001FA898(10/(scale*5)), func_002140F8 result), t
 *   p0.c/p1.c differ from retail only in scheduling: retail loads D_0015EE6C (lui+lwc1) before the first data stor
 */
extern char *func_0020D348(int);
extern float func_002140F8(float, float);
extern void func_L00_00251328(void *, int, int, int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern char *func_L00_0026E940(char *, int, int, int, float);
extern float D_0015EE6C MACRO_ADDR;
typedef int u128_2DE630 __attribute__((mode(TI)));

/* Creates a moby of type 0x442 and fills its data block from the arguments. */
char *func_L10_002DE630(int owner, char *pos, int arg, float f0, float f1, float f2) {
    char *moby = func_0020D348(0x442);
    if (moby != 0) {
        char *d = *(char **)(moby + 0x78);
        float k = D_0015EE6C;
        ((unsigned char *)moby)[0x30] = 0xFF;
        *(short *)(moby + 0x32) = 0xFF;
        moby[0x31] = 1;
        moby[0x23] = 0x40;
        moby[0x20] = 0;
        *(u128_2DE630 *)(moby + 0x10) = *(u128_2DE630 *)pos;
        func_L00_00251328(moby, 0x7F, 0x40, 0);
        *(int *)d = arg;
        *(float *)(d + 4) = f0;
        *(float *)(d + 8) = f1;
        *(float *)(d + 0xC) = f2;
        *(int *)(d + 0x10) = owner;
        *(int *)(d + 0x14) = func_001FA898_r(10.0f / (k * 5.0f));
        *(float *)(d + 0x18) = func_002140F8(0.999f, 0.97f);
        func_L00_0026E940(moby, 0x1F4F7F7F, func_001FA898_r(10.0f / (D_0015EE6C * 5.0f)), -1, 126000.01f);
    }
    return moby;
}
