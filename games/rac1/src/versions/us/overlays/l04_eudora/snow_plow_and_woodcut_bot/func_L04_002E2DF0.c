void suck_cannon_update(Moby *moby) {
    PortalVars_002E2DF0 *vars = (PortalVars_002E2DF0 *)moby->pvars;
    Exit_002E2DF0 *exit;

    moby->rot.z = FastAddRots(moby->rot.z, g_frame_time_r * 1.5707964f);
    if (eudora_game_mode_r == 2)
        moby->flags |= 0x41;
    else if (moby->flags & 1)
        moby->flags &= ~0x41;
    switch (moby->state) {
        case 0:
            initialize_moby_particle_effect(moby);
            if (*(discount_purchase_pricing_r + 0xE)) {
                remove_moby_002E2DF0(moby);
                break;
            }
            moby->state = 1;
            moby->pos.z += 1.0f;
            break;
        case 1:
            update_suck_cannon_particles_r(moby);
            if (vec_distance_xy(&moby->pos, gHeroPos_r) < 3.0f) {
                moby->flags |= 0x41;
                start_scene(2);
                moby->state = 2;
            }
            break;
        case 2:
            if (eudora_game_mode_r == 2)
                break;
            if (!*(gHaveHeliPack_r + 7))
                queue_dialogue_message(0xFA7, -1);
            else
                queue_dialogue_message(0x53DF, -1);
            dialogue_message_duration_frames_r = 0xB4;
            grant_inventory_item(9, 1);
            if (vars->exitMoby != -1) {
                exit = &portal_mobys_002E2DF0[vars->exitMoby];
                jet_reset_camera(&exit->pos, &exit->rot);
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
