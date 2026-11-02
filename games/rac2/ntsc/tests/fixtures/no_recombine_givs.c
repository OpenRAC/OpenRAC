/* Fixture for tests/test_no_recombine_givs.py: authored, not game code. */
struct entry {
    unsigned char pad[16];
    unsigned int size;
    unsigned char pad14[12];
    char name[32];
};

extern struct entry dir[];
extern int compare(const char *a, const char *b, int n);
extern int seen[4];

/* Three reduced pointers: the entry, its name and name + 5. The digit read is
   entry->name[4], combined with the entry pointer; giv recombination would move
   it onto name + 5 (the last pointer set before it) as -1(name + 5). */
int scan(int n)
{
    int i;
    int blocks = 0;

    for (i = 0; i < n; i++) {
        struct entry *e = &dir[i];
        char *name = (char *)((int)dir + i * 64 + 32);

        blocks += e->size;
        if (compare(name, "save", 4) == 0 && compare(name + 5, "dat", 4) == 0) {
            int slot = (unsigned char)e->name[4] - '0';

            if (slot >= 0 && slot < 4)
                seen[slot] = 1;
        }
    }
    return blocks;
}

/* Control: one pointer only, nothing to recombine. */
int count(int n)
{
    int i;
    int blocks = 0;

    for (i = 0; i < n; i++)
        blocks += dir[i].size + (unsigned char)dir[i].name[4];
    return blocks;
}
