/* The save-info slots at D_001393E0, as in func_0029B0B0 (0029AE78.c): the entries start at 0x30.
 * The other fields' roles are unknown. D_00258D70 holds two ints per entry. */
typedef struct {
    int first, second, third, fourth;
    char stamp[8];
    int invalid;
} SlotEntry_002DC900;

typedef struct {
    char pad000[0x8];
    int state;
    char pad00C[0x30 - 0xC];
    SlotEntry_002DC900 entries[4];
    char pad0A0[0x15C - 0xA0];
    int mode;
    int pad160;
    int selected;
} Slots_002DC900;

extern Slots_002DC900 D_001393E0;
extern char D_001F2840[];
extern int D_00258D70[];
extern int D_001ABBEC __attribute__((sda));
extern void func_0027C540(long);
extern void func_0027C660(void);
extern char *func_00288FB8(int);
extern char *func_002DF1F8(char *, int);
extern void func_0027FAA8(int, int, unsigned long, char *, int);

/* Menu line for the current slot: prints its name (and the follow-up line)
   while the menu is in its listing state. */
int func_002DC900(int *box)
{
    char buf[64];
    char *g = D_001F2840;
    Slots_002DC900 *s = &D_001393E0;
    int idx;
    int next;
    int y;

    idx = s->entries[*(int *)(*(char **)(*(char **)(g + 0x14) + 0xE8) + 0x40)].first % 0x1C;
    func_0027C540(0);
    if (s->mode < 3 && s->selected < 0 && *(int *)(g + 0x164) >= 11 && s->state == 2) {
        if (idx == -1) {
            func_0027FAA8(box[6], box[7] + D_001ABBEC / 2, 0x80F0F0F0, func_00288FB8(0x2F14), -1);
        } else {
            next = D_00258D70[idx * 2 + 1];
            y = 0;
            if (next < 0) y = D_001ABBEC >> 1;
            func_0027FAA8(box[6], box[7] + y, 0x80F0F0F0, func_002DF1F8(buf, idx), -1);
            if (next >= 0) {
                func_0027FAA8(box[6], box[7] + D_001ABBEC, 0x80F0F0F0, func_00288FB8(D_00258D70[idx * 2 + 1]), -1);
            }
        }
    }
    func_0027C660();
    return 2;
}
