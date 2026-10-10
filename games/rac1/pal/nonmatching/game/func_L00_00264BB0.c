/* Stores f at obj+0x70 when the flag byte at D_0015EEB4+3 is nonzero; otherwise does nothing. */
extern unsigned char D_0015EEB4_m[4] __asm__("D_0015EEB4") MACRO_ADDR;
void func_L00_00264BB0(char *obj, float f) {
    if (D_0015EEB4_m[3] == 0) {
        return;
    }
    *(float *)(obj + 0x70) = f;
}
