typedef int TI128 __attribute__((mode(TI)));

typedef struct {
    float x;
    float y;
    float z;
    float w;
} Vec16_Fc936841d;

extern int func_002B4038(short id, int mode);
extern int func_00283DB8(Vec16_Fc936841d *v, float scale);
extern void func_L00_003E5330(void);
extern void func_0029F338(char *object);
extern void func_L00_003E5710(char *object);
extern void func_L00_003E5978(char *object);
extern void func_L00_003E55B0(char *object);
extern void func_L00_003E5BD8(char *object);
extern void func_L00_003E6030(char *object);
extern void func_L00_003E6710(char *object);

void func_L00_003E5650(char *object)
{
    char *data;
    Vec16_Fc936841d local;
    int result;

    data = *(char **)(object + 0x68);
    if (func_002B4038(*(short *)(object + 0xAA), *(int *)(data + 0x28)) == 0) {
        func_L00_003E5330();
        func_0029F338(object);
        return;
    }
    *(TI128 *)&local = *(TI128 *)(object + 0x10);
    local.w = 2.0f;
    result = func_00283DB8(&local, 100.0f);
    if (result != -1) {
        if (*(unsigned char *)(object + 0x20) != 6) {
            func_L00_003E5710(object);
            func_L00_003E5978(object);
            func_L00_003E55B0(object);
        }
        func_L00_003E5BD8(object);
        func_L00_003E6030(object);
        func_L00_003E6710(object);
    }
}
