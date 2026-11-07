extern int vendor_quantity_increase_hold_frames MACRO_ADDR;
extern float D_E1_L00_00161E24 MACRO_ADDR;
void open_vendor_menu(VendorMoby_pv *m) {
    s32 i;
    Vec4 *v;
    s32 half;
    s32 sel;
    s32 k;
    VendorSlot_pv *slot;

    FastMemSet((Vendor_pv *)vendor_menu, 0, 0x220);
    FastMemSet(vendor_extra, 0, 0x40);
    if (m == 0) {
        m = CreateMoby(0xB);
        *(s16 *)((char *)m + 0x32) = 0x40;
        *(f32 *)((char *)m + 0x10) = gCameraLvl.x + *(f32 *)&vendor_camera_eye_local_offset;
        *(f32 *)((char *)m + 0x14) = gCameraLvl.y;
        *(f32 *)((char *)m + 0x18) = gCameraLvl.z - vendor_camera_eye_local_offset_z;
        clear_u64_value(&m->rot);
        m->rot.f[2] = 3.1415927f;
        moby_build_matrix(m);
        m->state = 3;
        VP.mode = 1;
    }
    if ((VP.timer = g_game_language - 1) < 0) {
        VP.timer = 0;
    }
    VP.f44 = 0;
    VP.f48 = VP.title_text = VP.item_text = -1;
    VP.cursor = 0;
    l00_vendor_close_pending = 0;
    l00_vendor_opening_countdown_active = 0;
    vendor_quantity_increase_hold_frames = 0;
    VP.f5C = 0;
    vendor_buy_quantity = 0;
    D_E1_L00_00161E24 = 0;
    snd_PauseAllSoundsInGroup(0x1D);
    music_Pause(0);
    snd_FlushSoundCommands();
    build_vendor_menu_rows(VP.mode != 0);
    half = VP.count;
    sel = half / 2;
    if (half < 3)
        sel = 1;
    VP.cursor = half / 2;
    VP.scroll = (sel - half / 2) * 0x28;
    reset_hud_and_suspend_weapon_updates();
    VP.title_text = queue_animation_update(0x12, 0x754E, initialize_bolt_counter_hud,
                                           update_bolt_counter_hud_animation_and_visibility,
                                           draw_bolt_counter_hud_panel, &gBolts, 0x98967F);
    if (VP.entries[VP.cursor].selected == 1) {
        s32 item = VP.entries[VP.cursor].item;

        VP.item_text = queue_animation_update(0x30, item + 0xEA60, initialize_numeric_hud_timer_and_base_dimensions,
                                              Hud_UpdateCounter, draw_weapon_ammo_hud_panel, &weapon_ammo_counts[item],
                                              item_price_table[item].hE);
    }
    skip_frame = 1;
    VP.f0 = 0;
    VP.f10 = lvl_resource_buffers.x8 + 0x60000;
    VP.f14 = lvl_resource_buffers.x4 + 0x60000;
    VP.f8 = 0;
    VP.moby = (char *)m;
    Moby_PlaySound(3, 0, m);
    if (VP.mode == 0) {
        FadeToBlack(4);
    }
    clear_active_help_record();
    *(f32 *)&gFadeLvl = 1.0f;
    *(s32 *)((char *)&skip_frame - 0x14) = 5;
    level_white_fade_level = 0;
    HeroEnterMoveState(100, 1);
    {
        u8 *hp = (u8 *)(char *)&g_hero;
        hp[0x20AC] = 1;
        hp[0x20A5] = 1;
    }
    set_hero_item_moby_flag_bit0();
    if (VP.mode != 0) {
        MobySetAnimation(m, 3, 0);
    } else {
        MobySetAnimation(m, 2, 0);
    }
    m->f58 = frame_scale * 0.5f;
    if (VP.mode == 0) {
        transform_vector_by_basis(&VP.eye, &vendor_camera_eye_local_offset, VP.moby + 0xC0);
        FastVecAdd(&VP.eye, &VP.eye, VP.moby + 0x10);
        VP.look.f[2] = FastAddRots(*(f32 *)(VP.moby + 0x48), 3.1415927f);
        jet_mount(&VP.eye, &VP.look, 1, 0, 0);
        set_camera_position(&VP.eye);
        set_camera_rotation(&VP.look);
    }
    v = &level_moby_lights_14_direction;
    transform_vector_by_basis(v, &vendor_light_local_direction, VP.moby + 0xC0);
    qcopy(v - 1, &vendor_light_color);
    v += 2;
    qzero(v);
    qzero(&level_moby_lights_14_field_20);
    for (i = 0; i < 4; i++) {
        slot = &vendor_slot_manipulators[i];
        if (slot->hidden == 0) {
            AttachManipulator(VP.moby, i + 0x14, slot);
        }
    }
    if (VP.count < 8) {
        k = 7 - VP.count;
        vendor_slot_manipulators[0].offset = k * 0x681;
        vendor_slot_manipulators[1].offset = k * -0x681;
        vendor_slot_manipulators[2].offset = k * 0x681;
        vendor_slot_manipulators[3].offset = k * -0x681;
    } else {
        vendor_slot_manipulators[0].offset = 0;
        vendor_slot_manipulators[1].offset = 0;
        vendor_slot_manipulators[2].offset = 0;
        vendor_slot_manipulators[3].offset = 0;
    }
    vendor_auxiliary_moby_active = 0;
    vendor_auxiliary_moby_load_pending = 0;
}
