/* Arms the shared transition once: stores the two reciprocal steps. Retail is built with -g
   (the nops before each div.s). */
typedef unsigned char u8;
typedef signed int s32;
typedef float f32;

typedef struct {
    u8 pad[0x274];
    f32 f274;
    f32 f278;
    f32 f27C;
    s32 i280;
} T_001B99C0;

extern T_001B99C0 D_L16_001B99C0;

void func_L16_002D2D10(s32 a0, f32 f12, f32 f13) {
    if (D_L16_001B99C0.f27C == 0.0f) {
        D_L16_001B99C0.i280 = a0;
        D_L16_001B99C0.f27C = 1.0f;
        D_L16_001B99C0.f274 = 1.0f / f12;
        D_L16_001B99C0.f278 = 1.0f / f13;
    }
}
