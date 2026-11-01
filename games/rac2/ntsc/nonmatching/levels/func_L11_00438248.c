/* GPT-6.1 Sol (high), task v0783, USA v1.01.
   Best: 212/212 bytes, 0/1 places, 13 instructions differ under
   -O2 -G0 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier -g.
   Every remaining difference is the consistent s2/s3 register interchange,
   including the order of their prologue saves. The loop counter is now s4,
   and the initial position-copy address and all instruction scheduling agree.
   Compute the first walking pointer after the initial scale call, separately
   from the first copy destination, to obtain this result. Reusing that pointer
   for obj+0xC0 and moving next-pointer declarations into the loop both tie.
   Next: independently evidenced pointer lifetimes or allocation priority.
   Required declarations are included below. */
#include "common.h"
typedef int TI128 __attribute__((mode(TI)));
extern void func_002833F0(char *, char *);
extern void func_00282D30(char *, char *, float);
extern void func_00282F20(char *, char *, float);
extern void func_003004E0(void *, void *, void *);



void func_L11_00438248(char *obj)
{
    char *state;
    TI128 *a;
    TI128 *b;
    TI128 *a1;
    TI128 *b1;
    TI128 v;
    int i;

    state = *(char **)(obj + 0x68);
    func_002833F0(obj + 0xC0, obj + 0xF0);
    *(TI128 *)(state + 0x100) = *(TI128 *)(obj + 0x10);
    b = (TI128 *)(state + 0x1D0);
    *b = *(TI128 *)(obj + 0x10);
    func_00282D30((char *)&v, obj + 0xC0, -1.0f);
    a = (TI128 *)(state + 0x100);
    for (i = 0; i < 12; i++) {
        func_00282F20((char *)&v, (char *)&v, *(float *)(state + 0x2F0 + 4 * i));
        a1 = a + 1;
        func_003004E0(a1, a, &v);
        b1 = b + 1;
        qcopy(b1, a1);
        a = a1;
        b = b1;
    }
}
