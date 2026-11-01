#include "common.h"

typedef int s128 __attribute__((mode(TI)));

extern const char ref_L06_00444D00_00[];
extern const char ref_L06_00444D00_01[];
extern s32 ref_L06_00444D00_02(void *, s32);
extern void ref_L06_00444D00_03(void *, void *, s32, s32);
extern void ref_L06_00444D00_04(s32, void *, s32);

void func_L06_00444D00(char *object)
{
    char *state;
    s128 buf;
    s32 flag = 0;
    s32 i;
    s32 j;
    s32 k;
    s32 *q;
    s32 found;

    state = *(char **)(object + 0x68);
    *(u8 *)(object + 0x30) = 0xFF;
    if (state[0x7E])
        qcopy(&buf, ref_L06_00444D00_00);
    else
        qcopy(&buf, ref_L06_00444D00_01);
    found = *(u8 *)(object + 0x95);
    if (found == 0) {
        for (i = 0; ; i++) {
            if (i >= 4) goto done;
            if (((s32 *)state)[i] == -1) continue;
            if (ref_L06_00444D00_02(&buf, ((s32 *)state)[i])) break;
        }
        goto set;
    } else if (found == 1) {
set:
        flag = 1;
    } else {
        flag = 0;
    }
done:
    if ((flag && !*(u8 *)(object + 0xBC)) ||
        (!flag && *(u8 *)(object + 0xBC)) ||
        !*(u8 *)(object + 0x20)) {
        if (flag) *(u8 *)(object + 0xBC) = 1;
        else *(u8 *)(object + 0xBC) = 0;
        *(u8 *)(object + 0x20) = *(u8 *)(object + 0xBC) + 1;
        q = (s32 *)(state + 0x44);
        for (j = 0; j < 13; j++) {
            if (((s32 *)(state + 0x10))[j] != -1)
                ref_L06_00444D00_03(object, state, ((s32 *)(state + 0x10))[j], flag);
        }
        for (k = 0; k < 14; k++) {
            if (q[k] != -1)
                ref_L06_00444D00_04(q[k], state, flag);
        }
    }
}
