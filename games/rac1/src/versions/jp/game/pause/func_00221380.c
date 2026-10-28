int draw_checking_memory_card_data_menu(struct Stream_21380 *menu) {
    FontWindow_21380 text_window;
    short window_fields[12];
    struct Cell_21380 *entry;
    char *text;
    int state = menu->state;
    long texture;
    extern void font_print_window_small(FontWindow_21380 *, long, char *, int);
    int first;
    unsigned char *tp;

    if (state < 2) {
        if (!(menu->flags & 0x100)) {
            return 1;
        }
        if (((MemoryCardState *)memory_card_state)->card[0].type != 2) {
            goto done;
        }
        if (((MemoryCardState *)memory_card_state)->state < 3 && ((MemoryCardState *)memory_card_state)->err < 0 && ((struct MS_21380 *)menu_system)->ticks >= 0xB) {
            goto done;
        }
        SetupGifPaging(0);
        text = ((char *(*)(int))msg_string)(((struct MS_21380 *)menu_system)->card_op_pending ? ((struct MS_21380 *)menu_system)->card_op_text : 0x4FBA);
        ((void *(*)(void *, int, unsigned int))libc_memset)(window_fields, 0, sizeof(window_fields));
        window_fields[1] = menu->height + 1;
        window_fields[0] = 1;
        window_fields[2] = 1;
        window_fields[3] = menu->width + 1;
        window_fields[4] = menu->width >> 1;
        window_fields[5] = 5;
        window_fields[8] = 16;
        window_fields[9] = 5;
        text_window = *(FontWindow_21380 *)window_fields;
        font_print_window_small(&text_window, 0x80000000L, text, -1);
        text_window.anchor_y = (menu->height - text_window.rendered_height) >> 1;
        text_window.flags ^= 4;
        font_print_window_small(&text_window, 0x80000000L, text, -1);
        text_window.top--;
        text_window.bottom--;
        text_window.left--;
        text_window.right--;
        text_window.anchor_x--;
        text_window.anchor_y--;
        font_print_window_small(&text_window, 0x80FFA888L, text, -1);
        DoGifPaging();
        goto done;
    }
    if (menu->flags & 4) {
        first = state < 4;
        entry = &((struct MS_21380 *)menu_system)->current->focus->cells[menu->loaded_entry[first ^ 1]];
        if (entry->kind == 0 && (tp = (unsigned char *)&item_available, tp[entry->id] == 0)) {
            return 1;
        }
        if (entry->kind == 1 && gSpecialItems[entry->id] == 0) {
            return 1;
        }
    }
    SetupGifPaging(0);
    if (menu->state < 4) {
        texture = load_texture_file(menu->buffer[0]);
    } else {
        texture = load_texture_file(menu->buffer[1]);
    }
    DrawTexturedQuad(0, 0, fs_aa_buffer[0xB0], fs_aa_buffer[0xB1], 0, 0, menu->tw, menu->th, 0x80808080L, texture);
    DoGifPaging();
    return 0x10;
done:
    return 2;
}
