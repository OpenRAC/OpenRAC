/* GPT-6.1 Sol low, task v0487, 2026-10-28.
 * 372/372 bytes, 0/28 places; USA v1.01, default -O2 -G0 -ffunction-sections.
 * Typed bank-pair arrays recover the missing address copy and full size.
 * Remaining: modulo/base register allocation and pair-load scheduling.
 * Offset-before-modulo and operand-order variants tie; repeated inline modulo grows to 380.
 * Next: recover original bank/index expression lifetimes. Declarations are below.
 */
typedef struct {
    char pad[0x44];
    int state;
    int first;
    int second;
} State2DAD18;

extern short D_001A63AC;

extern unsigned char D_001A7BB4;

typedef struct { int first, second; } BankPairAD;
typedef struct { char pad[0x36C]; int offset; char pad370[0xDD0-0x370]; BankPairAD first[5]; BankPairAD second[5]; } BankAD;
extern BankAD D_0014B540;

extern int func_002B7B18(int, int, int);

int func_002DAD18(State2DAD18 *o)
{
    switch (o->state) {
    case 0:
        if (o->first != 0) {
            if (D_001A63AC != 0)
                break;
            {
                BankAD *base = &D_0014B540;
                int offset = base->offset;
                unsigned int frame = D_001A7BB4;
                int slot = frame % 5;

                if (func_002B7B18(o->first,
                                  base->first[slot].first + offset,
                                  base->first[slot].second) != 0)
                    o->state++;
                else
                    o->state = -1;
            }
        }
        break;
    case 1:
        if (D_001A63AC != 0)
            break;
        o->state = 2;
        break;
    case 2:
        if (o->second != 0) {
            if (D_001A63AC != 0)
                break;
            {
                BankAD *base = &D_0014B540;
                int offset = base->offset;
                unsigned int frame = D_001A7BB4;
                int slot = frame % 5;

                if (func_002B7B18(o->second,
                                  base->second[slot].first + offset,
                                  base->second[slot].second) != 0)
                    o->state++;
                else
                    o->state = -1;
            }
        }
        break;
    case 3:
        if (D_001A63AC != 0)
            break;
        o->state = 4;
        break;
    }
    return 0;
}
