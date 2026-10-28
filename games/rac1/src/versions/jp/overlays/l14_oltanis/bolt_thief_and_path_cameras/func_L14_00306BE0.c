void CollectiblePickupUpdate(struct Moby *moby) {
    u8 *o = (u8 *)moby;
    u8 *d = (u8 *)moby->pvars;
    f32 v[4];
    u8 state = moby->state;

    switch (state) {
    case 0:
        if (saved_mission_state_r[moby->mission + g_current_level_index_r * 16] == 0xFF) {
            DeleteMoby_r(moby);
            return;
        }
        initialize_morphoray_private_state_r((void *)moby);
        ProbeGroundHeight_r(o + 0x10, 0, 0.5f);
        moby->rot.x = 0.0f;
        moby->rot.y = -1.5707964f;
        moby->pos.z = oltanis_coll_hit_point_z_r[0] + D_L14_001622B8_06BE0;
        qcopy(d, o + 0x10);
        moby->state = 1;
        break;
    case 1:
        *(f32 *)(o + 0x48) = FastAddRots_r(*(f32 *)(o + 0x48), D_L14_001622BC_06BE0);
        qcopy(o + 0x10, d);
        FastVecScale_r2(v, o + 0xd0, D_L14_001622C0_06BE0);
        FastVecAdd_r2(o + 0x10, o + 0x10, v);
        update_morphoray_particles_r(moby);
        if (oltanis_frame_count_r % 10 != 0)
            return;
        if (!(vec_distance_xy_r(o + 0x10, g_hero_r.motion.pos.f) < 1.0f))
            return;
        if (!(FastAbsF_r(*(f32 *)(o + 0x18) - g_hero_r.motion.pos.f[2]) < 2.0f))
            return;
        if (gHaveMorphORay_r != 0)
            *(s32 *)(d + 0x18) = state;
        else
            *(s32 *)(d + 0x18) = 0;
        grant_inventory_item_r(0x15, 1);
        StartCutscene_r(6);
        moby->state = 2;
        break;
    case 2:
        if (oltanis_game_mode_r != state) {
            if (saved_mission_state_r[moby->mission + g_current_level_index_r * 16] != 0xFF) {
                jet_clear_persistent_r(moby->mission);
                memcard_Save_r(0, -1);
            }
            if (*(s32 *)(d + 0x18) != 0)
                ShowBanner_r(0x5437, -1);
            else
                ShowBanner_r(0x36b2, -1);
            Help_Request_r(0x36b0, 0x7a);
            DeleteMoby_r(moby);
            return;
        }
        *(u16 *)(o + 0x34) |= 0x41;
        break;
    case 3:
        ApproachValue_r((f32 *)(o + 0x2c), 0.0f, *(f32 *)(*(u8 **)(o + 0x24) + 0x24) * 0.02f);
        if (*(f32 *)(o + 0x2c) == 0.0f)
            DeleteMoby_r(moby);
        break;
    }
}
