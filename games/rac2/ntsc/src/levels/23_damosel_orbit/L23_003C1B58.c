#include "include_asm.h"

/* func_L23_003C1B58 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give retail's grouped `lq`/`sq`
   with register addresses and keep the instructions around the copy on their own side of it. */

typedef int TI128 __attribute__((mode(TI)));

extern void func_00282D30(void *, void *, float);

extern void func_00282CC0(void *, void *, void *);

extern char D_00189EA0[];

extern char D_00189E20[];

extern char *D_0018C0B0;

extern int D_L23_001B1644 __attribute__((sda));

extern int D_L23_001B164C __attribute__((sda));

extern int D_L23_001B36AC __attribute__((sda));

extern void *D_L23_002AB170[];

extern void func_L23_00418650(char *, float);

extern int func_002A77E0(int);

extern void func_L23_0041C6B0(char *, char *, int);

extern void func_L23_003C00D0(char *);

extern void func_002A7750(char *, int, int);

extern void func_L23_003BFFC0(char *, int, int, int, int, float);

/* Eight ways to place the offset vector v[6] (which of the two vectors at 0xC0 and 0xD0 of the
   target, or the average of two of them), picked by func_002A77E0(8). The dispatch reads the
   level's own jump table (eight code addresses in the level's data at 0x2AB170); `cases` only
   keeps the labels alive. */
void func_L23_003C1B58(char *arg0) {
    static void *cases[8] = {&&c0, &&c1, &&c2, &&c3, &&c4, &&c5, &&c6, &&c7};
    char *state = *(char **)(arg0 + 0x68);
    TI128 v[7];
    unsigned int sel;

    v[1] = *(TI128 *)D_00189EA0;
    func_L23_00418650((char *)&v[1], 0.0f);
    func_00282D30((char *)&v[0], *(char **)(D_00189EA0 + 0x2210) + 0xE0, -80.0f);
    sel = func_002A77E0(8);
    if (sel < 8)
        goto *D_L23_002AB170[sel];
    goto tail;
c0:
    func_00282D30((char *)&v[6], D_0018C0B0 + 0xC0, 50.0f);
    goto tail;
c1:
    func_00282D30((char *)&v[6], D_0018C0B0 + 0xC0, -50.0f);
    goto tail;
c2:
    func_00282D30((char *)&v[6], D_0018C0B0 + 0xD0, 50.0f);
    goto tail;
c3:
    func_00282D30((char *)&v[6], D_0018C0B0 + 0xD0, -50.0f);
    goto tail;
c4: {
    char *b = D_00189E20;

    func_00282D30((char *)&v[4], *(char **)(b + 0x2290) + 0xD0, 50.0f);
    func_00282D30((char *)&v[2], *(char **)(b + 0x2290) + 0xC0, 50.0f);
    func_00282CC0((char *)&v[6], (char *)&v[4], (char *)&v[2]);
    func_00282D30((char *)&v[6], (char *)&v[6], 0.5f);
    goto tail;
}
c5: {
    char *b = D_00189E20;

    func_00282D30((char *)&v[2], *(char **)(b + 0x2290) + 0xC0, 50.0f);
    func_00282D30((char *)&v[5], *(char **)(b + 0x2290) + 0xD0, -50.0f);
    func_00282CC0((char *)&v[6], (char *)&v[2], (char *)&v[5]);
    func_00282D30((char *)&v[6], (char *)&v[6], 0.5f);
    goto tail;
}
c6: {
    char *b = D_00189E20;

    func_00282D30((char *)&v[5], *(char **)(b + 0x2290) + 0xD0, -50.0f);
    func_00282D30((char *)&v[3], *(char **)(b + 0x2290) + 0xC0, -50.0f);
    func_00282CC0((char *)&v[6], (char *)&v[5], (char *)&v[3]);
    func_00282D30((char *)&v[6], (char *)&v[6], 0.5f);
    goto tail;
}
c7: {
    char *b = D_00189E20;

    func_00282D30((char *)&v[3], *(char **)(b + 0x2290) + 0xC0, -50.0f);
    func_00282D30((char *)&v[4], *(char **)(b + 0x2290) + 0xD0, 50.0f);
    func_00282CC0((char *)&v[6], (char *)&v[3], (char *)&v[4]);
    func_00282D30((char *)&v[6], (char *)&v[6], 0.5f);
}
tail:
    func_00282CC0((char *)&v[0], (char *)&v[0], (char *)&v[6]);
    func_00282CC0((char *)&v[0], (char *)&v[0], (char *)&v[1]);
    func_L23_0041C6B0(arg0 + 0x10, (char *)&v[0], 1);
    func_L23_00418650(arg0 + 0x10, 0.0f);
    *(TI128 *)(state + 0x290) = *(TI128 *)(arg0 + 0x10);
    func_L23_003C00D0(arg0);
    func_002A7750(arg0, 5, 0);
    func_L23_003BFFC0(arg0, 3, 0, 0x14, 3, 1.0f);
    D_L23_001B164C = (D_L23_001B164C + 1) % D_L23_001B1644;
    D_L23_001B36AC = D_L23_001B36AC + 1;
}
