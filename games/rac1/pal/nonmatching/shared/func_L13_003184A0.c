/* NON_MATCHING func_L13_003184A0 -- src/overlays/shared/vendor_002B8FC0.c
 * Best so far: SIZE ours 60 / retail 56, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef struct {
    char pad0[0x39];
    unsigned char field39;
} Level13VendorItem;

typedef struct {
    char pad0[0x1C];
    Level13VendorItem *item;
} Level13VendorRecord;

typedef struct {
    char pad0[0x86];
    short class_id;
} Level13VendorCurrentMoby;

extern Level13VendorCurrentMoby *D_L13_00167180;
extern Level13VendorRecord *D_L13_0015F050;

void func_L13_003184A0(int index) {
    if (D_L13_00167180->class_id == 0x13) {
        D_L13_0015F050[index].item->field39 = 1;
    }
}
