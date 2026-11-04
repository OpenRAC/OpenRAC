/* from src/levels/shared/L01_00397110-g8.c: C that compiles but does not link at its places yet */
/* Turns each of the three carried objects with a swinging pitch: it copies the carrier's
   orientation into the object, adds the level's per-frame turn and pitch to the object's own
   heading, and lets the object's tilt routine see the carrier's position.

   Measured with tools/check.py func_L01_00398938 --diff in the -G8 unit
   src/levels/shared/L01_00397110-g8.c: 352 bytes against retail's 340, at 0 of 5 places.

   What differs: this cc1 hoists five loop-invariant float constants into the loop header
   (the frame saves $f20..$f25), while retail hoists three ($f20..$f23) and re-materialises
   the two the last statement uses (-1.0471977f and 0.3490659f) inside the loop body. With
   the two extra constants the frame grows to 0x70 and the loop body gains the extra
   "lui/ori/mtc1" triples. The loop's induction variable is also moved into the loop header
   by this cc1 (the counter is decremented next to the null test), while retail keeps the
   decrement in the latch; retail's null test branches into that latch, which keeps the
   induction variable out of the header. Reading the carried object through the pointer
   (*(char **)p) rather than through a local is what reproduces retail's reload of it after
   every store and call.

   What was tried: holding the object in a local (352 -> 360 bytes, no reloads); the copy
   through a separate pointer variable; the loop as a do/while; the pitch computed into the
   local before the first call (352 -> 356 bytes, and then the argument copy differs). */
typedef int TI128 __attribute__((mode(TI)));

extern void func_002E3470(void *, void *, void *);

extern void func_002E3CB8(char *, char *);

extern void func_002E3738(void *, char *, float);

extern void func_00308938(char *);

/* The two level constants the pitch is measured against: they sit in the resident image at
   another address in every level that has this function. */
extern float ref_L01_00398938_00 __attribute__((sda));

extern float ref_L01_00398938_01 __attribute__((sda));

/* A four-float vector: the engine's VU0 helpers take and return these. */
typedef struct {
    float x;
    float y;
    float z;
    float w;
} Vec4;

void func_L01_00398938(char *a0)
{
    char *s1 = *(char **)(a0 + 0x68);
    char *p = s1 + 0x14;
    char *src = a0 + 0x10;
    Vec4 v;
    float f20;
    int i;

    for (i = 2; i >= 0; i--) {
        if (*(char **)p != 0) {
            *(TI128 *)(*(char **)p + 0x10) = *(TI128 *)src;
            *(float *)(*(char **)p + 0x18) = *(float *)(*(char **)p + 0x18) + ref_L01_00398938_00;
            *(float *)(*(char **)p + 0xF4) = ref_L01_00398938_01 * 0.017453294f;
            f20 = *(float *)(s1 + 0x20) * 2.4f;
            func_002E3CB8(*(char **)p + 0xC0, *(char **)p + 0xF0);
            func_002E3738(&v, *(char **)p + 0xC0, f20 - 1.2f);
            func_002E3470(*(char **)p + 0x10, *(char **)p + 0x10, &v);
            *(float *)(*(char **)p + 0xF4) = *(float *)(s1 + 0x24) * -1.0471977f - 0.3490659f;
            func_002E3CB8(*(char **)p + 0xC0, *(char **)p + 0xF0);
            func_00308938(*(char **)p);
        }
        p += 4;
    }
}
