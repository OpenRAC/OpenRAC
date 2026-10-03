/* NON_MATCHING func_L00_00209940 -- src/overlays/shared/help_00203E98.c
 * Best so far: SIZE ours 640 / retail 632, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   HeroItemPoseFromRatchet: builds a display list at dst+0x18 from a -1-terminated index list (three passes: 64-b
 *   Difference: the allocator swaps s3/s4 (ours pad=s3, loop counter n=s4; retail the reverse), which shifts every
 *   Would unblock: the source form that gives pad a higher allocation priority than n (for(i=0;i<pad;i++) and if(p
 */
extern unsigned char D_0013E633[] NOT_SDA;
extern long func_L00_001FF718(int, float);
extern void func_L00_00255B90(int);

struct PoseHdr {
    short z0;
    short z2;
    short z4;
    short len;
    short sz;
    short n2;
    short szc;
    short n3;
    long term;
};

/* builds a pose display list from three index lists */
void func_L00_00209940(int *src, struct PoseHdr *dst, char *p2, int pad, int x) {
    long *out;
    int n;
    int n2;
    int n3;
    int t1;
    int t2;
    int i;
    int j;

    if (x == 0) x = *(int *)(D_0013E633 + 0x2E9D);
    func_L00_00255B90(x);
    dst->term = 0x7FFF000000000000L;
    out = (long *)((char *)dst + 0x18);
    for (n = 1; src[n] >= 0; n++) {
        *out++ = func_L00_001FF718((src[n] << 6) + 0x70000000, 32768.0f);
    }
    if (pad > 0) {
        i = pad;
        do {
            *out++ = 0x7FFF000000000000L;
            n++;
        } while (--i != 0);
    }
    t1 = n * 8;
    n2 = 0;
    for (j = 1; src[j] >= 0; j++) {
        long r = func_L00_001FF718((src[j] << 6) + 0x70000010, 4096.0f) & 0xFFFFFFFFFFFFL;
        if (r != 0x100010001000L) {
            *out = r;
            n2++;
            *(short *)((char *)out + 6) = j | -0x8000;
            out++;
        }
    }
    t2 = n + n2;
    n3 = 0;
    for (j = 1; src[j] >= 0; j++) {
        long a = func_L00_001FF718((src[j] << 6) + 0x70000020, 1.0f) & 0xFFFFFFFFFFFFL;
        long b = func_L00_001FF718(*(int *)(p2 + 0x18) + (j << 4), 1.0f) & 0xFFFFFFFFFFFFL;
        if (a != b) {
            *out = a;
            n3++;
            *(short *)((char *)out + 6) = j;
            out++;
        }
    }
    if (((int)out & 0xF) != 0) {
        *out = 0;
        out++;
    }
    dst->n3 = n3;
    dst->len = ((char *)out - 0x10 - (char *)dst) >> 4;
    dst->n2 = n2;
    dst->sz = t1;
    dst->szc = t2 << 3;
    dst->z0 = 0;
    dst->z2 = 0;
    dst->z4 = 0;
}
