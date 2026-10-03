/* NON_MATCHING func_L02_002F9ED8 -- src/overlays/shared/vendor_002A5218.c
 * Best so far: BYTES 14/48 (70.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char *D_L02_00167480;

void func_L02_002F9ED8(float a, float b, float c,
                       float d, float e, float f) {
    char *moby = D_L02_00167480;
    char *data = *(char **)(moby + 0x70);
    char *dst = data + 0x30;
    *(float *)(dst + 0x24) = d;
    *(float *)(dst + 0x20) = e;
    *(float *)(dst + 0x28) = f;
    data = *(char **)(moby + 0x70);
    *(float *)(data + 0x18) = c;
    *(float *)(data + 0x10) = b;
    *(float *)(data + 0x14) = a;
}
