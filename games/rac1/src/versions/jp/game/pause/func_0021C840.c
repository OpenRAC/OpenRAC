s32 update_menu_grid_selection(MenuScreen *grid) {
    s32 column_count;
    s32 row_count;
    s32 cursor;
    s32 row;
    s32 column_index;
    s32 column;
    s32 hidden;
    s32 next_rows;
    MenuPage *back_page;
    s32 total;
    s32 next_column_count;
    s32 next_columns;
    MenuScreen *next;
    MenuGridCell *entry;
    s32 slot;

    if (menu_system_C840.current->focus != grid) {
        return 0;
    }
    if ((controller_state_C840.pressed_unmasked & 0xD00) && menu_system_C840.close_locked == 0) {
        return 1;
    }
    if (controller_state_C840.pressed_unmasked & 0x10) {
        back_page = menu_system_C840.current->back;
        if (back_page != 0) {
            menu_system_C840.next = back_page;
            return 0;
        } else if (menu_system_C840.close_locked == 0) {
            return -1;
        }
    }

    column_count = grid->data.grid.cols;
    cursor = grid->data.grid.selected_cell;
    row_count = grid->data.grid.rows;
    row = cursor / column_count;
    column_index = cursor % column_count;
    column = column_index;

    /* Process every pressed direction in order; a page change can also adjust
       column before the following direction checks. */
    if (controller_state_C840.pressed_unmasked & 0x1000) {
        if (row != 0) {
            grid->data.grid.selected_cell -= column_count;
        } else if (grid->data.grid.up != 0) {
            next = grid;
            do {
                next = next->data.grid.up;
                next_rows = next->data.grid.rows;
                next_columns = next->data.grid.cols;
                hidden = 0;
                if (menu_system_C840.unk134 != 0 && (next->data.grid.flags & 8)) {
                    hidden = 1;
                }
                if (menu_system_C840.unk138 != 0 && (next->data.grid.flags & 4)) {
                    hidden = 1;
                }
            } while (hidden);
            menu_system_C840.current->pending_focus = next;
            if (next_columns == 5 && grid->data.grid.cols == 3) {
                column++;
            }
            if (next_columns == 3 && grid->data.grid.cols == 5) {
                column--;
                if (column > 2) {
                    column = 2;
                } else if (column < 0) {
                    column = 0;
                }
            }
            next->data.grid.selected_cell = (next_rows - 1) * next_columns +
                                            (column < next_columns - 1 ? column : next_columns - 1);
        } else if (!(grid->data.grid.flags & 0x8000)) {
            grid->data.grid.selected_cell = column_count * (row_count - 1) + cursor;
        }
    }

    if (controller_state_C840.pressed_unmasked & 0x4000) {
        if (row + 1 < row_count) {
            grid->data.grid.selected_cell = grid->data.grid.selected_cell + column_count;
        } else if (grid->data.grid.down != 0) {
            next = grid;
            do {
                next = next->data.grid.down;
                next_column_count = next->data.grid.cols;
                hidden = 0;
                if (menu_system_C840.unk134 != 0 && (next->data.grid.flags & 8)) {
                    hidden = 1;
                }
                if (menu_system_C840.unk138 != 0 && (next->data.grid.flags & 4)) {
                    hidden = 1;
                }
            } while (hidden);
            menu_system_C840.current->pending_focus = next;
            if (next_column_count == 5 && grid->data.grid.cols == 3) {
                column++;
            }
            if (next_column_count == 3 && grid->data.grid.cols == 5) {
                column--;
                if (column > 2) {
                    column = 2;
                } else if (column < 0) {
                    column = 0;
                }
            }
            next->data.grid.selected_cell =
                column < next_column_count - 1 ? column : next_column_count - 1;
        } else if (!(grid->data.grid.flags & 0x8000)) {
            grid->data.grid.selected_cell =
                grid->data.grid.selected_cell - grid->data.grid.cols * (grid->data.grid.rows - 1);
        }
    }

    if (controller_state_C840.pressed_unmasked & 0x8000) {
        if (column != 0) {
            grid->data.grid.selected_cell = grid->data.grid.selected_cell - 1;
        } else if (grid->data.grid.left != 0) {
            menu_system_C840.current->pending_focus = grid->data.grid.left;
        } else if (!(grid->data.grid.flags & 0x8000)) {
            grid->data.grid.selected_cell += grid->data.grid.cols - 1;
        }
    }

    if (controller_state_C840.pressed_unmasked & 0x2000) {
        if (column + 1 < column_count) {
            grid->data.grid.selected_cell = grid->data.grid.selected_cell + 1;
        } else if (grid->data.grid.right != 0) {
            menu_system_C840.current->pending_focus = grid->data.grid.right;
        } else if (!(grid->data.grid.flags & 0x8000)) {
            grid->data.grid.selected_cell -= grid->data.grid.cols - 1;
        }
    }

    if (grid->data.grid.selected_cell != cursor || menu_system_C840.current->pending_focus != 0) {
        play_menu_sound_C840(1, 0x11, grid->moby);
    }

    if ((controller_state_C840.pressed_unmasked & 0x60) && ((grid->data.grid.flags ^ 1) & 1)) {
        if (menu_system_C840.select_cooldown != 0) {
            return 0;
        }
        entry = &grid->data.grid.cells[grid->data.grid.selected_cell];
        if (entry->id != 0 && item_available_C840[entry->id] != 0) {
            slot = menu_item_info_C840[entry->id].slot;
            play_menu_sound_C840(0, 0x11, grid->moby);
            menu_system_C840.select_cooldown = 10;
            if (menu_system_C840.equipped[slot] == entry->id && slot != 0 && slot != 3) {
                menu_system_C840.equipped[slot] = 0;
            } else if (entry->id == 0x18) {
                if (item_count_C840(0x18) != 0 && menu_system_C840.state == 3) {
                    next_columns = item_count_C840(0x18);
                    hero_C840.unk1FF5 = 1;
                    total = hero_C840.ammo_capacity + next_columns;
                    if (total > 6) {
                        total = 6;
                    }
                    hero_C840.ammo_capacity = total;
                }
            } else {
                menu_system_C840.equipped[slot] = entry->id;
            }
        } else {
            play_menu_sound_C840(2, 0x11, grid->moby);
        }
    }
    return 0;
}
