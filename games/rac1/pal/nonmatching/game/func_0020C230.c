/* DmaToSprSync(void): waits while channel 8's CHCR has STR (0x100) set. */
void func_0020C230(void)
{
    while (*(unsigned int *)0x1000D400 & 0x100) {
    }
}
