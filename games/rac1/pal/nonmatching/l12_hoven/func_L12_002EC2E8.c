/* NON_MATCHING func_L12_002EC2E8 -- src/overlays/l12_hoven/vendor_002C0310.c
 * Best so far: BYTES 6/692 (99.1% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef struct {
    int count;
    int pad[3];
    float e[1][4];
} Path_2ec2e8;
extern Path_2ec2e8 *D_L12_001B0C30_p[] __asm__("D_L12_001B0C30");
extern void func_L00_00260D30(void *, void *, float);
extern float func_L00_00259148(float *vel, float cur, float target, float k, float d, float max);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_001F9D10(void *, void *);
extern void func_L12_002EC180(char *moby);

/* Moves moby m along path `path`: steers at the current node (or, within 5 units of the end, at the
 * final approach point and brakes), advances to the next node within 2 units, and returns 1 once it
 * has stopped at the end. */
int func_L12_002EC2E8(char *m, int path) {
    float tgt[4];
    float pad[4][4];
    Path_2ec2e8 *l = D_L12_001B0C30_p[path];
    char *d = *(char **)(m + 0x78);
    float rem = 0.0f;
    int i;
    for (i = *(int *)(d + 0xB4); i < l->count - 1; i++) {
        rem += l->e[i][3];
    }
    if (5.0f < rem) {
        char *n = (char *)l + (*(int *)(d + 0xB4) << 4);
        float y = func_L00_001FF860(*(float *)(n + 0x10) - *(float *)(m + 0x10), *(float *)(n + 0x14) - *(float *)(m + 0x14));
        float r = func_L00_00259148((float *)(d + 0x88), *(float *)(m + 0x48), y, 0.004f, 0.3f, D_0015EE6C * 0.7853982f);
        float a = D_0015EE6C * 10.0f;
        float b = D_0015EE70 * 7.0f;
        *(float *)(m + 0x48) = r;
        func_00214D28(d + 0x94, a, b);
    } else {
        float y;
        float r;
        float b;
        func_L00_00260D30(m, tgt, 48.0f);
        y = func_L00_001FF860(tgt[0] - *(float *)(m + 0x10), tgt[1] - *(float *)(m + 0x14));
        r = func_L00_00259148((float *)(d + 0x88), *(float *)(m + 0x48), y, 0.004f, 0.3f, D_0015EE6C * 0.7853982f);
        b = D_0015EE70 * 9.0f;
        *(float *)(m + 0x48) = r;
        func_00214D28(d + 0x94, 0.0f, b);
        if (*(float *)(d + 0x94) == 0.0f) return 1;
    }
    func_001F9BF0(tgt, l->e[*(int *)(d + 0xB4)], m + 0x10);
    func_L00_001FF4B0(d + 0x70, tgt, *(float *)(d + 0x94));
    func_001F9BD8(m + 0x10, m + 0x10, d + 0x70);
    if (func_001F9D10(m + 0x10, l->e[*(int *)(d + 0xB4)]) < 2.0f) {
        (*(int *)(d + 0xB4))++;
        if (l->count - 1 < *(int *)(d + 0xB4)) *(int *)(d + 0xB4) = l->count - 1;
    }
    func_L12_002EC180(m);
    return 0;
}
