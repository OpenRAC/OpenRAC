#include "include_asm.h"
extern int D_001B1688;
extern char *D_001B1B68;
extern float ref_L00_00330BC0_00 __attribute__((sda));
extern float func_00282E48(void *, void *);
extern void func_00302440(void *, void *);

void func_PAL_00332720(char *object)
{
    char *cursor;
    if (D_001B1688 & 1) {
        cursor = D_001B1B68;
        while (cursor) {
            int kind = *(unsigned char *)(cursor + 0x20);
            if (kind == 0xfe) return;
            if (kind == 0xfd) return;
            if (cursor && *(char **)(cursor + 0x24) &&
                *(short *)(*(char **)(cursor + 0x24) + 0x46) == 0x13) {
                if (func_00282E48(cursor + 0x10, object + 0x10) < ref_L00_00330BC0_00)
                    func_00302440(cursor, object);
            }
            cursor = *(char **)(cursor + 0x28);
        }
    }
}
