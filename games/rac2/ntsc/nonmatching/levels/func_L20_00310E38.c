/* Retained C reconstruction; not linked or counted.
 * Profile: -O2 -G8 -ffunction-sections.
 * 1192/1212 bytes. us: 0/1 places; us2: absent; pal: absent.
 * Remaining: The object walk uses the level-state record as its table/count base; the
 * numeric level ID is only read after the walk. The annulled branch delay
 * slot is not treated as an executed load on the active-loop path. Actual
 * canonical callees, complete stack/FP inputs, callback, 64-byte producer
 * storage and 48-byte copy extent remain preserved. The 0xE0 frame agrees,
 * but base-address lifetime, branch layout and register allocation differ.
 * Reopen with: Independent level-state lifetime/branch evidence; preserve the corrected
 * table base and complete helper storage/contracts. No linked/runtime equality
 * has been established.
 */

#include "common.h"

typedef int Quad __attribute__((mode(TI)));
typedef union { Quad q; float f[4]; } Vector;
/* Measured level-state prefix; no full allocation size is inferred. */
typedef struct {
    u8 before_mode[0x30];
    int mode, state;
    u8 before_count[0xC];
    s16 count;
    u8 before_target[0x152];
    void *target;
    int owner;
    u8 before_alternate[8];
    void *alternate;
    u8 before_secondary_owner[8];
    int secondary_owner;
    void *secondary_target;
} YeedilStatePrefix;
extern YeedilStatePrefix D_L20_001BDCC0;
extern u32 D_001A79F0;
extern u8 D_00189E20[], D_0018A550[];
extern u8 D_L20_001C6AE0[], D_L20_001BA1F0[], D_L20_001B9E80[];
extern int D_L20_001A9430 __attribute__((sda));
extern int D_L20_001A9434 __attribute__((sda));
extern int D_L20_001A9438 __attribute__((sda));
extern int D_L20_001A943C __attribute__((sda));
extern int D_L20_001A9440 __attribute__((sda));
extern Vector D_L20_001A9420;
extern int D_L20_001AF3B0 NOT_SDA;
extern void func_L00_002FC980(void *);
extern void func_002E64A0(void *);
extern void func_L00_00326048(char *,float *,s32,s32,float);
extern void func_00282A60(void *,int,int);
extern void func_0029FC20(void *,int,void *);
extern void func_0029FCC0(void *,int,void *);
extern void func_00283678(void *,void *);
extern void func_00282F20(void *,void *,float);
extern void func_00282CC0(void *,void *,void *);
extern void func_00282CF0(void *,void *,void *);
extern void func_00283098(void *,void *,void *);
extern void func_0030CAD0(void *);
extern void func_0030CCD0(void *);
extern void func_L00_002E6EF8(void (*)(void *),void *);
extern void func_0030D430(void *);
extern void func_0032DA58(void *,void *,void *,float);
extern void func_L03_00326458(void);
extern void func_0032C700(void *,int,int,int,int,int,int,int,float,float,int,int,int,int);

void func_L20_00310E38(void *incoming)
{
    YeedilStatePrefix *st;
    u32 *walk;
    u8 *object;
    int i, count, lower, upper;
    s16 kind;
    void *target;
    /* 29FC20 uses the fourth row at output+0x30; 83678 copies only
     * the first three rows. The same workspace later holds one vector. */
    Vector matrix[4], work, offset;
    float value;

    func_L00_002FC980(incoming);
    st = &D_L20_001BDCC0;
    i = 0;
    count = st->count;
    if (count > 0) {
        u8 *table = (u8 *)st;
        walk = (u32 *)(table + 0x18C);
        do {
            object = (u8 *)*walk;
            if (object) {
                kind = *(s16 *)(object+0xAA);
                if (kind == 0xE29) {
                    *(u16 *)(object+0x34) |= 0x800;
                    kind = *(s16 *)(object+0xAA);
                }
                if (kind == 0xD54) func_002E64A0(object);
            }
            if (object && *(s16 *)(object+0xAA) == 0xFA0 && *(void **)(object+0x68))
                func_L00_00326048((char *)object,(float *)(*(u8 **)(object+0x68)+0xC),
                                  0x80AAAAD2,0x80396464,0.06108653545f);
            ++i;
            ++walk;
            count = *(s16 *)(table+0x44);
        } while (i < count);
    }
    if (D_001A79F0 != 20) return;
    if ((u32)(st->mode-4) < 2) {
        *(int *)(D_00189E20+0x5C0) = st->owner;
        if (st->mode == 4) {
            *(int *)(D_00189E20+0x5C0) = st->secondary_owner;
            target = st->secondary_target;
            lower = D_L20_001A9434;
            upper = D_L20_001A9438;
        } else {
            target = st->alternate;
            lower = D_L20_001A943C;
            upper = D_L20_001A9440;
        }
        if (st->state == lower) {
            func_00282A60(D_L20_001C6AE0,0,0x50);
            D_L20_001AF3B0 = 1;
            *(float *)(D_L20_001C6AE0+0x24) = 0.8f;
        }
        if (lower < st->state && st->state < upper) {
            func_0029FC20(target,0,matrix);
            func_00283678(D_0018A550,matrix);
            value = *(float *)(D_L20_001C6AE0+0x24);
            value += (12.0f-value)*0.07f;
            *(float *)(D_L20_001C6AE0+0x24) = value;
            func_0029FCC0(target,0,work.f);
            func_00282F20(D_L20_001C6AE0+0x10,D_0018A550,value);
            func_00282CC0(D_L20_001C6AE0+0x10,D_L20_001C6AE0,D_L20_001C6AE0+0x10);
            func_0030CAD0(target);
            func_0030CCD0(target);
            func_L00_002E6EF8(func_0030D430,target);
            if (st->mode == 5 && st->state == upper-1) {
                offset.q = 0;
                offset.f[0] = 18.0f; offset.f[1] = -2.5f; offset.f[2] = 1.0f;
                func_00283098(&offset,&offset,D_L20_001BA1F0);
                func_00282CC0(&offset,&offset,D_L20_001BA1F0-0x230);
                func_00282CF0(&work,D_0018A550-0x230,&offset);
                func_00282F20(&work,&work,1.0f);
                func_0032DA58(*(void **)(D_0018A550-0x170),&offset,&work,0.0f);
            }
        } else *(int *)(D_00189E20+0x5C0) = 0;
    }
    if ((u32)(st->mode-2) >= 3 || st->count <= 0) return;
    walk = (u32 *)((u8 *)st+0x18C);
    i = 0;
    do {
        u16 word;
        object = (u8 *)*walk;
        word = *(u16 *)(object+0xAA);
        if ((u32)(word-0xD0E) < 2)
            *(u16 *)(object+0x7C) = (s16)word == 0xD0F ? 15 : 0x3FF;
        if (*(s16 *)(object+0xAA) == 0xCE0 && st->mode == 3 && st->state < D_L20_001A9430)
            func_L03_00326458();
        ++i; ++walk;
        count = st->count;
    } while (i < count);
    if (st->mode == 3 && st->state == D_L20_001A9430) {
        qcopy(matrix,(u8 *)st->target+0x10);
        func_00282CC0(matrix,matrix,&D_L20_001A9420);
        func_0032C700(matrix,0,0x19,0x14,0x14,0x14,0x2D,1,1.0f,20.0f,1,1,0,0);
        *(float *)(D_L20_001B9E80+0x160) = 0.09f;
        *(int *)(D_L20_001B9E80+0x168) = 0x37;
        *(float *)(D_L20_001B9E80+0x180) = 0.08f;
        *(int *)(D_L20_001B9E80+0x188) = 0x46;
        *(int *)(D_L20_001B9E80+0x16C) = 0;
    }
}
