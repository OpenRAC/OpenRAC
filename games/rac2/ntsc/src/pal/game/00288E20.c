#include "include_asm.h"

typedef struct {
    unsigned int marker;
    unsigned char reserved04[0x38];
    unsigned short class_code;
    unsigned char reserved3E[0xa2];
} MappedClassEntry;
typedef unsigned char u8;
extern unsigned char D_00139568[];
extern MappedClassEntry D_L00_00262570[];
typedef struct {
    void *slot[6];
    int flag;
} SlotTable;
extern SlotTable ref_L00_00288508_00;
extern unsigned short D_0018C964;
extern int func_00288E18(int, int);
extern void func_002B0A18(int, int);

/* Pick the action of the mapped class of PARAM, then hand it to the game. */
void func_PAL_00288E20(int param)
{
    SlotTable table;
    int code;
    int i;

    if (param == 10) {
        func_00288E18(0x9F8, 0x4F);
    } else if (param == 27) {
        func_00288E18(0x9F7, 0x4E);
    } else if (D_0018C964 == 0) {
        func_00288E18(0x9AA, 1);
    }

    code = -1;
    table = ref_L00_00288508_00;
    {
        MappedClassEntry *entry = &D_L00_00262570[D_00139568[param]];
        int *p = 0;
        if (entry->reserved04[0] < 6) {
            p = (int *)table.slot[((u8 *)entry)[4]];
        }
        if (p) {
            for (i = 0; p[i] != -1; i += 2) {
                if (p[i] == param) { code = p[i + 1]; break; }
            }
        }
    }
    if (param == 10) {
        u8 *entry2 = (u8 *)&D_L00_00262570[D_00139568[10]];
        int kind2 = entry2[4];
        if (kind2 >= 2) code = 0x1296;
        if (kind2 >= 3) code = 0x1297;
        if (kind2 >= 4) code = 0x1296;
        if (kind2 >= 5) code = 0x1297;
    }
    if (code >= 0)
        func_002B0A18(code, -1);
    else
        func_002B0A18(0, -1);
}

