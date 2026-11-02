/*
 * Near miss, 28 October 2026 (worker 6): -O2 -G8 -ffunction-sections, own unit L00_003F5750-g8.c.
 * 288/288 bytes, 4 masked diff lines (was a swap of s0/s1 plus pointer-add operand order).
 * New: int arithmetic with the record offset FIRST, `(char *)(offset + (int)*entry + 0x10)` and
 * `entry = (char **)((*(short *)(state + 0x18) << 2) + (int)table)`, as the neighbouring L00 walkers
 * write it; that fixed the saved-register order and every addu operand order.
 * Left: after the func_002A7878 call retail loads `lwc1 $f1,0x195C($gp)` before `addu $v0,$s1,$v0`
 * and `lwc1 $f13,0x1964($gp)` before `lw $v0,0($s0)`; ours loads them one slot later. Statement
 * reorders, dropping low/next/high locals, separate result locals: all stay at 4.
 */
#include "common.h"
typedef int TI128 __attribute__((mode(TI)));
extern char *func_L00_0032A088(int,int);
extern float func_00283CE0(int);
extern float func_002ABB18(float);
extern float func_002A7878(float,float);
extern float func_002A78D8(float,float);
extern void func_L00_003F5870(char *,int);
extern float D_L00_001B0940 __attribute__((sda));
extern float D_L00_001B0944 __attribute__((sda));
extern float D_L00_001B0948 __attribute__((sda));
extern float D_L00_001B094C __attribute__((sda));
extern float D_L00_001B0950 __attribute__((sda));
extern float D_L00_001B0954 __attribute__((sda));

void func_L00_003F5750(char *object, char *position)
{
    char *state = *(char **)(object + 0x68);
    char **table = (char **)func_L00_0032A088(0xc04, *(short *)(state + 0x1a));
    short index = *(short *)(state + 0x20);
    char **entry;
    int offset = index * 0x30;
    float degrees, result, next, low, high;
    entry = (char **)((*(short *)(state + 0x18) << 2) + (int)table);
    qcopy((char *)(offset + (int)*entry + 0x10),position);
    degrees = func_00283CE0(index) * 0.017453294f;
    result = func_002ABB18(degrees * D_L00_001B0948);
    low = D_L00_001B0940; high = D_L00_001B0944;
    *(float *)(offset + (int)*entry + 0x20) = result;
    result = func_002A7878(low,high);
    low = D_L00_001B0950; next = D_L00_001B094C;
    *(float *)(offset + (int)*entry + 0x28) = result * 0.017453294f;
    high = D_L00_001B0954;
    *(float *)(offset + (int)*entry + 0x24) = next;
    result = func_002A78D8(low,high);
    *(float *)(offset + (int)*entry + 0x2c) = result * 0.016666668f;
    func_L00_003F5870(*entry,index);
    (*(unsigned short *)(state + 0x20))++;
}
