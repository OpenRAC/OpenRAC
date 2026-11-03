/* Keep the key lookup as a canonical call rather than inlining its level copy. */
extern int func_L01_003246F8(int key);
extern void func_L01_003246A0(int index);

void func_L01_003246D0(int key)
{
    func_L01_003246A0(func_L01_003246F8(key));
}
