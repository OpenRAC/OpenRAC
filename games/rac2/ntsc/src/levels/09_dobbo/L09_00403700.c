/* func_L09_00403700 (152 bytes): while the object is in state 7, sparks four of its bones
   through func_002A8D70 with the controller's byte as the second coordinate. */
#include "common.h"

extern int func_002A8D70(float x, float y, float z, void *object, int bone, int mode, int first, int second, int third);

void func_L09_00403700(char *a0) {
    char *s2 = *(char **)(a0 + 0x68);
    s32 i;

    if (*(u8 *)(a0 + 0x20) == 7) {
        for (i = 0; i < 4; i++)
            func_002A8D70(0.17f, *(u8 *)(s2 + 0x34), 1.0f, a0, i, 0x4000001, 0, 1, 0);
    }
}
