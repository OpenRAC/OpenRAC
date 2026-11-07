extern char *D_E1_L00_0015FFA4 MACRO_ADDR;
char *select_ryno_target(float *pos, float *lim, int *skip, float maxy, float maxp, float maxd) {
    char *best = 0;
    float bestScore = 1e9f;
    char *m = D_E1_L00_0015FFA4;
    while (m != 0) {
        if (((struct Moby *)m)->draw_dist != 0) {
            if (skip != 0) {
                int found = 0;
                int k;
                for (k = 0; k < 7; k++) {
                    if (skip[k] == (int)m) {
                        found = 1;
                        break;
                    }
                }
                if (found)
                    goto next;
            } else if (((struct Moby *)m)->visible == 0) {
                goto next;
            }
            {
                float *r = (float *)GetFlaggedPvarWord(m);
                if (r != 0 && !(r[0] < 0.0f) && (((struct Moby *)m)->flags & 0x1000) && m != 0 &&
                    *(int *)&((struct Moby *)m)->pclass != 0 &&
                    *(short *)(*(int *)&((struct Moby *)m)->pclass + 0x46) == 5) {
                    float e;
                    float *p = &((struct Moby *)m)->pos.x;
                    float d = vec_distance_xy(pos, p);
                    if (d < maxd) {
                        float v[4];
                        float a, b, g;
                        qcopy(v, p);
                        v[2] = v[2] + (r[4] + 0.1f);
                        a = fast_atan2(v[0] - pos[0], v[1] - pos[1]);
                        a = FastDiffRots(lim[2], a);
                        e = a * a;
                        if (e < maxy * maxy) {
                            b = fast_atan2(d, v[2] - pos[2]);
                            b = FastDiffRots(lim[1], b);
                            g = b * b;
                            if (g < maxp * maxp) {
                                if (d > 5.0f) {
                                    e = e * g * d;
                                    e = e + d;
                                } else
                                    e = d / 5.0f;
                                if (e < bestScore) {
                                    if (coll_line(pos, v, 2, (int)m, 0) == 0) {
                                        bestScore = e;
                                        best = m;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    next:
        m = *(char **)(m + 0x28);
    }
    return best;
}
