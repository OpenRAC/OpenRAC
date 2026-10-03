/* NON_MATCHING func_L07_0030C320 -- src/overlays/l07_umbris/vendor_002CE470.c
 * Best so far: SIZE ours 192 / retail 188, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Init of a moby from its spawn record (0x24 ptr): state/flags stores, calls func_L00_00251B58(moby, 0x80807F7F,
 *   Only difference is register sharing: retail's sb 0x30 / sb 0x31 reuse the very registers ($a3=0xFF, $a2=1) loa
 *   Unblock: a wording that makes the stores share the arg constants without merging with the saved 0xFF.
 */
extern void func_L00_00251B58(void *, int, int, int);
extern void func_L00_0025E210(void *);
extern void func_L00_00251E30(void *);
extern int func_L00_0025D390(char *);

/* initialise a moby from its spawn record and set its effect's float */
void func_L07_0030C320(char *moby) {
    char *data;
    unsigned char c = 0xFF;
    char *r;
    moby[0x20] = 0;
    data = *(char **)(moby + 0x24);
    moby[0xBC] = 0;
    *(unsigned short *)(moby + 0x34) = *(unsigned short *)(data + 0x44);
    *(float *)(moby + 0x2C) = *(float *)(data + 0x24);
    *(short *)(moby + 0x32) = c;
    func_L00_00251B58(moby, 0x80807F7F, moby[0x31] = 1, ((unsigned char *)moby)[0x30] = 0xFF);
    moby[0x71] = c;
    *(short *)(moby + 0x36) = 0x7F80;
    moby[0x72] = c;
    moby[0xA4] = c;
    *(int *)(moby + 0x94) = *(int *)(*(char **)(moby + 0x24) + 0x10);
    func_L00_0025E210(moby);
    func_L00_00251E30(moby);
    r = (char *)func_L00_0025D390(moby);
    if (r != 0) {
        *(float *)r = (float)*(short *)(r + 4);
    }
}
