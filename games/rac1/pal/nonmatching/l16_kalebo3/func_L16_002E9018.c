/* NON_MATCHING func_L16_002E9018 -- src/overlays/l16_kalebo3/vendor_002E7C70.c
 * Best so far: SIZE ours 620 / retail 608, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   - Gather moby pointers from terminated signed-short IDs; signed-short count; path period=(float)path count-1.
 *   - Iterate count times, decrementing signed-short rank. Final object from last slot; others choose random unuse
 *   - Configure flags, scale, predecessor owner and path spacing rank*period/count.
 *   - Randomize height and speed with GP constants; timestep scales speed and state becomes1.
 *   1. SIZE584: gather postincrement moves destination before store, single last alias removes FP and duplicate ra
 *   2. SIZE620: gather matches. Distinguish stable last index from late modulus alias, use explicit next rank/inde
 *   3. BYTES355: size matches but nextRank is truncated too early, and nextI coalesces into the loop counter. Keep
 *   4. SIZE592: premature nextRank birth lets rank and chosen-object registers overlap, losing FP. Compute nextRan
 */
#include "common.h"
extern int func_002140B0(int);
extern float func_001FA888(int);
extern float func_002140F8(float,float);
extern short *D_L16_001ABFC0[];
extern char *D_L16_001B0C30[];
extern char *D_L16_00160098 MACRO_ADDR;
extern short D_L16_00161F50,D_L16_00161F54,D_L16_00161F3C,D_L16_00161F40,D_L16_00161F30,D_L16_00161F34;
extern float D_0015EE6C MACRO_ADDR;
typedef struct {int path_index;float parameter,speed,period;char *owner;float height_step,target_speed;short countdown,token;} L16SpacingData;
/* Randomly order path followers and initialize their spacing and speed. */
void func_L16_002E9018(char *m) {
    char *objects[16],*previous;
    L16SpacingData *d=*(L16SpacingData**)(m+0x78);
    short *list=D_L16_001ABFC0[*(unsigned char*)(m+0x21)];
    short count=0,rank;
    int i,last;
    float period;
    if(list==0 || d->path_index==-1) return;
    period=(float)*(int*)D_L16_001B0C30[d->path_index]-1.0f;
    do {objects[count]=D_L16_00160098+((*(unsigned short*)list&0x7FFF)<<8);count++;} while(*list++>=0);
    last=count-1;previous=objects[last];rank=count;
    for(i=0;i<count;i++,rank--) {
        char *current;
        L16SpacingData *data;
        if(i==count-1) current=objects[i];
        else {
            int selected=func_002140B0(last);
            if(objects[selected]!=0) {current=objects[selected];objects[selected]=0;}
            else {
                do {selected=(selected+1)%last;} while(objects[selected]==0);
                current=objects[selected];objects[selected]=0;
            }
        }
        *(unsigned char*)(current+0x30)=255;
        *(int*)(current+0x94)=0;
        *(unsigned short*)(current+0x32)=*(unsigned short*)&D_L16_00161F50;
        *(float*)(current+0x2C)*=*(float*)&D_L16_00161F54;
        data=*(L16SpacingData**)(current+0x78);
        data->parameter=(float)rank*(period/func_001FA888(count));
        data->owner=previous;data->period=period;previous=current;
        data->height_step=func_002140F8(*(float*)&D_L16_00161F3C,*(float*)&D_L16_00161F40);
        data->speed=func_002140F8(*(float*)&D_L16_00161F30,*(float*)&D_L16_00161F34)*D_0015EE6C;
        data->target_speed=data->speed;
        *(unsigned char*)(previous+0x20)=1;
    }
}
