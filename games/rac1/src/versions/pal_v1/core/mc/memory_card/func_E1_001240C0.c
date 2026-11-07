int func_E1_001240C0(int port, int slot, char *dir, char *cur) {
    extern char D_E1_00159FC0[];
    extern void func_E1_00124038(char *);
    char *cd = mc_rpc_client;
    char *fp;
    int r;

    if (*(int *)(cd + 0x24) == 0) {
        return -100;
    }
    if (poll_semaphore(mc_sema) < 0) {
        return -200;
    }
    if (dir == 0 || *dir == 0) {
        signal_semaphore(mc_sema);
        return -210;
    }
    fp = mc_rpc_path_request;
    *(int *)(fp + 0x0) = port;
    *(char **)(fp + 0x10) = D_E1_00159FC0;
    *(int *)(fp + 0x4) = slot;
    libc_strncpy(fp + 0x14, dir, 0x3FF);
    fp[0x413] = 0;
    sceSifWriteBackDCache(D_E1_00159FC0, 0x400);
    r = sif_call_rpc(cd, 0xC, 1, fp, 0x414, mc_rpc_result, 4, func_E1_00124038, cur);
    if (r == 0) {
        mc_current_func = 0xC;
    } else {
        signal_semaphore(mc_sema);
    }
    return r;
}
