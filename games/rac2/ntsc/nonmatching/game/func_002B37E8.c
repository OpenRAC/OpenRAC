/* func_002B37E8 in its own unit: it copies two 128-bit fields with both
   addresses in registers, the compiler's opt-in grouped copy
   (config/cflags.txt gives this file -frac2-qcopy). */

#include "include_asm.h"

typedef int TI128 __attribute__((mode(TI)));

extern void func_002A0F48(char *);

/* Resets an object's slot and, when a template is given, copies its two
   vectors and its 8-byte field into place. */
void func_002B37E8(char *obj, char *src)
{
    *(unsigned char *)(obj + 0x30) = 0xFF;
    *(short *)(obj + 0x32) = 0xFF;
    *(unsigned char *)(obj + 0x31) = 1;
    if (src != 0) {
        TI128 *d = (TI128 *)(obj + 0x10);
        TI128 *s = (TI128 *)(src + 0x10);
        TI128 *d2;
        TI128 *s2;

        *(char **)(obj + 0xB8) = src;
        *d = *s;
        d2 = (TI128 *)(obj + 0xF0);
        s2 = (TI128 *)(src + 0xF0);
        *d2 = *s2;
        *(long long *)(obj + 0x38) = *(long long *)(src + 0x38);
        func_002A0F48(obj);
    }
    *(unsigned short *)(obj + 0x34) &= 0xFFF8;
}
