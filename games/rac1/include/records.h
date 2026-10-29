#ifndef RECORDS_H
#define RECORDS_H

/* Small records many level files read through the same layout. */

/* A function-pointer dispatch entry (0x14 bytes) in the camera and mode tables. */
typedef struct {
    char unk_00[8];
    void (*fn_08)(void *);
    char unk_0C[4];
    void (*fn_10)(void *);
} DispatchRec;

/* The slot tables of the pad-driven selector state at D_001A01F0. */
typedef struct {
    int _pad0[0x9E];
    int use[5];
    int flags[5];
    int sel;
    int size[5];
} PadSlots;

/* An animation sequence header. */
typedef struct AnimSeq {
    u8 _pad00[0x10];
    u8 nframes;
    u8 unk11;
    u8 trigger_count;
    u8 _pad13;
    s32 f14;
    s32 _pad18;
    void *frames[1];
} AnimSeq;

/* Object (TIE) render class header. */
typedef struct ObjectRenderClass {
    s32 lod_tables[3];          /* 0x00: relative offsets to LOD tables */
    s32 f0C;                    /* 0x0C */
    u8 pad10[0x10];             /* 0x10 */
    u8 lod_counts[3];           /* 0x20: count for each LOD table */
    u8 packet_count;            /* 0x23: count of render packets */
    s16 flags;                  /* 0x24 */
    s16 instance_count;         /* 0x26 */
    void *first_instance;       /* 0x28 */
    s32 packets_offset;         /* 0x2C: relative offset to packets */
    u8 pad30[0x16];             /* 0x30 */
    s16 class_slot;             /* 0x46 */
    f32 scale;                  /* 0x48 */
} ObjectRenderClass;

/* Vendor item pricing and ammo capacity records (D_001E02B0). */
typedef struct VendorItemPrice {
    s32 price;            /* 0x00: item purchase price */
    s32 discount_price;   /* 0x04: discount price (Persuader) */
    u16 ammo_price;       /* 0x08: price per ammo round */
    u16 ammo_price_alt;   /* 0x0A */
    u8 pad_0C[2];         /* 0x0C */
    u16 max_ammo;         /* 0x0E: maximum ammo capacity */
    u8 pad_10[8];         /* 0x10 */
} VendorItemPrice;

/* Level collectible/save state (gold bolts, level-specific items). */
typedef struct {
    u8 pad0[0x454];
    u8 collected[1];
} LevelState;

#endif /* RECORDS_H */
