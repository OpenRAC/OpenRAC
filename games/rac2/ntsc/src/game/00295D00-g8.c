#include "include_asm.h"

/* The map screen's pan/zoom update. Its unit is -G8 -g: the two screen-size words
   are $gp reads and the divisions carry the statement-boundary padding. */

extern int func_00295C70(void);
extern int func_002E5E58(int, unsigned int, void *);
extern int D_001C4FA0[];
extern void func_00298170(void);
extern char D_00138180[];
extern int D_001A95C0 __attribute__((sda));
extern int D_001A95C4 __attribute__((sda));

/* Map screen pan/zoom update: returns 1 (after func_002E5E58(0x13)) while the pad
   word +0x1C4 has 0x510 set, 0 when the map has nothing open (+0x24) or no slot
   (+0x234 < 0). Otherwise scales the slot's zoom (+0xB4) by 1 - speed * 0.02,
   clamps it to [0.65, 4], pans x/y (+0x108 / +0x15C) by the pad velocities times
   3000000 / zoom and clamps them to [lo, 0x10000000 - lo]. */
int func_00295D00(void)
{
    char *pad;
    char *m;
    float *zoom;
    int *xs;
    int *ys;

    func_00298170();
    func_00295C70();
    pad = D_00138180;
    if (*(int *)(pad + 0x1C4) & 0x510) {
        func_002E5E58(0x13, 0, 0);
        return 1;
    }
    m = (char *)D_001C4FA0;
    if (*(int *)(m + 0x24) == 0)
        return 0;
    if (*(int *)(m + 0x234) < 0)
        return 0;
    zoom = (float *)(m + 0xB4);
    zoom[*(int *)(m + 0x234)] *= 1.0f - *(float *)(pad + 0x144) * 0.02f;
    if (zoom[*(int *)(m + 0x234)] > 4.0f)
        zoom[*(int *)(m + 0x234)] = 4.0f;
    if (zoom[*(int *)(m + 0x234)] < 0.65f)
        zoom[*(int *)(m + 0x234)] = 0.65f;
    xs = (int *)(m + 0x108);
    ys = (int *)(m + 0x15C);
    {
        float scale = 3000000.0f / zoom[*(int *)(m + 0x234)];
        xs[*(int *)(m + 0x234)] += (int)(*(float *)(pad + 0x148) * scale);
        ys[*(int *)(m + 0x234)] += (int)(*(float *)(pad + 0x14C) * scale);
    }
    {
        float z = zoom[*(int *)(m + 0x234)];
        int xlo = (int)((float)(0x1000 - D_001A95C0 * 16) / z) << 15;
        int ylo = (int)((float)(0xD00 - D_001A95C4 * 16) / z) << 15;
        int xhi = 0x10000000 - xlo;
        int yhi = 0x10000000 - ylo;

        if (xs[*(int *)(m + 0x234)] < xlo)
            xs[*(int *)(m + 0x234)] = xlo;
        if (xs[*(int *)(m + 0x234)] > xhi)
            xs[*(int *)(m + 0x234)] = xhi;
        if (ys[*(int *)(m + 0x234)] < ylo)
            ys[*(int *)(m + 0x234)] = ylo;
        if (ys[*(int *)(m + 0x234)] > yhi)
            ys[*(int *)(m + 0x234)] = yhi;
    }
    return 0;
}
