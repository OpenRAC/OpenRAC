#include "include_asm.h"

#include "common.h"
extern void func_002A7750(char *arg, int mode, int value);
extern void func_L00_003FF7B8_1(char *arg);
extern void func_L00_003FF890(char *arg);
extern float func_00282E80_3(char *arg, char *table);
extern float func_00282C48_10(float value);
extern void func_002B0A18(int arg, int value);
extern void func_00288308(int arg);
extern void func_002AD860(int arg);
extern void func_0029F338_4(char *arg);
extern char D_00189EA0_2[];
extern unsigned char D_001A7B2E __attribute__((sda));

/* two-state update on a record: state 0 marks a byte with 255 and either runs a shared two-call teardown when a global latch byte is set and a sub-object field is non-zero, or plays a mode/value pair plus a call; state 1 runs a setup call and two float queries against a stored table and, when both are below 2.0, plays three fixed calls and either tears down or writes 120 into a sub-object field */
void func_US2_L00_00400F70(char *arg)
{
    int *object = *(int **)(arg + 104);
    unsigned char state = *(unsigned char *)(arg + 32);

    switch (state) {
    case 0:
        *(unsigned char *)(arg + 48) = 255;
        if (D_001A7B2E != 0 && object[17] != 0) {
            func_0029F338_4((char *)object[17]);
            func_0029F338_4(arg);
            return;
        } else {
            func_002A7750(arg, 1, -1);
            func_L00_003FF7B8_1(arg);
        }
        break;
    case 1:
        func_L00_003FF890(arg);
        if (func_00282E80_3(arg + 16, D_00189EA0_2) < 2.0f) {
            if (func_00282C48_10(*(float *)(arg + 24) - *(float *)(D_00189EA0_2 + 8)) < 2.0f) {
                func_002B0A18(4658, -1);
                func_00288308(54);
                func_002AD860(54);
                if (object[17] != 0) {
                    func_0029F338_4((char *)object[17]);
                    func_0029F338_4(arg);
                    return;
                }
                object[16] = 120;
            }
        }
        break;
    }
}

