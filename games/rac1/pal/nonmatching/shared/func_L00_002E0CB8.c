/* NON_MATCHING func_L00_002E0CB8 -- src/overlays/shared/vendor_002D9438.c
 * Best so far: SIZE ours 388 / retail 396, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a moby (type 0x4A8), random rotation vector via func_002140F8 x3, sets fields, calls func_L00_00251328 
 *   Wall: retail passes a 16-byte local (at 0x10($sp)) to func_L00_00251328 by copying it to 0($sp) (lq/sq) with a
 *   Local rot also lands at 0x0($sp) in ours vs 0x10 in retail. Needs the callee's real signature (stack-passed ve
 */
typedef int u128 __attribute__((mode(TI)));
typedef struct { float v[4]; } V4;
extern char *func_0020D348(int);
extern float func_002140F8(float, float);
extern void f328(void *, int, int, int, u128) __asm__("func_L00_00251328");
extern void func_L00_00251E30(void *, int);

/* Spawns a moby of type 0x4A8 with random rotation, colour and position. */
char *func_L00_002E0CB8(float sc, char *pos, char *vec, char *vec2, int arg7, unsigned char a, unsigned char b, unsigned char c, unsigned char d) {
    char *m = func_0020D348(0x4A8);
    if (m != 0) {
        char *p;
        V4 rot;
        p = *(char **)(m + 0x78);
        *(u128 *)&rot = 0;
        rot.v[0] = func_002140F8(-3.1415927f, 3.1415927f);
        rot.v[1] = func_002140F8(-3.1415927f, 3.1415927f);
        rot.v[2] = func_002140F8(-3.1415927f, 3.1415927f);
        m[0x20] = 0;
        m[0x23] = d;
        m[0x31] = 1;
        m[0x30] = 0xFF;
        *(short *)(m + 0x32) = 0xFF;
        f328(m, a, b, c, *(u128 *)&rot);
        *(float *)(p + 0x18) = *(float *)(m + 0x2C) * sc;
        *(int *)(m + 0x2C) = 0;
        *(char **)(p + 0x10) = pos;
        *(u128 *)(m + 0x10) = *(u128 *)vec;
        *(u128 *)p = *(u128 *)vec2;
        *(u128 *)(m + 0x40) = *(u128 *)&rot;
        *(short *)(p + 0x1E) = d;
        *(int *)(p + 0x14) = arg7;
        *(unsigned short *)(p + 0x1C) = *(unsigned short *)(p + 0x14);
        func_L00_00251E30(m, 0);
    }
    return m;
}
