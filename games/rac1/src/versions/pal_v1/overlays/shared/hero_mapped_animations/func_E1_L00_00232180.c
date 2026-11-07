extern char *D_E1_001413D0;
extern void func_0020D6D0(void *);
void func_E1_L00_00232180(int a, int b) {
    char *p = D_E1_001413D0;

    p[0x50] = b;
    p[0x51] = b + 1;
    p[0x52] = a;
    p[0x53] = a;
    *(int *)(p + 0x54) = 0;
    func_0020D6D0(p);
}
