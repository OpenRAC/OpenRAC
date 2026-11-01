typedef int TI128 __attribute__((mode(TI)));

extern char *func_3224B0(int);
extern void func_300D78(char *);

int func_L11_003973F8(char *a0, char *a1, float f12, float f13)
{
    char *s1 = a0;
    char *s2 = a1;
    char *s0 = func_3224B0(0x12B2);
    char *p;

    if (s0 != 0) {
        *(int *)(s0 + 0xB8) = (int)s1;
        *(unsigned char *)(s0 + 0x30) = 0xFF;
        *(TI128 *)(s0 + 0x10) = *(TI128 *)(s1 + 0x10);
        *(TI128 *)(s0 + 0xF0) = *(TI128 *)(s1 + 0xF0);
        func_300D78(s0 + 0xC0);
        p = *(char **)(s0 + 0x68);
        *(int *)(p + 0x10) = (int)s1;
        *(int *)(p + 0x24) = (int)s2;
        *(float *)(p + 0x20) = f12;
        *(float *)(p + 0x28) = f13;
        return 1;
    }
    return 0;
}
