/* 0 of 1 places match - last line: 19_grelbin: 168 bytes, expected 176
   The body is exact; the catalogue extent is not.  This C compiles to 168 bytes in which all 42
   instructions equal retail's (the %hi/%lo/$gp words resolve to D_L19_001FB5A0, D_L19_001B2A1C
   and D_L19_001B2A38 under Placement.derive).  Retail's row has 176 bytes because the extents
   merged a second function into it: the `jr $ra; nop` at +0xA8 (0x0041C3D0) is an empty method of
   its own, entered 15 times from the level's vtbl (0x002A71D8, 0x002A71E4, ... 0x002A7280, every
   0xC bytes); nothing falls into it and every branch of this function goes to the first return
   at +0xA0.  A function's C cannot hold it: the build gives each function its own
   -ffunction-sections section, so the body and a separate `void f(void) {}` are two sections
   (the same merge makes func_L14_004321C0, func_L13_0043E8C0, func_L06_0047C1D0 and
   func_L00_0042A4E8 rows of stubs entered from a vtbl/camvtbl).  Tried, all still 168 bytes with
   the same words: a final `return;`, and the walk wrapped in `if (arg0[0x21] != 0xFF) { ... }`
   instead of the early return.
   Needs: tools/levels.py to split the row at 0x0041C3D0 into this function (168 bytes) and an
   8-byte empty function; this body then places as is.
   Same shape as func_L07_003E5AC8 (matched), with +0x240 instead of +0x244. */

typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;
typedef int s32;

extern u16 *D_L19_001FB5A0[];

/* `sda` on an array of unknown size: cc1 emits the one-instruction load, which the assembler
   expands absolute (`lui $r` / `lw $r,%lo($r)`), as retail has it in this -G0 unit. */
extern u8 *D_L19_001B2A1C[] __attribute__((sda));
extern s32 D_L19_001B2A38[] __attribute__((sda));

/* Sets the +0x240 word of every entity of the object's group state to 1. */
void func_L19_0041C328(u8 *arg0)
{
    u16 *p;
    u8 *e;

    if (arg0[0x21] == 0xFF)
        return;
    p = D_L19_001FB5A0[arg0[0x21]];
    while (arg0[0x21] != 0xFF && arg0[0x21] <= D_L19_001B2A38[0] && p != 0 &&
           ((p != 0 && (s16)p[-1] >= 0) || p == D_L19_001FB5A0[arg0[0x21]])) {
        e = D_L19_001B2A1C[0] + ((*p & 0x7FFF) << 8);
        if (e == 0)
            break;
        *(s32 *)(*(u8 **)(e + 0x68) + 0x240) = 1;
        p++;
    }
}
