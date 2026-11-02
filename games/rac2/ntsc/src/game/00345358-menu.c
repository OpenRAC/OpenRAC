#include "include_asm.h"
typedef unsigned char u8;
extern float *func_00335E10(void *);
extern void func_0027F120(void),func_0027F110(void);
extern int func_00336A40(void *);
extern void func_00336A78(void *);
extern void func_0027EAE8(int,int,int,int,int,void *);
extern int func_00335E20(u8 *);
extern void func_00335F88(void *,float,float,float,float);
extern void func_00336A38(void *,int);
extern int func_00288FB8_2(int);
typedef struct {unsigned int first,second;} MenuColors;
void func_00348138(char *o)
{
    float *pos=func_00335E10(o);
    int i=0;
    MenuColors colors, first, second;
    func_0027F120();
    pos[0]=(*(float **)(o+0x4c))[0];
    for(i=0;i<*(int *)(o+0xb0);i++) {
        int offset=i*20;
        pos[1]=(float)(*(int *)(o+0xa8)+*(int *)(o+0xac)*i)+(*(float **)(o+0x4c))[1];
        func_00335F88(o,*(float *)(offset+*(unsigned int *)(o+0x58)),0.0f,0.0f,0.0f);
        if(*(int *)(offset+*(unsigned int *)(o+0x58)+4)==0x31e4) pos[1]+=3.0f;
        if(*(int *)(o+0xb4)) func_00336A38(o,*(int *)(offset+*(unsigned int *)(o+0x58)+4));
        else func_00336A38(o,func_00288FB8_2(*(int *)(offset+*(unsigned int *)(o+0x58)+4)));
        { char *entry=o+i*4;
            if(*(int *)(entry+0x5c)) {
                if(*(int *)(o+0xbc)==i) {
                    int x,y;
                    first.second=first.first=(*(unsigned int *)(o+0x9c)&0xffffff)|0x20000000;
                    colors=first;
                    x=(int)(pos[0]-(float)(func_00336A40(o)>>1)-2.0f-16.0f);
                    y=(int)(pos[1]+2.0f);
                    func_0027EAE8(x,y,(int)(pos[0]-(float)(func_00336A40(o)>>1)-2.0f-16.0f+8.0f),(int)(pos[1]-2.0f+16.0f),0,&colors);
                }
                if(*(int *)(o+0x50)==i) {
                    *(int *)func_00335E20((u8 *)o)=*(int *)(o+0x9c);
                    if(*(int *)(o+0xb8)) {
                        int x,y;
                        second.second=second.first=(*(unsigned int *)(o+0x9c)&0xffffff)|0x20000000;
                        colors=second;
                        x=(int)(pos[0]-(float)(func_00336A40(o)>>1)-8.0f);
                        y=(int)(pos[1]-2.0f);
                        func_0027EAE8(x,y,(int)(pos[0]+(float)(func_00336A40(o)>>1)+9.0f),(int)(pos[1]+2.0f+16.0f),0,&colors);
                    }
                } else *(int *)func_00335E20((u8 *)o)=*(int *)(o+0xa0);
            } else *(int *)func_00335E20((u8 *)o)=*(int *)(o+0xa4);
        }
        func_00336A78(o);
    }
    func_0027F110();
}
