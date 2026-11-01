/* Claude Opus 5.5, xhigh tier, USA v1.01, 31 October 2026.
 * 1552/1540 bytes, 0/1 places, -O2 -G8 -ffunction-sections, in a unit of its own
 * (src/game/0032EB28-trigger-g8.c, for the jump table's .rodata). Closer than the earlier
 * 1532-byte draft, whose size only looked close: the 12 extra bytes here are the spill
 * below. No linked build was run. All required declarations are below.
 *
 * What still differs: `first` gets no register, so it is spilled (frame 0x60, sw/lw).
 * Retail: s5=first, s6=allowed, s7=-1, fp=%hi(D_001BAD40); the hoisted constant 1 and
 * &D_00139768 get no register, and reload rematerialises them in $v1 inside the loop.
 * Here: s5=allowed, s6=-1, s7=%hi(D_001BAD40), fp=1, first on the stack.
 * The aligned diff shows only that allocation, the frame offsets it causes, and one delay
 * slot: at `beqz $s2` retail takes `lui $a0` from the fatal block, this fills with
 * allowed=0. Whether that slot follows from the allocation is unverified.
 *
 * tools/regalloc.py priorities: allowed 8/140, -1 3/82, %hi 1/28, 1 3/86, first 3/118.
 * Retail's order needs first above allowed, i.e. 4 refs (8/118) at the same live length.
 * That is inferred from the assignment, not observed in a compile.
 * Tried: a redundant `first=1` after the first guard (4 refs, but live length doubles to
 * 236, so 2/59); `else first=1` (same); nested guards instead of && (same bytes);
 * -fno-expensive-optimizations (same loop decisions).
 * Next: find a real fourth reference to first that is gone by the final code and leaves
 * its live length alone.
 *
 * Established, keep these:
 * - State 1 falls through into state 2; it breaks only if nothing armed it.
 * - The source guard is two tests. Retail's third test (list[0] != -1) is the for loop's
 *   duplicated exit test, and `first=0` is the body's first statement; loop.c hoists it
 *   to where retail has it.
 * - func_002F5CC8 is called if D_001B1688 < 5 || (level == 6 && !event). Earlier drafts
 *   had this condition wrong.
 * - A separate mask temporary keeps sllv + and (no srlv + andi from combine).
 * - The t1/t0 copies are gcse PRE of the level and event loads. PRE needs a block between
 *   the join after the failed call and the continue label to insert the reloads.
 *   `changed=1` below makes that block, and flow deletes it as dead. CAUTION: this is a
 *   dead assignment with no evidence of the original statement. It needs review, and an
 *   exact-bytes result relying on it must say so.
 * - States 3 and 4 use separate changed variables (s2, s0); changed=1 goes after the
 *   level != 24 call.
 * - D_001A7BC8, D_001B1688 and the absolute read of D_001A79F0 are retail's tied macro
 *   form: incomplete sda arrays (D_001A7BC8 also gives the beql at state 0). The $gp reads
 *   of D_001A79F0 need the SDATA view, or the assembler makes the absolute alias $gp too.
 * - Earlier notes wanted the D_00139768 hoist stopped. It need not stop: per the assignment
 *   above it is hoisted in retail too and simply loses the register.
 */
#include "common.h"

typedef struct {
    int cuboid;
    int instance;
    u8 event;
    s8 level;
    u8 armed;
    s8 sound;
    union { struct { s8 movie; u8 queue; u8 pad; u8 active; } bytes; u32 word; } flags;
} TriggerState;
typedef struct { char pad[0x2294]; int weapon; char pad2298[0x234]; int enabled; } TriggerHero;
extern TriggerHero D_00189E20;
extern u8 D_001A7BC8[] __attribute__((sda));
extern u8 D_0019B2E8[];
extern int D_001A79F0 SDATA(D_001A79F0);
extern int D_001A8BB0 __attribute__((sda));
extern int D_001B1688[] __attribute__((sda));
extern short **D_0026C320[];
extern u32 D_00139768[];
extern char D_001BAD40[];
extern char D_001B0ED0[];
extern char D_0026E6A0[];
extern char *D_001B1C4C __attribute__((sda));
extern int D_001A79F0_abs[] __asm__("D_001A79F0") __attribute__((sda));
extern void func_0029F338(void *);
extern void func_002A7750(void *,int,int);
extern int func_002B38B8(void *);
extern int func_002ACBC8(void *,int);
extern int func_00288AA0(void *,int);
extern void func_0026FEB8(const char *,...);
extern int func_00288BE8(void *);
extern void func_002F5CC8(void);
extern int func_002F5D10(void);
extern void func_0026F778(void *,void *,int,int);
extern void func_002AD7B8(int);
extern void func_002AF0A8(int);
extern void func_002A0430(int);
extern void func_002DF3F8(void *,void *);
extern void func_00288CB0(int,int);
extern void func_0029B3E0(int,int);
#define UB(p,o) (*(u8 *)((char *)(p)+(o)))
#define SH(p,o) (*(s16 *)((char *)(p)+(o)))
void func_003334B0(char *obj)
{
    TriggerState *s=*(TriggerState **)(obj+0x68);
    int changed;
    int changed4;
    switch(UB(obj,0x20)) {
    case 0:
        if ((s->level!=-1 && D_001A7BC8[s->level]) ||
            (UB(obj,0xb0)!=255 && (D_0019B2E8[UB(obj,0xb0)+(D_001A79F0<<4)]&0x80))) {
            func_0029F338(obj); break;
        }
        func_002A7750(obj,1,-1);
    case 1:
        if(!s->armed && !func_002B38B8(obj) && !UB(obj,0xbc)) break;
        s->armed=1;
        func_002A7750(obj,2,-1);
    case 2:
        if(!s->armed) { func_002A7750(obj,1,-1); break; }
        if(D_00189E20.weapon==28 || D_00189E20.enabled<=0 || D_001A8BB0) break;
        if(s->cuboid!=-1 && !func_002ACBC8((char *)&D_00189E20+0x80,s->cuboid)) break;
        {
            int allowed=1, first=1;
            int i;
            if(D_0026C320[D_001A79F0] && D_0026C320[D_001A79F0][s->event]) {
                for(i=0;D_0026C320[D_001A79F0][s->event][i]!=-1;i++) {
                    short sid=D_0026C320[D_001A79F0][s->event][i];
                    u32 id=sid;
                    first=0;
                    if(!s->flags.bytes.queue) {
                        u32 mask=1u<<(id&31);
                        u32 word=id>>2;
                        if(mask & D_00139768[word]) continue;
                    }
                    if(!func_00288AA0(D_001BAD40,(int)id)) {
                        if(i) {
                            allowed=0;
                        } else {
                            func_0026FEB8(D_0026E6A0,SH(obj,0xaa),SH(obj,0xb2),D_001A79F0,s->event,(short)id);
                            return;
                        }
                    }
                    changed=1;
                }
            }
            if(first) func_0026FEB8(D_001B0ED0,D_001A79F0,s->event);
            if(allowed) { s->flags.bytes.active=0; func_002A7750(obj,4,-1); }
            else {
                s->flags.bytes.active=1;
                func_00288BE8(D_001BAD40);
                if(D_001B1688[0]<5 || (D_001A79F0==6 && !s->event)) func_002F5CC8();
                func_002A7750(obj,3,-1);
            }
        }
        break;
    case 3:
        if(!func_002F5D10()) {
            if(s->instance!=-1) {
                char *p=D_001B1C4C+(s->instance<<7);
                func_0026F778(p+0x30,p+0x70,0,1);
            }
            func_002A7750(obj,4,-1);
        }
        if(D_001A79F0_abs[0] || s->level==-1 || func_002F5D10()) break;
        changed=0;
        if(s->level!=-1) {
            func_002AD7B8(s->level);
            if(s->level!=24) func_002AF0A8(s->level);
            changed=1;
        }
        if(UB(obj,0xb0)!=255 && !(D_0019B2E8[UB(obj,0xb0)+(D_001A79F0<<4)]&0x80)) {
            func_002A0430(UB(obj,0xb0)); changed=1;
        }
        if(s->sound) {
            if(s->flags.word & 0xffff0000) {
                if(s->instance!=-1) {
                    char *p=D_001B1C4C+(s->instance<<7);
                    func_002DF3F8(p+0x30,p+0x70);
                }
            } else func_002DF3F8(obj+0x10,obj+0xf0);
        }
        if(s->flags.bytes.movie!=-1) func_00288CB0(s->flags.bytes.movie,1);
        else if(changed || s->flags.bytes.active) func_0029B3E0(0,-1);
        break;
    case 4:
        changed4=0;
        if(s->level!=-1) {
            func_002AD7B8(s->level);
            if(s->level!=24) func_002AF0A8(s->level);
            changed4=1;
        }
        if(UB(obj,0xb0)!=255 && !(D_0019B2E8[UB(obj,0xb0)+(D_001A79F0<<4)]&0x80)) {
            func_002A0430(UB(obj,0xb0)); changed4=1;
        }
        if(s->sound>0) {
            if(s->flags.word & 0xffff0000) {
                if(s->instance!=-1) {
                    char *p=D_001B1C4C+(s->instance<<7);
                    func_002DF3F8(p+0x30,p+0x70);
                }
            } else func_002DF3F8((char *)&D_00189E20+0x80,(char *)&D_00189E20+0x90);
        }
        if(s->flags.bytes.movie!=-1) func_00288CB0(s->flags.bytes.movie,1);
        else if((changed4 || s->flags.bytes.active) && s->sound!=-1) func_0029B3E0(0,-1);
        func_0029F338(obj);
        break;
    }
}
