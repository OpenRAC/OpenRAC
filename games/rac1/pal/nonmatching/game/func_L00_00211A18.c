/* Whether moby m is of class 0x47: 0 for no moby (retail branches to the shared "return 0" at
   func_001E97C8's place), else o_class (+0xA6, signed halfword) == 0x47. */
int func_L00_00211A18(char *m) {
    if (m == 0) {
        return 0;
    }
    return *(short *)(m + 0xA6) == 0x47;
}
