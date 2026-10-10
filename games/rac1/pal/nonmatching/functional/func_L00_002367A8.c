/* func_L00_002367A8 -- src/overlays/shared/hud_00235960.c (functional C for the port, not a match)
 * HUD panel hold: finds the level's HUD bank record (13 records of 0x90 at D_L00_0017DD50) whose
 * id (+0x64) is `id`; if there is one and it is not locked (+0x68 == 0), raises its show timer
 * (+0x7C) to at least `ticks`. The pause and page menus call it with scale_ticks(0xB4) to keep a
 * panel on screen. The level copy of the search in func_001FFDA0 (src/game/hud.c).
 * Retail also leaves 0 in $v0 on the found path; every caller declares it void.
 * From the staged attempts in build-sn/try/func_L00_002367A8 (SIZE 136/128: only the shared
 * epilogue differs).
 * equiv: NEAR, only in the return value: retail clears $v0 after using it for the record address.
 */
typedef struct {
    unsigned char pad0[0x64];
    int id;
    int locked;
    unsigned char pad6C[0x7C - 0x6C];
    int timer;
    unsigned char pad80[0x90 - 0x80];
} HudBank_2367A8;

extern HudBank_2367A8 D_L00_0017DD50_2367A8[] __asm__("D_L00_0017DD50");

void func_L00_002367A8(int id, int ticks) {
    int i;

    for (i = 0; i < 13; i++) {
        if (D_L00_0017DD50_2367A8[i].id == id) {
            break;
        }
    }
    if (i < 13 && D_L00_0017DD50_2367A8[i].locked == 0) {
        HudBank_2367A8 *b = &D_L00_0017DD50_2367A8[i];
        int cur = b->timer;
        b->timer = ticks < cur ? cur : ticks;
    }
}
