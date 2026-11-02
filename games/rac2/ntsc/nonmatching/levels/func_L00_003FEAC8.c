/* GPT-6.1 Sol low, v0385, current upstream compiler, USA v1.01.
 * 212/212 bytes, 0/27 places; base/index s0/s1 allocation and mult/move order differ. Four total forms measured; late base initialization gives 208 bytes with correct saved register assignment but worse scheduling. Profile: -O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier.
 * Required declarations follow below.
 */
/* 26 October 2026 (M24): -O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier, 212/212 bytes, 9 lines off, registers only: base and index swap s0/s1 (and one mult/move order). Entry through a pointer local, the last callee with its floats before the count, the slot store through a struct view. In the permuter farm. */
/* from src/levels/shared/L00_003F8E08-g8.c: C that compiles close to the retail's bytes
   but does not match; needs a -G8 unit (the body reads level data through $gp).

   Measured in wave 8: with the opt-in quadword-copy profile
   (-O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier) the body is 208
   bytes against the retail's 212 (204 in the default -G0 profile).  What is left is
   the address of the two globals and the register of the product:
     retail: lui $a0,0x14 ; lbu $v0,-0x6A7F($a0)          (the byte at 0x00139581)
             lui $v1,0x26 ; addiu $v1,$v1,0x2570 ; mult $v0,$a1 ; addu $v0,$v0,$v1
     ours:   lui $v1,%hi(D_00139581) ; addiu %lo ; lbu $t0,0(%v0)
             lui $v0,%hi(D_L00_00262570) ; addiu %lo ; mult $t0,$a0 ; addu $v0,$v0,$t0
   so the retail loads the boot's byte through the one-register form (the %hi in a
   register, the %lo as the displacement) and keeps the table's base in a register of
   its own, while this cc1 materialises the full address of both globals and puts the
   product in $t0.  The two globals are declared as arrays here; a scalar declaration
   is the next thing to try. */
typedef int HeroQ __attribute__((mode(TI)));
typedef union { unsigned char b[0x2600]; signed char c[0x2600]; short h[0x1300]; unsigned short uh[0x1300]; int i[0x980]; unsigned int u[0x980]; float f[0x980]; char *ptr[0x980]; HeroQ q[0x260]; } Hero;
extern unsigned char D_00189E20[];
#define H ((Hero *)D_00189E20)


/* The body: the object keeps a pointer to a table of 0xE0-byte records at +0x68; the
   record the boot's byte at 0x00139581 picks supplies one float; the new object is
   handed to the level's own copy of func_00282F20 and the result is written back. */

#ifndef RAC2_T_QUAD_3FEAC8
#define RAC2_T_QUAD_3FEAC8
typedef int Quad_3feac8 __attribute__((mode(TI)));
#endif

#ifndef RAC2_T_ENTRY_262570_3FEAC8
#define RAC2_T_ENTRY_262570_3FEAC8
/* One record of the level's table of 0xE0-byte entries at D_L00_00262570. */
typedef struct {
    unsigned char pad00[0x70];
    float f70;
    unsigned char pad74[0xE0 - 0x74];
} Entry_262570_3feac8;
#endif

/* The boot's resident byte that picks the table row (0x00139581). */
extern unsigned char D_00139581[];

/* The level's own table of records, and the small-data float it hands to the last call. */
extern Entry_262570_3feac8 D_L00_00262570[];
extern float D_L00_001B0C14 __attribute__((sda));

/* The bodies the retail calls reach, at 0x3FEBA0, 0x2EE238, 0x31A768 and 0x3A98E8 in
   level 00; each sits at another address in the other levels. */
extern void ref_L00_003FEAC8_00(void *, void *, void *, int);
extern float ref_L00_003FEAC8_01(void *);
extern int ref_L00_003FEAC8_02(int, int);
extern int ref_L00_003FEAC8_03(void *, void *, void *, float, float, int, float);

typedef struct { int pad[4]; int spawned[1]; } StateSlots;

void func_L00_003FEAC8(char *self, char *arg1, Quad_3feac8 *arg2, int index)
{
    StateSlots *base = *(StateSlots **)(self + 0x68);
    Quad_3feac8 vec;
    float f21;
    Entry_262570_3feac8 *entry;
    float f20;
    int r;

    vec = *arg2;
    entry = D_L00_00262570 + D_00139581[0];
    f21 = entry->f70;
    ref_L00_003FEAC8_00(self, arg1, arg2, index);
    f20 = ref_L00_003FEAC8_01(&vec);
    r = ref_L00_003FEAC8_02(1, 30);
    r = ref_L00_003FEAC8_03(arg1, &vec, self, f20, f21, r, D_L00_001B0C14);
    base->spawned[index] = r;
    if (r != 0) {
        *(int *)(r + 0xB8) = self;
    }
}
