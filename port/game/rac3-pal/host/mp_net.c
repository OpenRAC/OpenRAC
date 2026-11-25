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
 *   members by the server's PACKET_RELAY packet; what comes back goes to DME's receive path
 *   (0x0014CE48), which runs the game's handlers.
 *
 * The servers: OPENRAC_SERVER=host[:port] names the hub (hub.openrac.dev:7177 when unset), which
 * logs the player in to its account (the game's login: a new name registers) and lists the game
 * servers (the game's city list). The city picked is the game server the lobby and the rooms run
 * on: a second connection, which proves the hub's login with the session token. A plain game
 * server named in OPENRAC_SERVER (its UDP offer has no FEATURE_SERVERS) is played on directly, as
 * before the hub, without accounts. OPENRAC_RAC3_NET_LOG=1 logs every lobby message and room
 * packet. */
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
#include "mp_protocol.h"
#include "mp_wide.h"
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
#define NET_LOBBY_HANDLE RAC3_MADDR(0x001A30DCu) /* the lobby connection's handle, 0 when none */
#define NET_PROFILES RAC3_MADDR(0x001FC6B0u)     /* the online profiles (8 x 0x1F0) */
#define NET_SERVER_IP (NET_BLOCK + 0x182u)       /* the server's IP string in use (32 bytes) */
#define NET_MASTER_IP (NET_BLOCK + 0x2A2u)       /* the master server's IP string */
#define NET_AUSTRALIA_IP (NET_BLOCK + 0x242u)    /* the second server's IP string */
#define NET_TRANSPORT_UP RAC3_MADDR(0x001A2A58u) /* SCE-RT's "network started" word */
#define NET_TRANSPORT_LOCAL_IP (NET_TRANSPORT_UP + 4u) /* SCE-RT's local address (low octet 1st) */

/* ---- The OpenRAC server's protocol (OpenRAC/Server README, "Wire format") ---- */

enum {
    PACKET_HELLO,
    PACKET_PARTY,
    PACKET_CHAT,
    PACKET_DATA,
    PACKET_ERROR,
    PACKET_TIME,
    PACKET_UDP,
    PACKET_RELAY,
    PACKET_REGIONS,
    PACKET_REGISTER,
    PACKET_UI,
    PACKET_HUB,
    PACKET_SERVERS,
    PACKET_ACCOUNT,
    PACKET_STATS
};

enum {
    PARTY_CREATE,
    PARTY_JOIN,
    PARTY_LEAVE,
    PARTY_INFO,
    PARTY_JOINED,
    PARTY_LEFT,
    PARTY_END,
    PARTY_FIND,
    PARTY_LIST,
    PARTY_HOST_CHANGED,
    PARTY_RESUME,
    PARTY_KICK,
    PARTY_KICKED,
    PARTY_ADVERT,
    PARTY_SLOTS
};

/* PACKET_UI's opcodes (OpenRAC/Server README, "Lobby screens"): the lobby screens' description. */
enum {
    UI_GET,
    UI_DESCRIPTION,
    UI_ACTION,
    UI_NOTICE
};

/* PACKET_SERVERS's opcodes: the hub's live list of game servers, every one then each change. */
enum {
    SERVERS_WATCH,
    SERVERS_ADDED,
    SERVERS_CHANGED,
    SERVERS_REMOVED
};

/* PACKET_ACCOUNT's opcodes: log in at the hub (a new name registers), its answer, the session's
 * token proved at a game server, and the session ended at the hub. */
enum {
    ACCOUNT_LOGIN,
    ACCOUNT_LOGGED_IN,
    ACCOUNT_TOKEN,
    ACCOUNT_LOGOUT
};

/* PACKET_STATS's opcodes (between a client and the hub): the game's own ladder rows and career
 * bytes, which the hub keeps per game family without reading them. Each answers one Medius request
 * of the game, one to one (OpenRAC/Server README, "Stats and ladders"). */
enum {
    STATS_TOTALS,
    STATS_LADDER_WRITE,
    STATS_LADDER_READ,
    STATS_LADDER_PAGE,
    STATS_LADDER_POSITION,
    STATS_CAREER_WRITE,
    STATS_CAREER_READ
};

/* PACKET_ERROR's reasons. */
enum {
    ERROR_PARTY_NOT_FOUND,
    ERROR_PARTY_FULL,
    ERROR_ALREADY_IN_PARTY,
    ERROR_NOT_IN_PARTY,
    ERROR_NOT_HOST,
    ERROR_RESUME_REFUSED,
    ERROR_OTHER_GAME,
    ERROR_REGION_KEY,
    ERROR_HUB_FULL,
    HUB_ERROR_WRONG_PASSWORD,
    ERROR_INVALID_ACCOUNT,
    ERROR_SESSION_INVALID,
    ERROR_TRY_LATER,
    ERROR_NOT_LOGGED_IN
};

#define LIST_ADVERTS 1
#define FEATURE_REGIONS 16 /* the UDP offer's bit: the server answers PACKET_REGIONS */
#define FEATURE_UI 32      /* the UDP offer's bit: the server answers PACKET_UI */
int rac3_mp_ui_describe(const char* json, size_t size); /* mp_ui.c */
void rac3_mp_ui_notice(const char* text);               /* mp_ui.c */
#define FEATURE_SERVERS 64 /* the UDP offer's bit: a hub (log in, the game servers' list) */
#define HUB_DEFAULT_HOST "hub.openrac.dev"
#define HUB_DEFAULT_PORT "7177"
/* A game server that did not take the connection is tried again after this long. */
#define SERVER_RETRY_MS 10000
#define LADDER_VALUES 100     /* a ladder row's values (LOBBY_LADDER_STATS_WRITE_REQUEST's Stats) */
#define CAREER_BYTES 256      /* the career stats (LOBBY_CAREER_STATS_UPLOAD_REQUEST's Stats) */
#define LADDER_PAGE_MAX 32    /* the hub's longest ladder page */
#define STATS_WAITS 16        /* the game's stats requests waiting for the hub */
#define STATS_GIVE_UP_MS 4500 /* a request the hub has not answered: failed (the game waits 5 s) */
/* The stats requests' share of the hub's request budget (HUB_REQUEST_BURST 10, a few left for the
 * rest), and its refill a second (HUB_REQUEST_PER_SECOND). */
#define STATS_BURST 8
#define STATS_PER_SECOND 2
#define CLIENT_SLOTS 2 /* PACKET_HELLO's client bit: send PARTY_SLOTS (the room's stable slots) */
#define CLIENT_KEEPS_ALIVE                                                                         \
    4                      /* PACKET_HELLO's client bit: this client sends PACKET_TIME every 2 s */
#define KEEP_ALIVE_MS 2000 /* how often: the server drops a client silent for 30 s */
/* The hub drops no quiet client; this keeps routers' state, within its request budget. */
#define HUB_KEEP_ALIVE_MS 30000
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
#define M_ACCOUNT_NOT_FOUND (-998) /* the login's message 0xF: the hub's ERROR_INVALID_ACCOUNT */
#define M_SERVER_ERROR (-991)      /* message 1: the hub's ERROR_TRY_LATER */
#define M_INVALID_PASSWORD (-978)  /* message 0xA: the hub's ERROR_WRONG_PASSWORD */
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
    {1, 0x03, 1, 0x04, 0x030, 0}, /* LOBBY_SESSION_OPEN_REQUEST */
    {1, 0x05, 1, 0x06, 0x01C, 0}, /* LOBBY_SESSION_CLOSE_REQUEST */
    {1, 0x07, 1, 0x08, 0x0C4, 0}, /* LOBBY_LOGIN_REQUEST */
    {1, 0x11, 1, 0x12, 0x01C, 0}, /* LOBBY_CAREER_STATS_UPLOAD_REQUEST */
    {1, 0x15, 1, 0x16, 0x01C, 0}, /* LOBBY_LOGOUT_REQUEST */
    {1, 0x17, 1, 0x18, 0x020, 0}, /* LOBBY_ACCOUNT_ID_REQUEST */
    {1, 0x1B, 1, 0x1C, 0x030, 0}, /* LOBBY_KEEP_ALIVE_REQUEST */
    {1, 0x25, 1, 0x26, 0x0B8, 0}, /* LOBBY_CHANNEL_JOIN_REQUEST */
    {1, 0x2F, 1, 0x30, 0x148, 1}, /* LOBBY_GAME_PLAYER_LIST_REQUEST */
    {1, 0x31, 1, 0x32, 0x148, 0}, /* LOBBY_ACCOUNT_INFO_REQUEST */
    {1, 0x33, 1, 0x34, 0x18C, 0}, /* LOBBY_GAME_INFO_REQUEST */
    {1, 0x35, 1, 0x36, 0x064, 0}, /* LOBBY_CHANNEL_INFO_REQUEST */
    {1, 0x39, 1, 0x3A, 0x070, 0}, /* LOBBY_PLAYER_FIND_REQUEST */
    {1, 0x3F, 1, 0x40, 0x01C, 0}, /* LOBBY_BUDDY_ADD_REQUEST */
    {1, 0x41, 1, 0x42, 0x01C, 0}, /* LOBBY_BUDDY_REMOVE_REQUEST */
    {1, 0x47, 1, 0x48, 0x120, 1}, /* LOBBY_LICENCE_TEXT_REQUEST (EndOfText) */
    {1, 0x4B, 1, 0x4D, 0x40C, 1}, /* LOBBY_NEWS_REQUEST */
    {1, 0x4C, 1, 0x4D, 0x40C, 1}, /* LOBBY_ALL_NEWS_REQUEST */
    {1, 0x4E, 1, 0x4F, 0x01C, 0}, /* LOBBY_GAME_LIST_FILTER_ADD_REQUEST */
    {1, 0x50, 1, 0x51, 0x01C, 0}, /* LOBBY_GAME_LIST_FILTER_REMOVE_REQUEST */
    {1, 0x54, 1, 0x55, 0x020, 0}, /* LOBBY_CLAN_CREATE_REQUEST */
    {1, 0x56, 1, 0x57, 0x01C, 0}, /* LOBBY_CLAN_DISBAND_REQUEST */
    {1, 0x58, 1, 0x59, 0x168, 0}, /* LOBBY_CLAN_BY_ID_REQUEST */
    {1, 0x5A, 1, 0x5B, 0x148, 0}, /* LOBBY_CLAN_BY_NAME_REQUEST */
    {1, 0x5C, 1, 0x5D, 0x01C, 0}, /* LOBBY_CLAN_LEADER_TRANSFER_REQUEST */
    {1, 0x60, 1, 0x61, 0x01C, 0}, /* LOBBY_CLAN_MEMBER_REMOVE_REQUEST */
    {1, 0x62, 1, 0x63, 0x01C, 0}, /* LOBBY_CLAN_INVITE_REQUEST */
    {1, 0x64, 1, 0x65, 0x118, 1}, /* LOBBY_CLAN_INVITATION_LIST_REQUEST */
    {1, 0x66, 1, 0x67, 0x01C, 0}, /* LOBBY_CLAN_INVITATION_REPLY_REQUEST */
    {1, 0x68, 1, 0x69, 0x01C, 0}, /* LOBBY_CLAN_INVITATION_REVOKE_REQUEST */
    {1, 0x6A, 1, 0x6B, 0x01C, 0}, /* LOBBY_CLAN_CHALLENGE_REQUEST */
    {1, 0x6C, 1, 0x6D, 0x0EC, 1}, /* LOBBY_CLAN_MESSAGE_LIST_REQUEST */
    {1, 0x6E, 1, 0x6F, 0x01C, 0}, /* LOBBY_CLAN_MESSAGE_POST_REQUEST */
    {1, 0x70, 1, 0x71, 0x01C, 0}, /* LOBBY_CLAN_MESSAGE_EDIT_REQUEST */
    {1, 0x72, 1, 0x73, 0x01C, 0}, /* LOBBY_CLAN_MESSAGE_DELETE_REQUEST */
    {1, 0x74, 1, 0x75, 0x01C, 0}, /* LOBBY_CLAN_CHALLENGE_REPLY_REQUEST */
    {1, 0x76, 1, 0x77, 0x01C, 0}, /* LOBBY_CLAN_CHALLENGE_REVOKE_REQUEST */
    {1, 0x78, 1, 0x79, 0x028, 1}, /* LOBBY_CLAN_CHALLENGE_HISTORY_REQUEST */
    {1, 0x7A, 1, 0x7B, 0x114, 1}, /* LOBBY_CLAN_INVITATIONS_SENT_REQUEST */
    {1, 0x7C, 1, 0x7D, 0x170, 1}, /* LOBBY_MY_CLAN_REQUEST */
    {1, 0x7E, 1, 0x7F, 0x0F0, 1}, /* LOBBY_CLAN_ALL_MESSAGES_REQUEST */
    {1, 0x82, 1, 0x83, 0x1C4, 1}, /* LOBBY_CLAN_CHALLENGE_LIST_REQUEST */
    {1, 0x84, 1, 0x85, 0x01C, 0}, /* LOBBY_CLAN_STATS_UPLOAD_REQUEST */
    {1, 0x88, 1, 0x89, 0x028, 0}, /* LOBBY_WORLD_SECURITY_REQUEST */
    {1, 0x8C, 1, 0x8D, 0x064, 1}, /* LOBBY_CITY_LIST_REQUEST */
    {1, 0x96, 1, 0x97, 0x01C, 0}, /* LOBBY_CHAT_TOGGLE_REQUEST */
    {1, 0x98, 1, 0x99, 0x05C, 0}, /* LOBBY_CHAT_TEXT_CHECK_REQUEST */
    {1, 0xA3, 1, 0xA4, 0x01C, 0}, /* LOBBY_LOCALE_SET_REQUEST */
    {1, 0xA5, 1, 0xA6, 0x0C8, 0}, /* LOBBY_FILE_CREATE_REQUEST */
    {1, 0xA7, 1, 0xA9, 0x028, 0}, /* LOBBY_FILE_UPLOAD_REQUEST */
    {1, 0xAA, 1, 0xAB, 0x0C8, 0}, /* LOBBY_FILE_CLOSE_REQUEST */
    {1, 0xAC, 1, 0xAD, 0x1FC, 0}, /* LOBBY_FILE_DOWNLOAD_REQUEST */
    {1, 0xC0, 1, 0xC1, 0x048, 1}, /* LOBBY_IGNORE_LIST_REQUEST */
    {1, 0xC2, 1, 0xC3, 0x01C, 0}, /* LOBBY_IGNORE_ADD_REQUEST */
    {1, 0xC4, 1, 0xC5, 0x01C, 0}, /* LOBBY_IGNORE_REMOVE_REQUEST */
    {1, 0xD2, 1, 0xD3, 0x048, 1}, /* LOBBY_CLAN_LADDER_REQUEST */
    {1, 0xD6, 1, 0xD7, 0x0D0, 1}, /* LOBBY_BUDDY_LIST_REQUEST */
    {1, 0xDA, 1, 0xDB, 0x1DC, 1}, /* LOBBY_CLAN_MEMBER_LIST_REQUEST */
    {1, 0xDC, 1, 0xDD, 0x0D0, 1}, /* LOBBY_CHANNEL_PLAYER_LIST_REQUEST */
    {1, 0xEA, 1, 0xEB, 0x190, 1}, /* LOBBY_GAME_LIST_REQUEST */
    {1, 0xF1, 1, 0xF2, 0x024, 0}, /* LOBBY_LADDER_POSITION_REQUEST */
    {1, 0xF3, 1, 0x24, 0x0BC, 0}, /* LOBBY_GAME_JOIN_REQUEST */
    {1, 0xF4, 1, 0x1E, 0x020, 0}, /* LOBBY_GAME_CREATE_REQUEST */
    {4, 0x0B, 1, 0xCF, 0x01C, 0}, /* LOBBY_LADDER_STATS_WRITE_REQUEST */
    {4, 0x0D, 4, 0x0E, 0x1B0, 0}, /* LOBBY_LADDER_STATS_READ_REQUEST */
    {4, 0x0F, 1, 0xF0, 0x1D8, 1}, /* LOBBY_LADDER_PAGE_REQUEST */
    {4, 0x12, 4, 0x13, 0x034, 0}, /* LOBBY_CHANNEL_FILTER_SET_REQUEST */
    {4, 0x14, 1, 0x22, 0x020, 0}, /* LOBBY_CHANNEL_CREATE_REQUEST */
    {4, 0x15, 1, 0xED, 0x080, 1}, /* LOBBY_CHANNEL_LIST_REQUEST */
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

/* A connection to the hub or to a game server. */
typedef struct {
    net_socket fd;
    int state; /* 0 not tried, 1 connected and greeted, 2 hello sent, -1 failed */
    int32_t session;
    int features; /* the UDP offer's bits: which requests the server takes */
    int64_t alive_ms; /* when PACKET_TIME last went */
    uint8_t in[LINK_IN];
    size_t in_len;
    uint8_t out[LINK_OUT];
    size_t out_len, frame;
    int paced;         /* frames go out within the hub's request budget (HUB_REQUEST_BURST) */
    int tokens;        /* frames that may go now */
    int64_t tokens_ms; /* when the last token was added */
} net_link;

/* The hub takes 10 frames at once from a player, then 2 a second, and drops what is over
 * (Server HUB_REQUEST_BURST, HUB_REQUEST_PER_SECOND): the hub link keeps under that, holding a
 * frame over budget until it may go. The game's login sends a dozen at once (stats, the city
 * list), and a dropped one is retried by the game only after its own timeout. */
#define HUB_REQUEST_BURST 8
#define HUB_REQUEST_MS 550 /* one more frame may go every this many ms */

/* The game server (the lobby's games, the rooms) and the hub (the login, the city list). */
static net_link srv = {.fd = NET_INVALID}, hub = {.fd = NET_INVALID, .paced = 1};

/* The link the frame being written goes to: the game server's, unless hub_begin began it. */
static net_link* tx = &srv;

/* The host's clock in milliseconds, for PACKET_TIME and the room's clock. */
static int64_t host_ms(void) {
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

/* The server's clock less the host's, from the last PACKET_TIME answer (half its round trip
 * added): every console of a room reads the same room clock from it. */
static struct {
    int64_t offset;
    int known;
} server_clock;

static void srv_begin(int id, int opcode) {
    tx->frame = tx->out_len;
    tx->out_len += 4;
    tx->out[tx->out_len++] = (uint8_t)id;
    if (opcode >= 0) {
        tx->out[tx->out_len++] = (uint8_t)opcode;
    }
}

/* A frame for the hub; srv_end sends it, and the next frame is the game server's again. */
static void hub_begin(int id, int opcode) {
    tx = &hub;
    srv_begin(id, opcode);
}

static void srv_u8(int v) {
    tx->out[tx->out_len++] = (uint8_t)v;
}

static void srv_i32(int32_t v) {
    tx->out[tx->out_len++] = (uint8_t)(v >> 24);
    tx->out[tx->out_len++] = (uint8_t)(v >> 16);
    tx->out[tx->out_len++] = (uint8_t)(v >> 8);
    tx->out[tx->out_len++] = (uint8_t)v;
}

static void srv_bytes(const void* p, size_t n) {
    memcpy(tx->out + tx->out_len, p, n);
    tx->out_len += n;
}

/* A string: its length as a big-endian short (-1 for an empty one), then its bytes. */
static void srv_text(const char* s) {
    const size_t n = strlen(s);
    const uint16_t len = n == 0 ? 0xFFFFu : (uint16_t)n;
    tx->out[tx->out_len++] = (uint8_t)(len >> 8);
    tx->out[tx->out_len++] = (uint8_t)len;
    srv_bytes(s, n);
}

static void link_close(net_link* l) {
    if (l->fd != NET_INVALID) {
        net_close(l->fd);
    }
    l->fd = NET_INVALID;
    l->state = -1;
    l->out_len = 0;
    l->features = 0;
    if (l == &srv) {
        server_clock.known = 0; /* the next game server's clock is its own */
    }
}

static void link_flush(net_link* l);

/* Ends the frame begun by srv_begin (its length first) and sends what is queued. */
static void srv_end(void) {
    net_link* l = tx;
    tx = &srv;
    const uint32_t len = (uint32_t)(l->out_len - l->frame - 4);
    uint8_t* p = l->out + l->frame;
    p[0] = (uint8_t)(len >> 24);
    p[1] = (uint8_t)(len >> 16);
    p[2] = (uint8_t)(len >> 8);
    p[3] = (uint8_t)len;
    if (l->fd == NET_INVALID) {
        l->out_len = 0;
        return;
    }
    link_flush(l);
}

/* Sends what L has queued: all of it, or on a paced link the whole frames its budget allows. */
static void link_flush(net_link* l) {
    if (l->fd == NET_INVALID || l->out_len == 0) {
        return;
    }
    size_t end = l->out_len;
    if (l->paced) {
        const int64_t now = host_ms();
        if (l->tokens_ms == 0) {
            l->tokens = HUB_REQUEST_BURST;
            l->tokens_ms = now;
        }
        while (l->tokens < HUB_REQUEST_BURST && now - l->tokens_ms >= HUB_REQUEST_MS) {
            l->tokens++;
            l->tokens_ms += HUB_REQUEST_MS;
        }
        if (l->tokens == HUB_REQUEST_BURST) {
            l->tokens_ms = now;
        }
        end = 0;
        while (l->tokens > 0 && l->out_len - end >= 4) {
            const uint8_t* p = l->out + end;
            end += 4 + ((size_t)p[0] << 24 | (size_t)p[1] << 16 | (size_t)p[2] << 8 | p[3]);
            l->tokens--;
        }
        if (end == 0) {
            return;
        }
    }
    size_t sent = 0;
    while (sent < end) {
        const long n = (long)send(l->fd, (const char*)l->out + sent, (int)(end - sent), 0);
        if (n <= 0) {
#ifndef _WIN32
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                continue;
            }
#endif
            fprintf(
                stderr, "[warning] rac3: online: the %s went away\n", l == &hub ? "hub" : "server"
            );
            link_close(l);
            break;
        }
        sent += (size_t)n;
    }
    if (l->fd == NET_INVALID) {
        return;
    }
    memmove(l->out, l->out + end, l->out_len - end);
    l->out_len -= end;
}

/* Room for a frame of N bytes more in the send buffer. */
static int srv_room(size_t n) {
    return srv.out_len + n + 16 < LINK_OUT;
}

/* OPENRAC_SERVER's address: host:port, [v6]:port or [v6], a bare name or IPv4, or a bare IPv6
 * address (more than one colon and no brackets: no port). HOST and PORT keep their defaults for
 * what it leaves out. */
static void net_split_address(
    const char* text, char* host, size_t host_size, char* port, size_t port_size
) {
    const char* close = text[0] == '[' ? strchr(text, ']') : NULL;
    const char* colon = strrchr(text, ':');
    if (close != NULL) {
        snprintf(host, host_size, "%.*s", (int)(close - text - 1), text + 1);
        if (close[1] == ':') {
            snprintf(port, port_size, "%s", close + 2);
        }
    } else if (colon != NULL && strchr(text, ':') == colon) {
        snprintf(host, host_size, "%.*s", (int)(colon - text), text);
        snprintf(port, port_size, "%s", colon + 1);
    } else {
        snprintf(host, host_size, "%s", text);
    }
}

/* Connects L to HOST:PORT and says hello under NAME. The game thread waits for the TCP connection
 * (a local or LAN server answers at once), never for an answer. */
static int link_open(net_link* l, const char* host, const char* port, const char* name) {
    const char* what = l == &hub ? "hub" : "game server";
#ifdef _WIN32
    static int started;
    if (!started) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
        started = 1;
    }
#endif
    struct addrinfo hints, *found = NULL;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &found) != 0 || found == NULL) {
        fprintf(stderr, "[warning] rac3: online: cannot resolve the %s %s:%s\n", what, host, port);
        l->state = -1;
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
        fprintf(stderr, "[warning] rac3: online: no OpenRAC %s at %s:%s\n", what, host, port);
        l->state = -1;
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
    l->fd = fd;
    l->in_len = 0;
    l->out_len = 0;
    l->features = 0;
    l->tokens_ms = 0; /* a new connection: a full budget */
    l->state = 2;
    fprintf(
        stderr,
        "[info] rac3: online: connected to the OpenRAC %s at %s:%s as %s\n",
        what,
        host,
        port,
        name
    );
    tx = l;
    srv_begin(PACKET_HELLO, -1);
    srv_text(NET_VERSION);
    srv_text(name);
    srv_u8(CLIENT_SLOTS | CLIENT_KEEPS_ALIVE);
    srv_end();
    return 1;
}

/* A server action chosen on a native lobby screen (mp_ui.c): its key and value to the server. */
void rac3_mp_net_ui_action(const char* key, int value) {
    if (srv.state != 1) {
        return;
    }
    srv_begin(PACKET_UI, UI_ACTION);
    srv_text(key);
    srv_i32(value);
    srv_end();
}

/* ---- The login: the hub's account, then the game server picked from its list ---- */

static struct {
    char host[128], port[16]; /* the hub's address, which a listed server with no host shares */
    int direct; /* OPENRAC_SERVER names a game server, not a hub: played on as before the hub */
    char name[33], password[33];
    char token[80];   /* the hub's session token, empty when logged out */
    int32_t account;  /* the account's id, the game's AccountID */
    int region;       /* the region the game server link is to, -1 none */
    int picked;       /* the city the game picked (an index of lobby.regions), -1 none */
    int64_t tried_ms; /* when the game server was last tried */
} net_login = {.region = -1, .picked = -1};

/* Connects to the hub (OPENRAC_SERVER) and says hello under NAME; the login goes once its UDP offer
 * says it is a hub. */
static int hub_open(const char* name) {
    if (hub.fd != NET_INVALID) {
        return 1;
    }
    snprintf(net_login.host, sizeof net_login.host, "%s", HUB_DEFAULT_HOST);
    snprintf(net_login.port, sizeof net_login.port, "%s", HUB_DEFAULT_PORT);
    const char* env = openrac_env_string("OPENRAC_SERVER");
    if (env != NULL && *env != 0) {
        net_split_address(
            env, net_login.host, sizeof net_login.host, net_login.port, sizeof net_login.port
        );
    }
    net_login.direct = 0;
    return link_open(&hub, net_login.host, net_login.port, name);
}

/* A game server named directly in OPENRAC_SERVER (no hub): connected and greeted under NAME. */
static int srv_open(const char* name) {
    if (srv.fd != NET_INVALID) {
        return 1;
    }
    return link_open(&srv, net_login.host, net_login.port, name);
}

static void hub_send_login(void) {
    hub_begin(PACKET_ACCOUNT, ACCOUNT_LOGIN);
    srv_text(net_login.name);
    srv_text(net_login.password);
    srv_end();
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

/* The room's members: the server's rooms go up to 24 players. The game's own tables hold 8 clients
 * (net + 0x148[8], the remote players, the settings object's arrays: protocol.md 7.4); a room of
 * more than 8 (its game's MaxPlayers) has them widened to its size by host/mp_wide.c, and only
 * slots below GAME_CLIENTS are given to the game. */
#define ROOM_MAX 24
#define GAME_CLIENTS rac3_mp_wide_slots()

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
 * then the members as they joined, which PARTY_INFO lists and PARTY_JOINED appends). */
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
    uint8_t login[0x66]; /* the LOBBY_LOGIN_REQUEST waiting for the server's hello */
    int login_waiting;
    uint8_t list_req[0x1A];
    int list_waiting;
    uint8_t create_req[0xD4];
    int create_waiting;
    uint8_t join_req[0xC4];
    int join_waiting;
    listed_game games[32];
    int games_count;
    game_advert own; /* this console's game: LOBBY_GAME_CREATE_REQUEST, each LOBBY_GAME_REPORT */
    uint8_t locations_req[0x26];
    int locations_waiting;

    /* The game servers the hub lists, one city each: NAME its region code (or, from a game
     * server before the hub, its region's name), SERVER its own name (empty: none). */
    struct {
        int32_t id; /* the hub's id of the server, -1 from PACKET_REGIONS */
        char name[64];
        char server[64];
        char host[128];
        int32_t port, players;
    } regions[16];

    int regions_count;
    int watching;       /* SERVERS_WATCH sent on this hub link: the list stays current */
    int64_t watched_ms; /* when */
    int own_valid;
} lobby;

/* ---- What the city list shows: the game server's name and its region ---- */

/* The region codes game servers register with (Server REGION: every ISO 3166-1 alpha-2 code, then
 * EU and LOCAL, in the Server's Regions.java order), each with a short label (ISO alpha-3) and the
 * name the game shows, in plain ASCII for the game's font. The same list as the rac1/rac2
 * multiplayer mod's (openrac-mods multiplayer/src/regions.cpp). A code not listed is shown as it
 * came. */
static const struct {
    const char* code;
    const char* label;
    const char* name;
} NET_REGION_NAMES[] = {
    // clang-format off
    {"AD", "AND", "Andorra"},
    {"AE", "ARE", "United Arab Emirates"},
    {"AF", "AFG", "Afghanistan"},
    {"AG", "ATG", "Antigua and Barbuda"},
    {"AI", "AIA", "Anguilla"},
    {"AL", "ALB", "Albania"},
    {"AM", "ARM", "Armenia"},
    {"AO", "AGO", "Angola"},
    {"AQ", "ATA", "Antarctica"},
    {"AR", "ARG", "Argentina"},
    {"AS", "ASM", "American Samoa"},
    {"AT", "AUT", "Austria"},
    {"AU", "AUS", "Australia"},
    {"AW", "ABW", "Aruba"},
    {"AX", "ALA", "Aland Islands"},
    {"AZ", "AZE", "Azerbaijan"},
    {"BA", "BIH", "Bosnia and Herzegovina"},
    {"BB", "BRB", "Barbados"},
    {"BD", "BGD", "Bangladesh"},
    {"BE", "BEL", "Belgium"},
    {"BF", "BFA", "Burkina Faso"},
    {"BG", "BGR", "Bulgaria"},
    {"BH", "BHR", "Bahrain"},
    {"BI", "BDI", "Burundi"},
    {"BJ", "BEN", "Benin"},
    {"BL", "BLM", "St. Barthelemy"},
    {"BM", "BMU", "Bermuda"},
    {"BN", "BRN", "Brunei"},
    {"BO", "BOL", "Bolivia"},
    {"BQ", "BES", "Caribbean Netherlands"},
    {"BR", "BRA", "Brazil"},
    {"BS", "BHS", "Bahamas"},
    {"BT", "BTN", "Bhutan"},
    {"BV", "BVT", "Bouvet Island"},
    {"BW", "BWA", "Botswana"},
    {"BY", "BLR", "Belarus"},
    {"BZ", "BLZ", "Belize"},
    {"CA", "CAN", "Canada"},
    {"CC", "CCK", "Cocos (Keeling) Islands"},
    {"CD", "COD", "Congo - Kinshasa"},
    {"CF", "CAF", "Central African Republic"},
    {"CG", "COG", "Congo - Brazzaville"},
    {"CH", "CHE", "Switzerland"},
    {"CI", "CIV", "Cote d'Ivoire"},
    {"CK", "COK", "Cook Islands"},
    {"CL", "CHL", "Chile"},
    {"CM", "CMR", "Cameroon"},
    {"CN", "CHN", "China"},
    {"CO", "COL", "Colombia"},
    {"CR", "CRI", "Costa Rica"},
    {"CU", "CUB", "Cuba"},
    {"CV", "CPV", "Cape Verde"},
    {"CW", "CUW", "Curacao"},
    {"CX", "CXR", "Christmas Island"},
    {"CY", "CYP", "Cyprus"},
    {"CZ", "CZE", "Czechia"},
    {"DE", "DEU", "Germany"},
    {"DJ", "DJI", "Djibouti"},
    {"DK", "DNK", "Denmark"},
    {"DM", "DMA", "Dominica"},
    {"DO", "DOM", "Dominican Republic"},
    {"DZ", "DZA", "Algeria"},
    {"EC", "ECU", "Ecuador"},
    {"EE", "EST", "Estonia"},
    {"EG", "EGY", "Egypt"},
    {"EH", "ESH", "Western Sahara"},
    {"ER", "ERI", "Eritrea"},
    {"ES", "ESP", "Spain"},
    {"ET", "ETH", "Ethiopia"},
    {"FI", "FIN", "Finland"},
    {"FJ", "FJI", "Fiji"},
    {"FK", "FLK", "Falkland Islands"},
    {"FM", "FSM", "Micronesia"},
    {"FO", "FRO", "Faroe Islands"},
    {"FR", "FRA", "France"},
    {"GA", "GAB", "Gabon"},
    {"GB", "GBR", "United Kingdom"},
    {"GD", "GRD", "Grenada"},
    {"GE", "GEO", "Georgia"},
    {"GF", "GUF", "French Guiana"},
    {"GG", "GGY", "Guernsey"},
    {"GH", "GHA", "Ghana"},
    {"GI", "GIB", "Gibraltar"},
    {"GL", "GRL", "Greenland"},
    {"GM", "GMB", "Gambia"},
    {"GN", "GIN", "Guinea"},
    {"GP", "GLP", "Guadeloupe"},
    {"GQ", "GNQ", "Equatorial Guinea"},
    {"GR", "GRC", "Greece"},
    {"GS", "SGS", "South Georgia and South Sandwich Islands"},
    {"GT", "GTM", "Guatemala"},
    {"GU", "GUM", "Guam"},
    {"GW", "GNB", "Guinea-Bissau"},
    {"GY", "GUY", "Guyana"},
    {"HK", "HKG", "Hong Kong SAR China"},
    {"HM", "HMD", "Heard and McDonald Islands"},
    {"HN", "HND", "Honduras"},
    {"HR", "HRV", "Croatia"},
    {"HT", "HTI", "Haiti"},
    {"HU", "HUN", "Hungary"},
    {"ID", "IDN", "Indonesia"},
    {"IE", "IRL", "Ireland"},
    {"IL", "ISR", "Israel"},
    {"IM", "IMN", "Isle of Man"},
    {"IN", "IND", "India"},
    {"IO", "IOT", "British Indian Ocean Territory"},
    {"IQ", "IRQ", "Iraq"},
    {"IR", "IRN", "Iran"},
    {"IS", "ISL", "Iceland"},
    {"IT", "ITA", "Italy"},
    {"JE", "JEY", "Jersey"},
    {"JM", "JAM", "Jamaica"},
    {"JO", "JOR", "Jordan"},
    {"JP", "JPN", "Japan"},
    {"KE", "KEN", "Kenya"},
    {"KG", "KGZ", "Kyrgyzstan"},
    {"KH", "KHM", "Cambodia"},
    {"KI", "KIR", "Kiribati"},
    {"KM", "COM", "Comoros"},
    {"KN", "KNA", "St. Kitts and Nevis"},
    {"KP", "PRK", "North Korea"},
    {"KR", "KOR", "South Korea"},
    {"KW", "KWT", "Kuwait"},
    {"KY", "CYM", "Cayman Islands"},
    {"KZ", "KAZ", "Kazakhstan"},
    {"LA", "LAO", "Laos"},
    {"LB", "LBN", "Lebanon"},
    {"LC", "LCA", "St. Lucia"},
    {"LI", "LIE", "Liechtenstein"},
    {"LK", "LKA", "Sri Lanka"},
    {"LR", "LBR", "Liberia"},
    {"LS", "LSO", "Lesotho"},
    {"LT", "LTU", "Lithuania"},
    {"LU", "LUX", "Luxembourg"},
    {"LV", "LVA", "Latvia"},
    {"LY", "LBY", "Libya"},
    {"MA", "MAR", "Morocco"},
    {"MC", "MCO", "Monaco"},
    {"MD", "MDA", "Moldova"},
    {"ME", "MNE", "Montenegro"},
    {"MF", "MAF", "St. Martin"},
    {"MG", "MDG", "Madagascar"},
    {"MH", "MHL", "Marshall Islands"},
    {"MK", "MKD", "North Macedonia"},
    {"ML", "MLI", "Mali"},
    {"MM", "MMR", "Myanmar (Burma)"},
    {"MN", "MNG", "Mongolia"},
    {"MO", "MAC", "Macao SAR China"},
    {"MP", "MNP", "Northern Mariana Islands"},
    {"MQ", "MTQ", "Martinique"},
    {"MR", "MRT", "Mauritania"},
    {"MS", "MSR", "Montserrat"},
    {"MT", "MLT", "Malta"},
    {"MU", "MUS", "Mauritius"},
    {"MV", "MDV", "Maldives"},
    {"MW", "MWI", "Malawi"},
    {"MX", "MEX", "Mexico"},
    {"MY", "MYS", "Malaysia"},
    {"MZ", "MOZ", "Mozambique"},
    {"NA", "NAM", "Namibia"},
    {"NC", "NCL", "New Caledonia"},
    {"NE", "NER", "Niger"},
    {"NF", "NFK", "Norfolk Island"},
    {"NG", "NGA", "Nigeria"},
    {"NI", "NIC", "Nicaragua"},
    {"NL", "NLD", "Netherlands"},
    {"NO", "NOR", "Norway"},
    {"NP", "NPL", "Nepal"},
    {"NR", "NRU", "Nauru"},
    {"NU", "NIU", "Niue"},
    {"NZ", "NZL", "New Zealand"},
    {"OM", "OMN", "Oman"},
    {"PA", "PAN", "Panama"},
    {"PE", "PER", "Peru"},
    {"PF", "PYF", "French Polynesia"},
    {"PG", "PNG", "Papua New Guinea"},
    {"PH", "PHL", "Philippines"},
    {"PK", "PAK", "Pakistan"},
    {"PL", "POL", "Poland"},
    {"PM", "SPM", "St. Pierre and Miquelon"},
    {"PN", "PCN", "Pitcairn Islands"},
    {"PR", "PRI", "Puerto Rico"},
    {"PS", "PSE", "Palestinian Territories"},
    {"PT", "PRT", "Portugal"},
    {"PW", "PLW", "Palau"},
    {"PY", "PRY", "Paraguay"},
    {"QA", "QAT", "Qatar"},
    {"RE", "REU", "Reunion"},
    {"RO", "ROU", "Romania"},
    {"RS", "SRB", "Serbia"},
    {"RU", "RUS", "Russia"},
    {"RW", "RWA", "Rwanda"},
    {"SA", "SAU", "Saudi Arabia"},
    {"SB", "SLB", "Solomon Islands"},
    {"SC", "SYC", "Seychelles"},
    {"SD", "SDN", "Sudan"},
    {"SE", "SWE", "Sweden"},
    {"SG", "SGP", "Singapore"},
    {"SH", "SHN", "St. Helena"},
    {"SI", "SVN", "Slovenia"},
    {"SJ", "SJM", "Svalbard and Jan Mayen"},
    {"SK", "SVK", "Slovakia"},
    {"SL", "SLE", "Sierra Leone"},
    {"SM", "SMR", "San Marino"},
    {"SN", "SEN", "Senegal"},
    {"SO", "SOM", "Somalia"},
    {"SR", "SUR", "Suriname"},
    {"SS", "SSD", "South Sudan"},
    {"ST", "STP", "Sao Tome and Principe"},
    {"SV", "SLV", "El Salvador"},
    {"SX", "SXM", "Sint Maarten"},
    {"SY", "SYR", "Syria"},
    {"SZ", "SWZ", "Eswatini"},
    {"TC", "TCA", "Turks and Caicos Islands"},
    {"TD", "TCD", "Chad"},
    {"TF", "ATF", "French Southern Territories"},
    {"TG", "TGO", "Togo"},
    {"TH", "THA", "Thailand"},
    {"TJ", "TJK", "Tajikistan"},
    {"TK", "TKL", "Tokelau"},
    {"TL", "TLS", "Timor-Leste"},
    {"TM", "TKM", "Turkmenistan"},
    {"TN", "TUN", "Tunisia"},
    {"TO", "TON", "Tonga"},
    {"TR", "TUR", "Turkiye"},
    {"TT", "TTO", "Trinidad and Tobago"},
    {"TV", "TUV", "Tuvalu"},
    {"TW", "TWN", "Taiwan"},
    {"TZ", "TZA", "Tanzania"},
    {"UA", "UKR", "Ukraine"},
    {"UG", "UGA", "Uganda"},
    {"UM", "UMI", "U.S. Outlying Islands"},
    {"US", "USA", "United States"},
    {"UY", "URY", "Uruguay"},
    {"UZ", "UZB", "Uzbekistan"},
    {"VA", "VAT", "Vatican City"},
    {"VC", "VCT", "St. Vincent and Grenadines"},
    {"VE", "VEN", "Venezuela"},
    {"VG", "VGB", "British Virgin Islands"},
    {"VI", "VIR", "U.S. Virgin Islands"},
    {"VN", "VNM", "Vietnam"},
    {"VU", "VUT", "Vanuatu"},
    {"WF", "WLF", "Wallis and Futuna"},
    {"WS", "WSM", "Samoa"},
    {"YE", "YEM", "Yemen"},
    {"YT", "MYT", "Mayotte"},
    {"ZA", "ZAF", "South Africa"},
    {"ZM", "ZMB", "Zambia"},
    {"ZW", "ZWE", "Zimbabwe"},
    {"EU", "EU", "Europe"},
    {"LOCAL", "LAN", "Local network"},
    // clang-format on
};

/* The longest city text, in characters: what one line of the city select fits (a longer one wraps
 * onto the next city's line), and what the licence text and the log give it. */
#define NET_CITY_TEXT 23
#define NET_CITY_LONG_TEXT 47

static int net_same_text(const char* a, const char* b) {
    for (; *a != 0 && *b != 0; a++, b++) {
        const int x = *a >= 'a' && *a <= 'z' ? *a - 32 : *a;
        const int y = *b >= 'a' && *b <= 'z' ? *b - 32 : *b;
        if (x != y) {
            return 0;
        }
    }
    return *a == *b;
}

/* Region CODE's row of NET_REGION_NAMES, -1 when it is not listed. */
static int net_region(const char* code) {
    for (size_t i = 0; i < sizeof NET_REGION_NAMES / sizeof NET_REGION_NAMES[0]; i++) {
        if (net_same_text(code, NET_REGION_NAMES[i].code)) {
            return (int)i;
        }
    }
    return -1;
}

/* City K as the game shows it, at most MAX characters (NET_CITY_TEXT or NET_CITY_LONG_TEXT):
 * "<server name> (<region>)", or the region alone when the server has no name. A long one gives
 * the region's short label ("USA") in place of its name, then shortens the server's name. */
static void net_city_text(int k, int max, char* out, size_t size) {
    const char* code = lobby.regions[k].name;
    const int row = net_region(code);
    const char* region = row >= 0 ? NET_REGION_NAMES[row].name : code;
    const char* label = row >= 0 ? NET_REGION_NAMES[row].label : code;
    const char* server = lobby.regions[k].server;
    const int length = (int)strlen(server);
    char text[NET_CITY_LONG_TEXT + 1];
    if (server[0] == 0) {
        snprintf(text, sizeof text, "%.*s", max, (int)strlen(region) <= max ? region : label);
    } else if (length + (int)strlen(region) + 3 <= max) {
        snprintf(text, sizeof text, "%s (%s)", server, region);
    } else {
        const int room = max - (int)strlen(label) - 3;
        if (room < 4) {
            snprintf(text, sizeof text, "%.*s", max, server);
        } else {
            snprintf(text, sizeof text, "%.*s (%s)", room, server, label);
        }
    }
    snprintf(out, size, "%s", text);
}

/* Logged in at a hub: the game server comes from its list. */
static int hub_logged_in(void) {
    return !net_login.direct && net_login.token[0] != 0;
}

/* The game server of the city picked (the first listed when none is), connected once logged in at
 * the hub: hello, then the hub's session token (ACCOUNT_TOKEN), which it checks with the hub. The
 * lobby's requests that need the game server call this first. */
static void srv_ensure(void) {
    if (!hub_logged_in() || srv.fd != NET_INVALID || lobby.regions_count == 0) {
        return;
    }
    const int64_t now = host_ms();
    if (net_login.tried_ms != 0 && now - net_login.tried_ms < SERVER_RETRY_MS) {
        return;
    }
    net_login.tried_ms = now;
    const int k =
        net_login.picked >= 0 && net_login.picked < lobby.regions_count ? net_login.picked : 0;
    char port[16];
    snprintf(port, sizeof port, "%d", (int)lobby.regions[k].port);
    const char* host = lobby.regions[k].host[0] != 0 ? lobby.regions[k].host : net_login.host;
    char city[NET_CITY_LONG_TEXT + 1];
    net_city_text(k, NET_CITY_LONG_TEXT, city, sizeof city);
    fprintf(stderr, "[info] rac3: online: city %s\n", city);
    if (!link_open(&srv, host, port, net_login.name)) {
        return;
    }
    net_login.region = k;
    srv_begin(PACKET_ACCOUNT, ACCOUNT_TOKEN);
    srv_text(net_login.token);
    srv_end();
}

/* The city the game picked (LOBBY_CHANNEL_FILTER_SET_REQUEST's city bits): its game server from
 * now on. Another city's link is closed, unless this console is in a game there. */
static void lobby_pick_city(const uint8_t* req, size_t size) {
    if (size < 0x2C || !hub_logged_in() || get32(req + 0x28) != 1) {
        return;
    }
    const uint32_t bits = (uint32_t)get32(req + 0x1C);
    if (bits == 0 || bits == 0xFFFFFFFFu) {
        return;
    }
    int k = 0;
    while (!(bits >> k & 1u)) {
        k++;
    }
    if (k >= lobby.regions_count) {
        return;
    }
    net_login.picked = k;
    if (srv.fd != NET_INVALID && net_login.region != k && !room.in) {
        link_close(&srv);
        lobby.games_count = 0;
        net_login.tried_ms = 0;
    }
    srv_ensure();
}

static void stats_forget(void);

/* The session ended at the hub (the game logs out): the game server's link goes with it, unless
 * this console is in a game there. */
static void hub_logout(void) {
    if (net_login.token[0] == 0) {
        return;
    }
    if (hub.fd != NET_INVALID) {
        hub_begin(PACKET_ACCOUNT, ACCOUNT_LOGOUT);
        srv_text(net_login.token);
        srv_end();
    }
    net_login.token[0] = 0;
    stats_forget();
    if (!net_login.direct && !room.in) {
        link_close(&srv);
        net_login.region = -1;
        net_login.tried_ms = 0;
    }
    fprintf(stderr, "[info] rac3: online: logged out\n");
}

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

/* The most players a game this console hosts takes: the create page's MaxPlayers, or
 * OPENRAC_RAC3_MAX_PLAYERS (9-24: a room past the game's 8, mp_wide.c) where the tables can grow. */
static int32_t net_own_game_max_players; /* this console's game's own MaxPlayers, as it set it */

static int32_t net_host_max_players(int32_t game) {
    net_own_game_max_players = game;
    const char* e = openrac_env_string("OPENRAC_RAC3_MAX_PLAYERS");
    const int want = e != NULL ? atoi(e) : 0;
    if (want > 0 && rac3_mp_wide_available()) {
        return want > ROOM_MAX ? ROOM_MAX : want;
    }
    return game;
}

/* The CREATE page (level 39's screen 0x0069A630, built by 0x00699838): its number of players
 * (widget 3, a spinner the constructor makes with value 8, from 2 to 8) offers up to the room's
 * most (ROOM_MAX, or OPENRAC_RAC3_MAX_PLAYERS) online, where the tables can grow (mp_wide.c). The
 * game's own value stays 8 until the player turns it up; LOCAL PLAY keeps its 8. */
#define CREATE_WIDGETS 0x110u    /* the screen's widget pointers */
#define CREATE_PLAYERS_WIDGET 3u /* the number of players */
#define SPINNER_MAX 0x70u        /* a spinner's largest value (its constructor 0x00682840) */

#define SPINNER_VALUE 0x64u /* a spinner's value, then its smallest at +0x6C */
#define SPINNER_STEP 0x74u  /* its step; the players spinner counts from 2 by 1 */

static gaddr create_players; /* the page's players spinner, from its constructor */

int mp_func_L39_00699838(int screen) {
    const int r = mp_func_L39_00699838__game(screen);
    create_players = GREF(gaddr, (gaddr)screen + CREATE_WIDGETS + 4u * CREATE_PLAYERS_WIDGET);
    return r;
}

/* The page is built when the multiplayer menu opens, before the player logs in, and kept: the
 * spinner's largest value follows the console's state each time the lobby runs, the room's most
 * while connected, the game's own 8 otherwise. */
static void create_page_sync(void) {
    if (create_players == 0 || !rac3_mp_wide_available()
        || GREF(int32_t, create_players + SPINNER_STEP) != 1
        || GREF(int32_t, create_players + 0x6Cu) != 2) {
        return; /* not built yet, or the page's memory is the level's no longer */
    }
    int most = 8;
    if (lobby.connected && rac3_mp_wide_available()) {
        const char* e = openrac_env_string("OPENRAC_RAC3_MAX_PLAYERS");
        most = e != NULL && atoi(e) > 0 ? atoi(e) : ROOM_MAX;
        most = most < 8 ? 8 : most > ROOM_MAX ? ROOM_MAX : most;
    }
    if (GREF(int32_t, create_players + SPINNER_MAX) != most) {
        GREF(int32_t, create_players + SPINNER_MAX) = most;
        if (GREF(int32_t, create_players + SPINNER_VALUE) > most) {
            GREF(int32_t, create_players + SPINNER_VALUE) = most;
        }
    }
}

/* The settings of a game as LOBBY_GAME_INFO_REQUEST and the game list give them, from a
 * LOBBY_GAME_CREATE_REQUEST. */
static void advert_from_create(game_advert* a, const uint8_t* req) {
    memset(a, 0, sizeof *a);
    a->min_players = get32(req + 0x2C);
    a->max_players = net_host_max_players(get32(req + 0x30));
    a->level = get32(req + 0x34);
    memcpy(a->name, req + 0x38, 63);
    a->skill = get32(req + 0xB8);
    a->rules = get32(req + 0xBC);
    a->field1 = get32(req + 0xC0);
    a->field2 = get32(req + 0xC4);
    a->field3 = get32(req + 0xC8);
    a->host_type = get32(req + 0xCC);
}

/* ...and from the host's LOBBY_GAME_REPORT, which the host's own LOBBY_GAME_INFO_REQUEST poll must
 * see again. */
static void advert_from_report(game_advert* a, const uint8_t* rep) {
    memcpy(a->name, rep + 0x1C, 63);
    a->name[63] = 0;
    memcpy(a->stats, rep + 0x5C, 256);
    a->min_players = get32(rep + 0x15C);
    a->max_players = net_host_max_players(get32(rep + 0x160));
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
    srv_begin(PACKET_PARTY, PARTY_ADVERT);
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
    put32(reply + 0x1C, net_login.direct ? srv.session : net_login.account);
    put32(reply + 0x24, M_WORLD_LOBBY);
    lobby_connect_info(reply + 0x28, M_WORLD_LOBBY);
    lobby_push(r->reply_cls, r->reply_type, reply, r->size);
    lobby.login_waiting = 0;
}

/* LOBBY_CITY_LIST_ENTRY (0x64 bytes: +0x18 the location id, +0x1C its name, +0x5C the status, +0x60
 * EndOfList), one record per region; the city select lists them. */
static void lobby_reply_locations(void) {
    const lobby_reply* r = lobby_reply_of(1, 0x8C);
    uint8_t reply[0x64];
    for (int i = 0; i < lobby.regions_count; i++) {
        lobby_start(reply, r, lobby.locations_req, 0);
        put32(reply + 0x18, 40 + i); /* the city select keeps ids 40..99 only */
        char city[NET_CITY_TEXT + 1];
        net_city_text(i, NET_CITY_TEXT, city, sizeof city);
        put_text(reply + 0x1C, city, 64);
        put32(reply + 0x5C, M_SUCCESS);
        put32(reply + 0x60, i == lobby.regions_count - 1);
        lobby_push(r->reply_cls, r->reply_type, reply, r->size);
    }
    if (lobby.regions_count == 0) {
        lobby_start(reply, r, lobby.locations_req, 0);
        put32(reply + 0x5C, M_NO_RESULT);
        put32(reply + 0x60, 1);
        lobby_push(r->reply_cls, r->reply_type, reply, r->size);
    }
    lobby.locations_waiting = 0;
}

static void lobby_reply_list(void) {
    const lobby_reply* r = lobby_reply_of(1, 0xEA);
    uint8_t reply[0x190];
    int shown = 0;
    /* The request's PageSize (+0x18): the list drops records past it (L39_006D55E0). */
    const int page = lobby.list_req[0x18] | lobby.list_req[0x19] << 8;
    const int count = page > 0 && page < lobby.games_count ? page : lobby.games_count;
    for (int i = 0; i < count; i++) {
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
        put32(reply + 0x18C, i == count - 1);
        if (net_log()) {
            fprintf(
                stderr,
                "[info] rac3: online: game list %d/%d: world %d %.16s, %d players, rules %d\n",
                i + 1,
                count,
                g->world,
                g->ad.name,
                g->players,
                g->ad.rules
            );
        }
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
    int32_t max_players = 0;
    if (world == M_WORLD_OWN && lobby.own_valid) {
        a = &lobby.own;
        players = room.count > 0 ? room.count : 1;
        /* The host's own game as it made it (its CREATE page's 8): it waits for its info to agree
         * with its settings; the room's wider size is in the advert the others list. */
        max_players = net_own_game_max_players;
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
        put32(reply + 0x24, max_players != 0 ? max_players : a->max_players);
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

/* ---- The game's stats, kept by the hub ---- */

/* The game's ladder and career requests go to the hub once logged in there (STATS_*); their
 * replies wait for its answers. The hub takes a few requests a second from a client and drops the
 * rest unanswered, so they go out within its budget, and what the hub already has (the row and the
 * bytes this console wrote or read last) is answered here. */
typedef struct {
    int op;        /* STATS_* */
    int sent;      /* gone to the hub, else waiting for the budget */
    int cls, type; /* the game's request (0, 0: the port's own read, no reply) */
    int32_t account;
    int64_t since_ms;
    size_t size;
    uint8_t req[0x1AC];
} stats_request;

static struct {
    stats_request wait[STATS_WAITS];
    int count;
    double budget;
    int64_t budget_ms;
    uint8_t career[CAREER_BYTES]; /* this account's career bytes, once read or written */
    int career_known;
    uint8_t ladder[4 * LADDER_VALUES]; /* this account's ladder row as last written */
    int ladder_known;
} stats = {.budget = STATS_BURST};

static void srv_i64(int32_t v) {
    srv_i32(v < 0 ? -1 : 0);
    srv_i32(v);
}

/* The reply to the game's request W: from the hub's answer R (after the account id, which matched),
 * or a failure when R is NULL. */
static void stats_reply(const stats_request* w, reader* r) {
    if (w->cls == 0) { /* the port's own read: this account's career bytes */
        if (r != NULL && w->op == STATS_CAREER_READ) {
            memset(stats.career, 0, sizeof stats.career);
            memcpy(stats.career, r->p, r->n < CAREER_BYTES ? r->n : CAREER_BYTES);
            stats.career_known = 1;
        }
        return;
    }
    const lobby_reply* lr = lobby_reply_of(w->cls, w->type);
    uint8_t reply[LOBBY_REPLY_MAX];
    switch (w->op) {
        case STATS_LADDER_WRITE:
        case STATS_CAREER_WRITE:
            lobby_start(reply, lr, w->req, r != NULL ? M_SUCCESS : M_SERVER_ERROR);
            if (r != NULL && w->op == STATS_CAREER_WRITE) {
                memcpy(stats.career, w->req + 0x26, CAREER_BYTES);
                stats.career_known = 1;
            } else if (r != NULL && get32(w->req + 0x18) == 0) {
                memcpy(stats.ladder, w->req + 0x1C, sizeof stats.ladder);
                stats.ladder_known = 1;
            }
            break;
        case STATS_LADDER_READ: { /* LOBBY_LADDER_STATS_READ_ANSWER: +0x1C the id, +0x20 Stats[100] */
            lobby_start(reply, lr, w->req, r != NULL ? M_SUCCESS : M_SERVER_ERROR);
            put32(reply + 0x1C, w->account);
            if (r != NULL) {
                rd_i32(r); /* the ladder type */
                const int count = rd_u8(r);
                for (int k = 0; k < count; k++) {
                    const int32_t v = rd_i32(r);
                    if (k < LADDER_VALUES) {
                        put32(reply + 0x20 + 4 * k, v);
                    }
                }
            }
            break;
        }
        case STATS_LADDER_POSITION: { /* LOBBY_LADDER_POSITION_ANSWER: +0x1C position, +0x20 total */
            const int32_t position = r != NULL ? rd_i32(r) : 0;
            const int32_t total = r != NULL ? rd_i32(r) : 0;
            lobby_start(
                reply,
                lr,
                w->req,
                r == NULL      ? M_SERVER_ERROR
                : position > 0 ? M_SUCCESS
                               : M_NO_RESULT
            );
            put32(reply + 0x1C, position);
            put32(reply + 0x20, total);
            break;
        }
        case STATS_LADDER_PAGE: { /* LOBBY_LADDER_PAGE_ENTRY records: +0x1C position, +0x20 the value
                                   * sorted by, +0x24 AccountID, +0x28 AccountName, +0x1D4 EndOfList */
            const int index = get32(w->req + 0x18);
            int count = 0;
            if (r != NULL) {
                rd_i32(r); /* the accounts on the ladder */
                count = rd_u8(r);
            }
            for (int i = 0; i < count; i++) {
                int32_t values[LADDER_VALUES] = {0};
                char name[33];
                lobby_start(reply, lr, w->req, M_SUCCESS);
                put32(reply + 0x1C, rd_i32(r));
                rd_i32(r);
                put32(reply + 0x24, rd_i32(r));
                rd_text(r, name, sizeof name);
                put_text(reply + 0x28, name, 32);
                const int n = rd_u8(r);
                for (int k = 0; k < n; k++) {
                    const int32_t v = rd_i32(r);
                    if (k < LADDER_VALUES) {
                        values[k] = v;
                    }
                }
                put32(reply + 0x20, index >= 0 && index < LADDER_VALUES ? values[index] : 0);
                put32(reply + lr->size - 4, i == count - 1);
                lobby_push(lr->reply_cls, lr->reply_type, reply, lr->size);
            }
            if (count > 0) {
                return;
            }
            lobby_start(reply, lr, w->req, r != NULL ? M_NO_RESULT : M_SERVER_ERROR);
            break;
        }
        case STATS_CAREER_READ: /* LOBBY_ACCOUNT_INFO_ANSWER: +0x1C AccountName, +0x48 Stats */
            lobby_start(reply, lr, w->req, M_SUCCESS);
            if (w->account == net_login.account) {
                put_text(reply + 0x1C, net_login.name, 32);
            }
            if (r != NULL) {
                memcpy(reply + 0x48, r->p, r->n < CAREER_BYTES ? r->n : CAREER_BYTES);
                if (w->account == net_login.account) {
                    memset(stats.career, 0, sizeof stats.career);
                    memcpy(stats.career, r->p, r->n < CAREER_BYTES ? r->n : CAREER_BYTES);
                    stats.career_known = 1;
                }
            }
            break;
        default:
            return;
    }
    lobby_push(lr->reply_cls, lr->reply_type, reply, lr->size);
}

static void stats_drop(int i, reader* r) {
    const stats_request w = stats.wait[i];
    memmove(
        &stats.wait[i], &stats.wait[i + 1], sizeof stats.wait[0] * (size_t)(stats.count - i - 1)
    );
    stats.count--;
    stats_reply(&w, r);
}

/* Sends request W to the hub: the game family, then the opcode's fields. */
static void stats_send(stats_request* w) {
    const uint8_t* q = w->req;
    hub_begin(PACKET_STATS, w->op);
    srv_text(NET_FAMILY);
    switch (w->op) {
        case STATS_LADDER_WRITE: /* LadderType, Stats[100] */
            srv_i32(get32(q + 0x18));
            srv_u8(LADDER_VALUES);
            for (int k = 0; k < LADDER_VALUES; k++) {
                srv_i32(get32(q + 0x1C + 4 * k));
            }
            break;
        case STATS_LADDER_READ: /* AccountID, LadderType */
            srv_i64(w->account);
            srv_i32(get32(q + 0x1C));
            break;
        case STATS_LADDER_PAGE: { /* the value to sort by, the order, the first position, the page */
            const int index = get32(q + 0x18);
            const int32_t start = get32(q + 0x20);
            const int32_t page = get32(q + 0x24);
            srv_i32(0);
            srv_u8(index >= 0 && index < LADDER_VALUES ? index : 0);
            srv_u8(get32(q + 0x1C) != 0);
            srv_i32(start < 1 ? 1 : start);
            srv_u8(page < 1 ? 1 : page > LADDER_PAGE_MAX ? LADDER_PAGE_MAX : page);
            break;
        }
        case STATS_LADDER_POSITION: { /* AccountID, the value to sort by, the order */
            const int index = get32(q + 0x1C);
            srv_i64(w->account);
            srv_i32(0);
            srv_u8(index >= 0 && index < LADDER_VALUES ? index : 0);
            srv_u8(get32(q + 0x20) != 0);
            break;
        }
        case STATS_CAREER_WRITE: /* Stats[256] */
            srv_bytes(q + 0x26, CAREER_BYTES);
            break;
        case STATS_CAREER_READ: /* the account */
            srv_i64(w->account);
            break;
        default:
            break;
    }
    srv_end();
    w->sent = 1;
    if (net_log()) {
        fprintf(
            stderr,
            "[info] rac3: online: stats request %d to the hub (account %d)\n",
            w->op,
            w->account
        );
    }
}

/* Sends what the hub's budget allows, and fails what it left unanswered too long. */
static void stats_pump(void) {
    const int64_t now = host_ms();
    if (stats.budget_ms != 0) {
        stats.budget += (double)(now - stats.budget_ms) * STATS_PER_SECOND / 1000.0;
        if (stats.budget > STATS_BURST) {
            stats.budget = STATS_BURST;
        }
    }
    stats.budget_ms = now;
    for (int i = 0; i < stats.count;) {
        if (now - stats.wait[i].since_ms > STATS_GIVE_UP_MS) {
            fprintf(
                stderr,
                "[warning] rac3: online: the hub did not answer stats request %d\n",
                stats.wait[i].op
            );
            stats_drop(i, NULL);
            continue;
        }
        if (!stats.wait[i].sent && stats.budget >= 1.0 && hub.fd != NET_INVALID) {
            stats.budget -= 1.0;
            stats_send(&stats.wait[i]);
        }
        i++;
    }
}

/* Queues one of the game's stats requests (or, with CLS 0, the port's own read) for the hub. */
static void stats_ask(int op, int cls, int type, const uint8_t* req, size_t size, int32_t account) {
    if (stats.count == STATS_WAITS) {
        stats_drop(0, NULL);
    }
    stats_request* w = &stats.wait[stats.count++];
    memset(w, 0, sizeof *w);
    w->op = op;
    w->cls = cls;
    w->type = type;
    w->account = account;
    w->since_ms = host_ms();
    w->size = size < sizeof w->req ? size : sizeof w->req;
    if (req != NULL) {
        memcpy(w->req, req, w->size);
    }
    stats_pump();
}

/* The hub's answer of opcode OP: the oldest request it answers (an account's read: that account's). */
static void stats_answer(int op, reader* r) {
    int32_t account = 0;
    const int by_account =
        op == STATS_LADDER_READ || op == STATS_LADDER_POSITION || op == STATS_CAREER_READ;
    if (by_account) {
        rd_i32(r);
        account = rd_i32(r);
    }
    for (int i = 0; i < stats.count; i++) {
        if (stats.wait[i].sent && stats.wait[i].op == op
            && (!by_account || stats.wait[i].account == account)) {
            stats_drop(i, r);
            return;
        }
    }
}

/* The hub refused a stats request (ERROR_NOT_LOGGED_IN, ERROR_TRY_LATER): the oldest one sent. */
static void stats_refused(int reason) {
    for (int i = 0; i < stats.count; i++) {
        if (stats.wait[i].sent) {
            fprintf(
                stderr,
                "[warning] rac3: online: the hub refused stats request %d (reason %d)\n",
                stats.wait[i].op,
                reason
            );
            stats_drop(i, NULL);
            return;
        }
    }
}

/* One of the game's ladder or career requests: to the hub when logged in there (1), else answered by
 * the lobby's default (0). What the hub already has from this console is answered at once. */
static int stats_request_from_game(int cls, int type, const uint8_t* req, size_t size) {
    if (!hub_logged_in() || hub.fd == NET_INVALID) {
        return 0;
    }
    switch (cls << 8 | type) {
        case LOBBY_LADDER_STATS_WRITE_REQUEST:
            if (size < 0x1AC) {
                return 0;
            }
            if (get32(req + 0x18) == 0 && stats.ladder_known
                && memcmp(stats.ladder, req + 0x1C, sizeof stats.ladder) == 0) {
                return 0; /* the hub has this row */
            }
            stats_ask(STATS_LADDER_WRITE, cls, type, req, size, net_login.account);
            return 1;
        case LOBBY_CAREER_STATS_UPLOAD_REQUEST:
            if (size < 0x126) {
                return 0;
            }
            if (stats.career_known && memcmp(stats.career, req + 0x26, CAREER_BYTES) == 0) {
                return 0;
            }
            stats_ask(STATS_CAREER_WRITE, cls, type, req, size, net_login.account);
            return 1;
        case LOBBY_LADDER_STATS_READ_REQUEST: {
            if (size < 0x20) {
                return 0;
            }
            const int32_t account = get32(req + 0x18);
            if (account == net_login.account && stats.ladder_known && get32(req + 0x1C) == 0) {
                const lobby_reply* lr = lobby_reply_of(cls, type);
                uint8_t reply[0x1B0];
                lobby_start(reply, lr, req, M_SUCCESS);
                put32(reply + 0x1C, account);
                memcpy(reply + 0x20, stats.ladder, sizeof stats.ladder);
                lobby_push(lr->reply_cls, lr->reply_type, reply, lr->size);
                return 1;
            }
            stats_ask(STATS_LADDER_READ, cls, type, req, size, account);
            return 1;
        }
        case LOBBY_LADDER_PAGE_REQUEST:
            if (size < 0x28) {
                return 0;
            }
            stats_ask(STATS_LADDER_PAGE, cls, type, req, size, 0);
            return 1;
        case LOBBY_LADDER_POSITION_REQUEST:
            if (size < 0x24) {
                return 0;
            }
            stats_ask(STATS_LADDER_POSITION, cls, type, req, size, get32(req + 0x18));
            return 1;
        case LOBBY_ACCOUNT_INFO_REQUEST: {
            if (size < 0x2C) {
                return 0;
            }
            const int32_t account = get32(req + 0x28);
            if (account == net_login.account && stats.career_known) {
                const lobby_reply* lr = lobby_reply_of(cls, type);
                uint8_t reply[0x148];
                lobby_start(reply, lr, req, M_SUCCESS);
                put_text(reply + 0x1C, net_login.name, 32);
                memcpy(reply + 0x48, stats.career, CAREER_BYTES);
                lobby_push(lr->reply_cls, lr->reply_type, reply, lr->size);
                return 1;
            }
            stats_ask(STATS_CAREER_READ, cls, type, req, size, account);
            return 1;
        }
        default:
            return 0;
    }
}

/* Every request waiting for the hub fails (the hub's link closed, or the account logged out). */
static void stats_forget(void) {
    while (stats.count > 0) {
        stats_drop(0, NULL);
    }
    stats.career_known = 0;
    stats.ladder_known = 0;
}

static void lobby_reply_players(const uint8_t* req) {
    const lobby_reply* r = lobby_reply_of(1, 0x2F);
    uint8_t reply[0x148];
    for (int i = 0; i < room.count; i++) {
        lobby_start(reply, r, req, M_SUCCESS);
        put32(reply + 0x1C, room.m[i].id);
        put_text(reply + 0x20, room.m[i].name, 32);
        if (room.m[i].id == srv.session && stats.career_known) {
            memcpy(reply + 0x40, stats.career, CAREER_BYTES); /* Stats: this account's career */
        }
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
        /* LOBBY_GAME_REPORT: the host's game as it is now, listed with its party. */
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
        /* LOBBY_PRESENCE_UPDATE (left the game world): the game is left, so is its party. */
        srv_begin(PACKET_PARTY, PARTY_LEAVE);
        srv_end();
        room.in = 0;
        room.count = 0;
        return;
    }
    if (r == NULL) {
        return; /* no reply: the reports, chat, user state, the DNAS post */
    }
    if (stats_request_from_game(cls, type, req, size)) {
        return;
    }
    lobby_start(reply, r, req, M_SUCCESS);
    switch (cls << 8 | type) {
        case LOBBY_SESSION_OPEN_REQUEST: /* the session key */
            put_text(reply + 0x1C, "OPENRACSESSION01", 17);
            break;
        case LOBBY_LOGIN_REQUEST: { /* the hub's account login, the reply when it answers */
            char password[33];
            memcpy(lobby.login, req, sizeof lobby.login);
            memcpy(lobby.name, req + 0x26, 32);
            lobby.name[32] = 0;
            memcpy(password, req + 0x46, 32);
            password[32] = 0;
            if (lobby.name[0] == 0) {
                snprintf(lobby.name, sizeof lobby.name, "Player");
            }
            const int same = strcmp(net_login.name, lobby.name) == 0
                             && strcmp(net_login.password, password) == 0;
            snprintf(net_login.name, sizeof net_login.name, "%s", lobby.name);
            snprintf(net_login.password, sizeof net_login.password, "%s", password);
            if (net_login.direct ? srv.state == 1 : same && hub_logged_in()) {
                lobby_reply_login(M_SUCCESS);
            } else if (!net_login.direct && hub.state == 1 && (hub.features & FEATURE_SERVERS)) {
                lobby.login_waiting = 1; /* another account, or a retry: the hub's link is up */
                hub_send_login();
            } else if (net_login.direct ? srv_open(lobby.name) : hub_open(lobby.name)) {
                lobby.login_waiting = 1;
            } else {
                lobby_reply_login(M_LOGIN_FAILED);
            }
            return;
        }
        case LOBBY_SESSION_CLOSE_REQUEST: /* the game logs out */
        case LOBBY_LOGOUT_REQUEST:
            hub_logout();
            break;
        case LOBBY_KEEP_ALIVE_REQUEST:
            put_text(reply + 0x1C, "127.0.0.1", 20);
            break;
        case LOBBY_CHANNEL_JOIN_REQUEST: /* the one channel's lobby */
            srv_ensure();
            lobby_connect_info(reply + 0x1C, M_WORLD_LOBBY);
            break;
        case LOBBY_GAME_PLAYER_LIST_REQUEST: /* the party's members */
            lobby_reply_players(req);
            return;
        case LOBBY_GAME_INFO_REQUEST:
            lobby_reply_game_info(req, size >= 0x2C ? get32(req + 0x28) : 0);
            return;
        case LOBBY_CHAT_TEXT_CHECK_REQUEST: /* the text passes as it is */
            memcpy(reply + 0x1C, req + 0x2C, 64);
            break;
        case LOBBY_LICENCE_TEXT_REQUEST: /* waits for a text (+0x1C), then its end (+0x11C) */
            /* What the player is connecting to: the city picked, else the first listed. */
            if (hub_logged_in() && lobby.regions_count > 0) {
                const int k = net_login.picked >= 0 && net_login.picked < lobby.regions_count
                                  ? net_login.picked
                                  : 0;
                char city[NET_CITY_LONG_TEXT + 1], text[96];
                net_city_text(k, NET_CITY_LONG_TEXT, city, sizeof city);
                snprintf(text, sizeof text, "Online play on %s, an OpenRAC server.", city);
                put_text(reply + 0x1C, text, 256);
            } else {
                put_text(reply + 0x1C, "Online play on an OpenRAC server.", 256);
            }
            break;
        case LOBBY_NEWS_REQUEST: /* none */
        case LOBBY_ALL_NEWS_REQUEST:
            put32(reply + M_STATUS, M_NO_RESULT);
            break;
        case LOBBY_CITY_LIST_REQUEST: { /* the hub's regions, the game's city list */
            memcpy(lobby.locations_req, req, sizeof lobby.locations_req);
            /* The hub's list; a game server named directly that lists regions (one from before
             * the hub) its own. A server that takes no PACKET_REGIONS closes on it: the game's
             * own cities then. */
            net_link* from = !net_login.direct ? &hub : &srv;
            /* The hub's list is kept current (SERVERS_WATCH): answered at once, unless the watch
             * has only just begun and its servers are still on the way. */
            if (from == &hub && lobby.watching) {
                if (lobby.regions_count == 0 && host_ms() - lobby.watched_ms < 2000) {
                    lobby.locations_waiting = 1;
                    return;
                }
                lobby_reply_locations();
                return;
            }
            if (from->fd == NET_INVALID || !(from->features & FEATURE_REGIONS)) {
                put32(reply + 0x5C, M_NO_RESULT);
                break;
            }
            lobby.locations_waiting = 1;
            if (from == &hub) {
                hub_begin(PACKET_REGIONS, -1);
            } else {
                srv_begin(PACKET_REGIONS, -1);
            }
            srv_text(NET_FAMILY);
            srv_end();
            return;
        }
        case LOBBY_CHANNEL_FILTER_SET_REQUEST: /* it carries the city chosen: its game server */
            lobby_pick_city(req, size);
            if (net_log()) {
                char hex[3 * 0x30 + 1];
                for (size_t k = 0; k < size && k < 0x30; k++) {
                    snprintf(hex + 3 * k, 4, "%02x ", req[k]);
                }
                fprintf(stderr, "[info] rac3: online: lobby world filter %s\n", hex);
            }
            break;
        case LOBBY_CHANNEL_LIST_REQUEST: /* one numbered channel ("%02X" at name + 0xB) */
            put32(reply + 0x1C, M_WORLD_LOBBY);
            put16(reply + 0x20, (int16_t)(room.count + 1));
            put16(reply + 0x22, 256);
            put32(reply + 0x28, 1);
            put_text(reply + 0x3C, "OpenRAC CH 01", 64);
            break;
        case LOBBY_CHANNEL_CREATE_REQUEST:
            put32(reply + 0x1C, M_WORLD_LOBBY);
            break;
        case LOBBY_GAME_LIST_REQUEST: /* the server's parties of this game */
            memcpy(lobby.list_req, req, sizeof lobby.list_req);
            srv_ensure();
            if (srv.fd == NET_INVALID || lobby.list_waiting) {
                if (!lobby.list_waiting) {
                    lobby_reply_list();
                }
                return;
            }
            lobby.list_waiting = 1;
            srv_begin(PACKET_PARTY, PARTY_LIST);
            srv_text(NET_FAMILY);
            srv_u8(LIST_ADVERTS);
            srv_end();
            return;
        case LOBBY_GAME_CREATE_REQUEST: /* a party hosted, its settings its advert */
            memcpy(lobby.create_req, req, sizeof lobby.create_req);
            advert_from_create(&lobby.own, req);
            lobby.own_valid = 1;
            srv_ensure();
            if (srv.fd == NET_INVALID) {
                put32(reply + M_STATUS, -987);
                break;
            }
            lobby.create_waiting = 1;
            rac3_mp_wide_set_slots(lobby.own.max_players);
            srv_begin(PACKET_PARTY, PARTY_CREATE);
            srv_text(lobby.own.name);
            srv_text(NET_FAMILY);
            srv_end();
            return;
        case LOBBY_GAME_JOIN_REQUEST: { /* own game at once, another's once the server took us in */
            const int32_t world = get32(req + 0x28);
            memcpy(lobby.join_req, req, sizeof lobby.join_req);
            if (world == M_WORLD_OWN && room.in) {
                lobby_reply_join(M_SUCCESS, world);
                return;
            }
            srv_ensure();
            const listed_game* g = lobby_game(world);
            if (g == NULL || srv.fd == NET_INVALID) {
                lobby_reply_join(M_GAME_NOT_FOUND, world);
                return;
            }
            lobby.join_waiting = 1;
            rac3_mp_wide_set_slots(g->ad.max_players);
            srv_begin(PACKET_PARTY, PARTY_JOIN);
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

/* The first byte of a PACKET_RELAY payload is the kind (RELAY_* in mp_protocol.h): a DME packet
 * follows (RELAY_ROOM_PACKET); a net object (RELAY_OBJECT): id, type, owner, creator, then its
 * data; or a net object released (RELAY_OBJECT_RELEASE): its id (the game never destroys one). */

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
    GREF(int32_t, dme.conn + CONN_CLIENTS) = GAME_CLIENTS;
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
    for (int i = 0; i < GAME_CLIENTS; i++) {
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
        if (src < 0 || src >= GAME_CLIENTS) {
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
 * member (the room join request goes to -1), 0xFFFF the DME server (none here: dropped). */
#define END_MESSAGE_BYTES 0x68 /* channel 5's message */
#define END_MESSAGE_COMMAND 0x0C
#define END_MESSAGE_SCORES 0x82
static int dme_relay(int dest, gaddr data, int len) {
    if (srv.fd == NET_INVALID || !room.in || len <= 0 || len > 0x5F0) {
        return 0;
    }
    int32_t target = 0;
    if (dest == -1) {
        dest = 0xFFFE;
    }
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
    /* A message to its own index comes straight back, as DME delivers it (the host's own
     * CHANNEL_SLOT_UPDATE after a map load: state 8 in slot 0 opens the start barrier). */
    if (target == srv.session) {
        if (dme.inbox_count < (int)(sizeof dme.inbox / sizeof dme.inbox[0])) {
            dme.inbox[dme.inbox_count].from = srv.session;
            dme.inbox[dme.inbox_count].len = (uint16_t)len;
            memcpy(dme.inbox[dme.inbox_count].data, G(data), (size_t)len);
            dme.inbox_count++;
        }
        return 0;
    }
    if (!srv_room((size_t)len + 8)) {
        return 0;
    }
    /* The match's end (channel 5, command 0x82: the scores of the game's eight slots) is followed
     * by the others' in a wide room: they go first, so a console has them before the message. */
    if (len == 2 + END_MESSAGE_BYTES && GREF(uint8_t, data + 1) == CHANNEL_MATCH_STATE
        && GREF(uint8_t, data + 2 + END_MESSAGE_COMMAND) == END_MESSAGE_SCORES
        && rac3_mp_wide_slots() > 8 && srv_room(0x100)) {
        uint8_t wide[0xC0];
        const int n = rac3_mp_wide_end_pack(wide, (int)sizeof wide);
        if (n > 0) {
            srv_begin(PACKET_RELAY, -1);
            srv_i32(target);
            srv_u8(RELAY_MATCH_END_WIDE);
            srv_u8((uint8_t)rac3_mp_wide_slots());
            srv_bytes(wide, (size_t)n);
            srv_end();
        }
    }
    srv_begin(PACKET_RELAY, -1);
    srv_i32(target);
    srv_u8(RELAY_ROOM_PACKET);
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
 * others get a copy and the type's callbacks (on_create(creator, id) the first time, then
 * on_update(creator, id, -1) for the whole object). DME's own object layer needs its peer session, which
 * the room does not run, so the port keeps them: a record (+0 id, +4 owner client, +8 creator, +0x1C
 * the data) in the game's memory, the data relayed whole at each publish. */
#define OBJ_MAX 64
#define OBJ_TYPES 16

static struct {
    int32_t size, on_create, on_update, on_destroy;
} obj_type[OBJ_TYPES];

static struct {
    int32_t id, type, owner, creator;
    gaddr record, data;
    uint8_t* sent; /* the data as last sent, for the owner's change check */
} objs[OBJ_MAX];

static int obj_count, obj_next = 1;

/* OBJECT_GAME_SETTINGS_WIDE (host/mp_wide.c): slots 8-23 of the settings object's arrays, kept
 * beside the game's object of that type ("tNW_GameSettings") and sent with it, first, in a room of
 * more than 8. One a console, as the game has one settings object. */
static struct {
    int32_t type; /* the settings object's type, -1 until the game registers it */
    gaddr data;
    uint8_t* sent;
} gs_wide = {-1, 0, NULL};

static void gs_wide_make(void) {
    if (gs_wide.data == 0) {
        gs_wide.data = openrac_guest_static(rac3_mp_wide_object_size());
        gs_wide.sent = calloc(1, rac3_mp_wide_object_size());
    }
    rac3_mp_wide_object_clear(gs_wide.data);
    memcpy(gs_wide.sent, G(gs_wide.data), rac3_mp_wide_object_size());
    rac3_mp_wide_object(gs_wide.data);
}

static int gs_wide_sends(int i) {
    return objs[i].type == gs_wide.type && gs_wide.data != 0 && rac3_mp_wide_slots() > 8;
}

static int obj_find(int32_t id) {
    for (int i = 0; i < obj_count; i++) {
        if (objs[i].id == id) {
            return i;
        }
    }
    return -1;
}

static int obj_new(int32_t id, int32_t type, int32_t owner, int32_t creator) {
    if (obj_count == OBJ_MAX || type < 0 || type >= OBJ_TYPES || obj_type[type].size <= 0) {
        return -1;
    }
    const int i = obj_count++;
    objs[i].id = id;
    objs[i].type = type;
    objs[i].owner = owner;
    objs[i].creator = creator;
    objs[i].sent = calloc(1, (size_t)obj_type[type].size);
    objs[i].record = openrac_guest_static(0x20);
    objs[i].data = openrac_guest_static((uint32_t)obj_type[type].size);
    GREF(int32_t, objs[i].record + 0x0) = id;
    GREF(int32_t, objs[i].record + 0x4) = owner;
    GREF(int32_t, objs[i].record + 0x8) = creator;
    GREF(int32_t, objs[i].record + 0x1C) = (int32_t)objs[i].data;
    if (type == gs_wide.type) {
        gs_wide_make();
    }
    return i;
}

static void gs_log(const char* who);

/* Sends object I to every other member (TARGET 0) or to one session. */
static void obj_send(int i, int32_t target) {
    const int32_t size = obj_type[objs[i].type].size;
    if (srv.fd == NET_INVALID || !room.in || !srv_room((size_t)size + 32)) {
        return;
    }
    if (gs_wide_sends(i)) {
        const uint32_t wide = rac3_mp_wide_object_size();
        if (!srv_room((size_t)wide + (size_t)size + 64)) {
            return;
        }
        srv_begin(PACKET_RELAY, -1);
        srv_i32(target);
        srv_u8(RELAY_OBJECT_WIDE);
        srv_i32(objs[i].id);
        srv_bytes(G(gs_wide.data), wide);
        srv_end();
        memcpy(gs_wide.sent, G(gs_wide.data), wide);
    }
    srv_begin(PACKET_RELAY, -1);
    srv_i32(target);
    srv_u8(RELAY_OBJECT);
    srv_i32(objs[i].id);
    srv_i32(objs[i].type);
    srv_i32(objs[i].owner);
    srv_i32(objs[i].creator);
    srv_bytes(G(objs[i].data), (size_t)size);
    srv_end();
    memcpy(objs[i].sent, G(objs[i].data), (size_t)size);
    if (net_log() && objs[i].type == gs_wide.type) {
        gs_log("sent");
    }
}

/* DME's update sends what changed in the objects this console owns, every frame (the game edits
 * them in place: slot states, ready, start times). The port's update does the same, whole. */
static void obj_send_changes(void) {
    const int me = room_index_of(srv.session);
    for (int i = 0; i < obj_count; i++) {
        /* A released object (owner -1, GameSettings after START) is everyone's: whoever
         * changes it sends it, as the host does its start time (+0x170) and slot states. */
        if ((objs[i].owner == me || objs[i].owner == -1)
            && (memcmp(objs[i].sent, G(objs[i].data), (size_t)obj_type[objs[i].type].size) != 0
                || (gs_wide_sends(i)
                    && memcmp(gs_wide.sent, G(gs_wide.data), rac3_mp_wide_object_size()) != 0))) {
            obj_send(i, 0);
        }
    }
}

/* The type's callbacks take the creator's client index and the object id; on_update's field is -1
 * for the whole object (-2 is "the owner changed", which the game's callbacks ignore). */
static void obj_callback(int32_t fn, int32_t creator, int32_t id, int32_t field, int with_field) {
    if (fn == 0) {
        return;
    }
    if (with_field) {
        GFN(void (*)(int, int, int), (gaddr)fn)(creator, id, field);
    } else {
        GFN(void (*)(int, int), (gaddr)fn)(creator, id);
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

/* The settings object's slots (client:state per slot with a client), logged when they change: the
 * room's staging as this console sees it (OPENRAC_RAC3_NET_LOG). */
static void gs_log(const char* who) {
    static char last[512];
    char line[512];
    size_t n = 0;
    const int slots = rac3_mp_wide_slots();
    for (int slot = 0; slot < slots && n + 16 < sizeof line; slot++) {
        const gaddr client = rac3_mp_wide_settings(0x0E0u, slot);
        const gaddr state = rac3_mp_wide_settings(0x0F0u, slot);
        if (client == 0 || GREF(int16_t, client) < 0) {
            continue;
        }
        n += (size_t)snprintf(
            line + n,
            sizeof line - n,
            " %d:%d/%d",
            slot,
            GREF(int16_t, client),
            GREF(int16_t, state)
        );
    }
    line[n] = 0;
    if (strcmp(line, last) != 0) {
        snprintf(last, sizeof last, "%s", line);
        fprintf(stderr, "[info] rac3: online: settings (%s), slot client/state:%s\n", who, line);
    }
}

static void obj_receive(int kind, int32_t from, reader* r) {
    (void)from;
    const int32_t id = rd_i32(r);
    if (kind == RELAY_OBJECT_WIDE) {
        /* Before the settings object it belongs to, which may be new: kept for it. */
        const uint32_t wide = rac3_mp_wide_object_size();
        if (r->n >= wide) {
            if (gs_wide.data == 0) {
                gs_wide_make();
            }
            memcpy(G(gs_wide.data), r->p, wide);
            memcpy(gs_wide.sent, r->p, wide);
            rac3_mp_wide_object(gs_wide.data);
        }
        return;
    }
    int i = obj_find(id);
    if (kind == RELAY_OBJECT_RELEASE) {
        if (i >= 0) {
            obj_callback(obj_type[objs[i].type].on_destroy, objs[i].creator, id, 0, 0);
            objs[i] = objs[--obj_count];
        }
        return;
    }
    const int32_t type = rd_i32(r);
    const int32_t owner = rd_i32(r);
    const int32_t creator = rd_i32(r);
    const int fresh = i < 0;
    if (fresh) {
        /* The wide half came first: a new settings object keeps it. */
        uint8_t* kept = NULL;
        if (type == gs_wide.type && gs_wide.data != 0) {
            kept = malloc(rac3_mp_wide_object_size());
            if (kept != NULL) {
                memcpy(kept, G(gs_wide.data), rac3_mp_wide_object_size());
            }
        }
        i = obj_new(id, type, owner, creator);
        if (kept != NULL) {
            memcpy(G(gs_wide.data), kept, rac3_mp_wide_object_size());
            memcpy(gs_wide.sent, kept, rac3_mp_wide_object_size());
            free(kept);
        }
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
    memcpy(objs[i].sent, r->p, (size_t)obj_type[objs[i].type].size);
    if (fresh) {
        obj_callback(obj_type[objs[i].type].on_create, creator, id, 0, 0);
    } else {
        obj_callback(obj_type[objs[i].type].on_update, creator, id, -1, 1);
    }
    if (net_log() && objs[i].type == gs_wide.type) {
        gs_log("received");
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
        /* LOBBY_GAME_CREATE_REQUEST's answer: this console's own world; its settings listed. */
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
        case PARTY_INFO:
            on_info(r);
            break;
        case PARTY_JOINED: {
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
        case PARTY_LEFT: {
            const int32_t id = rd_i32(r);
            member* m = room_member(id);
            if (m != NULL) {
                *m = room.m[--room.count];
            }
            dme_report_members();
            break;
        }
        case PARTY_END:
        case PARTY_KICKED:
            fprintf(stderr, "[info] rac3: online: the party is gone\n");
            rac3_mp_wide_set_slots(8);
            room.in = 0;
            room.count = 0;
            dme_report_members();
            break;
        case PARTY_HOST_CHANGED:
            room.host = rd_i32(r);
            dme_conn_refresh();
            if (dme.joined) {
                const member* h = room_member(room.host);
                dme_callback(dme.cb_host, 0, h != NULL ? h->index : 0, 0);
            }
            break;
        case PARTY_SLOTS: {
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
        case PARTY_LIST:
            on_list(r);
            break;
        default:
            break;
    }
}

/* The hub's regions: name, host (empty: the hub's own address), port, players. */
static void on_regions(reader* r) {
    const int count = rd_u8(r);
    lobby.regions_count = 0;
    for (int i = 0; i < count && lobby.regions_count < 16; i++) {
        const int k = lobby.regions_count++;
        lobby.regions[k].id = -1;
        lobby.regions[k].server[0] = 0;
        rd_text(r, lobby.regions[k].name, sizeof lobby.regions[k].name);
        rd_text(r, lobby.regions[k].host, sizeof lobby.regions[k].host);
        lobby.regions[k].port = rd_i32(r);
        lobby.regions[k].players = rd_i32(r);
    }
    /* Then each server's name, in the same order (a hub from before names stops at the list). */
    for (int i = 0; i < lobby.regions_count && r->n >= 2; i++) {
        rd_text(r, lobby.regions[i].server, sizeof lobby.regions[i].server);
    }
    if (net_log()) {
        fprintf(stderr, "[info] rac3: online: %d cities listed\n", lobby.regions_count);
    }
    if (lobby.locations_waiting) {
        lobby_reply_locations();
    }
}

/* The index in lobby.regions of the hub's server ID, -1 when it is not listed. */
static int servers_find(int32_t id) {
    for (int i = 0; i < lobby.regions_count; i++) {
        if (lobby.regions[i].id == id) {
            return i;
        }
    }
    return -1;
}

/* The hub's live list (PACKET_SERVERS): a server added (or listed again), its counts changed, or
 * gone. The city the game picked and the one connected to are indices: they follow a removal. */
static void on_servers(reader* r) {
    const int opcode = rd_u8(r);
    const int32_t id = rd_i32(r);
    int k = servers_find(id);
    if (opcode == SERVERS_ADDED) {
        if (k < 0) {
            if (lobby.regions_count == 16) {
                return;
            }
            k = lobby.regions_count++;
        }
        lobby.regions[k].id = id;
        rd_text(r, lobby.regions[k].name, sizeof lobby.regions[k].name);
        rd_text(r, lobby.regions[k].server, sizeof lobby.regions[k].server);
        rd_text(r, lobby.regions[k].host, sizeof lobby.regions[k].host);
        lobby.regions[k].port = rd_i32(r);
        lobby.regions[k].players = rd_i32(r);
        if (net_log()) {
            char city[NET_CITY_TEXT + 1];
            net_city_text(k, NET_CITY_TEXT, city, sizeof city);
            fprintf(stderr, "[info] rac3: online: city listed: %s\n", city);
        }
    } else if (opcode == SERVERS_CHANGED && k >= 0) {
        lobby.regions[k].players = rd_i32(r);
    } else if (opcode == SERVERS_REMOVED && k >= 0) {
        memmove(
            &lobby.regions[k],
            &lobby.regions[k + 1],
            sizeof lobby.regions[0] * (size_t)(lobby.regions_count - k - 1)
        );
        lobby.regions_count--;
        if (net_login.picked == k) {
            net_login.picked = -1;
        } else if (net_login.picked > k) {
            net_login.picked--;
        }
        if (net_login.region == k) {
            net_login.region = -1; /* its link closes by itself when the server goes */
        } else if (net_login.region > k) {
            net_login.region--;
        }
    } else {
        return;
    }
    if (lobby.locations_waiting && opcode == SERVERS_ADDED) {
        lobby_reply_locations();
    }
}

static void on_frame(int id, reader* r) {
    switch (id) {
        case PACKET_HELLO:
            srv.session = rd_i32(r);
            srv.state = 1;
            fprintf(stderr, "[info] rac3: online: session %d\n", srv.session);
            if (lobby.login_waiting && net_login.direct) {
                lobby_reply_login(M_SUCCESS);
            }
            break;
        case PACKET_ACCOUNT:
            /* The game server checked the hub's token with the hub: the account plays here. */
            if (rd_u8(r) == ACCOUNT_LOGGED_IN) {
                char name[33];
                rd_i32(r);
                const int32_t account = rd_i32(r);
                rd_text(r, name, sizeof name);
                fprintf(
                    stderr,
                    "[info] rac3: online: the game server took account %d (%s)\n",
                    account,
                    name
                );
            }
            break;
        case PACKET_PARTY:
            on_party(r);
            break;
        case PACKET_ERROR: {
            const int reason = rd_u8(r);
            if (reason == ERROR_SESSION_INVALID || reason == ERROR_TRY_LATER) {
                /* ACCOUNT_TOKEN's answer: the hub knows no such session, or the server cannot
                 * reach the hub. The game goes on under its hello's name, as a guest. */
                fprintf(
                    stderr,
                    "[warning] rac3: online: the game server did not take the hub's login (%s); "
                    "playing as a guest\n",
                    reason == ERROR_SESSION_INVALID ? "session invalid" : "try later"
                );
                break;
            }
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
        case PACKET_UDP:
            /* The UDP offer: its token and port (unused here), then the server's feature bits. */
            rd_i32(r);
            rd_i32(r);
            rd_i32(r);
            srv.features = rd_u8(r);
            /* A server that answers UI is asked for this disc's lobby screens. */
            if (srv.features & FEATURE_UI) {
                srv_begin(PACKET_UI, UI_GET);
                srv_text(NET_FAMILY);
                srv_end();
            }
            break;
        case PACKET_REGIONS: /* from a game server from before the hub, named directly */
            on_regions(r);
            break;
        case PACKET_RELAY: {
            const int32_t from = rd_i32(r);
            const int kind = rd_u8(r);
            if (kind == RELAY_OBJECT || kind == RELAY_OBJECT_RELEASE || kind == RELAY_OBJECT_WIDE) {
                obj_receive(kind, from, r);
                break;
            }
            if (kind == RELAY_MATCH_END_WIDE) {
                /* The room's width, then the arrays of slots 8 up; a console of another width
                 * (never in one room) ignores it. */
                if (r->n >= 1 && r->p[0] == rac3_mp_wide_slots()) {
                    rac3_mp_wide_end_unpack(r->p + 1, (int)r->n - 1);
                }
                break;
            }
            if (kind != RELAY_ROOM_PACKET || r->n == 0 || r->n > sizeof dme.inbox[0].data) {
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
        case PACKET_UI: {
            const int opcode = rd_u8(r);
            if (opcode == UI_DESCRIPTION) {
                const int32_t revision = rd_i32(r);
                /* None keeps the game's own screens; a refused one keeps the screens in force. */
                if (r->n > 0 && rac3_mp_ui_describe((const char*)r->p, r->n)) {
                    fprintf(stderr, "[info] rac3: online: lobby screens, revision %d\n", revision);
                }
            } else if (opcode == UI_NOTICE) {
                char text[257];
                rd_text(r, text, sizeof text);
                fprintf(stderr, "[info] rac3: online: the server says: %s\n", text);
                rac3_mp_ui_notice(text);
            }
            break;
        }
        case PACKET_TIME: {
            const int64_t sent = (int64_t)rd_i32(r) << 32 | (uint32_t)rd_i32(r);
            const int64_t server = (int64_t)rd_i32(r) << 32 | (uint32_t)rd_i32(r);
            const int64_t now = host_ms();
            server_clock.offset = server + (now - sent) / 2 - now;
            server_clock.known = 1;
            break;
        }
        default:
            break; /* UDP offers, chat, data: unused here */
    }
}

/* PACKET_TIME every KEEP_ALIVE_MS once greeted, which HELLO promised (CLIENT_KEEPS_ALIVE): a game
 * that hangs or dies stops sending, and the server drops it from its party. The answer is ignored. */
static void link_keep_alive(net_link* l) {
    const int64_t ms = host_ms();
    const int64_t every = l == &hub ? HUB_KEEP_ALIVE_MS : KEEP_ALIVE_MS;
    if (l->state != 1 || ms - l->alive_ms < every || l->out_len + 32 >= LINK_OUT) {
        return;
    }
    l->alive_ms = ms;
    tx = l;
    srv_begin(PACKET_TIME, -1);
    srv_i32((int32_t)(ms >> 32));
    srv_i32((int32_t)ms);
    srv_end();
}

/* Reads what L's server sent and hands every whole frame to ON. Whether L is still connected. */
static int link_pump(net_link* l, void (*on)(int, reader*)) {
    if (l->fd == NET_INVALID) {
        return 0;
    }
    for (;;) {
        if (l->in_len == LINK_IN) {
            break;
        }
        const long n = (long)recv(l->fd, (char*)l->in + l->in_len, (int)(LINK_IN - l->in_len), 0);
        if (n > 0) {
            l->in_len += (size_t)n;
            continue;
        }
        if (n == 0) {
            fprintf(
                stderr,
                "[warning] rac3: online: the %s closed the connection\n",
                l == &hub ? "hub" : "server"
            );
            link_close(l);
        }
        break;
    }
    size_t at = 0;
    while (l->in_len - at >= 4) {
        const uint8_t* p = l->in + at;
        const size_t len = (size_t)p[0] << 24 | (size_t)p[1] << 16 | (size_t)p[2] << 8 | p[3];
        if (len == 0 || len > LINK_IN - 4) {
            at = l->in_len;
            break;
        }
        if (l->in_len - at - 4 < len) {
            break;
        }
        reader r = {p + 5, len - 1};
        on(p[4], &r);
        at += 4 + len;
    }
    memmove(l->in, l->in + at, l->in_len - at);
    l->in_len -= at;
    link_flush(l); /* a paced link's frames held over its budget */
    link_keep_alive(l);
    return l->fd != NET_INVALID;
}

static void srv_pump(void) {
    if (srv.fd != NET_INVALID && !link_pump(&srv, on_frame)) {
        room.in = 0;
    }
}

/* The hub's answer to a login: the game's own login failures for the hub's reasons. */
static void hub_login_failed(int reason) {
    static const char* const why[] = {
        [HUB_ERROR_WRONG_PASSWORD] = "the password is not the account's",
        [ERROR_INVALID_ACCOUNT] = "the hub takes no such name or password (4 to 32 characters)",
        [ERROR_TRY_LATER] = "too many attempts, or the hub cannot answer now",
    };
    const int32_t status = reason == HUB_ERROR_WRONG_PASSWORD ? M_INVALID_PASSWORD
                           : reason == ERROR_INVALID_ACCOUNT  ? M_ACCOUNT_NOT_FOUND
                           : reason == ERROR_TRY_LATER        ? M_SERVER_ERROR
                                                              : M_LOGIN_FAILED;
    fprintf(
        stderr,
        "[warning] rac3: online: the hub refused the login of %s: %s\n",
        net_login.name,
        reason >= 0 && reason < (int)(sizeof why / sizeof why[0]) && why[reason] != NULL
            ? why[reason]
            : "refused"
    );
    if (lobby.login_waiting) {
        lobby_reply_login(status);
    }
}

static void on_hub_frame(int id, reader* r) {
    switch (id) {
        case PACKET_HELLO:
            hub.session = rd_i32(r);
            hub.state = 1;
            break;
        case PACKET_UDP:
            /* The UDP offer: no datagrams at a hub (token and port 0), then its feature bits. A
             * server without FEATURE_SERVERS is a game server named directly: played on as before
             * the hub (hub_pump moves to it). */
            rd_i32(r);
            rd_i32(r);
            rd_i32(r);
            hub.features = rd_u8(r);
            if (!(hub.features & FEATURE_SERVERS)) {
                net_login.direct = 1;
            } else if (lobby.login_waiting) {
                hub_send_login();
            }
            break;
        case PACKET_ACCOUNT: {
            if (rd_u8(r) != ACCOUNT_LOGGED_IN) {
                break;
            }
            char name[33];
            rd_i32(r); /* the account id's high word: ids stay small */
            const int32_t account = rd_i32(r);
            rd_text(r, name, sizeof name);
            rd_text(r, net_login.token, sizeof net_login.token);
            const int created = rd_u8(r);
            fprintf(
                stderr,
                "[info] rac3: online: %s account %d (%s) at the hub\n",
                created ? "registered" : "logged in to",
                account,
                name
            );
            /* Another account than the game server's link proved: that link goes, and so do the
             * stats kept for the last one. */
            if (net_login.account != account) {
                stats_forget();
                if (srv.fd != NET_INVALID && !room.in) {
                    link_close(&srv);
                    net_login.region = -1;
                }
            }
            net_login.account = account;
            net_login.tried_ms = 0;
            if (lobby.login_waiting) {
                lobby_reply_login(M_SUCCESS);
            }
            /* The list now, so that a game server is there before the game asks for the cities,
             * and every change after (SERVERS_ADDED, _CHANGED, _REMOVED). */
            if (!lobby.watching) {
                lobby.watching = 1;
                lobby.watched_ms = host_ms();
                lobby.regions_count = 0;
                hub_begin(PACKET_SERVERS, SERVERS_WATCH);
                srv_text(NET_FAMILY);
                srv_end();
            }
            /* This account's career bytes, which its player list entries and account info show. */
            stats_ask(STATS_CAREER_READ, 0, 0, NULL, 0, account);
            break;
        }
        case PACKET_STATS:
            stats_answer(rd_u8(r), r);
            break;
        case PACKET_ERROR: {
            const int reason = rd_u8(r);
            if (lobby.login_waiting) {
                hub_login_failed(reason);
            } else {
                stats_refused(reason);
            }
            break;
        }
        case PACKET_REGIONS:
            on_regions(r);
            break;
        case PACKET_SERVERS:
            on_servers(r);
            break;
        default:
            break; /* the clock's answers */
    }
}

static void hub_pump(void) {
    if (hub.fd == NET_INVALID) {
        return;
    }
    if (!link_pump(&hub, on_hub_frame)) {
        stats_forget();
        lobby.watching = 0; /* a new hub link watches again */
        if (lobby.login_waiting) {
            lobby_reply_login(M_LOGIN_FAILED);
        }
    }
    stats_pump();
    if (net_login.direct && hub.fd != NET_INVALID) {
        /* Not a hub: the game server's own link to the same address takes the login (its hello's
         * answer gives the reply), and this one closes. */
        link_close(&hub);
        fprintf(
            stderr,
            "[info] rac3: online: %s:%s is a game server, not a hub\n",
            net_login.host,
            net_login.port
        );
        if (lobby.login_waiting && !srv_open(net_login.name)) {
            lobby_reply_login(M_LOGIN_FAILED);
        }
    }
}

/* Everything that waits for a frame of the game: the server's packets, the lobby's callbacks. */
static void net_pump(void) {
    hub_pump();
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

/* ---- The NETWORK STARTUP page (level 39's 0x0067D758) ---- */

/* The page's globals: its state ($gp - 0x175C on PAL, - 0x177C on NTSC-U), the message state's
 * next states on Cross (two words) and on Triangle, and the message it shows (D_00372840, 0x100
 * bytes, which the page prints through a 0xC0-byte buffer, D_00370C40). NTSC-U's from its own
 * f_0067AC48 (the same code: $gp 0x00248140, the message at lui 0x37 + 0x2940). */
#ifdef RAC3_PAL
#define NETCNF_STATE 0x00246864u
#define NETCNF_MESSAGE 0x00372840u
#else
#define NETCNF_STATE 0x002469C4u
#define NETCNF_MESSAGE 0x00372940u
#endif
#define NETCNF_ON_CROSS (NETCNF_STATE + 4u)
#define NETCNF_ON_CROSS_ELSE (NETCNF_STATE + 8u)
#define NETCNF_ON_TRIANGLE (NETCNF_STATE + 12u)
#define NETCNF_LIST_STATE 4     /* the configurations' list */
#define NETCNF_MESSAGE_STATE 11 /* a message, left with Cross or Triangle */
#define NETCNF_MESSAGE_SIZE 0xC0u

/* The combination list: its slots' count (+0), then four slots of 0xC8 bytes from +0x30, the
 * page's rows 1 to 4 (ezNetCnfGetCombination, 0x00137008, is list + 0x30 + 0xC8 (n - 1) for n up
 * to the count, else NULL). A slot's first word is negative when it is empty: its row is hidden
 * and the cursor skips it. +0x08 is the name ezNetCnfGetProperties reads, +0x48 the row's text
 * ("%d. %s"), +0x88 the page's line under the list. */
#define NETCNF_SLOTS 4
#define NETCNF_SLOT 0xC8u
#define NETCNF_SLOT_NAME 0x08u
#define NETCNF_SLOT_ROW 0x48u
#define NETCNF_SLOT_DESCRIPTION 0x88u

/* The server OPENRAC_SERVER names, without its port and brackets, for the page. */
static void netcnf_server_host(char* out, size_t size) {
    const char* env = openrac_env_string("OPENRAC_SERVER");
    if (env == NULL || *env == 0) {
        snprintf(out, size, "%s", "game.openrac.dev");
        return;
    }
    const char* close = env[0] == '[' ? strchr(env, ']') : NULL;
    const char* colon = strrchr(env, ':');
    if (close != NULL) {
        snprintf(out, size, "%.*s", (int)(close - env - 1), env + 1);
    } else if (colon != NULL && strchr(env, ':') == colon) {
        snprintf(out, size, "%.*s", (int)(colon - env), env);
    } else {
        snprintf(out, size, "%s", env);
    }
}

/* ================================ The game's functions ================================ */

/* sceSifSearchModuleByName: the network adaptor's modules are loaded (the menus' "no network
 * adaptor" test, ezNetCnf's and ezNetCtl's inits). */
int mp_func_0012C220(int name) {
    (void)name;
    return 1;
}

/* sceDevctl(device, command, argument, its size, buffer, its size): the fileio RPC to a device of
 * the IOP. Leaving multiplayer (0x00195338, EXIT MULTIPLAYER) and the power button's thread
 * (0x001951A0) switch the network adaptor off with "dev9x:" DDIOC_OFF (0x4402), retried while the
 * answer is below 0. No IOP answers the RPC in the port: the translated call found its reply word
 * 0 and returned -11 every time, so the game spun there and drew no frame again. The adaptor that
 * 0x0012C220 says is loaded is switched off at once. */
int mp_func_0012B610(
    int device, int command, int argument, int argument_size, int buffer, int buffer_size
) {
    (void)device;
    (void)command;
    (void)argument;
    (void)argument_size;
    (void)buffer;
    (void)buffer_size;
    return 0;
}

/* ezNetCnfGetCombinationList(name, type, list): one configuration, the OpenRAC server, in the first
 * of the page's four slots; the other three are empty, so the page shows one row. Returns the
 * configurations' count. */
int mp_func_00136ED0(int a0, int a1, int list) {
    (void)a0;
    (void)a1;
    memset(G(list), 0, 0x30 + NETCNF_SLOTS * NETCNF_SLOT);
    GREF(int32_t, list) = NETCNF_SLOTS;
    for (int n = 1; n < NETCNF_SLOTS; n++) {
        GREF(int32_t, (gaddr)list + 0x30u + NETCNF_SLOT * (gaddr)n) = -1;
    }
    const gaddr slot = (gaddr)list + 0x30u;
    char host[64];
    char row[0x40];
    netcnf_server_host(host, sizeof host);
    snprintf(row, sizeof row, "OpenRAC server (%s)", host);
    put_text((uint8_t*)G(slot + NETCNF_SLOT_NAME), "OpenRAC", 0x40);
    put_text((uint8_t*)G(slot + NETCNF_SLOT_ROW), row, sizeof row);
    put_text((uint8_t*)G(slot + NETCNF_SLOT_DESCRIPTION), "OpenRAC server", 0x31);
    return 1;
}

/* Square on the page (level 39's 0x0067CED8): the console ran the disc's network setup program
 * (cdrom0:\NETGUI\NTGUI.ELF), which the port does not have. The page's own message says where the
 * server is set instead; Cross or Triangle goes back to the list. */
void mp_func_L39_0067CED8(void) {
    char host[64];
    char text[NETCNF_MESSAGE_SIZE];
    netcnf_server_host(host, sizeof host);
    snprintf(
        text,
        sizeof text,
        "Online play goes through the OpenRAC server at %s. Set the multiplayer server in the "
        "launcher: Settings > General.",
        host
    );
    /* The page prints the message as a format. */
    for (char* c = text; *c != 0; c++) {
        if (*c == '%') {
            *c = ' ';
        }
    }
    put_text((uint8_t*)G(NETCNF_MESSAGE), text, sizeof text);
    GREF(int32_t, NETCNF_STATE) = NETCNF_MESSAGE_STATE;
    GREF(int32_t, NETCNF_ON_CROSS) = NETCNF_LIST_STATE;
    GREF(int32_t, NETCNF_ON_CROSS_ELSE) = 0;
    GREF(int32_t, NETCNF_ON_TRIANGLE) = NETCNF_LIST_STATE;
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
    GREF(uint32_t, NET_TRANSPORT_LOCAL_IP) = 0x0100007Fu;
    GREF(uint32_t, NET_TRANSPORT_UP) = 1;
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
    GREF(int32_t, NET_LOBBY_HANDLE) = handle;
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
    GREF(int32_t, NET_LOBBY_HANDLE) = 0;
    lobby.connected = 0;
    return 0;
}

/* MediusUpdate: the server's packets, then the connect callback and the queued replies through
 * the library's dispatcher (it calls the response's handler, which calls the game's callback). */
static int medius_update(void) {
    static gaddr buf;
    create_page_sync();
    net_pump();
    if (lobby.connect_due) {
        lobby.connect_due = 0;
        if (lobby.connect_cb != 0) {
            GFN(void (*)(int, int),
                (gaddr)lobby.connect_cb)(GREF(int32_t, NET_LOBBY_HANDLE), lobby.connect_ud);
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

/* The vblank count (0x00241704) of the network's vblank handler 0x0019BC10, which the talking flag
 * (app id 19) sends on at every 60th: the handler is installed with the voice code, which the port
 * does not start, so the count follows the host's clock at 60 a second while nothing else moves it. */
#ifdef RAC3_PAL
#define NET_VBLANKS 0x00241704u /* $gp - 0x68BC */
#else
#define NET_VBLANKS 0x00241884u
#endif

int mp_func_L39_0067CEA0(void);

static void vblank_count(void) {
    static int32_t written;
    const int32_t now = (int32_t)((int64_t)mp_func_L39_0067CEA0() * 60 / 1000);
    if (GREF(int32_t, NET_VBLANKS) == written) {
        written = now;
        GREF(int32_t, NET_VBLANKS) = now;
    }
}

/* dme_get_time(out): the room's clock in milliseconds, which the net tick (0x001959F8) copies to
 * net + 0x128 each frame and the start barrier of a map compares with GameSettings +0x170 and
 * +0x174. DME's own (0x0014C6B0) is synchronised by its server; here it is the OpenRAC server's
 * clock, the same on every console of the room. */
int mp_func_0014B008(int out) {
    if (out != 0) {
        const int64_t ms = host_ms() + (server_clock.known ? server_clock.offset : 0);
        GREF(int32_t, (gaddr)out) = (int32_t)(ms & 0x7FFFFFFF);
    }
    return 0;
}

/* The kernel library's timer alarm (ticks, handler, argument): the handler runs as
 * handler(id, 0, 0, argument) once the ticks are past. The port runs no timer interrupt, so the
 * handler runs at once. The network's vblank timer (0x0019BC10) arms one each vblank with a count of
 * sub-ticks that its handler 0x0019BCD0 counts down by arming the next; the voice stop at START
 * (0x0019BF20, from L39_006C4448) waits in a loop for that count (0x002416E4) to reach 0, which
 * without this never happens. The chain is as long as the count (6), so the recursion is short. */
int mp_func_0012E3E8(int ticks, int handler, int argument) {
    static int32_t next_id;
    (void)ticks;
    const int32_t id = ++next_id & 0x7FFFFFFF;
    if (handler == 0) {
        return id;
    }
    GFN(int (*)(int, int, int, int), (gaddr)handler)(id, 0, 0, argument);
    return id;
}

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
    rac3_mp_wide_reset();
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
    vblank_count();
    if (dme.joined && rac3_mp_wide_slots() > 8) {
        /* The room's capacity (net + 0x134, which the room join request checks a join against):
         * the CREATE page offers up to 8, so a wider room's host sets it from the room's size. */
        GREF(int32_t, NET_BLOCK + 0x134u) = rac3_mp_wide_slots();
    }
    net_pump();
    dme_deliver();
    obj_send_changes();
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
#define NET_ROOM_HEAP 0xC4000u
#define NET_ROOM_BLOCKS 1024

static struct {
    gaddr base;

    struct {
        uint32_t at, size;
        int used;
    } block[NET_ROOM_BLOCKS];

    int count;
} dme_heap;

int mp_func_00198608(int size) {
    const uint32_t want = ((uint32_t)size + 15u) & ~15u;
    if (dme_heap.base == 0) {
        dme_heap.base = openrac_guest_static(NET_ROOM_HEAP);
        dme_heap.block[0].at = 0;
        dme_heap.block[0].size = NET_ROOM_HEAP;
        dme_heap.block[0].used = 0;
        dme_heap.count = 1;
    }
    for (int i = 0; i < dme_heap.count; i++) {
        if (dme_heap.block[i].used || dme_heap.block[i].size < want) {
            continue;
        }
        if (dme_heap.block[i].size > want && dme_heap.count < NET_ROOM_BLOCKS) {
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
        if (name != 0 && strcmp((const char*)G((gaddr)name), "tNW_GameSettings") == 0) {
            gs_wide.type = type;
        }
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
    const int i = obj_new(id, type, me, me);
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

/* nwobj_set_player(id): the console's player object in its client record; it sends nothing. */
int mp_func_0016ED68(int id) {
    return obj_find(id) < 0;
}

int mp_func_0016EDD0(int id) {
    const int i = obj_find(id);
    if (i < 0) {
        return 1;
    }
    /* Release gives up ownership (owner -1); the object stays with everyone. */
    objs[i].owner = -1;
    GREF(int32_t, objs[i].record + 0x4) = -1;
    obj_send(i, 0);
    return 0;
}

/* nwobj_send(target, id): the whole object to one client index or to all (-1). */
int mp_func_0016EF40(int target, int id) {
    const int i = obj_find(id);
    if (i < 0) {
        return 1;
    }
    const member* m = target >= 0 ? room_member_at(target) : NULL;
    obj_send(i, m != NULL ? m->id : 0);
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
