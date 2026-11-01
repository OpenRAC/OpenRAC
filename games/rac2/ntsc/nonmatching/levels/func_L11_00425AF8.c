/* GPT-6.1 Sol worker v0412, USA v1.01, default game compiler profile.
 * Best: 228/228 bytes, 0/1 places; all instructions from +0x34 match.
 * Corrected the older draft's control flow: negative activity and non-0xCF6
 * records skip to the signed list terminator test, rather than return. This
 * restores all three retail annulling branches and their load delay slots.
 * Remaining: initial kind/0xFF and table temporaries use swapped v0/v1;
 * base-address load scheduling also differs. u8/int kind and explicit int
 * index reload spellings tie. Needs original type/lifetime evidence.
 * Declarations below must be plain non-sda int objects.
 */
#include "include_asm.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef signed char s8;
typedef signed short s16;
extern int D_L11_001FC420[];
extern int D_L11_001B359C;

void func_L11_00425AF8(void *a0)
{
    u8 kind = *(u8 *)(a0 + 0x21);
    int entry;
    int base;
    s16 *p;
    u8 *rec;
    u16 id;
    void *obj;
    s16 v;

    if (0xFF == kind) {
        return;
    }
    entry = D_L11_001FC420[kind];
    if (entry == 0) {
        return;
    }
    a0 = *(void **)(a0 + 0x68);
    base = D_L11_001B359C;
    p = (s16 *)entry;
    for (;;) {
        id = *(u16 *)p;
        id &= 0x7FFF;
        rec = (u8 *)(base + (id << 8));
        if (*(s8 *)(rec + 0x20) < 0) {
            goto next;
        }
        if (*(s16 *)(rec + 0xAA) == 0xCF5) {
            obj = *(void **)(rec + 0x68);
            *(u16 *)(obj + 0x266) = 1;
            *(unsigned int *)(obj + 0x560) += 1;
            if (*(float *)(a0 + 0x20) <= 0.0f) {
                *(unsigned int *)(obj + 0x584) += 1;
            }
        }
        if (*(s8 *)(rec + 0x20) < 0) {
            goto next;
        }
        if (*(s16 *)(rec + 0xAA) != 0xCF6) {
            goto next;
        }
        obj = *(void **)(rec + 0x68);
        *(u16 *)(obj + 0x266) = 1;
        *(unsigned int *)(obj + 0x560) += 1;
next:
        v = *(s16 *)p;
        p++;
        if (v >= 0) {
            continue;
        }
        return;
    }
}
