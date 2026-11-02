#include "common.h"
extern float D_L17_001B0328 __attribute__((sda));
extern int D_L17_001C0018 NOT_SDA;
extern unsigned char D_L17_001DBF81[];
extern char *D_L17_001DAD00[];
extern float func_0031CD58(void *, int, float);
extern void func_0031ACD8(void *, int, int);
extern int func_002EEAF8(void *);
extern void func_00310EC8(void *);
extern void func_002EEEE8(void *, void *, void *);
extern void func_00323370(void *, void *, void *, void *);
typedef unsigned int StateQuad __attribute__((mode(TI)));
void func_L17_003C5C28(char *self) {
    union { StateQuad q; float f[4]; } oldpos, delta;
    char *data = *(char **)(self + 0x68);
    float ground;
    unsigned char index;
    oldpos.q = *(StateQuad *)(self + 0x10);
    switch (*(unsigned char *)(self + 0x20)) {
    case 0:
        if (*(int *)(data + 0x90) != -1) {
            ground = func_0031CD58(self + 0x10, 0, 0.5f);
            *(int *)(data + 0x9C) = D_L17_001C0018;
            if (ground < *(float *)(self + 0x18)) {
                *(float *)(data + 0x98) += D_L17_001B0328 * 0.00027777778f;
                *(float *)(self + 0x18) -= *(float *)(data + 0x98);
            } else {
                *(int *)(data + 0x98) = 0;
                *(float *)(self + 0x18) = ground;
            }
        }
        break;
    case 1:
        if (*(unsigned char *)(self + 0x42) == 1 && (*(unsigned char *)(self + 0x60) & 2)) {
            *(short *)(self + 0xAA) = 0x921;
            index = D_L17_001DBF81[0];
            *(unsigned char *)(self + 0x22) = index;
            *(char **)(self + 0x24) = D_L17_001DAD00[*(unsigned char *)(self + 0x22)];
            *(int *)(self + 0x98) = *(int *)(*(char **)(self + 0x24) + 0x10);
            func_0031ACD8(self, 0, 0);
            *(unsigned char *)(self + 0x20) = 2;
            if (*(int *)(data + 0x90) != -1) *(int *)(data + 0x94) = 600;
        }
        break;
    case 2:
        if (*(int *)(data + 0x90) != -1 && func_002EEAF8(data + 0x94)) {
            func_00310EC8(self);
            return;
        }
        break;
    }
    func_002EEEE8(&delta, self + 0x10, &oldpos);
    func_00323370(data + 0x50, &delta, self + 0xF0, self + 0xF0);
}
