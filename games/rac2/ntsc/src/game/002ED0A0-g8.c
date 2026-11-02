/* func_002ED0A0: the state block is one struct global (retail bases every field on
   &D_001B8840 and re-derives it from the kept %hi after the calls). The colour loop counts
   j up with the source table indexed by it: loop.c reverses the counter and emits the
   reduced table pointer and count after the hoisted invariants, retail's preheader order. */
#include "include_asm.h"
typedef struct { char pad0[0x30]; int a30; int count34; char pad38[0xC]; short n44; char pad46[0xA]; char *obj50; char pad54[0x138]; char *list[1]; } State_2ED0A0;
extern State_2ED0A0 D_001B8840;
extern void func_00282CC0(unsigned char *, unsigned char *, unsigned char *);
/* Sets up the four fading quads of one record in the resident state block:
   picks the 0xD54 record, rebuilds its 0x10-byte blocks through the colour
   helpers and stores the record's first word into the state's draw object. */
extern int D_001AC200 __attribute__((sda));
extern char D_002628D0[];
extern char D_002628F0[];
extern char D_001B5340[];

extern void func_00283098(char *, char *, char *);
extern void func_00282CF0(char *, char *, char *);
extern void func_00282F20(char *, char *, float);

void func_002ED0A0(void)
{
    char *record;
    int index = -1;
    int flag = 0;
    int limit = 0x270F;
    int i;

    if (D_001B8840.a30 == 0) {
        index = 0x32;
        flag = 1;
    }
    if (index == -1)
        return;
    if (D_001B8840.count34 < index)
        return;
    if (limit < D_001B8840.count34)
        return;
    D_001B8840.obj50 = D_002628D0;
    record = 0;
    for (i = 0; i < D_001B8840.n44; i++) {
        char *entry = D_001B8840.list[i];

        if (*(short *)(entry + 0xAA) == 0xD54) {
            record = entry;
            break;
        }
    }
    if (record != 0) {
        char *base = D_002628F0;
        char buf[16];
        int j;

        for (j = 0; j < 4; j++) {
            func_00283098(base, (char *)&D_001AC200 + j * 0x10, record + 0xC0);
            func_00282CC0((unsigned char *)base, (unsigned char *)base, (unsigned char *)(record + 0x10));
            func_00282CF0(buf, D_001B5340, base);
            func_00282F20(buf, buf, 0.15f);
            func_00282CC0((unsigned char *)base, (unsigned char *)base, (unsigned char *)buf);
            base += 0x10;
        }
    }
    *(int *)(D_001B8840.obj50 + 0x68) = (int)D_001B8840.list[flag];
}
