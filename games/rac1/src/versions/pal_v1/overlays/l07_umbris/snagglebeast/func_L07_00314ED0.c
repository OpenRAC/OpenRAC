int update_snagglebeast_chase_movement(char *m, char *a, void *p6, float *p7, float thr) {
    float v[4];
    char *mp = (char *)&((Moby *)m)->pos;
    float d = vec_distance_xy(mp, p6);
    int r = 4;
    char *tbl = umbris_paths[*(int *)(a + 0x290)];
    if (thr < d) {
        r = ClassifyTargetInCone(m, a + 0x70, p7, a + 0x100);
        if (r == 2) {
            int i = select_point_index_by_distance(mp, tbl, 0.0f);
            FastVecSub(v, tbl + (i * 16 + 0x10), mp);
            clamp_vec_length(v, v, *(float *)(a + 0x94));
            FastVecAdd(mp, mp, v);
        }
    } else {
        Hero *g = (Hero *)g_hero;
        float h;
        float dx = g->motion.pos.f[0] - ((Moby *)m)->pos.x;
        float dy = g->motion.pos.f[1] - ((Moby *)m)->pos.y;
        h = fast_atan2(dx, dy);
        MobySpringTurn_v(m, h, (float *)(a + 0x168), g_frame_time_sq * 2.0943952f, g_frame_time_sq * 0.6981317f,
                         g_frame_time * 2.0943952f);
    }
    if ((umbris_frame_count & 3) == 3) {
        char *mp2 = (char *)&((Moby *)m)->pos;
        if (probe_vertical_ground_collision(mp2, (int)m, 0, 0.1f, 1.0f)) {
            int *t = umbris_coll_hit;
            Moby *q = (Moby *)(*(unsigned char **)(t + 6));
            if (q != 0 && t[7] > 0 && q->oclass == 0x42D && q->command == 0) {
                if (((struct SnagglebeastVars *)q->pvars)->unk_BE != 0) {
                    FastVecSub(v, &q->pos, mp2);
                    v[2] = 0;
                    FastVecScale(v, v, 0.5f);
                    FastVecAdd(mp2, mp2, v);
                }
                (*(unsigned char **)(t + 6))[0xBC] = 1;
            }
        }
    }
    return r;
}
