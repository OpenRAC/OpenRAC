/* GPT-6.1 Sol high; USA v1.01, -O2 -G0 -ffunction-sections -fno-gcse.
 * Best check.py: 1192/1192 bytes, 0/1 places; four register differences
 * at +0xDC, +0xE4, +0xE8, +0xEC in button address/load/mask/branch.
 * Retail uses v1 for the address and v0 for the loaded/masked buttons;
 * this source uses v0 then v1. A named long local, unsigned long and
 * the existing D_00138180[0x1A0/8] array view retain that allocation.
 * Casting before the mask changes ld to lw; casting after the mask adds
 * dsll32/dsra32 (1200 bytes). Needs an independently evidenced source
 * shape or compiler allocation remedy, rather than more mask wordings.
 * All required declarations follow. No linked build was run.
 */
#include "include_asm.h"

extern int func_002A7820(int, int);
extern void func_002A7750(void *, int, int);
extern int func_002ACBC8(void *, int);
extern void func_002B0CE0(int, int, int);
extern long D_00138320;
extern char D_00189EA0[];
extern int func_002B3868(void *);
extern char *func_L30_00413490(void *);
extern void func_L30_00413528(void *, void *, char *, int, int, int);
extern unsigned char *func_002C65A0(void *, void *, int, int, int, int, int, float, float, float, int);
extern unsigned char *D_L30_001FF940[];
extern char D_L30_001B2110[];
extern char D_L30_001B2120[];
extern char D_L30_001B2130[];
extern char D_L30_001B2140[];
extern char D_L30_001B2150[];
extern char D_L30_001B2158[];
extern char D_L30_001B2160[];
extern char D_L30_001B2168[];
extern char D_L30_001B2178[];
extern char D_L30_001B2188[];
extern char D_L30_001B2198[];
extern char D_L30_001B21A8[];
extern char D_L30_001B21B8[];
extern char D_L30_001B21C0[];
extern char D_L30_001B21D0[];
extern char D_L30_001B21E0[];
extern char D_L30_001B21F0[];

typedef struct { char pad[0x4c]; short minimum, maximum; } MuseumParticleBounds;

void func_L30_004186B0(char *obj) {
    char *st = *(char **)(obj + 0x68);
    unsigned char *particle;
    switch ((unsigned char)obj[0x20]) {
    case 0:
        if (((MuseumParticleBounds *)st)->maximum < 0) ((MuseumParticleBounds *)st)->maximum = 0;
        if (((MuseumParticleBounds *)st)->minimum > ((MuseumParticleBounds *)st)->maximum) ((MuseumParticleBounds *)st)->minimum = ((MuseumParticleBounds *)st)->maximum;
        else if (((MuseumParticleBounds *)st)->minimum < 0) ((MuseumParticleBounds *)st)->minimum = 0;
        *(float *)(st+0xc) = func_002A7820(((MuseumParticleBounds *)st)->minimum, ((MuseumParticleBounds *)st)->maximum);
        func_002A7750(obj,1,-1);
        break;
    case 1:
        if (*(int *)(st+0x48) != -1 && func_002ACBC8(D_00189EA0, *(int *)(st+0x48))) {
            func_002B0CE0(1,0x27e6,1);
            if ((D_00138320 & 3) == 3) func_002A7750(obj,2,-1);
        }
        break;
    case 2:
        if (func_002B3868(obj)) {
            *(char **)(st+0x3c) = func_L30_00413490(obj);
            func_L30_00413528(*(char **)(st+0x3c), st+0x0, D_L30_001B2110, 1, -1000, 1000);
            func_L30_00413528(*(char **)(st+0x3c), st+0x4, D_L30_001B2120, 1, -1000, 1000);
            func_L30_00413528(*(char **)(st+0x3c), st+0x8, D_L30_001B2130, 1, -1000, 1000);
            func_L30_00413528(*(char **)(st+0x3c), st+0x10, D_L30_001B2140, 0, 1, 1000);
            func_L30_00413528(*(char **)(st+0x3c), st+0x14, D_L30_001B2150, 0, 0, 255);
            func_L30_00413528(*(char **)(st+0x3c), st+0x18, D_L30_001B2158, 0, 0, 255);
            func_L30_00413528(*(char **)(st+0x3c), st+0x1c, D_L30_001B2160, 0, 0, 255);
            func_L30_00413528(*(char **)(st+0x3c), st+0x20, D_L30_001B2168, 0, -1000, 1000);
            func_L30_00413528(*(char **)(st+0x3c), st+0x24, D_L30_001B2178, 0, 0, 127);
            func_L30_00413528(*(char **)(st+0x3c), st+0x28, D_L30_001B2188, 0, 0, 127);
            func_L30_00413528(*(char **)(st+0x3c), st+0x2c, D_L30_001B2198, 1, 0, 1000);
            func_L30_00413528(*(char **)(st+0x3c), st+0x30, D_L30_001B21A8, 1, 0, 1000);
            func_L30_00413528(*(char **)(st+0x3c), st+0x34, D_L30_001B21B8, 1, -1000, 1000);
            func_L30_00413528(*(char **)(st+0x3c), st+0x40, D_L30_001B21C0, 0, 0, 103);
            func_L30_00413528(*(char **)(st+0x3c), st+0x44, D_L30_001B21D0, 0, 0, 20);
            func_L30_00413528(*(char **)(st+0x3c), st+0x4c, D_L30_001B21E0, 3, 0, 1000);
            func_L30_00413528(*(char **)(st+0x3c), st+0x4e, D_L30_001B21F0, 3, 0, 1000);
        }
        func_002B0CE0(1,0,1);
        if (!*(char **)(st+0x3c) || *(short *)(*(char **)(st+0x3c)+0xaa) != 0x118a || (unsigned char)(*(char **)(st+0x3c))[0x20] == 0xfe || (unsigned char)(*(char **)(st+0x3c))[0x20] == 0xfd)
            func_002A7750(obj,1,-1);
        break;
    }
    if ((*(float *)(st+0xc))-- == 0.0f) {
        if (((MuseumParticleBounds *)st)->minimum > ((MuseumParticleBounds *)st)->maximum) ((MuseumParticleBounds *)st)->minimum = ((MuseumParticleBounds *)st)->maximum;
        else if (((MuseumParticleBounds *)st)->minimum < 0) ((MuseumParticleBounds *)st)->minimum = 0;
        *(float *)(st+0xc) = func_002A7820(((MuseumParticleBounds *)st)->minimum, ((MuseumParticleBounds *)st)->maximum);
        particle = func_002C65A0(obj+0x10, st, *(int *)(st+0x10), (*(int *)(st+0x1c)<<16) | (*(int *)(st+0x18)<<8) | *(int *)(st+0x14), *(int *)(st+0x20), *(int *)(st+0x24), *(int *)(st+0x28), *(float *)(st+0x2c)*210000.0f, *(float *)(st+0x30)*210000.0f, *(float *)(st+0x34), *(int *)(st+0x38));
        if (particle) {
            particle[9] = 0xff;
            particle[2] = D_L30_001FF940[*(int *)(st+0x40)][*(int *)(st+0x44)];
        }
    }
}
