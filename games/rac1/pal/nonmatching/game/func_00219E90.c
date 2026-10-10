/* Opens the pause menu: makes the start page current, sets up the menu camera vectors and the
   texture and help text buffers behind the two stream buffers, then creates the fourteen menu
   mobys and starts each on its page animation.
   Adapted from Lombyte (MIT) for PAL: src/rendering/fun_00218f98.c, FUN_00218f98. */
void func_00219E90(void) {
    s32 i;
    s32 a, b, c, d;
    D_001D5F70_19E90.state = 2;
    D_001D5F70_19E90.current = &D_001D4948_19E90;
    D_001D5F70_19E90.next = &D_001D4948_19E90;
    D_001D5F70_19E90.close_locked = 0;
    func_001F9C30_19E90((s32)D_0019C250_19E90, (s32)&D_001602C0_19E90, 1.0f);
    qcopy(D_0019C250_19E90 - 0x10, &D_001602D0_19E90);
    qzero(D_0019C250_19E90 + 0x20);
    qzero(D_0019C260_19E90);
    func_00234AC8_19E90(1);
    func_00122598_19E90(0);
    a = D_001941C0_19E90[1] + 0xA0000;
    b = D_001941C0_19E90[2] + 0xA0000;
    c = a + 0x30000;
    d = b + 0xE0000;
    D_001D5F70_19E90.unkFC = c + 0xC1000;
    D_0015F538_19E90++;
    D_001D5F70_19E90.unk100 = d + 0x11800;
    D_0016100C_19E90 = 0xA0000;
    D_001D5F70_19E90.unk104 = a;
    D_001D5F70_19E90.unk10 = b;
    D_001D5F70_19E90.help_text_buffer = c;
    D_001D5F70_19E90.unk10C = d;
    func_00226D50_19E90(1);
    func_002348E8_19E90();
    D_001D5F70_19E90.saved_texture_start = D_0015EF78_19E90;
    if (D_001D5F70_19E90.current != 0) {
        for (i = 0; i < 14; i++) {
            struct O2_19E90 *o = (struct O2_19E90 *)func_00226720_19E90(0x472);
            D_001D6120_19E90[i] = o;
            if (o != 0) {
                s32 k;
                o->f34 &= 0xFFFD;
                D_001D6120_19E90[i]->f74 = D_0023B578_19E90;
                D_001D6120_19E90[i]->f10 = D_00187040_19E90.f140;
                D_001D6120_19E90[i]->f14 = D_00187040_19E90.f144;
                D_001D6120_19E90[i]->f18 = D_00187040_19E90.f148;
                D_001D6120_19E90[i]->f40 = 0;
                D_001D6120_19E90[i]->f44 = 0;
                D_001D6120_19E90[i]->f48 = 0;
                k = D_001D5F70_19E90.current->moby_anims[i];
                func_00213D28_19E90(D_001D6120_19E90[i], k, D_001D6120_19E90[i]->f24->tbl[k]->f10 - 1);
            }
        }
    }
}

