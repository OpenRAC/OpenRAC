char *spawn_hit_proxy(char *owner, void *position, float scale) {
    char *moby = CreateMoby(0x660);
    if (moby != 0) {
        char *data = ((char *)((Moby *)moby)->pvars);
        ((struct Moby *)moby)->update_dist = 0xFF;
        ((Moby *)moby)->draw_dist = 0xFF;
        ((struct Moby *)moby)->visible = 1;
        ((struct Moby *)moby)->state = 0;
        ((struct HitProxyVars *)data)->owner = owner;
        ((Moby *)moby)->scale = *(float *)(*(char **)&((struct Moby *)moby)->pclass + 0x24) * scale;
        qcopy(&((struct Moby *)moby)->pos, position);
        if (owner != 0) {
            ((struct HitProxyVars *)data)->owner_id = *(short *)(owner + 0xA6);
        }
        moby_build_matrix(moby);
    }
    return moby;
}
