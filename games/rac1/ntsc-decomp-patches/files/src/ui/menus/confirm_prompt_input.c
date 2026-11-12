/* OpenRAC's addition (games/rac1/ntsc-decomp-patches): the confirm prompt's input, the update of the memory
 * card pages' Yes/No prompt (overwrite a save, ...). The splat config has its first 0x38 bytes as a
 * text gap (text_gap_00221ab8_after_fun_00221a88) and the rest as FUN_00221af0; the retail function
 * is the 148 bytes from 0x221AB8 (rac1-decomp's Menu_ConfirmPromptInput, func_00222B00, the same
 * instructions: games/rac1/config/overlays/us_map.tsv in OpenRAC). Unless the card is good or new
 * (save_card_state 1 or 0x10) it goes back; otherwise it runs on into FUN_00221af0's answer. Not
 * built by the decomp; the port reads it. */
#include "types.h"
#include "rnc/globals.h"
#include "rnc/ui/menus/menu_system.h"
#include "rnc/ui/menus/save_data/save_card_state.h"

s32 update_menu_confirm_page(void) __asm__("FUN_00221af0");
s32 confirm_prompt_input(void) __asm__("FUN_00221ab8");

s32 confirm_prompt_input(void) {
    /* The card's state (D_0015EEB0), not mode_freeze_flags beside it: with that the answer was
     * never read and overwriting a save went back to the slot list. */
    if (save_card_state != CS_NEWCARD && save_card_state != CS_GOOD_SAVE) {
        menu_system.next = menu_system.current->back;
        return 0;
    }
    return update_menu_confirm_page();
}
