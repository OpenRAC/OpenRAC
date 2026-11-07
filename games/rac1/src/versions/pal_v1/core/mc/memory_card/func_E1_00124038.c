void func_E1_00124038(char *dst) {
    extern char D_E1_00159FC0[];
    int n;

    if (dst != 0) {
        char *p = (char *)((unsigned int)D_E1_00159FC0 | 0x20000000);
        if ((unsigned int)libc_strlen(p) < 0x400) {
            n = libc_strlen(p);
        } else {
            n = 0x3FF;
        }
        memcpy(dst, (char *)((unsigned int)D_E1_00159FC0 | 0x20000000), n);
        dst[n] = 0;
    }
}
