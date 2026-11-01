extern float func_00283CE0(int value);
extern int func_00283CF0(float value);
int func_L00_003DB0A8(int value)
{
    int alignment = 16;
    if (value % alignment != 0)
        {
        float quotient = func_00283CE0(value);
        quotient = quotient / (float)alignment;
        value = (func_00283CF0(quotient) + 1) * alignment;
    }
    return value;
}
