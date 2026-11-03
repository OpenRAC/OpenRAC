void magnetboots_update(Moby *m) {
    char *d = (char *)m->pvars;
    m->rot.z = FastAddRots(m->rot.z, g_frame_time_x * 1.5707964f);
    switch (m->state) {
        case 0:
            initialize_moby_particle_effect(m);
            if (*(race_gates_base_r + 7) != 0) {
                DeleteMoby(m);
                return;
            }
            m->state = 1;
            m->pos.z = m->pos.z + 0.5f;
            m->scale = m->scale * 0.667f;
            break;
        case 1:
            emit_moby_sprite_particles(m);
            if (vec_distance_xy(&m->pos, g_hero_r + 0x80) < 3.0f) {
                char *h = g_hero_r;
                if (FastAbsF(m->pos.z - *(float *)(h + 0x88)) < 2.0f && *(int *)(h + 0x22A8) != 0) {
                    m->flags |= 0x41;
                    StartCutscene(2);
                    m->state = 2;
                }
            }
            break;
        case 2:
            if (orxon_cinematic_mode_r != 2) {
                ShowBanner(0x271B, -1);
                dialogue_message_duration_r = 0xB4;
                grant_inventory_item(0x1C, 1);
                if (*(int *)(d + 4) != -1) {
                    char *o = orxon_mobys_r + (*(int *)(d + 4) << 8);
                    jet_reset_camera(o + 0x10, o + 0x40);
                }
                memcard_Save(0, -1);
                m->state = 3;
            }
            break;
        case 3:
            DeleteMoby(m);
            break;
    }
}
