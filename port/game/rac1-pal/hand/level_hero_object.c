/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Native PAL object-type predicate, recovered from the six instructions
 * at 00211A18. The null branch reaches the shared return-zero epilogue
 * at 00211A30 (catalogued as func_001E97C8). Not a PS2 byte match. */
int func_L00_00211A18(void *object) {
    if (!object) return 0;
    return *(short *)((char *)object + 0xA6) == 0x47;
}

extern unsigned char D_0013F450[];
/* The catalogue splits this accessor's return paths into 0020DB1C and
 * 0020DB24. Include both tails: an active module slot returns its object,
 * any other state returns null. Preserve the 0x50-byte guest slot stride. */
char *func_L00_0020DAF8(int slot) {
    unsigned char *entry = D_0013F450 + slot * 0x50;
    if (*(int *)(entry + 0x10B4) != 2) return 0;
    return *(char **)(entry + 0x1090);
}
