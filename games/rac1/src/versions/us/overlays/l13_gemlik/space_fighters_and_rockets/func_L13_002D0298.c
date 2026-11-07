void forcefield_tower_update(char *moby) {
    struct ForcefieldTowerVars *d = (struct ForcefieldTowerVars *)((Moby *)moby)->pvars;
    float vec[4];
    float blk[4];
    float acc[4];
    int state = ((Moby *)moby)->state;

    switch (state) {
        case 0: {
            int a = ((Moby *)moby)->update_dist;
            int b = ((Moby *)moby)->draw_dist;
            unsigned short h;
            int w, rnd;
            ((Moby *)moby)->command = 0;
            ((Moby *)moby)->state = 1;
            if (a < b)
                ((Moby *)moby)->update_dist = *(unsigned char *)&((Moby *)moby)->draw_dist;
            rnd = g_current_level_index;
            d->voice = -1;
            h = ((Moby *)moby)->save_id;
            w = *(int *)(level_moby_killed + ((short)h >> 5) * 4 + (rnd << 8));
            if ((w >> (h & 0x1F)) & 1) {
                if (d->linked_slot != -1)
                    linked_state_flags[d->linked_slot] = 1;
                DeleteMoby(moby);
            } else {
                moby_point_position(moby, 0, vec);
                vec[2] = vec[2] + 1.1f;
                spawn_hit_proxy(moby, vec, 2.7f);
                ((Moby *)moby)->flags &= 0xEFFF;
            }
            break;
        }
        case 1: {
            int t = d->linked_slot;
            int v;
            if (t != -1)
                linked_state_flags[t] = 0;
            if (d->voice == -1)
                goto alloc;
            if (voice_is_playing(moby, d->voice))
                goto done;
            if (d->voice != -1)
                goto play;
        alloc:
            d->voice = Moby_PlaySound_s(0, 4, (int)moby);
            if (d->voice == -1)
                goto done;
        play:
            qzero(vec);
            vec[2] = vec[2] + 8.7f;
            set_voice_position(d->voice, vec);
        done:
            v = ((Moby *)moby)->visible;
            if (v != 0 && random_integer_below(9) == 0) {
                int s17 = func_001160D8();
                int s16, s18, e, m;
                float f20;
                s17 = (s17 + 0x30) & 0x3F;
                s16 = func_001160D8();
                s16 = (s16 + 0x20) & 0x3F;
                s18 = func_001160D8() & 0x2F;
                moby_point_position(moby, 0, vec);
                vec[2] = vec[2] + 0.5f;
                f20 = random_float_between(900000.0f, 1800000.0f);
                e = func_001160D8();
                m = scale_ticks(e % 40 + 40);
                func_L00_0026AA10_C4748(vec, s17, s16, s18, f20, 0.0f, m);
            }
            process_forcefield_tower_damage(moby, ((char *)d), (float *)d->hit_block);
            break;
        }
        case 2: {
            float f = g_frame_time;
            float s;
            int c;
            int k;
            *(u128z_2D0298 *)blk = 0;
            f = f * 8.0f;
            s = 1000.0f;
            c = ((Moby *)moby)->command;
            blk[2] = f;
            *(u128z_2D0298 *)vec = *(u128z_2D0298 *)blk;
            switch (c) {
                case 0:
                    s = 8.5f;
                    break;
                case 1:
                    s = 17.0f;
                    break;
                case 2:
                    s = 17.0f;
                    break;
            }
            if (s < 100.0f) {
                if (is_value_within_interpolated_window(moby, s)) {
                    moby_point_position(moby, ((Moby *)moby)->command + 1, blk);
                    spawn_explosion_s(moby, vec, blk, 0.0f, 0.0f, 10, 3, 16, 4.0f, 2.0f, 0.0f, 1.0f, -1, 20.0f, 1, 1,
                                      -1, 0);
                    ((Moby *)moby)->command = ((Moby *)moby)->command + 1;
                }
            }
            if (((Moby *)moby)->anim_flags & 2) {
                spawn_explosion_s(moby, vec, 0, 0.0f, 0.0f, 10, 3, 16, 4.0f, 2.0f, 0.0f, 1.0f, -1, 20.0f, 1, 1, -1, 0);
                moby_point_position(moby, 3, blk);
                FastVecSub(blk, blk, &((Moby *)moby)->pos);
                k = 4;
                do {
                    float r;
                    k--;
                    r = random_float_between(0.0f, 3.0f);
                    FastVecScale(acc, blk, r);
                    FastVecAdd(acc, acc, &((Moby *)moby)->pos);
                    emit_forcefield_tower_destruction_effects(moby, acc, 1);
                } while (k >= 0);
                if (d->linked_slot != -1)
                    linked_state_flags[d->linked_slot] = 1;
                DeleteMoby(moby);
            }
        } break;
    }
}
