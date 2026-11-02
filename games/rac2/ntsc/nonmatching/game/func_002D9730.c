extern void func_002FC560(unsigned int, unsigned long long);
extern void func_00295F60(int, int);
int func_002D9730(void *object)
{
    *(int *)((char *)object + 0x44) = 0;
    func_002FC560(66, 68);
    func_002FC560(71, 11);
    func_00295F60(1, 0);
    return 8;
}
