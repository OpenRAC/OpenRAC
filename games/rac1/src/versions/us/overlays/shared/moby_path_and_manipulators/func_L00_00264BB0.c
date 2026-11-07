void manipulator_set_scale(struct ManipState *p, float x) {
    if (*(unsigned char *)0x15EDB7) {
        p->goal_scale = x;
        return;
    }
    p->goal_scale = 1.0f;
}
