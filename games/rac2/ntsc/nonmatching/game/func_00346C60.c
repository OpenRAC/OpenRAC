/* Tier sol-high, USA v1.01, default game -O2 -G8 -ffunction-sections.
 * Best: 1620/1620 bytes, 0/28 places; final check.py first mismatch +0x3a4.
 * Corrected func_002E5E58 to its existing int-returning definition in
 * src/game/002E10B8.c: this fixes every selection-wrap v0/v1 difference and
 * the final selected-entry store's address register. Do not restore void.
 * Separate initialRecord from record and keep an availability-array base:
 * this removes the earlier extra instruction. Remaining differences are
 * address allocation/scheduling at +0x3a4..+0x408: mapping/count temporaries
 * occupy a1/a2 instead of a0/a1, and the availability base's low half is
 * completed in the ownership branch delay slot instead of before the branch.
 * Explicit base-pointer, integer-address and struct-member-base forms all
 * produced the same bytes; stop on that tie. Reusing selection for wrap
 * temporaries perturbs long-lived allocation; initializing wrap before the
 * sound call adds callee-saved registers. No flags/toolchain changes tried.
 * Required declarations are below. Compile as a dedicated -G8 source unit.
 * Linked placement/symbol consistency has not been checked (not a C match).
 */
extern int func_002E5E58(int, int, void *);
extern void *func_00115484(void *,int,int);
extern int func_002A9588(int,int,int,int,int);
extern int func_00335E20(unsigned char *);
extern void func_00335E38(void *,float,float,float,float);
extern char *func_00288FB8(int);
extern int func_00115DA8(char *,char *,...);
extern char D_001AE508[],D_001AE510[],D_001AE518[],D_001AE520[];
extern char D_001AE528[],D_001AE530[],D_001AE538[],D_001ADF98[],D_001AE548[],D_001ACBB8[];
extern short D_00259F58[][5];
typedef struct { unsigned char bytes[224]; } ItemRecord;
extern ItemRecord D_00239BA0[];
extern unsigned char D_00139568[],D_001A7AF8[],D_001A7B30[];
extern int D_00139688[];
extern int D_001ADB10 __attribute__((sda));
extern int D_001F2880[];

int func_00346C60(char *object,int buttons)
{
    float a[2] __attribute__((aligned(16)));
    float b[2] __attribute__((aligned(16)));
    float c[2] __attribute__((aligned(16)));
    float d[2] __attribute__((aligned(16)));
    float e[2] __attribute__((aligned(16)));
    float f[2] __attribute__((aligned(16)));
    float g[2] __attribute__((aligned(16)));
    float h[2] __attribute__((aligned(16)));
    float i[2] __attribute__((aligned(16)));
    int result=-1;
    int old=*(int *)(object+0x4fc)+*(int *)(object+0x500)*4;
    int selection,n;
    int *color;
    int entry,item,price,max;
    unsigned char *record, *available, *initialRecord;
    char *text;
    if(buttons&0x1000) {
        func_002E5E58(3,0,0);
        n=*(int *)(object+0x500)-1;
        *(int *)(object+0x500)=n<=result?5:n;
    } else if(buttons&0x4000) {
        func_002E5E58(3,0,0);
        n=*(int *)(object+0x500)+1;
        *(int *)(object+0x500)=n<=5?n:0;
    } else if(buttons&0x8000) {
        func_002E5E58(3,0,0);
        n=*(int *)(object+0x4fc)-1;
        *(int *)(object+0x4fc)=n<=result?3:n;
    } else if(buttons&0x2000) {
        func_002E5E58(3,0,0);
        n=*(int *)(object+0x4fc)+1;
        *(int *)(object+0x4fc)=n<=3?n:0;
    }
    selection=*(int *)(object+0x4fc)+*(int *)(object+0x500)*4;
    func_00115484(a,0,8);func_00115484(b,0,8);
    func_00115484(c,0,8);func_00115484(d,0,8);
    __builtin_memcpy(e,D_001AE508,8);
    func_00115484(f,0,8);
    __builtin_memcpy(g,D_001AE510,8);
    __builtin_memcpy(h,D_001AE518,8);
    __builtin_memcpy(i,D_001AE520,8);
    if(old!=selection) func_002A9588(0,0,1,0,1);
    color=(int *)func_00335E20((unsigned char *)object+0xe4);
    *color=func_002A9588(0x60442d00,0x70fffeed,20,0,0);
    func_00335E38(object+0x0,a[0]+(*(float **)(object+0x498))[0],a[1]+(*(float **)(object+0x498))[1],0.0f,0.0f);
    func_00335E38(object+0x4c,b[0]+(*(float **)(object+0x498))[0],b[1]+(*(float **)(object+0x498))[1],0.0f,0.0f);
    func_00335E38(object+0x130,c[0]+(*(float **)(object+0x498))[0],c[1]+(*(float **)(object+0x498))[1],0.0f,0.0f);
    func_00335E38(object+0x98,d[0]+(*(float **)(object+0x498))[0],d[1]+(*(float **)(object+0x498))[1],0.0f,0.0f);
    func_00335E38(object+0xe4,e[0]+(*(float **)(object+0x498))[0]+(float)*(int *)(object+0x4fc)*53.5f,e[1]+(*(float **)(object+0x498))[1]+(float)*(int *)(object+0x500)*47.0f,0.0f,0.0f);
    func_00335E38(object+0x17c,f[0]+(*(float **)(object+0x498))[0],f[1]+(*(float **)(object+0x498))[1],0.0f,0.0f);
    func_00335E38(object+0x210,g[0]+(*(float **)(object+0x498))[0],g[1]+(*(float **)(object+0x498))[1],0.0f,0.0f);
    D_001ADB10=-1;
    if(selection>=0) {
        entry=D_00259F58[selection][3];
        item=D_00139568[entry];
        initialRecord=D_00239BA0[item].bytes;
        available=D_001A7AF8;
        if(initialRecord[4]) { available[entry]=1; D_001A7B30[entry]=1; }
        if(available[entry]) {
            text=object+0x4a0;
            D_001ADB10=*(short *)(D_00239BA0[D_00139568[entry]].bytes+6);
            func_00115DA8(object+0x4b8,func_00288FB8(*(int *)(D_00239BA0[D_00139568[entry]].bytes+8)));
            record=D_00239BA0[D_00139568[entry]].bytes;
            price=D_00139688[entry];
            max=*(unsigned short *)(record+0x8e);
            if(*(unsigned short *)(record+0x88)) {
                text+=func_00115DA8(text,D_001AE528,func_00288FB8(0x2dc9));
                if(price<1000) text+=func_00115DA8(text,D_001AE530,price);
                else text+=func_00115DA8(text,D_001AE538,price/1000,price%1000);
                if(max<1000) func_00115DA8(text,D_001ADF98,max);
                else func_00115DA8(text,D_001AE548,max/1000,max%1000);
            } else {
                func_00115DA8(text,D_001ACBB8,func_00288FB8(0x2db9));
            }
            if(buttons&0x40) {
                result=selection;
                func_002E5E58(4,0,0);
                D_001F2880[0]=entry;
            }
        } else {
            object[0x4a0]=0;object[0x4b8]=0;
            if(buttons&0x40) func_002E5E58(5,0,0);
        }
    }
    func_00335E38(object+0x258,h[0]+(*(float **)(object+0x498))[0],h[1]+(*(float **)(object+0x498))[1],0.0f,0.0f);
    func_00335E38(object+0x2a0,i[0]+(*(float **)(object+0x498))[0],i[1]+(*(float **)(object+0x498))[1],0.0f,0.0f);
    return result;
}
