/* NON_MATCHING func_L11_0031BEF0 -- src/overlays/l11_pokitaru/vendor_00312BD8.c
 * Best so far: SIZE ours 132 / retail 136, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern void func_0020D678(void *);
extern int func_L01_0026EFB8(int, int);
extern void func_0022EE28(int, int, int);
extern void func_L00_00264DB8(int, int);
typedef struct { char pad[0x13]; unsigned char flag; } FlagData;
extern unsigned char D_0013D50F[];

void func_L11_0031BEF0(char *moby) {
    int *data = *(int **)(moby + 0x78);
    if (data[0] == -1) {
        func_0020D678(moby);
    } else if (data[1] != -1 && func_L01_0026EFB8(data[1], -1) == 0) {
        unsigned char *flag = &((FlagData *)(D_0013D50F + 1))->flag;
        if (*flag == 0) {
            *flag = 1;
            func_0022EE28(1, 0, 0);
            func_L00_00264DB8(0x53DB, -1);
        }
    }
}
