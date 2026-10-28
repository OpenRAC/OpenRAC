void suck_cannon_update(Moby *moby) {
    PortalVars_002E2DF0 *vars = (PortalVars_002E2DF0 *)moby->pvars;
    Exit_002E2DF0 *exit;

    moby->rot.z = add_rot_002E2DF0(moby->rot.z, g_frame_time_r * 1.5707964f);
    if (eudora_game_mode_r == 2)
        moby->flags |= 0x41;
    else if (moby->flags & 1)
        moby->flags &= ~0x41;
    switch (moby->state) {
    case 0:
        initialize_moby_particle_effect_r(moby);
        if (*(discount_purchase_pricing_r + 0xE)) {
            remove_moby_002E2DF0(moby);
            break;
        }
        moby->state = 1;
        moby->pos.z += 1.0f;
        break;
    case 1:
        update_suck_cannon_particles_r(moby);
        if (dist_002E2DF0(&moby->pos, gHeroPos_r) < 3.0f) {
            moby->flags |= 0x41;
            StartCutscene_r(2);
            moby->state = 2;
        }
        break;
    case 2:
        if (eudora_game_mode_r == 2)
            break;
        if (!*(gHaveHeliPack_r + 7))
            ShowBanner_r(0xFA7, -1);
        else
            ShowBanner_r(0x5436, -1);
        dialogue_message_duration_frames_r = 0xB4;
        grant_inventory_item_r(9, 1);
        if (vars->exitMoby != -1) {
            exit = &portal_mobys_002E2DF0[vars->exitMoby];
            jet_reset_camera_r(&exit->pos, &exit->rot);
        } else {
            STUB_printf_r(suck_cannon_no_exit_fmt_r, *(s16 *)&moby->save_id);
        }
        moby->state = 3;
        memcard_Save_r(0, -1);
        break;
    case 3:
        remove_moby_002E2DF0(moby);
        break;
    }
}
