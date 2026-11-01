#include "include_asm.h"

/* func_L00_002A9600 in its own unit for the grouped lq/sq profile. Its first two
   callees differ by level (ref_ names). */

typedef int TI128 __attribute__((mode(TI)));

typedef struct {
    char pad000[0x80];
    TI128 position;
    TI128 unk90;
    char padA0[0x150];
    short unk1F0;
    char pad1F2[0x5E];
    float unk250;
    float unk254;
    char pad258[0xC];
    float unk264;
    char pad268[0x2028];
    unsigned char *moby;
    char pad2294[0x20];
    unsigned char unk22B4;
    unsigned char unk22B5;
} HeroView_L00_002A9600;

extern char D_00189E20[];
extern void ref_L00_002A9600_00(void);
extern void ref_L00_002A9600_01(void);
extern void func_L00_002D2E68(void);
extern void func_00282AB0(void *, void *, int);
extern void func_002A0F48(void *);
extern void func_L00_002D3778(void *, void *, float);
extern void func_L00_00310748(void *, int, void *, float, float);

/* Places the hero's moby at the hero and, when it is standing on something,
   updates its two foot contacts. */
void func_L00_002A9600(void)
{
    float v[4] __attribute__((aligned(16)));
    float t;
    unsigned char *m;

    if (((HeroView_L00_002A9600 *)D_00189E20)->unk22B5 != 0)
        ref_L00_002A9600_00();
    else
        ref_L00_002A9600_01();
    if (((HeroView_L00_002A9600 *)D_00189E20)->unk22B4 != 4)
        func_L00_002D2E68();
    ((HeroView_L00_002A9600 *)D_00189E20)->moby[0xA8] = 0xFF;
    m = ((HeroView_L00_002A9600 *)D_00189E20)->moby;
    *(TI128 *)(m + 0x10) = ((HeroView_L00_002A9600 *)D_00189E20)->position;
    *(TI128 *)(m + 0xF0) = ((HeroView_L00_002A9600 *)D_00189E20)->unk90;
    func_00282AB0(m + 0xC0, D_00189E20, 0x30);
    func_002A0F48(((HeroView_L00_002A9600 *)D_00189E20)->moby);
    if (((HeroView_L00_002A9600 *)D_00189E20)->unk1F0 != 0 || *(int *)(((HeroView_L00_002A9600 *)D_00189E20)->moby + 0x98) == 0)
        return;
    *(TI128 *)v = ((HeroView_L00_002A9600 *)D_00189E20)->position;
    func_L00_002D3778(v, v, ((HeroView_L00_002A9600 *)D_00189E20)->unk254);
    func_L00_00310748(((HeroView_L00_002A9600 *)D_00189E20)->moby, 0, v, ((HeroView_L00_002A9600 *)D_00189E20)->unk264, ((HeroView_L00_002A9600 *)D_00189E20)->unk250 - ((HeroView_L00_002A9600 *)D_00189E20)->unk254);
    t = ((HeroView_L00_002A9600 *)D_00189E20)->unk264 + 0.05f;
    *(TI128 *)v = ((HeroView_L00_002A9600 *)D_00189E20)->position;
    func_L00_002D3778(v, v, t);
    func_L00_00310748(((HeroView_L00_002A9600 *)D_00189E20)->moby, 1, v, ((HeroView_L00_002A9600 *)D_00189E20)->unk264, ((HeroView_L00_002A9600 *)D_00189E20)->unk250 - t);
}
