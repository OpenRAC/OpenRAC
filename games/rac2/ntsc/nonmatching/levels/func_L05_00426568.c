/* Bounded manual reconstruction, 2026-10-25; not linked.
 * Profile: -O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier.
 * 228/236 bytes; us: 0/4 placements; us2: 0/4 placements; pal: 0/4 placements.
 * The complete position copy uses the reviewed helper. The half-rate store now occurs on both cache
 * paths, correcting the prior draft. The head and cached object are pointer values, with the head
 * outside small data. Header constant sharing, list tag materialization and scheduling remain
 * different.
 * Reopen with independently qualified header-field and list-tag lifetime evidence. Preserve the
 * unconditional half-rate store.
 * Prior complete variants and diagnostic dumps are preserved privately.
 * No candidate or linked-build credit is claimed.
 */
#include "common.h"




typedef int TI128 __attribute__((mode(TI)));

extern char *func_0029EFD0(int);

extern void func_00282C88(TI128 *);

extern float func_002A7940(void);

extern void func_L05_00426810(char *);

extern char *D_L05_001B1500 __attribute__((sda));

extern char *D_L05_001B2528 NOT_SDA;

char *func_L05_00426568(char *object)
{
    char *obj = (char *)func_0029EFD0(0x117D);
    char *node;
    char *found;
    float half;
    float f;
    char *cached;

    if (obj != 0) {
        *(u8 *)(obj + 0x30) = 0xFF;
        *(u16 *)(obj + 0x32) = 0xFF;
        *(u8 *)(obj + 0x31) = 1;
        qcopy(obj + 0x10, object + 0x10);
        func_00282C88((TI128 *)(obj + 0xF0));
        f = func_002A7940();
        half = *(float *)(obj + 0x2C) * 0.5f;
        cached = D_L05_001B1500;
        *(float *)(obj + 0xF8) = f;
        *(float *)(obj + 0x2C) = half;
        if (cached == 0) {
            node = (char *)D_L05_001B2528;
            if (node != 0) {
                found = 0;
                do {
                    if (*(s16 *)(node + 0xAA) == 0xFBF)
                        found = node;
                    node = *(char **)(node + 0x28);
                } while (node != 0);
                D_L05_001B1500 = found;
            }
        }
        func_L05_00426810(obj);
        *(float *)(obj + 0x2C) = *(float *)(*(char **)(obj + 0x24) + 0x24);
        return obj;
    }
    return 0;
}
