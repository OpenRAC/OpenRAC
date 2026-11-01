/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L00_002E3340: one instruction differs of 119 (+0x1A0): the last func_00280190 call's
   second argument 0xFFFFFFFF is built here as `lui a1,0xFFFF; dsrl32 a1,a1,0`, retail has
   `addiu a1,zero,-1; dsrl32 a1,a1,0`. Tried: 0xFFFFFFFFUL, 0xFFFFFFFFu, (unsigned long)(unsigned)-1,
   ~0UL >> 32, masked forms (all constant-fold to the same lui form). Everything else matches
   (D_001A7340 needs `extern s32 D[] __attribute__((sda))` to reload with lui/lw each use). */
typedef int s32;
typedef float f32;
typedef struct {
    char pad00[4];
    s32 base;          /* +0x04 */
    char pad08[0x0C];
    char *pixels;      /* +0x14 */
    char pad18[8];
    s32 width;         /* +0x20 */
    char pad24[4];
} Line_r002E3340;

extern s32 func_002798A0(s32, s32 *, s32 *);
extern void func_0027DE50(int, int, int, int, unsigned long);
extern void func_002802E0(void *, int, int, int, int, int, int, int, int);
extern void func_00280190(void *, unsigned long, char *, int);
extern s32 func_L00_002E36F8(s32 key);
typedef struct {
    char pad000[0x20];
    short cur;         /* +0x20 */
    char pad022[0x362];
    s32 mode;          /* +0x384 */
} Menu_r002E3340;

extern Menu_r002E3340 D_L00_001BC200;
extern Line_r002E3340 D_L00_001BC588[];
extern s32 D_001A7340[] __attribute__((sda));

void func_L00_002E3340(s32 index, f32 alpha) {
    s32 a, b;
    char *text;
    char *p;
    char frame[0x20];
    s32 w;

    if (func_002798A0(index, &a, &b)) {
        text = (char *)func_L00_002E36F8(D_L00_001BC588[a].base + b);
        if (text) {
            w = D_L00_001BC588[D_L00_001BC200.cur].width;
            p = D_L00_001BC588[D_L00_001BC200.cur].pixels + index * w;
            func_0027DE50((s32)p + 2, (s32)p + w - 2, 0, D_001A7340[0], (s32)(alpha * 96.0f) << 24);
            func_002802E0(frame, (s32)p, (s32)p + D_L00_001BC588[D_L00_001BC200.cur].width, 0, D_001A7340[0],
                          D_001A7340[0] / 2 + 2, (s32)p + D_L00_001BC588[D_L00_001BC200.cur].width / 2 + 1, D_L00_001BC200.mode, 3);
            func_00280190(frame, 0x80000000UL, text, -1);
            func_002802E0(frame, (s32)p, (s32)p + D_L00_001BC588[D_L00_001BC200.cur].width, 0, D_001A7340[0],
                          D_001A7340[0] / 2, (s32)p + D_L00_001BC588[D_L00_001BC200.cur].width / 2, D_L00_001BC200.mode, 3);
            func_00280190(frame, 0xFFFFFFFFUL, text, -1);
        }
    }
}
