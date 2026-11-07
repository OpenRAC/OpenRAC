extern int *D_E1_00161180 MACRO_ADDR;
extern void func_E1_002A0738(void);
void queue_framebuffer_upload(unsigned int source) {
    int remaining = frame_size[0];
    int destination = draw_buffer_address >> 8;
    unsigned long count;
    unsigned long transfer;
    unsigned char *packet;
    unsigned long *words;

    do {
        count = (long)remaining;
        if (count > 0x80)
            count = 0x80;
        *(unsigned int *)(((unsigned char *)D_E1_00161180) + 0) = 0x10000006;
        remaining -= 0x80;
        *(unsigned int *)(((unsigned char *)D_E1_00161180) + 4) = 0;
        *(unsigned int *)(((unsigned char *)D_E1_00161180) + 8) = 0;
        *(unsigned int *)(((unsigned char *)D_E1_00161180) + 12) = 0x50000006;
        packet = ((unsigned char *)D_E1_00161180);
        D_E1_00161180 = (int *)(packet + 0x10);
        transfer = (unsigned long)count << 7;
        words = (unsigned long *)(packet + 0x10);
        words[0] = 0x4000000000000001UL;
        words[1] = 0x0eeeeeee;
        words[2] = ((unsigned long)(unsigned int)destination << 32) | 0x8000000000000UL;
        words[3] = 0x50;
        words[4] = 0;
        words[5] = 0x51;
        words[6] = ((unsigned long)count << 32) | 0x200;
        words[7] = 0x52;
        words[8] = 0;
        words[9] = 0x53;
        words[10] = transfer | 0x0800000000008000UL;
        words[11] = 0;
        D_E1_00161180 = (int *)(packet + 0x70);
        *(unsigned int *)(packet + 0x70) = (unsigned int)(long)(int)(transfer | 0x30000000);
        *(unsigned int *)(((unsigned char *)D_E1_00161180) + 4) = source;
        source += (int)(count * 0x800);
        destination += (int)(count * 8);
        *(unsigned int *)(((unsigned char *)D_E1_00161180) + 8) = 0;
        *(unsigned int *)(((unsigned char *)D_E1_00161180) + 12) = (unsigned int)transfer | 0x50000000;
        D_E1_00161180 = (int *)(((unsigned char *)D_E1_00161180) + 0x10);
    } while (remaining > 0);
    func_E1_002A0738();
}
