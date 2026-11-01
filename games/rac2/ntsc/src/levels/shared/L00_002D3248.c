/* The resident block the level state lives in. */
extern char D_00189E20[];

/* Writes the level state's clock pair and its packed byte pair: the two floats at
 * +0x1430 and +0x1434, the short argument at +0x1438, the byte at +0x143C, and the
 * third and fourth byte arguments spliced into the halfword at +0x143E as
 * (old & ~arg3) | arg2. */
void func_L00_002D3248(short arg0, unsigned char arg1, unsigned char arg2, unsigned char arg3,
                       float arg4, float arg5)
{
    char *base = D_00189E20;
    unsigned short old = *(unsigned short *)(base + 0x143E);

    *(float *)(base + 0x1434) = arg5;
    *(int *)(base + 0x1438) = arg0;
    *(short *)(base + 0x143C) = arg1;
    *(short *)(base + 0x143E) = (short)((old & ~arg3) | arg2);
    *(float *)(base + 0x1430) = arg4;
}
