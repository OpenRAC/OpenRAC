typedef int TI128 __attribute__((mode(TI)));
typedef unsigned char u8;
void func_L05_00432158(u8 *a0, u8 *a1) {
    TI128 *s;
    TI128 *d;
    int i;
    *(TI128 *)(a1 + 0x10) = *(TI128 *)(a0 + 0x10);
    s = (TI128 *)(a0 + 0xC0);
    d = (TI128 *)(a1 + 0xC0);
    for (i = 0; i < 3; i++)
        *d++ = *s++;
}
