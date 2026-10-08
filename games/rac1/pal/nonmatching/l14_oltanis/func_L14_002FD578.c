/* NON_MATCHING func_L14_002FD578 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: BYTES 44/756 (94.2% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern int D_0015EEF0_x[] __asm__("D_0015EEF0") MACRO_ADDR;
extern short D_0015EEF0_s __asm__("D_0015EEF0");
extern unsigned char D_0013D50F_x[] __asm__("D_0013D50F");
extern char D_L14_001FD180[];
extern char D_L14_001FD1B8[];
extern short D_L14_0016202C;
extern void func_0022EE28(int, int, int);
extern void func_L00_00264DB8(int, int);
extern void func_L00_00260108(void *, void *, int, float, float);
extern void func_L00_0028EBF0(int);
extern int func_001E9730_c(void *, int) __asm__("func_001E9730");
extern void func_0020D678(void *);
extern int func_L14_002FDD18__s(char *moby) __asm__("func_L14_002FDD18");
extern void func_L14_002FDE28(char *);
extern void func_L14_002FD870(char *);
extern int func_001F9850(int);
extern int func_L00_0028EB98(void *, int);
extern int func_0022ED80_c(int, int, char *) __asm__("func_0022ED80");
extern char *func_L00_0025B478(void *, int, int);

/* Path-riding gadget: rides its path; when hit it breaks, counts toward the level's skill point, and respawns later. */
void func_L14_002FD578(char *m) {
    char *d = *(char **)(m + 0x78);
    char *h = func_L00_0025B478(m, 0x800000, 0);
    ((unsigned char *)m)[0xA4] = 0xFF;
    if (h != 0 && ((unsigned char *)m)[0x20] != 2) {
        if (D_0015EE84_m == 0xE) {
            int n = D_0015EEF0_x[6] + 1;
            ((int *)&D_0015EEF0_s)[6] = n;
            if (n >= 3) {
                unsigned char *s = D_0013D50F_x + 1;
                if (s[0x18] == 0) {
                    s[0x18] = 1;
                    func_0022EE28(1, 0, 0);
                    func_L00_00264DB8(0x53DB, -1);
                }
            }
        }
        m[0x20] = 2;
        func_L00_00260108(m, m + 0x10, -1, 5.0f, 13.0f);
        *(int *)(m + 0x94) = 0;
        *(unsigned short *)(m + 0x34) |= 0x41;
        if (*(int *)(d + 0x6C) != -1) {
            char *e = D_0013E633 + 0x1D + *(int *)(d + 0x6C) * 0x70;
            if (*(char **)(e + 0x88) == m && ((unsigned char *)e)[0x74] != 0) {
                func_L00_0028EBF0(*(int *)(d + 0x6C));
            }
        }
        *(int *)(d + 0x6C) = -1;
    }
    switch (((unsigned char *)m)[0x20]) {
    case 0:
        if (*(int *)(d + 0x68) == -1) {
            func_001E9730_c(D_L14_001FD180, *(short *)(m + 0xB2));
            func_0020D678(m);
            return;
        }
        if (*(int *)D_L14_001B0F30[*(int *)(d + 0x68)] == 0) {
            func_001E9730_c(D_L14_001FD1B8, *(short *)(m + 0xB2));
            func_0020D678(m);
            return;
        }
        func_L14_002FDD18__s(m);
        m[0x20] = 1;
        ((unsigned char *)m)[0x30] = 0xFF;
        *(short *)(m + 0x32) = 0xFF;
        *(int *)(d + 0x6C) = -1;
        *(int *)(d + 0x7C) = func_001F9850(*(int *)&D_L14_0016202C);
        break;
    case 1:
        func_L14_002FDE28(m);
        break;
    case 2:
        if (func_001F9908(d + 0x7C)) {
            char *path = D_L14_001B0F30[*(int *)(d + 0x68)];
            *(unsigned short *)(m + 0x34) &= 0xFFBE;
            *(int *)(m + 0x94) = *(int *)(*(char **)(m + 0x24) + 0x10);
            *(int *)(d + 0x7C) = func_001F9850(*(int *)&D_L14_0016202C);
            m[0x20] = 1;
            *(int *)(d + 0x60) = 0;
            *(int *)(d + 0x64) = 0;
            qcopy(m + 0x10, path + 0x10);
            *(float *)(m + 0x48) = func_L00_001FF860(*(float *)(path + 0x20) - *(float *)(path + 0x10),
                                                      *(float *)(path + 0x24) - *(float *)(path + 0x14));
            *(float *)(d + 0x84) = *(float *)(d + 0x88) = *(float *)(d + 0x70) = *(float *)(d + 0x74) =
                *(float *)(d + 0x78) = *(float *)(d + 0x80) = 0.0f;
        }
        break;
    }
    func_L14_002FD870(m);
    if (((unsigned char *)m)[0x20] != 0 && ((unsigned char *)m)[0x20] != 2) {
        if (func_L00_0028EB98(m, *(int *)(d + 0x6C)) == 0) {
            *(int *)(d + 0x6C) = func_0022ED80_c(0, 4, m);
        }
    }
}
