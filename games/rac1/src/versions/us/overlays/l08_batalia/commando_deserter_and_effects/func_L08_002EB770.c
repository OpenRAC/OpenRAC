void GroupClearUpdate(char *m) {
    extern unsigned char special_items_base[];
    extern char ApproachValue_b[] __asm__("func_00214D28");
    struct HeightFollowerVars *d = (struct HeightFollowerVars *)((Moby *)m)->pvars;
    char *o;
    int i;
    unsigned char *g2;
    switch (((Moby *)m)->state) {
        case 0:
            d->base_z = ((Moby *)m)->pos.z;
            ((Moby *)m)->state = 1;
            break;
        case 1: {
            extern short batalia_mobys;
            i = d->driver;
            if (i < 0)
                break;
            o = (char *)((i << 8) + *(int *)&batalia_mobys);
        }
            if (((Moby *)o)->oclass == MOBY_CLASS_TRESSPASSER_PAD || ((Moby *)o)->oclass == 0x23F) {
                if (((Moby *)o)->state != 4)
                    break;
                g2 = special_items_base + 0x13B;
                g2[0x35] = 1;
                Moby_PlaySound_r(0, 0, (struct Moby *)m);
                ((Moby *)m)->state = 2;
            } else {
                ((void (*)(char *, float, float))(void *)ApproachValue_b)(
                    m + 0x18, (d->top_z - d->base_z) * *((float *)((Moby *)o)->pvars) + d->base_z,
                    g_frame_time_r * 10.0f);
            }
            break;
        case 2:
            ((void (*)(char *, float, float))(void *)ApproachValue_b)(m + 0x18, d->top_z, g_frame_time_r * 10.0f);
            break;
    }
}
