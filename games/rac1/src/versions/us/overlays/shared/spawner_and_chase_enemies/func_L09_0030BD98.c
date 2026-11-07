void process_spawner_hit_record(char *moby, char *d) {
    int n;
    struct HitMessage *m = (struct HitMessage *)MobyGetHitMessage(moby, 0xC30000, 0);
    ComputeHitResponse(moby, m, d + 0x20, 0, &n, 0, 0, 4);
    if (n >= 2) {
        float v = *(float *)(d + 0x20) - m->damage;
        *(float *)(d + 0x20) = v;
        if (v > 0.0f) {
            d[0x67] = 0x5A;
            RerollEffectCounter(moby, (short *)(d + 0x60));
        } else {
            SetDeathBits(moby, 0, -1);
            moby[0x20] = 2;
            d[0x67] = 0x78;
            RerollEffectCounter(moby, (short *)(d + 0x60));
        }
    }
    ((unsigned char *)moby)[0xA4] = 0xFF;
    StepColorFade(moby, d + 0x60);
}
