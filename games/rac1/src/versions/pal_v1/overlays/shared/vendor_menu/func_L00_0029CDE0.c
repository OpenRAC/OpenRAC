extern int D_E1_L00_00161E28 MACRO_ADDR;
extern int D_E1_L00_00161E2C MACRO_ADDR;
extern int D_E1_L00_00161E30 SDATA(D_E1_L00_00161E30);
void update_vendor_purchase(void) {
    float v[4];
    int n;
    int room;
    int price;
    int cost;
    if (vendor_menu_r4.state == 0)
        return;
    if (vendor_menu_r4.state == 1) {
        int pr1;
        vendor_buy_quantity = 1;
        vendor_purchase_confirmed = 0;
        InitMobyInstance(vendor_menu_r4.obj + 0x400, 0x471);
        *(short *)(vendor_menu_r4.obj + 0x432) = 0x40;
        *(u128_9CDE0 *)v = 0;
        FastVecAdd(vendor_menu_r4.obj + 0x410, v, vendor_menu_r4.cam + 0x10);
        qcopy_nc(vendor_menu_r4.obj + 0x440, vendor_menu_r4.cam + 0x40);
        MobySetAnimation(vendor_menu_r4.obj + 0x400, 0, 0);
        if (*(u8 *)(*(char **)(*(char **)(vendor_menu_r4.obj + 0x424) + 0x48) + 0x10) < 2) {
            *(int *)(vendor_menu_r4.obj + 0x458) = 0;
            *(char *)(vendor_menu_r4.obj + 0x451) = 0;
            update_moby_animation_state(vendor_menu_r4.obj + 0x400);
        }
        moby_build_matrix(vendor_menu_r4.obj + 0x400);
        pack_render_command_fields(vendor_menu_r4.obj + 0x400, 0x101010, 0xE, 0xE, 0);
        vendor_quantity_adjustable = 0;
        if (vendor_menu_r4.e[vendor_menu_r4.idx].sel == 1 &&
            weapon_ammo_counts[vendor_menu_r4.e[vendor_menu_r4.idx].item] >=
                item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].hE) {
            Moby_PlaySound(2, 0, vendor_menu_r4.cam);
        } else if (vendor_menu_r4.e[vendor_menu_r4.idx].sel == 1) {
            pr1 = vendor_menu_r4.f40 ? item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].hA
                                     : item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].h8;
            if (gBolts < pr1) {
                Moby_PlaySound(2, 0, vendor_menu_r4.cam);
            } else {
                vendor_quantity_adjustable = 1;
            }
        } else {
            pr1 = gHavePersuader[0] ? item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].f4
                                    : item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].f0;
            if (gBolts < pr1) {
                Moby_PlaySound(2, 0, vendor_menu_r4.cam);
            }
        }
        vendor_menu_r4.state = 2;
    } else if (vendor_menu_r4.state == 2) {
        int n2;
        if (vendor_quantity_adjustable == 1) {
            if (D_0013CA40_9CDE0.held & 0x8000)
                D_E1_L00_00161E28++;
            else
                D_E1_L00_00161E28 = 0;
            if (D_0013CA40_9CDE0.held & 0x2000)
                D_E1_L00_00161E2C++;
            else
                D_E1_L00_00161E2C = 0;
            if ((D_0013CA40_9CDE0.pressed & 0x8000) || (D_E1_L00_00161E28 >= 16 && !(D_E1_L00_00161E28 & 7)) ||
                D_E1_L00_00161E28 >= 48) {
                if (vendor_buy_quantity >= 2) {
                    vendor_buy_quantity--;
                    if (!(D_E1_L00_00161E28 & 3) || D_E1_L00_00161E28 < 16) {
                        Moby_PlaySound(1, 0, vendor_menu_r4.cam);
                    }
                }
            }
            if ((D_0013CA40_9CDE0.pressed & 0x2000) || (D_E1_L00_00161E2C >= 16 && !(D_E1_L00_00161E2C & 7)) ||
                D_E1_L00_00161E2C >= 48) {
                if ((vendor_menu_r4.f40 ? item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].hA
                                        : item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].h8) > 0) {
                    n2 = vendor_menu_r4.f40 ? gBolts / item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].hA
                                            : gBolts / item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].h8;
                    room = item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].hE -
                           weapon_ammo_counts[vendor_menu_r4.e[vendor_menu_r4.idx].item];
                    if (room < n2)
                        n2 = room;
                    if (vendor_buy_quantity < n2) {
                        vendor_buy_quantity++;
                        if (!(D_E1_L00_00161E2C & 3) || D_E1_L00_00161E2C < 16) {
                            Moby_PlaySound(1, 0, vendor_menu_r4.cam);
                        }
                    }
                }
            }
        }
        if ((D_0013CA40_9CDE0.pressed & 0x10) && !(D_E1_L00_00161E30 & 0x10)) {
            char *m;
            vendor_menu_r4.state = 3;
            m = vendor_menu_r4.obj;
            vendor_purchase_confirmed = 0;
            MobyBlendAnimation(m + 0x400, 0, ((VBObj *)m)->pc->sub->cnt - 1, 1);
            moby_build_matrix(vendor_menu_r4.obj + 0x400);
        } else if ((D_0013CA40_9CDE0.pressed & 0x40) && !(D_E1_L00_00161E30 & 0x40)) {
            char *m;
            vendor_menu_r4.state = 3;
            m = vendor_menu_r4.obj;
            vendor_purchase_confirmed = 1;
            MobyBlendAnimation(m + 0x400, 0, ((VBObj *)m)->pc->sub->cnt - 1, 8);
            moby_build_matrix(vendor_menu_r4.obj + 0x400);
        } else {
            MobyAnimAdvance(vendor_menu_r4.obj + 0x400);
            if (*(u8 *)(vendor_menu_r4.obj + 0x453) == 0 && (*(u8 *)(vendor_menu_r4.obj + 0x470) & 2)) {
                MobyBlendAnimation(vendor_menu_r4.obj + 0x400, 1, 0, 1);
            }
            moby_build_matrix(vendor_menu_r4.obj + 0x400);
        }
    } else if (vendor_menu_r4.state == 3) {
        MobyAnimAdvance(vendor_menu_r4.obj + 0x400);
        if (*(u8 *)(vendor_menu_r4.obj + 0x453) == 0 && (*(u8 *)(vendor_menu_r4.obj + 0x470) & 2)) {
            *(float *)(vendor_menu_r4.obj + 0x458) = -1.0f;
            vendor_menu_r4.state = 4;
        }
        moby_build_matrix(vendor_menu_r4.obj + 0x400);
    } else if (vendor_menu_r4.state == 4) {
        MobyAnimAdvance(vendor_menu_r4.obj + 0x400);
        if (*(u8 *)(vendor_menu_r4.obj + 0x470) & 2) {
            if (vendor_purchase_confirmed == 1) {
                if (vendor_menu_r4.e[vendor_menu_r4.idx].sel == 1) {
                    cost = vendor_menu_r4.f40
                               ? item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].hA * vendor_buy_quantity
                               : item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].h8 * vendor_buy_quantity;
                } else {
                    cost = gHavePersuader[0] ? item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].f4
                                             : item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].f0;
                }
                if (gBolts < cost && weapon_ammo_counts[vendor_menu_r4.e[vendor_menu_r4.idx].item] >=
                                         item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].hE) {
                    Moby_PlaySound(0, 0, vendor_menu_r4.cam);
                } else if (vendor_menu_r4.e[vendor_menu_r4.idx].sel == 1) {
                    if (weapon_ammo_counts[vendor_menu_r4.e[vendor_menu_r4.idx].item] <
                        item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].hE) {
                        price = (vendor_menu_r4.f40 ? item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].hA
                                                    : item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].h8) *
                                vendor_buy_quantity;
                        if (gBolts >= price) {
                            if (weapon_ammo_counts[vendor_menu_r4.e[vendor_menu_r4.idx].item] >
                                item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].hE) {
                                weapon_ammo_counts[vendor_menu_r4.e[vendor_menu_r4.idx].item] =
                                    item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].hE;
                            }
                            n = vendor_buy_quantity;
                            n -= add_ammo(vendor_menu_r4.e[vendor_menu_r4.idx].item, n);
                            price =
                                (vendor_menu_r4.f40 ? item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].hA
                                                    : item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].h8) *
                                n;
                            ((int *)(level_render_state_base + 0xA))[vendor_menu_r4.e[vendor_menu_r4.idx].item] += n;
                            handle_vendor_auxiliary_moby_event(3);
                            gBolts = gBolts - price;
                            build_vendor_menu_rows(vendor_menu_r4.f40 != 0);
                            set_scrolling_status_message(msg_string(0x4F5F));
                            Moby_PlaySound(7, 0, vendor_menu_r4.cam);
                        } else {
                            set_scrolling_status_message(msg_string(0x4F60));
                            Moby_PlaySound(0, 0, vendor_menu_r4.cam);
                        }
                    } else {
                        Moby_PlaySound(0, 0, vendor_menu_r4.cam);
                    }
                } else {
                    price = gHavePersuader[0] ? item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].f4
                                              : item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].f0;
                    if (gBolts >= price) {
                        handle_vendor_auxiliary_moby_event(2);
                        grant_inventory_item(vendor_menu_r4.e[vendor_menu_r4.idx].item, 1);
                        vendor_menu_r4.f48 = vendor_menu_r4.e[vendor_menu_r4.idx].item;
                        if (vendor_menu_r4.e[vendor_menu_r4.idx].item == 0x18) {
                            hero_globals_base[0x2E12] = 1;
                        }
                        gBolts = gBolts - price;
                        build_vendor_menu_rows(vendor_menu_r4.f40 != 0);
                        if (vendor_menu_r4.f50 != -1)
                            set_animation_parameter(vendor_menu_r4.f50, 0);
                        vendor_menu_r4.f50 = -1;
                        if (vendor_menu_r4.e[vendor_menu_r4.idx].sel == 1) {
                            vendor_menu_r4.f50 =
                                queue_animation_update(0x30, vendor_menu_r4.e[vendor_menu_r4.idx].item + 0xEA60,
                                                       initialize_numeric_hud_timer_and_base_dimensions,
                                                       Hud_UpdateCounter, draw_weapon_ammo_hud_panel,
                                                       &weapon_ammo_counts[vendor_menu_r4.e[vendor_menu_r4.idx].item],
                                                       item_price_table[vendor_menu_r4.e[vendor_menu_r4.idx].item].hE);
                        } else {
                            if (vendor_menu_r4.count - 1 < vendor_menu_r4.idx) {
                                vendor_menu_r4.idx = vendor_menu_r4.count - 1;
                            }
                            initialize_vendor_selected_item_mobys();
                        }
                        set_scrolling_status_message(msg_string(0x4F5F));
                        Moby_PlaySound(7, 0, vendor_menu_r4.cam);
                    } else {
                        set_scrolling_status_message(msg_string(0x4F60));
                        Moby_PlaySound(0, 0, vendor_menu_r4.cam);
                    }
                }
            } else {
                Moby_PlaySound(0, 0, vendor_menu_r4.cam);
            }
            vendor_menu_r4.state = 0;
        }
        moby_build_matrix(vendor_menu_r4.obj + 0x400);
    }
}
