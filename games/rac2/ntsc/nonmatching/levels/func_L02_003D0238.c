/* OF2 (2026-10-26): honest form, 188 of 184 bytes in a -g8 unit with
   "-O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier -frac2-zero-ti-barrier".
   D_L02_001B013C as `int [4] __attribute__((sda))` (sized sda array > 8 bytes) gives
   retail's `lui $a1 ; lw $a1,0x13C($a1)`; D_L02_001B0138 stays a $gp scalar. The wall
   left: cse keeps self+0x10 (the first copy's destination register) in $s2 from the
   prologue to the last call, where retail recomputes it (`addiu $a0,$s1,0x10` for the
   copy, `addiu $a1,$a0,0x10` for the call); that also swaps $s0/$s1 and adds a save.
   An inline wrapper for the last call and `((Q128 *)self)[1]` did not break the CSE.
   The zero store still comes out `por ; sq $v0,0x50($s1)` against retail's
   `addiu $s0,$s0,0x50 ; sq $zero,0($s0)`. */
/* flags: -O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier -frac2-zero-ti-barrier; file src/levels/02_maktar_nebula/L02_003D0238-g8.c (honest: Q128 copies, no inline-asm qcopy) */
#include "common.h"

typedef int Q128 __attribute__((mode(TI)));

extern int D_L02_001B0138 __attribute__((sda));
extern int D_L02_001B013C[4] __attribute__((sda));
extern int func_L01_0031BC68(int, int, float);
extern void func_002A7750(char *, int, int);
extern void func_L00_003294F0(char *, int);
extern int func_L02_002EF388(void *, void *, void *, void *, int);

static __inline__ void finish(char *m)
{
    func_L02_002EF388(m, m + 0x10, m + 0xF0, 0, 0);
}

void func_L02_003D0238(char *self)
{
    char *state = *(char **)(self + 0x68);

    *(int *)(state + 0xD8) = 0;
    *(int *)(self + 0x78) = func_L01_0031BC68(D_L02_001B0138, D_L02_001B013C[0], *(float *)(state + 0xD8));
    func_002A7750(self, 1, -1);
    *(Q128 *)(self + 0x10) = *(Q128 *)(state + 0xA0);
    *(Q128 *)(state + 0x60) = *(Q128 *)(state + 0xA0);
    *(Q128 *)(state + 0x50) = 0;
    self[0x31] = 1;
    *(unsigned short *)(self + 0x34) = (*(unsigned short *)(self + 0x34) & 0xFFFE) | 0x1000;
    func_L00_003294F0(self, 0);
    finish(self);
}

#if 0
/*
 * Complete manual reconstruction retained as nonmatching; no exact-C, linked-build
 * or runtime credit. Fresh worker checks on 2026-10-25, base dcfcd1f90746f3fbba715825d25bf19e88bf875c.
 * Tested profile: -O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier -frac2-zero-ti-barrier -mno-split-addresses -fno-schedule-insns
 * Coverage: US: 0/1 placements; US2: 0/1 placements; PAL: 0/1 placements.
 * US compiled size: 192 bytes; retail: 184 bytes.
 * Remaining difference: 192 bytes versus 184. A self-position pointer is kept in S2 across three calls, producing an extra save/restore; retail recomputes that address. The actual allocator dump confirms the three-call live range. Helper and address/scheduling variants tied.
 * Reopen only with: A genuine field lifetime or qualified API contract that causes address recomputation without artificial barriers, register pins or false alias information.
 * Earlier complete variants and command logs are preserved privately.
 */
#include "common.h"
typedef int Quad __attribute__((mode(TI)));
extern int D_L02_001B0138 __attribute__((sda));
extern int D_L02_001B013C NOT_SDA;
extern int func_L01_0031BC68(int, int, float);
extern void func_002A7750(u8 *, int, int), func_L00_003294F0(u8 *, int);
extern int func_L02_002EF388(void *, void *, void *, void *, int);
/* Restores the actor's saved spawn point and registers its hidden proxy. */
/* Partial actor prefix used only for its measured state and vector fields. */
typedef struct { u8 prefix[0x10]; Quad position; u8 gap[0x48]; char *state; } L02_SpawnActorView;
void func_L02_003D0238(char *self) {
    L02_SpawnActorView *actor=(L02_SpawnActorView *)self;
    char *state = actor->state;
    *(int *)(state + 0xD8) = 0;
    *(int *)(self + 0x78) = func_L01_0031BC68(D_L02_001B0138, D_L02_001B013C, *(float *)(state + 0xD8));
    func_002A7750((u8 *)self, 1, -1);
    qcopy(&actor->position,state + 0xA0);
    qcopy(state + 0x60,state + 0xA0);
    qzero(state + 0x50);
    self[0x31] = 1;
    *(u16 *)(self + 0x34) = (*(u16 *)(self + 0x34) & 0xFFFE) | 0x1000;
    func_L00_003294F0((u8 *)self, 0);
    func_L02_002EF388(self, &actor->position, self + 0xF0, 0, 0);
}

#endif
