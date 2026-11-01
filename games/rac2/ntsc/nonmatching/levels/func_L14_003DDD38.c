/* sol-high, USA v1.01, -O2 -G8 -ffunction-sections.
 * Best check: 1440/1440 bytes, 0 of 1 places match; two instructions differ.
 * At +0x36C/+0x370, C emits li v0,1; bne s6,v0 instead of retail's
 * xori v0,s6,1; bnez v0. All remaining instructions match after relocation.
 * The old ternary draft now has four differing instructions; explicit nested
 * if/goto recovers the two-word near miss under the current compiler.
 * XOR mutation adds a word and changes allocation; unsigned-char narrowing
 * and 64-bit unsigned nearEnd both tie with the retained two-word mismatch.
 * Six checker runs; stop on the comparison-lowering tie. A new way to retain
 * XOR as a temporary without mutating nearEnd could unblock it.
 * All required declarations are below. Use a dedicated -g8.c unit; the switch
 * needs a linked check if matched. No linked build claimed.
 */
#include "common.h"
typedef unsigned int Quad __attribute__((mode(TI)));
typedef union { Quad q; float f[4]; } Vec;
extern char D_00189E20[], D_00188660[];
extern void func_321F58(),func_2F5C68(),func_2F5D68(),func_328DA8();
extern int func_002B3868(char *), func_002ACBC8(void *,int), func_L00_00324120(void *);
extern int func_002E5CF8(int,int,void *),func_00282948(char *);
extern void func_002E59F8(int);
extern char *D_L14_001B259C;
extern float func_00282DF0(void *), func_00282E48(void *,void *);
extern int func_L14_003DE2D8(char *);
extern void func_L14_003DE360(char *,float), func_L14_003DE488(char *), func_L14_003DE310(char *);
void func_L14_003DDD38(char *self) {
    Vec oldpos, oldvel, delta, a, b;
    char *s=*(char **)(self+0x68);
    int i;
    char **objects;
    float *lengths;
    qcopy(&oldpos,self+0x10);
    qcopy(&oldvel,self+0xF0);
    switch((unsigned char)self[0x20]) {
    case 0:
        *(unsigned short *)(self+0x34)|=0x100;
        *(float *)(s+0x7C)=0.00872664712f;
        *(int *)(s+0x78)=-1;
        *(int *)(s+0x5C)|=1;
        if(*(int *)(s+0x70)) {
            qcopy(s+0x90,self+0x10);
            qcopy(s+0xA0,s+0x60);
            *(float *)(s+0x80)=1.57079637f;
            func_321F58(self,4,-1);
        } else {
            qcopy(s+0xA0,self+0x10);
            qcopy(s+0x90,s+0x60);
            *(int *)(s+0x80)=0;
            func_321F58(self,3,-1);
        }
        qcopy(s+0xB0,s+0xA0);
        *(float *)(s+0xB8)=*(float *)(s+0x98);
        func_2F5C68(&a,s+0xA0,s+0xB0);
        *(float *)(s+0x84)=func_00282DF0(&a);
        func_2F5C68(&b,s+0x90,s+0xB0);
        *(float *)(s+0x88)=func_00282DF0(&b);
        func_2F5D68(s+0xC0,&b,&a);
        for(objects=(char **)(s+0xE0), lengths=(float *)(s+0xF0),i=3;i>=0;objects++,i--,lengths++) {
            int id=*(int *)((char *)lengths-0x20);
            if(id>0) {
                char *obj=D_L14_001B259C+(id<<8);
                float length;
                *objects=obj;
                func_2F5C68(&a,obj+0x10,s+0xB0);
                length=func_00282DF0(&a);
                if((unsigned char)self[0x20]==4) {
                    *lengths=length;
                    lengths[4]=(*(float *)(s+0x84)-*(float *)(s+0x88))+length;
                } else {
                    lengths[4]=length;
                    *lengths=(*(float *)(s+0x88)-*(float *)(s+0x84))+length;
                }
            } else *objects=0;
        }
        if(func_L14_003DE2D8(self) && *(int *)(s+0x74)>=0) func_321F58(self,1,-1);
        break;
    case 1:
        func_002B3868(self);
        if(func_002ACBC8(D_00189E20+0x80,*(int *)(s+0x74))) func_321F58(self,2,-1);
        break;
    case 2:
        func_321F58(self,3,-1);
    case 3:
    case 4:
        if(func_002B3868(self)) {
            func_L14_003DE488(self);
            *(int *)(s+0x8C)=100;
        }
        func_00282948(s+0x8C);
        if(!func_L00_00324120(self)) {
            float first, second;
            int nearEnd;
            if(!(func_00282E48(D_00189E20+0x80,self+0x10)<30.0f)) break;
            first=func_00282E48(D_00189E20+0x80,s+0x90);
            second=func_00282E48(D_00189E20+0x80,s+0xA0);
            nearEnd=second<first;
            first=func_00282E48(self+0x10,s+0x90);
            second=func_00282E48(self+0x10,s+0xA0);
            if(second<first) {
                if (nearEnd ^ 1) goto startMove;
            } else if (nearEnd) goto startMove;
            break;
        }
startMove:
        if(!func_L14_003DE2D8(self) && *(int *)(s+0x8C)<=0) {
            if((unsigned char)self[0x20]==4) func_321F58(self,5,-1);
            else func_321F58(self,6,-1);
        }
        break;
    case 5:
        if(func_002B3868(self)) *(int *)(s+0x78)=func_002E5CF8(0,4,self);
        if(*(float *)(s+0x80)>0.0f) func_L14_003DE360(self,-*(float *)(s+0x7C));
        else {
            int n=*(int *)(s+0x78);
            if(n>=0) {
                if(n!=-1) {
                    unsigned char *e=(unsigned char *)D_00188660+n*0x70;
                    if(*(char **)(e+0x88)==self && e[0x74]) func_002E59F8(n);
                }
                *(int *)(s+0x78)=-1;
            }
            if(!func_L00_00324120(self)) func_321F58(self,3,-1);
        }
        break;
    case 6:
        if(func_002B3868(self)) *(int *)(s+0x78)=func_002E5CF8(0,4,self);
        if(*(float *)(s+0x80)<1.57079637f) func_L14_003DE360(self,*(float *)(s+0x7C));
        else {
            int n=*(int *)(s+0x78);
            if(n>=0) {
                if(n!=-1) {
                    unsigned char *e=(unsigned char *)D_00188660+n*0x70;
                    if(*(char **)(e+0x88)==self && e[0x74]) func_002E59F8(n);
                }
                *(int *)(s+0x78)=-1;
            }
            if(!func_L00_00324120(self)) func_321F58(self,4,-1);
        }
        break;
    }
    func_L14_003DE310(self);
    func_2F5C68(&delta,self+0x10,&oldpos);
    func_328DA8(s+0x20,&delta,&oldvel,self+0xF0);
}
