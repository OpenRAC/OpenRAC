/* Carried by port/tools/versionmap/carry_functions.py from the decompilation of Ratchet & Clank (PAL), src/overlays/shared/pause_menu.c (games/rac1): func_L00_00280708, matched C there.
 * Kind `same` in the version map (../../README.md): the two versions have the same instructions for it.
 * Names and addresses are this version's. Functional C, not matched C. */

typedef short s16_c00_27f700;
typedef int s32_c00_27f700;
typedef unsigned char u8_c00_27f700;
typedef unsigned int u32_c00_27f700;
typedef float f32_c00_27f700;
struct PadState_c00_27f700 {
    u8_c00_27f700 pad_0[0x100] __attribute__((aligned(16)));
    f32_c00_27f700 analog[16];
    f32_c00_27f700 analog_prev[16];
    u8_c00_27f700 profile[4];
    u8_c00_27f700 act[4];
    u8_c00_27f700 unk188;
    u8_c00_27f700 unk189;
    u8_c00_27f700 pad_18A[0x2];
    s16_c00_27f700 unk18C;
    s16_c00_27f700 hist_idx;
    s32_c00_27f700 hist_count;
    s32_c00_27f700 socket;
    s32_c00_27f700 profile_state;
    s32_c00_27f700 device_state;
    s32_c00_27f700 held;
    s32_c00_27f700 pressed;
    s32_c00_27f700 released;
    s32_c00_27f700 prev_held;
    s32_c00_27f700 raw_held;
    s32_c00_27f700 raw_pressed;
    s32_c00_27f700 raw_released;
    s32_c00_27f700 prev_raw_held;
    u32_c00_27f700 held_unmasked;
    u32_c00_27f700 pressed_unmasked;
    u32_c00_27f700 released_unmasked;
    s32_c00_27f700 mode;
    s32_c00_27f700 no_buttons;
    s32_c00_27f700 no_direction;
    s32_c00_27f700 stick_moved;
    s32_c00_27f700 unk1DC;
    s32_c00_27f700 hist_btn[30];
    f32_c00_27f700 hist_ang[30];
    f32_c00_27f700 hist_mag[30];
    s32_c00_27f700 unk348;
    u8_c00_27f700 pad_34C;
    u8_c00_27f700 rep_delay;
    u8_c00_27f700 rep_rate;
    u8_c00_27f700 rep_timer;
};
typedef struct PadState_c00_27f700 PadState_c00_27f700;
struct MenuPage_280708_c00_27f700 {
    unsigned char pad0[0x38];
    struct MenuPage_280708_c00_27f700 *back;
    unsigned char pad3C[0x84 - 0x3C];
    int unk84;
};
struct MenuSystem_280708_c00_27f700 {
    unsigned char pad0[4];
    struct MenuPage_280708_c00_27f700 *current;
    struct MenuPage_280708_c00_27f700 *next;
    unsigned char pad0C[0xD4 - 0xC];
    int unkD4;
};
extern struct MenuSystem_280708_c00_27f700 level_menu_system_r2_c00_27f700 __asm__("D_L00_001B9CF0");
extern struct PadState_c00_27f700 g_controller_state_r2_c00_27f700 __asm__("D_0013C940");
extern int mode_freeze_state_r_c00_27f700 __asm__("D_0015EEB0") ;
int FUN_L00_0027f700(void) {
    int state = mode_freeze_state_r_c00_27f700;
    unsigned int pressed;
    if (state != 0x10 && state != 1) {
        level_menu_system_r2_c00_27f700.next = level_menu_system_r2_c00_27f700.current->back;
        return 0;
    }
    pressed = (unsigned int)g_controller_state_r2_c00_27f700.pressed;
    if (pressed & 0x20) {
        level_menu_system_r2_c00_27f700.unkD4 = 0;
        level_menu_system_r2_c00_27f700.next = level_menu_system_r2_c00_27f700.current->back;
        level_menu_system_r2_c00_27f700.current->unk84 = 1;
    } else if (pressed & 0x10) {
        level_menu_system_r2_c00_27f700.next = level_menu_system_r2_c00_27f700.current->back;
        level_menu_system_r2_c00_27f700.current->unk84 = 0;
    }
    return 0;
}
