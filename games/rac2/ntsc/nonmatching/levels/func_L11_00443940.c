/* Retained manual C reconstruction; not linked and not matching.
 * Profile: -O2 -G8 -ffunction-sections
 * Local tools/check.py measurements: us: 908 compiled bytes, 0/1 placements, exit 1; us2: 908 compiled bytes, 0/1 placements, exit 1; pal: 908 compiled bytes, 0/1 placements, exit 1.
 * Retail extent: 924 bytes. No exact-C credit and no full build performed.
 * Residual: 908 versus 924 bytes (-16), 0x60 frame. GP coordinate/color objects are explicit; FP argument order/saved registers and selection loop scheduling differ. Joba has one special entry.
 * Reopen: Reopen with a qualified selection-table layout and measured pulse/label allocation hypothesis.
 * Arrays with unknown total extent remain incomplete. Pointer-offset views do
 * not establish the full sizes or ownership of game records.
 */
/* Draws the contextual selection help and locates the next available entry. */
typedef unsigned char u8;
extern u8 *D_L11_001A904C;
extern float D_L11_001B3E04;
extern int D_L11_001B30C8;
extern u8 D_001395B8[], D_00139568[], D_00138180[];
extern u8 D_L11_00279BA0[];
extern u8 *D_L11_001B14F8[] __attribute__((sda));
extern void func_L00_0037B7B0(void);
extern int func_00336D50(u8 *, int);
extern void func_00300A00(char *, int, float, float, float, float, float);
extern unsigned int func_00283D38(unsigned int, unsigned int, float);
extern int func_L11_0043E108(int);
extern int func_00288D58(void);
extern void func_002FC560(unsigned int, unsigned long long);
extern float D_L11_001B161C __attribute__((sda));
extern int D_L11_001B1620 __attribute__((sda));
extern int D_L11_001B1624 __attribute__((sda));
extern int D_L11_001B1628 __attribute__((sda));
extern int D_L11_001B162C __attribute__((sda));
extern int D_L11_001B1630 __attribute__((sda));
extern int D_L11_001B1634 __attribute__((sda));
extern unsigned int D_L11_001B1638 __attribute__((sda));
extern unsigned int D_L11_001B163C __attribute__((sda));
extern unsigned int D_L11_001B1644 __attribute__((sda));
extern unsigned int D_L11_001B1648 __attribute__((sda));
void func_L11_00443940(u8 *object, int enabled)
{
    u8 *state;
    int label, phase, i;
    unsigned int color;
    float pulse;
    if (D_L11_001A904C == 0) return;
    func_L00_0037B7B0();
    state = *(u8 **)(object + 0x68);
    label = func_00336D50(D_L11_001A904C + 0x8710, 0x49);
    func_00300A00((char *)label, D_L11_001B163C, (float)D_L11_001B1620,
        (float)(int)((float)D_L11_001B1624 * D_L11_001B3E04 + 0.5f), D_L11_001B161C, 1.0f, 0.0f);
    label = func_00336D50(D_L11_001A904C + 0x8710, 0x48);
    func_00300A00((char *)label, D_L11_001B1638, (float)D_L11_001B1620,
        (float)(int)((float)D_L11_001B1624 * D_L11_001B3E04 + 0.5f), D_L11_001B161C, 1.0f, 0.0f);
    phase = D_L11_001B30C8 % 50;
    if (phase >= 25) pulse = 1.0f - (float)(phase - 25) * 0.04f;
    else pulse = (float)phase * 0.04f;
    color = func_00283D38(D_L11_001B1638, D_L11_001B163C, pulse);
    if (enabled) {
        if (*(short *)(state + 0x1e8) != 0) {
            label = func_00336D50(D_L11_001A904C + 0x8710, 0x4b);
            func_00300A00((char *)label, (*(unsigned int *)(D_00138180 + 0x1a0) & 0x8000) ? D_L11_001B1644 : color,
                (float)D_L11_001B1628, (float)(int)((float)D_L11_001B162C * D_L11_001B3E04 + 0.5f), D_L11_001B161C, 1.0f, 0.0f);
        }
        for (i = *(short *)(state + 0x1e8) + 1; i < 13; i++) {
            if (D_001395B8[func_L11_0043E108(*(int *)(D_L11_001B14F8[i] + 8))] == 0) continue;
            if (i >= 11 && !func_00288D58()) continue;
            if (i == 8 && D_L11_00279BA0[D_00139568[0x1e] * 0xe0 + 4] == 0) continue;
            label = func_00336D50(D_L11_001A904C + 0x8710, 0x4a);
            func_00300A00((char *)label, (*(unsigned int *)(D_00138180 + 0x1a0) & 0x2000) ? D_L11_001B1648 : color,
                (float)D_L11_001B1630, (float)(int)((float)D_L11_001B1634 * D_L11_001B3E04 + 0.5f), D_L11_001B161C, 1.0f, 0.0f);
            break;
        }
    }
    func_002FC560(0x47, 0x5360b);
}
