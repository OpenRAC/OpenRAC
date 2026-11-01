/* GPT-6.1 Sol high worker, USA v1.01, default game -O2 -G0 profile.
 * Best retained check.py: 1032/1036 bytes; 0 of 28 places match.
 * Corrected the earlier draft's missing second string in the D_001ADB58 and
 * D_001ADB90 calls. Retail keeps it in t2; both call tails now remain separate
 * without an alias, and formatting through +0x1F4 matches apart from relocations.
 * Remaining: screen-height high/low split, speculative lui in the centering
 * branch delay slot (retail has li 1), centering register/arithmetic ordering,
 * and rectangle flags/center-coordinate load scheduling. Not runtime validated.
 * Scalar, array, struct, nosda and .data forms retain the 1032-byte outcome.
 * A dedicated -mno-split-addresses trial emitted 1036 bytes and fixed the tied
 * height load but grouped every format-string address, disrupting their retail
 * instruction interleaving. SDA/MACRO_ADDR forms emitted a GP load in the branch
 * slot and 1028 bytes; -fno-sched-spec and -Wa,-G0 did not fix that trial.
 * Explicit height/midpoint locals and store-order variants did not close it.
 * Next: a measured declaration/source layout that keeps the height absolute
 * load tied while preserving split format addresses, then rectangle scheduling.
 * All required declarations are below; no additional headers are needed.
 */
extern int func_00115DA8(char *, char *, ...);
extern char D_001ADB58[], D_001ADB68[], D_001ADB78[], D_001ADB90[];
extern char D_001ADBB0[], D_001ADBB8[], D_001ADBC8[];
extern int D_001A7344;
extern void func_00280200(void *, int, char *, int);
extern void func_00280190(void *, int, char *, int);
extern void func_00280270(void *, int, char *, int);
extern void func_0027EB88(int, int, int, int, int, unsigned int);
extern void func_003384C0(void *, int, unsigned int, float, float, float, float);

void func_003380B0(void *object, short *box, int font, char *text, int mode,
                     int center, int color, int single, char *first, char *second, int third)
{
    char buffer[768];
    int flags = box[9];
    int top, bottom, left, right;
    int halfWidth, halfHeight;
    box[9] |= 3;
    if (first) {
        if (second) {
            if (third) {
                if (single) func_00115DA8(buffer, D_001ADB58, text, 1, first, 1, second);
                else func_00115DA8(buffer, D_001ADB68, text, 1, 1, first, 1, second);
            } else {
                if (single) func_00115DA8(buffer, D_001ADB78, text, 1, first, second);
                else func_00115DA8(buffer, D_001ADB90, text, 1, 1, first, second);
            }
        } else {
            if (single) func_00115DA8(buffer, D_001ADBB0, text, 1, first);
            else func_00115DA8(buffer, D_001ADBB8, text, 1, 1, first);
        }
    } else func_00115DA8(buffer, D_001ADBC8, text);
    box[9] |= 4;
    switch (mode) {
    case 0: func_00280200(box, font, buffer, color); break;
    case 1: func_00280190(box, font, buffer, color); break;
    case 2: func_00280270(box, font, buffer, color); break;
    }
    if (center) {
        int y = (D_001A7344 - box[7]) >> 1;
        box[1] = y + (unsigned short)box[7];
        box[5] = y + (box[7] >> 1);
        box[0] = y;
    }
    switch (mode) {
    case 0: func_00280200(box, font, buffer, color); break;
    case 1: func_00280190(box, font, buffer, color); break;
    case 2: func_00280270(box, font, buffer, color); break;
    }
    halfWidth = (box[6] >> 1) + 10;
    halfHeight = (box[7] >> 1) + 5;
    left = box[4]; top = box[5];
    right = left + halfWidth; bottom = top + halfHeight;
    top -= halfHeight; left -= halfWidth;
    box[9] &= ~4;
    func_0027EB88(top, bottom, left, right, 96, 0x60442D00);
    func_003384C0(object, 96, 0x55F0C070, (float)top, (float)bottom, (float)left, (float)right);
    switch (mode) {
    case 0: func_00280200(box, font, buffer, color); break;
    case 1: func_00280190(box, font, buffer, color); break;
    case 2: func_00280270(box, font, buffer, color); break;
    }
    box[9] = flags;
}


