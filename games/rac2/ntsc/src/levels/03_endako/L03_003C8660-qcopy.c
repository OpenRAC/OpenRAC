typedef int TI128 __attribute__((mode(TI)));

extern void func_31B920(char *a0, int a1, int a2);

void func_L03_003C8660(char *a0, char *a1) {
    char *v1 = *(char **)(a0 + 0x68);

    *(int *)(v1 + 0xAC) = 0;
    *(int *)(v1 + 0xA0) = 0;
    *(TI128 *)(v1 + 0x80) = *(TI128 *)a1;
    func_31B920(a0, 3, -1);
}
