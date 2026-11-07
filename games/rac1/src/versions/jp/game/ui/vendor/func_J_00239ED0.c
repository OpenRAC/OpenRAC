void func_J_00239ED0(unsigned char *src) {
    char *st = vendor_state;
    char *e;
    unsigned char *d;
    int i;

    FastMemSet(*(char **)(st + 0x2C), 0x20202020, 0x24);
    d = (unsigned char *)(*(char **)(st + 0x2C) + 0x24);
    for (i = 0; src[i] != 0; i++) {
        if (src[i] == 'b') {
            *d = 0xF;
            i++;
            d++;
            *d = src[i];
            d++;
            *d = 8;
        } else {
            if ((unsigned char)(src[i] - 8) < 8) {
                continue;
            }
            *d = src[i];
        }
        d++;
    }
    *d = 0;
    e = vendor_state;
    *(int *)(e + 0x44) = 0;
}
