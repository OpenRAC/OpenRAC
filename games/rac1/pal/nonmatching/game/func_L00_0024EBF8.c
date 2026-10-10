/* Spins while bit 0x100 of the word at the third argument (register $3 in retail: a flag word the caller points to) is set; returns when clear. */
void func_L00_0024EBF8(void *a0, void *a1, unsigned int *flag) {
    while ((*flag & 0x100) != 0) {
    }
}
