extern char *D_L08_001B29E8;
extern float D_L08_001AFA18 __attribute__((sda));
extern float func_00282E80(void *, void *);
extern int func_00283CF0(float);

void func_L08_003C63F0(char *actor)
{
    char *item = D_L08_001B29E8;
    for (; item != 0; item = *(char **)(item + 0x28)) {
        int kind = *(short *)(item + 0xAA);
        if (kind == 0x942 || kind == 0x962 || kind == 0xFFF || kind == 0x1002) {
            float distance = func_00282E80(actor + 0x10, item + 0x10);
            if (distance < D_L08_001AFA18) {
                *(unsigned char *)(item + 0xBC) |= 2;
                if ((float)*(unsigned char *)(item + 0x30) < distance)
                    *(unsigned char *)(item + 0x30) = func_00283CF0(distance + 5.0f);
                if ((float)*(short *)(item + 0x32) < distance)
                    *(short *)(item + 0x32) = func_00283CF0(distance + 5.0f);
            }
        }
    }
}
