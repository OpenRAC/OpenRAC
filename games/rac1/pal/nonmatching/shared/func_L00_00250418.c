/* NON_MATCHING func_L00_00250418 -- src/overlays/shared/mobyfunc_0024FD50.c
 * Best so far: BYTES 6/92 (93.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern void func_L00_002501C8(void *, unsigned char *);

void func_L00_00250418(unsigned char *moby, unsigned char *out, int a, int b, int count, int flag)
{
    float inv = 1.0f / (float)count;
    out[0x22] = moby[0x53];
    out[0x20] = moby[0x51];
    out[0x23] = a;
    out[0x21] = b;
    *(int *)(out + 0x24) = 0;
    *(float *)(out + 0x2C) = inv;
    *(float *)(out + 0x28) = 1.0f;
    if (flag) {
        func_L00_002501C8(moby, out);
    }
}
