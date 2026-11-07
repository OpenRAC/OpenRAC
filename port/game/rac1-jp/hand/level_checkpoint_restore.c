/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Native recovery of PAL 002862E0 (0x1B4), checked against retail.
 * Restore the saved checkpoint block and reconnect the hero state.
 * Explicit word offsets avoid the candidate's conflicting byte-array
 * declarations; pointer arguments remain pointers on the native host.
 * This is not a compiler-matching PS2 implementation. */
extern char D_L00_001BA760[] NOT_SDA;
extern unsigned char D_L00_001BB3C0[];
extern char D_0013F450[], D_0013E650[], D_001517D0[];
extern char *D_L00_00160098 MACRO_ADDR;
extern char *D_L00_0016009C MACRO_ADDR;
extern void func_001FAAD8(void *, int, int);
extern void checkpoint_copy(void *, void *, unsigned short)
    __asm__("func_L00_001FFF90");
extern void func_001FAAA0(void);
extern void func_L00_00212068(int, int, void *);
extern void func_L00_00252D90(void *);
extern void func_001FB320(void *, void *);
extern void func_L00_001ED980(void);
extern void func_00217158(void);

void func_L00_002870B8(void) {
    unsigned char *snapshot = D_L00_001BB3C0;
    char *hero = D_0013F450;
    char *status = D_0013E650;
    int i;
    if (*(int *)snapshot == 0) {
        func_001FAAD8(D_L00_001BA760, 0, 0xC60);
        return;
    }
    checkpoint_copy(D_L00_001BA760, snapshot, 0xC60);
    for (i = 0; i < 4; ++i)
        ((unsigned *)(hero + 0x80))[i] = ((unsigned *)(snapshot + 0x10))[i];
    for (i = 0; i < 4; ++i)
        ((unsigned *)(hero + 0x90))[i] = ((unsigned *)(snapshot + 0x20))[i];
    *(int *)(*(char **)(hero + 0x2080) + 0x38) = *(int *)(snapshot + 0x30);
    *(int *)(*(char **)(hero + 0x2080) + 0x3C) = *(int *)(snapshot + 0x34);
    *(int *)(*(char **)(hero + 0x2080) + 0x80) = *(int *)(snapshot + 0x38);
    status[0x6B] |= 7;
    *(unsigned short *)(D_001517D0 + 0x38) = *(unsigned short *)(snapshot + 0xC54);
    *(int *)(status + 0x64) = *(int *)(snapshot + 0x3C);
    status[0x68] = snapshot[0x40];
    status[0x69] = snapshot[0x41];
    status[0x6A] = snapshot[0x42];
    int mode = *(int *)(snapshot + 0x44);
    if (mode != 0 && mode != 3) {
        int object_class = 0;
        if (mode == 1) object_class = 0x57;
        else if (mode == 2) object_class = 0x1A3;
        else func_001FAAA0();
        char *end = D_L00_0016009C;
        for (char *entry = D_L00_00160098; entry < end; entry += 0x100) {
            if (*(short *)(entry + 0xA6) == object_class) {
                /* Retail keeps the found object in a2 across the call. */
                func_L00_00212068(*(int *)(snapshot + 0x44), *(int *)(snapshot + 0x4C), entry);
                break;
            }
        }
    }
    func_L00_00252D90(*(void **)(hero + 0x2080));
    func_001FB320(*(char **)(hero + 0x2080) + 0xC0, hero + 0x90);
    func_L00_001ED980();
    func_00217158();
}
