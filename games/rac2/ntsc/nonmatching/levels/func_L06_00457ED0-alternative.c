/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L06_00457ED0: 624 of 616 bytes (2 instructions over), 0 of 1 place. Same instruction
 * stream as retail apart from three things: (1) the two "list = table[id]; if (list)" tests
 * come out as likely branches (beql with the next block's lw duplicated into the slot, +1
 * instruction each) where retail has "lw v0; beqz v0; daddu s2,v0" (copy in the slot, test on
 * the loaded temp); (2) the globals base is "lui v1; lw a0,0(v1)" in mine and
 * "lui a0; lw a0,0x281C(a0)" in retail (same load, different temp); (3) branch offsets that follow.
 * Tried: assigning the list inside the condition, a separate temp copied after the test, loading
 * the id into a local, ternary table lookup (worse, 632), base pointer in a local, store order.
 * Constants s5/s6/s7 and the store order inside the body were fixed by writing obj[0x20] = 5
 * before obj[0x95] = 1.
 */
#include "common.h"
extern int func_35CBA0(char *, int, int);
extern float func_0035A780(void);
extern void func_0035A270(char *, int, int, int);
extern void func_L06_00458138(char *);
extern char *D_L06_001B281C;
extern char *D_L06_001FB3A0[];

void func_L06_00457ED0(char *a0) {
    char *s4;
    char *obj;
    char *p;
    short *list;
    char *first;
    char *base;
    int id;
    int r;

    if (*(unsigned char *)(a0 + 0x20) == 0) {
        a0[0x20] = 1;
        *(unsigned short *)(a0 + 0x34) |= 0x800;
    }
    s4 = *(char **)(a0 + 0x68);
    r = func_35CBA0(a0, 0x10000, 0);
    if (r != 0 && *(short *)(*(char **)(r + 0x20) + 0xAA) != 0xFE5) {
        if (*(int *)s4 != 0xFF && (first = D_L06_001FB3A0[*(int *)s4]) != 0) {
            list = (short *)first;
            do {
                base = D_L06_001B281C;
                obj = base + ((*(unsigned short *)list & 0x7FFF) << 8);
                if (obj[0x20] >= 0 && *(short *)(obj + 0xAA) == 0xBBA) {
                    p = *(char **)(obj + 0x68);
                    if (0.0f < *(float *)(p + 0x20)) {
                        *(float *)(p + 0x5A4) = func_0035A780();
                        qcopy(p + 0x5B0, p + 0x460);
                        *(short *)(p + 0x24C) = 0x10E;
                        obj[0x20] = 4;
                        func_0035A270(obj, 6, 0, 10);
                        *(int *)(p + 0x5A0) = 0;
                        *(short *)(p + 0x492) = 0;
                        obj[0x20] = 5;
                        obj[0x95] = 1;
                    }
                }
            } while (*list++ >= 0);
        }
        id = *(int *)(s4 + 4);
        if (id == 0xFF || (first = D_L06_001FB3A0[id]) == 0) {
            func_L06_00458138(a0);
            return;
        }
        list = (short *)first;
        do {
            base = D_L06_001B281C;
            obj = base + ((*(unsigned short *)list & 0x7FFF) << 8);
            if (obj[0x20] >= 0 && *(short *)(obj + 0xAA) == 0xBBA) {
                p = *(char **)(obj + 0x68);
                *(float *)(p + 0x5A4) = func_0035A780();
                qcopy(p + 0x5B0, p + 0x460);
                *(short *)(p + 0x24C) = 0x10E;
                obj[0x20] = 4;
                func_0035A270(obj, 6, 0, 10);
                *(int *)(p + 0x5A0) = 0;
                *(short *)(p + 0x492) = 0;
                obj[0x20] = 5;
                obj[0x95] = 1;
            }
        } while (*list++ >= 0);
        func_L06_00458138(a0);
    }
    *(unsigned char *)(a0 + 0xA8) = 0xFF;
}
