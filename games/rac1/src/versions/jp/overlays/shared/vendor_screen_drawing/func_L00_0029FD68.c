void draw_vendor_purchase_text(void *m, int x, int y) {
    TBox box;
    char text[256];
    int cost;
    int message_id;
    int text_x;
    unsigned short ammo_cost;
    char *fmt;
    int total;

    draw_framebuffer_rect(0, 0, 0x200, 0x80, 0x200, 0x80, 0);
    { int n = sizeof(box); libc_memset((void *)&box, 0, n); }
    box.half_x = x / 2;
    box.half_y = y / 2;
    box.left = x - 8;
    box.top = y - 8;
    box.width = 0x10;
    box.style = 3;
    box.y = (short)y;
    box.x = (short)x;

    if (MENU.rows[MENU.f58].fD4 == 1 && weapon_ammo_counts[MENU.rows[MENU.f58].fD0] >= item_price_table[MENU.rows[MENU.f58].fD0].hE) {
        message_id = 0x528D;
        goto message;
    }
    if (MENU.rows[MENU.f58].fD4 != 1)
        goto purchase;
    if (MENU.f40)
        ammo_cost = item_price_table[MENU.rows[MENU.f58].fD0].hA;
    else
        ammo_cost = item_price_table[MENU.rows[MENU.f58].fD0].h8;
    if (gBolts < ammo_cost)
        goto no_money;
    DrawMobyList(MENU.f20 + 0x500, 1);
    box.half_y = 6;
    box.style = 1;
    font_print_window_small((void *)&box, 0x80f0f0f0L, msg_string(0x528E), -1);
    sprintf(text, vendor_quantity_format, vendor_buy_quantity);
    if (vendor_buy_quantity >= 100)
        text_x = 0x17;
    else {
        if (vendor_buy_quantity < 10)
            goto quantity_small;
        text_x = 0x1e;
    }
    FontPrintSmall(text_x, 0x32, 0x80f0f0f0L, text, -1);
    goto quantity_done;
quantity_small:
    FontPrintSmall(0x25, 0x32, 0x80f0f0f0L, text, -1);
quantity_done:
    FontPrintSmall(0xc, 0x32, 0x80f0f0f0L, vendor_quantity_left_label, -1);
    FontPrintSmall(0x40, 0x32, 0x80f0f0f0L, vendor_quantity_right_label, -1);
    fmt = vendor_quantity_format;
    if (MENU.f40)
        total = vendor_buy_quantity * item_price_table[MENU.rows[MENU.f58].fD0].hA;
    else
        total = vendor_buy_quantity * item_price_table[MENU.rows[MENU.f58].fD0].h8;
    sprintf(text, fmt, total);
    font_print_right_small(0x40, 0x4c, 0x80f0f0f0L, text, -1);
    return;
no_money:
    message_id = 0x528F;
message:
    font_print_window_small((void *)&box, 0x80f0f0f0L, msg_string(message_id), -1);
    return;
purchase:
    cost = discount_purchase_pricing ? item_price_table[MENU.rows[MENU.f58].fD0].i4 : item_price_table[MENU.rows[MENU.f58].fD0].i0;
    if (gBolts >= cost) {
        FontPrintSmall(6, 6, 0x80f0f0f0L, msg_string(0x5290), -1);
        FontPrintSmall(2, 0x20, 0x80f0f0f0L, msg_string(0x528B), -1);
        FontPrintSmall(2, 0x3e, 0x80f0f0f0L, msg_string(0x52A2), -1);
    } else {
        font_print_window_small((void *)&box, 0x80f0f0f0L, msg_string(0x528F), -1);
    }
}
