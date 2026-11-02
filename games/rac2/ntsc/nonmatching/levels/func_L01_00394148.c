/*
 func_L01_00394148 (248 bytes, 4 places) - own unit with -O2 -G0 -ffunction-sections -mno-split-addresses.
 Size exact; 5 masked lines differ (w5, 2026-10-28): every instruction matches except where `daddu $s0,$zero,$zero`
 (i = 0) sits: this cc1 schedules it first and reorg puts it in the delay slot of the beqz on D_001A7A40, retail has the
 `lui $v1,0x1C` there and the i = 0 in the `blez` slot.
 What got here: re-read the base in the loop body (`base = (char *)D_L01_001BD1C0;` at the top of each iteration) so the
 guard keeps its own temporary and the loop gets retail's copy `daddu $s2,$v1,$zero`; plain incomplete arrays for the loop
 globals (not sda: sda turns the macro loads in branch-likely slots into $gp loads); no literal addresses.
 Reopen: a spelling that puts i = 0 after the count load in RTL order (do/while with a guard, if(count>0){for}, ++i forms
 and a separate guard variable were all worse: 14-66 lines).
 */
extern int D_L01_001A8F40;
extern unsigned char D_001A7A40[];
extern char *D_L01_001BD1C0[];
extern unsigned char D_L01_001B1E61[];
extern char D_L01_001B1E60[];
extern float D_L01_001B1E80[];
extern float D_L01_001B1E84[];
extern float D_L01_001B1E88[];
extern void func_0029F978(char *p, int a1, char *a2);

void func_L01_00394148(void)
{
    char *base;
    char *guard;
    char **table;
    char *p;
    int i;

    if (D_L01_001A8F40 != 2)
        return;
    if (D_001A7A40[0] == 0)
        return;
    base = (char *)D_L01_001BD1C0;
    for (i = 0; i < *(short *)(base + 0x44); i++) {
        base = (char *)D_L01_001BD1C0;
        table = (char **)(base + 0x18C);
        p = table[i];
        if (*(short *)(p + 0xAA) == 0x398 || *(short *)(p + 0xAA) == 0x1BF) {
            if (D_L01_001B1E61[0] != 0)
                continue;
            func_0029F978(p, 0, D_L01_001B1E60);
            D_L01_001B1E80[0] = 2.1f;
            D_L01_001B1E84[0] = 2.1f;
            D_L01_001B1E88[0] = 2.1f;
        }
    }
}
