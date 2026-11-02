
extern unsigned char *D_L10_001B2528;
extern void func_0029F338(void *);

/* Like func_L05_0037B108 without its first three kinds, with this function's own callees. */
void func_L10_00425798(void) {
    unsigned char *object = D_L10_001B2528;
    int kind;
    while (object) {
        kind = *(short *)(object + 0xAA);
        if (kind == 0x1315 ||
            kind == 0x1327 || kind == 0x1328 || kind == 0x10F9 || kind == 0x946 ||
            kind == 0x10CF || kind == 0x117C || kind == 0x117D || kind == 0x117F)
            func_0029F338(object);
        object = *(unsigned char **)(object + 0x28);
    }
}
