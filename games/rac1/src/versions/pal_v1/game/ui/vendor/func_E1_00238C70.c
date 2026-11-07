void func_E1_00238C70(void) {
    Vendor *v;
    int idx;
    int price;

    draw_framebuffer_rect(0, 0, 0x200, 0x80, 0x200, 0x80, 0);
    v = &vendor_state_v;
    if (v->sel != 0) {
        idx = v->idx;
        if (v->slots[idx].kind == 1 && weapon_ammo_counts[v->slots[idx].type] >=
                                           ((WeaponInfo *)(vendor_item_prices + v->slots[idx].type * 0x18))->max) {
            return;
        }
        if (v->slots[idx].kind == 1) {
            if (v->alt) {
                price = ((WeaponInfo *)(vendor_item_prices + v->slots[idx].type * 0x18))->priceAlt;
            } else {
                price = ((WeaponInfo *)(vendor_item_prices + v->slots[idx].type * 0x18))->price;
            }
            if (gBolts >= price) {
                FontPrintCenterSmall(0x28, 0x14, 0x80F0F0F0L, msg_string(0x4EE0), -1);
            }
        } else {
            if (gHavePersuader[0] != 0) {
                price = ((struct { WeaponInfoA a[1]; } *)vendor_item_prices)->a[v->slots[idx].type].second;
            } else {
                price = ((WeaponInfoA *)(vendor_item_prices + v->slots[idx].type * 0x18))->first;
            }
            if (gBolts >= price) {
                FontPrintCenterSmall(0x28, 0x14, 0x80F0F0F0L, msg_string(0x524B), -1);
            }
        }
    } else {
        FontPrintCenterSmall(0x28, 0x14, 0x80F0F0F0L, msg_string(0x4EE0), -1);
    }
}
