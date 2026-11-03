/* GPT-6.1 Sol (high), 29 October 2026: retained near miss.
 * Profile: -O2 -G8 -ffunction-sections (default split addresses).
 * USA v1.01: 1184/1184 bytes, 0/28 placements; final check.py first
 * mismatch: +0x7C. Fifteen opcode/register differences in the boot:
 * +0x70..+0x9C and +0x290/+0x298/+0x2A0. Relocation values are not a
 * separate match claim; the checker rejects the nonrelocated instructions.
 * The remaining blocks initialize the save context/header and calculate the
 * extra-line X coordinate (a0/v1 allocation). Moving context into the loop
 * improves initialization scheduling but still folds the header address.
 * The real 64-bit func_0027F5B8 style parameter restores the saved-register
 * set. D_00139410 is the four-slot table at D_001393E0 + 0x30; using its
 * symbol reproduces retail's mtlo/madd indexing without HI/LO artifacts.
 * Incomplete coefficient views reproduce the measured absolute accesses;
 * scalar coefficients remain GP-relative, and format strings are absolute.
 * Typed header-root and byte header-root variants emit 1180 bytes and lose
 * the madd; wide pointer arithmetic emits 1208. -mno-split-addresses also
 * worsens the body. Reopen with original table/header pointer lifetimes.
 * Other releases and linked builds were not run. All declarations follow.
 */
#include "common.h"
extern int D_001A7420;
typedef struct { int state, score, extra, ticks; unsigned char pad[4], date[4]; int unknown; } SaveRow;
typedef struct { int head[12]; SaveRow records[4]; unsigned char gap[0x15C-0xA0]; int mode, unknown160, selected; } SaveData;
extern SaveRow D_00139410[] MACRO_ADDR;
extern u8 D_001F2840[];
extern int D_001ABBF0 __attribute__((sda)),D_001ABBF4 __attribute__((sda)),D_001ABBF8 __attribute__((sda)),D_001ABBFC __attribute__((sda));
extern int D_001ABC00 __attribute__((sda)),D_001ABC04 __attribute__((sda)),D_001ABC08 __attribute__((sda));
extern int D_001ABC10 __attribute__((sda)),D_001ABC14[] MACRO_ADDR,D_001ABC18 __attribute__((sda)),D_001ABC1C[] MACRO_ADDR,D_001ABC20 __attribute__((sda));
extern char D_001ABC28[] NOT_SDA,D_001ABAF0[] NOT_SDA,D_001ABB20[] NOT_SDA,D_001ABC38[] NOT_SDA,D_001ABC48[] NOT_SDA;
extern void func_0027C540(int),func_0027C660(void),func_0028E868(int,int,int,int,int,int),func_0027F5B8(int,int,u64,u8 *,int);
extern int func_0028E588(int,int),func_00288FB8(int),func_0027FB40(int,int,unsigned long,u8 *,int),func_00115DA8(char *,char *,...);
int func_002DDA10(u8 *menu)
{
    char text[80];
    SaveRow *record;
    SaveData *context;
    int y,hours,minutes,score,extra,color,i;
    if (D_001A7420==10) return 2;
    y=*(int *)(menu+0x1c);func_0027C540(0);
    func_0028E868(func_0028E588(0xeaa9,0),D_001ABC10,D_001ABC14[0],D_001ABC18,D_001ABC1C[0],D_001ABC20);
    for(i=0;i<4;i++) {
        record = &D_00139410[i];
        context = (SaveData *)((u8 *)D_00139410 - 0x30);
        if (context->mode >= 3 || context->selected >= 0 || *(int *)(D_001F2840+0x164) <= 10 || context->head[2] != 2) {
            y += D_001ABBF0 * 3;
        } else {
            if (record->state == -1) {
                y+=D_001ABBF0;
                func_0027FB40(*(int *)(menu+0x18)+*(int *)(menu+0x20)/2,y+D_001ABC08,0x80f0f0f0UL,(u8 *)func_00288FB8(0x2f14),-1);
                y+=D_001ABBF0; y+=D_001ABBF0;
            } else {
                hours=record->ticks/216000;minutes=record->ticks/3600-hours*60;
                color=0x80f0f0f0;if (*(int *)(menu+0x40)==i) color=0x70fffeed;
                if (hours>99) {hours=99;minutes=59;}
                func_00115DA8(text,D_001ABC28,hours,minutes);
                func_0028E868(func_0028E588(0xe99d,3),D_001ABC04+4,y+D_001ABC08,D_001ABBF8,D_001ABBFC,128);
                func_0027F5B8(D_001ABC04+(long)(D_001ABBFC+6),y+D_001ABC08,color,(u8 *)text,-1);
                if (record->extra) {
                    func_0028E868(func_0028E588(0xe99d,4),D_001ABC04+D_001ABC00,y+D_001ABC08,D_001ABBF8,D_001ABBFC,128);
                    extra=record->extra;if (extra>99) extra=99;
                    func_00115DA8(text,D_001ABAF0,extra);
                    {
                        int iconX = D_001ABBFC + 2;
                        iconX = D_001ABC04 + iconX;
                        func_0027F5B8((long)iconX + D_001ABC00,y+D_001ABC08,color,(u8 *)text,-1);
                    }
                }
                score=record->score;if (score>9999999) score=9999999;y+=D_001ABBF0;
                if (score<1000) func_00115DA8(text,D_001ABAF0,score);
                else if (score<1000000) func_00115DA8(text,D_001ABB20,score/1000,score%1000);
                else func_00115DA8(text,D_001ABC38,score/1000000,(score%1000000)/1000,score%1000);
                func_0028E868(func_0028E588(0x7567,0),D_001ABC04+4,y+D_001ABC08,D_001ABBF8,D_001ABBFC,128);
                func_0027F5B8(D_001ABC04+(long)(D_001ABBFC+6),y+D_001ABC08,color,(u8 *)text,-1);
                y+=D_001ABBF0;
                func_00115DA8(text,D_001ABC48,record->date[2],record->date[1],record->date[3]);
                func_0028E868(func_0028E588(0xe99d,2),D_001ABC04+4,y+D_001ABC08,D_001ABBF8,D_001ABBFC,128);
                func_0027F5B8(D_001ABC04+(long)(D_001ABBFC+6),y+D_001ABC08,color,(u8 *)text,-1);
                y+=D_001ABBF0;
            }
        }
        y+=D_001ABBF4;
    }
    func_0027C660();return 2;
}
