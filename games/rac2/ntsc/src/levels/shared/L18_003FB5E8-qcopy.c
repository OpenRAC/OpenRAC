typedef int TI128 __attribute__((mode(TI)));

void func_L18_003FB5E8(char *a0, char *a1) {
    char *v1 = *(char **)(a0 + 0x68);

    *(TI128 *)a1 = *(TI128 *)v1;
    *(float *)(a1 + 8) = *(float *)(a1 + 8) - 1.0f;
}
