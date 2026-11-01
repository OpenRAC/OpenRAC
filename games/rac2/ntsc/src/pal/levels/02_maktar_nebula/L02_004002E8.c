#include "include_asm.h"

typedef unsigned short u16;
typedef unsigned int u32;
typedef short s16;
extern char *D_L02_001B359C __attribute__((sda));
extern u32 D_L02_001FC060[];
extern void func_L02_003D2390(char *);
extern void func_L02_003D4900(char *);
extern void func_L02_003DB6D8(char *);
extern void func_L02_003F18E8(char *);
extern void func_L02_0040B980(char *);
extern void func_L02_0040D180(char *);
extern char *D_L02_001B359C __attribute__((sda));
extern u32 D_L02_001FC060[];
extern char *D_L02_001B359C __attribute__((sda));
extern u32 D_L02_001FC060[];
extern void func_L02_003D2390(char *);
extern void func_L02_003D4900(char *);
extern void func_L02_003DB6D8(char *);
extern void func_L02_003F18E8(char *);
extern void func_L02_0040B980(char *);
extern void func_L02_0040D180(char *);
extern char *D_L02_001B359C __attribute__((section(".sdata")));
extern s16 func_L02_004024A0(char *);

/* Walks the 16-bit stream the object's table entry points at and calls the handler of the
   entity type each word names; a negative word ends the stream. */
void func_PAL_L02_004002E8(char *a0) {
    char *state = *(char **)(a0 + 0x68);
    u32 index = *(u32 *)(state + 0x40);
    u32 entry;
    short *p;
    char *rec;
    s16 type;

    if (index == 0xFF)
        return;
    entry = D_L02_001FC060[index];
    if (entry == 0)
        return;
    p = (short *)entry;
    do {
        rec = D_L02_001B359C + ((*(u16 *)p & 0x7FFF) << 8);
        type = *(s16 *)(rec + 0xAA);
        switch (type) {
        case 0x9CA:
            func_L02_003D2390(rec);
            break;
        case 0xB22:
            func_L02_003F18E8(rec);
            break;
        case 0xC3B:
            func_L02_0040B980(rec);
            break;
        case 0xC3E:
            func_L02_0040D180(rec);
            break;
        case 0x9F8:
            func_L02_003D4900(rec);
            break;
        case 0xA44:
            func_L02_003DB6D8(rec);
            break;
        }
    } while (*p++ >= 0);
}

