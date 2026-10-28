int draw_menu_selection_text(char *arg0) {
    char *g = menu_system;
    MemoryCardState *b = (MemoryCardState *)memory_card_state;
    int row = b->card[0].slots[*(int *)(*(char **)(*(char **)(g + 4) + 0x40) + 0x40)].id;

    SetupGifPaging(0);
    if (b->state < 3 && b->err < 0
        && *(int *)(g + 0x154) >= 0xB && b->card[0].type == 2) {
        if (row == -1) {
            FontPrintCenter(*(int *)(arg0 + 0x20) / 2, *(int *)(arg0 + 0x24) / 2 - 8,
                          0x80FFA888L, msg_string_id(0x526D), -1);
        } else {
            FontPrintCenter(*(int *)(arg0 + 0x20) / 2, 4, 0x80FFA888L,
                          msg_string_id(planet_icon_rows_i[row].a), -1);
            FontPrintCenter(*(int *)(arg0 + 0x20) / 2, 0x14, 0x80FFA888L,
                          msg_string_id(planet_icon_rows_i[row].b), -1);
        }
    }
    DoGifPaging();
    return 2;
}
