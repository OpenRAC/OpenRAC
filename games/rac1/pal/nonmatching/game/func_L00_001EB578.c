/* Finds the first 0xA0-byte entry, from D_L00_00167250 up to 0x1E00 bytes on, whose halfword at +0x86 is a0; 0 when none has it. */
extern char D_L00_00167250_b[] __asm__("D_L00_00167250");

char *func_L00_001EB578(int a0) {
    char *p = D_L00_00167250_b;
    char *end = D_L00_00167250_b + 0x1E00;
    if (*(short *)(p + 0x86) == a0) {
        return p;
    }
    p += 0xA0;
    while (p < end) {
        if (*(short *)(p + 0x86) == a0) {
            return p;
        }
        p += 0xA0;
    }
    return 0;
}
