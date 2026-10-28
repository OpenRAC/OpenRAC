void render_vendor_item_details_pass(void) {
    char buf[0x100];
    int w;

    draw_framebuffer_rect(0, 0, 0x200, 0x80, 0x200, 0x80, 0);
    DrawMobyList(vendor_state_m.f20 + 0x100, 1);
    DrawMobyList(vendor_state_m.f20, 1);
    if (vendor_state_m.slots[vendor_state_m.idx].kind == 1) {
        FontPrintSmall_c(6, 8, 0x80F0F0F0L, msg_string_id(item_info_table_t[vendor_state_m.slots[vendor_state_m.idx].id].text), -1);
        FontPrintSmall_c(0x18, 0x18, 0x80F0F0F0L, msg_string_id(0x4F5E), -1);
        format_scaled_display_value(buf, vendor_item_prices_p[vendor_state_m.slots[vendor_state_m.idx].id].c);
        font_print_right_small_c(0x76, 0x65, 0x80F0F0F0L, buf, -1);
        if (vendor_state_m.f40 != 0) {
            w = measure_text_width_regular((unsigned char *)buf, -1);
            draw_hud_rect_blended(0x75 - w, 0x6D, 0x7B, 0x70, 0x20959544L, 0);
            draw_hud_rect_blended(0x76 - w, 0x6D, 0x7A, 0x70, 0x30959544L, 0);
            draw_hud_rect_blended(0x77 - w, 0x6D, 0x79, 0x70, 0x40959544L, 0);
            draw_hud_rect_blended(0x78 - w, 0x6D, 0x78, 0x70, 0x50959544L, 0);
            draw_hud_rect_blended(0x79 - w, 0x6D, 0x77, 0x70, 0x60959544L, 0);
            draw_hud_rect_blended(0x7A - w, 0x6D, 0x76, 0x70, 0x70959544L, 0);
            draw_hud_rect_blended(0x7B - w, 0x6D, 0x75, 0x70, 0x80959544L, 0);
            format_scaled_display_value(buf, vendor_state_m.f40 ? vendor_item_prices_p[vendor_state_m.slots[vendor_state_m.idx].id].d
                                                 : vendor_item_prices_p[vendor_state_m.slots[vendor_state_m.idx].id].c);
            font_print_right_small_c(0x76, 0x55, 0x80F0F0F0L, buf, -1);
        }
    } else {
        FontPrintSmall_c(6, 8, 0x80F0F0F0L, msg_string_id(item_info_table_t[vendor_state_m.slots[vendor_state_m.idx].id].text), -1);
        format_scaled_display_value(buf, vendor_item_prices_p[vendor_state_m.slots[vendor_state_m.idx].id].a);
        font_print_right_small_c(0x76, 0x65, item_available_b[0x23] ? 0x80808080L : 0x80F0F0F0L, buf, -1);
        if (item_available_b[0x23] != 0) {
            w = measure_text_width_regular((unsigned char *)buf, -1);
            draw_hud_rect_blended(0x75 - w, 0x6D, 0x7B, 0x70, 0x20959544L, 0);
            draw_hud_rect_blended(0x76 - w, 0x6D, 0x7A, 0x70, 0x30959544L, 0);
            draw_hud_rect_blended(0x77 - w, 0x6D, 0x79, 0x70, 0x40959544L, 0);
            draw_hud_rect_blended(0x78 - w, 0x6D, 0x78, 0x70, 0x50959544L, 0);
            draw_hud_rect_blended(0x79 - w, 0x6D, 0x77, 0x70, 0x60959544L, 0);
            draw_hud_rect_blended(0x7A - w, 0x6D, 0x76, 0x70, 0x70959544L, 0);
            draw_hud_rect_blended(0x7B - w, 0x6D, 0x75, 0x70, 0x80959544L, 0);
            format_scaled_display_value(buf, item_available_b[0x23] ? vendor_item_prices_p[vendor_state_m.slots[vendor_state_m.idx].id].b
                                                 : vendor_item_prices_p[vendor_state_m.slots[vendor_state_m.idx].id].a);
            font_print_right_small_c(0x76, 0x55, 0x80F0F0F0L, buf, -1);
        }
    }
}
