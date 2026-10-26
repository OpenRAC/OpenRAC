int draw_menu_icon_list(char *arg0) {
    int x = (*(int *)(arg0 + 0x20) * 16 - 0x200) >> 1;
    int y = *(int *)(arg0 + 0x5C);
    int top = y;
    int i;

    SetupGifPaging(0);
    for (i = 0; i < *(int *)(arg0 + 0x40); i++) {
        char *e = *(char **)(arg0 + 0x48) + i * 10;

        if (*(int *)(arg0 + 0x3C) == i) {
            draw_hud_rect_blended(x - 0x30, y - 0x30, x + 0x230, y + 0x230,
                          (unsigned int)((subtract_integer_with_clamp((game_frame_counter & 0x3F) - 0x20) + 0x40)
                                         * 0x10202 | 0x80000000), 1);
            draw_hud_rect_blended(x - 0x10, y - 0x10, x + 0x210, y + 0x210,
                          *(int *)&panel_clear_color, 1);
        }
        draw_hud_sprite_subpixel(GetIconFrame(*(unsigned short *)e, *(short *)(e + 2)), x, y,
                      0x200, 0x200, 0x80);
        y += 0x260;
    }
    if (top < 0) {
        draw_hud_rect_blended(0, 0, *(int *)(arg0 + 0x20), 0x14, *(int *)&panel_clear_color, 0);
        HudSprite(GetIconFrame(0xE99E, 6), x >> 4, 2, 0x20, 0x10, 0x80);
    }
    if (*(int *)(arg0 + 0x24) * 16 < y) {
        draw_hud_rect_blended(0, *(int *)(arg0 + 0x24) - 0x14, *(int *)(arg0 + 0x20),
                      *(int *)(arg0 + 0x24), *(int *)&panel_clear_color, 0);
        draw_hud_sprite_flipped(GetIconFrame(0xE99E, 6), x >> 4, *(int *)(arg0 + 0x24) - 0x12,
                      0x20, 0x10, 0x80);
    }
    DoGifPaging();
    return 2;
}
