/* OpenRAC's addition (games/rac1/ntsc-decomp-patches): three menu routines of the executable that the
 * splat config has only as text gaps (text_gap_0021d168_before_fun_0021d1f8,
 * text_gap_0021e7c8_before_fun_0021e890, text_gap_00221800_after_fun_002217f8), so they had no C. The
 * executable's menu pages name them: the help history list, the Quit game page and the ship's planet
 * list. Written from the retail instructions. rac1-decomp has the same routines as matched C
 * (build_reversed_active_message_list func_0021E170, Menu_BackInput func_0021F7D0,
 * initialize_selection_entries_and_index func_00222848), and this decomp has the levels' copies
 * (FUN_L00_0027adb0, pause_menu_entry_input FUN_L00_0027c410, FUN_L00_0027f448): the same code with the
 * executable's addresses. Not built by the decomp; the port reads it. */
#include "types.h"
#include "rnc/data_labels.h"
#include "rnc/game/game_globals.h"
#include "rnc/input/pad_state.h"
#include "rnc/ui/help/message_state.h"
#include "rnc/ui/map/level_map.h"
#include "rnc/ui/map/map_state.h"
#include "rnc/ui/menus/menu_globals.h"
#include "rnc/ui/menus/menu_pages.h"
#include "rnc/ui/menus/menu_screen.h"
#include "rnc/ui/menus/menu_system.h"

/* Each help message's text id, at +2 of a 4-byte row. */
extern u8 help_message_texts[] __asm__("D_00199710");
extern s32 current_level_index_gp __asm__("D_0015ED84");
extern s32 selected_level_index_lui __asm__("D_001A0314");
extern s32 space_load_level_lui __asm__("D_0015F600");
extern s32 galaxy_list[] __asm__("D_0013D510");
extern struct MenuTextItem planet_select_items[] __asm__("D_001D62B8");

void build_help_history_list(struct MenuScreen *screen, s32 entering) __asm__("FUN_0021d168");
s32  quit_game_page_input(struct MenuScreen *screen) __asm__("FUN_0021e7c8");
void build_planet_select_list(struct MenuScreen *screen, s32 entering) __asm__("FUN_00221800");

/* On opening: one row per help message seen so far, newest first, then a 0 text. */
void build_help_history_list(struct MenuScreen *screen, s32 entering) {
    s32 i, j;
    for (i = 0; i < active_message_record_count; i = j) {
        j = i + 1;
        help_history_menu_items[i].action = MENU_ACTION_NO_OP;
        help_history_menu_items[i].text = *(u16 *)(help_message_texts + 2 +
            active_message_records[active_message_record_count - j] * 4);
    }
    help_history_menu_items[active_message_record_count].text = 0;
}

/* Each tick: Start, Select or R3 leave the menu (-1); Triangle goes to the page's back page, or
 * leaves when it has none and the menu may close; Circle quits to the title. Unlike rac1-decomp's
 * (PAL), retail here still reads Circle in the tick Triangle set the back page. */
s32 quit_game_page_input(struct MenuScreen *screen) {
    s32 pressed = controller_state.pressed_unmasked;
    struct MenuPage *back;
    if (pressed & 0xD00) {
        if (screen->data.list.flags & 0x20) {
            selected_level_index_lui = current_level_index_gp;
        }
        return -1;
    }
    if (pressed & 0x10) {
        if (screen->data.list.flags & 0x20) {
            selected_level_index_lui = current_level_index_gp;
        }
        back = menu_system.current->back;
        if (back != 0) {
            menu_system.next = back;
        } else if (menu_system.close_locked == 0) {
            return -1;
        }
    }
    if (((volatile struct PadState *)&controller_state)->pressed_unmasked & 0x20) {
        D_0013D384 = 0;
        space_load_level_lui = -1;
        space_load_in_progress = 1;
        space_load_pending = 1;
    }
    return 0;
}

/* On opening: a row for each planet the ship can go to (up to 20, the galaxy list), then the
 * current planet's row selected. */
void build_planet_select_list(struct MenuScreen *screen, s32 entering) {
    s32 i;
    for (i = 0; i < 20 && galaxy_list[i] != 0; i++) {
        planet_select_items[i].action = MENU_ACTION_NO_OP;
        planet_select_items[i].param.value = 0;
        planet_select_items[i].text = *(u16 *)&level_select_texts[galaxy_list[i]].description_text;
        planet_select_items[i].subtext = *(u16 *)&level_select_texts[galaxy_list[i]].label_text;
    }
    planet_select_items[i].text = 0;
    screen->data.list.selected = 0;
    screen->data.list.flags |= 0x8000;
    for (i = 0; menu_level_indices[i] != 0; i++) {
        if (level_map_selection.level == menu_level_indices[i]) {
            screen->data.list.selected = i;
            break;
        }
    }
}
