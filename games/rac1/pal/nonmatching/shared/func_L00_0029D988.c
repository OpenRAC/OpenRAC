/* NON_MATCHING func_L00_0029D988 -- src/overlays/shared/update_0029B6A0.c
 * Best so far: SIZE ours 5156 / retail 5164, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   5164-byte VendorModeUpdate. Frame120: all nine saved GPRs, f20-f22, two Vec16 locals0/10, matrix64 at20. State
 *   p0: initial typed reconstruction. p1: distinguish runtime controller flags at resident +2604 from vendor input
 *   p0/p1 SIZE4996 (logical offset fixes compile same size); p2 SIZE5004 separate mode alias duplicates a high loa
 *   p8 SIZE5060: correct Gadget299 flag offset1 (initial sketch typo at4); inline state1 vector-add destinations s
 *   p12 SIZE5092: gadget-local pointer/counter moves the gadget loop toward retail; explicit signed-validity branc
 *   p17 SIZE5084: direct scene fields before fade and named 96/80 threshold recover retail MOVN. p18 COMPILE C90 d
 *   Best p29 remains plain C complete reconstruction, correct frame120 and gadget flag1. Retail first2B8 bytes mat
 */
#include "common.h"
typedef struct { float x,y,z,w; } __attribute__((aligned(16))) VV299;
typedef struct { VV299 row[4]; } VM299;
typedef struct { s32 id,kind,price,fC,f10; } Item299;
typedef struct { u8 f0,flag; char pad2[0x3E]; } Gadget299;
typedef struct { char pad0[12]; u8 count; char padD[0x3B]; s32 bindings[1]; } Class299;
typedef struct Moby299 {
 char pad0[0x10]; VV299 pos; u8 state; char pad21[1]; u8 slot; char pad23[1]; Class299 *cls;
 char pad28[0xA]; u16 mode; char pad34[0xC]; VV299 rot;
 u8 anim0,anim1; u8 pad52[2]; float fraction,rate; char pad5C[0x15]; u8 opacity; char pad72[6]; VV299 *anim; char pad7C[3]; u8 flags;
 char pad80[0x26]; s16 class_id; char padA8[0x18]; VM299 matrix;
} Moby299;
typedef struct { s32 mode,timer,f8,fC,f10; Moby299 *template; s32 decoded; Moby299 *main,*preview,*item,*camera;
 s32 f2C,f30,f34,f38,f3C,own,f44,index,scroll,handle,handle2,selected,buy;
 VV299 view60,view70; VM299 transform; VV299 origin; Item299 entries[16]; s32 count;
} Vendor299;
typedef struct { VV299 pos,rot; } Frame299;
typedef struct { char pad0[0x34]; s32 timer,frame,count; s16 length; char pad42[2]; s16 objects_count; char pad46[0xE]; Frame299 *frames; char pad58[0x120]; Moby299 *objects[1]; } Scene299;
typedef struct { char pad0[8]; s32 flags; } Update299;
typedef struct { float x,y,z,fC,rx,ry; char pad18[0x28]; } Pos64_299;
typedef struct { float x,y,z,fC,rx,ry; char pad18[0x18]; } Pos48_299;
typedef struct { char pad0[0xE]; u16 count; char pad10[8]; } Desc299;
typedef struct { s32 f0,f4,f8,fC,text; } Text299;
extern Vendor299 vendor299 __asm__("D_L00_001CA7C0");
extern Update299 update299 __asm__("D_L00_0016C158");
extern Scene299 scene299 __asm__("D_L00_0016C960");
extern VM299 transform299 __asm__("D_L00_001CA840");
extern VV299 camera_pos299 __asm__("D_L00_00166EC0");
extern VV299 camera_target299 __asm__("D_L00_00166ED0");
extern VM299 camera_rows299 __asm__("D_L00_001670D0");
extern VV299 snapshot299[3] __asm__("D_L00_00166080");
extern Class299 *classes299[] __asm__("D_L00_00197680");
extern Gadget299 gadgets299[] __asm__("D_L00_00165F80");
extern s32 indices299[] __asm__("D_L00_001CA320");
extern VV299 positions299[] __asm__("D_L00_001C9310") MACRO_ADDR;
extern Pos64_299 rotations299[] __asm__("D_L00_001C92F0");
extern VV299 positions2_299[] __asm__("D_L00_001C9C50") MACRO_ADDR;
extern Pos48_299 rotations2_299[] __asm__("D_L00_001C9C30");
extern Text299 texts299[] __asm__("D_L00_001C2488");
extern Desc299 descriptions299[] __asm__("D_L00_001C43B0");
extern s32 purchase299[] __asm__("D_0013D50F");
extern char pad_data299[] __asm__("D_0013A5E0");
typedef struct { char pad[0x1090]; Moby299 *hero; } Hero299;
extern char hero_data299[] __asm__("D_0013E633");
extern char game_data299[] __asm__("D_0014171B");
extern char timing0_299[] __asm__("D_L00_001611E8");
extern char timing1_299[] __asm__("D_L00_001611F8");
extern char timing2_299[] __asm__("D_L00_00161208");
extern VV299 item_offset299 SDATA(D_L00_00161140);
extern VV299 preview_offset299 SDATA(D_L00_00161170);
extern VV299 second_offset299 SDATA(D_L00_00161180);
extern VV299 bob_offset299 SDATA(D_L00_00161160);
extern volatile s32 D_L00_0016124C_299 __asm__("D_L00_0016124C") MACRO_ADDR;
extern float D_L00_0015F4FC_299 __asm__("D_L00_0015F4FC") MACRO_ADDR;
extern float D_L00_00161F24_299 __asm__("D_L00_00161F24") MACRO_ADDR;
extern float D_L00_0016CBF0_299 __asm__("D_L00_0016CBF0") NOT_SDA;
extern float D_0015EE60_299 __asm__("D_0015EE60") MACRO_ADDR;
extern s32 D_0015EE80_299 __asm__("D_0015EE80") MACRO_ADDR;
extern s32 G00161248_299 __asm__("D_L00_00161248") MACRO_ADDR;
extern s32 G00161F14_299 __asm__("D_L00_00161F14") MACRO_ADDR;
extern s32 G00161F18_299 __asm__("D_L00_00161F18") MACRO_ADDR;
extern s32 G00161F1C_299 __asm__("D_L00_00161F1C") MACRO_ADDR;
extern s32 G00161F20_299 __asm__("D_L00_00161F20") MACRO_ADDR;
extern s32 G00161F34_299 __asm__("D_L00_00161F34") MACRO_ADDR;
extern s32 G0015F6BC_299 __asm__("D_L00_0015F6BC") MACRO_ADDR;
extern s32 small0016124C_299 SDATA(D_L00_0016124C);
extern s32 small00161F14_299 SDATA(D_L00_00161F14);
extern s32 small00161F18_299 SDATA(D_L00_00161F18);
extern s32 small00161F20_299 SDATA(D_L00_00161F20);
extern s32 small00161F34_299 SDATA(D_L00_00161F34);
extern void call299_L00_00299108(void) __asm__("func_L00_00299108");
extern void call299_00213C78(void) __asm__("func_00213C78");
extern void call299_0022EF68(void) __asm__("func_0022EF68");
extern void call299_001E9768(void *,s32) __asm__("func_001E9768");
extern void call299_L00_002076E8(void) __asm__("func_L00_002076E8");
extern void call299_00218A80(void) __asm__("func_00218A80");
extern s32 call299_001F9850(s32) __asm__("func_001F9850");
extern void call299_00213DE0(void *,s32,s32,s32) __asm__("func_00213DE0");
extern void call299_0022ED80(s32,s32,void *) __asm__("func_0022ED80");
extern void call299_0020D9D8(void *,void *) __asm__("func_0020D9D8");
extern void call299_L00_0029C070(void) __asm__("func_L00_0029C070");
extern void call299_L00_0029BED8(s32) __asm__("func_L00_0029BED8");
extern s32 call299_L00_0023B610(s32) __asm__("func_L00_0023B610");
extern void call299_001F49B0(void *,void *) __asm__("func_001F49B0");
extern void call299_L00_002EBE88(void *) __asm__("func_L00_002EBE88");
extern void call299_L00_002EBEE0(void *) __asm__("func_L00_002EBEE0");
extern void call299_00202260(void) __asm__("func_00202260");
extern void call299_L00_00299148(void) __asm__("func_L00_00299148");
extern void call299_L00_0029B6A0(void) __asm__("func_L00_0029B6A0");
extern void call299_0020D440(void *,s32) __asm__("func_0020D440");
extern void call299_L00_00251E30(void *) __asm__("func_L00_00251E30");
extern void call299_0020E340(void *,s32,s32,s32,s32) __asm__("func_0020E340");
extern void call299_L00_0029C840(s32) __asm__("func_L00_0029C840");
extern void call299_L00_002514B8(void *) __asm__("func_L00_002514B8");
extern void call299_001F9EC0(void *,void *,void *) __asm__("func_001F9EC0");
extern void call299_001F9BD8(void *,void *,void *) __asm__("func_001F9BD8");
extern void call299_002391E8(void *) __asm__("func_002391E8");
extern float call299_001F9878(float) __asm__("func_001F9878");
extern float call299_001FA748(float,float) __asm__("func_001FA748");
extern float call299_001F9FA8(float) __asm__("func_001F9FA8");
extern void call299_001FA1F8(void *,void *) __asm__("func_001FA1F8");
extern void * call299_001FE540(s32) __asm__("func_001FE540");
extern void call299_002391A8(void *) __asm__("func_002391A8");
extern void call299_001FFDA0(s32,s32) __asm__("func_001FFDA0");
extern s32 call299_001FFB38(s32,s32,s32,s32,s32,s32,s32) __asm__("func_001FFB38");
extern void call299_L00_0029CDE0(void) __asm__("func_L00_0029CDE0");
extern void call299_002348B8(void) __asm__("func_002348B8");
extern void call299_00205270(s32,s32) __asm__("func_00205270");
extern void call299_0020D6D0(void *) __asm__("func_0020D6D0");
extern void call299_L00_0029C9E8(void) __asm__("func_L00_0029C9E8");
extern void call299_001F3140(void) __asm__("func_001F3140");
extern void call299_0020D678(void *) __asm__("func_0020D678");
extern void call299_L00_0029C2E8(void) __asm__("func_L00_0029C2E8");
extern void call299_00205220(s32) __asm__("func_00205220");
extern void call299_00125358(float *) __asm__("func_00125358");
extern void call299_001254A0(float *,float *,float) __asm__("func_001254A0");
extern void call299_00125548(float *,float *,float) __asm__("func_00125548");
extern void call299_001253F8(float *,float *,float) __asm__("func_001253F8");
extern float call299_001FA888(s32) __asm__("func_001FA888");
extern void call299_001F9C30(void *,void *,float) __asm__("func_001F9C30");
extern void call299_00214550(void *) __asm__("func_00214550");
extern void call299_L00_002353B8(void *) __asm__("func_L00_002353B8");
extern void call299_0022DD68(void) __asm__("func_0022DD68");
extern void callback299_L00_002A0C20(void) __asm__("func_L00_002A0C20");
extern void callback299_L00_0023B0F8(void) __asm__("func_L00_0023B0F8");
extern void callback299_L00_00236830(void) __asm__("func_L00_00236830");
extern void callback299_L00_0023B140(void) __asm__("func_L00_0023B140");
#define PAD299 (*(s32 *)(pad_data299+0x2604))
#define BUTTON299 (*(s32 *)(pad_data299+0x2624))
#define HERO299 hero_state->hero
#define SELECT299 vendor299.entries
#define SELECTION299 vendor299.entries[vendor299.selected].id
extern s32 small00161F1C_299 SDATA(D_L00_00161F1C);
/* Vendor transitions, model previews, input selection and camera playback. */
void func_L00_0029D988(void) {
    VV299 v0, v1;
    VM299 mat;
    s32 initial=vendor299.mode, active, i, n, selection;
    Moby299 *m, *saved;
    Class299 *cls;
    Frame299 *frame;
    Scene299 *scene;
    VV299 *track, *position;
    float angle, limit, fade;
    u8 snap;
    switch(initial) {
    case 0:
    case 2:
        if(!vendor299.own || vendor299.mode==2) {
            if((update299.flags&2) || ((update299.flags&16)&&(PAD299&512))) call299_L00_00299108();
            if((update299.flags&2) || ((update299.flags&16)&&(PAD299&512))) {call299_00213C78();call299_0022EF68();}
            call299_001E9768(timing0_299,3);
            if((update299.flags&1) || ((update299.flags&16)&&(PAD299&512))) call299_L00_002076E8();
            call299_001E9768(timing1_299,7);
            if((update299.flags&4) || ((update299.flags&16)&&(PAD299&512))) call299_00218A80();
            call299_001E9768(timing2_299,5);
        }
        {
        if(vendor299.mode==0) {
            D_L00_0015F4FC_299-=0.34f;
            if(D_L00_0015F4FC_299<0.0f) D_L00_0015F4FC_299=0.0f;
            vendor299.timer++;
            if(vendor299.timer>=call299_001F9850(40) || vendor299.own) {
                D_L00_0015F4FC_299=0.0f;vendor299.fC=1;vendor299.mode=1;vendor299.timer=0;
                call299_00213DE0(vendor299.main,3,0,8);
                vendor299.main->rate=1.0f;G00161F20_299=1;small00161F1C_299=8;
                call299_0022ED80(4,0,vendor299.main);
            }
        } else if(vendor299.mode==2) {
            active=0;
            if(!vendor299.own && vendor299.index>=0) {
                active=indices299[vendor299.index]>=0;
            }
            vendor299.timer++;
            if(vendor299.timer>=call299_001F9850(40) || vendor299.own) {
                call299_00213DE0(vendor299.main,1,0,8);
                { Gadget299 *gadget=gadgets299; s32 gadget_i;
                  for(gadget_i=3;gadget_i>=0;gadget_i--,gadget++) {
                    if(gadget->flag==1) call299_0020D9D8(vendor299.main,gadget);
                  }
                }
                if(active) call299_L00_0029C070();
                else {call299_L00_0029BED8(0);call299_0022ED80(6,0,vendor299.main);}
            }
            call299_L00_0023B610(1);
        }
        }
        if(call299_001F9850(36)>=vendor299.timer) call299_001F49B0(callback299_L00_002A0C20,vendor299.main);
        if(!vendor299.own) {call299_L00_002EBE88(&vendor299.view60);call299_L00_002EBEE0(&vendor299.view70);}
        call299_00202260();
        if((update299.flags&2) || ((update299.flags&16)&&(PAD299&512))) call299_L00_00299148();
        break;
    case 1:
        if(!G00161248_299 && *(s16 *)(game_data299+0x100BD)==0 && D_L00_0016124C_299) {
            call299_L00_0029B6A0();vendor299.item=vendor299.template;
            call299_0020D440(vendor299.template,12);
            vendor299.item->mode=64;vendor299.item->rot.z=vendor299.main->rot.z;
            call299_L00_00251E30(vendor299.item);call299_0020E340(vendor299.item,0x202020,14,14,0);
            G00161248_299=initial;small0016124C_299=0;call299_L00_0029C840(1);
        }
        {
        call299_L00_002514B8(vendor299.main);call299_L00_00251E30(vendor299.main);
        if(G00161248_299==1) {
            call299_001F9EC0(&vendor299.item->pos,&item_offset299,&vendor299.main->matrix);
            call299_001F9BD8(&vendor299.item->pos,&vendor299.item->pos,&vendor299.main->pos);
            call299_L00_002514B8(vendor299.item);call299_L00_00251E30(vendor299.item);call299_002391E8(vendor299.item);
        }
        call299_001F9EC0(&vendor299.preview->pos,&preview_offset299,&vendor299.main->matrix);
        call299_001F9BD8(&vendor299.preview->pos,&vendor299.preview->pos,&vendor299.main->pos);
        saved=vendor299.preview;angle=call299_001F9878(-0.05f);vendor299.preview->rot.x=call299_001FA748(saved->rot.x,angle);
        vendor299.preview->rot.y=-0.36f;
        saved=vendor299.preview;angle=call299_001F9878(0.05f);vendor299.preview->rot.z=call299_001FA748(saved->rot.z,angle);
        call299_L00_00251E30(vendor299.preview);
        call299_001F9EC0((char *)vendor299.preview+0x510,&second_offset299,&vendor299.main->matrix);
        call299_001F9BD8((VV299 *)((char *)vendor299.preview+0x510),(VV299 *)((char *)vendor299.preview+0x510),&vendor299.main->pos);
        saved=vendor299.preview;angle=call299_001F9878(-0.05f);((Moby299 *)((char *)vendor299.preview+0x500))->rot.x=call299_001FA748(((Moby299 *)((char *)saved+0x500))->rot.x,angle);
        ((Moby299 *)((char *)vendor299.preview+0x500))->rot.y=-0.36f;
        saved=vendor299.preview;angle=call299_001F9878(0.05f);((Moby299 *)((char *)vendor299.preview+0x500))->rot.z=call299_001FA748(((Moby299 *)((char *)saved+0x500))->rot.z,angle);
        call299_L00_00251E30((char *)vendor299.preview+0x500);
        angle=call299_001F9878(0.05f);D_L00_00161F24_299=call299_001FA748(D_L00_00161F24_299,angle);
        call299_L00_002514B8((char *)vendor299.preview+0x100);
        }
        {
        if(((Moby299 *)((char *)vendor299.preview+0x100))->pad52[1]==0 && (((u8 *)vendor299.preview)[0x170]&2)) call299_00213DE0((char *)vendor299.preview+0x100,1,0,8);
        call299_L00_00251E30((char *)vendor299.preview+0x100);
        v1=(VV299){0};v1.z=call299_001F9FA8(D_L00_00161F24_299)/20.0f;v0=v1;
        call299_001F9BD8(&v0,&bob_offset299,&v0);
        call299_001F9BD8(&v0,&v0,(char *)positions299+(vendor299.entries[vendor299.selected].id<<6));
        call299_001F9EC0((char *)vendor299.preview+0x210,&v0,&vendor299.main->matrix);
        call299_001F9BD8((VV299 *)((char *)vendor299.preview+0x210),(VV299 *)((char *)vendor299.preview+0x210),&vendor299.main->pos);
        ((Moby299 *)((char *)vendor299.preview+0x200))->rot.x=rotations299[vendor299.entries[vendor299.selected].id].rx;
        ((Moby299 *)((char *)vendor299.preview+0x200))->rot.y=rotations299[vendor299.entries[vendor299.selected].id].ry;
        ((Moby299 *)((char *)vendor299.preview+0x200))->rot.z=D_L00_00161F24_299;
        call299_L00_00251E30((char *)vendor299.preview+0x200);
        v1=(VV299){0};v1.z=call299_001F9FA8(D_L00_00161F24_299)/20.0f;v0=v1;
        call299_001F9BD8(&v0,&bob_offset299,&v0);
        call299_001F9BD8(&v0,&v0,(char *)positions2_299+vendor299.entries[vendor299.selected].id*48);
        call299_001F9EC0((char *)vendor299.preview+0x310,&v0,&vendor299.main->matrix);
        call299_001F9BD8((VV299 *)((char *)vendor299.preview+0x310),(VV299 *)((char *)vendor299.preview+0x310),&vendor299.main->pos);
        ((Moby299 *)((char *)vendor299.preview+0x300))->rot.x=rotations2_299[vendor299.entries[vendor299.selected].id].rx;
        ((Moby299 *)((char *)vendor299.preview+0x300))->rot.y=rotations2_299[vendor299.entries[vendor299.selected].id].ry;
        ((Moby299 *)((char *)vendor299.preview+0x300))->rot.z=D_L00_00161F24_299;
        call299_L00_002514B8((char *)vendor299.preview+0x300);
        }
        {
        if(((Moby299 *)((char *)vendor299.preview+0x300))->pad52[1]==0 && (((u8 *)vendor299.preview)[0x370]&2)) call299_00213DE0((char *)vendor299.preview+0x300,1,0,8);
        call299_L00_00251E30((char *)vendor299.preview+0x300);
        v1.x=rotations2_299[vendor299.entries[vendor299.selected].id].x;v1.y=rotations2_299[vendor299.entries[vendor299.selected].id].y;v1.z=rotations2_299[vendor299.entries[vendor299.selected].id].z;
        call299_001FA1F8((char *)vendor299.preview+0x3C0,&((Moby299 *)((char *)vendor299.preview+0x300))->rot);
        call299_001F9EC0(&v1,&v1,(char *)vendor299.preview+0x3C0);
        call299_001F9BD8((VV299 *)((char *)vendor299.preview+0x310),(VV299 *)((char *)vendor299.preview+0x310),&v1);
        vendor299.timer++;
        }
        if(!vendor299.buy && !G00161F18_299) {
            if(vendor299.count<8) {
                if((BUTTON299&0x2000) && vendor299.selected<vendor299.count-1) {
                    G00161F34_299=1;call299_L00_0029C840(0);vendor299.selected++;call299_0022ED80(1,0,vendor299.main);
                    call299_002391A8(call299_001FE540(texts299[SELECTION299].text));
                    if(vendor299.handle!=-1)call299_001FFDA0(vendor299.handle,0);
                    vendor299.handle=-1;
                    if(SELECT299[vendor299.selected].kind==1) {
                        vendor299.handle=call299_001FFB38(48,SELECTION299+60000,(s32)callback299_L00_0023B0F8,(s32)callback299_L00_00236830,(s32)callback299_L00_0023B140,(s32)((char *)purchase299+33+SELECTION299*4),descriptions299[SELECTION299].count);
                    }
                }
                if((BUTTON299&0x8000) && vendor299.selected>0) {
                    G00161F34_299=1;call299_L00_0029C840(0);vendor299.selected--;call299_0022ED80(1,0,vendor299.main);
                    call299_002391A8(call299_001FE540(texts299[SELECTION299].text));
                    if(vendor299.handle!=-1)call299_001FFDA0(vendor299.handle,0);
                    vendor299.handle=-1;
                    if(SELECT299[vendor299.selected].kind==1)goto selected_sound299;
                }
            } else {
                if((BUTTON299&0x2000) && vendor299.scroll<57) {
                    vendor299.scroll+=56;small00161F34_299=1;call299_0022ED80(1,0,vendor299.main);call299_L00_0029C840(0);
                    if(vendor299.selected<vendor299.count-1)vendor299.selected++;else vendor299.selected=0;
                    call299_002391A8(call299_001FE540(texts299[SELECTION299].text));
                    if(vendor299.handle!=-1)call299_001FFDA0(vendor299.handle,0);
                    vendor299.handle=-1;
                    if(SELECT299[vendor299.selected].kind==1) {
                        vendor299.handle=call299_001FFB38(48,SELECTION299+60000,(s32)callback299_L00_0023B0F8,(s32)callback299_L00_00236830,(s32)callback299_L00_0023B140,(s32)((char *)purchase299+33+SELECTION299*4),descriptions299[SELECTION299].count);
                    }
                }
                if((BUTTON299&0x8000) && vendor299.scroll>=-56) {
                    vendor299.scroll-=56;small00161F34_299=1;call299_0022ED80(1,0,vendor299.main);call299_L00_0029C840(0);
                    n=vendor299.selected;if(n<=0)n=vendor299.count;vendor299.selected=n-1;
                    call299_002391A8(call299_001FE540(texts299[SELECTION299].text));
                    if(vendor299.handle!=-1)call299_001FFDA0(vendor299.handle,0);
                    vendor299.handle=-1;
                    if(SELECT299[vendor299.selected].kind==1) {
selected_sound299:
                        vendor299.handle=call299_001FFB38(48,SELECTION299+60000,(s32)callback299_L00_0023B0F8,(s32)callback299_L00_00236830,(s32)callback299_L00_0023B140,(s32)((char *)purchase299+33+SELECTION299*4),descriptions299[SELECTION299].count);
                    }
                }
            }
            if((BUTTON299&64) && SELECT299[vendor299.selected].price==0) {vendor299.buy=1;call299_0022ED80(0,0,vendor299.main);}
            if((BUTTON299&16) && small0016124C_299==0) {G00161F18_299=1;G00161F14_299=8;call299_0022ED80(5,0,vendor299.main);}
        }
        call299_L00_0029CDE0();n=small00161F18_299;
        if(G00161F20_299==1) {
            if(G00161F1C_299==0)small00161F20_299=0;else G00161F1C_299--;
            n=G00161F18_299;
        }
        if(n==1) {
            i=G00161F14_299-1;small00161F14_299=i;
            if(i==0) {
                G0015F6BC_299=n;call299_002348B8();call299_00213DE0(vendor299.main,4,9,8);
                vendor299.main->rate=D_0015EE60_299*-0.5f;
                if(vendor299.own)qcopy(&camera_target299,&vendor299.view70);
                { Hero299 *hero_state=(Hero299 *)(hero_data299+0xE1D);
                camera_rows299.row[2]=snapshot299[2];vendor299.mode=2;
                camera_rows299.row[0]=snapshot299[0];camera_rows299.row[1]=snapshot299[1];
                vendor299.timer=0;vendor299.f8=0;
                if(HERO299) {
                    call299_00205270(HERO299->class_id,-1);HERO299->cls=classes299[HERO299->slot];call299_0020D6D0(HERO299);
                }
                }
            }
        } else if(small00161F34_299)call299_L00_0029C9E8();
        call299_L00_0023B610(1);break;
    case 3:
        call299_L00_00299108();call299_00213C78();call299_0022EF68();call299_L00_002076E8();call299_00218A80();
        scene299.frame++;scene299.timer++;
        if(scene299.length-12<scene299.timer ?
            (limit=1.0f,fade=D_L00_0015F4FC_299+0.1f,D_L00_0015F4FC_299=fade,fade>limit) :
            (limit=0.0f,fade=D_L00_0015F4FC_299-0.125f,D_L00_0015F4FC_299=fade,fade<limit))
            D_L00_0015F4FC_299=limit;
        scene=&scene299;
        if(scene->timer>=scene->length) {
            D_L00_0015F4FC_299=0.0f;call299_002348B8();D_L00_0016CBF0_299=0.63f;call299_001F3140();
{
                s32 destroy_i; Moby299 *destroy_m; Class299 *destroy_cls;
            for(destroy_i=0;destroy_i<scene->objects_count;destroy_i++) {
                destroy_m=scene->objects[destroy_i];
                if(destroy_m) {destroy_m->cls->count--;destroy_cls=destroy_m->cls;destroy_cls->bindings[destroy_cls->count]=0;call299_0020D678(destroy_m);}
            }
            }
            if(vendor299.camera)call299_L00_0029C2E8();else call299_L00_0029BED8(1);
        } else {
            s32 limit_ticks=96;
            VV299 *rotation;
            if(D_0015EE80_299)limit_ticks=80;
            if(scene->frame>=limit_ticks) {scene->count++;call299_00205220(scene->count);}
            frame=scene->frames+scene->frame;rotation=&frame->rot;snap=((u8 *)&frame->pos)[12];
            D_L00_0016CBF0_299=rotation->w;call299_001F3140();
            qcopy(&camera_pos299,&frame->pos);
            call299_001F9EC0(&camera_pos299,&camera_pos299,&transform299);
            call299_001F9BD8(&camera_pos299,&camera_pos299,(char *)&transform299+64);
            angle=call299_001FA748(rotation->z,((Vendor299 *)((char *)&transform299-128))->transform.row[3].z);
            call299_00125358((float *)&mat);call299_001254A0((float *)&mat,(float *)&mat,frame->rot.x);
            call299_00125548((float *)&mat,(float *)&mat,rotation->y);call299_001253F8((float *)&mat,(float *)&mat,angle);
            {char *cam=(char *)&camera_pos299-0x140;
             float r20=-mat.row[2].x,r00=-mat.row[0].x,r21=-mat.row[2].y,r01=-mat.row[0].y;
             float r22=-mat.row[2].z,r02=-mat.row[0].z,r10=mat.row[1].x,r11=mat.row[1].y,r12=mat.row[1].z;
             *(float *)(cam+0x378)=r12;*(float *)(cam+0x350)=r20;*(float *)(cam+0x360)=r00;
             *(float *)(cam+0x370)=r10;*(float *)(cam+0x354)=r21;*(float *)(cam+0x364)=r01;
             *(float *)(cam+0x374)=r11;*(float *)(cam+0x358)=r22;*(float *)(cam+0x368)=r02;}
            for(i=0;i<scene->objects_count;i++) {
                m=scene->objects[i];n=scene->frame>>1;m->anim1=n+1;m->anim0=n;call299_0020D6D0(m);
                m->fraction=call299_001FA888(scene->frame&1)*0.5f;
                if(snap && (scene->frame&1))m->fraction=1.0f;
                position=&m->pos;track=m->anim;
                call299_001F9C30(&v0,track+m->anim0,1.0f-m->fraction);
                call299_001F9C30(&v1,track+m->anim1,m->fraction);
                call299_001F9BD8(position,&v0,&v1);call299_001F9EC0(position,position,&transform299);
                call299_001F9BD8(position,position,(char *)&transform299+64);
                m->opacity=255;m->rot.z=((Vendor299 *)((char *)&transform299-128))->transform.row[3].z;
                call299_L00_00251E30(m);if(m->flags)call299_00214550(m);if(!m->class_id)call299_L00_002353B8(m);
            }
            call299_00202260();call299_L00_00299148();
        }
        break;
    }
    call299_0022DD68();
}
