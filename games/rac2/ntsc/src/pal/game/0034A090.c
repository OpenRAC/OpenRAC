#include "include_asm.h"

extern int func_00336E40(void *);
extern void *func_00336DA8(int, int);
extern void *func_003369A8(void *, void *, void *);
extern char D_001AE5A8[];

void func_PAL_0034A090(char *object, void *data)
{
    int *position;
    *(int *)(object + 0x60) = 0;
    *(int *)(object + 0xC0) = 0;
    *(void **)(object + 0x58) = data;
    if (data) {
        position = func_00336DA8(16, func_00336E40(data));
        *(int **)(object + 0x5C) = position;
        position[1] = 0; position[2] = 0; position[3] = 0; position[0] = 0;
    }
    func_003369A8(object, D_001AE5A8, data);
    *(int *)(object + 0xAC) = 0x70FFFEED;
    *(int *)(object + 0xB0) = 0x80F0F0F0;
    *(int *)(object + 0xB4) = 0x80808080;
    *(int *)(object + 0xBC) = 32;
    *(int *)(object + 0xB8) = 0;
    *(int *)(object + 0xC4) = 0;
    *(int *)(object + 0xC8) = 0;
    *(int *)(object + 0xCC) = -1;
}

