/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L12_003EB470: NOT matched (600 bytes, same size; control flow, calls, constants and
   data identical, about a dozen instructions differ from three causes):
   (1) `(levels & (1 << D_001A79F0)) == 0` (records 0x5D and 0x7F): cc1 folds the bit test into
       srav/andi 1, retail keeps lw/sllv/and (tried !(...), == 0, signed/unsigned shift);
   (2) the byte flags D_001A7B08[0x23] and D_001A7AF8[7] are loaded into v1 here, into the lui's
       own register v0 in retail, which also changes one delay slot (beql + lw a1 in retail);
   (3) the record-0x5C levels update computes 1 << D_001A79F0 before loading levels in retail.
   -g made no difference. Needs -G8 (D_001A7390 and D_L12_001B2E9C through $gp): its home is
   src/levels/12_todano/L12_00398E50-g8.c, which declares SubtitleHistory, D_L12_001B2E9C,
   func_00288E18 and func_002A7750. */
#include "common.h"
typedef struct {
    unsigned short count, pad;
    unsigned int time, levels;
} SubtitleHistory;
extern u8 *D_L12_001B2E9C __attribute__((sda));
extern void func_00288E18(int, int);
extern void func_002A7750(void *, int, int);
extern SubtitleHistory D_0018C958[];
extern char D_00189EA0[];
extern unsigned char D_001A7AF8[];
extern unsigned char D_001A7B08[];
extern int D_001A79F0 NOT_SDA;
extern int D_0018C0B4 NOT_SDA;
extern unsigned int D_001A7390 MACRO_ADDR;
extern int func_002ACBC8(void *, int);
extern int func_00282948(int *);

void func_L12_003EB470(u8 *self)
{
    int *state = *(int **)(self + 0x68);
    u8 *moby;

    switch (self[0x20]) {
    case 0:
        self[0x30] = 0xFF;
        state[4] = -1;
        func_002A7750(self, 1, -1);
        break;
    case 1:
        if (func_002ACBC8(D_00189EA0, state[0]) && *(int *)(D_00189EA0 + 0x540) != 0 &&
            D_0018C958[0x79].count == 0)
            func_00288E18(0xA22, 0x79);

        if (func_002ACBC8(D_00189EA0, state[1]) && D_001A7B08[0x23] == 0) {
            unsigned int now = D_001A7390;

            if (D_0018C958[0x5C].count != 0) {
                if (now - D_0018C958[0x5C].time >= 0x439 || D_0018C958[0x5C].time == 0)
                    func_00288E18(0xA05, 0x5C);
                else if (D_0018C958[0x5C].time < now)
                    D_0018C958[0x5C].time = now;
            } else {
                D_0018C958[0x5C].count++;
                if (D_0018C958[0x5C].time < now)
                    D_0018C958[0x5C].time = now;
                D_0018C958[0x5C].levels = (D_0018C958[0x5C].levels | (1 << D_001A79F0)) | 0x80000000u;
            }
        }

        if (func_002ACBC8(D_00189EA0, state[2])) {
            moby = D_L12_001B2E9C + state[3] * 0x100;
            if (moby && *(short *)(moby + 0xAA) == 0xFB8 && moby[0x20] != 0xFE && moby[0x20] != 0xFD) {
                if (state[4] < 0)
                    state[4] = 0x2A30;
                else if (state[4] == 0 && (D_0018C958[0x5D].levels & (1 << D_001A79F0)) == 0)
                    func_00288E18(0xA06, 0x5D);
            }
        }

        if (D_001A7AF8[7] != 0 && D_0018C0B4 != 0x6F && (D_0018C958[0x7F].levels & (1 << D_001A79F0)) == 0)
            func_00288E18(0xA28, 0x7F);

        if (state[4] >= 0)
            func_00282948(&state[4]);
        break;
    }
}
