typedef int TI128 __attribute__((mode(TI)));
typedef unsigned char u8;

extern int D_L14_001B259C __attribute__((sda));

void func_L14_003E5030(u8 *obj, int c) {
    char *state = *(char **)(obj + 0x68);
    TI128 *src = (TI128 *)(obj + 0x10);
    int v;

    *(TI128 *)(state + 0x50) = *src;
    *(TI128 *)(state + 0x60) = *src;
    v = *(int *)(state + 0x1C);
    if (v >= 0) {
        *(int *)(state + 0x28) = D_L14_001B259C + (v << 8);
    }
    v = *(int *)(state + 0x20);
    if (v >= 0) {
        *(int *)(state + 0x2C) = D_L14_001B259C + (v << 8);
    }
    v = *(int *)(state + 0x24);
    if (v >= 0) {
        *(int *)(state + 0x30) = D_L14_001B259C + (v << 8);
    }
    v = *(int *)(state + 0x38);
    if (v >= 0) {
        *(int *)(state + 0x3C) = D_L14_001B259C + (v << 8);
    }
}
