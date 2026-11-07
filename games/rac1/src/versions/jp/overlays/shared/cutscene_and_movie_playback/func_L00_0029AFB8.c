extern int D_J_00173AF0[];
void update_slideshow(void) {
    int t, i, r;
    int *src, *dst;
    float f;
    update_audio_stream_until_idle(0);
    f = *(float *)&gFadeLvl - 0.05f;
    *(float *)&gFadeLvl = f;
    if (f < 0.0f) {
        *(float *)&gFadeLvl = 0.0f;
    }
    if (IST == 0) {
        if (ITMR >= D_J_00173AF0[IIDX] - 0x24) {
            if (*(short *)cd_read_active != 0) {
                update_audio_stream_until_idle(1);
            }
            if (IIDX < 0x25) {
                ICNT = -1;
                IST = 1;
            } else {
                end_slideshow();
                return;
            }
        }
    } else if (IST == 1) {
        VU1_syncChain(1);
        t = ICNT;
        if (t < 0x20) {
            src = (int *)slideshow[3];
            dst = (int *)slideshow[2];
            src += t;
            dst += t;
            for (i = 0; i <= 0x37FFF; i += 0x20) {
                *dst = *src;
                dst += 0x20;
                src += 0x20;
            }
        } else if (t >= 0x24) {
            IST = 0;
            ICNT = -1;
            IIDX = IIDX + 1;
            r = IIDX % 20;
            if (pal_mode != 0) {
                start_audio_stream_read(slideshow[3], disc_table.alt[r].a, disc_table.alt[r].b);
            } else {
                start_audio_stream_read(slideshow[3], disc_table.normal[r].a, disc_table.normal[r].b);
            }
        }
    }
    ICNT = ICNT + 1;
    ITMR = ITMR + 1;
    if (*(int *)(&pad0_pressed) & 0x800) {
        end_slideshow();
    }
    sound_update();
}
