#include "common.h"
extern unsigned char D_00188660[];
extern void func_00382AF8(int);
extern float func_0030EFA8(void *,void *);
typedef struct { char data[256]; } L04MoverTrigger;
extern L04MoverTrigger *D_L04_001B24DC[] __attribute__((sda));
extern int func_0034C4C8(void *);
extern int func_0030E9D8(void *);
extern void func_0030EDC8(void *, void *, void *);
extern void func_0030F090(void *, void *, float);
extern void func_0030ED98(void *, void *, void *);
extern int func_00382DF8(int, int, void *);
void func_L04_003ED3D8(char *o)
{
    float delta[4], step[4];
    char *p = *(char **)(o+0x68);
    switch ((unsigned char)o[0x20]) {
    case 0:
        *(char **)(p+0xc) = (char *)&D_L04_001B24DC[0][*(int *)p];
        o[0x20] = 1;
        qcopy(p+0x10,o+0x10);
        *(float *)(p+0x20)=340.57f;
        *(float *)(p+0x24)=219.92f;
        *(float *)(p+0x28)=*(float *)(o+0x18);
        *(int *)(p+0x34)=-1;
        break;
    case 1:
        if (func_0034C4C8(*(void **)(p+0xc)) && func_0030E9D8(p+8)) {
            o[0x20]=3;
            *(char **)(p+0xc)=(char *)&D_L04_001B24DC[0][*(int *)(p+4)];
        }
        break;
    case 3: {
        func_0030EDC8(delta,p+0x20,o+0x10);
        func_0030F090(step,delta,0.07f);
        func_0030ED98(o+0x10,o+0x10,step);
        if (*(int *)(p+0x38)==1) {
            char *a=(char *)D_00188660+*(int *)(p+0x34)*0x70;
            if (*(char **)(a+0x88)!=o || !*(unsigned char *)(a+0x74))
                *(int *)(p+0x34)=func_00382DF8(0,4,o);
        }
        if (func_0030EFA8(o+0x10,p+0x20)<0.4f) {
            o[0x20]=2;
            if (*(int *)(p+0x38)==1) {
                func_00382DF8(1,0,o);
                if (*(int *)(p+0x34)!=-1) {
                    char *a=(char *)D_00188660+*(int *)(p+0x34)*0x70;
                    if (*(char **)(a+0x88)==o && *(unsigned char *)(a+0x74)) func_00382AF8(*(int *)(p+0x34));
                }
                *(int *)(p+0x34)=-1;
            }
        }
        break;
    }
    case 2:
        if (func_0034C4C8(*(void **)(p+0xc))) {
            *(unsigned short *)(o+0x34)&=0xffbe;
            *(int *)(o+0x98)=*(int *)(*(char **)(o+0x24)+0x10);
            o[0x20]=4;
        }
        break;
    case 4: {
        func_0030EDC8(delta,p+0x10,o+0x10);
        func_0030F090(step,delta,0.07f);
        func_0030ED98(o+0x10,o+0x10,step);
        if (*(int *)(p+0x38)==1) {
            char *a=(char *)D_00188660+*(int *)(p+0x34)*0x70;
            if (*(char **)(a+0x88)!=o || !*(unsigned char *)(a+0x74))
                *(int *)(p+0x34)=func_00382DF8(0,4,o);
        }
        if (func_0030EFA8(o+0x10,p+0x10)<0.1f) {
            qcopy(o+0x10,p+0x10);
            o[0x20]=5;
            if (*(int *)(p+0x38)==1) {
                func_00382DF8(1,0,o);
                if (*(int *)(p+0x34)!=-1) {
                    char *a=(char *)D_00188660+*(int *)(p+0x34)*0x70;
                    if (*(char **)(a+0x88)==o && *(unsigned char *)(a+0x74)) func_00382AF8(*(int *)(p+0x34));
                }
                *(int *)(p+0x34)=-1;
            }
        }
        break;
    }
    }
}
