/* NON_MATCHING func_L00_00235960 -- src/overlays/shared/hud_00235960.c
 * Best so far: SIZE ours 468 / retail 480, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   HudMoveBank: uploads up to four bank blocks (sizes at +0x54/+0x5C/+0x60/+0x64 of the record at D_L00_0017E5D8+
 *   Best is p6.c: same instruction sequence, size 468 vs 480, differs in two ways: retail keeps the lui of D_L00_0
 *   Budget spent; what would unblock: the source form that makes the fourth block rebuild the address from the sha
 *   Hint found later (func_L00_002618D8 matched this way): retail's `sym+0xNN` operands are usually separate real 
 */
extern char D_L00_0017E5D8[];
extern char D_L00_001BA070[] NOT_SDA;
extern int D_L00_0017E610;
extern int D_L00_0017E618;
extern int D_L00_0017E61C;
extern int D_L00_0017E620;
extern int func_00234238(void *dest, unsigned int slot, int offset, int size, int mode);
extern void func_0020C468_2(int, int) __asm__("func_0020C468");
extern void func_001FF7F0(int bank, int addr);

/* Uploads the HUD bank blocks for the current set into consecutive memory. */
void func_L00_00235960(int arg) {
    char *bb = D_L00_001BA070;
    char *b = D_L00_0017E5D8;
    int addr = *(int *)(bb + 0x104);
    int dest = *(int *)(bb + 0x10);
    char *p = *(char **)(b + 0x18);
    int s1, s2, s3;

    s1 = *(int *)(p + 0x54);
    if (s1 != 0) {
        func_00234238((void *)dest, *(unsigned int *)(p + 0x94), 0, D_L00_0017E610 / 16, 0);
        func_0020C468_2(dest, addr);
        func_001FF7F0(0, addr);
        addr += s1;
        p = *(char **)(b + 0x18);
    }
    s2 = *(int *)(p + 0x5C);
    if (s2 != 0) {
        func_00234238((void *)dest, *(unsigned int *)(p + 0x9C), 0, D_L00_0017E618 / 16, 0);
        func_0020C468_2(dest, addr);
        func_001FF7F0(2, addr);
        addr += s2;
    }
    if (arg == 0 || arg == 2) {
        p = *(char **)(b + 0x18);
        s3 = *(int *)(p + 0x60);
        if (s3 != 0) {
            func_00234238((void *)dest, *(unsigned int *)(p + 0xA0), 0, D_L00_0017E61C / 16, 0);
            func_0020C468_2(dest, addr);
            func_001FF7F0(3, addr);
            addr += s3;
        }
    }
    if ((unsigned int)(arg - 1) < 2) {
        p = *(char **)(D_L00_0017E5D8 + 0x18);
        if (*(int *)(p + 0x64) != 0) {
            func_00234238((void *)dest, *(unsigned int *)(p + 0xA4), 0, D_L00_0017E620 / 16, 0);
            func_0020C468_2(dest, addr);
            func_001FF7F0(4, addr);
        }
    }
}
