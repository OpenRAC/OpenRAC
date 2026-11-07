int MissionMenu_DrawObjectives(char *arg) {
    TextBox_p t;
    TextBox_p s;
    int mask;
    int n;
    int i;
    int y;
    int *list;
    int *ids;

    y = 0x18;
    i = 0;
    mask = 0;
    SetupGifPaging(0);
    n = collect_mission_ids((void *)0x70000000, &mask, (void *)0, 0);
    VU1_addGSregister(0x42, 0x44);
    VU1_addGSregister(0x47, 0x10B);
    FP68E8(4, 4, 0x80FFA888L, MSGS(0x4F59), -1);
    ((void *(*)(void *, int, int))libc_memset)(s.f, 0, 0x18);
    s.f[1] = *(unsigned short *)(arg + 0x24);
    s.f[3] = *(unsigned short *)(arg + 0x20);
    s.f[8] = 0x10;
    t = s;
    if (i < n) {
        list = (int *)(arg + 0x30);
        ids = (int *)0x70000000;
        do {
            int sel = *(int *)((char *)list + (level_map_selection[0x89] << 2)) == i;
            int color = 0x80FFA888;
            int h;
            int alt = y + 0xA;
            if (sel)
                color = 0x8020FFFF;
            if (sel)
                disable_font_color_codes();
            t.f[5] = y;
            t.f[4] = 0x10;
            font_print_window_small(t.f, color, MSGS(*ids), -1);
            if (sel)
                enable_font_color_codes();
            h = (short)t.f[7];
            h = h / (short)t.f[8];
            ids++;
            draw_menu_selection_marker(9, (h == 2) ? y + 0x10 : alt, (mask >> i) & 1);
            y += h * 16;
            i++;
        } while (i < n);
    }
    if (mask < 0) {
        t.f[9] = 1;
        t.f[5] = y + 8;
        t.f[4] = *(int *)(arg + 0x24) >> 1;
        font_print_window_small(t.f, 0x8020FFFFL, MSGS(0x523D), -1);
    }
    DoGifPaging();
    return 2;
}
