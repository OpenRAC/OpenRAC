typedef int TI128 __attribute__((mode(TI)));

void func_L14_00328E70(char *a0, char *a1, char *a2) {
    *(TI128 *)a1 = *(TI128 *)(a0 + 0x10);
    *(TI128 *)a2 = *(TI128 *)(a0 + 0xF0);
}
