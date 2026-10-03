/* NON_MATCHING func_L11_003175D0 -- src/overlays/l11_pokitaru/vendor_00312BD8.c
 * Best so far: SIZE ours 172 / retail 176, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern void *func_L00_002DCDA8(void *);

void *func_L11_003175D0(unsigned char *arg) {
    void *found = func_L00_002DCDA8(arg);
    if (found != 0) {
        unsigned int state = arg[0x20];
        unsigned char kind = state;
        if (kind == 2 || kind == 4 || kind == 5 || kind == 6 ||
            kind == 14 || kind == 15 || kind == 16 || kind == 20) {
            if (state != 20) {
                arg[0xBC] = state;
                arg[0x20] = 20;
            }
        } else {
            found = 0;
        }
    } else if (arg[0x20] == 20) {
        arg[0x20] = arg[0xBC];
    }
    return found;
}
