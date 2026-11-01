/* Retained near miss, 24 October 2026. Game -O2 -G8 -ffunction-sections.
 * USA v1.01, PAL and USA v2.00: 244/248 bytes, 0/28 placements each.
 * Six checker invocations, no scratch compiles: raw-index form 232 bytes,
 * duplicated helper calls 264, shared call and semantic switch 244.
 * Retail dispatches types 12, 14, 17, 18 and 53 to mode 1, others to mode 2.
 * Its named table has arms at +0x60/+0x64. A computed-goto probe had different
 * arm offsets; the retained switch avoids depending on those wrong offsets.
 * Record extent, field offsets and the three-argument copy-helper interface
 * were read from retail; field roles remain inferred, not runtime verified.
 * Register allocation, address setup and a four-byte size deficit remain.
 * The size mismatch prevents relocation-qualified placement checks; generated
 * switch data would also require integration qualification. Reopen with source
 * type/lifetime evidence for the root/index and verified dispatch-arm layout.
 * 29 October 2026: setting mode 2 before the switch and assigning mode 1 in
 * the selected cases, then passing that mode directly to the shared helper,
 * compiles to 196/248 bytes and 0/28. This removes the later mode-selection
 * branch but changes the shared tail and register layout; the 244-byte
 * retained candidate remains closer. A 29 October probe capturing the type
 * field once before dispatch measures 192/248 and 0/28, but GCC lowers the
 * switch to a computed-goto table with arm offsets that do not match retail;
 * the generated table is not integration-qualified and this form is not kept.
 */
/* func_002F6510: the copied records have the measured 0xE0-byte extent. */
typedef int F6510Quad __attribute__((mode(TI)));
typedef union {
    F6510Quad raw[14];
    struct { char prefix[0x80]; int value; char tail[0x5C]; } fields;
} F6510Record;
extern int D_L00_0022F7C0[];
extern unsigned char D_00139568[];
extern F6510Record D_L00_00262570[];
extern int func_00288870(void *, int, int);

int func_002F6510(void)
{
    F6510Record local;
    int *state = D_L00_0022F7C0;
    char *row = (char *)state + state[0x80 / 4] * 0x18;
    int first = 0;
    int result = 0;
    int ok;

    if (*(int *)(row + 0x154) != 0) {
        switch (*(int *)(row + 0x140)) {
        case 12:
        case 14:
        case 17:
        case 18:
        case 53:
            first = 1;
            break;
        default:
            break;
        }
        {
            int type;
            int mode;
            if (first) {
                type = *(int *)((char *)D_L00_0022F7C0 +
                    D_L00_0022F7C0[0x80 / 4] * 0x18 + 0x140);
                mode = 1;
            } else {
                type = *(int *)((char *)D_L00_0022F7C0 +
                    D_L00_0022F7C0[0x80 / 4] * 0x18 + 0x140);
                mode = 2;
            }
            ok = func_00288870(&local, type, mode);
        }
        if (ok)
            result = local.fields.value;
    } else if (*(int *)(row + 0x150) == 0) {
        result = D_L00_00262570[D_00139568[*(int *)(row + 0x140)]].fields.value;
    }
    return result;
}
