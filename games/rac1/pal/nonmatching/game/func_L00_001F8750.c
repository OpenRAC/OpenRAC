/* Clears the loader's state words (sixteen 4-byte globals; three of them are reached through $gp in retail, written here at their absolute addresses 0x15F40C, 0x15F420, 0x15F430). */
extern int D_L00_0015F524 MACRO_ADDR;
extern int D_L00_0015F52C MACRO_ADDR;
extern int D_L00_0015F530 MACRO_ADDR;
extern int D_L00_0015F528 MACRO_ADDR;
extern int D_L00_0015F534 MACRO_ADDR;
extern int D_L00_0015F3F0 MACRO_ADDR;
extern int D_L00_0015F3F4 MACRO_ADDR;
extern int D_L00_0015F504 MACRO_ADDR;
extern int D_L00_0015F508 MACRO_ADDR;
extern int D_L00_0015F6E8 MACRO_ADDR;
extern int D_L00_00161390 MACRO_ADDR;
extern int D_L00_00161394 MACRO_ADDR;
extern int D_L00_00161398 MACRO_ADDR;

void func_L00_001F8750(void) {
    D_L00_0015F524 = 0;
    D_L00_0015F52C = 0;
    D_L00_0015F530 = 0;
    D_L00_0015F528 = 0;
    D_L00_0015F534 = 0;
    D_L00_0015F3F0 = 0;
    D_L00_0015F3F4 = 0;
    D_L00_0015F504 = 0;
    D_L00_0015F508 = 0;
    *(int *)0x0015F40C = 0;
    *(int *)0x0015F420 = 0;
    *(int *)0x0015F430 = 0;
    D_L00_0015F6E8 = 0;
    D_L00_00161390 = 0;
    D_L00_00161394 = 0;
    D_L00_00161398 = 0;
}
