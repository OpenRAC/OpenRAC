/* Selects the object action from its command, trigger readiness and linked parent state. */
#include "common.h"
extern int D_L09_001B0F40 __attribute__((sda));
extern void func_L09_003F9708(char *);
extern int func_L09_003F7A88(void *, int);
extern int func_L09_003F7C50(void *, int, int);
extern void func_L09_003F7518(void *, int, int, int);
extern int func_L09_003F9928(char *);
extern void func_002A7750(void *, int, int);
extern int func_002A7820(int, int);
extern int func_001163B0(void);
extern void func_002A7470(void *, int, int, int);
void func_L09_003F9A70(char *obj)
{
    char *s = *(char **)(obj + 0x68);
    char *parent;
    int ready = 1;
    if (*(int *)(s + 0x2DC) != -1) {
        ready = func_L09_003F7A88(obj, *(int *)(s + 0x2DC)) != 0;
        if (*(int *)(s + 0x180) >= 0x33) ready = 1;
    }
    if (ready) {
        switch (*(u8 *)(obj + 0x95)) {
        case 3: {
            int alternate;
            *(int *)(s + 0x138) &= ~4;
            if (*(u8 *)(s + 0x302) & 2) *(u8 *)(s + 0x302) = 0;
            func_L09_003F7C50(obj, *(int *)(s + 0x2BC), -1);
            alternate = *(u8 *)(s + 0x30E);
            if (alternate) {
                func_L09_003F7518(obj, 4, 0, func_002A7820(7, 12));
                func_002A7750(obj, 4, 5);
            } else {
                func_L09_003F7518(obj, 12, 0, func_002A7820(7, 12));
                func_002A7750(obj, 8, 5);
            }
            return;
        }
        case 4:
            func_L09_003F7C50(obj, *(int *)(s + 0x2B4), -1);
            func_002A7750(obj, 5, 1);
            return;
        case 5: {
            int result = func_L09_003F9928(obj);
            if (!*(u8 *)(s + 0x303) || result) {
                func_L09_003F7518(obj, 12, 0, func_002A7820(7, 12));
                func_002A7750(obj, 8, 5);
                *(short *)(s + 0x2E4) = D_L09_001B0F40;
            }
            return;
        }
        case 1:
        case 6: {
            int result;
            func_L09_003F9928(obj);
            if (*(u8 *)(obj + 0x60) & 2) {
                result = func_001163B0() & 1;
                func_L09_003F7518(obj, result, 0, func_002A7820(10, 20));
                func_002A7750(obj, 5, -1);
            }
            return;
        }
        }
    } else if ((*(u8 *)(s + 0x303) && *(u8 *)(obj + 0x95) == 5) || (*(int *)(s + 0x4C0) && *(u8 *)(obj + 0x95) == 3)) {
        parent = *(char **)(s + 0x2F8);
        if (parent && *(short *)(parent + 0xAA) == 0xD0E && *(u8 *)(parent + 0x20) != 0xFE && *(u8 *)(parent + 0x20) != 0xFD) {
            *(u8 *)(s + 0x30C) = 0;
            *(u8 *)(s + 0x30D) = 1;
            if (*(u8 *)(parent + 0x42) == 10) func_L09_003F9708(obj);
            else if (*(u8 *)(parent + 0x43) != 10) func_002A7470(parent, 10, 0, 10);
        }
    }
}
