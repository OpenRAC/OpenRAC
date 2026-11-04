/* func_L00_002A7DE8 in its own unit (flags in config/cflags.txt: -O2 -G0 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier). */
#include "include_asm.h"
typedef int Quad __attribute__((mode(TI)));
extern char D_00189E20[];
extern void func_L00_002A7A10(void);
typedef struct {
    char pad[0x2080];
    Quad ring[8];
    int value[8];
    float first[8];
    float second[8];
    short index;
} Ring_L00_002A7DE8;

void func_L00_002A7DE8(Quad *src, int value, float first, float second)
{
    Ring_L00_002A7DE8 *r = (Ring_L00_002A7DE8 *)D_00189E20;
    int index = r->index;

    r->ring[index] = *src;
    r->value[index] = value;
    r->first[r->index] = first;
    r->second[r->index] = second;
    func_L00_002A7A10();
    r->index++;
}
