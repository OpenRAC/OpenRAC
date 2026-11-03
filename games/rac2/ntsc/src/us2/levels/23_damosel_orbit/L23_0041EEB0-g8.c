#include "include_asm.h"
extern char *D_L23_001B1484 __attribute__((sda));
extern int D_L23_001B1424 __attribute__((sda));
extern unsigned char D_001395B8[];
extern void func_0029B758(int, int);
extern void func_0029CE80(void);
extern void func_0029F338(void *);

/* Releases the five slot objects and the level object, then raises three flags. The flags are
 * written through a pointer set after the calls, as retail's `lui`/`addiu` base with the 0x88 and
 * 0xA1 displacements; written as D_001395B8[0xA1] and [0x88], the high half is hoisted into a
 * saved register (160 bytes). */
void func_US2_L23_0041EEB0(void) {
    char **p = (char **)(*(char **)(D_L23_001B1484 + 0x68) + 0x358);
    unsigned char *flags;
    int i;

    for (i = 4; i >= 0; i--) {
        if (*p != 0) {
            func_0029F338(*p);
            *p = 0;
        }
        p++;
    }
    if (D_L23_001B1484 != 0) {
        func_0029B758(0, 0);
        func_0029CE80();
        func_0029F338(D_L23_001B1484);
        D_L23_001B1484 = 0;
    }
    flags = D_001395B8;
    D_L23_001B1424 = 1;
    flags[0xA1] = 1;
    flags[0x88] = 1;
}
