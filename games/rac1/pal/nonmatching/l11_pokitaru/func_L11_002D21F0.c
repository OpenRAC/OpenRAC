/* NON_MATCHING func_L11_002D21F0 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: BYTES 108/780 (86.2% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern int D_L11_00160058_d __asm__("D_L11_00160058") MACRO_ADDR;
extern char D_L11_0016D2E0[];
extern short D_L11_00161470;
extern short D_L11_00161480;
extern float D_0015EE6C_d __asm__("D_0015EE6C") MACRO_ADDR;
extern float D_0015EE70_d __asm__("D_0015EE70") MACRO_ADDR;
extern float func_L00_001FF860(float, float);
extern float func_L00_0025CE58(float *, float *, float, float, float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_00215B18(char *, float);
extern void func_001F9BF0(void *, void *, void *);
extern int func_001F9850(int);
extern void func_001F9C30(void *, void *, float);
extern float func_001F9CE8(void *);
extern float func_L00_0025BC48(float *a, float *b, float *out, float speed, float g);
extern float func_001F9CB8(void *);
extern void func_001F9BC0(void *);
extern int func_L00_00261568_c(char *, char *, void *, void *, void *, void *) __asm__("func_L00_00261568");
extern int func_L11_0030B850(unsigned char *moby);
extern void func_00213DE0(void *, int, int, int);
extern void func_L11_002CCB50(char *moby);

/* Boss movement: turns toward and walks to its target point, lunges at it in a jump arc, follows its rail
 * moby, or (rarely) triggers its idle taunt. */
void func_L11_002D21F0(char *m) {
    char *d = *(char **)(m + 0x78);
    switch (((unsigned char *)m)[0x20]) {
    case 0: case 1: case 2: case 3: case 11: case 12:
        break;
    case 4: case 5: case 6: case 8: case 10: {
        float *rz = (float *)(m + 0x48);
        float yaw = func_L00_001FF860(*(float *)(d + 0x120) - *(float *)(m + 0x10), *(float *)(d + 0x124) - *(float *)(m + 0x14));
        func_L00_0025CE58(rz, (float *)(d + 0x150), yaw, D_0015EE70_d * 12.566371f,
                          D_0015EE70_d * 25.132742f, D_0015EE6C_d * 12.566371f);
        *(float *)(d + 0x130) = func_001F9F90(*(float *)(m + 0x48)) * *(float *)(d + 0x154);
        *(float *)(d + 0x134) = func_001F9FA8(*(float *)(m + 0x48)) * *(float *)(d + 0x154);
        *(float *)(d + 0x138) = *(float *)(d + 0x148) - D_0015EE70_d * 20.0f;
        if (0.0f < *(float *)(d + 0x138)) *(float *)(d + 0x138) = 0.0f;
        break;
    }
    case 7: {
        char *v = d + 0x130;
        char *tgt = d + 0x120;
        if (func_00215B18(m, 9.0f)) {
            func_001F9BF0(v, tgt, m + 0x10);
            func_001F9C30(v, v, 1.0f / (float)func_001F9850(0x3C));
            *(float *)(d + 0x138) = func_L00_0025BC48((float *)(m + 0x10), (float *)tgt, 0, func_001F9CE8(v), -(D_0015EE70_d * 10.8f));
            *(float *)(m + 0x58) = 0.6666667f;
        }
        if (0.0f < func_001F9CB8(v)) *(float *)(d + 0x138) -= D_0015EE70_d * 10.8f;
        if (func_00215B18(m, 29.0f)) {
            func_001F9BC0(v);
            *(float *)(m + 0x58) = 1.0f;
        }
        break;
    }
    case 9: {
        int *arr = (int *)(d + 0xF0);
        int k = *(int *)(d + 0x158) != 0;
        char *o = (char *)(D_L11_00160058_d + (arr[k] << 8));
        func_L00_00261568_c(m, o, &D_L11_00161470, &D_L11_00161480, m + 0x10, m + 0x40);
        if (func_L11_0030B850((unsigned char *)o) && ((unsigned char *)m)[0x53] != 5) {
            func_00213DE0(m, 5, 0, func_001F9850(0x14));
        }
        break;
    }
    case 13: {
        char *s = D_L11_0016D2E0;
        if (*(int *)(s + 0x30) == 1 && *(int *)(s + 0x34) == func_001F9850(0x7D0)) {
            func_L11_002CCB50((char *)(D_L11_00160058_d + (*(int *)(d + 0x104) << 8)));
        }
        break;
    }
    }
}
