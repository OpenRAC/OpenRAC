int draw_selected_item_ammo(char *arg0) {
    char text[0x50];
    short box[10];
    char *p = *(char **)(current_menu_page + 0x40);
    int id = *(short *)(*(int *)(p + 0x3C) * 10 + *(char **)(p + 0x48) + 6);
    int have, total;
    char *rec;

    if (((unsigned char *)&item_available)[id] == 0) {
        return 0;
    }
    have = weapon_ammo_counts[id];
    rec = vendor_item_prices + id * 0x18;
    total = *(unsigned short *)(rec + 0xE);
    VU1_addGSregister_l(0x42, 0x44);
    VU1_addGSregister_l(0x47, 0xB);
    if (*(unsigned short *)(rec + 8) == 0) {
        sprintf(text, msg_string_id(0x4F53));
    } else {
        char *q;
        if (have < 1000) {
            q = text + sprintf(text, fmt_ammo_have, have);
        } else {
            q = text + sprintf(text, fmt_ammo_have_thousands, have / 1000, have % 1000);
        }
        if (total < 1000) {
            sprintf(q, fmt_decimal, total);
        } else {
            sprintf(q, fmt_number_thousands, total / 1000, total % 1000);
        }
    }
    SetupGifPaging(0);
    pack_image_descriptor(box, arg0);
    box[8] = 0x10;
    box[9] = 3;
    font_print_window_regular(box, 0x80FFA888L, text, -1);
    DoGifPaging();
    return 2;
}
