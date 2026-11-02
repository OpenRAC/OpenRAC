/*
 * GPT-6.1 Sol high, v1181, 2026-10-29: retained near miss, no linked credit.
 * Profile: -O2 -G8 -ffunction-sections (default for a -g8.c unit).
 * US, US2, PAL: 1596/1596 bytes, 0/3 places each; first mismatch at +0x38.
 * The explicit condition label and repeated state-slot reads replace the older
 * 1660-byte draft. Ordinary while form grows to 1876 bytes; keeping a group-id
 * snapshot gives 1580 bytes with additional register differences.
 * Remaining differences: the five initial table-high copies into t3 are moved
 * ahead of their indexed loads instead of filling the branch delay slots; the
 * chained scans use a1 instead of a0 for the group-id lifetime. About 41
 * non-address instruction rows differ in the US diff. The loop bodies and
 * eight-case switch now have the retail layout and length.
 * Direct reads, scoped locals and an inline group-id accessor tie. -g,
 * -fno-rerun-loop-opt, -fno-sched-interblock and -frac2-gcse-unsplit-edges
 * tie on this source. Disabling sched1, sched2 or GCSE worsens the result.
 * Next useful input: evidence for the original iterator's pointer/index
 * lifetimes or the scheduler's treatment of the table-high copies. Do not
 * repeat the tied source shapes or restore -mno-split-addresses (1632 bytes
 * with explicit condition labels; original retained source was 1660 bytes).
 * Declarations required by this standalone draft follow below.
 */
#include "common.h"
extern unsigned short *D_L18_001FB320[];
extern char *D_L18_001B281C[] __attribute__((sda));
extern int D_L18_001B2838 __attribute__((sda));

/* Each group is a terminated stream of 15-bit actor indices. */
#define DISABLE_GROUP(field) do { \
    char *member; \
    unsigned short *entry; \
    unsigned short flags; \
    entry=D_L18_001FB320[*(int *)(state+(field))]; \
    goto test_##field; \
body_##field: \
    member=D_L18_001B281C[0]+(*entry&0x7fff)*0x100; \
    if(member==0) goto end_##field; \
    flags=*(unsigned short *)(member+0x34)&0xffbc; \
    *(unsigned short *)(member+0x34)=flags|0x1000; \
    *(int *)(member+0x98)=*(int *)(*(char **)(member+0x24)+0x10); \
    if(*(short *)(owner+0xaa)!=0xb69) { \
        member[0x31]=0; \
        *(unsigned short *)(member+0x34)=flags|0x1001; \
    } \
    entry++; \
test_##field: \
    if((*(int *)(state+(field))&0xff)!=0xff && *(int *)(state+(field))<=D_L18_001B2838 && entry!=0 && \
       ((short)entry[-1]>=0 || entry==D_L18_001FB320[*(int *)(state+(field))])) goto body_##field; \
end_##field: ; \
} while(0)

void func_L18_003FA180(char *owner,unsigned int mode)
{
    char *state=*(char **)(owner+0x68);
    if(mode>=8) return;
    switch(mode) {
    case 0:
        DISABLE_GROUP(0x60);
        DISABLE_GROUP(0x64);
        break;
    case 1:
        DISABLE_GROUP(0x68);
        break;
    case 2:
        DISABLE_GROUP(0x6c);
        DISABLE_GROUP(0x74);
        DISABLE_GROUP(0x78);
        break;
    case 3:
        DISABLE_GROUP(0x70);
        break;
    case 4:
        DISABLE_GROUP(0x7c);
        break;
    case 5: case 6: case 7:
        return;
    }
}
#undef DISABLE_GROUP
