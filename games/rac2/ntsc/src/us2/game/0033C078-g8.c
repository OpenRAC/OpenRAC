#include "include_asm.h"
extern int func_00336E40(void *);
extern int *func_00336DA8(int, int);
extern void func_003361E0(void *, void *, void *);
extern void func_003369A8(void *, void *, void *);
extern void func_00336710(void *, void *, void *);
extern void func_00336A30(void *, int);
extern void func_00336A38(void *, int);
extern int *func_00335E20(void *);
extern void func_00336390(void *, void *, int);
extern void func_00335F88(void *, float, float, float, float);
extern void func_00336920(void *, int, int);
extern int func_00288FB8(int);
extern int func_0033B540(char *, int);
extern char D_001ADC10[], D_001ADC18[], D_001ADC20[], D_001ADC28[];
extern char D_001ADD58[], D_001ADD60[], D_001ADD68[], D_001ADD70[], D_001ADD78[];
extern char *D_001A8CFC;

/* Constructor of an on-screen map object: stores the owner, allocates the 16-byte anchor (255, 207)
   at +0x1C8, then names and tints three sprites ("0", "1", "2" at +0x0, +0x4C, +0x98), three more
   named "3" (+0xE4, +0x130, +0x17C), three texts (Title +0x1D8, Exit +0x268, Sel +0x220) and two
   map sprites (MapSpr +0x2B0, MapPlanetSpr +0x2EC), sets their strings and places them with
   func_0033B540. The three sprite loops keep their own counters (i, j, k): sharing one counter
   keeps the loop-invariant high half of the "3" name out of a register. */
void func_US2_0033C1B8(char *self, char *owner)
{
    int i, j, k;

    *(char **)(self + 0x1CC) = owner;
    if (owner) {
        int *allocation = func_00336DA8(16, func_00336E40(owner));
        *(int **)(self + 0x1C8) = allocation;
        allocation[1] = 0;
        allocation[2] = 0;
        allocation[3] = 0;
        allocation[0] = 0;
    }
    *(int *)(self + 0x358) = 1;
    (*(float **)(self + 0x1C8))[0] = 255.0f;
    (*(float **)(self + 0x1C8))[1] = 207.0f;
    func_003361E0(self, D_001ADC10, owner);
    func_003361E0(self + 0x4C, D_001ADC18, owner);
    func_003361E0(self + 0x98, D_001ADC20, owner);
    for (i = 0; i < 3; i++)
        func_003361E0(self + 0xE4 + i * 0x4C, D_001ADC28, owner);
    func_003369A8(self + 0x1D8, D_001ADD58, owner);
    func_003369A8(self + 0x288, D_001ADD60, owner);
    func_003369A8(self + 0x230, D_001ADD68, owner);
    func_00336A30(self + 0x288, 0);
    func_00336A30(self + 0x230, 0);
    func_00336710(self + 0x2E0, D_001ADD70, owner);
    func_00336710(self + 0x31C, D_001ADD78, owner);
    *func_00335E20(self) = 0x60442D00;
    *func_00335E20(self + 0x4C) = 0x60241700;
    *func_00335E20(self + 0x98) = 0x55F0C070;
    for (j = 0; j < 3; j++)
        *func_00335E20(self + 0xE4 + j * 0x4C) = 0x80FFDE8D;
    *func_00335E20(self + 0x2E0) = 0x70A0C0C0;
    *func_00335E20(self + 0x1D8) = 0x80F0F0F0;
    *func_00335E20(self + 0x288) = 0x80F0F0F0;
    *func_00335E20(self + 0x230) = 0x80F0F0F0;
    func_00336390(self, D_001A8CFC + 0x8710, 0x87);
    func_00336390(self + 0x4C, D_001A8CFC + 0x8710, 0x88);
    func_00336390(self + 0x98, D_001A8CFC + 0x8710, 0x89);
    for (k = 0; k < 3; k++)
        func_00336390(self + 0xE4 + k * 0x4C, D_001A8CFC + 0x8710, 0x8A);
    func_00335F88(self + 0x130, 1.0f, -1.0f, 0.0f, 0.0f);
    func_00336920(self + 0x2E0, 0xE99A, 10);
    func_00335F88(self + 0x2E0, 240.0f, 194.0f, 0.0f, 0.0f);
    func_00335F88(self + 0x31C, 32.0f, 32.0f, 0.0f, 0.0f);
    *(int *)(self + 0x1D0) = 0;
    func_00336A38(self + 0x1D8, func_00288FB8(0x2BF6));
    func_00336A38(self + 0x288, func_00288FB8(0x2BE5));
    func_00336A38(self + 0x230, func_00288FB8(0x2C0B));
    func_0033B540(self, 0);
}
