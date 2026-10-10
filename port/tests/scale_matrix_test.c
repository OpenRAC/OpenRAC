/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/rac1-pal/hand/game_scale_matrix.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%d: %s\n",__LINE__,#c); exit(1); } } while (0)
int main(void) {
    const float scales[]={1024.0f,1.0f,0.0f,-0.0f,-2.5f,0.125f};
    for (unsigned int n=0;n<sizeof(scales)/sizeof(scales[0]);++n) {
        float guarded[24]; unsigned char before[sizeof(guarded)];
        for (int i=0;i<24;++i) guarded[i]=(float)(99+i);
        memcpy(before,guarded,sizeof(guarded));
        func_001FA1C0(guarded+4,scales[n]);
        CHECK(!memcmp(before,guarded,4*sizeof(float)));
        CHECK(!memcmp(before+20*sizeof(float),guarded+20,4*sizeof(float)));
        for (int i=0;i<16;++i) {
            float expected=i==15?1.0f:(i==0 || i==5 || i==10)?0.0f+scales[n]:0.0f;
            CHECK(!memcmp(&expected,&guarded[4+i],sizeof(float)));
        }
    }
    puts("scale matrix: OK"); return 0;
}
