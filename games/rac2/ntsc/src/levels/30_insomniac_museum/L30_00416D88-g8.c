#include "common.h"
extern char *func_316A50(int);
extern float func_2F5508(float, float);
extern float func_2F5E58(float, float);
extern float func_2F4FC8(void *);
extern float func_321230(void);
extern void func_326C10(void *, void *, void *, float);
extern void func_2F56B0(void *, void *);
extern float func_321168(float, float);
extern void func_33A020(void *, int, int, float, float, int, int, void *, float, int);
extern float D_L30_001B1FAC __attribute__((sda));
extern int D_L30_001B1F08 __attribute__((sda)), D_L30_001B1F0C __attribute__((sda));
extern char D_L30_001A8F20[] __attribute__((sda)) NOT_SDA;
char *func_L30_00416D88(char *owner, float *velocity, void *position, int type, float scale) {
    char *obj = func_316A50(0x11EA);
    char *st;
    float angle, size;
    if (obj) {
        *(unsigned char *)(obj + 0x30) = 0xFF;
        *(unsigned short *)(obj + 0x32) = 0xFF;
        *(unsigned char *)(obj + 0x31) = 1;
        *(unsigned char *)(obj + 0xBC) = type;
        *(unsigned char *)(obj + 0x20) = 0;
        *(unsigned long long *)(obj + 0x38) = *(unsigned long long *)(owner + 0x38);
        qcopy(obj + 0x10, position);
        st = *(char **)(obj + 0x68);
        qcopy(st, velocity);
        *(char **)(st + 0x10) = owner;
        *(float *)(st + 0x14) = scale;
        *(float *)(st + 0x20) = 0.15f;
        *(unsigned short *)(obj + 0x34) |= 0x208;
        *(float *)(obj + 0xF8) = func_2F5E58(func_2F5508(velocity[0], velocity[1]), 1.5707964897155762f);
        *(float *)(obj + 0xF0) = -func_2F5508(func_2F4FC8(velocity), velocity[2]);
        angle = func_321230();
        func_326C10(obj + 0xE0, obj + 0xE0, obj + 0xD0, angle);
        func_326C10(obj + 0xC0, obj + 0xC0, obj + 0xD0, angle);
        func_2F56B0(obj + 0xC0, obj + 0xF0);
        *(unsigned char *)(obj + 0x23) = 0x80;
        *(float *)(obj + 0x2C) = D_L30_001B1FAC;
        *(unsigned short *)(obj + 0x34) |= 0x100;
        *(int *)(st + 0x24) = 0;
        size = func_321168(8.0f, 10.0f);
        func_33A020(obj + 0x10, 15, D_L30_001B1F08, size * 0.3f, size, 0, 10, D_L30_001A8F20, 0.0f, 0);
        func_33A020(obj + 0x10, 15, D_L30_001B1F0C, size * 0.04f, size * 0.4f, 1, -10, D_L30_001A8F20, 0.0f, 0);
    }
    return obj;
}
