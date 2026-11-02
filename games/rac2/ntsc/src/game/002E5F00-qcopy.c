typedef int TI128 __attribute__((mode(TI)));
extern char D_00188660[];

int func_002E5F00(int a0, char *a1) {
    int off = a0 * 0x70;
    char *o = D_00188660 + off;
    char *base2 = D_00188660 + 0xA0;

    o[0x75] |= 0x40;
    *(TI128 *)(base2 + off) = *(TI128 *)a1;
    return 1;
}
