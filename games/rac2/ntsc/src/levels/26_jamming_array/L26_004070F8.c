#include "common.h"

extern void func_002A7750(char *, int, int);
extern int func_00288E18(int, int);

extern int D_L26_001A8FF0[] __attribute__((sda));
extern int D_001A79F0[] __attribute__((sda)) NOT_SDA;
typedef struct {
    unsigned short count, pad;
    unsigned int time, levels;
} SubtitleHistory;
extern SubtitleHistory D_0018C958[];
extern int D_0018C0B4 NOT_SDA;
extern int D_L26_001B1748[] __attribute__((sda));
extern unsigned int D_001A7390[] __attribute__((sda));

typedef struct {
    int f0;
    char pad[0x20];
    int f24;
} D_L26_0027D5C8_t;

typedef struct {
    char pad[0x1A4];
    unsigned short f1a4;
    int f1a8;
} D_0018C750_t;

extern D_L26_0027D5C8_t D_L26_0027D5C8;
extern D_0018C750_t D_0018C750;

void func_L26_004070F8(char *a0) {
    char *m = a0;
    char *ctl = *(char **)(m + 0x68);
    u8 kind = *(u8 *)(m + 0x20);
    unsigned int mask;

    switch (kind) {
    case 0:
        *(u8 *)(m + 0x30) = 0xFF;
        *(int *)(ctl + 0x4) = 0;
        *(int *)(ctl + 0x10) = 0;
        func_002A7750(m, 1, -1);
        break;
    case 1:
        if (D_L26_001A8FF0[0] != 0) {
            if (D_L26_0027D5C8.f0 == 0 && D_L26_0027D5C8.f24 == -1) {
                unsigned int flags = D_0018C958[28].levels;
                flags &= (unsigned int)kind << D_001A79F0[0];
                if (flags == 0) {
                    func_00288E18(0x9C5, 0x1C);
                }
            }
        }
        if (D_0018C750.f1a4 == 0 ||
            0x1A5E0u <
                (unsigned int)(D_001A7390[0] - D_0018C750.f1a8)) {
            if (D_0018C0B4 == 0 && D_L26_001B1748[0] >= 0x4B1) {
                unsigned int flags = D_0018C958[12].levels;
                mask = 1u << D_001A79F0[0];
                if ((flags & mask) == 0) {
                    func_00288E18(0x9B5, 0xC);
                }
            }
        }
        break;
    }
}
