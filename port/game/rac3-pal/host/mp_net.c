/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's online play (the multiplayer executable, I5BOOTN.ELF) over the OpenRAC server
 * (the OpenRAC/Server repository) instead of the retail servers.
 *
 * The retail game goes online through SCE-RT's libraries in its core: the network adaptor's IOP
 * modules (ezNetCnf, ezNetCtl, libnet), a DNAS check of the disc, then the Medius lobby client
 * (log in, channels, game lists, create and join) and the DME client (the game room: the players'
 * messages, net objects, the host). Every one of them talks to the IOP or to the console's sockets,
 * which the port does not have. The analysis (every layer, message and struct, with addresses) is
 * kept with the port's planning notes (rac3-mp/protocol.md); this file replaces the layers at the cuts it describes
 * (mp/libraries.tsv, the rows naming host/mp_net.c):
 *
 * - the network adaptor and its configuration: present, one combination, the interface up;
 * - DNAS: passed at once, its signature empty; the servers' names resolve to the OpenRAC server;
 * - SCE-RT's platform sockets: started without libnet (nothing below the lobby and the room uses
 *   them once the next two cuts are in);
 * - Medius at the message level: the game's requests (MediusSendLobbyMessage, class, type and the
 *   struct as the library sends it) are answered by a small lobby here, and its replies go through
 *   the library's own dispatcher (0x00152FB8) during MediusUpdate, so the library and the menus run
 *   as on the console. Logging in says hello to the server; the game list is the server's party
 *   list; creating a game hosts a party, with its settings as the party's advert; joining one joins
 *   that party;
 * - DME at the session level: connecting to the game's world gives a connection of the room's
 *   party, joining its session calls the game's three session callbacks (joined, a client came or
 *   went, the host), and every message the game or DME sends is relayed, unread, to the other
 *   members by the server's RELAY packet; what comes back goes to DME's receive path (0x0014CE48),
 *   which runs the game's handlers.
 *
 * The server: OPENRAC_SERVER=host[:port] (game.openrac.dev:7770 when unset). OPENRAC_RAC3_NET_LOG=1 logs
 * every lobby message and room packet. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
typedef SOCKET net_socket;
#define NET_INVALID INVALID_SOCKET
#define net_close closesocket
#else
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
typedef int net_socket;
#define NET_INVALID (-1)
#define net_close close
#endif

#include "mp/game_protos.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "rac3_version.h"

/* ---- The game's memory this file reads and writes (PAL's; NTSC-U's from rac3_mp_pairs.py) ---- */

/* The game's net state block and DNAS's proxy byte ($gp - 0x1870 in level 39), which no paired
 * instruction forms (rac3_mp_pairs.py, rac3_level_pairs.py): NTSC-U's read from its own code, the
 * resolver writing the hostnames at block + 0x1A2 (PAL 0x001A5E72, NTSC-U 0x001A5FF2) and the DNAS
 * screen reading the byte with $gp 0x00248140; the DNAS screen's state word, which it stores with
 * lui 0x25 and -0x2F40 (PAL) or -0x2E40 (NTSC-U); the PROFILES screen's substate, $gp - 0x4BC. */
#ifdef RAC3_PAL
#define NET_BLOCK 0x001A5CD0u
#define DNAS_PROXY_FLAG 0x00246750u
#define DNAS_STATE 0x0024D0C0u
#define PROFILES_SUBSTATE 0x00247B04u
#else
#define NET_BLOCK 0x001A5E50u
#define DNAS_PROXY_FLAG 0x002468D0u
#define DNAS_STATE 0x0024D1C0u
#define PROFILES_SUBSTATE 0x00247C84u
#endif
#define MEDIUS_HANDLE RAC3_MADDR(0x001A30DCu) /* the lobby connection's handle, 0 when none */
#define NET_PROFILES RAC3_MADDR(0x001FC6B0u)  /* the online profiles (8 x 0x1F0) */
#define NET_SERVER_IP (NET_BLOCK + 0x182u)    /* the server's IP string in use (32 bytes) */
#define NET_MASTER_IP (NET_BLOCK + 0x2A2u)    /* the master server's IP string */
#define NET_AUSTRALIA_IP (NET_BLOCK + 0x242u) /* the second server's IP string */
#define RT_NET_UP RAC3_MADDR(0x001A2A58u)     /* SCE-RT's "network started" word */
#define RT_LOCAL_IP (RT_NET_UP + 4u)          /* SCE-RT's local address (first octet low) */

/* ---- The OpenRAC server's protocol (OpenRAC/Server README, "Wire format") ---- */

enum {
    PK_HELLO,
    PK_PARTY,
    PK_CHAT,
    PK_DATA,
    PK_ERROR,
    PK_TIME,
    PK_UDP,
    PK_RELAY
};

enum {
    OP_CREATE,
    OP_JOIN,
    OP_LEAVE,
    OP_INFO,
    OP_JOINED,
    OP_LEFT,
    OP_END,
    OP_FIND,
    OP_LIST,
    OP_HOST_CHANGED,
    OP_RESUME,
    OP_KICK,
    OP_KICKED,
    OP_ADVERT,
    OP_SLOTS
};

#define LIST_ADVERTS 1
#define HELLO_READS_SLOTS 2 /* HELLO's client bit: send SLOTS (the room's stable slots) */
#ifdef RAC3_PAL
#define NET_FAMILY "rac3-pal-1"
#else
#define NET_FAMILY "rac3-ntsc-1"
#endif
#define NET_VERSION "rac3-online-1"

/* ---- Medius as this game's library sends it (protocol.md, "Lobby") ---- */

#define M_STATUS 0x18 /* every response's StatusCode */
#define M_SUCCESS 0
#define M_NO_RESULT 1
#define M_GAME_NOT_FOUND (-977)
#define M_LOGIN_FAILED (-995)
#define M_WORLD_LOBBY 2     /* the one channel's world id */
#define M_WORLD_OWN 900     /* this console's own game */
#define M_WORLD_LISTED 1000 /* the first listed game's world id */
#define M_PORT 10075        /* what the connection infos name; nothing connects to it */

/* A request with a reply: the reply's class, type and size, and whether it is a list (its last
 * word EndOfList). The sizes are the library's own (its handlers' results). */
typedef struct {
    uint8_t cls, type, reply_cls, reply_type;
    uint16_t size;
    uint8_t list;
} lobby_reply;

static const lobby_reply lobby_replies[] = {
    {1, 0x03, 1, 0x04, 0x030, 0}, /* SessionBegin */
    {1, 0x05, 1, 0x06, 0x01C, 0}, /* SessionEnd */
    {1, 0x07, 1, 0x08, 0x0C4, 0}, /* AccountLogin */
    {1, 0x11, 1, 0x12, 0x01C, 0}, /* AccountUpdateStats */
    {1, 0x15, 1, 0x16, 0x01C, 0}, /* AccountLogout */
    {1, 0x17, 1, 0x18, 0x020, 0}, /* AccountGetID */
    {1, 0x1B, 1, 0x1C, 0x030, 0}, /* GetMyIP */
    {1, 0x25, 1, 0x26, 0x0B8, 0}, /* JoinChannel */
    {1, 0x2F, 1, 0x30, 0x148, 1}, /* GameWorldPlayerList */
    {1, 0x31, 1, 0x32, 0x148, 0}, /* PlayerInfo */
    {1, 0x33, 1, 0x34, 0x18C, 0}, /* GameInfo0 */
    {1, 0x35, 1, 0x36, 0x064, 0}, /* ChannelInfo */
    {1, 0x39, 1, 0x3A, 0x070, 0}, /* FindPlayer */
    {1, 0x3F, 1, 0x40, 0x01C, 0}, /* AddToBuddyList */
    {1, 0x41, 1, 0x42, 0x01C, 0}, /* RemoveFromBuddyList */
    {1, 0x47, 1, 0x48, 0x120, 1}, /* Policy (EndOfText) */
    {1, 0x4B, 1, 0x4D, 0x40C, 1}, /* GetAnnouncements */
    {1, 0x4C, 1, 0x4D, 0x40C, 1}, /* GetAllAnnouncements */
    {1, 0x4E, 1, 0x4F, 0x01C, 0}, /* SetGameListFilter0 */
    {1, 0x50, 1, 0x51, 0x01C, 0}, /* ClearGameListFilter0 */
    {1, 0x54, 1, 0x55, 0x020, 0}, /* CreateClan */
    {1, 0x56, 1, 0x57, 0x01C, 0}, /* DisbandClan */
    {1, 0x58, 1, 0x59, 0x168, 0}, /* GetClanByID */
    {1, 0x5A, 1, 0x5B, 0x148, 0}, /* GetClanByName */
    {1, 0x5C, 1, 0x5D, 0x01C, 0}, /* TransferClanLeadership */
    {1, 0x60, 1, 0x61, 0x01C, 0}, /* RemovePlayerFromClan */
    {1, 0x62, 1, 0x63, 0x01C, 0}, /* InvitePlayerToClan */
    {1, 0x64, 1, 0x65, 0x118, 1}, /* CheckMyClanInvitations */
    {1, 0x66, 1, 0x67, 0x01C, 0}, /* RespondToClanInvitation */
    {1, 0x68, 1, 0x69, 0x01C, 0}, /* RevokeClanInvitation */
    {1, 0x6A, 1, 0x6B, 0x01C, 0}, /* RequestClanTeamChallenge */
    {1, 0x6C, 1, 0x6D, 0x0EC, 1}, /* GetMyClanMessages */
    {1, 0x6E, 1, 0x6F, 0x01C, 0}, /* SendClanMessage */
    {1, 0x70, 1, 0x71, 0x01C, 0}, /* ModifyClanMessage */
    {1, 0x72, 1, 0x73, 0x01C, 0}, /* DeleteClanMessage */
    {1, 0x74, 1, 0x75, 0x01C, 0}, /* RespondToClanTeamChallenge */
    {1, 0x76, 1, 0x77, 0x01C, 0}, /* RevokeClanTeamChallenge */
    {1, 0x78, 1, 0x79, 0x028, 1}, /* GetClanTeamChallengeHistory */
    {1, 0x7A, 1, 0x7B, 0x114, 1}, /* GetClanInvitationsSent */
    {1, 0x7C, 1, 0x7D, 0x170, 1}, /* GetMyClans */
    {1, 0x7E, 1, 0x7F, 0x0F0, 1}, /* GetAllClanMessages */
    {1, 0x82, 1, 0x83, 0x1C4, 1}, /* GetClanTeamChallenges */
    {1, 0x84, 1, 0x85, 0x01C, 0}, /* UpdateClanStats */
    {1, 0x88, 1, 0x89, 0x028, 0}, /* GetWorldSecurityLevel */
    {1, 0x8C, 1, 0x8D, 0x064, 1}, /* GetLocations */
    {1, 0x96, 1, 0x97, 0x01C, 0}, /* ChatToggle */
    {1, 0x98, 1, 0x99, 0x05C, 0}, /* TextFilter */
    {1, 0xA3, 1, 0xA4, 0x01C, 0}, /* SetLocalizationParams */
    {1, 0xA5, 1, 0xA6, 0x0C8, 0}, /* FileCreate */
    {1, 0xA7, 1, 0xA9, 0x028, 0}, /* FileUpload */
    {1, 0xAA, 1, 0xAB, 0x0C8, 0}, /* FileClose */
    {1, 0xAC, 1, 0xAD, 0x1FC, 0}, /* FileDownload */
    {1, 0xC0, 1, 0xC1, 0x048, 1}, /* GetIgnoreList */
    {1, 0xC2, 1, 0xC3, 0x01C, 0}, /* AddToIgnoreList */
    {1, 0xC4, 1, 0xC5, 0x01C, 0}, /* RemoveFromIgnoreList */
    {1, 0xD2, 1, 0xD3, 0x048, 1}, /* ClanLadderList */
    {1, 0xD6, 1, 0xD7, 0x0D0, 1}, /* GetBuddyList_ExtraInfo */
    {1, 0xDA, 1, 0xDB, 0x1DC, 1}, /* GetClanMemberList_ExtraInfo */
    {1, 0xDC, 1, 0xDD, 0x0D0, 1}, /* GetLobbyPlayerNames_ExtraInfo */
    {1, 0xEA, 1, 0xEB, 0x190, 1}, /* GameList_ExtraInfo0 */
    {1, 0xF1, 1, 0xF2, 0x024, 0}, /* LadderPosition_ExtraInfo */
    {1, 0xF3, 1, 0x24, 0x0BC, 0}, /* JoinGame */
    {1, 0xF4, 1, 0x1E, 0x020, 0}, /* CreateGame1 */
    {4, 0x0B, 1, 0xCF, 0x01C, 0}, /* UpdateLadderStatsWide */
    {4, 0x0D, 4, 0x0E, 0x1B0, 0}, /* GetLadderStatsWide */
    {4, 0x0F, 1, 0xF0, 0x1D8, 1}, /* LadderList_ExtraInfo */
    {4, 0x12, 4, 0x13, 0x034, 0}, /* SetLobbyWorldFilter */
    {4, 0x14, 1, 0x22, 0x020, 0}, /* CreateChannel */
    {4, 0x15, 1, 0xED, 0x080, 1}, /* ChannelList_ExtraInfo1 */
};

/* ---- Little-endian fields of the game's structs, in host buffers ---- */

static void put32(uint8_t* p, int32_t v) {
    memcpy(p, &v, 4);
}

static int32_t get32(const uint8_t* p) {
    int32_t v;
    memcpy(&v, p, 4);
    return v;
}

static void put16(uint8_t* p, int16_t v) {
    memcpy(p, &v, 2);
}

static void put_text(uint8_t* p, const char* s, size_t size) {
    strncpy((char*)p, s, size - 1);
    p[size - 1] = 0;
}

static int net_log(void) {
    static int on = -1;
    if (on < 0) {
        const char* v = openrac_env_string("OPENRAC_RAC3_NET_LOG");
        on = v != NULL && *v != 0 && *v != '0';
    }
    return on;
}

/* ================================ The server link ================================ */

#define LINK_IN 0x20000
#define LINK_OUT 0x8000

static struct {
    net_socket fd;
    int state; /* 0 not tried, 1 connected and greeted, 2 hello sent, -1 failed */
    int32_t session;
    uint8_t in[LINK_IN];
    size_t in_len;
    uint8_t out[LINK_OUT];
    size_t out_len, frame;
} srv = {NET_INVALID, 0, 0, {0}, 0, {0}, 0, 0};

static void srv_begin(int id, int opcode) {
    srv.frame = srv.out_len;
    srv.out_len += 4;
    srv.out[srv.out_len++] = (uint8_t)id;
    if (opcode >= 0) {
        srv.out[srv.out_len++] = (uint8_t)opcode;
    }
}

static void srv_u8(int v) {
    srv.out[srv.out_len++] = (uint8_t)v;
}

static void srv_i32(int32_t v) {
    srv.out[srv.out_len++] = (uint8_t)(v >> 24);
    srv.out[srv.out_len++] = (uint8_t)(v >> 16);
    srv.out[srv.out_len++] = (uint8_t)(v >> 8);
    srv.out[srv.out_len++] = (uint8_t)v;
}

static void srv_bytes(const void* p, size_t n) {
    memcpy(srv.out + srv.out_len, p, n);
    srv.out_len += n;
}

/* A string: its length as a big-endian short (-1 for an empty one), then its bytes. */
static void srv_text(const char* s) {
    const size_t n = strlen(s);
    const uint16_t len = n == 0 ? 0xFFFFu : (uint16_t)n;
    srv.out[srv.out_len++] = (uint8_t)(len >> 8);
    srv.out[srv.out_len++] = (uint8_t)len;
    srv_bytes(s, n);
}

/* Ends the frame begun by srv_begin (its length first) and sends what is queued. */
static void srv_end(void) {
    const uint32_t len = (uint32_t)(srv.out_len - srv.frame - 4);
    uint8_t* p = srv.out + srv.frame;
    p[0] = (uint8_t)(len >> 24);
    p[1] = (uint8_t)(len >> 16);
    p[2] = (uint8_t)(len >> 8);
    p[3] = (uint8_t)len;
    if (srv.fd == NET_INVALID) {
        srv.out_len = 0;
        return;
    }
    size_t sent = 0;
    while (sent < srv.out_len) {
        const long n =
            (long)send(srv.fd, (const char*)srv.out + sent, (int)(srv.out_len - sent), 0);
        if (n <= 0) {
#ifndef _WIN32
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                continue;
            }
#endif
            fprintf(stderr, "[warning] rac3: online: the server went away\n");
            net_close(srv.fd);
            srv.fd = NET_INVALID;
            srv.state = -1;
            break;
        }
        sent += (size_t)n;
    }
    srv.out_len = 0;
}

/* Room for a frame of N bytes more in the send buffer. */
static int srv_room(size_t n) {
    return srv.out_len + n + 16 < LINK_OUT;
}

/* Connects to the server and says hello under NAME. The game thread waits for the TCP connection
 * (a local or LAN server answers at once), never for an answer. */
static int srv_open(const char* name) {
    if (srv.fd != NET_INVALID) {
        return 1;
    }
#ifdef _WIN32
    static int started;
    if (!started) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
        started = 1;
    }
#endif
    char host[128] = "game.openrac.dev";
    char port[16] = "7770";
    const char* env = openrac_env_string("OPENRAC_SERVER");
    if (env != NULL && *env != 0) {
        /* host:port, [v6]:port or [v6], a bare name or IPv4, or a bare IPv6 address (more than one
         * colon and no brackets: no port). */
        const char* close = env[0] == '[' ? strchr(env, ']') : NULL;
        const char* colon = strrchr(env, ':');
        if (close != NULL) {
            snprintf(host, sizeof host, "%.*s", (int)(close - env - 1), env + 1);
            if (close[1] == ':') {
                snprintf(port, sizeof port, "%s", close + 2);
            }
        } else if (colon != NULL && strchr(env, ':') == colon) {
            snprintf(host, sizeof host, "%.*s", (int)(colon - env), env);
            snprintf(port, sizeof port, "%s", colon + 1);
        } else {
            snprintf(host, sizeof host, "%s", env);
        }
    }
    struct addrinfo hints, *found = NULL;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &found) != 0 || found == NULL) {
        fprintf(stderr, "[warning] rac3: online: cannot resolve the server %s:%s\n", host, port);
        srv.state = -1;
        return 0;
    }
    /* Each address the name gives, IPv6 and IPv4 alike, until one takes the connection. */
    net_socket fd = NET_INVALID;
    for (const struct addrinfo* at = found; at != NULL && fd == NET_INVALID; at = at->ai_next) {
        fd = socket(at->ai_family, at->ai_socktype, at->ai_protocol);
        if (fd != NET_INVALID && connect(fd, at->ai_addr, (int)at->ai_addrlen) != 0) {
            net_close(fd);
            fd = NET_INVALID;
        }
    }
    freeaddrinfo(found);
    if (fd == NET_INVALID) {
        fprintf(stderr, "[warning] rac3: online: no OpenRAC server at %s:%s\n", host, port);
        srv.state = -1;
        return 0;
    }
    const int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&one, sizeof one);
#ifdef _WIN32
    u_long nonblocking = 1;
    ioctlsocket(fd, FIONBIO, &nonblocking);
#else
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
#endif
    srv.fd = fd;
    srv.in_len = 0;
    srv.state = 2;
    fprintf(
        stderr,
        "[info] rac3: online: connected to the OpenRAC server at %s:%s as %s\n",
        host,
        port,
        name
    );
    srv_begin(PK_HELLO, -1);
    srv_text(NET_VERSION);
    srv_text(name);
    srv_u8(HELLO_READS_SLOTS);
    srv_end();
    return 1;
}

/* ---- Reading the server's frames ---- */

typedef struct {
    const uint8_t* p;
    size_t n;
} reader;

static int rd_u8(reader* r) {
    if (r->n < 1) {
        return 0;
    }
    r->n--;
    return *r->p++;
}

static int32_t rd_i32(reader* r) {
    if (r->n < 4) {
        r->n = 0;
        return 0;
    }
    const int32_t v = (int32_t)((uint32_t)r->p[0] << 24 | (uint32_t)r->p[1] << 16
                                | (uint32_t)r->p[2] << 8 | r->p[3]);
    r->p += 4;
    r->n -= 4;
    return v;
}

static int rd_u16(reader* r) {
    const int hi = rd_u8(r);
    return hi << 8 | rd_u8(r);
}

static void rd_text(reader* r, char* out, size_t size) {
    const int16_t len = (int16_t)rd_u16(r);
    size_t n = len < 1 ? 0 : (size_t)len;
    if (n > r->n) {
        n = r->n;
    }
    const size_t kept = n < size - 1 ? n : size - 1;
    memcpy(out, r->p, kept);
    out[kept] = 0;
    r->p += n;
    r->n -= n;
}

/* ================================ The room (a party) ================================ */

#define ROOM_MAX 8

typedef struct {
    int32_t id; /* the server's session id */
    char name[33];
    int index; /* the DME client index every member gives it */
} member;

static struct {
    int in; /* in a party */
    char code[8];
    int32_t host;
    member m[ROOM_MAX];
    int count, next_index;
} room;

static member* room_member(int32_t id) {
    for (int i = 0; i < room.count; i++) {
        if (room.m[i].id == id) {
            return &room.m[i];
        }
    }
    return NULL;
}

static member* room_member_at(int index) {
    for (int i = 0; i < room.count; i++) {
        if (room.m[i].index == index) {
            return &room.m[i];
        }
    }
    return NULL;
}

/* A member comes in: the next index, the same order on every console (the party's order: the host,
 * then the members as they joined, which INFO lists and JOINED appends). */
static member* room_add(int32_t id, const char* name) {
    member* m = room_member(id);
    if (m != NULL || room.count == ROOM_MAX) {
        return m;
    }
    m = &room.m[room.count++];
    m->id = id;
    snprintf(m->name, sizeof m->name, "%s", name);
    m->index = room.next_index++;
    return m;
}

static int room_index_of(int32_t id) {
    const member* m = room_member(id);
    return m != NULL ? m->index : -1;
}

/* ================================ The lobby (Medius) ================================ */

/* A Medius reply waiting for the next MediusUpdate: [class][type][struct]. */
#define LOBBY_QUEUE 96
#define LOBBY_REPLY_MAX 0x420

static struct {
    uint16_t len;
    uint8_t bytes[LOBBY_REPLY_MAX];
} lobby_queue[LOBBY_QUEUE];

static int lobby_queued;

/* The settings a game is listed with: the server keeps them as the party's advert, opaque to it. */
typedef struct {
    int32_t min_players, max_players, level, skill, rules, field1, field2, field3, status,
        host_type;
    char name[64];
    uint8_t stats[256];
} game_advert;

#define ADVERT_VERSION 1

typedef struct {
    char code[8];
    char host[33];
    int players, max;
    int32_t world;
    game_advert ad;
} listed_game;

static struct {
    int connected; /* MediusConnect said yes */
    int32_t connect_cb, connect_ud;
    int connect_due;     /* the connect callback is called at the next update */
    char name[33];       /* the account name logged in with */
    uint8_t login[0x66]; /* the AccountLogin waiting for the server's hello */
    int login_waiting;
    uint8_t list_req[0x1A];
    int list_waiting;
    uint8_t create_req[0xD4];
    int create_waiting;
    uint8_t join_req[0xC4];
    int join_waiting;
    listed_game games[32];
    int games_count;
    game_advert own; /* this console's game, from CreateGame1 and each WorldReport0 */
    int own_valid;
} lobby;

static void lobby_push(int cls, int type, const uint8_t* body, size_t size) {
    if (lobby_queued == LOBBY_QUEUE || size + 2 > LOBBY_REPLY_MAX) {
        fprintf(stderr, "[warning] rac3: online: lobby reply %d/0x%02x dropped\n", cls, type);
        return;
    }
    lobby_queue[lobby_queued].len = (uint16_t)(size + 2);
    lobby_queue[lobby_queued].bytes[0] = (uint8_t)cls;
    lobby_queue[lobby_queued].bytes[1] = (uint8_t)type;
    memcpy(lobby_queue[lobby_queued].bytes + 2, body, size);
    lobby_queued++;
    if (net_log()) {
        fprintf(
            stderr,
            "[info] rac3: online: lobby reply %d/0x%02x, %zu bytes, status %d\n",
            cls,
            type,
            size,
            size >= M_STATUS + 4 ? get32(body + M_STATUS) : 0
        );
    }
}

static const lobby_reply* lobby_reply_of(int cls, int type) {
    for (size_t i = 0; i < sizeof lobby_replies / sizeof lobby_replies[0]; i++) {
        if (lobby_replies[i].cls == cls && lobby_replies[i].type == type) {
            return &lobby_replies[i];
        }
    }
    return NULL;
}

/* A reply to REQ: its MessageID (the first 21 bytes) echoed, the status, the rest zero; a list's
 * EndOfList set. */
static void lobby_start(uint8_t* reply, const lobby_reply* r, const uint8_t* req, int32_t status) {
    memset(reply, 0, r->size);
    memcpy(reply, req, 21);
    reply[20] = 0;
    put32(reply + M_STATUS, status);
    if (r->list) {
        put32(reply + r->size - 4, 1);
    }
}

/* A NetConnectionInfo (0x9C bytes) naming world WORLD: nothing connects to it, but the game copies
 * it into the next connect. */
static void lobby_connect_info(uint8_t* p, int32_t world) {
    put32(p + 0x00, 1);
    put32(p + 0x04, 1);
    put_text(p + 0x08, "127.0.0.1", 16);
    put32(p + 0x18, M_PORT);
    put32(p + 0x34, world);
    put_text(p + 0x78, "OPENRACSESSION01", 17);
}

/* The settings of a game as GameInfo and the game list give them, from a CreateGame1 request. */
static void advert_from_create(game_advert* a, const uint8_t* req) {
    memset(a, 0, sizeof *a);
    a->min_players = get32(req + 0x2C);
    a->max_players = get32(req + 0x30);
    a->level = get32(req + 0x34);
    memcpy(a->name, req + 0x38, 63);
    a->skill = get32(req + 0xB8);
    a->rules = get32(req + 0xBC);
    a->field1 = get32(req + 0xC0);
    a->field2 = get32(req + 0xC4);
    a->field3 = get32(req + 0xC8);
    a->host_type = get32(req + 0xCC);
}

/* ...and from the host's WorldReport0, which the host's own GameInfo poll must see again. */
static void advert_from_report(game_advert* a, const uint8_t* rep) {
    memcpy(a->name, rep + 0x1C, 63);
    a->name[63] = 0;
    memcpy(a->stats, rep + 0x5C, 256);
    a->min_players = get32(rep + 0x15C);
    a->max_players = get32(rep + 0x160);
    a->level = get32(rep + 0x164);
    a->skill = get32(rep + 0x168);
    a->rules = get32(rep + 0x16C);
    a->field1 = get32(rep + 0x170);
    a->field2 = get32(rep + 0x174);
    a->field3 = get32(rep + 0x178);
    a->status = get32(rep + 0x17C);
}

/* The advert on the wire: a version byte, ten little-endian words, the name and the stats. */
static void srv_advert(const game_advert* a) {
    const int32_t words[10] =
        {a->min_players,
         a->max_players,
         a->level,
         a->skill,
         a->rules,
         a->field1,
         a->field2,
         a->field3,
         a->status,
         a->host_type};
    srv_u8(ADVERT_VERSION);
    srv_bytes(words, sizeof words);
    srv_bytes(a->name, sizeof a->name);
    srv_bytes(a->stats, sizeof a->stats);
}

static int advert_read(game_advert* a, reader* r, size_t n) {
    memset(a, 0, sizeof *a);
    if (n < 1 + 40 + 64 + 256 || r->n < n) {
        r->p += n < r->n ? n : r->n;
        r->n -= n < r->n ? n : r->n;
        return 0;
    }
    const uint8_t* p = r->p;
    const int version = p[0];
    int32_t w[10];
    memcpy(w, p + 1, sizeof w);
    a->min_players = w[0];
    a->max_players = w[1];
    a->level = w[2];
    a->skill = w[3];
    a->rules = w[4];
    a->field1 = w[5];
    a->field2 = w[6];
    a->field3 = w[7];
    a->status = w[8];
    a->host_type = w[9];
    memcpy(a->name, p + 41, 64);
    a->name[63] = 0;
    memcpy(a->stats, p + 105, 256);
    r->p += n;
    r->n -= n;
    return version == ADVERT_VERSION;
}

static void lobby_send_advert(void) {
    if (!room.in || srv.fd == NET_INVALID || !srv_room(400)) {
        return;
    }
    srv_begin(PK_PARTY, OP_ADVERT);
    srv_advert(&lobby.own);
    srv_end();
}

/* A game's world id, kept while the same party is listed. */
static int32_t lobby_world_of(const char* code) {
    static char codes[64][8];
    static int known;
    for (int i = 0; i < known; i++) {
        if (strcmp(codes[i], code) == 0) {
            return M_WORLD_LISTED + i;
        }
    }
    if (known == 64) {
        known = 0;
    }
    snprintf(codes[known], sizeof codes[known], "%s", code);
    return M_WORLD_LISTED + known++;
}

static const listed_game* lobby_game(int32_t world) {
    for (int i = 0; i < lobby.games_count; i++) {
        if (lobby.games[i].world == world) {
            return &lobby.games[i];
        }
    }
    return NULL;
}

static void lobby_reply_login(int32_t status) {
    const lobby_reply* r = lobby_reply_of(1, 0x07);
    uint8_t reply[0xC4];
    lobby_start(reply, r, lobby.login, status);
    put32(reply + 0x1C, srv.session);
    put32(reply + 0x24, M_WORLD_LOBBY);
    lobby_connect_info(reply + 0x28, M_WORLD_LOBBY);
    lobby_push(r->reply_cls, r->reply_type, reply, r->size);
    lobby.login_waiting = 0;
}

static void lobby_reply_list(void) {
    const lobby_reply* r = lobby_reply_of(1, 0xEA);
    uint8_t reply[0x190];
    int shown = 0;
    for (int i = 0; i < lobby.games_count; i++) {
        const listed_game* g = &lobby.games[i];
        lobby_start(reply, r, lobby.list_req, M_SUCCESS);
        put32(reply + 0x1C, g->world);
        put16(reply + 0x20, (int16_t)g->players);
        put16(reply + 0x22, (int16_t)g->ad.min_players);
        put16(reply + 0x24, (int16_t)g->ad.max_players);
        put32(reply + 0x28, g->ad.level);
        put32(reply + 0x2C, g->ad.skill);
        put32(reply + 0x30, g->ad.rules);
        put32(reply + 0x34, g->ad.field1);
        put32(reply + 0x38, g->ad.field2);
        put32(reply + 0x3C, g->ad.field3);
        put32(reply + 0x44, g->ad.status);
        put32(reply + 0x48, g->ad.host_type);
        memcpy(reply + 0x4C, g->ad.name, 64);
        memcpy(reply + 0x8C, g->ad.stats, 256);
        put32(reply + 0x18C, i == lobby.games_count - 1);
        lobby_push(r->reply_cls, r->reply_type, reply, r->size);
        shown++;
    }
    if (shown == 0) {
        lobby_start(reply, r, lobby.list_req, M_NO_RESULT);
        lobby_push(r->reply_cls, r->reply_type, reply, r->size);
    }
    lobby.list_waiting = 0;
}

static void lobby_reply_game_info(const uint8_t* req, int32_t world) {
    const lobby_reply* r = lobby_reply_of(1, 0x33);
    uint8_t reply[0x18C];
    const game_advert* a = NULL;
    int players = 1;
    if (world == M_WORLD_OWN && lobby.own_valid) {
        a = &lobby.own;
        players = room.count > 0 ? room.count : 1;
    } else {
        const listed_game* g = lobby_game(world);
        if (g != NULL) {
            a = &g->ad;
            players = g->players;
        }
    }
    lobby_start(reply, r, req, a != NULL ? M_SUCCESS : M_GAME_NOT_FOUND);
    if (a != NULL) {
        put32(reply + 0x1C, GFN(int (*)(void), RAC3_MADDR(0x00183BDCu))());
        put32(reply + 0x20, a->min_players);
        put32(reply + 0x24, a->max_players);
        put32(reply + 0x28, a->level);
        put32(reply + 0x2C, a->skill);
        put32(reply + 0x30, players);
        memcpy(reply + 0x34, a->stats, 256);
        memcpy(reply + 0x134, a->name, 64);
        put32(reply + 0x174, a->rules);
        put32(reply + 0x178, a->field1);
        put32(reply + 0x17C, a->field2);
        put32(reply + 0x180, a->field3);
        put32(reply + 0x184, a->status);
        put32(reply + 0x188, a->host_type);
    }
    lobby_push(r->reply_cls, r->reply_type, reply, r->size);
}

static void lobby_reply_join(int32_t status, int32_t world) {
    const lobby_reply* r = lobby_reply_of(1, 0xF3);
    uint8_t reply[0xBC];
    lobby_start(reply, r, lobby.join_req, status);
    put32(reply + 0x1C, get32(lobby.join_req + 0x50));
    lobby_connect_info(reply + 0x20, world);
    lobby_push(r->reply_cls, r->reply_type, reply, r->size);
    lobby.join_waiting = 0;
}

static void lobby_reply_players(const uint8_t* req) {
    const lobby_reply* r = lobby_reply_of(1, 0x2F);
    uint8_t reply[0x148];
    for (int i = 0; i < room.count; i++) {
        lobby_start(reply, r, req, M_SUCCESS);
        put32(reply + 0x1C, room.m[i].id);
        put_text(reply + 0x20, room.m[i].name, 32);
        put32(reply + 0x144, i == room.count - 1);
        lobby_push(r->reply_cls, r->reply_type, reply, r->size);
    }
    if (room.count == 0) {
        lobby_start(reply, r, req, M_NO_RESULT);
        lobby_push(r->reply_cls, r->reply_type, reply, r->size);
    }
}

/* One request of the game's Medius library: answered here, or once the server answers. */
static void lobby_request(int cls, int type, const uint8_t* req, size_t size) {
    const lobby_reply* r = lobby_reply_of(cls, type);
    uint8_t reply[LOBBY_REPLY_MAX];
    if (net_log()) {
        fprintf(
            stderr, "[info] rac3: online: lobby request %d/0x%02x, %zu bytes\n", cls, type, size
        );
    }
    if (cls == 1 && type == 0x00 && size >= 0x180) {
        /* WorldReport0: the host's game as it is now, listed with its party. */
        game_advert before = lobby.own;
        advert_from_report(&lobby.own, req);
        lobby.own_valid = 1;
        if (memcmp(&before, &lobby.own, sizeof before) != 0) {
            lobby_send_advert();
        }
        return;
    }
    if (cls == 1 && type == 0x49 && size >= 0x18 && get32(req + 0x14) == 2 && room.in
        && srv.fd != NET_INVALID) {
        /* UpdateUserState(LeftGameWorld): the game is left, so is its party. */
        srv_begin(PK_PARTY, OP_LEAVE);
        srv_end();
        room.in = 0;
        room.count = 0;
        return;
    }
    if (r == NULL) {
        return; /* no reply: the reports, chat, user state, the DNAS post */
    }
    lobby_start(reply, r, req, M_SUCCESS);
    switch (cls << 8 | type) {
        case 0x103: /* SessionBegin: the session key */
            put_text(reply + 0x1C, "OPENRACSESSION01", 17);
            break;
        case 0x107: /* AccountLogin: hello to the server, the reply when it answers */
            memcpy(lobby.login, req, sizeof lobby.login);
            memcpy(lobby.name, req + 0x26, 32);
            lobby.name[32] = 0;
            if (lobby.name[0] == 0) {
                snprintf(lobby.name, sizeof lobby.name, "Player");
            }
            if (srv.state == 1) {
                lobby_reply_login(M_SUCCESS);
            } else if (srv_open(lobby.name)) {
                lobby.login_waiting = 1;
            } else {
                lobby_reply_login(M_LOGIN_FAILED);
            }
            return;
        case 0x11B: /* GetMyIP */
            put_text(reply + 0x1C, "127.0.0.1", 20);
            break;
        case 0x125: /* JoinChannel: the one channel's lobby */
            lobby_connect_info(reply + 0x1C, M_WORLD_LOBBY);
            break;
        case 0x12F: /* GameWorldPlayerList: the party's members */
            lobby_reply_players(req);
            return;
        case 0x133: /* GameInfo0 */
            lobby_reply_game_info(req, size >= 0x2C ? get32(req + 0x28) : 0);
            return;
        case 0x198: /* TextFilter: the text passes as it is */
            memcpy(reply + 0x1C, req + 0x2C, 64);
            break;
        case 0x147: /* Policy: the licence screen waits for a text (+0x1C) and its end (+0x11C) */
            put_text(reply + 0x1C, "Online play on an OpenRAC server.", 256);
            break;
        case 0x14B: /* GetAnnouncements: none */
        case 0x14C:
            put32(reply + M_STATUS, M_NO_RESULT);
            break;
        case 0x415: /* ChannelList_ExtraInfo1: one numbered channel ("%02X" at name + 0xB) */
            put32(reply + 0x1C, M_WORLD_LOBBY);
            put16(reply + 0x20, (int16_t)(room.count + 1));
            put16(reply + 0x22, 256);
            put32(reply + 0x28, 1);
            put_text(reply + 0x3C, "OpenRAC CH 01", 64);
            break;
        case 0x414: /* CreateChannel */
            put32(reply + 0x1C, M_WORLD_LOBBY);
            break;
        case 0x1EA: /* GameList_ExtraInfo0: the server's parties of this game */
            memcpy(lobby.list_req, req, sizeof lobby.list_req);
            if (srv.fd == NET_INVALID || lobby.list_waiting) {
                if (!lobby.list_waiting) {
                    lobby_reply_list();
                }
                return;
            }
            lobby.list_waiting = 1;
            srv_begin(PK_PARTY, OP_LIST);
            srv_text(NET_FAMILY);
            srv_u8(LIST_ADVERTS);
            srv_end();
            return;
        case 0x1F4: /* CreateGame1: a party hosted, its settings its advert */
            memcpy(lobby.create_req, req, sizeof lobby.create_req);
            advert_from_create(&lobby.own, req);
            lobby.own_valid = 1;
            if (srv.fd == NET_INVALID) {
                put32(reply + M_STATUS, -987);
                break;
            }
            lobby.create_waiting = 1;
            srv_begin(PK_PARTY, OP_CREATE);
            srv_text(lobby.own.name);
            srv_text(NET_FAMILY);
            srv_end();
            return;
        case 0x1F3: { /* JoinGame: its own game at once, another's once the server took us in */
            const int32_t world = get32(req + 0x28);
            memcpy(lobby.join_req, req, sizeof lobby.join_req);
            if (world == M_WORLD_OWN && room.in) {
                lobby_reply_join(M_SUCCESS, world);
                return;
            }
            const listed_game* g = lobby_game(world);
            if (g == NULL || srv.fd == NET_INVALID) {
                lobby_reply_join(M_GAME_NOT_FOUND, world);
                return;
            }
            lobby.join_waiting = 1;
            srv_begin(PK_PARTY, OP_JOIN);
            srv_text(g->code);
            srv_text(NET_FAMILY);
            srv_end();
            return;
        }
        default:
            /* Clans, buddies, ladders, stats: nothing to give, and a list is empty. */
            if (r->list) {
                put32(reply + M_STATUS, M_NO_RESULT);
            }
            break;
    }
    lobby_push(r->reply_cls, r->reply_type, reply, r->size);
}

/* ================================ The game room (DME) ================================ */

/* DmeConn's fields the game reads (protocol.md, "In-game"). */
#define CONN_SIZE 0x480
#define CONN_MY_INDEX 0x404
#define CONN_HOST_INDEX 0x408
#define CONN_STATE 0x430
#define CONN_DISPATCH 0x434
#define CONN_CLIENTS 0x43C
#define CONN_SLOT 0x448
#define DME_RELAY_KIND 1 /* the first byte of a RELAY payload: a DME packet follows */
#define OBJ_RELAY_KIND 2 /* ...a net object: id, type, owner, then its data */
#define OBJ_GONE_KIND 3  /* ...a net object released: its id */

static struct {
    gaddr conn; /* the room's connection, in the game's memory */
    gaddr args; /* the callbacks' argument blocks */
    gaddr buf;  /* a received packet, for DME's receive path */
    int connected, joined, join_due;
    int32_t cb_joined, cb_client, cb_host;
    int reported[ROOM_MAX]; /* the members the game was told of, by index */

    struct {
        int32_t from;
        uint16_t len;
        uint8_t data[0x600];
    } inbox[64];

    int inbox_count;
} dme;

static void dme_conn_refresh(void) {
    if (dme.conn == 0) {
        return;
    }
    const member* host = room_member(room.host);
    GREF(int32_t, dme.conn + CONN_MY_INDEX) = room_index_of(srv.session);
    GREF(int32_t, dme.conn + CONN_HOST_INDEX) = host != NULL ? host->index : 0;
    GREF(int32_t, dme.conn + CONN_CLIENTS) = ROOM_MAX;
}

/* Calls a session callback of the game with an argument block: words at +0, +4 and +8. */
static void dme_callback(int32_t fn, int32_t w0, int32_t w4, int32_t w8) {
    if (fn == 0) {
        return;
    }
    GREF(int32_t, dme.args + 0) = w0;
    GREF(int32_t, dme.args + 4) = w4;
    GREF(int32_t, dme.args + 8) = w8;
    GFN(void (*)(gaddr), (gaddr)fn)(dme.args);
}

/* Tells the game which members are in the session now: who came (kind 0) and who went (1). */
static void dme_report_members(void) {
    if (!dme.joined) {
        return;
    }
    const int me = room_index_of(srv.session);
    for (int i = 0; i < ROOM_MAX; i++) {
        const int here = i != me && room_member_at(i) != NULL;
        if (here != dme.reported[i]) {
            dme.reported[i] = here;
            if (net_log()) {
                fprintf(
                    stderr, "[info] rac3: online: room client %d %s\n", i, here ? "joined" : "left"
                );
            }
            dme_callback(dme.cb_client, here ? 0 : 1, 0, i);
        }
    }
}

static void dme_deliver(void) {
    if (!dme.joined || dme.conn == 0) {
        return;
    }
    const int count = dme.inbox_count;
    dme.inbox_count = 0;
    for (int i = 0; i < count; i++) {
        const int src = room_index_of(dme.inbox[i].from);
        if (src < 0) {
            continue;
        }
        memcpy(G(dme.buf), dme.inbox[i].data, dme.inbox[i].len);
        GFN(int (*)(int, int, int, int, int), RAC3_MADDR(0x0014CE48u))(
            GREF(int32_t, dme.conn + CONN_DISPATCH),
            (int)dme.conn,
            src,
            (int)dme.buf,
            dme.inbox[i].len
        );
    }
}

/* Sends a DME packet ([class][id][payload]) to DEST: a client index, -1 / 0xFFFE every other
 * member, 0xFFFF the DME server (none here: dropped). */
static int dme_relay(int dest, gaddr data, int len) {
    if (srv.fd == NET_INVALID || !room.in || len <= 0 || len > 0x5F0) {
        return 0;
    }
    int32_t target = 0;
    dest &= 0xFFFF;
    if (dest == 0xFFFF) {
        return 0;
    }
    if (dest != 0xFFFE) {
        const member* m = room_member_at(dest);
        if (m == NULL) {
            return 0;
        }
        target = m->id;
    }
    if (!srv_room((size_t)len + 8)) {
        return 0;
    }
    srv_begin(PK_RELAY, -1);
    srv_i32(target);
    srv_u8(DME_RELAY_KIND);
    srv_bytes(G(data), (size_t)len);
    srv_end();
    if (net_log()) {
        fprintf(
            stderr,
            "[info] rac3: online: room send to %d: class %d id %d, %d bytes\n",
            dest == 0xFFFE ? -1 : dest,
            GREF(uint8_t, data),
            GREF(uint8_t, data + 1),
            len
        );
    }
    return 0;
}

/* ================================ Net objects ================================ */

/* DME's net objects (tNW_GameSettings, tNW_PlayerData): each has one owner, who publishes it; the
 * others get a copy and the type's callbacks (on_create(0, id) the first time, then
 * on_update(0, id, -2) for the whole object). DME's own object layer needs its peer session, which
 * the room does not run, so the port keeps them: a record (+0 id, +4 owner client, +8 id, +0x1C
 * the data) in the game's memory, the data relayed whole at each publish. */
#define OBJ_MAX 64
#define OBJ_TYPES 16

static struct {
    int32_t size, on_create, on_update, on_destroy;
} obj_type[OBJ_TYPES];

static struct {
    int32_t id, type, owner;
    gaddr record, data;
} objs[OBJ_MAX];

static int obj_count, obj_next = 1;

static int obj_find(int32_t id) {
    for (int i = 0; i < obj_count; i++) {
        if (objs[i].id == id) {
            return i;
        }
    }
    return -1;
}

static int obj_new(int32_t id, int32_t type, int32_t owner) {
    if (obj_count == OBJ_MAX || type < 0 || type >= OBJ_TYPES || obj_type[type].size <= 0) {
        return -1;
    }
    const int i = obj_count++;
    objs[i].id = id;
    objs[i].type = type;
    objs[i].owner = owner;
    objs[i].record = openrac_guest_static(0x20);
    objs[i].data = openrac_guest_static((uint32_t)obj_type[type].size);
    GREF(int32_t, objs[i].record + 0x0) = id;
    GREF(int32_t, objs[i].record + 0x4) = owner;
    GREF(int32_t, objs[i].record + 0x8) = id;
    GREF(int32_t, objs[i].record + 0x1C) = (int32_t)objs[i].data;
    return i;
}

/* Sends object I to every other member (TARGET 0) or to one session. */
static void obj_send(int i, int32_t target) {
    const int32_t size = obj_type[objs[i].type].size;
    if (srv.fd == NET_INVALID || !room.in || !srv_room((size_t)size + 32)) {
        return;
    }
    srv_begin(PK_RELAY, -1);
    srv_i32(target);
    srv_u8(OBJ_RELAY_KIND);
    srv_i32(objs[i].id);
    srv_i32(objs[i].type);
    srv_i32(objs[i].owner);
    srv_bytes(G(objs[i].data), (size_t)size);
    srv_end();
}

static void obj_callback(int32_t fn, int32_t id, int32_t field, int with_field) {
    if (fn == 0) {
        return;
    }
    if (with_field) {
        GFN(void (*)(int, int, int), (gaddr)fn)(0, id, field);
    } else {
        GFN(void (*)(int, int), (gaddr)fn)(0, id);
    }
}

/* A member came in: it gets every object this console owns. */
static void obj_send_all_to(int32_t session) {
    const int me = room_index_of(srv.session);
    for (int i = 0; i < obj_count; i++) {
        if (objs[i].owner == me) {
            obj_send(i, session);
        }
    }
}

static void obj_receive(int kind, int32_t from, reader* r) {
    (void)from;
    const int32_t id = rd_i32(r);
    int i = obj_find(id);
    if (kind == OBJ_GONE_KIND) {
        if (i >= 0) {
            obj_callback(obj_type[objs[i].type].on_destroy, id, 0, 0);
            objs[i] = objs[--obj_count];
        }
        return;
    }
    const int32_t type = rd_i32(r);
    const int32_t owner = rd_i32(r);
    const int fresh = i < 0;
    if (fresh) {
        i = obj_new(id, type, owner);
    }
    if (i < 0 || (int32_t)r->n < obj_type[objs[i].type].size) {
        return;
    }
    objs[i].owner = owner;
    GREF(int32_t, objs[i].record + 0x4) = owner;
    memcpy(G(objs[i].data), r->p, (size_t)obj_type[objs[i].type].size);
    if (net_log()) {
        fprintf(
            stderr,
            "[info] rac3: online: net object %08x (type %d, owner %d) %s\n",
            (unsigned)id,
            type,
            owner,
            fresh ? "came" : "changed"
        );
    }
    if (fresh) {
        obj_callback(obj_type[objs[i].type].on_create, id, 0, 0);
    } else {
        obj_callback(obj_type[objs[i].type].on_update, id, -2, 1);
    }
}

/* ================================ The server's packets ================================ */

static void on_info(reader* r) {
    char code[8];
    rd_text(r, code, sizeof code);
    const int32_t host = rd_i32(r);
    const int count = rd_u8(r);
    memset(&room, 0, sizeof room);
    room.in = 1;
    snprintf(room.code, sizeof room.code, "%s", code);
    room.host = host;
    for (int i = 0; i < count; i++) {
        char name[33];
        const int32_t id = rd_i32(r);
        rd_text(r, name, sizeof name);
        room_add(id, name);
    }
    memset(dme.reported, 0, sizeof dme.reported);
    fprintf(
        stderr,
        "[info] rac3: online: in party %s (%d members, host %d)\n",
        room.code,
        room.count,
        room.host
    );
    if (lobby.create_waiting) {
        /* CreateGame1's answer: this console's own world; its settings listed. */
        const lobby_reply* rr = lobby_reply_of(1, 0xF4);
        uint8_t reply[0x20];
        lobby_start(reply, rr, lobby.create_req, M_SUCCESS);
        put32(reply + 0x1C, M_WORLD_OWN);
        lobby_push(rr->reply_cls, rr->reply_type, reply, rr->size);
        lobby.create_waiting = 0;
        lobby_send_advert();
    }
    if (lobby.join_waiting) {
        lobby_reply_join(M_SUCCESS, get32(lobby.join_req + 0x28));
    }
    dme_conn_refresh();
}

static void on_list(reader* r) {
    const int count = rd_u8(r);
    lobby.games_count = 0;
    for (int i = 0; i < count && r->n > 0; i++) {
        listed_game g;
        memset(&g, 0, sizeof g);
        char description[64];
        rd_text(r, g.code, sizeof g.code);
        rd_text(r, g.host, sizeof g.host);
        g.players = rd_u8(r);
        g.max = rd_u8(r);
        rd_text(r, description, sizeof description);
        const size_t n = (size_t)rd_u16(r);
        const int ok = advert_read(&g.ad, r, n);
        /* A party of this game whose host has not described its game yet is not shown. */
        if (!ok || lobby.games_count == 32) {
            continue;
        }
        g.world = lobby_world_of(g.code);
        lobby.games[lobby.games_count++] = g;
    }
    if (lobby.list_waiting) {
        lobby_reply_list();
    }
}

static void on_party(reader* r) {
    const int op = rd_u8(r);
    switch (op) {
        case OP_INFO:
            on_info(r);
            break;
        case OP_JOINED: {
            char name[33];
            const int32_t id = rd_i32(r);
            rd_text(r, name, sizeof name);
            room_add(id, name);
            fprintf(stderr, "[info] rac3: online: %s joined the party\n", name);
            obj_send_all_to(id);
            dme_conn_refresh();
            dme_report_members();
            break;
        }
        case OP_LEFT: {
            const int32_t id = rd_i32(r);
            member* m = room_member(id);
            if (m != NULL) {
                *m = room.m[--room.count];
            }
            dme_report_members();
            break;
        }
        case OP_END:
        case OP_KICKED:
            fprintf(stderr, "[info] rac3: online: the party is gone\n");
            room.in = 0;
            room.count = 0;
            dme_report_members();
            break;
        case OP_HOST_CHANGED:
            room.host = rd_i32(r);
            dme_conn_refresh();
            if (dme.joined) {
                const member* h = room_member(room.host);
                dme_callback(dme.cb_host, 0, h != NULL ? h->index : 0, 0);
            }
            break;
        case OP_SLOTS: {
            /* Each member's stable slot: its DME client index on every console. */
            const int count = rd_u8(r);
            for (int i = 0; i < count; i++) {
                const int32_t id = rd_i32(r);
                const int slot = rd_u8(r);
                member* m = room_member(id);
                if (m != NULL && slot < ROOM_MAX) {
                    m->index = slot;
                }
            }
            dme_conn_refresh();
            dme_report_members();
            break;
        }
        case OP_LIST:
            on_list(r);
            break;
        default:
            break;
    }
}

static void on_frame(int id, reader* r) {
    switch (id) {
        case PK_HELLO:
            srv.session = rd_i32(r);
            srv.state = 1;
            fprintf(stderr, "[info] rac3: online: session %d\n", srv.session);
            if (lobby.login_waiting) {
                lobby_reply_login(M_SUCCESS);
            }
            break;
        case PK_PARTY:
            on_party(r);
            break;
        case PK_ERROR: {
            const int reason = rd_u8(r);
            fprintf(
                stderr, "[warning] rac3: online: the server refused a request (reason %d)\n", reason
            );
            if (lobby.join_waiting) {
                lobby_reply_join(M_GAME_NOT_FOUND, get32(lobby.join_req + 0x28));
            }
            if (lobby.create_waiting) {
                const lobby_reply* rr = lobby_reply_of(1, 0xF4);
                uint8_t reply[0x20];
                lobby_start(reply, rr, lobby.create_req, -987);
                lobby_push(rr->reply_cls, rr->reply_type, reply, rr->size);
                lobby.create_waiting = 0;
            }
            if (lobby.list_waiting) {
                lobby.games_count = 0;
                lobby_reply_list();
            }
            break;
        }
        case PK_RELAY: {
            const int32_t from = rd_i32(r);
            const int kind = rd_u8(r);
            if (kind == OBJ_RELAY_KIND || kind == OBJ_GONE_KIND) {
                obj_receive(kind, from, r);
                break;
            }
            if (kind != DME_RELAY_KIND || r->n == 0 || r->n > sizeof dme.inbox[0].data) {
                break;
            }
            if (dme.inbox_count == (int)(sizeof dme.inbox / sizeof dme.inbox[0])) {
                break;
            }
            if (net_log()) {
                fprintf(
                    stderr,
                    "[info] rac3: online: room packet from session %d: class %d id %d, %zu bytes\n",
                    from,
                    r->p[0],
                    r->n > 1 ? r->p[1] : 0,
                    r->n
                );
            }
            dme.inbox[dme.inbox_count].from = from;
            dme.inbox[dme.inbox_count].len = (uint16_t)r->n;
            memcpy(dme.inbox[dme.inbox_count].data, r->p, r->n);
            dme.inbox_count++;
            break;
        }
        default:
            break; /* UDP offers, the clock, chat, data: unused here */
    }
}

/* Reads what the server sent and handles every whole frame. */
static void srv_pump(void) {
    if (srv.fd == NET_INVALID) {
        return;
    }
    for (;;) {
        if (srv.in_len == LINK_IN) {
            break;
        }
        const long n =
            (long)recv(srv.fd, (char*)srv.in + srv.in_len, (int)(LINK_IN - srv.in_len), 0);
        if (n > 0) {
            srv.in_len += (size_t)n;
            continue;
        }
        if (n == 0) {
            fprintf(stderr, "[warning] rac3: online: the server closed the connection\n");
            net_close(srv.fd);
            srv.fd = NET_INVALID;
            srv.state = -1;
            room.in = 0;
        }
        break;
    }
    size_t at = 0;
    while (srv.in_len - at >= 4) {
        const uint8_t* p = srv.in + at;
        const size_t len = (size_t)p[0] << 24 | (size_t)p[1] << 16 | (size_t)p[2] << 8 | p[3];
        if (len == 0 || len > LINK_IN - 4) {
            at = srv.in_len;
            break;
        }
        if (srv.in_len - at - 4 < len) {
            break;
        }
        reader r = {p + 5, len - 1};
        on_frame(p[4], &r);
        at += 4 + len;
    }
    memmove(srv.in, srv.in + at, srv.in_len - at);
    srv.in_len -= at;
}

/* Everything that waits for a frame of the game: the server's packets, the lobby's callbacks. */
static void net_pump(void) {
    srv_pump();
    if (dme.join_due && dme.conn != 0) {
        /* dme_join_session's answer: joined (status 0), then the others. The host callback is for a
         * change of host only (it raises net + 0x169, and the staging room leaves on it). */
        dme.join_due = 0;
        dme.joined = 1;
        dme_conn_refresh();
        fprintf(
            stderr,
            "[info] rac3: online: in the game room as client %d (host %d)\n",
            GREF(int32_t, dme.conn + CONN_MY_INDEX),
            GREF(int32_t, dme.conn + CONN_HOST_INDEX)
        );
        dme_callback(dme.cb_joined, 0, 0, 0);
        dme_report_members();
    }
}

/* ================================ The game's functions ================================ */

/* sceSifSearchModuleByName: the network adaptor's modules are loaded (the menus' "no network
 * adaptor" test, ezNetCnf's and ezNetCtl's inits). */
int mp_func_0012C220(int name) {
    (void)name;
    return 1;
}

/* ezNetCnfGetCombinationList(name, type, list): one combination, the first entry (+0x30; its first
 * word >= 0 is valid, its name at +0x88). */
int mp_func_00136ED0(int a0, int a1, int list) {
    (void)a0;
    (void)a1;
    memset(G(list), 0, 0x30 + 0xC8);
    GREF(int32_t, list) = 1;
    put_text((uint8_t*)G(list + 0x30 + 0x88), "OpenRAC server", 0x31);
    return 1;
}

int mp_func_001370B8(int a0, int a1, int a2) {
    (void)a0;
    (void)a1;
    (void)a2;
    return 0;
}

int mp_func_00137370(int a0, int a1, int a2) {
    (void)a0;
    (void)a1;
    (void)a2;
    return 0;
}

int mp_func_00137490(int a0, int a1, int a2) {
    (void)a0;
    (void)a1;
    (void)a2;
    return 0;
}

int mp_func_00137520(int a0, int a1, int a2) {
    (void)a0;
    (void)a1;
    (void)a2;
    return 0;
}

/* ezNetCtlGetStatus(out, ...): the interface is up (+0x30 = 3, which the connection screen 0x0067D758
 * goes on with; 5 is down), no error (+0x28 = 0). */
int mp_func_001375B0(int out, int a1, int a2) {
    (void)a1;
    (void)a2;
    memset(G(out), 0, 0x38);
    GREF(int32_t, out + 0x30) = 3;
    return 0;
}

int mp_func_00137700(int a0, int a1, int a2) {
    (void)a0;
    (void)a1;
    (void)a2;
    return 0;
}

/* SCE-RT's platform sockets: the network is up on 127.0.0.1 without libnet; no socket opens (the
 * lobby and the room are this file's). */
int mp_func_0015FA00(void) {
    GREF(uint32_t, RT_LOCAL_IP) = 0x0100007Fu;
    GREF(uint32_t, RT_NET_UP) = 1;
    return 0;
}

int mp_func_0015FA68(void) {
    return 0;
}

int mp_func_0015FAA8(void) {
    return 0;
}

int mp_func_0015FB40(int sock, int ip, int port, int desc) {
    static gaddr dummy;
    (void)ip;
    (void)port;
    /* rt_msg_client's start opens its shared socket (kind 3); nothing reads or writes it. */
    if (dummy == 0) {
        dummy = openrac_guest_static(0x40);
    }
    GREF(int32_t, dummy + 0x0) = 1;
    GREF(int32_t, dummy + 0x4) = 1;
    GREF(int32_t, dummy + 0xC) = desc != 0 ? GREF(int32_t, desc) : 0;
    GREF(int32_t, sock) = (int32_t)dummy;
    if (net_log()) {
        fprintf(stderr, "[info] rac3: online: SCE-RT opened a socket (a placeholder)\n");
    }
    return 0;
}

int mp_func_0015FC18(int sock) {
    (void)sock;
    return 0;
}

int mp_func_00161A18(int name, int ip) {
    (void)name;
    GREF(uint32_t, ip) = 0x0100007Fu;
    return 0;
}

int mp_func_0016A778(int params) {
    const int r = mp_func_0016A778__game(params);
    if (net_log()) {
        fprintf(stderr, "[info] rac3: online: rt_msg_client startup %d\n", r);
    }
    return r;
}

/* ---- Medius ---- */

/* MediusSendLobbyMessage(class, type, size, data): every request of the library. */
int mp_func_0017DFA8(int cls, int type, int size, int data) {
    if (size > 0 && size <= LOBBY_REPLY_MAX) {
        uint8_t req[LOBBY_REPLY_MAX];
        memcpy(req, G(data), (size_t)size);
        lobby_request(cls & 0xFF, type & 0xFF, req, (size_t)size);
    }
    return 0;
}

/* MediusConnect(params, out[2], user data): connected at once; the game's callback (params +0xF4)
 * at the next update, as the library's completion would call it. */
int mp_func_0017DFE0(int params, int out, int ud) {
    const int32_t handle = 0x4F52;
    GREF(int32_t, MEDIUS_HANDLE) = handle;
    GREF(int32_t, out) = handle;
    GREF(int32_t, out + 4) = 0;
    lobby.connected = 1;
    lobby.connect_cb = GREF(int32_t, params + 0xF4);
    lobby.connect_ud = ud;
    lobby.connect_due = 1;
    if (net_log()) {
        fprintf(
            stderr, "[info] rac3: online: lobby connect (world %d)\n", GREF(int32_t, params + 0x34)
        );
    }
    return 0;
}

int mp_func_0017E140(void) {
    GREF(int32_t, MEDIUS_HANDLE) = 0;
    lobby.connected = 0;
    return 0;
}

/* MediusUpdate: the server's packets, then the connect callback and the queued replies through
 * the library's dispatcher (it calls the response's handler, which calls the game's callback). */
static int medius_update(void) {
    static gaddr buf;
    net_pump();
    if (lobby.connect_due) {
        lobby.connect_due = 0;
        if (lobby.connect_cb != 0) {
            GFN(void (*)(int, int),
                (gaddr)lobby.connect_cb)(GREF(int32_t, MEDIUS_HANDLE), lobby.connect_ud);
        }
    }
    if (buf == 0) {
        buf = openrac_guest_static(LOBBY_REPLY_MAX + 16);
    }
    const int count = lobby_queued;
    lobby_queued = 0;
    for (int i = 0; i < count; i++) {
        memcpy(G(buf), lobby_queue[i].bytes, lobby_queue[i].len);
        GFN(int (*)(int, int, int, int, int, int),
            RAC3_MADDR(0x00152FB8u))(0, 0, 0, (int)buf, lobby_queue[i].len, 0);
    }
    return 0;
}

int mp_func_0017E1A0(void) {
    return medius_update();
}

int mp_func_0017E208(void) {
    return medius_update();
}

int mp_func_0017E2D0(void) {
    return medius_update();
}

/* ---- DME ---- */

/* DME's send of one message (kind, conn, dest, class, id, len, data), through the server (0x0014CAB8)
 * or to a peer (0x0014C950): its internal messages and the net objects' too. To the room. */
static int dme_send_one(int dest, int cls, int id, int len, int data) {
    static gaddr buf;
    if (len <= 0 || len > 0x5E0) {
        return 0;
    }
    if (buf == 0) {
        buf = openrac_guest_static(0x600);
    }
    GREF(uint8_t, buf) = (uint8_t)cls;
    GREF(uint8_t, buf + 1) = (uint8_t)id;
    memcpy(G(buf + 2), G(data), (size_t)len);
    return dme_relay(dest, buf, len + 2);
}

int mp_func_0014AAB0(int conn, int a, int b) {
    (void)conn;
    (void)a;
    (void)b;
    return 0;
}

/* dme_send_app(kind, conn, dest, id, len, data): the app message [2][id][data] to the room. */
int mp_func_0014AD08(int kind, int conn, int dest, int id, int len, int data) {
    (void)kind;
    (void)conn;
    return dme_send_one(dest, 2, id, len, data);
}

/* dme_connect_world(params, out): the room's connection, connected at once. */
int mp_func_0014ADD0(int params, int out) {
    (void)params;
    if (dme.conn == 0) {
        dme.conn = openrac_guest_static(CONN_SIZE);
        dme.args = openrac_guest_static(0x20);
        dme.buf = openrac_guest_static(0x600);
    }
    memset(G(dme.conn), 0, CONN_SIZE);
    GREF(int32_t, dme.conn + CONN_STATE) = 1;
    GREF(int32_t, dme.conn + CONN_DISPATCH) = 1;
    GREF(int32_t, dme.conn + CONN_SLOT) = 1;
    dme_conn_refresh();
    dme.connected = 1;
    dme.joined = 0;
    memset(dme.reported, 0, sizeof dme.reported);
    memset(G(out), 0, 0xC);
    GREF(int32_t, out + 8) = (int32_t)dme.conn;
    fprintf(
        stderr,
        "[info] rac3: online: game room connection (party %s)\n",
        room.in ? room.code : "none"
    );
    return 0;
}

int mp_func_0014AF70(int out, int conn) {
    (void)conn;
    GREF(int32_t, out) = dme.conn != 0 ? GREF(int32_t, dme.conn + CONN_MY_INDEX) : 0x100;
    return 0;
}

int mp_func_0014B040(int status, int conn) {
    (void)conn;
    GREF(int32_t, status) = dme.connected ? 1 : 2;
    return 0;
}

/* dme_update: the server's packets, the session's callbacks and the room's messages. */
int mp_func_0014B118(void) {
    net_pump();
    dme_deliver();
    return 0;
}

int mp_func_0014B1B8(int conn) {
    (void)conn;
    dme.joined = 0;
    return 0;
}

int mp_func_0014B218(int conn) {
    (void)conn;
    dme.joined = 0;
    dme.connected = 0;
    return 0;
}

/* dme_join_session(params, out): joined at the next update, with the game's callbacks for the
 * join (+0x20), each client (+0x28) and the host (+0x30). */
int mp_func_0014B448(int params, int out) {
    (void)out;
    dme.cb_joined = GREF(int32_t, params + 0x20);
    dme.cb_client = GREF(int32_t, params + 0x28);
    dme.cb_host = GREF(int32_t, params + 0x30);
    dme.join_due = 1;
    return 0;
}

int mp_func_0014CAB8(int kind, int conn, int dest, int cls, int id, int len, int data) {
    (void)kind;
    (void)conn;
    return dme_send_one(dest, cls, id, len, data);
}

int mp_func_0014C950(int kind, int conn, int dest, int cls, int id, int len, int data) {
    (void)kind;
    (void)conn;
    return dme_send_one(dest, cls, id, len, data);
}

int mp_func_0016EFB0(int out, int conn) {
    (void)conn;
    GREF(int32_t, out) = dme.conn != 0 ? GREF(int32_t, dme.conn + CONN_HOST_INDEX) : 0;
    return 0;
}

/* ---- DNAS (level 39): passed ---- */

int mp_func_L39_004B3910(void) {
    GREF(uint8_t, DNAS_PROXY_FLAG) = 0;
    return 0;
}

int mp_func_L39_004B3950(int params) {
    (void)params;
    return 0;
}

/* DNAS's shutdown, which the DNAS screen calls once DNAS passed, right before it starts the
 * resolver thread: the addresses are written here as well, as the port runs that thread only when
 * the game's scheduling reaches it. */
static void net_write_addresses(void) {
    put_text((uint8_t*)G(NET_SERVER_IP), "127.0.0.1", 32);
    put_text((uint8_t*)G(NET_MASTER_IP), "127.0.0.1", 32);
    put_text((uint8_t*)G(NET_AUSTRALIA_IP), "127.0.0.1", 32);
}

int mp_func_L39_004B3B18(void) {
    net_write_addresses();
    return 0;
}

int mp_func_L39_004B3E10(void) {
    return 0;
}

/* The status the DNAS screen polls: type 5, authenticated. */
int mp_func_L39_004B3F28(int status) {
    memset(G(status), 0, 0x10);
    GREF(int32_t, status) = 5;
    return 0;
}

int mp_func_L39_004B3F88(int a0, int a1, int a2) {
    (void)a0;
    (void)a1;
    (void)a2;
    return 0;
}

/* The DNAS signature the lobby posts after its session begins: none. */
int mp_func_L39_0067BBE8(int message) {
    (void)message;
    return 0;
}

#ifdef RAC3_PAL /* NTSC-U's resolver (0x00677D90) is not paired yet (rac3_mp_pairs.py) */
/* The thread resolving the servers' names: the OpenRAC server's, written as the original writes
 * the addresses it resolves (any non-empty string ends the DNAS screen's wait). */
int mp_func_L39_0067A7E0(void) {
    net_write_addresses();
    GFN(int (*)(void), RAC3_MADDR(0x001260A0u))();
    return 0;
}
#endif

/* ---- The network modules ---- */

/* The front end's load of the network modules (DEV9.IRX, then the pack's NETCNF, INET, ... and
 * the USB keyboard's): the adaptor is looked for (0x001981D8 sets byte +0x2C9 of the net block),
 * nothing is loaded, and the block's state says the modules are in (2). */
int mp_func_00198248(int pack, int iop) {
    (void)pack;
    (void)iop;
    GFN(int (*)(void), RAC3_MADDR(0x001981D8u))();
    GREF(int32_t, NET_BLOCK + 0x120u) = 2;
    return 0;
}

/* sceSifMInitRpc and sceSifMBindRpc (the libnet RPC's): no IOP; the client is bound (+0x24). */
int mp_func_00192460(void) {
    return 0;
}

int mp_func_00192790(int client, int number, int mode, int a3, int a4, int a5) {
    (void)number;
    (void)mode;
    (void)a3;
    (void)a4;
    (void)a5;
    GREF(int32_t, client + 0x24) = 1;
    return 0;
}

/* The lobby screens' clock in milliseconds (level 39's 0x0067CEA0: timer 0's count over 0x24000),
 * which their waits and retries measure: the host's, from the first call. */
int mp_func_L39_0067CEA0(void) {
    static int64_t start;
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    const int64_t ms = (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
    if (start == 0) {
        start = ms - 1;
    }
    return (int)(ms - start);
}

/* OPENRAC_RAC3_PROFILE=NAME[:PASSWORD]: an online profile in the first slot of the profile table
 * (0x001FC6B0, eight records of 0x1F0 bytes: +0x1D the account name, +0x3D the password, +0x50
 * bit 0 the password kept, +0xEC in use) when the memory card has none, as the PROFILES screen
 * after DNAS lists them; for scripted runs, whose card is empty and whose profile editor needs the
 * on-screen keyboard. */
static void net_profile_from_env(void) {
    const char* env = openrac_env_string("OPENRAC_RAC3_PROFILE");
    const gaddr table = NET_PROFILES;
    if (env == NULL || *env == 0) {
        return;
    }
    for (int i = 0; i < 8; i++) {
        if (GREF(uint8_t, table + 0x1F0u * (gaddr)i + 0xECu) != 0) {
            return;
        }
    }
    char name[0x20], password[0x13] = "openrac";
    snprintf(name, sizeof name, "%s", env);
    char* colon = strchr(name, ':');
    if (colon != NULL) {
        *colon = 0;
        snprintf(password, sizeof password, "%s", colon + 1);
    }
    memset(G(table), 0, 0x1F0);
    put_text((uint8_t*)G(table + 0x1D), name, 0x20);
    put_text((uint8_t*)G(table + 0x3D), password, 0x13);
    GREF(uint8_t, table + 0x50) = 1;
    GREF(uint8_t, table + 0xEC) = 1;
    static int told;
    if (!told) {
        told = 1;
        fprintf(stderr, "[info] rac3: online: profile %s from OPENRAC_RAC3_PROFILE\n", name);
    }
}

/* The DNAS screen's update (level 39's 0x0067ABC8). In its state 10 it waits for the resolver
 * thread it started to write the server's address; the port runs that thread only when the game
 * blocks, which this screen never does, so the addresses are written before it looks. */
int mp_func_L39_0067ABC8(void) {
    if (GREF(int32_t, DNAS_STATE) == 10 && GREF(uint8_t, NET_SERVER_IP) == 0) {
        net_write_addresses();
    }
    return mp_func_L39_0067ABC8__game();
}

/* lgAudInit (the USB headset's library, for voice chat): no headset. Its RPC server would answer
 * with the driver's version; with no IOP the library halts in a loop. */
int mp_func_0018E628(int a0, int a1) {
    (void)a0;
    (void)a1;
    return -1;
}

/* The online profiles' reset to defaults (0x00195010), which the PROFILES screen runs every frame
 * while the memory card holds no multiplayer save: OPENRAC_RAC3_PROFILE's profile after it. */
int mp_func_00195010(void) {
    const int r = mp_func_00195010__game();
    net_profile_from_env();
    return r;
}

/* The PROFILES screen's update (level 39's 0x0068C0E0, event 5 back, 6 LOGIN, 7 select; its
 * substate 3 is the list). A scripted run's card holds no multiplayer save, and the screen then
 * takes no SELECT or LOGIN, so with OPENRAC_RAC3_PROFILE the list logs in on its own, once. */
int mp_func_L39_0068C0E0(int screen, int event) {
    static int done, shown;
    const char* env = openrac_env_string("OPENRAC_RAC3_PROFILE");
    /* The list's first frames check the memory card (and leave the list when it changed), so
     * LOGIN waits until it has been shown for two seconds. */
    shown = GREF(int32_t, PROFILES_SUBSTATE) == 3 ? shown + 1 : 0;
    if (!done && env != NULL && *env != 0 && event == 0 && shown > 100) {
        done = 1;
        event = 6;
        fprintf(stderr, "[info] rac3: online: LOGIN as OPENRAC_RAC3_PROFILE's profile\n");
    }
    return mp_func_L39_0068C0E0__game(screen, event);
}

/* The lobby's pump (0x00195410): every wait loop of the lobby screens calls it for a frame. Its hook
 * runs MediusUpdate only on some screens (the level's net update 0x001959F8), so the host lobby's
 * replies and the connect callback are delivered here too, as the console's library would have
 * delivered them by then. */
int mp_func_00195410(void) {
    medius_update();
    return mp_func_00195410__game();
}

/* DME's allocator and free hooks (0x00198608, 0x00198660, installed through 0x0014ADB8): newlib's
 * malloc within the budget the game gives DME (0xC4000 bytes), which the port's heap does not
 * give it. A heap of that size in the port's statics, first fit, with the sizes kept beside it. */
#define DME_HEAP 0xC4000u
#define DME_BLOCKS 1024

static struct {
    gaddr base;

    struct {
        uint32_t at, size;
        int used;
    } block[DME_BLOCKS];

    int count;
} dme_heap;

int mp_func_00198608(int size) {
    const uint32_t want = ((uint32_t)size + 15u) & ~15u;
    if (dme_heap.base == 0) {
        dme_heap.base = openrac_guest_static(DME_HEAP);
        dme_heap.block[0].at = 0;
        dme_heap.block[0].size = DME_HEAP;
        dme_heap.block[0].used = 0;
        dme_heap.count = 1;
    }
    for (int i = 0; i < dme_heap.count; i++) {
        if (dme_heap.block[i].used || dme_heap.block[i].size < want) {
            continue;
        }
        if (dme_heap.block[i].size > want && dme_heap.count < DME_BLOCKS) {
            memmove(
                &dme_heap.block[i + 2],
                &dme_heap.block[i + 1],
                sizeof dme_heap.block[0] * (size_t)(dme_heap.count - i - 1)
            );
            dme_heap.block[i + 1].at = dme_heap.block[i].at + want;
            dme_heap.block[i + 1].size = dme_heap.block[i].size - want;
            dme_heap.block[i + 1].used = 0;
            dme_heap.block[i].size = want;
            dme_heap.count++;
        }
        dme_heap.block[i].used = 1;
        return (int)(dme_heap.base + dme_heap.block[i].at);
    }
    fprintf(stderr, "[warning] rac3: online: DME's heap has no 0x%x bytes left\n", (unsigned)size);
    return 0;
}

int mp_func_00198660(int pointer) {
    for (int i = 0; pointer != 0 && i < dme_heap.count; i++) {
        if (dme_heap.base + dme_heap.block[i].at != (gaddr)pointer) {
            continue;
        }
        dme_heap.block[i].used = 0;
        /* The free block takes in a free neighbour on either side. */
        if (i + 1 < dme_heap.count && !dme_heap.block[i + 1].used) {
            dme_heap.block[i].size += dme_heap.block[i + 1].size;
            memmove(
                &dme_heap.block[i + 1],
                &dme_heap.block[i + 2],
                sizeof dme_heap.block[0] * (size_t)(dme_heap.count - i - 2)
            );
            dme_heap.count--;
        }
        if (i > 0 && !dme_heap.block[i - 1].used) {
            dme_heap.block[i - 1].size += dme_heap.block[i].size;
            memmove(
                &dme_heap.block[i],
                &dme_heap.block[i + 1],
                sizeof dme_heap.block[0] * (size_t)(dme_heap.count - i - 1)
            );
            dme_heap.count--;
        }
        break;
    }
    return 0;
}

/* ---- Net objects (the store above) ---- */

int mp_func_0016EE30(int type_out, int name, int size) {
    const int r = mp_func_0016EE30__game(type_out, name, size);
    const int32_t type = GREF(int32_t, type_out);
    if (r == 0 && type >= 0 && type < OBJ_TYPES) {
        obj_type[type].size = size;
    }
    return r;
}

int mp_func_0016EEB0(int type, int on_create, int on_update, int on_destroy) {
    if (type >= 0 && type < OBJ_TYPES) {
        obj_type[type].on_create = on_create;
        obj_type[type].on_update = on_update;
        obj_type[type].on_destroy = on_destroy;
    }
    return mp_func_0016EEB0__game(type, on_create, on_update, on_destroy);
}

/* create(&id, ..., t3 the initial data; on the caller's stack +0 the name, +8 the type). The id
 * carries the owner's client index, so the consoles' ids never meet. */
int mp_func_0016EC18(int id_out, int a1, int a2, int a3, int a4, int a5, int a6, int initial) {
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;
    (void)a6;
    const int32_t type = GREF(int32_t, openrac_guest_sp + 8u);
    const int me = room_index_of(srv.session) < 0 ? 0 : room_index_of(srv.session);
    const int32_t id = (int32_t)((uint32_t)me << 16 | (uint32_t)obj_next++);
    const int i = obj_new(id, type, me);
    if (i < 0) {
        fprintf(stderr, "[warning] rac3: online: net object of type %d not created\n", type);
        return 1;
    }
    if (initial != 0) {
        memcpy(G(objs[i].data), G(initial), (size_t)obj_type[type].size);
    }
    GREF(int32_t, id_out) = id;
    if (net_log()) {
        fprintf(
            stderr,
            "[info] rac3: online: net object %08x created (type %d, %d bytes)\n",
            (unsigned)id,
            type,
            obj_type[type].size
        );
    }
    obj_send(i, 0);
    return 0;
}

int mp_func_0016ED00(int record_out, int id) {
    const int i = obj_find(id);
    if (i < 0) {
        return 1;
    }
    GREF(int32_t, record_out) = (int32_t)objs[i].record;
    return 0;
}

int mp_func_0016ED68(int id) {
    const int i = obj_find(id);
    if (i < 0 || objs[i].owner != room_index_of(srv.session)) {
        return 1;
    }
    obj_send(i, 0);
    return 0;
}

int mp_func_0016EDD0(int id) {
    const int i = obj_find(id);
    if (i < 0) {
        return 1;
    }
    if (srv.fd != NET_INVALID && room.in && srv_room(16)) {
        srv_begin(PK_RELAY, -1);
        srv_i32(0);
        srv_u8(OBJ_GONE_KIND);
        srv_i32(id);
        srv_end();
    }
    objs[i] = objs[--obj_count];
    return 0;
}

int mp_func_0016EF40(int target, int id) {
    (void)target;
    (void)id;
    return 0;
}

/* Timer 0's count in ticks of 147456 a millisecond (0x0012DCB0), which the lobby's waits, list
 * refreshes (the game list's serial, 0x006D5AB8) and DME's clock divide by 0x24000: the port runs
 * no timer 0, so the host's clock, from the first call. */
unsigned long long mp_func_0012DCB0(void) {
    static int64_t start_ns;
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    const int64_t ns = (int64_t)now.tv_sec * 1000000000 + now.tv_nsec;
    if (start_ns == 0) {
        start_ns = ns - 1000000;
    }
    return (unsigned long long)((ns - start_ns) / 1000) * 147456ull / 1000ull;
}
