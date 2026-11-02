/* Unless 0x48 already holds the key with no pending value at 0x3C, stores the key at 0x48
   when 0x4E and 0x96 are clear, and otherwise queues it at 0x3C/0x3E. Own unit:
   -frac2-gcse-unsplit-edges lets PRE share the table's high half between the head and
   the tail block. */
typedef unsigned char u8;

extern u8 D_L01_001A63A8[];

void func_L01_00323310(int a0, int a1)
{
    u8 *b = D_L01_001A63A8;

    if (*(short *)(b + 0x48) == a0 && *(short *)(b + 0x3C) == -1)
        return;
    if (a0 != -1)
        a0 &= 0xFF;
    if (a1 != -1)
        a0 &= 0xFF;
    if (*(short *)(b + 0x4E) == 0 && *(short *)(b + 0x96) == 0) {
        *(short *)(b + 0x48) = a0;
        return;
    }
    {
        u8 *c = D_L01_001A63A8;

        *(short *)(c + 0x3C) = a0;
        *(short *)(c + 0x3E) = a1;
    }
}
