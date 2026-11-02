#include "include_asm.h"

extern int ref_L00_002A3B80_00;

int func_L00_002A3B80(void)
{
    return ref_L00_002A3B80_00;
}

INCLUDE_ASM("asm/levels", func_L00_002A3B88);

INCLUDE_ASM("asm/levels", func_L00_002A4928);

extern char D_00189E20[];
extern int D_001A79F0;
extern void func_L00_002D24B8(void);
extern int func_L00_002C42E0(int, int);
extern void func_L00_003B09F8(void);
/* Incomplete sda array: retail clears this word as `lui $at; sw` (COMPILER-NOTES.md). */
extern int D_001A73A0[] __attribute__((sda));

void func_L00_002A4D18(void)
{
    char *base = D_00189E20;

    *(float *)(base + 0x264) = 0.45f;
    *(float *)(base + 0x250) = 0.8f;
    *(float *)(base + 0x254) = 0.7f;
    *(float *)(base + 0x258) = 0.8f;
    *(float *)(base + 0x25C) = 0.7f;
    *(unsigned char *)(base + 0x24F0) = D_001A79F0 == 0x1A;
    D_001A73A0[0] = 0;
    *(int *)(base + 0x2294) = 0;
    *(float *)(base + 0xC24) = 1.0f;
    func_L00_002D24B8();
    func_L00_002C42E0(0, 1);
    func_L00_003B09F8();
}

extern void func_002A1130(void *a0, int a1, int a2, int a3);

extern void func_00282B50(int a0, int a1, int a2);

extern void func_0011AEA0(int a0);

/* walks one loaded entry of an owner's list and hands it to the vector routine when the
   tag bits are set, else to the count-driven copy, then polls and returns the caller's
   handle */
int func_L00_002A4DB0(char *a0, int a1, int a2, int a3)
{
    char *owner = *(char **)(a0 + 0x24);
    char **rows = (char **)(owner + 0x48);
    char *row = rows[a1];
    int *words = (int *)(row + 0x1C);
    int entry = words[a2];

    if (entry & 0xF0000000)
        func_002A1130(row + 0x1C + (*(unsigned char *)(row + 0x10) << 2) - -(*(unsigned char *)(row + 0x13) << 2), entry, a3, *(unsigned char *)(owner + 8));
    else
        func_00282B50(a3, entry, (*(short *)(entry + 6) + 1) << 4);
    func_0011AEA0(0);
    return a3;
}
