/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Native recovery of PAL 002862E0 (0x1B4), checked against retail.
 * Restore the saved checkpoint block and reconnect the hero state.
 * Explicit word offsets avoid the candidate's conflicting byte-array
 * declarations; pointer arguments remain pointers on the native host.
 * This is not a compiler-matching PS2 implementation. */
extern char D_L00_001BA960[] NOT_SDA;
extern unsigned char D_L00_001BB5C0[];
extern char D_0013F450[], D_0013E650[], D_001517D0[];
extern char *D_L00_00160098 MACRO_ADDR;
extern char *D_L00_0016009C MACRO_ADDR;
extern void func_001F99B0(void *, int, int);
extern void checkpoint_copy(void *, void *, unsigned short)
    __asm__("func_L00_001FF040");
extern void func_001F9978(void);
extern void func_L00_002110C0(int, int, void *);
extern void func_L00_00251E30(void *);
extern void func_001FA1F8(void *, void *);
extern void func_L00_001ED600(void);
extern void func_00216270(void);

void func_L00_002862E0(void) {
    unsigned char *snapshot = D_L00_001BB5C0;
    char *hero = D_0013F450;
    char *status = D_0013E650;
    int i;
    if (*(int *)snapshot == 0) {
        func_001F99B0(D_L00_001BA960, 0, 0xC60);
        return;
    }
    checkpoint_copy(D_L00_001BA960, snapshot, 0xC60);
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
        else func_001F9978();
        char *end = D_L00_0016009C;
        for (char *entry = D_L00_00160098; entry < end; entry += 0x100) {
            if (*(short *)(entry + 0xA6) == object_class) {
                /* Retail keeps the found object in a2 across the call. */
                func_L00_002110C0(*(int *)(snapshot + 0x44), *(int *)(snapshot + 0x4C), entry);
                break;
            }
        }
    }
    func_L00_00251E30(*(void **)(hero + 0x2080));
    func_001FA1F8(*(char **)(hero + 0x2080) + 0xC0, hero + 0x90);
    func_L00_001ED600();
    func_00216270();
}
