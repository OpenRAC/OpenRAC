int draw_menu_selection_text(MenuScreen *arg0) {
    MemoryCardState *b = (MemoryCardState *)memory_card_state;
    int row =
        b->card[0].slots[*(int *)(*(char **)(*(char **)&((struct MenuSystem *)menu_system)->current + 0x40) + 0x40)].id;

    SetupGifPaging(0);
    if (b->busy < 3 && b->state < 0 && b->card[0].type == 2) {
        if (row == -1) {
            FontPrintCenter(arg0->width / 2, arg0->height / 2 - 8, 0x80FFA888L, msg_string(0x5217), -1);
        } else {
            FontPrintCenter(arg0->width / 2, 4, 0x80FFA888L, msg_string(planet_icon_rows_i[row].a), -1);
            FontPrintCenter(arg0->width / 2, 0x14, 0x80FFA888L, msg_string(planet_icon_rows_i[row].b), -1);
        }
    }
    DoGifPaging();
    return 2;
}
