typedef struct {
    char pad[0x148];
    int o[20];
} OutV1;
void load_misc_animations_into_stash(void) {
    int h = lvl_load_scratch_buffer;
    int i;
    for (i = 0; i < 20; i++) {
        int sz = disc_table.e[i].n << 7;
        if (sz) {
            Load(h, disc_table.e[i].a, disc_table.e[i].n);
            ((OutV1 *)&level_menu_system)->o[i] = Stash_SendData(h, sz, sz, (int)&misc_animation_stash_tag);
        }
    }
}
