typedef int TI128 __attribute__((mode(TI)));

void func_L03_003C7AA0(char *a0, char *a1) {
    char *a2 = *(char **)(a0 + 0x68);

    *(TI128 *)a1 = *(TI128 *)(a0 + 0x10);
    *(float *)(a1 + 8) = *(float *)(a2 + 0xF8);
}
