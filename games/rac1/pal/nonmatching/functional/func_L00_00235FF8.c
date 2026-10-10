/* func_L00_00235FF8 -- src/overlays/shared/hud_00235960.c (functional C for the port, not a match)
 * HUD bank set switch: when `set` differs from the current set flag D_L00_0015F808, toggles the flag
 * and swaps four words between the two sets: D_L00_0015FB28..0015FB34 against D_L00_00160098,
 * 0016009C, 001600A0 and 001600A8 (the third word of the second set at 0x1600A4 is not touched).
 * The pause menu calls it with 0 on its way out. Retail reads 0x160098 through $gp
 * (-0x6C68) and writes it through a lui pair: one word, so one symbol here.
 * From the staged near miss (nonmatching/shared/func_L00_00235FF8.c, SIZE 144/148), made
 * self-contained. 0x15F808 is above the resident image, so it is a level symbol (D_L00_).
 * equiv: FUNCTIONAL.
 */
extern int D_L00_0015F808_235FF8 __asm__("D_L00_0015F808") MACRO_ADDR;
extern int D_L00_0015FB28_235FF8 __asm__("D_L00_0015FB28");
extern int D_L00_0015FB2C_235FF8 __asm__("D_L00_0015FB2C");
extern int D_L00_0015FB30_235FF8 __asm__("D_L00_0015FB30");
extern int D_L00_0015FB34_235FF8 __asm__("D_L00_0015FB34");
extern int D_L00_00160098_235FF8 __asm__("D_L00_00160098");
extern int D_L00_0016009C_235FF8 __asm__("D_L00_0016009C");
extern int D_L00_001600A0_235FF8 __asm__("D_L00_001600A0");
extern int D_L00_001600A8_235FF8 __asm__("D_L00_001600A8");

void func_L00_00235FF8(int set) {
    int t;

    if (set == D_L00_0015F808_235FF8) {
        return;
    }
    D_L00_0015F808_235FF8 ^= 1;
    t = D_L00_0015FB28_235FF8;
    D_L00_0015FB28_235FF8 = D_L00_00160098_235FF8;
    D_L00_00160098_235FF8 = t;
    t = D_L00_0015FB2C_235FF8;
    D_L00_0015FB2C_235FF8 = D_L00_0016009C_235FF8;
    D_L00_0016009C_235FF8 = t;
    t = D_L00_0015FB30_235FF8;
    D_L00_0015FB30_235FF8 = D_L00_001600A0_235FF8;
    D_L00_001600A0_235FF8 = t;
    t = D_L00_0015FB34_235FF8;
    D_L00_0015FB34_235FF8 = D_L00_001600A8_235FF8;
    D_L00_001600A8_235FF8 = t;
}
