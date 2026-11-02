typedef short s16;
typedef int s32;
typedef float f32;
extern f32 D_L18_001B12A8 __attribute__((sda));
extern f32 D_L18_001B12AC __attribute__((sda));
extern f32 D_L18_001B12B0 __attribute__((sda));
extern f32 D_L18_001B12B4 __attribute__((sda));
extern f32 D_L18_001B12B8 __attribute__((sda));
extern f32 D_L18_001B12BC __attribute__((sda));
extern f32 func_00282E80(char *, char *);
extern f32 func_00283248(f32, f32);
extern f32 func_00283C80(f32, f32);

/* Tests whether the target is close enough and inside this object's sight angle. */
s32 func_L18_00426E48(char *self, char *target) {
    f32 range = D_L18_001B12A8;
    f32 arc = D_L18_001B12AC;
    f32 distance, angle;
    s16 kind = *(s16 *)(self + 0xAA);
    if (kind == 0xED4) goto special_range;
    if (kind < 0xED5) goto test_target;
    if (kind == 0xEE1) goto alternate_range;
    goto test_target;
special_range:
    range = D_L18_001B12B0;
    arc = D_L18_001B12B4;
    goto test_target;
alternate_range:
    range = D_L18_001B12B8;
    arc = D_L18_001B12BC;
test_target:
    distance = func_00282E80(self + 0x10, target + 0x10);
    angle = func_00283248(*(f32 *)(target + 0x10) - *(f32 *)(self + 0x10),
                         *(f32 *)(target + 0x14) - *(f32 *)(self + 0x14));
    angle = func_00283C80(*(f32 *)(self + 0xF8), angle);
    return distance < range && angle < arc;
}
