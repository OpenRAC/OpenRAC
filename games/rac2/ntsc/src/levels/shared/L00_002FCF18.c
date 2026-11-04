#include "include_asm.h"
typedef int Q128 __attribute__((mode(TI)));

typedef struct {
    char pad0[0xC];
    void *buf;
    int used;
    char pad14[0x30 - 0x14];
} Slot_2FCF18;

typedef struct {
    float f0;
    float f4;
    float f8;
    float fC;
    float v10[4];
} Info_2FCF18;

extern Slot_2FCF18 D_L00_001C7040[];
extern Info_2FCF18 D_L00_001C6F40[];
extern char D_L00_001C71C0[][0x400];
extern void func_00282A88(void *, int);

int func_L00_002FCF18(void *pos, float a, float b, float c, float d, float e)
{
    int i;
    Info_2FCF18 *in;
    Slot_2FCF18 *s;

    for (i = 0; i < 8; i++) {
        if (D_L00_001C7040[i].used == 0)
            break;
    }
    if (i == 8)
        return -1;
    s = &D_L00_001C7040[i];
    in = &D_L00_001C6F40[i];
    *(Q128 *)in->v10 = *(Q128 *)pos;
    in->v10[3] = a;
    in->f0 = c;
    in->f4 = d;
    in->f8 = e;
    in->fC = b;
    func_00282A88(s, 0x30);
    s->used = 1;
    s->buf = D_L00_001C71C0[i];
    return i;
}
