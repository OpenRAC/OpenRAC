void poll_pad_device_state(void *arg0) {
    PadState *p = arg0;
    unsigned char buf[0x40];
    int n, i;

    p->held = 0;
    p->prev_held = p->unk348;
    p->prev_raw_held = p->raw_held;
    p->raw_held = 0;
    p->device_state = sce_pad2_get_state(p->socket);
    if (p->device_state == 1) {
        switch (p->profile_state) {
            case 0:
                n = sce_pad2_get_button_profile(p->socket, buf);
                if (*(unsigned int *)buf == 0xFFFFFFFF) {
                    p->unk1DC = 0x79;
                } else {
                    p->unk1DC = 0;
                }
                if (n < 5) {
                    for (i = 0; i < n; i++) {
                        p->profile[i] = buf[i];
                    }
                    p->profile_state = 1;
                } else {
                    n = 0;
                    p->profile_state = 2;
                }
                for (i = n; i < 4; i++) {
                    p->profile[i] = buf[i];
                }
                n = sce_vib_get_profile(p->socket, buf);
                if (n < 5) {
                    for (i = 0; i < n; i++) {
                        p->act[i] = buf[i];
                    }
                } else {
                    n = 0;
                }
                for (i = n; i < 4; i++) {
                    p->act[i] = buf[i];
                }
                ClearPadInput(p);
                break;
            case 1:
                n = scePad2Read(p->socket, buf);
                process_pad_input(p, buf, n);
                break;
            case 2:
                ClearPadInput(p);
                break;
        }
    } else {
        ClearPadInput(p);
        p->profile_state = 0;
    }
    p->unk188 = 0;
    p->unk189 = 0;
}
