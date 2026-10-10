/* sceSifCallRpc: take a free RPC packet, fill it and the client block, write
 * back the send and receive buffers, then send command 0x8000000A. With
 * mode & 1 it returns at once (end_func runs on completion); otherwise it
 * waits on a semaphore. Returns 0 when the call went through, -1 when no
 * packet is free, -2 or -3 when sending or the semaphore failed. */
s32 func_0011B4C8(void *client_p, s32 rpc_number, s32 mode, void *send_buf, s32 send_size,
                  void *recv_buf, s32 recv_size, void *end_func, void *end_data) {
    s32 params[3];
    char *client = (char *)client_p;
    char *packet;
    s32 request_id;
    s32 sema;
    s32 max;

    packet = (char *)func_0011AFE8(D_00156900);
    if (packet == 0) {
        return -1;
    }
    request_id = *(s32 *)(packet + 0x18);
    *(s32 *)(client + 0x20) = (s32)end_data;
    *(s32 *)(client + 0x0) = (s32)packet;
    *(s32 *)(client + 0x4) = request_id;
    *(s32 *)(client + 0x1C) = (s32)end_func;
    *(s32 *)(packet + 0x20) = rpc_number;
    *(s32 *)(packet + 0x24) = send_size;
    *(s32 *)(packet + 0x28) = (s32)recv_buf;
    *(s32 *)(packet + 0x2C) = recv_size;
    *(s32 *)(packet + 0x14) = (s32)packet;
    *(s32 *)(packet + 0x34) = *(s32 *)(client + 0x24);
    *(s32 *)(packet + 0x1C) = (s32)client;

    if ((mode & 2) == 0) {
        if (send_buf == recv_buf) {
            max = (send_size >= recv_size) ? send_size : recv_size;
            func_0011AD70(send_buf, max);
        } else {
            if (send_size > 0) {
                func_0011AD70(send_buf, send_size);
            }
            if (recv_size > 0) {
                func_0011AD70(recv_buf, recv_size);
            }
        }
    }

    if (mode & 1) {
        if (end_func != 0) {
            *(s32 *)(packet + 0x30) = 1;
        } else {
            *(s32 *)(packet + 0x30) = 0;
        }
        *(s32 *)(client + 0x8) = -1;
        if (func_0011ABC8(0x8000000A, (int)packet, 0x40, (int)send_buf,
                          *(s32 *)(client + 0x14), send_size) != 0) {
            return 0;
        }
        func_0011B090(packet);
        return -2;
    }

    params[1] = 1;
    params[2] = 0;
    sema = func_00118C70(params);
    *(s32 *)(client + 0x8) = sema;
    if (sema < 0) {
        func_0011B090(packet);
        return -3;
    }
    *(s32 *)(packet + 0x30) = 1;
    if (func_0011ABC8(0x8000000A, (int)packet, 0x40, (int)send_buf,
                      *(s32 *)(client + 0x14), send_size) == 0) {
        func_00118C80(*(s32 *)(client + 0x8));
        func_0011B090(packet);
        return -2;
    }
    func_00118CB0(*(s32 *)(client + 0x8));
    func_00118C80(*(s32 *)(client + 0x8));
    return 0;
}
