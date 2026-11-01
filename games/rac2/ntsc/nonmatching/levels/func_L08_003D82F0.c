typedef unsigned int ti_word __attribute__((mode(TI)));
typedef struct { char pad0[0x10]; ti_word copy; char pad1[0x48]; char *state; } TaboraObject;
extern float func_L08_003D8290(void *, float, float);

void func_L08_003D82F0(void *raw) {
    TaboraObject *object = (TaboraObject *)raw;
    char *state = object->state;
    float value = *(float *)(state + 0x3E8);
    ti_word copy = object->copy;
    *(float *)(state + 0x434) = func_L08_003D8290(&copy, value + 3.0f, value - 20.0f);
}

/* Wave 24 (worker 05): the same wall, measured again from the retail body.
   The retail keeps the address of the transfer in a register and uses a zero
   displacement:

     addiu $v1, $a0, 0x10
     ...
     lq    $v0, 0x0($v1)
     sq    $v0, 0x0($sp)

   while this cc1 folds the constant offset into the lq (`lq $v0,0x10($v1)`)
   and copies the argument into $v1 for it, so $a0 is dead before the call
   and the whole tail of the body moves.  check.py (08_tabora): "instruction
   at +0x4 differs" - the first difference is that addiu.

   What is measured and right: the copy is a mode(TI) object; the argument of
   the call is the local (retail `daddu $a0,$sp,$zero`); the moby pointer
   comes from the argument's 0x68 and is used after the call, so it has to be
   in a callee-saved register (retail $s0); the call takes f0 + 3.0f first
   and f0 - 20.0f second (retail `add.s $f12,$f0,$f12` before the jal and
   `sub.s $f13,$f0,$f13` in its delay slot); the result goes to moby + 0x434.  */
extern float func_L08_003D8290(void *, float, float);

void func_L08_003D82F0_w24(void *a0) {
    ti_word local[1];
    int moby;
    float f0;

    local[0] = *(ti_word *)((char *)a0 + 0x10);
    moby = *(int *)((char *)a0 + 0x68);
    f0 = *(float *)(moby + 0x3E8);
    f0 = func_L08_003D8290(&local[0], f0 + 3.0f, f0 - 20.0f);
    *(float *)(moby + 0x434) = f0;
}
