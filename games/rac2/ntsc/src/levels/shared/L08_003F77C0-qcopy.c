#include "include_asm.h"
/* func_L08_003F77C0: the level-08 sparkle seeding, the twin of func_L00_00382CE8 with its own
 * table, colour base, life and scale constants and twelve slots. */
#include "common.h"
extern void func_00282CF0(void *,void *,void *);
extern void func_00282CC0(void *,void *,void *);
extern void func_00282D30(void *,void *,float);
extern float func_00282DB0(void *,void *);
extern float func_00282DF0(void *);
extern float func_00282C48(float);
extern float func_00283C80(float,float);
extern float func_00283C28(float);
extern float func_00283180(float);
extern float func_002A7878(float,float);
extern float func_002A78D8(float,float);
extern float func_002A7940(void);
extern int func_002A77E0(int);
extern void func_002AF000(float *,float,float,float);
extern char *func_002C2210(void *,int,unsigned int,int,int,void *,int,float,float,float);
typedef struct {
    void *owner;
    int active;
    char pad08[8];
    short size[16];
    short life[16];
    unsigned int color[16];
    float phase[16];
    float scale[16];
} Sparkle;
extern Sparkle D_L08_001B9E00;
void func_L08_003F77C0(void *owner)
{
    int i, intensity, mode, component;
    unsigned int grey;
    int green;
    float value;
    if (D_L08_001B9E00.active == 1) return;
    D_L08_001B9E00.owner = owner;
    D_L08_001B9E00.active = 1;
    i = 1;
    grey = 0x706080;
    for (; i < 12; ++i) {
        intensity = func_002A77E0(63) + 1;
        mode = func_002A77E0(6);
        switch (mode) {
        case 0: D_L08_001B9E00.color[i] = intensity + grey - (intensity << 16); break;
        case 1: D_L08_001B9E00.color[i] = (intensity << 8) + (intensity + grey); break;
        case 2: D_L08_001B9E00.color[i] = (func_002A77E0(intensity) << 8) + (intensity + grey); break;
        case 3:
            component = func_002A77E0(intensity);
            green = (func_002A77E0(intensity) << 8) + grey;
            D_L08_001B9E00.color[i] = (component << 16) + green + intensity; break;
        default: D_L08_001B9E00.color[i] = grey; break;
        }
        D_L08_001B9E00.life[i] = func_002A77E0(3) + 1;
        value = func_002A7878(-0.95f, 2.5f);
        D_L08_001B9E00.phase[i] = value;
        value = func_00282C48(func_00283180(func_00283C28(value + 1)));
        D_L08_001B9E00.scale[i] = value * value * value * 1.7f;
        D_L08_001B9E00.size[i] = 23;
    }
    D_L08_001B9E00.phase[0] = -1;
    D_L08_001B9E00.scale[0] = 2;
    D_L08_001B9E00.color[0] = 0x808080;
    D_L08_001B9E00.life[0] = 0;
    D_L08_001B9E00.size[0] = 127;
}
