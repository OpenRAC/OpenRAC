/* OpenRAC's addition (OPENRAC_PATCHES.md): the confirm prompt's input, the update of the memory
 * card pages' Yes/No prompt (overwrite a save, ...). The splat config has its first 0x38 bytes as a
 * text gap (text_gap_00221ab8_after_fun_00221a88) and the rest as FUN_00221af0; the retail function
 * is the 148 bytes from 0x221AB8 (rac1-decomp's Menu_ConfirmPromptInput, func_00222B00, the same
 * instructions: games/rac1/config/overlays/us_map.tsv in OpenRAC). Outside modes 1 and 0x10 it goes
 * back; otherwise it runs on into FUN_00221af0's answer. Not built by the decomp; the port reads
 * it. */
#include "types.h"
#include "rnc/globals.h"
#include "rnc/ui/menus/menu_system.h"

s32 update_menu_confirm_page(void) __asm__("FUN_00221af0");
s32 confirm_prompt_input(void) __asm__("FUN_00221ab8");

s32 confirm_prompt_input(void) {
    if (mode_freeze_state != 0x10 && mode_freeze_state != 1) {
        menu_system.next = menu_system.current->back;
        return 0;
    }
    return update_menu_confirm_page();
}
