/* Advance the resident table pointer and hand the engine the table start. Every global
   carries the sda attribute: under -G0 that gives the assembler macro form (lui + access
   sharing one register), and the one store inside the jal delay slot takes the single $gp
   form, so one name serves both of the retail's address forms. */
extern void *D_001B22A8 __attribute__((sda));
extern void *D_001B20B4 __attribute__((sda));
extern int D_001A72D0 __attribute__((sda));
extern int D_001A72D4 __attribute__((sda));
extern char D_00214120[];
extern void func_0011AEA0(int);
extern void func_002E1228(void);
extern void func_00282BA8(void *first, int second, int third);
extern void func_002E0F98(void);

void func_002E11C0(void)
{
    char *pointer = (char *)D_001B22A8;
    int other = D_001A72D4;

    D_001B20B4 = pointer;
    pointer += 0x10;
    D_001A72D0 = other;
    D_001B22A8 = pointer;
    func_0011AEA0(0);
    func_002E1228();
    func_00282BA8(D_00214120, 0x3200, 0x40);
    func_002E0F98();
}
