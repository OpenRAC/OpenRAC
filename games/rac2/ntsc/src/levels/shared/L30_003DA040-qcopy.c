typedef int L30TI128 __attribute__((mode(TI)));

extern void func_002A7750(char *, int, int);
extern void func_002A7470(char *, int, int, int);
extern int func_002A9640(char *, int, int);

void func_L30_003DA040(char *a0) {
    char *st = *(char **)(a0 + 0x68);

    switch (*(unsigned char *)(a0 + 0x20)) {
    case 0:
        *(L30TI128 *)(st + 0x100) = *(L30TI128 *)(a0 + 0x10);
        func_002A7750(a0, 1, -1);
        if (*(unsigned char *)(a0 + 0x43) != 1) {
            func_002A7470(a0, 1, 0, 1);
        }
        *(float *)(a0 + 0x48) = 0.0f;
        break;
    case 1:
        if (func_002A9640(a0, 0x10000, 0) != 0) {
            func_002A7750(a0, 2, -1);
            if (*(unsigned char *)(a0 + 0x43) != 1) {
                func_002A7470(a0, 1, 0, 1);
            }
            *(float *)(a0 + 0x48) = 1.0f;
        }
        break;
    case 2:
        if ((*(unsigned char *)(a0 + 0x60) & 2) != 0) {
            *(L30TI128 *)(a0 + 0x10) = *(L30TI128 *)(st + 0x100);
            func_002A7750(a0, 1, -1);
            if (*(unsigned char *)(a0 + 0x43) != 0) {
                func_002A7470(a0, 0, 0, 1);
            }
            *(float *)(a0 + 0x48) = 0.0f;
        }
        break;
    }
    *(unsigned char *)(a0 + 0xA8) = 0xFF;
}
