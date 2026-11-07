extern char D_E1_L00_0015F560[] MACRO_ADDR;
void spawn_nearby_effect_without_direction(unsigned char *m, int x, int y, void *p, float f) {
    PostHitMessage_r(m, x, y, p, D_E1_L00_0015F560, f);
}
