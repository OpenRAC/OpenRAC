# Ratchet & Clank: Up Your Arsenal: online multiplayer protocol

PAL disc (SCES-52456) unless marked; NTSC-U (SCUS-97353) notes where checked. Everything here comes
from the game's own binaries (the multiplayer executable I5BOOTN.ELF, unpacked, and the level
programs on the disc) and the rac3 decomp; no Sony SDK material. Every item carries our own name (see Naming); the public
Medius/DME/RT name appears only in the "Horizon name" column, next to the binary's evidence
(sizes, handlers, call sites). Markers, one per item: **V** verified in the code and seen in a
run of the port (its `OPENRAC_RAC3_NET_LOG` trace or the server's log), **C** read in the code
only (disassembly or the decomp's matched C), **I** inferred (section 6 gives each reason and a
plan to confirm it).

Working material: `~/RACSwarm/rac3-mp-work/` (the unpacked images `pal/`, `us/`; the query tools in
`tools/`; each part's regenerating scripts in `transport/`, `lobby/`, `ingame/`). The three detailed
parts this document merges are kept as written in `parts/`.

Contents:
- [Naming](#naming) (our id names, one prefix per layer)
- [Opcode index](#index) (every item, its layer, marker and section; V/C/I counts per layer)
1. Summary
2. The network stack
3. A full session (sequence diagram)
4. The protocol, layer by layer: [4.1 OpenRAC server wire](#l1), [4.2 SCE-RT frames](#l2),
   [4.3 Medius lobby](#l3), [4.4 DME](#l4), [4.5 in-game channels and net objects](#l5),
   [4.6 the port's payloads](#l6)
5. [Flows, roles and timing](#s5)
6. [What is still inferred, and how to confirm it](#s6); [6.1 the cross-check with Horizon](#s6-horizon)
7. The port: what it did, and stage 2 (7.6: protocol findings that affect it)

The three analysis parts this document grew from are kept unchanged in `parts/` (`transport.md`,
`lobby.md`, `ingame.md`); start-up, IOP modules, sockets, DNAS and the hook points live there.
Verification scripts and notes of this version: `~/RACSwarm/rac3-mp-work/protocol-v2/` (names: `names.tsv`).

**Names.** Every packet, frame and message carries our name, defined in [Naming](#naming); the
column "Horizon name" gives the public name only to recognise an item.

**Sources.** The binaries and the rac3 decomp; the OpenRAC server's README and enums; for
cross-checking only, the [Horizon Private Server](https://github.com/Horizon-Private-Server/horizon-server)
(MIT license; research only, no code copied; see [6.1](#s6-horizon)).

---

<a id="naming"></a>
## Naming

Every id of the protocol has one name: ALL UPPERCASE, words joined by underscores, a category
prefix for its layer, no mixed case. No prefix repeats a Sony-library or Horizon word. The same
names are the enumerators in `host/mp_protocol.h` and the code of `host/mp_net.c`; the old to new
mapping of every name is `names-map.tsv` next to this document.

| Layer | Prefix | Source of the value part | Example |
|---|---|---|---|
| 1 OpenRAC server wire | `PACKET_` + the packet id's name; `PARTY_` + the party opcode; `ERROR_` + the error reason; `DATAGRAM_` + the UDP kind; flag bits as the server names them (`FEATURE_`, `CLIENT_`, `LIST_`) | the server's own enums | `PACKET_RELAY`, `PARTY_SLOTS`, `ERROR_PARTY_FULL`, `DATAGRAM_BIND`, `FEATURE_REGIONS`, `CLIENT_SLOTS`, `LIST_ADVERTS` |
| 2 SCE-RT transport frames | `TRANSPORT_` | the frame's descriptive name | `TRANSPORT_TCP_CONNECT_REQUEST` |
| 3 Medius lobby exchanges | `LOBBY_` | the exchange's descriptive name | `LOBBY_LOGIN_REQUEST`, `LOBBY_GAME_REPORT`, `LOBBY_GAME_LIST_ENTRY` |
| 4 DME room messages | `ROOM_` | the message's descriptive name | `ROOM_JOIN_WELCOME`, `ROOM_OBJECT_STATE` |
| 5 in-game app channels | `CHANNEL_` (unused channels 6, 8, 13: `CHANNEL_UNUSED_6`, ...); the two net objects `OBJECT_` | the channel's descriptive name | `CHANNEL_ROOM_JOIN_REQUEST` = 16, `OBJECT_GAME_SETTINGS`, `OBJECT_PLAYER_DATA` |
| 6 the port's relay kinds | `RELAY_`; the advert payload `GAME_ADVERT` | the kind's descriptive name | `RELAY_ROOM_PACKET` = 1, `RELAY_OBJECT` = 2, `RELAY_OBJECT_RELEASE` = 3 |

- **Value part.** The descriptive name split word by word into UPPER_SNAKE: a request and its answer
  end in `_REQUEST` / `_ANSWER`, list replies in `_ENTRY`, unsolicited messages in `_NOTICE`;
  acronyms are one word (`TCP`, `UDP`, `ID`, `DNAS`). A name that already starts with its layer's
  word is not doubled (`LOBBY_CHAT_TOGGLE_REQUEST`, not `LOBBY_LOBBY_...`); layer 4 drops a leading DME word.
- **Wire values.** The game's fixed values stay explicit in the header: a lobby exchange is
  class << 8 | type (`LOBBY_SESSION_OPEN_REQUEST` = 0x103), channels are 0..19, RT frames 0x00..0x1E.
- **Collisions.** None between layers (the prefixes differ). Two names needed a word of their own: the
  class-4 answer of `LOBBY_LADDER_STATS_WRITE_REQUEST` is `LOBBY_LADDER_STATS_WRITE_ANSWER_EXT`
  (0x40C; the class-1 answer 0x1CF keeps `LOBBY_LADDER_STATS_WRITE_ANSWER`), and the two RT frames
  that share id 0x1D are `TRANSPORT_MEMORY_QUERY_REQUEST` and `TRANSPORT_MEMORY_QUERY_ANSWER`.
- **The game's own strings.** The net objects' type strings tNW_GameSettings and tNW_PlayerData
  are quoted only as references: they are the game's data, the names above are ours.
- **Not renamed.** Fields, status codes and enum values of the public lobby structs (SessionKey,
  StatusCode, NoResult, JoinedChatWorld ...), the library's function names, and the server's Java
  class and method names stay as they are: they are the evidence being described, not our protocol names.

---

<a id="index"></a>
## Opcode index

Every item of the six layers, in order. Markers as above; the link goes to the item's table.

Counts per layer:

| Layer | V | C | I | Items |
|---|---|---|---|---|
| 1 OpenRAC server | 15 | 19 | 0 | 34 |
| 2 SCE-RT | 0 | 31 | 0 | 31 |
| 3 Medius | 28 | 54 | 0 | 82 |
| 4 DME | 0 | 23 | 0 | 23 |
| 5 In game | 5 | 17 | 0 | 22 |
| 6 Port relay | 3 | 1 | 0 | 4 |

| Layer | Id | Name | Mark | Section |
|---|---|---|---|---|
| 1 OpenRAC server | 0 | `PACKET_HELLO` | V | [4.1](#l1) |
| 1 OpenRAC server | 1 | `PACKET_PARTY` | V | [4.1](#l1) |
| 1 OpenRAC server | 2 | `PACKET_CHAT` | C | [4.1](#l1) |
| 1 OpenRAC server | 3 | `PACKET_DATA` | C | [4.1](#l1) |
| 1 OpenRAC server | 4 | `PACKET_ERROR` | C | [4.1](#l1) |
| 1 OpenRAC server | 5 | `PACKET_TIME` | V | [4.1](#l1) |
| 1 OpenRAC server | 6 | `PACKET_UDP` | V | [4.1](#l1) |
| 1 OpenRAC server | 7 | `PACKET_RELAY` | V | [4.1](#l1) |
| 1 OpenRAC server | 8 | `PACKET_REGIONS` | V | [4.1](#l1) |
| 1 OpenRAC server | 9 | `PACKET_REGISTER` | V | [4.1](#l1) |
| 1 OpenRAC server | PARTY 0 | `PARTY_CREATE` | V | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 1 | `PARTY_JOIN` | V | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 2 | `PARTY_LEAVE` | V | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 3 | `PARTY_INFO` | V | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 4 | `PARTY_JOINED` | V | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 5 | `PARTY_LEFT` | C | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 6 | `PARTY_END` | C | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 7 | `PARTY_FIND` | C | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 8 | `PARTY_LIST` | V | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 9 | `PARTY_HOST_CHANGED` | C | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 10 | `PARTY_RESUME` | C | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 11 | `PARTY_KICK` | C | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 12 | `PARTY_KICKED` | C | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 13 | `PARTY_ADVERT` | V | [4.1](#l1-party) |
| 1 OpenRAC server | PARTY 14 | `PARTY_SLOTS` | V | [4.1](#l1-party) |
| 1 OpenRAC server | ERROR 0 | `ERROR_PARTY_NOT_FOUND` | C | [4.1](#l1-error) |
| 1 OpenRAC server | ERROR 1 | `ERROR_PARTY_FULL` | C | [4.1](#l1-error) |
| 1 OpenRAC server | ERROR 2 | `ERROR_ALREADY_IN_PARTY` | C | [4.1](#l1-error) |
| 1 OpenRAC server | ERROR 3 | `ERROR_NOT_IN_PARTY` | C | [4.1](#l1-error) |
| 1 OpenRAC server | ERROR 4 | `ERROR_NOT_HOST` | C | [4.1](#l1-error) |
| 1 OpenRAC server | ERROR 5 | `ERROR_RESUME_REFUSED` | C | [4.1](#l1-error) |
| 1 OpenRAC server | ERROR 6 | `ERROR_OTHER_GAME` | C | [4.1](#l1-error) |
| 1 OpenRAC server | UDP 0 | `DATAGRAM_BIND` | C | [4.1](#l1-udp) |
| 1 OpenRAC server | UDP 1 | `DATAGRAM_STATE` | C | [4.1](#l1-udp) |
| 2 SCE-RT | 0x00 | `TRANSPORT_TCP_CONNECT_REQUEST` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x01 | `TRANSPORT_DISCONNECT_NOTICE` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x02 | `TRANSPORT_SEND_TO_WORLD` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x03 | `TRANSPORT_SEND_TO_CLIENT` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x04 | `TRANSPORT_SEND_TO_CLIENT_LIST` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x05 | `TRANSPORT_ECHO` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x06 | `TRANSPORT_CONNECT_REFUSED` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x07 | `TRANSPORT_TCP_CONNECT_ACCEPTED` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x08 | `TRANSPORT_CLIENT_CONNECTED_NOTICE` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x09 | `TRANSPORT_CLIENT_DISCONNECTED_NOTICE` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x0A | `TRANSPORT_FROM_SERVER` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x0B | `TRANSPORT_SEND_TO_SERVER` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x0C | `TRANSPORT_UDP_APP_MESSAGE` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x0D | `TRANSPORT_RECEIVE_FLAGS_SET` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x0E | `TRANSPORT_AGGREGATION_TIME_SET` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x0F | `TRANSPORT_FLUSH_ALL` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x10 | `TRANSPORT_FLUSH_TO_CLIENT` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x11 | `TRANSPORT_FORCED_DISCONNECT` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x12 | `TRANSPORT_CLIENT_PUBLIC_KEY` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x13 | `TRANSPORT_PEER_KEY` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x14 | `TRANSPORT_WORLD_KEY` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x15 | `TRANSPORT_TCP_CONNECT_WITH_UDP_REQUEST` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x16 | `TRANSPORT_UDP_HELLO` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x17 | `TRANSPORT_UDP_READY` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x18 | `TRANSPORT_UDP_SERVER_ADDRESS` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x19 | `TRANSPORT_UDP_ACCEPTED` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x1A | `TRANSPORT_CONNECT_COMPLETE` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x1B | `TRANSPORT_PEER_KEY_RELAY` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x1C | `TRANSPORT_SERVER_TEXT_NOTICE` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x1D | `TRANSPORT_MEMORY_QUERY_REQUEST / TRANSPORT_MEMORY_QUERY_ANSWER` | C | [4.2](#l2-ids) |
| 2 SCE-RT | 0x1E | `TRANSPORT_MEMORY_WRITE` | C | [4.2](#l2-ids) |
| 3 Medius | 1/0x00 → none | `LOBBY_GAME_REPORT` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x01 → none | `LOBBY_PLAYER_STATS_REPORT` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x02 → none | `LOBBY_GAME_LEAVE_REPORT` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x03 → 1/0x04 | `LOBBY_SESSION_OPEN_REQUEST / LOBBY_SESSION_OPEN_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x05 → 1/0x06 | `LOBBY_SESSION_CLOSE_REQUEST / LOBBY_SESSION_CLOSE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x07 → 1/0x08 | `LOBBY_LOGIN_REQUEST / LOBBY_LOGIN_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x11 → 1/0x12 | `LOBBY_CAREER_STATS_UPLOAD_REQUEST / LOBBY_CAREER_STATS_UPLOAD_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x15 → 1/0x16 | `LOBBY_LOGOUT_REQUEST / LOBBY_LOGOUT_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x17 → 1/0x18 | `LOBBY_ACCOUNT_ID_REQUEST / LOBBY_ACCOUNT_ID_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x1B → 1/0x1C | `LOBBY_KEEP_ALIVE_REQUEST / LOBBY_KEEP_ALIVE_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x1F → 1/0x20 | `LOBBY_SELF_HOSTED_GAME_CREATE_REQUEST / LOBBY_SELF_HOSTED_GAME_CREATE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x25 → 1/0x26 | `LOBBY_CHANNEL_JOIN_REQUEST / LOBBY_CHANNEL_JOIN_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x2F → 1/0x30 | `LOBBY_GAME_PLAYER_LIST_REQUEST / LOBBY_GAME_PLAYER_LIST_ENTRY` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x31 → 1/0x32 | `LOBBY_ACCOUNT_INFO_REQUEST / LOBBY_ACCOUNT_INFO_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x33 → 1/0x34 | `LOBBY_GAME_INFO_REQUEST / LOBBY_GAME_INFO_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x35 → 1/0x36 | `LOBBY_CHANNEL_INFO_REQUEST / LOBBY_CHANNEL_INFO_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x39 → 1/0x3A | `LOBBY_PLAYER_FIND_REQUEST / LOBBY_PLAYER_FIND_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x3B → none | `LOBBY_CHAT_LINE` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x3F → 1/0x40 | `LOBBY_BUDDY_ADD_REQUEST / LOBBY_BUDDY_ADD_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x41 → 1/0x42 | `LOBBY_BUDDY_REMOVE_REQUEST / LOBBY_BUDDY_REMOVE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x47 → 1/0x48 | `LOBBY_LICENCE_TEXT_REQUEST / LOBBY_LICENCE_TEXT_PART` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x49 → none | `LOBBY_PRESENCE_UPDATE` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x4B → 1/0x4D | `LOBBY_NEWS_REQUEST / LOBBY_NEWS_ENTRY` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x4C → 1/0x4D | `LOBBY_ALL_NEWS_REQUEST / LOBBY_NEWS_ENTRY` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x4E → 1/0x4F | `LOBBY_GAME_LIST_FILTER_ADD_REQUEST / LOBBY_GAME_LIST_FILTER_ADD_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x50 → 1/0x51 | `LOBBY_GAME_LIST_FILTER_REMOVE_REQUEST / LOBBY_GAME_LIST_FILTER_REMOVE_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x54 → 1/0x55 | `LOBBY_CLAN_CREATE_REQUEST / LOBBY_CLAN_CREATE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x56 → 1/0x57 | `LOBBY_CLAN_DISBAND_REQUEST / LOBBY_CLAN_DISBAND_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x58 → 1/0x59 | `LOBBY_CLAN_BY_ID_REQUEST / LOBBY_CLAN_BY_ID_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x5A → 1/0x5B | `LOBBY_CLAN_BY_NAME_REQUEST / LOBBY_CLAN_BY_NAME_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x5C → 1/0x5D | `LOBBY_CLAN_LEADER_TRANSFER_REQUEST / LOBBY_CLAN_LEADER_TRANSFER_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x60 → 1/0x61 | `LOBBY_CLAN_MEMBER_REMOVE_REQUEST / LOBBY_CLAN_MEMBER_REMOVE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x62 → 1/0x63 | `LOBBY_CLAN_INVITE_REQUEST / LOBBY_CLAN_INVITE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x64 → 1/0x65 | `LOBBY_CLAN_INVITATION_LIST_REQUEST / LOBBY_CLAN_INVITATION_ENTRY` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x66 → 1/0x67 | `LOBBY_CLAN_INVITATION_REPLY_REQUEST / LOBBY_CLAN_INVITATION_REPLY_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x68 → 1/0x69 | `LOBBY_CLAN_INVITATION_REVOKE_REQUEST / LOBBY_CLAN_INVITATION_REVOKE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x6A → 1/0x6B | `LOBBY_CLAN_CHALLENGE_REQUEST / LOBBY_CLAN_CHALLENGE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x6C → 1/0x6D | `LOBBY_CLAN_MESSAGE_LIST_REQUEST / LOBBY_CLAN_MESSAGE_ENTRY` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x6E → 1/0x6F | `LOBBY_CLAN_MESSAGE_POST_REQUEST / LOBBY_CLAN_MESSAGE_POST_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x70 → 1/0x71 | `LOBBY_CLAN_MESSAGE_EDIT_REQUEST / LOBBY_CLAN_MESSAGE_EDIT_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x72 → 1/0x73 | `LOBBY_CLAN_MESSAGE_DELETE_REQUEST / LOBBY_CLAN_MESSAGE_DELETE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x74 → 1/0x75 | `LOBBY_CLAN_CHALLENGE_REPLY_REQUEST / LOBBY_CLAN_CHALLENGE_REPLY_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x76 → 1/0x77 | `LOBBY_CLAN_CHALLENGE_REVOKE_REQUEST / LOBBY_CLAN_CHALLENGE_REVOKE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x78 → 1/0x79 | `LOBBY_CLAN_CHALLENGE_HISTORY_REQUEST / LOBBY_CLAN_CHALLENGE_HISTORY_ENTRY` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x7A → 1/0x7B | `LOBBY_CLAN_INVITATIONS_SENT_REQUEST / LOBBY_CLAN_INVITATIONS_SENT_ENTRY` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x7C → 1/0x7D | `LOBBY_MY_CLAN_REQUEST / LOBBY_MY_CLAN_ENTRY` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x7E → 1/0x7F | `LOBBY_CLAN_ALL_MESSAGES_REQUEST / LOBBY_CLAN_ALL_MESSAGES_ENTRY` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x82 → 1/0x83 | `LOBBY_CLAN_CHALLENGE_LIST_REQUEST / LOBBY_CLAN_CHALLENGE_ENTRY` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x84 → 1/0x85 | `LOBBY_CLAN_STATS_UPLOAD_REQUEST / LOBBY_CLAN_STATS_UPLOAD_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x88 → 1/0x89 | `LOBBY_WORLD_SECURITY_REQUEST / LOBBY_WORLD_SECURITY_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x8C → 1/0x8D | `LOBBY_CITY_LIST_REQUEST / LOBBY_CITY_LIST_ENTRY` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x96 → 1/0x97 | `LOBBY_CHAT_TOGGLE_REQUEST / LOBBY_CHAT_TOGGLE_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x98 → 1/0x99 | `LOBBY_CHAT_TEXT_CHECK_REQUEST / LOBBY_CHAT_TEXT_CHECK_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xA3 → 1/0xA4 | `LOBBY_LOCALE_SET_REQUEST / LOBBY_LOCALE_SET_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xA5 → 1/0xA6 | `LOBBY_FILE_CREATE_REQUEST / LOBBY_FILE_CREATE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xA7 → 1/0xA9 | `LOBBY_FILE_UPLOAD_REQUEST / LOBBY_FILE_CHUNK_REQUEST` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xA8 → none | `LOBBY_FILE_CHUNK` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xAA → 1/0xAB | `LOBBY_FILE_CLOSE_REQUEST / LOBBY_FILE_CLOSE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xAC → 1/0xAD | `LOBBY_FILE_DOWNLOAD_REQUEST / LOBBY_FILE_DOWNLOAD_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xC0 → 1/0xC1 | `LOBBY_IGNORE_LIST_REQUEST / LOBBY_IGNORE_LIST_ENTRY` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xC2 → 1/0xC3 | `LOBBY_IGNORE_ADD_REQUEST / LOBBY_IGNORE_ADD_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xC4 → 1/0xC5 | `LOBBY_IGNORE_REMOVE_REQUEST / LOBBY_IGNORE_REMOVE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xD2 → 1/0xD3 | `LOBBY_CLAN_LADDER_REQUEST / LOBBY_CLAN_LADDER_ENTRY` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xD6 → 1/0xD7 | `LOBBY_BUDDY_LIST_REQUEST / LOBBY_BUDDY_LIST_ENTRY` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xDA → 1/0xDB | `LOBBY_CLAN_MEMBER_LIST_REQUEST / LOBBY_CLAN_MEMBER_ENTRY` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xDC → 1/0xDD | `LOBBY_CHANNEL_PLAYER_LIST_REQUEST / LOBBY_CHANNEL_PLAYER_ENTRY` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xEA → 1/0xEB | `LOBBY_GAME_LIST_REQUEST / LOBBY_GAME_LIST_ENTRY` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xF1 → 1/0xF2 | `LOBBY_LADDER_POSITION_REQUEST / LOBBY_LADDER_POSITION_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xF3 → 1/0x24 | `LOBBY_GAME_JOIN_REQUEST / LOBBY_GAME_JOIN_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 1/0xF4 → 1/0x1E | `LOBBY_GAME_CREATE_REQUEST / LOBBY_GAME_CREATE_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 4/0x0A → none | `LOBBY_DNAS_SIGNATURE_REPORT` | C | [4.3](#l3-exchanges) |
| 3 Medius | 4/0x0B → 1/0xCF or 4/0x0C | `LOBBY_LADDER_STATS_WRITE_REQUEST / LOBBY_LADDER_STATS_WRITE_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 4/0x0D → 4/0x0E | `LOBBY_LADDER_STATS_READ_REQUEST / LOBBY_LADDER_STATS_READ_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 4/0x0F → 1/0xF0 | `LOBBY_LADDER_PAGE_REQUEST / LOBBY_LADDER_PAGE_ENTRY` | C | [4.3](#l3-exchanges) |
| 3 Medius | 4/0x12 → 4/0x13 | `LOBBY_CHANNEL_FILTER_SET_REQUEST / LOBBY_CHANNEL_FILTER_SET_ANSWER` | V | [4.3](#l3-exchanges) |
| 3 Medius | 4/0x14 → 1/0x22 | `LOBBY_CHANNEL_CREATE_REQUEST / LOBBY_CHANNEL_CREATE_ANSWER` | C | [4.3](#l3-exchanges) |
| 3 Medius | 4/0x15 → 1/0xED | `LOBBY_CHANNEL_LIST_REQUEST / LOBBY_CHANNEL_LIST_ENTRY` | C | [4.3](#l3-exchanges) |
| 3 Medius | 1/0x3C | `LOBBY_CHAT_LINE_NOTICE` | C | [4.3](#l3-unsolicited) |
| 3 Medius | 1/0x4A | `LOBBY_SERVER_ERROR_NOTICE` | C | [4.3](#l3-unsolicited) |
| 3 Medius | 1/0x9A | `LOBBY_GAME_WORLD_ID_CHANGED_NOTICE` | C | [4.3](#l3-unsolicited) |
| 3 Medius | 4/0x17 | `LOBBY_BINARY_MESSAGE_NOTICE` | C | [4.3](#l3-unsolicited) |
| 3 Medius | 4/0x06 | `LOBBY_BUDDY_ADDED_NOTICE` | C | [4.3](#l3-unsolicited) |
| 4 DME | 0x00 | `ROOM_SERVER_VERSION_NOTICE` | C | [4.4](#l4-ids) |
| 4 DME | 0x01 | `ROOM_PING` | C | [4.4](#l4-ids) |
| 4 DME | 0x02 | `ROOM_FRAGMENT` | C | [4.4](#l4-ids) |
| 4 DME | 0x03 | `ROOM_OBJECT_FIELD_UPDATE` | C | [4.4](#l4-ids) |
| 4 DME | 0x04 | `ROOM_OBJECT_STATE` | C | [4.4](#l4-ids) |
| 4 DME | 0x05 | `ROOM_STREAM_CLOSE` | C | [4.4](#l4-ids) |
| 4 DME | 0x09 | `ROOM_JOIN_WELCOME` | C | [4.4](#l4-ids) |
| 4 DME | 0x0A | `ROOM_OBJECT_DESTROYED` | C | [4.4](#l4-ids) |
| 4 DME | 0x0B | `ROOM_OBJECT_OWNERSHIP_REQUEST` | C | [4.4](#l4-ids) |
| 4 DME | 0x0C | `ROOM_OBJECT_OWNERSHIP_ANSWER` | C | [4.4](#l4-ids) |
| 4 DME | 0x0D | `ROOM_OBJECT_RELEASED` | C | [4.4](#l4-ids) |
| 4 DME | 0x0E | `ROOM_STREAM_CHUNK` | C | [4.4](#l4-ids) |
| 4 DME | 0x0F | `ROOM_CLIENT_RECORD` | C | [4.4](#l4-ids) |
| 4 DME | 0x10 | `ROOM_PEER_HELLO` | C | [4.4](#l4-ids) |
| 4 DME | 0x12 | `ROOM_CLIENT_LEAVING` | C | [4.4](#l4-ids) |
| 4 DME | 0x13 | `ROOM_SERVER_LIST_QUERY` | C | [4.4](#l4-ids) |
| 4 DME | 0x14 | `ROOM_SERVER_LIST_ANSWER` | C | [4.4](#l4-ids) |
| 4 DME | 0x15 | `ROOM_SESSION_JOIN_REQUEST` | C | [4.4](#l4-ids) |
| 4 DME | 0x16 | `ROOM_CLIENT_STATUS` | C | [4.4](#l4-ids) |
| 4 DME | 0x17 | `ROOM_JOIN_ORDER` | C | [4.4](#l4-ids) |
| 4 DME | 0x18 | `ROOM_VOICE_STREAM_SETUP` | C | [4.4](#l4-ids) |
| 4 DME | 0x19 | `ROOM_LAN_FIND_REQUEST` | C | [4.4](#l4-ids) |
| 4 DME | 0x1A | `ROOM_LAN_FIND_ANSWER` | C | [4.4](#l4-ids) |
| 5 In game | 0 (0x002405A8) | `CHANNEL_PLAYER_STATE_CHANGE` | C | [4.5](#l5) |
| 5 In game | 1 (0x002405AC) | `CHANNEL_DAMAGE_EVENT` | C | [4.5](#l5) |
| 5 In game | 2 (0x002405B0) | `CHANNEL_PROXIMITY_TRIGGER` | C | [4.5](#l5) |
| 5 In game | 3 (0x002405B4) | `CHANNEL_VEHICLE_UPDATE` | C | [4.5](#l5) |
| 5 In game | 4 (0x002405B8) | `CHANNEL_PLAYER_KILLED` | C | [4.5](#l5) |
| 5 In game | 5 (0x002405BC) | `CHANNEL_MATCH_STATE` | C | [4.5](#l5) |
| 5 In game | 6 (0x002405C0) | `CHANNEL_UNUSED_6` | C | [4.5](#l5) |
| 5 In game | 7 (0x002405C4) | `CHANNEL_UNIT_MESSAGE` | C | [4.5](#l5) |
| 5 In game | 8 (0x002405C8) | `CHANNEL_UNUSED_8` | C | [4.5](#l5) |
| 5 In game | 9 (0x002405CC) | `CHANNEL_PLAYER_STATE_STREAM` | C | [4.5](#l5) |
| 5 In game | 10 (0x002405D0) | `CHANNEL_PLAYER_SPAWN` | C | [4.5](#l5) |
| 5 In game | 11 (0x002405D4) | `CHANNEL_OBJECT_LINK` | C | [4.5](#l5) |
| 5 In game | 12 (0x002405D8) | `CHANNEL_DELAYED_DIRECTED_MESSAGE` | C | [4.5](#l5) |
| 5 In game | 13 (0x002405DC) | `CHANNEL_UNUSED_13` | C | [4.5](#l5) |
| 5 In game | 14 (0x002405E0) | `CHANNEL_WEAPON_FIRE` | C | [4.5](#l5) |
| 5 In game | 15 (0x002405E4) | `CHANNEL_KNOCKBACK` | C | [4.5](#l5) |
| 5 In game | 16 (0x00240648) | `CHANNEL_ROOM_JOIN_REQUEST` | V | [4.5](#l5) |
| 5 In game | 17 (0x00240640) | `CHANNEL_SLOT_UPDATE` | V | [4.5](#l5) |
| 5 In game | 18 (0x0024064C) | `CHANNEL_ROOM_JOIN_ANSWER` | V | [4.5](#l5) |
| 5 In game | 19 (0x00240644) | `CHANNEL_TALKING_FLAG` | V | [4.5](#l5) |
| 5 In game | — | `OBJECT_PLAYER_DATA` (the game's string "tNW_PlayerData" at 0x00241678; type var 0x0024063C) | C | [4.5](#l5-objects) |
| 5 In game | — | `OBJECT_GAME_SETTINGS` (the game's string "tNW_GameSettings" at 0x00241688; type var 0x00240638) | V | [4.5](#l5-objects) |
| 6 Port relay | 1 | `RELAY_ROOM_PACKET` | V | [4.6](#l6) |
| 6 Port relay | 2 | `RELAY_OBJECT` | V | [4.6](#l6) |
| 6 Port relay | 3 | `RELAY_OBJECT_RELEASE` | C | [4.6](#l6) |
| 6 Port relay | — | `GAME_ADVERT` | V | [4.6](#l6) |

---

## 1. Summary

- **Where the code is.** I5BOOTN.ELF is i5bootn's loader carrying a WAD-packed game: an engine core
  at 0x00118E80-0x0019D550 (the same SDK build as the boot program's, plus all the network
  libraries), the front end at 0x0027B100, and the multiplayer levels' programs loaded at 0x00241A00:
  **level 39** is the multiplayer menus and the whole online lobby (and the DNAS library, 112
  encrypted blocks), levels 40-49 the online/LAN maps, 50-53 split screen.
- **Libraries** (version strings in core.rdata): SCE-RT `rt_comm` 2.7.0, `rt_msg_client` 2.7.2,
  `rt_udp` 2.7.5 with `rt_p2p`, `rt_crypt` 2.7.0 (an RC4 variant, SHA1, 512-bit RSA), `rt_upnp`
  1.01, **Medius client 2.7.1** (lobby), **DME 2.7.10** (game rooms, net objects, StreamMedia
  voice), `libnetb` 2.00.0014 over the IOP's LIBNET, `ezNetCnf`/`ezNetCtl` (network configuration),
  the DNAS 2 client, Nellymoser voice, USB keyboard (lgkbm) and headset (lgaud).
- **Servers.** Medius authentication on `randc3-master.online.scee.com:10075` (PAL; the Australian
  `randc3-prod.rt.au.playstation.com` when the network profile says so); the lobby and channel
  servers come back in the login and join-channel replies; DNAS on `gate1.eu.dnas.playstation.org`
  and `ts01.eu.dnas.playstation.org`. ApplicationID 10683 (PAL) / 10684 (NTSC-U).
- **Topology.** Client-server through a DME world: every game message travels as an RT app message
  to the DME server, which forwards it to one client or all (`TRANSPORT_SEND_TO_CLIENT` / `TRANSPORT_SEND_TO_WORLD`). The game creates its worlds with host type 4 (*ClientServerAuxUDP*) or 0
  (*ClientServer*); the library's P2P paths exist but are dormant (`RATCHET3_LOBBY_USE_P2P` is parsed
  from a lobby announcement and ignored). Inside the world, one console is the **game host**, elected
  by DME and able to migrate: it owns `OBJECT_GAME_SETTINGS`, the match state, slot assignment and the AI
  it spawns; each console owns its own `OBJECT_PLAYER_DATA`, its player stream and the events it causes.
- **Reliability.** RT over TCP for everything reliable (`kind & 0x50`), an auxiliary UDP channel for
  `kind == 0x80` (the per-frame player stream, weapon fire, knockback, proximity). If UDP looks dead
  for 10 s, the game switches every console to reliable (match-state command 0x80).
- **Rates.** DME and the net update run every frame (50 Hz PAL); the player state stream every frame
  (a full block every 5th); unit ("guber") messages flushed every 15 frames, at most 5 at a time;
  host silence is flagged after 1.2 s; the host's world report to Medius every 15 s.

## 2. The network stack

```
 level 39 lobby screens (Ratchet3 code)            map programs 40-53 (game code)
        |  MediusXxx(req, cb, ud)                          |  16 app channels + 4 net-object channels
        v                                                  v
 Medius client 2.7.1 (0x17CD78-0x183C40)          DME 2.7.10 (0x14A440-0x150600, 0x16EAF0-0x174700)
   request -> MediusSendLobbyMessage 0x17DFA8         dme_send_app 0x14AD08 -> 0x14CB98
   reply   <- dispatcher 0x152FB8 (class,type)        receive 0x14CE48 -> dispatcher 0x152FB8 (class 2, id)
        \___________________ both on DME connections (0x14ADD0) ___________________/
                                    |
                     rt_msg_client 2.7.2 (table 0x1D7E30): RT frames, crypto, aux UDP
                                    |
                     rt_comm platform sockets (table 0x1D3970, 25 slots)
                                    |
                     libnetb (command ring to the IOP) -> LIBNET.IRX / INET / SMAP / DEV9 (IOP)
 DNAS 2 (level 39) -> its own BSD shim (level 39 0x5187D0..) -> libnet RPC -> LIBNET.IRX
 ezNetCnf / ezNetCtl (core 0x136D40..0x137700) -> RPC 0x75499128 / 0x75488909 -> NETCNF / INETCTL
```

## 3. A full session

```
 Console A (host)                    Retail servers                       Console B
 ----------------                    --------------                       ---------
 MULTIPLAYER > ONLINE PLAY
 profile + network configuration (memory card: the game's net.bin profiles, BWNETCNF)
 ezNetCnfGetCombinationList/GetProperties/SetProperties/UpInterface;
 poll ezNetCtlGetStatus until +0x30 == 5 (30 s)          [L39 0x67D758]
 DNAS: init, auth, poll status until type 5  ---HTTP POST--> gate1/ts01.eu.dnas   [L39 0x67ABC8]
 resolver thread: randc3-master -> IP        ---DNS------>                        [L39 0x67A7E0]
 net_lib_start: DME, rt_msg, rt_crypt, Medius init       [0x198478 -> 0x198690]
 MediusConnect(auth, 10075)  ===RT 0x15/0x07/0x18/0x16/0x19/0x17/0x1A===> MAS
 LOBBY_SESSION_OPEN_REQUEST 1/0x03 ----------------------------------> MAS
                     <----------------------------------  1/0x04 LOBBY_SESSION_OPEN_ANSWER (SessionKey)
 LOBBY_LOCALE_SET_REQUEST 1/0xA3 / LOBBY_LOCALE_SET_ANSWER 1/0xA4
 LOBBY_DNAS_SIGNATURE_REPORT 4/0x0A (no reply)
 LOBBY_LOGIN_REQUEST 1/0x07 (name, password) -----------------> MAS
                     <----------------------------------  1/0x08 LOBBY_LOGIN_ANSWER (AccountID + lobby NetConnectionInfo)
 MediusDisconnect; MediusConnect(lobby)  =============> MLS
 LOBBY_PRESENCE_UPDATE(JoinedChatWorld) 1/0x49
 LOBBY_LICENCE_TEXT_REQUEST 1/0x47, LOBBY_NEWS_REQUEST / LOBBY_ALL_NEWS_REQUEST 1/0x4B/0x4C,
 LOBBY_CHAT_TOGGLE_REQUEST 1/0x96
 LOBBY_CHANNEL_LIST_REQUEST 4/0x15 / LOBBY_CHANNEL_LIST_ENTRY 1/0xED records; pick a numbered channel
 (or LOBBY_CHANNEL_CREATE_REQUEST 4/0x14)
 LOBBY_CHANNEL_JOIN_REQUEST 1/0x25 / LOBBY_CHANNEL_JOIN_ANSWER 1/0x26 channel NetConnectionInfo;
 reconnect to it; LOBBY_PRESENCE_UPDATE(1)
 LOBBY_GAME_LIST_REQUEST 1/0xEA / LOBBY_GAME_LIST_ENTRY 1/0xEB records (periodic refresh)
 CREATE GAME: LOBBY_GAME_CREATE_REQUEST 1/0xF4 (settings packed into Min/Max/Level/Skill/Rules/GF1-3)
                     <----------------------------------  1/0x1E LOBBY_GAME_CREATE_ANSWER (MediusWorldID)
 LOBBY_GAME_JOIN_REQUEST 1/0xF3 (own world) / LOBBY_GAME_JOIN_ANSWER 1/0x24 DME NetConnectionInfo
 LOBBY_GAME_REPORT 1/0x00 (no reply), then LOBBY_GAME_INFO_REQUEST 1/0x33 polled (<= 21 times) until it echoes the report
 DME connect(world)  ===RT handshake===> DME server; dme_join_session (net+0x5E name)
 join complete -> I am client 0, host = me; create OBJECT_GAME_SETTINGS
                                                                          ... B: same login, list ...
                                                                          LOBBY_GAME_LIST_ENTRY 1/0xEB shows A's game
                                                                          LOBBY_GAME_INFO_REQUEST 1/0x33 / LOBBY_GAME_INFO_ANSWER 1/0x34
                                                                          LOBBY_GAME_JOIN_REQUEST 1/0xF3 / LOBBY_GAME_JOIN_ANSWER 1/0x24
                                     DME server <======================= DME connect + join session
 client-status cb: B joined (index 1)                                     join complete: index 1, host 0
 <-- ch 0x240648 join/settings request (0x58) ------- via DME server ---- B
 --> ch 0x24064C answer (4) --------------------------------------------> B
 <-- ch 0x240640 slot request (0x20) to the host ------------------------ B
 OBJECT_GAME_SETTINGS +0xE0/+0xF0 slot -> client; net-object update -> all
 staging room: both ready -> host starts the match (match state, ch 5) and the map loads
 in game: ch 9 player stream every frame (UDP), ch 7 unit messages every 15 frames (TCP),
          ch 1 damage, ch 4 kills, ch 14 weapon fire, ch 15 knockback, ch 3 vehicles, ch 5 state
 end: host broadcasts ch 5 command 0x82 (scores); stats to Medius (LOBBY_CAREER_STATS_UPLOAD_REQUEST 1/0x11,
      LOBBY_LADDER_STATS_WRITE_REQUEST 4/0x0B, LOBBY_CLAN_STATS_UPLOAD_REQUEST 1/0x84,
      LOBBY_PLAYER_STATS_REPORT 1/0x01, LOBBY_GAME_LEAVE_REPORT 1/0x02)
 leave: dme_leave_session, dme_disconnect; LOBBY_PRESENCE_UPDATE(LeftGameWorld) then (JoinedChatWorld)
 log out: LOBBY_SESSION_CLOSE_REQUEST 1/0x05, MediusDisconnect
```


---

---

## 4. The protocol, layer by layer

One table per layer, from the wire the port speaks today down to the game's own messages:
4.1 the OpenRAC server, 4.2 SCE-RT frames, 4.3 Medius, 4.4 DME, 4.5 the in-game channels and
net objects, 4.6 the port's own payloads. Each row: what the item does, its sender and receiver
(functions with PAL addresses), direction, reliability, and one marker; fields in sub-tables.

<a id="l1"></a>
### 4.1 Layer 1: the OpenRAC server wire

Source of truth: OpenRAC/Server `README.md` ("Wire format", "UDP", "Hub and regions") and the
enums `services/common/.../packets/Packets.java` (packet id = ordinal) and `Opcodes.java`
(`Party`, `Error`). Server side: `services/server/.../net/ServerPacketHandler.java` (`handleXxx`),
`party/PartyController.java`, `hub/RegionController.java`, `net/ServerDatagramHandler.java`. Port
side: `port/game/rac3-pal/host/mp_net.c` (`srv_open`, `lobby_request`, `on_frame`, `on_party`,
`dme_relay`, `obj_send`). Markers: **V** seen in the port's runs (box1 server log and the port's
`OPENRAC_RAC3_NET_LOG` trace), **C** read in the server or port code only.

**Framing (C).** TCP, one connection per game (default port 7770). Frame = `u32` big-endian length
(of what follows), `u8` packet id, then `u8` opcode for `PACKET_PARTY` and `PACKET_ERROR`, then the payload. At
most 65,536 bytes a frame. Integers big-endian; a *string* is `s16` length then UTF-8 bytes (-1 =
empty). TCP: reliable and in order; each packet kind has its own rate limit (README "Limits").
The first packet must be `PACKET_HELLO`; anything malformed, unknown or oversized closes the connection.

| Id | Name | Horizon name | Dir | What it does (sender → receiver) | Payload (big-endian) | Port uses | Mark |
|---|---|---|---|---|---|---|---|
| 0 | `PACKET_HELLO` | — | C→S | Greets the server under a display name; opens the session. Port: `srv_open` on the first LOBBY_LOGIN_REQUEST (`lobby_request` 1/0x07); server: `handleHello` creates the `Session` | version string (`rac3-online-1`), name string (the profile's AccountName), client-flags `u8` (optional, [flags](#l1-flags)) | yes | V |
| 0 | `PACKET_HELLO` | — | S→C | The session id. Port `on_frame`: `srv.session`, then the queued LOBBY_LOGIN_ANSWER (1/0x08) | session id `s32` (from 1) | yes | V |
| 1 | `PACKET_PARTY` | — | both | Party requests and notices: `u8` opcode ([table](#l1-party)), then its payload |  | yes | V |
| 2 | `PACKET_CHAT` | — | C→S / S→members | Party chat. Server `handleChat` forwards to every member (sender included) | C→S: message string (1-256); S→C: sender id `s32`, message string | no | C |
| 3 | `PACKET_DATA` | — | C→S / S→C | Host-star game data: a member's goes to the host (target ignored); the host's to target 0 (all others) or one session. Server `handleData` | C→S: target `s32`, bytes to the end; S→C: sender `s32`, bytes | no | C |
| 4 | `PACKET_ERROR` | — | S→C | A refused request; `u8` reason ([table](#l1-error)). Port `on_frame`: answers a pending LOBBY_GAME_JOIN_REQUEST with -977, a pending LOBBY_GAME_CREATE_REQUEST with -987, a pending list with NoResult | reason `u8` (+ family string for 6) | yes | C |
| 5 | `PACKET_TIME` | — | C→S / S→C | Clock sync: the client's time echoed with the server's; also the keep-alive of a client that set `CLIENT_KEEPS_ALIVE` (the port sends it every 2 s; 30 s of silence drops it, `KEEP_ALIVE_IDLE_SECONDS`). Server `handleTime` | C→S: client ms `s64`; S→C: client ms `s64`, server ms `s64` | no | V |
| 6 | `PACKET_UDP` | — | S→C | Sent right after the `PACKET_HELLO` answer: the UDP token and port, the server's [feature bits](#l1-flags), the resume token. Port `on_frame`: keeps only the feature bits (`FEATURE_REGIONS`) | token `s64`, UDP port `s32`, features `u8`, resume token `s64` | yes | V |
| 7 | `PACKET_RELAY` | — | C→S / S→C | Any member to any member, unread by the server (`handleRelay` → `Party.relay`); never over UDP; own rate (`RELAY_PER_SECOND`). The port's whole game room travels in it ([layer 6](#l6)) | C→S: target `s32` (0 = every other member, else a session id), bytes to the end; S→C: sender `s32`, bytes | yes | V |
| 8 | `PACKET_REGIONS` | — | C→hub / hub→C | The region list (the game's city list). Port: `lobby_request` 1/0x8C LOBBY_CITY_LIST_REQUEST sends it when the server's feature bit 16 is set; `on_frame` turns the answer into LOBBY_CITY_LIST_ENTRY records. Server: `RegionController.list` | C→S: family string (`rac3-pal-1` / `rac3-ntsc-1`); S→C: count `u8`, per region: name string, host string (empty = the hub's address), port `s32`, players `s32` | yes | V |
| 9 | `PACKET_REGISTER` | — | region→hub | A region server lists itself with the hub (after `PACKET_HELLO`, then every `REGION_REPORT_SECONDS`). Server `handleRegister` | key string, name string, host string, port `s32`, players `s32` | no (server to server) | V |

<a id="l1-party"></a>**`PACKET_PARTY` opcodes** (`Opcodes.Party`; port `on_party`, server `handleParty` / `PartyController`):

| Op | Name | Horizon name | Dir | What it does | Payload after the opcode | Port uses | Mark |
|---|---|---|---|---|---|---|---|
| 0 | `PARTY_CREATE` | — | C→S | Host a new party. Port: LOBBY_GAME_CREATE_REQUEST (1/0xF4) → `PARTY_CREATE` with the game's name; the `PARTY_INFO` answer completes LOBBY_GAME_CREATE_ANSWER (world id 900) and sends the `PARTY_ADVERT` | description string (optional; sanitised to 48 printable ASCII), family string (optional) | yes | V |
| 1 | `PARTY_JOIN` | — | C→S | Join a party by its 6-character code. Port: LOBBY_GAME_JOIN_REQUEST (1/0xF3) of a listed world → `PARTY_JOIN` with that party's code; `PARTY_INFO` completes LOBBY_GAME_JOIN_ANSWER, `PACKET_ERROR` fails it (-977) | code string, family string (optional) | yes | V |
| 2 | `PARTY_LEAVE` | — | C→S | Leave the party (`PACKET_ERROR` `ERROR_NOT_IN_PARTY` when in none). Port: LOBBY_PRESENCE_UPDATE(2 LeftGameWorld) → `PARTY_LEAVE` | — | yes | V |
| 3 | `PARTY_INFO` | — | S→C | Describes the party to a new member / the creator. Port `on_info`: the room's members in order (DME client indices), the host, completes a pending Create/LOBBY_GAME_JOIN_REQUEST | code string, host id `s32`, count `u8`, per member: id `s32` + name string, description string; then (when meaningful) epoch `s32`, away count `u8` + ids `s32` | yes | V |
| 4 | `PARTY_JOINED` | — | S→members | Someone joined. Port: `room_add`, sends it every object this console owns (`obj_send_all_to`), tells the game (client-status callback kind 0) | id `s32`, name string | yes | V |
| 5 | `PARTY_LEFT` | — | S→members | Someone left. Port: removes it; client-status callback kind 1 | id `s32` | yes | C |
| 6 | `PARTY_END` | — | S→members | The host left: the party is gone. Port: room cleared, every member reported gone | — | yes | C |
| 7 | `PARTY_FIND` | — | C→S | Join the oldest party of the family with room, else host one | family string (optional) | no | C |
| 8 | `PARTY_LIST` | — | C→S / S→C | The family's parties with room (not the asker's own), oldest first, at most 32. Port: LOBBY_GAME_LIST_REQUEST (1/0xEA) → `PARTY_LIST` with adverts; `on_list` → one LOBBY_GAME_LIST_ENTRY record per party whose advert parses (version 1) | C→S: family string, flags `u8` (bit 0 = with adverts); S→C: count `u8`, per party: code string, host name string, members `u8`, max size `u8`, description string, then (adverts) length `u16` + advert bytes ([format](#l6-advert)) | yes | V |
| 9 | `PARTY_HOST_CHANGED` | — | C→S / S→members | C→S (host only): who may follow it, or a new host declining; S→C: the new host. Port: `room.host`, host callback (+0x30) | C→S: decline `u8`, count `u8`, ids `s32`; S→C: new host id `s32`, epoch `s32` | receives | C |
| 10 | `PARTY_RESUME` | — | C→S / S→members | Come back after a drop (clients that set `PACKET_HELLO` bit 0) | C→S: code string, session id `s32`, resume token `s64`; S→C: id `s32`, `u8` 0 dropped / 1 back | no | C |
| 11 | `PARTY_KICK` | — | C→S | Host removes a member | member id `s32` | no | C |
| 12 | `PARTY_KICKED` | — | S→member | The host removed you. Port: as `PARTY_END` | — | yes | C |
| 13 | `PARTY_ADVERT` | — | C→S | Host sets the opaque bytes `PARTY_LIST` shows (≤ 512). Port `lobby_send_advert`: after LOBBY_GAME_CREATE_REQUEST's `PARTY_INFO`, and when a LOBBY_GAME_REPORT changes the game | advert bytes to the end ([format](#l6-advert)) | yes | V |
| 14 | `PARTY_SLOTS` | — | S→members | Each member's stable slot (only to clients with `PACKET_HELLO` bit 1); after `PARTY_INFO` and on every join/leave. Port: `member.index` = slot = DME client index | count `u8`, per member: id `s32` + slot `u8` | yes | V |

<a id="l1-error"></a>**`PACKET_ERROR` reasons** (`Opcodes.Error`, server → client, after packet id 4; the port treats every
reason alike, `on_frame`):

| Reason | Name | Horizon name | When | Mark |
|---|---|---|---|---|
| 0 | `ERROR_PARTY_NOT_FOUND` | — | `PARTY_JOIN` to an unknown code (or another family's, when the client named none) | C |
| 1 | `ERROR_PARTY_FULL` | — | `PARTY_JOIN` to a full party (rac3 families: 8, `FAMILY_PARTY_SIZES`) | C |
| 2 | `ERROR_ALREADY_IN_PARTY` | — | `PARTY_CREATE`/`PARTY_JOIN`/`PARTY_FIND` while in a party | C |
| 3 | `ERROR_NOT_IN_PARTY` | — | `PARTY_LEAVE`, `PACKET_CHAT`, `PARTY_ADVERT`, `PARTY_KICK`, `PARTY_HOST_CHANGED` outside a party | C |
| 4 | `ERROR_NOT_HOST` | — | `PARTY_ADVERT`, `PARTY_KICK`, `PARTY_HOST_CHANGED` from a member | C |
| 5 | `ERROR_RESUME_REFUSED` | — | `PARTY_RESUME` with a gone slot or a wrong token | C |
| 6 | `ERROR_OTHER_GAME` | — | `PARTY_JOIN` into another family's party; the party's family string follows | C |

<a id="l1-udp"></a>**`DATAGRAM_` kinds over UDP** (same port; optional; `ServerDatagramHandler`). Client → server: session id `s32`,
token `s64`, kind `u8`, payload; server → client: kind `u8`, payload. No length prefix, ≤ 1,200
bytes, big-endian, unreliable and unordered. The rac3 port sends none (its room uses `PACKET_RELAY`, TCP).

| Kind | Name | Horizon name | Dir | Payload | Mark |
|---|---|---|---|---|---|
| 0 | `DATAGRAM_BIND` | — | C→S | none: binds/rebinds the sender's address to the session (NAT keep-alive) | C |
| 1 | `DATAGRAM_STATE` | — | C→S / S→C | C→S: target `s32`, bytes; S→C: sender `s32`, bytes. `PACKET_DATA`'s routing; falls back to TCP `PACKET_DATA` for an unbound receiver | C |

<a id="l1-flags"></a>**Flag bytes (C).** `PACKET_HELLO`'s client flags: bit 0 (1) `CLIENT_RESUMES` (the client can `PARTY_RESUME`); bit 1
(2) `CLIENT_SLOTS` (send me `PARTY_SLOTS`), (4) `CLIENT_KEEPS_ALIVE` (I send `PACKET_TIME` every few seconds; drop me when I go quiet); the port sets 2 | 4. `PARTY_LIST`'s request flag: bit 0 (1)
`LIST_ADVERTS`. `PACKET_UDP`'s server feature bits (`Constants.FEATURES`): bit 0 (1) `FEATURE_KICK`,
bit 1 (2) `FEATURE_RESUME`, bit 2 (4) `FEATURE_ADVERT`, bit 3 (8) `FEATURE_RELAY`, bit 4 (16)
`FEATURE_REGIONS`. Claimed by the UI work but not in the enums yet (STATUS.md): packet id 10
`PACKET_UI`, feature bit 5 (32) `FEATURE_UI`.

**Game families (C).** `PARTY_CREATE`, `PARTY_JOIN`, `PARTY_FIND`, `PARTY_LIST` end with the asker's family string; the
rac3 port sends `rac3-pal-1` (PAL) or `rac3-ntsc-1` (NTSC-U), so the two discs list only their own
games.

<a id="l2"></a>
### 4.2 Layer 2: SCE-RT transport frames (rt_msg_client 2.7.2)

The retail transport under Medius and DME. The port replaces the layers above it (Medius at
`MediusSendLobbyMessage`, DME at its API), so **no RT frame is ever built or parsed in the port's
runs: nothing here is V**. Everything below was read in the code (C) unless marked: receive
dispatch `rt_dispatch` **0x0016CDD8** (jump table 0x001BBF80, index id-2, 29 entries for ids
0x02-0x1E; anything else goes to the default 0x0016D030 "ignore", result 3), frame builder
`rt_frame_flush` **0x0016E618**, the one queueing function `rt_queue_msg(conn, id, target, data,
len, flush)` **0x0016CA28** (10 call sites, every constant id below is read at one of them,
script `protocol-v2/rt/jargs2.py`), app send `rt_send_app` **0x0016ACE0** (API slot +0x30).

<a id="l2-frame"></a>**TCP frame (C, 0x0016E618 build / 0x0016CC58 parse).** All integers little-endian
(0x00152418 put16, 0x00152478 put32, 0x00152440 get16, 0x001524B0 get32). Frames ≤ 0x249 bytes, app
data ≤ 0x200 a message. Messages of one type and target are aggregated in the connection's queue
(+0x80) and flushed together. The aux UDP stream carries the same frames.

| Off | Size | Type | Field | Values |
|---|---|---|---|---|
| 0 | 1 | u8 | id \| 0x80 when encrypted | ids below |
| 1 | 2 | u16 | payload length (bytes after the header) | ≤ 0x246 |
| 3 | 4 | u32 | hash (only when bit 7 is set and length > 0): RC4-variant/SHA1 4-byte tag (rt_crypt table 0x001D3660: +0x3C encrypt, +0x40 decrypt) | |
| 3 / 7 | n | | payload; for 0x03 and 0x1B the first u16 (target client) is sent in clear, the cipher covers the rest; encrypted payloads padded to 64 when the context's hash has bits 29-31 set | |

Reliability: the TCP stream (connection record +0x78) is reliable and ordered; the aux UDP stream
(+0x74) is used by `rt_send_app` for kind 0x80 only, when it is up. Ordering on UDP: none (DME's
player stream carries its own sequence).

<a id="l2-ids"></a>

| Id | Name | Horizon name | Dir | Sender (when) | Receiver / handler (what it does) | Body | Mark |
|---|---|---|---|---|---|---|---|
| 0x00 | `TRANSPORT_TCP_CONNECT_REQUEST` | *RT_MSG_CLIENT_CONNECT_TCP* | C→S | 0x0016C710 when the connection has no aux UDP (+4 == 0), first message on a new socket | server | [connect body](#l2-connect) | C |
| 0x01 | `TRANSPORT_DISCONNECT_NOTICE` | *RT_MSG_CLIENT_DISCONNECT* | — | never sent (no 0x0016CA28 site passes it) | ignored on receive (default) | — | C (unused) |
| 0x02 | `TRANSPORT_SEND_TO_WORLD` | *RT_MSG_CLIENT_APP_BROADCAST* | C→S→C | `rt_send_app` with target 0xFFFE (DME dest -1) | 0x0016D650 app delivery (state 8): DME / Medius dispatch 0x00152FB8 | app bytes `[class][id][payload]...` | C |
| 0x03 | `TRANSPORT_SEND_TO_CLIENT` | *RT_MSG_CLIENT_APP_SINGLE* | C→S→C | `rt_send_app` with a client index | 0x0016D650 | u16 target (clear), app bytes | C |
| 0x04 | `TRANSPORT_SEND_TO_CLIENT_LIST` | *RT_MSG_CLIENT_APP_LIST* | — | never sent | ignored | — | C (unused) |
| 0x05 | `TRANSPORT_ECHO` | *RT_MSG_CLIENT_ECHO* | C↔S | 0x0016BE08 (the RT update) when the echo interval (+0x2C ms) passed; counts outstanding echoes (+0x38) | 0x0016CF90: decrements +0x38 | 1 byte 0xA5 | C |
| 0x06 | `TRANSPORT_CONNECT_REFUSED` | *RT_MSG_SERVER_CONNECT_REJECT* | S→C | server | 0x0016D080: error 2..9 by reason, connection fails | 3-byte version, u8 reason 0..6 | C |
| 0x07 | `TRANSPORT_TCP_CONNECT_ACCEPTED` | *RT_MSG_SERVER_CONNECT_ACCEPT_TCP* | S→C | server, answer to 0x00/0x15 | 0x0016D080: checks the version (0x0016EA50), my client index → +0x54, u16 → +0x58, IP string → +0x60, `crypt[+0x58](ctx, index)`; state 5 (aux UDP next) or 7 | [accept body](#l2-connect) | C |
| 0x08 | `TRANSPORT_CLIENT_CONNECTED_NOTICE` | *RT_MSG_SERVER_CONNECT_NOTIFY* | S→C | server | 0x0016D4D0 → the RT connect-notify callback (DME 0x0014D098: sends DME 0x10 ROOM_PEER_HELLO to it) | 0x52 bytes (checked): u16 client index, 16-byte address (parsed by 0x00151FA8), 64-byte public key (copied) | C |
| 0x09 | `TRANSPORT_CLIENT_DISCONNECTED_NOTICE` | *RT_MSG_SERVER_DISCONNECT_NOTIFY* | S→C | server | 0x0016D4D0 → disconnect callback (DME: client left, 0x001725C0) | 0x12 bytes (checked): u16 client index, 16-byte address | C |
| 0x0A | `TRANSPORT_FROM_SERVER` | *RT_MSG_SERVER_APP* | S→C | server (Medius replies; DME server messages) | 0x0016D650 → 0x00152FB8 | app bytes | C |
| 0x0B | `TRANSPORT_SEND_TO_SERVER` | *RT_MSG_CLIENT_APP_TOSERVER* | C→S | `rt_send_app` with target 0xFFFF: every Medius request (`MediusSendLobbyMessage` → 0x0014AC48) | server | app bytes | C |
| 0x0C | `TRANSPORT_UDP_APP_MESSAGE` | *RT_MSG_UDP_APP* | — | never sent (UDP uses 0x02/0x03/0x0B) | ignored | — | C (unused) |
| 0x0D | `TRANSPORT_RECEIVE_FLAGS_SET` | *RT_MSG_CLIENT_SET_RECV_FLAG* | C→S | 0x0016B210 (API slot +0x50) | server | u16 flag | C (I: Horizon says RT_RECV_FLAG bits 1 broadcast, 2 list, 4 single, 8 notification) |
| 0x0E | `TRANSPORT_AGGREGATION_TIME_SET` | *RT_MSG_CLIENT_SET_AGG_TIME* | C→S | 0x0016B3F8 (API slot +0x5C) | server | u16 ms | C |
| 0x0F | `TRANSPORT_FLUSH_ALL` | *RT_MSG_CLIENT_FLUSH_ALL* | C→S | 0x0016BE08 when aggregation is on (+0x40) and its time (+0x3C) passed, queue target 0xFFFE | server | none | C |
| 0x10 | `TRANSPORT_FLUSH_TO_CLIENT` | *RT_MSG_CLIENT_FLUSH_SINGLE* | C→S | 0x0016BE08, queue target a client | server | u16 target | C |
| 0x11 | `TRANSPORT_FORCED_DISCONNECT` | *RT_MSG_SERVER_FORCED_DISCONNECT* | S→C | server | 0x0016D728: connection closed | (not read; I: Horizon says u8 reason 0 none … 7 banned) | C |
| 0x12 | `TRANSPORT_CLIENT_PUBLIC_KEY` | *RT_MSG_CLIENT_CRYPTKEY_PUBLIC* | C→S | 0x0016C6C8 (encrypted connect) | server | 64-byte key (0x00151E40) | C (position in the handshake: I) |
| 0x13 | `TRANSPORT_PEER_KEY` | *RT_MSG_SERVER_CRYPTKEY_PEER* | S→C | server | 0x0016D7C8: installs the peer key | key bytes | C |
| 0x14 | `TRANSPORT_WORLD_KEY` | *RT_MSG_SERVER_CRYPTKEY_GAME* | S→C | server | `crypt[+0x24](ctx, ..., data)`: sets the world's key | key bytes | C |
| 0x15 | `TRANSPORT_TCP_CONNECT_WITH_UDP_REQUEST` | *RT_MSG_CLIENT_CONNECT_TCP_AUX_UDP* | C→S | 0x0016C710 (DME world connections) | server | [connect body](#l2-connect) | C |
| 0x16 | `TRANSPORT_UDP_HELLO` | *RT_MSG_CLIENT_CONNECT_AUX_UDP* | C→S (UDP) | 0x0016C908 on 0x18; resent every 200 ms while in state 6 (0x0016BE08) | server | 29 bytes: 3-byte version, u16 world (+0x18), u32 app id, 16-byte IP string, u16 UDP port, u16 my index | C |
| 0x17 | `TRANSPORT_UDP_READY` | *RT_MSG_CLIENT_CONNECT_READY_AUX_UDP* | C→S | 0x0016D2E0 on 0x19 | server | none | C |
| 0x18 | `TRANSPORT_UDP_SERVER_ADDRESS` | *RT_MSG_SERVER_INFO_AUX_UDP* | S→C | server | 0x0016D888: sends 0x16, state 6 | 16-byte IP string, u16 UDP port | C |
| 0x19 | `TRANSPORT_UDP_ACCEPTED` | *RT_MSG_SERVER_CONNECT_ACCEPT_AUX_UDP* | S→C | server | 0x0016D2E0 (state 6): sends 0x17 | (not read) | C |
| 0x1A | `TRANSPORT_CONNECT_COMPLETE` | *RT_MSG_SERVER_CONNECT_COMPLETE* | S→C | server | 0x0016D438: u16 → +0x58, state 8 (app messages flow), connect callback `(+0x80)(+0x7C, 1, index, u16, 0)` | u16 | C |
| 0x1B | `TRANSPORT_PEER_KEY_RELAY` | *RT_MSG_CLIENT_CRYPTKEY_PEER (Horizon: RT_MSG_CLIENT_CRYPTKEY_PEER)* | S→C | server | same handler as 0x13 (0x0016CFB0) | u16 target (clear), key | C |
| 0x1C | `TRANSPORT_SERVER_TEXT_NOTICE` | *RT_MSG_SERVER_SYSTEM_MESSAGE* | S→C | server | 0x0016D958 | (not read; I: Horizon says u8 severity, u8 encoding, u8 language, u8 end flag, text) | C |
| 0x1D | `TRANSPORT_MEMORY_QUERY_REQUEST / TRANSPORT_MEMORY_QUERY_ANSWER` | *RT_MSG_SERVER_CHEAT_QUERY* | S→C, C→S | server; reply by 0x0016D9A0 with id 0x1D (13 bytes) | 0x0016D9A0: hashes/reads memory (crypt +0x64..+0x70) | (I: Horizon says u8 query type, s32 sequence, u32 address, s32 length, data) | C |
| 0x1E | `TRANSPORT_MEMORY_WRITE` | *RT_MSG_SERVER_MEMORY_POKE* | S→C | server | 0x0016E168: memcpy to the address | u32 address, u32 length, bytes | C |

<a id="l2-connect"></a>**Connect bodies (C).**

| Message | Off | Size | Type | Field |
|---|---|---|---|---|
| 0x00 / 0x15 | 0 | 3 | u8[3] | version `01 08 01` (0x0016EA80) |
| | 3 | 2 | u16 | world id (connection +0x18; the Medius world of the DME connect) |
| | 5 | 4 | u32 | application id (`[[0x001A1B38]]`: 10683 = 0x29BB PAL at 0x001A3358, 10684 NTSC-U) |
| | 9 | 64 | u8[64] | session key encrypted with the server key (`crypt[+0x78]` makes it, `crypt[+0x48]` encrypts); 64 zero bytes when crypto is off |
| | 0x49 | n | | access-key blob (connection +0x34 pointer, +0x38 length) |
| 0x07 | 0 | 3 | u8[3] | server version (checked) |
| | 3 | 2 | u16 | my client index → +0x54 |
| | 5 | 2 | u16 | → +0x58 (I: Horizon says PlayerCount for Medius ≤ 1.08) |
| | 7 | 16 | char[16] | my IP as the server sees it → +0x60 |

Handshake (C): socket (rt_comm slot +0x14) → **0x15** (or 0x00) → **0x07** (or 0x06) → state 5 →
**0x18** → **0x16** over UDP → state 6 → **0x19** → **0x17** → **0x1A** → state 8; then 0x05 echoes and
0x0F/0x10 flushes from 0x0016BE08. App messages (Medius, DME) ride in 0x0B (to the server), 0x0A
(from it), 0x02/0x03 (via it to other clients).

<a id="l3"></a>
### 4.3 Layer 3: Medius lobby requests and responses (Medius client 2.7.1)

Library at core.text 0x0017CD78-0x00183C40 (request API 0x0017CD78-0x0017FC7C, response handlers
0x0017FFC8-0x00182600, registration 0x00182610). Names: the binary has no message-name strings, so
every message carries our name and, in the Horizon name column, the public one (from public Medius write-ups, used only to recognise the
message), next to the binary's evidence: the library's own struct size for that (class, type) (the
handler's return value / the send size, all 268 agree with the public layouts), the handler it
registers, and the call sites; and a descriptive name from what the code does with it.

**Framing (C, 0x00152FB8).** One RT app message (RT 0x0B up, 0x0A down) carries one or more records
`[u8 class][u8 type][struct of exactly <size> bytes]`, packed. Class 1 = Lobby, class 4 = LobbyExt
*(public names)*. Structs are little-endian; strings are fixed NUL-terminated arrays.
**Request header (C, every sender):** `+0x00 MessageID char[21]` (the game writes the 2-byte string
"1"; LOBBY_KEEP_ALIVE_REQUEST and the core LOBBY_ACCOUNT_INFO_REQUEST write an empty or 1-byte id), `+0x15 SessionKey char[17]`
(requests that carry one, copied by 0x001206F4 / MediusGetSessionKey 0x0017DF20), pad to 0x28.
**Response header (C):** `+0x00 MessageID[21]`, pad, `+0x18 s32 StatusCode` (except LOBBY_CITY_LIST_REQUEST,
below). The library never matches MessageID: the response's (class, type) picks the callback slot
(0x001EB570 + 4·slot, user data 0x001EB380 + 4·slot). Lists: one record per item, the last with
EndOfList = 1 (an empty list: one record, StatusCode 1 NoResult, EndOfList 1).
**Send** (C): every request goes through `MediusSendLobbyMessage(class, type, size, data)`
**0x0017DFA8** (77 call sites in 74 functions) → `0x0014AC48(0x40, [0x001A30DC], 0xFFFF, ...)`:
reliable (TCP), to the lobby server, in order. **Receive** (C): MediusUpdate 0x0017E208 (and its
twins 0x0017E1A0/0x0017E2D0, all from the net update 0x001959F8, pumped by the game through
0x00195410) → 0x00152FB8 → the library handler → the game's callback, mostly through the core
trampolines 0x00197638-0x001980D8 into the level-39 table 0x0022F280 (filled by L39_006D7930).
Marks: **V** request sent and its reply consumed in the port's runs (logged by
`OPENRAC_RAC3_NET_LOG`), **C** read in the code, **I** inferred.

<a id="l3-status"></a>**StatusCode values** (C: the name table MediusCallbackStatus at 0x001A3128, not referenced
by code): 0 Success, 1 NoResult, 2 RequestAccepted, 3 WorldCreatedSizeReduced, 4 Pass; -1000
BeginSessionFailed, -999 AccountAlreadyExists, -998 AccountNotFound, -997 AccountLoggedIn, -996
EndSessionFailed, -995 LoginFailed, -994 RegistrationFailed, -993 IncorrectLoginStep, -992
AlreadyLeaderOfClan, -991 WMError, -990 NotClanLeader, -989 PlayerNotPrivileged, -988 DBError,
-987 DMEError, -986 ExceedsMaxWorlds, -985 RequestDenied, -984/-983/-982 Set/Clear/GetGameListFilter
Failed, -981 NumFiltersAtMax, -980 FilterNotFound, -979 InvalidRequestMsg, -978 InvalidPassword,
-977 GameNotFound, -976 ChannelNotFound, -975 GameNameExists, -974 ChannelNameExists, -973
GameNameNotFound, -972 PlayerBanned, -971 ClanNotFound, -970 ClanNameInUse, -969 SessionKeyInvalid,
-968 TextStringInvalid, -967 FilterFailed, -966 Fail. API return codes (MediusErrorCode, 0x001A3270):
0 None, -5 SendingMessage, -11 SessionInactive, -14 NoResult, -15 NotInitialized, -16
NotConnected, -17 AlreadyConnected, -19 CallMediusCloseBareBeforeReinitializing (others in parts/lobby.md).

**How many messages.** The library registers 267 (class, type) handlers (0x00182610). It can send
**77 requests** (one per (class, type); the "79" is not a count in the binary: 77 requests plus
LOBBY_CHAT_LINE_NOTICE and LOBBY_SERVER_ERROR_NOTICE, the two unsolicited messages the game registers callbacks for, is
the nearest reading). The game calls 71 of them; the other 6 are library-internal (LOBBY_SELF_HOSTED_GAME_CREATE_REQUEST,
the four file transfers, LOBBY_FILE_CHUNK). It handles 5 unsolicited server messages. The
remaining 117 registered types are never sent or used by this build ([list](#l3-unused)).

<a id="l3-exchanges"></a>
#### 4.3.1 Every exchange

Sender = the API function ← the game function that fills the struct (L39 = level 39, the lobby;
core = the executable's net code). Receiver = the library handler → the game's handler. Sizes are
request / reply. Fields: the offsets the game writes (C), beyond the header; field tables linked. On
the clan, buddy, ignore and ladder rows the field *names* come from the public layouts (I) unless the
game's use shows them; their offsets and constants are read at the call sites (`protocol-v2/medius/reqfields.txt`).

| Req → reply | Name | Horizon name | Size | Sender: when | Receiver: what it does | Fields | Mark |
|---|---|---|---|---|---|---|---|
| 1/0x00 → none | `LOBBY_GAME_REPORT` | *WorldReport0* | 0x180 / — | 0x0017F010 ← core 0x00196478 (fills 0x00196380): at create (L39_006D3A30), every 15 s and when flagged (net update 0x001959F8, host only), and at leave with WorldStatus 3 (0x00196038) | server: the game's listing (LOBBY_GAME_LIST_ENTRY and LOBBY_GAME_INFO_ANSWER echo it) | [table](#l3-worldreport) | V |
| 1/0x01 → none | `LOBBY_PLAYER_STATS_REPORT` | *PlayerReport* | 0x118 / — | 0x0017EFB0 ← core 0x00196218: in a world, every 15 s or when net+0x47C is set | server | +0x00 SessionKey, +0x14 MediusWorldID, +0x18 0x34 bytes of stat words from net+0x88, rest of Stats[256] zero | V |
| 1/0x02 → none | `LOBBY_GAME_LEAVE_REPORT` | *EndGameReport* | 0x7C / — | 0x0017F070 ← core 0x00196038 (leave: L39_006D40B0, L39_006AEF70, L39 0x005FBF18), after the final LOBBY_GAME_REPORT | server | +0x00 SessionKey, +0x14 MediusWorldID, +0x18 "" , +0x58 "", +0x78 0 (offsets C; I: Horizon says WinningTeam[64], WinningPlayer[32], FinalScore) | V |
| 1/0x03 → 1/0x04 | `LOBBY_SESSION_OPEN_REQUEST / LOBBY_SESSION_OPEN_ANSWER` | *SessionBegin / SessionBeginResponse* | 0x1C / 0x30 | 0x0017E370 ← L39_006DC7C8: after each connect to the authentication server | 0x00180020 copies +0x1C SessionKey to 0x001A30E8 when status 0 → L39_006DCD38: clears conn+0x99, keeps the reply | [table](#l3-session) | V |
| 1/0x05 → 1/0x06 | `LOBBY_SESSION_CLOSE_REQUEST / LOBBY_SESSION_CLOSE_ANSWER` | *SessionEnd / SessionEndResponse* | 0x26 / 0x1C | 0x0017E3E8 ← L39_006DCAD0: log out | 0x00180088 clears the key → L39_006DCCE0: status | header only | C |
| 1/0x07 → 1/0x08 | `LOBBY_LOGIN_REQUEST / LOBBY_LOGIN_ANSWER` | *AccountLogin / AccountLoginResponse* | 0x66 / 0xC4 | 0x0017E4E0 ← L39_006CCE90 (from L39_006CCD08, login screen 0x0068C4E0) | 0x001800E8 saves reply +0x44 to 0x001E7E08 → L39_006CD2C8: AccountID → net+0x34; -997 → log out and retry | [table](#l3-session) | V |
| 1/0x11 → 1/0x12 | `LOBBY_CAREER_STATS_UPLOAD_REQUEST / LOBBY_CAREER_STATS_UPLOAD_ANSWER` | *AccountUpdateStats / AccountUpdateStatsResponse* | 0x126 / 0x1C | 0x0017E600 ← core 0x00196700: net update when not in a world and net+0x47C set; L39 0x006E0B38 | 0x00180288 → core 0x00196CF0: net+0x628 = 0, net+0x62C = status | +0x26 Stats[256] (the game's stat words, de-obfuscated from 0x00240410..) | V |
| 1/0x15 → 1/0x16 | `LOBBY_LOGOUT_REQUEST / LOBBY_LOGOUT_ANSWER` | *AccountLogout / AccountLogoutResponse* | 0x26 / 0x1C | 0x0017E568 ← L39_006CD038: after -997 at login, or -991 | 0x00180148 → L39_006CD488: status (-988/-991 special) | header only | C |
| 1/0x17 → 1/0x18 | `LOBBY_ACCOUNT_ID_REQUEST / LOBBY_ACCOUNT_ID_ANSWER` | *AccountGetID / AccountGetIDResponse* | 0x46 / 0x20 | 0x0017F468 ← L39_006D9C90 (buddy/clan screens) | 0x00181220 → L39_006D9E10 | +0x26 AccountName[32]; reply +0x18 AccountID, +0x1C StatusCode (order as copied by L39_006D9C90) | C |
| 1/0x1B → 1/0x1C | `LOBBY_KEEP_ALIVE_REQUEST / LOBBY_KEEP_ALIVE_ANSWER` | *GetMyIP / GetMyIPResponse* | 0x26 / 0x30 | 0x0017F360 ← core 0x001959F8: online, ≥ 2 clients, the player stream silent > 1.5 s, none pending | 0x001810E0 → core 0x00197638: clears the pending byte, net+0x7D0 = net time; the IP (+0x1C) is not read | header (empty MessageID) | V |
| 1/0x1F → 1/0x20 | `LOBBY_SELF_HOSTED_GAME_CREATE_REQUEST / LOBBY_SELF_HOSTED_GAME_CREATE_ANSWER` | *CreateGameOnSelf / CreateGameOnSelfResponse* | 0xEC / 0x20 | 0x0017DC48 (deferred sender; the tag 0x12 that would queue it has no setter) | 0x00180350 (slot 16) | addresses at +0xBC | C (never sent) |
| 1/0x25 → 1/0x26 | `LOBBY_CHANNEL_JOIN_REQUEST / LOBBY_CHANNEL_JOIN_ANSWER` | *JoinChannel / JoinChannelResponse* | 0x4C / 0xB8 | 0x0017E920 ← L39_006DA680(chan, world, password): after picking a channel | 0x00180428 → L39_006DA878: copies the reply; the game reconnects to its +0x1C NetConnectionInfo, LOBBY_PRESENCE_UPDATE(1), 0x001A5D48 = world | [table](#l3-channels) | V |
| 1/0x2F → 1/0x30 | `LOBBY_GAME_PLAYER_LIST_REQUEST / LOBBY_GAME_PLAYER_LIST_ENTRY` | *GameWorldPlayerList / GameWorldPlayerListResponse* | 0x2C / 0x148 ×n | 0x0017E9B8 ← L39_006D45B8, L39_006D48C8 (game info and staging screens) | 0x00180538 → L39_006D4688 / L39_006D4A68: one record per player | [table](#l3-games) | C |
| 1/0x31 → 1/0x32 | `LOBBY_ACCOUNT_INFO_REQUEST / LOBBY_ACCOUNT_INFO_ANSWER` | *PlayerInfo / PlayerInfoResponse* | 0x2C / 0x148 | 0x0017EA38 ← L39_006D9328 (+0x28 AccountID), core 0x00199C80 (while the host builds OBJECT_GAME_SETTINGS) | 0x00180578 → L39_006D9808: copies 0x148 to its context, clears the wait byte | req +0x28 AccountID; reply fields: [table](#l3-misc) | V |
| 1/0x33 → 1/0x34 | `LOBBY_GAME_INFO_REQUEST / LOBBY_GAME_INFO_ANSWER` | *GameInfo0 / GameInfoResponse0* | 0x2C / 0x18C | 0x0017EAB8 ← L39_006D4188, L39_006D4380 (game details; the host's create check polls it ≤ 21 times) | 0x001805B8 → L39_006D4278 / L39_006D44F0: copies; the host compares it with its LOBBY_GAME_REPORT | [table](#l3-games) | V |
| 1/0x35 → 1/0x36 | `LOBBY_CHANNEL_INFO_REQUEST / LOBBY_CHANNEL_INFO_ANSWER` | *ChannelInfo / ChannelInfoResponse* | 0x2C / 0x64 | 0x0017EB38 ← L39_006DB750 | 0x001805F8 → L39_006DB890: copies 0x64 to channel ctx+0xF8 | req +0x28 MediusWorldID; reply: [table](#l3-misc) | V |
| 1/0x39 → 1/0x3A | `LOBBY_PLAYER_FIND_REQUEST / LOBBY_PLAYER_FIND_ANSWER` | *FindPlayer / FindPlayerResponse* | 0x50 / 0x70 | 0x0017EBB8 ← L39_006D98C8 (by id), L39_006D9A40 (by name) | 0x00180678 → L39_006D9BC0 | +0x28 search type (0 id, 1 name: C, agrees with Horizon), +0x2C AccountID, +0x30 name[32] | C |
| 1/0x3B → none | `LOBBY_CHAT_LINE` | *ChatMessage* | 0x70 / — | 0x0017EC38 ← L39_006DA090 (to all), L39_006D9E68 (whisper), after LOBBY_CHAT_TEXT_CHECK_REQUEST | server (others get LOBBY_CHAT_LINE_NOTICE) | +0x28 type (0 broadcast, 1 whisper: C, agrees with Horizon), +0x2C target (-1 all), +0x30 Message[64] | C |
| 1/0x3F → 1/0x40 | `LOBBY_BUDDY_ADD_REQUEST / LOBBY_BUDDY_ADD_ANSWER` | *AddToBuddyList / AddToBuddyListResponse* | 0x2C / 0x1C | 0x0017ECA8 ← L39_006CDC30 | 0x00180778 → L39_006CDD90 | +0x28 AccountID | C |
| 1/0x41 → 1/0x42 | `LOBBY_BUDDY_REMOVE_REQUEST / LOBBY_BUDDY_REMOVE_ANSWER` | *RemoveFromBuddyList / RemoveFromBuddyListResponse* | 0x2C / 0x1C | 0x0017ED28 ← L39_006CDDB0 | 0x001807B8 → L39_006CDF18 | +0x28 AccountID | C |
| 1/0x47 → 1/0x48 | `LOBBY_LICENCE_TEXT_REQUEST / LOBBY_LICENCE_TEXT_PART` | *Policy / PolicyResponse* | 0x2C / 0x120 ×n | 0x0017EF30 ← L39_006DDE00 (licence screen 0x006AF318) | 0x00180950 → L39_006DDF78: appends +0x1C Text[256] until +0x11C EndOfText | [table](#l3-session) | V |
| 1/0x49 → none | `LOBBY_PRESENCE_UPDATE` | *UpdateUserState* | 0x18 / — | 0x0017E310 ← L39_006DD060 (1 after each lobby connect), L39_006D40B0 (2 then 1 on leaving a game) | server | [table](#l3-session) | V |
| 1/0x4B → 1/0x4D | `LOBBY_NEWS_REQUEST / LOBBY_NEWS_ENTRY` | *GetAnnouncements / GetAnnouncementsResponse* | 0x2C / 0x40C ×n | 0x0017F0D8 ← L39_006DD860 (arg 0) | 0x001809A0 → L39_006DD9B0: shows the text, or parses it as lobby settings when it starts with "{RATCHET3_LOBBY_SETTINGS}" ([4.3.3](#l3-settings)) | +0x28 ApplicationID (0x00183BDC); reply [table](#l3-session) | V |
| 1/0x4C → 1/0x4D | `LOBBY_ALL_NEWS_REQUEST / LOBBY_NEWS_ENTRY` | *GetAllAnnouncements / GetAnnouncementsResponse* | 0x2C / 0x40C ×n | 0x0017F158 ← L39_006DD860 (arg 1) | as 1/0x4B | as 1/0x4B | V |
| 1/0x4E → 1/0x4F | `LOBBY_GAME_LIST_FILTER_ADD_REQUEST / LOBBY_GAME_LIST_FILTER_ADD_ANSWER` | *SetGameListFilter0 / SetGameListFilterResponse0* | 0x34 / 0x1C | 0x0017F1D8 ← L39_006D52D0(list, field, op, value), from L39_006CC930 per active search option | 0x001809E0 → L39_006D5440: status | [table](#l3-games) | V |
| 1/0x50 → 1/0x51 | `LOBBY_GAME_LIST_FILTER_REMOVE_REQUEST / LOBBY_GAME_LIST_FILTER_REMOVE_ANSWER` | *ClearGameListFilter0 / ClearGameListFilterResponse* | 0x2C / 0x1C | 0x0017F258 ← L39_006D5460(list, id) | 0x00180A20 → L39_006D55C0: 0 or -980 both count as cleared | +0x28 FilterID | V |
| 1/0x54 → 1/0x55 | `LOBBY_CLAN_CREATE_REQUEST / LOBBY_CLAN_CREATE_ANSWER` | *CreateClan / CreateClanResponse* | 0x4C / 0x20 | 0x0017CD78 ← L39_006CEEE0 | 0x00180AA0 → L39_006CF080 | +0x28 AppID, +0x2C ClanName[32] | C |
| 1/0x56 → 1/0x57 | `LOBBY_CLAN_DISBAND_REQUEST / LOBBY_CLAN_DISBAND_ANSWER` | *DisbandClan / DisbandClanResponse* | 0x2C / 0x1C | 0x0017CDF8 ← L39_006CF0A0 | 0x00180AE0 → L39_006CF218 | +0x28 ClanID | C |
| 1/0x58 → 1/0x59 | `LOBBY_CLAN_BY_ID_REQUEST / LOBBY_CLAN_BY_ID_ANSWER` | *GetClanByID / GetClanByIDResponse* | 0x30 / 0x168 | 0x0017CEF8 ← L39_006D31B8 | 0x00180B20 → L39_006D3318 | +0x28 AppID, +0x2C ClanID | C |
| 1/0x5A → 1/0x5B | `LOBBY_CLAN_BY_NAME_REQUEST / LOBBY_CLAN_BY_NAME_ANSWER` | *GetClanByName / GetClanByNameResponse* | 0x4C / 0x148 | 0x0017CE78 ← L39_006D2FA0 | 0x00180B60 → L39_006D30F8 | +0x28 AppID, +0x2C ClanName[32] | C |
| 1/0x5C → 1/0x5D | `LOBBY_CLAN_LEADER_TRANSFER_REQUEST / LOBBY_CLAN_LEADER_TRANSFER_ANSWER` | *TransferClanLeadership / TransferClanLeadershipResponse* | 0x4C / 0x1C | 0x0017CF78 ← L39_006CF238 | 0x00180BA0 → L39_006CF3C8 | +0x28 new leader AccountID, +0x2C name[32] | C |
| 1/0x60 → 1/0x61 | `LOBBY_CLAN_MEMBER_REMOVE_REQUEST / LOBBY_CLAN_MEMBER_REMOVE_ANSWER` | *RemovePlayerFromClan / RemovePlayerFromClanResponse* | 0x30 / 0x1C | 0x0017CFF8 ← L39_006D0228 | 0x00180C20 → L39_006D0398 | +0x28 AccountID, +0x2C ClanID | C |
| 1/0x62 → 1/0x63 | `LOBBY_CLAN_INVITE_REQUEST / LOBBY_CLAN_INVITE_ANSWER` | *InvitePlayerToClan / InvitePlayerToClanResponse* | 0xF4 / 0x1C | 0x0017D078 ← L39_006CFEA8 | 0x00180C60 → L39_006D0058 | +0x28 AccountID, +0x2C invite message | C |
| 1/0x64 → 1/0x65 | `LOBBY_CLAN_INVITATION_LIST_REQUEST / LOBBY_CLAN_INVITATION_ENTRY` | *CheckMyClanInvitations / CheckMyClanInvitationsResponse* | 0x30 / 0x118 ×n | 0x0017D0F8 ← L39_006D1B40 | 0x00180CA0 → L39_006D1BF8 | +0x28 Start = 1, +0x2C PageSize | C |
| 1/0x66 → 1/0x67 | `LOBBY_CLAN_INVITATION_REPLY_REQUEST / LOBBY_CLAN_INVITATION_REPLY_ANSWER` | *RespondToClanInvitation / RespondToClanInvitationResponse* | 0xF8 / 0x1C | 0x0017D1F8 ← L39_006D03B8 | 0x00180D20 → L39_006D0550 | +0x28 invitation id, +0x2C response, +0x30 message | C |
| 1/0x68 → 1/0x69 | `LOBBY_CLAN_INVITATION_REVOKE_REQUEST / LOBBY_CLAN_INVITATION_REVOKE_ANSWER` | *RevokeClanInvitation / RevokeClanInvitationResponse* | 0x2C / 0x1C | 0x0017D278 ← L39_006D0078 | 0x00180D60 → L39_006D0208 | +0x28 AccountID | C |
| 1/0x6A → 1/0x6B | `LOBBY_CLAN_CHALLENGE_REQUEST / LOBBY_CLAN_CHALLENGE_ANSWER` | *RequestClanTeamChallenge / RequestClanTeamChallengeResponse* | 0xF4 / 0x1C | 0x0017D4F8 ← L39_006CF920 | 0x00180DA0 → L39_006CFAD0 | +0x28 ClanID, +0x2C message | C |
| 1/0x6C → 1/0x6D | `LOBBY_CLAN_MESSAGE_LIST_REQUEST / LOBBY_CLAN_MESSAGE_ENTRY` | *GetMyClanMessages / GetMyClanMessagesResponse* | 0x34 / 0xEC ×n | 0x0017D2F8 ← L39_006D0DD0 | 0x00180DE0 → L39_006D12C8 | +0x28 ClanID, +0x2C Start = 1, +0x30 PageSize | C |
| 1/0x6E → 1/0x6F | `LOBBY_CLAN_MESSAGE_POST_REQUEST / LOBBY_CLAN_MESSAGE_POST_ANSWER` | *SendClanMessage / SendClanMessageResponse* | 0xEE / 0x1C | 0x0017D378 ← L39_006CF3E8 | 0x00180E20 → L39_006CF588 | +0x26 message | C |
| 1/0x70 → 1/0x71 | `LOBBY_CLAN_MESSAGE_EDIT_REQUEST / LOBBY_CLAN_MESSAGE_EDIT_ANSWER` | *ModifyClanMessage / ModifyClanMessageResponse* | 0xF4 / 0x1C | 0x0017D3F8 ← L39_006CF5A8 | 0x00180E60 → L39_006CF750 | +0x28 message id, +0x2C message | C |
| 1/0x72 → 1/0x73 | `LOBBY_CLAN_MESSAGE_DELETE_REQUEST / LOBBY_CLAN_MESSAGE_DELETE_ANSWER` | *DeleteClanMessage / DeleteClanMessageResponse* | 0x2C / 0x1C | 0x0017D478 ← L39_006CF770 | 0x00180EA0 → L39_006CF900 | +0x28 message id | C |
| 1/0x74 → 1/0x75 | `LOBBY_CLAN_CHALLENGE_REPLY_REQUEST / LOBBY_CLAN_CHALLENGE_REPLY_ANSWER` | *RespondToClanTeamChallenge / RespondToClanTeamChallengeResponse* | 0xF8 / 0x1C | 0x0017D578 ← L39_006CFAF0 | 0x00180EE0 → L39_006CFCB0 | +0x28 challenge id, +0x2C response, +0x30 message | C |
| 1/0x76 → 1/0x77 | `LOBBY_CLAN_CHALLENGE_REVOKE_REQUEST / LOBBY_CLAN_CHALLENGE_REVOKE_ANSWER` | *RevokeClanTeamChallenge / RevokeClanTeamChallengeResponse* | 0x2C / 0x1C | 0x0017D5F8 ← L39_006CFCD0 | 0x00180F20 → L39_006CFE68 | +0x28 challenge id | C |
| 1/0x78 → 1/0x79 | `LOBBY_CLAN_CHALLENGE_HISTORY_REQUEST / LOBBY_CLAN_CHALLENGE_HISTORY_ENTRY` | *GetClanTeamChallengeHistory / GetClanTeamChallengeHistoryResponse* | 0x38 / 0x28 ×n | 0x0017D800 ← L39_006D29F0 | 0x00181060 → L39_006D2AC0 | +0x28 ClanID, +0x2C, +0x30 Start = 1, +0x34 PageSize | C |
| 1/0x7A → 1/0x7B | `LOBBY_CLAN_INVITATIONS_SENT_REQUEST / LOBBY_CLAN_INVITATIONS_SENT_ENTRY` | *GetClanInvitationsSent / GetClanInvitationsSentResponse* | 0x30 / 0x114 ×n | 0x0017D178 ← L39_006D1658 | 0x00180CE0 → L39_006D1710 | +0x28 Start = 1, +0x2C PageSize | C |
| 1/0x7C → 1/0x7D | `LOBBY_MY_CLAN_REQUEST / LOBBY_MY_CLAN_ENTRY` | *GetMyClans / GetMyClansResponse* | 0x30 / 0x170 ×n | 0x0017D6F8 ← L39_006CEA18 (connect flow, clan screens) | 0x00180F60 → L39_006CED88: ClanID → net+0x38, name → net+0x3C, leader → net+0x5D | [table](#l3-misc) | V |
| 1/0x7E → 1/0x7F | `LOBBY_CLAN_ALL_MESSAGES_REQUEST / LOBBY_CLAN_ALL_MESSAGES_ENTRY` | *GetAllClanMessages / GetAllClanMessagesResponse* | 0x30 / 0xF0 ×n | 0x0017D780 ← L39_006D0DD0 | 0x00181020 → L39_006D0F08 | +0x28 Start = 1, +0x2C PageSize | C |
| 1/0x82 → 1/0x83 | `LOBBY_CLAN_CHALLENGE_LIST_REQUEST / LOBBY_CLAN_CHALLENGE_ENTRY` | *GetClanTeamChallenges / GetClanTeamChallengesResponse* | 0x3C / 0x1C4 ×n | 0x0017D678 ← L39_006D2500 | 0x00180FA0 → L39_006D25E0 | +0x28 ClanID, +0x2C Start = 1, +0x30, +0x34, +0x38 | C |
| 1/0x84 → 1/0x85 | `LOBBY_CLAN_STATS_UPLOAD_REQUEST / LOBBY_CLAN_STATS_UPLOAD_ANSWER` | *UpdateClanStats / UpdateClanStatsResponse* | 0x12C / 0x1C | 0x0017D880 ← core 0x00196F18 | 0x001810A0 → core 0x001970E0: net+0x624 = status | +0x28 ClanID, +0x2C stats (4 × 8 bytes copied) | C |
| 1/0x88 → 1/0x89 | `LOBBY_WORLD_SECURITY_REQUEST / LOBBY_WORLD_SECURITY_ANSWER` | *GetWorldSecurityLevel / GetWorldSecurityLevelResponse* | 0x30 / 0x28 | 0x0017F3E0 ← L39_006DE380 | 0x00181160 → L39_006DE4E0 | +0x28 MediusWorldID, +0x2C AppType | C |
| 1/0x8C → 1/0x8D | `LOBBY_CITY_LIST_REQUEST / LOBBY_CITY_LIST_ENTRY` | *GetLocations / GetLocationsResponse* | 0x26 / 0x64 ×n | 0x0017F4E8 ← L39_006D7018 (connect screen 0x0068BAD8, city screens) | 0x00181260 → L39_006D7138: keeps ids 40..99; none kept → the game's own five cities | [table](#l3-locations) | V |
| 1/0x96 → 1/0x97 | `LOBBY_CHAT_TOGGLE_REQUEST / LOBBY_CHAT_TOGGLE_ANSWER` | *ChatToggle / ChatToggleResponse* | 0x2C / 0x1C | 0x0017F590 ← L39_006DE030 (0), L39_006DE180 (1) | 0x001813E0 → L39_006DE2D0: status → ctx+0x2F58 | +0x28 0 / 1 (off/on: I, public enum) | V |
| 1/0x98 → 1/0x99 | `LOBBY_CHAT_TEXT_CHECK_REQUEST / LOBBY_CHAT_TEXT_CHECK_ANSWER` | *TextFilter / TextFilterResponse* | 0x6C / 0x5C | 0x0017E720 ← L39_006DD608 (before LOBBY_CHAT_LINE) | 0x00181420 → L39_006DD770 | +0x28 type, +0x2C Text[64]; reply +0x1C Text[64] (I) | C |
| 1/0xA3 → 1/0xA4 | `LOBBY_LOCALE_SET_REQUEST / LOBBY_LOCALE_SET_ANSWER` | *SetLocalizationParams / SetLocalizationParamsResponse* | 0x30 / 0x1C | 0x0017F748 ← L39_006DC7C8 after LOBBY_SESSION_OPEN_REQUEST | 0x00181598 → L39_006DD3D0: status ≠ 0 → UI error 0x51; clears conn+0x9C | [table](#l3-session) | V |
| 1/0xA5 → 1/0xA6 | `LOBBY_FILE_CREATE_REQUEST / LOBBY_FILE_CREATE_ANSWER` | *FileCreate / FileCreateResponse* | 0x1D8 / 0xC8 | 0x0017FCB0 (reconnect resume only) | 0x001819B0 (slot 103) | (library-internal) | C (game never) |
| 1/0xA7 → 1/0xA9 | `LOBBY_FILE_UPLOAD_REQUEST / LOBBY_FILE_CHUNK_REQUEST` | *FileUpload / FileUploadServerReq* | 0xCC / 0x28 | 0x0017FCB0 (resume only) | 0x001819F8 LOBBY_FILE_CHUNK_REQUEST: answers with 1/0xA8 | (library-internal) | C (game never) |
| 1/0xA8 → none | `LOBBY_FILE_CHUNK` | *FileUploadResponse* | 0x1FC / — | handler 0x001819F8 (answer to LOBBY_FILE_CHUNK_REQUEST) | server | (library-internal) | C (game never) |
| 1/0xAA → 1/0xAB | `LOBBY_FILE_CLOSE_REQUEST / LOBBY_FILE_CLOSE_ANSWER` | *FileClose / FileCloseResponse* | 0xC4 / 0xC8 | 0x0017FCB0 (resume only) | 0x00181AD8 | (library-internal) | C (game never) |
| 1/0xAC → 1/0xAD | `LOBBY_FILE_DOWNLOAD_REQUEST / LOBBY_FILE_DOWNLOAD_ANSWER` | *FileDownload / FileDownloadResponse* | 0xC4 / 0x1FC | 0x0017FCB0 (resume only) | 0x00181B18 | (library-internal) | C (game never) |
| 1/0xC0 → 1/0xC1 | `LOBBY_IGNORE_LIST_REQUEST / LOBBY_IGNORE_LIST_ENTRY` | *GetIgnoreList / GetIgnoreListResponse* | 0x26 / 0x48 ×n | 0x0017EDB0 ← L39_006D5D68 | 0x00180890 → L39_006D5DF8 | header only | C |
| 1/0xC2 → 1/0xC3 | `LOBBY_IGNORE_ADD_REQUEST / LOBBY_IGNORE_ADD_ANSWER` | *AddToIgnoreList / AddToIgnoreListResponse* | 0x2C / 0x1C | 0x0017EE30 ← L39_006D6210 | 0x001808D0 → L39_006D6378 | +0x28 AccountID | C |
| 1/0xC4 → 1/0xC5 | `LOBBY_IGNORE_REMOVE_REQUEST / LOBBY_IGNORE_REMOVE_ANSWER` | *RemoveFromIgnoreList / RemoveFromIgnoreListResponse* | 0x2C / 0x1C | 0x0017EEB0 ← L39_006D6398 | 0x00180910 → L39_006D6500 | +0x28 AccountID | C |
| 1/0xD2 → 1/0xD3 | `LOBBY_CLAN_LADDER_REQUEST / LOBBY_CLAN_LADDER_ENTRY` | *ClanLadderList / ClanLadderListResponse* | 0x28 / 0x48 ×n | 0x0017D908 ← L39_006D2030 | 0x00181618 → L39_006D20E8 | +0x18, +0x1C, +0x20, +0x24 (sort, start, page: names I) | C |
| 1/0xD6 → 1/0xD7 | `LOBBY_BUDDY_LIST_REQUEST / LOBBY_BUDDY_LIST_ENTRY` | *GetBuddyList_ExtraInfo / GetBuddyList_ExtraInfoResponse* | 0x15 / 0xD0 ×n | 0x0017F7D8 ← L39_006CD768 | 0x00181698 → L39_006CD808: copies +0x00..0xC0, EndOfList +0xCC | MessageID only; reply: [table](#l3-misc) | V |
| 1/0xDA → 1/0xDB | `LOBBY_CLAN_MEMBER_LIST_REQUEST / LOBBY_CLAN_MEMBER_ENTRY` | *GetClanMemberList_ExtraInfo / GetClanMemberList_ExtraInfoResponse* | 0x24 / 0x1DC ×n | 0x0017D990 ← L39_006D08E8 | 0x00181758 → L39_006D0990 | +0x18 ClanID, +0x1C, +0x20 (start, page) | C |
| 1/0xDC → 1/0xDD | `LOBBY_CHANNEL_PLAYER_LIST_REQUEST / LOBBY_CHANNEL_PLAYER_ENTRY` | *GetLobbyPlayerNames_ExtraInfo / GetLobbyPlayerNames_ExtraInfoResponse* | 0x1C / 0xD0 ×n | 0x0017F858 ← L39_006DAB18 | 0x00181718 → L39_006DABF0 | +0x18 MediusWorldID | C |
| 1/0xEA → 1/0xEB | `LOBBY_GAME_LIST_REQUEST / LOBBY_GAME_LIST_ENTRY` | *GameList_ExtraInfo0 / GameList_ExtraInfoResponse0* | 0x1A / 0x190 ×n | 0x0017F8D0 ← L39_006D5AB8 (list screens; refreshed by L39_006D8B88) | 0x00181798 → L39_006D55E0: keeps records with status 0, PlayerCount ≠ 0, name ≠ "\t[DEAD GAME]" | [table](#l3-games) | V |
| 1/0xF1 → 1/0xF2 | `LOBBY_LADDER_POSITION_REQUEST / LOBBY_LADDER_POSITION_ANSWER` | *LadderPosition_ExtraInfo / LadderPosition_ExtraInfoResponse* | 0x24 / 0x24 | 0x0017FA40 ← L39_006D6CF8 | 0x00181970 → L39_006D6808 | +0x18, +0x1C, +0x20 (id, type, sort: names I) | C |
| 1/0xF3 → 1/0x24 | `LOBBY_GAME_JOIN_REQUEST / LOBBY_GAME_JOIN_ANSWER` | *JoinGame / JoinGameResponse* | 0xC4 / 0xBC | 0x0017E828 ← L39_006D34D8 (join screens; L39_006D3A30 for the host's own world) | 0x001803E8 → L39_006D3950: status; then the DME connect 0x00195530 on reply +0x20 | [table](#l3-games) | V |
| 1/0xF4 → 1/0x1E | `LOBBY_GAME_CREATE_REQUEST / LOBBY_GAME_CREATE_ANSWER` | *CreateGame1 / CreateGameResponse* | 0xD4 / 0x20 | 0x0017E698 ← L39_006D3A30 (create screen L39_0069A630 via L39_0069AEF0) | 0x00180310 → L39_006D4030: world id stored (0x0019B5B8); -975 → UI 0x2B, -967 → 3, else 0x2A | [table](#l3-games) | V |
| 4/0x0A → none | `LOBBY_DNAS_SIGNATURE_REPORT` | *DnasSignaturePost* | 0x50 / — | 0x0017E480 ← L39_0068C210 after login to the authentication server | server | +0x15 SessionKey, +0x28 DNAS data (L39_0067BBE8; the port writes none; I: Horizon says +0x28 DnasCategory, +0x2C u8 length, +0x2D signature[32]) | C |
| 4/0x0B → 1/0xCF or 4/0x0C | `LOBBY_LADDER_STATS_WRITE_REQUEST / LOBBY_LADDER_STATS_WRITE_ANSWER` | *UpdateLadderStatsWide / UpdateLadderStatsResponse* | 0x1AC / 0x1C | 0x0017FAC8 ← core 0x00196510 (via L39_006D6E30, 6 callers); waits ≤ 5 s | 0x00181878 / 0x001818B0 (one slot, 68) → core 0x001966E8: net+0x60C = 0, net+0x610 = status | +0x18 LadderType (0; I: Horizon says Player), +0x1C s32[100] Stats (agrees with Horizon) | V |
| 4/0x0D → 4/0x0E | `LOBBY_LADDER_STATS_READ_REQUEST / LOBBY_LADDER_STATS_READ_ANSWER` | *GetLadderStatsWide / GetLadderStatsWideResponse* | 0x20 / 0x1B0 | 0x0017FB48 ← core 0x00196D08 (via L39_006D6E50, 15 callers: connect screen with the AccountID, clan screens); waits ≤ 5 s | 0x00181930 → core 0x00196E38: status → net+0x61C, +0x20 Stats[100] → the caller's buffer (net+0x618) | +0x18 AccountID or ClanID, +0x1C LadderType (0) | V |
| 4/0x0F → 1/0xF0 | `LOBBY_LADDER_PAGE_REQUEST / LOBBY_LADDER_PAGE_ENTRY` | *LadderList_ExtraInfo / LadderList_ExtraInfoResponse* | 0x28 / 0x1D8 ×n | 0x0017F9C0 ← L39_006D6750 | 0x00181828 → L39_006D68C0 | +0x18..+0x24 (field, sort, start, page: names I) | C |
| 4/0x12 → 4/0x13 | `LOBBY_CHANNEL_FILTER_SET_REQUEST / LOBBY_CHANNEL_FILTER_SET_ANSWER` | *SetLobbyWorldFilter / SetLobbyWorldFilterResponse* | 0x30 / 0x34 | 0x0017FBC8 ← L39_006DC148 (city chosen), L39_006DC1D8 (game chat channels), L39_006DB5D0 (clear); the API keeps a copy the reconnect resume re-sends | 0x00182548 → L39_006DB5B0 / L39_006DB730: status | [table](#l3-channels) | V |
| 4/0x14 → 1/0x22 | `LOBBY_CHANNEL_CREATE_REQUEST / LOBBY_CHANNEL_CREATE_ANSWER` | *CreateChannel / CreateChannelResponse* | 0xA4 / 0x20 | 0x0017E7A0 ← L39_006DA3F8 (when no listed channel fits; also a game's chat channel) | 0x001803A0 → L39_006DA628 | [table](#l3-channels) | C |
| 4/0x15 → 1/0xED | `LOBBY_CHANNEL_LIST_REQUEST / LOBBY_CHANNEL_LIST_ENTRY` | *ChannelList_ExtraInfo1 / ChannelList_ExtraInfoResponse* | 0x1A / 0x80 ×n | 0x0017F948 ← L39_006DAFC8 (channel pick L39_006DB948) | 0x001817E0 → L39_006DB098: records until EndOfList | [table](#l3-channels) | C |

<a id="l3-unsolicited"></a>**Unsolicited server → client** (callbacks given at MediusInitialize 0x0017DE30 by L39_006D8D80):

| Type | Name | Horizon name | Size | Receiver: what it does | Mark |
|---|---|---|---|---|---|
| 1/0x3C | `LOBBY_CHAT_LINE_NOTICE` | *ChatFwdMessage* | 0x80 | 0x001806B8, slot 27 → 0x00198028 → L39_006D8E90: the chat screen's hook (lobby ctx+0x20) | C |
| 1/0x4A | `LOBBY_SERVER_ERROR_NOTICE` | *ErrorMessage* | 0x104 | 0x0017FFC8, slot 0 → 0x00198050 → L39_006D8EC0: lobby ctx+0x24 hook | C |
| 1/0x9A | `LOBBY_GAME_WORLD_ID_CHANGED_NOTICE` | *ServerReassignGameMediusWorldID* | 0x8 | 0x00181458, slot 52 → 0x00198078: in a world, its id := +0x04 (0x0019B5B8) | C |
| 4/0x17 | `LOBBY_BINARY_MESSAGE_NOTICE` | *BinaryFwdMessage* | 0x1B0 | 0x001806F8, slot 73: no game callback (dropped) | C |
| 4/0x06 | `LOBBY_BUDDY_ADDED_NOTICE` | *AddToBuddyListFwdConfirmation* | 0x40 | 0x00180838, slot 31: no game callback (dropped) | C |

#### 4.3.2 Field layouts

<a id="l3-session"></a>**Session and login** (offsets after the header; C unless marked):

| Message | Off | Size | Type | Field | Values / units |
|---|---|---|---|---|---|
| LOBBY_SESSION_OPEN_REQUEST 1/0x03 | 0x18 | 4 | s32 | ConnectionClass *(public)* | 1 (I: Horizon says MediusConnectionType 1 = Ethernet) |
| LOBBY_SESSION_OPEN_ANSWER 1/0x04 | 0x18 | 4 | s32 | StatusCode | 0 = go on |
| | 0x1C | 17 | char | SessionKey | any 16 chars; the library keeps it for every later request |
| LOBBY_LOCALE_SET_REQUEST 1/0xA3 | 0x28 | 4 | s32 | character encoding | 2 (I: Horizon says UTF8) |
| | 0x2C | 4 | s32 | language | from the game's language byte 0x001A58C7 via 0x0019B760: 0→2, 2→8, 3→7, 4→6, 5→5 (I: Horizon says 2 UK English, 5 Italian, 6 Spanish, 7 German, 8 French, matching a PAL game's English, French, German, Spanish, Italian) |
| LOBBY_LOGIN_REQUEST 1/0x07 | 0x26 | 32 | char | AccountName | the profile's name |
| | 0x46 | 32 | char | Password | the profile's password |
| LOBBY_LOGIN_ANSWER 1/0x08 | 0x18 | 4 | s32 | StatusCode | 0; -998 → UI 0xF; -997 → UI 0x10 + logout/retry; -991/-989/-987 → UI 1; -978 → UI 0xA |
| | 0x1C | 4 | s32 | AccountID | → account ctx +0, net+0x34 (put in OBJECT_PLAYER_DATA / OBJECT_GAME_SETTINGS) |
| | 0x20 | 4 | s32 | AccountType *(public)* | not read (I) |
| | 0x24 | 4 | s32 | MediusWorldID *(public)* | not read (I) |
| | 0x28 | 0x9C | NetConnectionInfo | the lobby server to connect to next ([below](#l3-nci)); +0x44 (its 2nd address) saved at 0x001E7E08 | |
| LOBBY_LICENCE_TEXT_REQUEST 1/0x47 | 0x28 | 4 | s32 | PolicyType *(public)* | 1 (I: Horizon says 1 = Privacy, 0 = Usage) |
| LOBBY_LICENCE_TEXT_PART 1/0x48 | 0x1C / 0x11C | 256 / 4 | char / s32 | Text, EndOfText | text appended per record until EndOfText ≠ 0 |
| LOBBY_NEWS_ENTRY 1/0x4D | 0x1C / 0x20 / 0x408 | 4 / 1000 / 4 | s32 / char / s32 | AnnouncementID (not read, I), Announcement, EndOfList | |
| LOBBY_PRESENCE_UPDATE 1/0x49 | 0x00 / 0x14 | 17 / 4 | char / s32 | SessionKey, UserAction | 1 joined the chat world, 2 left the game world (0 keep-alive: public, unused) |

<a id="l3-nci"></a>**NetConnectionInfo (0x9C) and ConnectParams (0x100)** (C, MediusMakeConnectParams 0x0017F620 and
the game's reconnects): +0x00 s32 Type (1); +0x04 two addresses {s32 AddressType, char Address[16],
u32 Port} at +0x04/+0x08/+0x18 and +0x1C/+0x20/+0x30; +0x34 s32 WorldID; +0x38 u8[64] ServerKey
(0x001A3318 when encrypted); +0x78 char[17] SessionKey; +0x89 char[17] AccessKey (zeroed; name I);
ConnectParams adds +0x9C..+0xEF RT options (from 0x0014DD58), +0xF0 s32 (0xFF), +0xF4 the connect
callback (game 0x006DD028), +0xF8/+0xFC two more callbacks (empty; I: disconnect notices).

<a id="l3-locations"></a>**LOBBY_CITY_LIST_ENTRY 1/0x8D (0x64, its own layout, C, L39_006D7138):**

| Off | Size | Type | Field | Values |
|---|---|---|---|---|
| 0x00 | 21 | char | MessageID | copied |
| 0x18 | 4 | s32 | LocationID | kept only if 40 ≤ id < 100; it becomes the city's filter bit (`1 << (id-40)`) |
| 0x1C | 64 | char | LocationName | shown |
| 0x5C | 4 | s32 | StatusCode | 0 = a record |
| 0x60 | 4 | s32 | EndOfList | read as a byte; at the end with nothing kept the game adds 75 Veldin, 76 Blackwater City, 77 Metropolis, 78 Aridia, 79 Orxon |

<a id="l3-channels"></a>**Channels:**

| Message | Off | Size | Type | Field | Values (C unless marked) |
|---|---|---|---|---|---|
| LOBBY_CHANNEL_FILTER_SET_REQUEST 4/0x12 | 0x18 | 4 | u32 | channel kind *(FilterMask1)* | 1 city lobby channels, 6 a game's chat channel, 0xFFFFFFFF clear (compared with a channel's GenericField1: I) |
| | 0x1C | 4 | u32 | city bits 0-31 *(FilterMask2)* | `1 << (LocationID-40)` for 40-71; 0; all ones to clear |
| | 0x20 | 4 | u32 | city bits 32-63 *(FilterMask3)* | `1 << (LocationID-72)` for 72-103 (75 Veldin = bit 3) |
| | 0x24 | 4 | u32 | *(FilterMask4)* | 0 / all ones |
| | 0x28 | 4 | s32 | *(LobbyFilterType)* | 1 set, 0 clear (I: Horizon says 1 EqualsFilter, 0 EqualsLobby) |
| | 0x2C | 4 | s32 | *(FilterMaskLevelType)* | 0x20 (I: Horizon says Level123 = masks 1-3 compared) |
| LOBBY_CHANNEL_FILTER_SET_ANSWER 4/0x13 | 0x18 | 4 | s32 | StatusCode | the rest (+0x1C..+0x33) not read (I: the masks echoed) |
| LOBBY_CHANNEL_LIST_REQUEST 4/0x15 | 0x16 / 0x18 | 2 / 2 | u16 | PageID (1), PageSize | |
| LOBBY_CHANNEL_LIST_ENTRY 1/0xED | 0x1C | 4 | s32 | MediusWorldID | |
| | 0x20 / 0x22 | 2 / 2 | u16 | PlayerCount (auto-pick needs < 219) / MaxPlayers (I) | |
| | 0x24 | 4 | s32 | SecurityLevel (I) | |
| | 0x28 | 4 | s32 | GenericField1: channel kind | 1 numbered city channel (its name +0xB parsed "%02X"), 6 game chat |
| | 0x2C..0x38 | 4 each | s32 | GenericField2-4, GenericFieldLevel (I) | |
| | 0x3C / 0x7C | 64 / 4 | char / s32 | LobbyName, EndOfList | |
| LOBBY_CHANNEL_JOIN_REQUEST 1/0x25 | 0x28 / 0x2C | 4 / 32 | s32 / char | MediusWorldID, LobbyChannelPassword | |
| LOBBY_CHANNEL_JOIN_ANSWER 1/0x26 | 0x1C | 0x9C | NetConnectionInfo | the channel's lobby server: the game reconnects to it | |
| LOBBY_CHANNEL_CREATE_REQUEST 4/0x14 | 0x28..0xA0 | | | ApplicationID, MaxPlayers, LobbyName[64] +0x30, LobbyPassword[32] +0x70, GenericField1-4 +0x90..+0x9C, GenericFieldLevel +0xA0 | offsets C, names I |
| LOBBY_CHANNEL_CREATE_ANSWER 1/0x22 | 0x18 / 0x1C | 4 / 4 | s32 | StatusCode (0, 3, or -974 for GF1 6: existing id), MediusWorldID | |

<a id="l3-games"></a>**Games:**

| Message | Off | Size | Type | Field | Values (C unless marked) |
|---|---|---|---|---|---|
| LOBBY_GAME_LIST_REQUEST 1/0xEA | 0x16 / 0x18 | 2 / 2 | u16 | PageID (1), PageSize | |
| LOBBY_GAME_LIST_ENTRY 1/0xEB | 0x1C | 4 | s32 | MediusWorldID (what LOBBY_GAME_JOIN_REQUEST/LOBBY_GAME_INFO_REQUEST name) | |
| | 0x20 | 2 | u16 | PlayerCount | 0 hides the record |
| | 0x22 / 0x24 | 2 / 2 | u16 | MinPlayers, MaxPlayers | (I: shown via the copy) |
| | 0x28..0x3C | 4 each | s32 | GameLevel, PlayerSkillLevel, RulesSet, GenericField1-3 | the create settings ([4.3.3](#l3-settings)) (I: read via the copy by the list screens) |
| | 0x40 / 0x44 / 0x48 | 4 each | s32 | WorldSecurityLevel, WorldStatus, GameHostType | (I) |
| | 0x4C | 64 | char | GameName | "\t[DEAD GAME]" hides it |
| | 0x8C / 0x18C | 256 / 4 | | GameStats (I), EndOfList (read as a byte) | |
| LOBBY_GAME_LIST_FILTER_ADD_REQUEST 1/0x4E | 0x28 / 0x2C / 0x30 | 4 each | s32 | settings word index (FilterField), comparison mask from L39_006CC2A8 (ComparisonOperator: I), baseline value | |
| LOBBY_GAME_INFO_REQUEST 1/0x33 | 0x28 | 4 | s32 | MediusWorldID | |
| LOBBY_GAME_INFO_ANSWER 1/0x34 | 0x1C | 4 | s32 | ApplicationID (I) | |
| | 0x20..0x2C | 4 each | s32 | MinPlayers, MaxPlayers, GameLevel, PlayerSkillLevel | compared with the host's report |
| | 0x30 / 0x34 | 4 / 256 | | PlayerCount (I), GameStats (I) | |
| | 0x134 | 64 | char | GameName | compared |
| | 0x174..0x180 | 4 each | s32 | RulesSet, GenericField1-3 | compared |
| | 0x184 / 0x188 | 4 / 4 | s32 | WorldStatus, GameHostType (I; layout agrees with Horizon) | |
| LOBBY_GAME_CREATE_REQUEST 1/0xF4 | 0x28 / 0x2C / 0x30 / 0x34 | 4 each | s32 | ApplicationID, MinPlayers, MaxPlayers, GameLevel | from the settings object |
| | 0x38 / 0x78 | 64 / 64 | char | GameName, password block | the game copies one string at +0x78 (C); Horizon says GamePassword[32] +0x78 and SpectatorPassword[32] +0x98 (I) |
| | 0xB8 / 0xBC / 0xC0-0xC8 | 4 each | s32 | PlayerSkillLevel, RulesSet, GenericField1-3 | packed options ([4.3.3](#l3-settings)) |
| | 0xCC / 0xD0 | 4 / 4 | s32 | GameHostType, WorldAttributes (I) | 4 ClientServerAuxUDP when byte 0x001A5F98 is set, else 0 ClientServer *(public names)*; 0 |
| LOBBY_GAME_CREATE_ANSWER 1/0x1E | 0x18 / 0x1C | 4 / 4 | s32 | StatusCode, MediusWorldID | |
| LOBBY_GAME_JOIN_REQUEST 1/0xF3 | 0x28 / 0x2C | 4 / 4 | s32 | MediusWorldID, JoinType (0; I: Horizon says JoinAsPlayer) | |
| | 0x30 / 0x50 | 32 / 4 | | GamePassword, GameHostType (as create) | |
| | 0x54 / 0x94 | 64 / 0x30 | | PubKey (zero; I), NetAddressList (only on the dormant P2P path) | |
| LOBBY_GAME_JOIN_ANSWER 1/0x24 | 0x18 / 0x1C | 4 / 4 | s32 | StatusCode, GameHostType (I) | |
| | 0x20 | 0x80 used | NetConnectionInfo | the DME server of the game: copied into the DME connect (0x00195530) | |
| LOBBY_GAME_PLAYER_LIST_REQUEST 1/0x2F | 0x28 | 4 | s32 | MediusWorldID | |
| LOBBY_GAME_PLAYER_LIST_ENTRY 1/0x30 | 0x18 | 4 | s32 | StatusCode | 0 a player, 1 end/empty, -991/-977 → UI 0x35, other < 0 → 0x2F |
| | 0x1C / 0x20 / 0x40 / 0x144 | 4 / 32 / 256 / 4 | | AccountID, AccountName, Stats, EndOfList | |

<a id="l3-worldreport"></a>**LOBBY_GAME_REPORT 1/0x00 (0x180, buffer net+0x48C = 0x001A615C; C):**

| Off | Size | Type | Field | Source |
|---|---|---|---|---|
| 0x00 | 17 | char | SessionKey | 0x0017DF20 |
| 0x14 | 4 | s32 | MediusWorldID | 0x0019B5C8 |
| 0x18 | 4 | s32 | PlayerCount | 1 at create; no other writer found (I: may stay 1) |
| 0x1C | 64 | char | GameName | settings +0 |
| 0x5C | 256 | | GameStats | zeroed |
| 0x15C..0x178 | 4 each | s32 | MinPlayers, MaxPlayers, GameLevel, PlayerSkillLevel, RulesSet, GenericField1-3 | settings +0x168, +0x164, +0x16C, +0x170, +0x174, +0x178, +0x17C, +0x180 |
| 0x17C | 4 | s32 | WorldStatus | net+0x136C (0x0019B5F8); 3 at leave (I: Horizon says 3 = WorldClosed; 0 Inactive, 1 Staging, 2 Active) |

<a id="l3-misc"></a>**Other replies the game reads:**

| Message | Off | Size | Field | Use (C unless marked) |
|---|---|---|---|---|
| LOBBY_MY_CLAN_ENTRY 1/0x7D (0x170) | 0x1C | 4 | ClanID | → net+0x38; status ≠ 0 = no clan (net+0x38 = -1) |
| | 0x24 | 32 | ClanName | → net+0x3C (the clan tag source) |
| | 0x44 / 0x48 | 4 / 32 | LeaderAccountID, LeaderAccountName | = my AccountID → net+0x5D "I lead" |
| | 0x68 | 256 | ClanStats | parsed by L39_006CED20 into 12 bytes |
| | 0x20, 0x168, 0x16C | | ApplicationID, ClanStatus, EndOfList | not read (I) |
| LOBBY_ACCOUNT_INFO_ANSWER 1/0x32 (0x148) | 0x1C / 0x3C / 0x40 / 0x44 / 0x48 | 32 / 4 / 4 / 4 / 256 | AccountName, ApplicationID, PlayerStatus, ConnectionClass, Stats | copied whole, inner fields I |
| LOBBY_CHANNEL_INFO_ANSWER 1/0x36 (0x64) | 0x1C / 0x5C / 0x60 | 64 / 4 / 4 | LobbyName, ActivePlayerCount, MaxPlayers | copied whole, inner fields I |
| LOBBY_BUDDY_LIST_ENTRY 1/0xD7 (0xD0) | 0x1C / 0x20 / 0x40.. / 0xCC | | AccountID, AccountName, online state, EndOfList | EndOfList C; the rest I |
| LOBBY_KEEP_ALIVE_ANSWER 1/0x1C (0x30) | — | | IP, StatusCode | not read (C); Horizon says IP char[20] at +0x15 and StatusCode at +0x2C, unlike the usual +0x18 (I) |

<a id="l3-settings"></a>#### 4.3.3 What the game puts in the lobby's generic fields (C, L39_006CC360 / 006CC4F8 / 006CC458)

The create screen's 29 options (an s32 array) are packed into LOBBY_GAME_CREATE_REQUEST's words by the table at
0x0047F0C0 ({option, field, mask, flag}): options 0/1/2 → Min/Max/GameLevel; 3 → PlayerSkillLevel
bits 24-31; 4-11 → PlayerSkillLevel bits 0-7 (one flag each); 12 → RulesSet; 13 → GenericField1; 14
→ GenericField2; 15/16/17 → GenericField3 bits 0-2 / 3-6 / 11-12; 18, 19, 20 → bits 13, 14, 18; 21/22/23
→ bits 19-21 / 22 / 23-26; 24-28 → bits 27-31. Meanings proven: option 16 + 0x27 = the map's level
(40-55, also OBJECT_GAME_SETTINGS +0x17C); option 0x11 = game mode (OBJECT_GAME_SETTINGS +0x180); option 12 RulesSet
(OBJECT_GAME_SETTINGS +0x184; 2 makes LOBBY_GAME_CREATE_REQUEST compare GenericField1 with the player's clan id); option 0x16
→ OBJECT_GAME_SETTINGS +0x19F. Everything else: I (no strings tie the options to names). The host checks
after create + join that LOBBY_GAME_INFO_ANSWER echoes name, Min/Max, Level, Skill, Rules and GF1-3 (≤ 21
polls, else UI error 2), so a lobby must serve LOBBY_GAME_INFO_ANSWER from the latest LOBBY_GAME_REPORT.

An announcement starting with **"{RATCHET3_LOBBY_SETTINGS}"** (0x0024A018) is not shown but parsed
(L39_006DE688 → L39_006DE8D8) as `key = value` lines: the float tuning table 0x004B2258, `kEnableCS`,
`kEnableCSCD`, `CSBLK[n]` (n < 192, into 0x001A6A80), and per-weapon arrays `ma[n]`, `mps[n]`, `sps[n]`,
`gd1[n]`, `gd2[n]` (n < 26), `bdd`, `bpd`, `fdd`, `fpd`, `td` (into 0x001A6D90). The key
`RATCHET3_LOBBY_USE_P2P` is skipped (C): P2P stays off.

<a id="l3-unused"></a>#### 4.3.4 Registered but unused (C: a handler exists, no request API or game callback uses it)

1/0x09-0x0A LOBBY_ACCOUNT_REGISTRATION (+ answer), 0x0B-0x0C LOBBY_ACCOUNT_GET_PROFILE, 0x0D-0x0E
LOBBY_ACCOUNT_UPDATE_PROFILE, 0x0F-0x10 LOBBY_ACCOUNT_UPDATE_PASSWORD (+ status), 0x13-0x14 LOBBY_ACCOUNT_DELETE, 0x19-0x1A
LOBBY_ANONYMOUS_LOGIN, 0x1D LOBBY_GAME_CREATE_REQUEST_OLD, 0x21 LOBBY_CHANNEL_CREATE_REQUEST_OLD, 0x23 LOBBY_GAME_JOIN_REQUEST_OLD, 0x27-0x28
LOBBY_JOIN_CHANNEL_FWD, 0x29-0x2A LOBBY_GAME_LIST_OLD, 0x2B-0x2C LOBBY_CHANNEL_LIST_OLD, 0x2D-0x2E LOBBY_WORLD_PLAYER_LIST, 0x37-0x38
LOBBY_FIND_WORLD_BY_NAME, 0x3D-0x3E LOBBY_GET_BUDDY_LIST, 0x43-0x46 LOBBY_ADD_TO_BUDDY_LIST_CONFIRMATION (request/fwd forms),
0x52-0x53 LOBBY_GET_GAME_LIST_FILTER, 0x5E-0x5F LOBBY_ADD_PLAYER_TO_CLAN, 0x80-0x81 LOBBY_CONFIRM_CLAN_TEAM_CHALLENGE, 0x86-0x87
LOBBY_VERSION_SERVER, 0x8A-0x8B LOBBY_BAN_PLAYER, 0x8E-0x8F LOBBY_PICK_LOCATION, 0x90-0x91 LOBBY_GET_CLAN_MEMBER_LIST, 0x92-0x93
LOBBY_LADDER_POSITION, 0x94-0x95 LOBBY_LADDER_LIST, 0x9B-0xA2 LOBBY_GET_TOTAL_GAMES / LOBBY_GET_TOTAL_CHANNELS /
LOBBY_GET_LOBBY_PLAYER_NAMES / LOBBY_GET_TOTAL_USERS, 0xAE-0xBF the other file operations (stream download, delete,
list, attributes, metadata, search, cancel), 0xC6-0xC7 LOBBY_SET_MESSAGE_AS_READ, 0xC8-0xCA
LOBBY_GET_UNIVERSE_INFORMATION / LOBBY_UNIVERSE_NEWS / LOBBY_UNIVERSE_STATUS_LIST, 0xCB LOBBY_MACHINE_SIGNATURE_POST, 0xCC-0xCD
LOBBY_LADDER_POSITION_FAST, 0xCE / 0xD0-0xD1 LOBBY_UPDATE_LADDER_STATS / LOBBY_GET_LADDER_STATS (narrow forms), 0xD4-0xD5
LOBBY_CLAN_LADDER_POSITION, 0xD8-0xD9 LOBBY_GET_TOTAL_RANKINGS, 0xEC LOBBY_CHANNEL_LIST_EXTRA_INFO_OLD, 0xEE
LOBBY_INVITE_PLAYER_TO_CLAN_BY_NAME, 0xEF LOBBY_LADDER_LIST_EXTRA_INFO_OLD, 0xF5-0xFC the Util*World messages;
4/0x00 LOBBY_CHANNEL_CREATE_EXT, 0x01-0x02 LOBBY_UTIL_GET_SERVER_VERSION, 0x03-0x04 LOBBY_GET_UNIVERSE_EXTRA_INFO /
LOBBY_UNIVERSE_STATUS_LIST_EXTRA_INFO, 0x05 / 0x07 LOBBY_ADD_TO_BUDDY_LIST_CONFIRMATION forms, 0x08-0x09
LOBBY_GET_BUDDY_INVITATIONS, 0x10 LOBBY_UTIL_EVENT_MSG_HANDLER, 0x11 LOBBY_UNIVERSE_VARIABLE_INFORMATION, 0x16 LOBBY_BINARY_MESSAGE,
0x18-0x19 LOBBY_POST_DEBUG_INFO (names of ours for the public layouts, recognised by size). Script:
`protocol-v2/medius/unused.py`.

<a id="l4"></a>
### 4.4 Layer 4: DME messages (dme 2.7.10): the library's own (class 0) and the app channels (class 2)

The game room. A DME connection (`DmeConn`, 4-slot table 0x001CD4C8) runs over its own RT connection
to the DME server (`dme_connect_world` 0x0014ADD0 with the NetConnectionInfo from LOBBY_GAME_JOIN_ANSWER),
or over rt_p2p when the connection's slot is 0 (dormant in this build). Marks: **V** seen in the
port's runs, **C** read in the code (m2c of every sender and handler: `protocol-v2/dme/dec/`),
**I** inferred.

**Framing (C).** A DME packet is one or more items `[u8 class][u8 id][payload]`, packed; it travels
as the payload of RT 0x02 (to every other client: dest -1 → 0xFFFE), 0x03 (one client) or 0x0B (the
DME server: dest 0xFFFF). The receive dispatcher **0x00152FB8**(conn, index, src, buf, len, 0) walks
the items; each handler `handler(conn, index, src_client, payload)` returns the payload bytes it
consumed. Registry 0x00152C88: **6 classes × 256 ids** (table 0x001A27CC, lookup 0x001531C8).
Classes: **0** DME's own (23 ids: 22 registered by 0x00150FF0, one more through 0x00152EE0), **1 / 4** the Medius lobby (layer 3,
on the lobby connection), **2** the game's app channels (layer 5), 3 and 5 unused.

**Sending (C).** App: `dme_send_app(kind, conn, dest, id, len, data)` **0x0014AD08** → 0x0014CB98
with class 2. Internal: `dme_send_internal(conn, dest, id, len, data)` **0x0014CE08** = 0x0014CB98
with class 0 and kind **0x50**. 0x0014CB98 checks class < 6, id < 0x100, len > 0; a message of
0x1FF bytes or more is cut into class-0 id-2 fragments (not to the server). Server path: 0x0014CAB8
→ 0x0014C7D8 → rt_msg slot +0x30 (0x0016ACE0); peer path (conn slot 0): 0x0014C950 → rt_p2p.
**Reliability by kind (C, 0x0016ACE0):** any of 0x10/0x40/0x50 → the TCP stream (reliable, in
order); 0x80 alone → the aux UDP stream (unreliable, unordered); the game's sends use
`kind = net+0x7D4 | flag`, net+0x7D4 = 0x40 once UDP is judged dead. Dest: a client index, -1 every
other client, 0xFFFF the DME server; to oneself nothing is sent.

**Receiving (C).** Server path: rt_msg app callback 0x0014D6C0 (registered by 0x0014DE98); peer
path: 0x00150338. Both → **0x0014CE48**(index, conn, src, buf, len) → 0x00152FB8. Runs inside
`dme_update` **0x0014B118** (impl 0x0014F0A8; each frame from 0x0019B728 in the net update
0x001959F8), which also runs the object sync tick (world slot +0x84 = 0x00171B78), the stream tick
(+0x88 = 0x00171878) and 0x00171D30 (+0x8C), and sends pings.

<a id="l4-ids"></a>
#### 4.4.1 DME's own messages (class 0)

Handlers from the registration 0x00150FF0 and the world-object table 0x001D86A0 (filled by
0x00172248). Ids 0x06-0x08, 0x11 and 0x1B-0xFF are not registered (dropped).

| Id | Name | Horizon name | Size | Sender: when | Dest, kind | Receiver: what it does | Mark |
|---|---|---|---|---|---|---|---|
| 0x00 | `ROOM_SERVER_VERSION_NOTICE` | *ServerVersion* | 0x10 | the DME server only (no client sender) | — | 0x00150738: copies 16 bytes to conn+0x410 (size agrees with Horizon's version string, DME_VERSION_LENGTH 16) | C (meaning I: Horizon says the DME server's version string) |
| 0x01 | `ROOM_PING` | *Ping* | 8 | 0x0014ECD8 via 0x0014AC48: from dme_update (periodic) and 3 times after a ROOM_JOIN_WELCOME (0x00173FC0); the echo from the handler | one client, **0x80 (UDP)** | 0x00150AA0: request → echo {own clock, slot, 0}; echo → round trip = now - sent (table 0x001EA598), sets the clock parameters 0x001A1B38+0x54/+0x58, calls the slot's callback | C |
| 0x02 | `ROOM_FRAGMENT` | *PacketFragment* | 0x14 + piece | 0x0014CB98 for any message ≥ 0x1FF bytes | as the message; kind \| 0x50 | 0x00150C90: stores the piece in the reassembly buffer (0x001511F0); when complete, calls the original (class, id) handler with the whole message | C |
| 0x03 | `ROOM_OBJECT_FIELD_UPDATE` | — | 4 + n × (1 + field bytes) | 0x001709A0: the object sync tick (0x00171B78, from dme_update) for the fields changed since the last send (0x001706F8 diffs data against the last-sent copy +0x20); once more on release (0x001704D8, kind 0x50) | every other client (or the object's client bitmap) | 0x00173210: copies each field into the object's data and copy, stamps its time, calls the type's **on_update(owner, id, field index)** | C |
| 0x04 | `ROOM_OBJECT_STATE` | — | 0x24 + all fields | 0x00170AE0 = **nwobj_send (API 0x0016EF40)**(target, id): the game calls it after creating or changing an object (0x00199C80, 0x0019A6F8, 0x00199A20, 0x0019A180); the sync tick sends it to a newcomer and every obj+0x30 ms | target client or all | 0x00172DC0: new id → allocates the record (0x44) and data, **on_create(creator, id)**; known → copies all fields, **on_update(creator, id, -1)** (-1 = whole object) | C |
| 0x05 | `ROOM_STREAM_CLOSE` | — | 1 | 0x00171710 from the stream tick when a send stream is done | the stream's peer | 0x00173BF0: the stream type's close callback, frees it | C (purpose I: no game call reaches the stream API) |
| 0x09 | `ROOM_JOIN_WELCOME` | — | 8 | 0x00173FC0 by the **session master** (state 5) answering a 0x15 from a new client | the joiner, 0x50 | 0x001508D0: state 4 (joined), 0x16 to all, **host := sender** (0x00171D18 sets conn+0x408), **DME clock := +0** (0x0014C1D8), my object/stream id bases, broadcasts my 0x0F, join complete (0x00172020 → the game's join callback) | C |
| 0x0A | `ROOM_OBJECT_DESTROYED` | — | 2 | 0x0016F4A0 from nwobj_destroy (world slot +0x74) by the object's creator | every other client | 0x00172B30 → 0x0016F378: on_destroy, frees the record | C (the game never calls +0x74) |
| 0x0B | `ROOM_OBJECT_OWNERSHIP_REQUEST` | — | 2 | 0x00170388 (world slot +0x78, take ownership) when the object is unowned and not mine | the object's creator | 0x00173558: unowned → the app hook conn+0x458 decides (grant when none) → owner := requester, on_update(owner, id, -2), 0x0C result 1 to all; refused → 0x0C result 2; owned → result 3 | C (the game never calls +0x78) |
| 0x0C | `ROOM_OBJECT_OWNERSHIP_ANSWER` | — | 6 | 0x00170388, 0x00173558 | all / the requester | 0x001737A8: result 1 → owner := +0, on_update(owner, id, -2); ownership hook conn+0x450 | C |
| 0x0D | `ROOM_OBJECT_RELEASED` | — | 2 | 0x001704D8 = **nwobj_release (API 0x0016EDD0)**, owner only: owner := -1, a last 0x03, then 0x0D | every other client | 0x00173898: owner := -1, on_update(-1, id, -2) (or hook conn+0x450 result 4). **The object stays** | C |
| 0x0E | `ROOM_STREAM_CHUNK` | — | 0xC + n | 0x00171878 (stream tick, ≤ 90,000 B/s) | the stream's peer | 0x00173960: opens the receive stream on first data, appends, the type's data callback | C (purpose I, as 0x05) |
| 0x0F | `ROOM_CLIENT_RECORD` | — | 0x24 | 0x0014A668(conn, dest): at join complete (0x00172020, to all), to a newcomer (0x001720B0), after nwobj_set_player 0x0016ED68 (to all) | one / all, 0x50 | 0x001507A8: creates/updates the sender's record; **a client's first record → the game's client-status callback, kind 0 (joined)**, and our own 0x0F back to it | C |
| 0x10 | `ROOM_PEER_HELLO` | *ClientConnects* | 0x48 | 0x0014C390: on the RT connect notify (RT 0x08 → 0x0014D098) and its p2p twin 0x00150230 | the new client | 0x00150650: unless our own echo, installs the peer's key (rt_crypt +0x18), answers with 0x0014C390, marks the client active, 0x16 status | C (fields +0/+2 I) |
| 0x12 | `ROOM_CLIENT_LEAVING` | — | 1 | 0x00171EE8 = dme_leave_session (API 0x0014B1B8) | every other client | 0x00150770 → 0x001725C0: record state 2, **client-status callback kind 1 (left)**, its owned objects → unowned, the objects it created destroyed, its id ranges freed | C |
| 0x13 | `ROOM_SERVER_LIST_QUERY` | *RequestServers* | 4 | none in the client | — | 0x001508C0: consumes 4 bytes | C (meaning I) |
| 0x14 | `ROOM_SERVER_LIST_ANSWER` | *ServerResponse* | 8 | none in the client | — | 0x001508C8: consumes 8 bytes | C (meaning I) |
| 0x15 | `ROOM_SESSION_JOIN_REQUEST` | — | 4 | 0x0016F7D8 = dme_join_session (API 0x0014B448) to every active other client; 0x00150DD8 replies to a peer while joining | each other client, 0x50 | 0x00173D28: sender's state := +0, our 0x16 back; **if I am session master: 0x00173FC0 → 0x09 welcome, 0x17 order, 3 pings** | C |
| 0x16 | `ROOM_CLIENT_STATUS` | *UpdateClientStatus* | 0x10 | 0x0014BDB0(conn, dest or -1): on every state change (join, leave, welcome, peer hello) | one / all others, 0x50 | 0x00150DD8: updates the sender's record; state 5 → host := sender (0x00171D18, host migration); first → conn+0x420 callback; while joining → reply 0x15; else the per-state jump table 0x001B70D0 | C (jump-table cases I) |
| 0x17 | `ROOM_JOIN_ORDER` | — | 4 | 0x00173FC0 (session master, after 0x09) | the joiner | 0x00173DE0: my record +0x18 := order (max + 1), 0x16 to all | C (use of the order I: likely the host-migration rank) |
| 0x18 | `ROOM_VOICE_STREAM_SETUP` | — | 0x1C | 0x001754B0 (StreamMedia, from 0x00175190 / 0x00175A18) | one client | 0x00173E90: StreamMedia table [0x001A1B68]+0x24 | C (fields I) |
| 0x19 | `ROOM_LAN_FIND_REQUEST` | *LANFindPacket* | TLV | not via 0x0014CE08 (broadcast socket) | broadcast | 0x0014B500: answers with tag 0x8001 (0x6C info block: version 0x0207000A "2.7.10", app id, client count) | C (purpose I) |
| 0x1A | `ROOM_LAN_FIND_ANSWER` | *LANFindResultsPacket* | TLV | 0x0014B500 | the querier | 0x0014B998: parses tags 0x8001 (info) / 0x8002 (user data), filters (0x001A1B48), calls 0x001A1B48+8 | C (purpose I) |

<a id="l4-fields"></a>**Field layouts (C unless marked):**

| Id | Off | Size | Type | Field | Values |
|---|---|---|---|---|---|
| 0x01 ROOM_PING | 0 | 4 | u32 | sender's DME clock | ms (0x0014C6B0) |
| | 4 | 1 | u8 | pending slot | index into 0x001EA598 |
| | 5 | 1 | u8 | 1 request, 0 echo | |
| 0x02 ROOM_FRAGMENT | 0 / 1 | 1 / 1 | u8 | original class, original id | |
| | 2 / 4 / 6 | 2 each | u16 | piece length (≤ 0x1EA = 490; Horizon caps 484), piece count (⌈len/490⌉), piece index | |
| | 8 | 1 | u8 | message tag | random (0x00152768) |
| | 0xC / 0x10 | 4 / 4 | u32 | total length, offset of this piece | |
| | 0x14 | n | | piece bytes | |
| 0x03 Field update | 0 / 1 | 1 / 1 | u8 | type id, field count n | |
| | 2 | 2 | u16 | object id | < 0x1000 |
| | 4.. | | | n × {u8 field index, the field's bytes (size × count)} | |
| 0x04 Full state | 0 / 1 | 1 / 1 | u8 | creator client, type id (< 16) | |
| | 2 / 3 / 4 | 1 each | u8 | distribution flag (≠ 0: per-client bitmap), obj+3, obj+2 | |
| | 6 / 8 | 2 / 2 | u16 | owner client (0xFFFF none), object id | |
| | 0xC / 0x10 | 4 / 4 | u32 | obj+0x2C (I), obj+0x30 resend interval (I: ms, compared with the clock) | |
| | 0x14 | 16 | char | object name | the type name the game passes |
| | 0x24 | Σ | | every field in type order (sum < 0x1DB) | whole object, not a diff |
| 0x05 Stream closed | 0 | 1 | u8 | stream id | |
| 0x09 Join welcome | 0 | 4 | u32 | master's DME clock | ms |
| | 4 / 6 | 2 / 2 | u16 | joiner's object id base, stream id base | |
| 0x0A / 0x0B / 0x0D | 0 | 2 | u16 | object id | |
| 0x0C Ownership result | 0 / 2 / 4 | 1 / 2 / 1 | u8 / u16 / u8 | new owner, object id, result | 1 granted, 2 refused, 3 already owned (4 = released, hook only) |
| 0x0E Stream chunk | 0..4 | 1 each | u8 | stream id, route flag, stream type, ordered flag, buffered flag | |
| | 6 / 0xA / 0xC | 2 / 2 / n | u16 | buffer size, n, data | |
| 0x0F Client record | 0 | 1 | u8 | client index | |
| | 8 | 2 | u16 | the client's player object id | 0x1000 none |
| | 0xA / 0xC / 0xE / 0x10 | 2 each | u16 | object id base, count, stream id base, count | |
| | 0x16 | 12 | char | client name | dme_join_session's name (net+0x5E) |
| 0x10 Peer hello | 0 / 1 / 2 | 1 / 1 / 4 | | ? (I), sender's index, a 4-byte value compared with ours to drop our own echo (I: Horizon says the player's IPv4 address, which fits that check) | |
| | 8 | 64 | | public key (0x00151E40) | |
| 0x12 Leaving | 0 | 1 | u8 | leaver's index | |
| 0x15 Join request | 0 / 1 / 2 / 3 | 1 each | u8 | sender's state (3 joining), sender's index, object ids wanted (game: 3), streams wanted (0) | |
| 0x16 Status | 0 / 1 | 1 / 1 | u8 | state, sender's index | 0 none, 2 left/idle, 3 joining, 4 joined, 5 session master (host) (C, agrees with Horizon's NetClientStatus: 1 NotConnected, 2 Connected, 3 Joining, 4 Joined, 5 JoinedSessionMaster) |
| | 2 / 3 / 4 | 1 / 1 / 2 | | rec+0xC, rec+0x10, world slot +0x18 value (I) | |
| | 8 / 0xC | 4 / 4 | u32 | join order, conn+0x440 | |
| 0x17 Join order | 0 | 4 | u32 | order | 1 = first client |
| 0x18 Voice setup | 0 | 28 | u32[7] | StreamMedia parameters | I |

Session sequence: see [5.3](#s-dme-session).

#### 4.4.2 App channels (class 2)

`dme_register_app_handler(&id, handler)` **0x0014AB38** gives the next free class-2 id (`count[2]++`,
C). It has exactly 20 callers, all from `net_channels_register` 0x001987F0 at start-up (16 channels
by 0x00198868, then 4 by 0x00199180), so **the ids are the registration order and the same on every
console** (C): ids 0-15 = the channel variables 0x002405A8 + 4·n, 16 = 0x00240648, 17 = 0x00240640,
18 = 0x0024064C, 19 = 0x00240644 (V: the port's run saw id 19 every frame). Each channel is one row
of [layer 5](#l5).

<a id="l5"></a>
### 4.5 Layer 5: in game: the 20 app channels and the two net objects

PAL executable with level 39 (lobby, staging) or 40 (Bakisi Isles) overlaid; level code given as
its level-39 twin (`L39_...`, matched C in the decomp's `src/levels/shared/`) with the L40 address
where useful. Every channel is a DME class-2 message (layer 4): `[u8 2][u8 id][payload]`. Handlers
`handler(conn, index, src_client, payload)` return their fixed length. The 16 game channels go
through core receive wrappers (0x00198A38..0x00198FF0, one per channel) that stamp
`net+0x68C[slot of src]` = net time (channel 9 also net+0x7B0/net+0x7D0) and call the level's handler
at 0x002405E8 + 4·n (installed by L39_006774F0, cleared by L39_006775B8; none installed → -1).
Every map sender is guarded by `net+0x170 == 0` (online) and sends `kind = net+0x7D4 | flag`.
Kind: R = reliable (0x10/0x40/0x50 → TCP), U = unreliable (0x80 → UDP), RU = 0xC0 (TCP on the server
path). Dest -1 = every other client. `net` = the game's net state block 0x001A5CD0. Marks: V seen in
the port's runs, C read in code (fields C unless the field row says I), I inferred.

| DME id | Variable | Name | Horizon name | Len | Kind, dest | Sender: when | Receiver: what it does | Mark |
|---|---|---|---|---|---|---|---|---|
| 0 | 0x002405A8 | `CHANNEL_PLAYER_STATE_CHANGE` | — | 0x1C | U, all | L39_0064EA40 (L40 0x520B40), the hero update, on a state/command change | L39_006775E8: by the player index +0, if the player is valid (protected byte +0x19E4, net-player +0x2558), copies 28 bytes to net-player +0x255C | C |
| 1 | 0x002405AC | `CHANNEL_DAMAGE_EVENT` | — | 0x20 | R (one site U), all | L39_00539398 ×4, L39_0066E100, L39_00672C30: when this console's player or unit deals damage | L39_006776D8: kind 13 → L39_0053BED0, else L39_0066E100(attacker, victim, kind, 0, event UID, time) applies it | C |
| 2 | 0x002405B0 | `CHANNEL_PROXIMITY_TRIGGER` | — | 3 | RU / U, all | L39_0054C7E8 (L40 0x404568), L39_0054B4B8: a level object sees the player within range and line of sight | the level's handler (installed by L40 0x004EDEF0) | C (receiver's use I) |
| 3 | 0x002405B4 | `CHANNEL_VEHICLE_UPDATE` | — | 0x24 | R (0x10), all | L39_006670C0 (L40 0x5391C0): the driver's vehicle state | L39_00677B40: only if the UID's type nibble is 3 (vehicle) → L39_006684A0 | C |
| 4 | 0x002405B8 | `CHANNEL_PLAYER_KILLED` | — | 0xC | R, all | L39_00628038 (L40 0x4F9F30): when this console's player dies; also applied locally | L39_00677B88 → L39_00670AE0: the kill (score, respawn) | C |
| 5 | 0x002405BC | `CHANNEL_MATCH_STATE` | — | 0x68 | R, all or one | core 0x001959F8 (0x80, 0x81), L39_0066F948 (state, to one client), L39_00670C50 (0x82 end/scores, host) | L39_00677CB8 → L39_0066FA90: 0x80 → net+0x7D4 = 0x40 (everyone reliable); 0x81 ignored; 0x82 end and score blocks; else the match state into the game-mode struct 0x00352940 | C |
| 6 | 0x002405C0 | `CHANNEL_UNUSED_6` | — | — | — | no sender | no handler installed | C |
| 7 | 0x002405C4 | `CHANNEL_UNIT_MESSAGE` | — | var | R (0x50), all | L39_0066BD80 (L40 0x53ED70): every 15th frame, ≤ 5 queued messages (create/update/destroy of AI units), also applied locally | L39_0066C110: queues it on the unit's inbox (unit+0x38) or the create inbox 0x0024CF64; destroy of an unknown unit destroys it | C |
| 8 | 0x002405C8 | `CHANNEL_UNUSED_8` | — | — | — | no sender | none | C |
| 9 | 0x002405CC | `CHANNEL_PLAYER_STATE_STREAM` | — | 0x18 + n (≤ 0x98) | U, all | L39_0065F4B8 (L40 0x5315B8): every frame while the player is active (not the first 5 after spawn), the extended block every 5th | L39_00677740 → core 0x0019AF90: 16-entry jitter buffer (net-player +0x1F0, 0xA4 each) ordered by seq (window 100), played back with a delay; refreshes net+0x7B0 (UDP health) | C |
| 10 | 0x002405D0 | `CHANNEL_PLAYER_SPAWN` | — | 0x20 | R, all | L39_00637EC8 (L40 0x509DC0) | L39_006778A0: copies 32 bytes to net-player +0x1D0, flag 2 at +0x1AC | C |
| 11 | 0x002405D4 | `CHANNEL_OBJECT_LINK` | — | 8 | R, all | L41 0x0041C170 (not on L40) | L39_006779A8: moby (0x0024BF5C + index·0x100, skipped when its state is 0xFE/0xFD): if `*(moby+0x68)[0]` is -1, stores the value | C |
| 12 | 0x002405D8 | `CHANNEL_DELAYED_DIRECTED_MESSAGE` | — | var | R, one client | L39_00662830 (L40 0x534930): a queue (0x001DAAE4) of nodes sent when net time ≥ node+0x4C to client node+0x50 | L40 0x0054B400 → 0x005349F0 (L01_00489FB0) | C (payload I) |
| 13 | 0x002405DC | `CHANNEL_UNUSED_13` | — | — | — | no sender | none | C |
| 14 | 0x002405E0 | `CHANNEL_WEAPON_FIRE` | — | 0x24 | U, all | L39_005514A0 (L40 0x409220), L39_00672DD8 (L40 0x546510): when this console's player fires | L39_00677A20 → L39_00672DD8: runs the weapon at player +0x1A40 with the sender's time | C |
| 15 | 0x002405E4 | `CHANNEL_KNOCKBACK` | — | 0x10 | U, all | L39_00627EF8 (L40 0x4F9DF0), L39_006618D8 (L40 0x5339D8): an impulse on another player | L39_00677BB8 → L39_00627F88: adds the vector to the target's +0x24EC, ORs the flags into +0x24FA | C |
| 16 | 0x00240648 | `CHANNEL_ROOM_JOIN_REQUEST` | — | 0x58 | R (0x50), all | L39_006D34D8 (LOBBY_GAME_JOIN_REQUEST flow, in `L39_006D08E8.c`) after the DME connect and session join, when not host; L39_006D3A30 after create (ignored by others); then waits on net+0x630 | 0x0019A6F8 (host only): checks room, account, start, teams; on accept fills free OBJECT_GAME_SETTINGS slots and sends the whole object (0x0016EF40), world report due; answers on id 18 || V |
| 17 | 0x00240640 | `CHANNEL_SLOT_UPDATE` | — | 0x20 | R (0x50), client → host or host → one client | client: L39_006D4ED8 (ready 6 / not ready 0, from staging L39_006C20B0), L39_006C4308 (leave: 1), L39_00575CC8 (L40 0x4425C8: map loaded, 8); host: L39_006A0540, L39_006A5F30, L39_006CC018 (slot released 2/3), L39_006C4360 (removed, 4) | 0x0019A4D0: host → applies to the sender's slot in OBJECT_GAME_SETTINGS; client → net+0x680 = 1, +0x684 = state, +0x688 = local index (the staging screen reacts) || V |
| 18 | 0x0024064C | `CHANNEL_ROOM_JOIN_ANSWER` | — | 4 | R (0x50), the requester | 0x0019A6F8 (host) | 0x0019AE40: net+0x634 = result, net+0x630 = 0 (ends the wait; L39_006D34D8 maps 4/1/3/5 to UI errors 0x31/0x32/0x33/0x34) || V |
| 19 | 0x00240644 | `CHANNEL_TALKING_FLAG` | — | 4 | R (0x50), **the host** | the voice update 0x0019C2C0 (non-host), when `[0x00241704] % 60 == 0`: that word is the network's vblank counter (incremented by the VBLANK_S handler 0x0019BC10, installed by 0x0019BEB8 with AddIntcHandler2(2, ..)), so once every 60 vblanks, 1.2 s PAL | 0x0019A460 (host only): sets/clears bit `1 << src` in OBJECT_GAME_SETTINGS +0x19E | V |

#### 4.5.1 Channel field layouts (C unless marked)

| Ch | Off | Size | Type | Field | Values / units |
|---|---|---|---|---|---|
| 0 | 0 / 1 | 1 / 1 | u8 | player index, command/state | |
| | 2 / 3 | 1 / 1 | u8 | sequence counters (player +0x44ED / +0x44EE) | |
| | 4 / 0x10 | 12 / 12 | vec3 | player +0xA0, +0xB0 (I: velocity / position offset) | |
| 1 | 0x00 | 12 | vec3 | hit position | world units |
| | 0x0C | 4 | s32 | net time | ms |
| | 0x10 / 0x14 / 0x18 | 4 each | u32 | attacker UID, victim UID (type 0 player, 2 unit, 3 vehicle), event UID | |
| | 0x1C | 1 | u8 | damage kind | 13 = special path |
| 2 | 0 | 1 | u8 | the object's index (its vars +0x64) | (meaning I) |
| | 1 | 1 | u8 | command | 2 = player in range |
| | 2 | 1 | u8 | value (vars +0x68) | (meaning I: pickup/trigger) |
| 3 | 0x00 / 0x0C | 12 / 12 | vec3 | vehicle +0x230 / +0x240 (or +0x210) (I: position, velocity) | |
| | 0x18 | 4 | s32 | seat/driver (I) | |
| | 0x1C | 4 | u32 | vehicle UID (type nibble 3 checked) | |
| | 0x20 | 4 | s32 | flags | 1/2/4/8 (I) |
| 4 | 0 | 1 | s8 | killer player index | -1 none / not a player |
| | 1 | 1 | u8 | victim's decoded state byte (protected +0x19E4) | |
| | 2 / 3 | 1 / 1 | u8 | victim player index, cause (player +0x24D0) | |
| | 4 | 1 | u8 | random 0-15 | |
| | 8 | 4 | u32 | killer UID (player +0x24CC, last attacker) | |
| 5 | 0x00 | 1 | u8 | end result (0x82: argument of the end, L39_00670D90) | |
| | 0x04 / 0x08 | 4 / 4 | s32 | game-mode words (0x00352940 +0 / +4) | |
| | 0x0C | 1 | u8 | command | 0x80 all reliable, 0x81 UDP probe (ignored), 0x82 end/scores, other = state (mode +8) |
| | 0x0D | 1 | u8 | sub-state (mode +0xC) | |
| | 0x0E / 0x10 | 2 / 2 | s16 | 0x82: two counters (mode +0x210 → +8/+0xC) (I: team scores) | |
| | 0x12 | 8 | u8[8] | per-player state | one per slot |
| | 0x1C | 4 | s32 | 0x82: mode +0x10; -1 in state sends | |
| | 0x20 / 0x40 / 0x50 / 0x58 / 0x60 | 32 / 16 / 8 / 8 / 8 | | 0x82 score blocks (mode +0x18, +0x198, +0x38, +0x48, +0x50) | meanings I |
| 7 | 0x00 | 4 | s32 | size of the whole message | |
| | 0x04 / 0x08 | 4 / 4 | s32 | sender client, type | 0 create, 1 update, 2 destroy, 3 create-like, 4/5 ownership hand-over (Ranger), 6 special |
| | 0x0C / 0x10 / 0x14 | 4 each | u32 / s32 / s32 | unit UID (creator in the top nibble), time (frame + delay), unit class | |
| | 0x18 | n | | the class's data (`include/structs/guber.h`, names.tsv ranger_*, ball_bot_*, ...) | |
| 9 | 0x00 | 16 | | 4 recent history words (or 8 bytes from player +0x431C in vehicle/turret mode) | |
| | 0x10 | 8 | bits | x, y, z as u16 (position·64 + 0.5), seq u8; word +0x14: bits 28-31 player index, bit 25 local-index flag, bit 24 extended block, 2 mode bits | |
| | 0x18 | 8 + ⌈bits/8⌉ | | input/animation bitstream (player +0x430C), length from L39_0066F260 | |
| | … | 0x11 | | extended block (every 5th): packed vector (player +0xB0), 1 protected byte (I: health), f32 position ×3 | |
| 10 | 0 / 1 / 2 / 3 | 1 each | u8 | player index, two flags (I), sequence | |
| | 4 / 0x10 | 12 / 12 | vec3 | position, rotation | |
| | 0x1C / 0x1E | 2 / 1 | s16 / u8 | (I) | |
| 11 | 0 / 4 | 4 / 4 | s32 | moby index, value stored | |
| 12 | 0 | n | | the queued node's bytes | I |
| 14 | 0x00 | 2 | s16 | fire kind (3) | |
| | 0x02 / 0x03 | 1 / 1 | u8 | player index (low nibble) \| 0x40/0x80 flags, weapon | |
| | 0x04 / 0x08 | 4 / 4 | s32 / u32 | net time (ms), target UID | |
| | 0x0C / 0x18 | 12 / 12 | vec3 | origin, offset to the target | |
| 15 | 0x00 | 12 | vec3 | impulse | |
| | 0x0C / 0x0D / 0x0E | 1 each | u8 | sender's sequence (net-player +0xC34), target player (low nibble), flags (net-player +0xC31 \| 0x40) | |
| 16 | 0x00 | 4 | s32 | local players asking (net+0xC: 1, or 2 split screen) | |
| | 0x04 | 4 | s32 | account id (LOBBY_LOGIN_ANSWER) | refused (2) if already in OBJECT_GAME_SETTINGS +0x150 |
| | 0x08 / 0x18 | 16 / 16 | f32[4] | net+0x88 / net+0x98 per-mode values → OBJECT_GAME_SETTINGS +0x110 / +0x130 | I: per-mode ratings |
| | 0x28 / 0x2C | 4 / 4 | u16[2] | team pick, skin per local player | |
| | 0x30 / 0x40 | 16 / 16 | char | name (net+0x5E), second player's name ("%s ~ %d") | |
| | 0x50 | 8 | char | clan tag (net+0x80, 4 chars used) | |
| 17 | 0x00 | 4 | s32 | local index (which of the sender's slots) | |
| | 0x04 / 0x05 / 0x06 | 1 each | u8 | team pick, skin, slot state | states below |
| | 0x07 / 0x17 | 16 / 4 | char | name, clan tag | |
| 18 | 0 | 4 | s32 | result | 0 ok, 1 room full, 2 refused, 3 no reserved slot (I: clan hook), 4 already started, 5 team full |
| 19 | 0 | 4 | s32 | talking | 1 while the voice stream handle 0x002416B8 is open (I: "talking"), else 0 |

Slot states (OBJECT_GAME_SETTINGS +0xF0[slot], C from L39_006C1730 / L39_006C20B0 / L39_00575CC8 / 0x0019A4D0):
0 joined, not ready; 1 request: leave this slot; 2/3 host: slot released (in staging / after start);
4 host: you are removed; 5 being kicked; 6 ready; 7 (slot 0) START pressed; 8 map loaded.

<a id="l5-objects"></a>#### 4.5.2 Net objects

Types are registered once by 0x00199180 (`nwtype_add_field(&id, offset, elem_size, type, count,
default)` 0x0016EB60 → 0x001741A8: ≤ 0x40 fields a type, field data < 0x1DB bytes a type, ≤ 16
types). Type codes (from the sizes and values: I for the names): 0 bytes/char, 1 u8, 2 s16, 3 u16,
4 s32, 5 UID, 6 f32, 0xD vec3. On the wire an object is DME class 0 **0x04** (whole) or **0x03**
(changed fields) ([layer 4](#l4)); the object record (`nwobj_get` 0x0016ED00): +0x00 type, +0x04
owner client (-1 none), +0x08 **creator client index**, +0x0C name, +0x1C data, +0x20 last-sent copy.

| Object | Horizon name | Size | Created by: when | Callbacks (create / update / destroy) | Mark |
|---|---|---|---|---|---|
| `OBJECT_PLAYER_DATA` (the game's string "tNW_PlayerData" at 0x00241678; type var 0x0024063C) | — (the game's own string) | 0x90 | each console for each of its players: 0x0019A180(index, spawn pos, spawn rot, ?, spawn kind, team, max health, ..) from L39_00661A40 (L40 0x533B40), then nwobj_send and nwobj_set_player | 0x00199A60 (net-player slot = data +0x54; net-player +0 = record +0x08; clears the jitter buffer) / 0x00199B00 (field ≠ -2: bit `1 << field` in data +0x7C) / 0x00199B78 | C |
| `OBJECT_GAME_SETTINGS` (the game's string "tNW_GameSettings" at 0x00241688; type var 0x00240638) | — (the game's own string) | 0x1A4 | the host only: 0x00196150 → 0x00199C80 when the session is joined (offline at 0x002359E0); sent whole at creation, after each accepted join, at START (then released, 0x00199A20); field changes by DME's tick | 0x00199968 (adopt the pointer gp-0x6980, +0x1A0 = id) / 0x001999B8 (net+0x130 = +0x178, net+0x638 = +0x17E) / 0x001999E8 || V |

**OBJECT_PLAYER_DATA (0x90):**

| Off | Elem·count | Type | Field | Meaning (C unless marked) |
|---|---|---|---|---|
| 0x00 | 0x10·4 | 6 | position (field 0) | spawn x, y, z at creation; registered as 4 × 16 bytes overlapping the next fields (I: a 4-vector history) |
| 0x10 | 0xC·1 | 0xD | spawn rotation | vec3 |
| 0x1C | 4·1 | 4 | creation time | net time, ms |
| 0x34 | 4·1 | 4 | account id | net+0x34 |
| 0x38 | 4·1 | 5 | player UID | `client << 28 \| 0 << 24 \| counter` (0x00197498) |
| 0x3C | 4·1 | 4 | spawn kind | player +0x251A (indexes 0x001DAAA0; meaning I) |
| 0x40 | 4·1 | 4 | team | OBJECT_GAME_SETTINGS +0xD0[slot] |
| 0x44 | 4·1 | 6 | max health | 15.0 at setup (name I) |
| 0x48 | 4·1 | 4 | (player +0x478) | I |
| 0x4C / 0x50 | 4·1 each | 4 | (0 at creation) | I: no reader found |
| 0x54 | 4·1 | 4 | player slot | 0-7 |
| 0x58 / 0x64 / 0x70 | 4·3 each | 4 | invert Y, invert X, camera speed, per camera mode | from 0x001A58D0 / 0x001A5930 / 0x001A5990 (speed /64, clamped 0.25-1.25) |
| 0x7C | 4 | — | dirty mask (local, not a field) | `1 << field` |
| 0x80 / 0x82 / 0x84 | 2·1 each | 3 | (0 at creation) | I: no reader found |
| 0x86 | 2·1 | 3 | camera mode | 0, 1, 2 (0x001A58B4[slot]); applied by L39_00647150 (mode names I) |

**OBJECT_GAME_SETTINGS (0x1A4):**

| Off | Elem·count | Type | Field | Meaning (C unless marked) |
|---|---|---|---|---|
| 0x000 | 16·8 | 0 | name[slot] | host "%s" of net+0x5E; extra local player "%s P%d" offline, "%s ~ %d" online |
| 0x080 | 8·8 | 0 | clan tag[slot] | 4 chars |
| 0x0C0 | 2·8 | 2 | skin[slot] | |
| 0x0D0 | 2·8 | 2 | team[slot] | 0x00199C48: free-for-all → slot index, else the pick |
| 0x0E0 | 2·8 | 2 | client[slot] | owning DME client index, -1 empty |
| 0x0F0 | 2·8 | 2 | state[slot] | slot states above; [0] ≥ 7 = started |
| 0x100 | 2·8 | — | reservation[slot] (local) | 3 the host's, 1-8 from joins |
| 0x110 / 0x130 | 4·8 each | 6 | per-mode values A / B [slot] | from the join request +0x08 / +0x18 (I: ratings) |
| 0x150 | 4·8 | 4 | account id[slot] | -1 empty |
| 0x170 | 4·1 | 4 | map start time | net time + 550 ms once all reached state 8 |
| 0x174 | 4·1 | 4 | match start time | START: net time + 4000 ms; staging starts the map at it |
| 0x178 | 4·1 | 4 | player count | → net+0x130 on update |
| 0x17C | 2·1 | 2 | map level | create option 0x10 + 0x27 (40-55) |
| 0x17E | 2·1 | 2 | (→ net+0x638) | I: no other reader |
| 0x180 | 4·1 | 4 | game mode | create option 0x11 (names I) |
| 0x184 | 4·1 | 4 | rules set | create option 0xC |
| 0x188 | 1·22 | 0 | create-menu option bytes | copy of 0x00241648; +0x19D = team-play flag; others I |
| 0x19E | 1·1 | 1 | talking mask | bit per client (channel 19) |
| 0x19F | 1·1 | 0 | option 0x16 | GenericField3 bit 22 (meaning I) |
| 0x1A0 | 4 | — | object id (local) | |

<a id="l6"></a>
### 4.6 Layer 6: the port's own payloads (`mp_net.c`)

The port replaces Medius at `MediusSendLobbyMessage` (its lobby answers from `lobby_request`) and DME
at its API (section 7.2), so between consoles only the OpenRAC server's `PACKET_RELAY` (packet 7) and the
party's advert (`PARTY_ADVERT`) carry game bytes. A `PACKET_RELAY` payload starts with a one-byte kind; the
integers the port adds are big-endian (`srv_i32`), the game's own bytes inside stay little-endian.
Reliability and order: TCP through the server (`PACKET_RELAY` never goes over UDP), so every kind is
reliable and ordered per sender, including the game's unreliable kinds (0x80). Marks: V seen in the
runs (`OPENRAC_RAC3_NET_LOG` "room send/packet"), C read in the port's code.

| Kind | Name | Horizon name | Sender: when | Receiver: what it does | Payload after the kind byte | Mark |
|---|---|---|---|---|---|---|
| 1 | `RELAY_ROOM_PACKET` | — | `dme_relay` ← `dme_send_one` ← the port's 0x0014AD08 (every app channel), 0x0014CAB8 / 0x0014C950 (DME's internal sends): dest client index → that member's session, -1/0xFFFE → target 0 (all others), 0xFFFF (DME server) dropped | `on_frame` queues it (`dme.inbox`, ≤ 64); `dme_deliver` (in the port's dme_update 0x0014B118) calls 0x0014CE48(conn+0x434, conn, the sender's client index, buf, len) → 0x00152FB8 → the game's handlers. Senders not among client indices 0-7 are dropped | `[u8 class][u8 id][payload]` exactly as DME would send it (≤ 0x5F0 bytes): class 2 = a layer-5 channel, class 0 = a layer-4 message | V (class 2 id 19 every frame) |
| 2 | `RELAY_OBJECT` | — | `obj_send` ← the port's nwobj_create 0x0016EC18 (to all), nwobj_set_player 0x0016ED68 (to all, owner only), `obj_send_all_to` on `PARTY_JOINED` (to the newcomer) | `obj_receive`: new id → a record in guest memory (+0 id, +4 owner, +8 id, +0x1C data) and the type's on_create(0, id); known → copies the data, on_update(0, id, -2) | `s32` object id (owner's client index << 16 \| counter), `s32` type id, `s32` owner client index, then the object's bytes (the type's registered size) | V |
| 3 | `RELAY_OBJECT_RELEASE` | — | the port's nwobj_release 0x0016EDD0 (to all) | `obj_receive`: on_destroy(0, id), the record dropped | `s32` object id | C |
| — | `GAME_ADVERT` | — | `srv_advert` ← `lobby_send_advert`: after LOBBY_GAME_CREATE_REQUEST's `PARTY_INFO`, and when a LOBBY_GAME_REPORT changes the game | `advert_read` ← `on_list`: one LOBBY_GAME_LIST_ENTRY record per party with a version-1 advert; LOBBY_GAME_INFO_ANSWER from it | [below](#l6-advert) | V |

<a id="l6-advert"></a>**Advert v1 (C, 361 bytes; the server keeps it opaque, ≤ 512):**

| Off | Size | Type | Field | From |
|---|---|---|---|---|
| 0 | 1 | u8 | version | 1 (`ADVERT_VERSION`); other versions are not listed |
| 1 | 40 | s32[10] LE | MinPlayers, MaxPlayers, GameLevel, PlayerSkillLevel, RulesSet, GenericField1-3, WorldStatus, GameHostType | LOBBY_GAME_CREATE_REQUEST (+0x2C..+0xCC), then each LOBBY_GAME_REPORT (+0x15C..+0x17C) |
| 41 | 64 | char | GameName | LOBBY_GAME_CREATE_REQUEST +0x38 / LOBBY_GAME_REPORT +0x1C |
| 105 | 256 | | GameStats | LOBBY_GAME_REPORT +0x5C |

Port-side values the game sees (C): session key `OPENRACSESSION01`; world ids 2 (the one lobby
channel "OpenRAC CH 01"), 900 (this console's own game), 1000 + n (the n-th party code listed);
NetConnectionInfo `127.0.0.1:10075`; AccountID = the server session id; DME client index = the
member's `PARTY_SLOTS` slot (else the party's join order).

Differences from the game's own DME semantics, found by this verification (C, details in 7.6):
kind 2 carries no creator index and calls on_update with -2 (the game's callbacks ignore -2: whole
objects come with -1); kind 3 destroys where the game only releases ownership; nothing relays
nwobj_send 0x0016EF40 (the game's "send the whole object") or DME's per-field updates (0x03).

---

<a id="s5"></a>
## 5. Flows, roles and timing

### 5.1 The lobby flow (level 39; C)

Lobby context `D_L39_0024D760` (created by L39_006D7930): +0x01 waiting, +0x08 deadline (now +
0x24000 ticks), +0x10/+0x14 UI error codes, +0x30 account, +0x3C game, +0x4C channel, +0x50 connection
object (SessionKey +0, server IP +0x11, port +0x94, +0x9D connected), +0x54 announcements, +0x70 game
list. Every blocking step sets a waiting byte, sends, then loops `0x00195410(); if
(!L39_006D8AD8(ctx)) fail;` until the callback clears it. The pump hook L39_0067CA48 runs the net
update 0x001959F8 and the lobby's periodic work L39_006D8B88 (game-list and channel refresh).

| Step | Function | Messages (layer 3) |
|---|---|---|
| init | L39_006D8D80 | MediusInitialize (ChatFwd, LOBBY_SERVER_ERROR_NOTICE, ReassignWorldID callbacks) |
| authentication server + session | L39_0068C210 → L39_006DC7C8(conn, crypto byte 0x001A5E50, ip 0x001A5E52, port 0x001A5F94) → L39_006DCDB8 | MediusConnect (4 tries), LOBBY_SESSION_OPEN_REQUEST, MediusGetSessionKey, LOBBY_LOCALE_SET_REQUEST, LOBBY_DNAS_SIGNATURE_REPORT |
| login | L39_006CCD08 → L39_006CCE90 | LOBBY_LOGIN_REQUEST (-997: LOBBY_LOGOUT_REQUEST and retry) |
| lobby server | L39_006CCD08 → L39_006DD340, L39_006DD060 | MediusDisconnect, MediusConnect(reply +0x28), LOBBY_PRESENCE_UPDATE(1); LOBBY_MY_CLAN_REQUEST, LOBBY_LADDER_STATS_READ_REQUEST, LOBBY_LADDER_STATS_WRITE_REQUEST, LOBBY_BUDDY_LIST_REQUEST |
| licence, news, chat | L39_006DDE00, L39_006DD860, L39_006DE030 | LOBBY_LICENCE_TEXT_REQUEST, LOBBY_NEWS_REQUEST / LOBBY_ALL_NEWS_REQUEST, LOBBY_CHAT_TOGGLE_REQUEST |
| city | L39_006D7018, L39_006DC148 | LOBBY_CITY_LIST_REQUEST, LOBBY_CHANNEL_FILTER_SET_REQUEST (the city bit) |
| channel | L39_006DB948 (auto-pick: numbered channels, < 219 players), L39_006DAFC8, L39_006DA3F8, L39_006DB750 | LOBBY_CHANNEL_LIST_REQUEST, LOBBY_CHANNEL_CREATE_REQUEST (none fits), LOBBY_CHANNEL_INFO_REQUEST |
| join channel | L39_006DA680 | LOBBY_CHANNEL_JOIN_REQUEST; reconnect to its server; LOBBY_PRESENCE_UPDATE(1) |
| game list | L39_006D5AB8 (filters L39_006D52D0 / L39_006D5460) | LOBBY_GAME_LIST_FILTER_ADD_REQUEST / LOBBY_GAME_LIST_FILTER_REMOVE_REQUEST, LOBBY_GAME_LIST_REQUEST |
| details | L39_006D4188 / L39_006D4380 | LOBBY_GAME_INFO_REQUEST, LOBBY_GAME_PLAYER_LIST_REQUEST |
| create | L39_006D3A30 (create screen L39_0069A630) | LOBBY_GAME_CREATE_REQUEST, LOBBY_GAME_JOIN_REQUEST (own world), LOBBY_GAME_REPORT, LOBBY_GAME_INFO_REQUEST poll (≤ 21) until it echoes the report; DME connect + join; room join request (ch 16); game chat channel (L39_006DC3D0) |
| join | L39_006D34D8 | LOBBY_GAME_JOIN_REQUEST → DME connect 0x00195530(reply +0x20), session join 0x00195810, room join request (ch 16), wait for ch 18 (net+0x630); LOBBY_GAME_PLAYER_LIST_REQUEST |
| in game | core 0x001959F8 | LOBBY_GAME_REPORT (host, 15 s), LOBBY_PLAYER_STATS_REPORT (15 s), LOBBY_KEEP_ALIVE_REQUEST (keep-alive), LOBBY_CAREER_STATS_UPLOAD_REQUEST |
| leave | L39_006D40B0 → core 0x00196038 | LOBBY_GAME_REPORT (status 3), LOBBY_GAME_LEAVE_REPORT, dme_leave_session + dme_disconnect, LOBBY_PRESENCE_UPDATE(2), LOBBY_PRESENCE_UPDATE(1) |
| log out | L39_006DCAD0 | LOBBY_SESSION_CLOSE_REQUEST, MediusDisconnect |

There is no "start game" lobby message: the match starts in the room (slot state 7, OBJECT_GAME_SETTINGS
+0x174, channel 5).

<a id="s-dme-session"></a>
### 5.2 Connecting to a game room (C)

1. 0x00198690 (once): DME init (UDP port 6000 + rand%500, callback 0x00197598); 0x001987F0 registers
   the 20 channels and the two object types.
2. **0x00195530**(connect info): net state 6, `dme_connect_world` (RT handshake, layer 2), loop {1 ms
   sleep, dme_update, status} until connected → net+4 = DmeConn*, state 7.
3. **0x00195810**: state 9, `dme_join_session` (name net+0x5E, callbacks 0x00195F98 join complete,
   0x001970F8 client status, 0x00197460 host changed), loop until state ≠ 9 (30 s inside DME).
4. Inside DME (layer 4): RT 0x08 for each client already there → **0x10** peer hello both ways →
   **0x16** status. The join sends **0x15** to every other client; with nobody there the console
   becomes session master (state 5, host = itself, order 1, join complete at once). Otherwise the
   master answers **0x16**, **0x09** welcome (its clock, the joiner's id ranges, host := master),
   **0x17** order and 3 **0x01** pings; the joiner broadcasts **0x0F**; each console seeing a client's
   first 0x0F calls the game's client-status callback (kind 0: net+0x148[n] = client, net+0x6AD[client]
   = n, net+0x12C++) and the object tick sends the newcomer every owned object (**0x04**).
5. State 0xA: 0x00196150 sets net+0x16C = host (0x0016EFB0) and, on the host, creates OBJECT_GAME_SETTINGS
   (0x00199C80); dme_set_timeouts (90,90) / (30,30) / (0,0).
6. The joiner sends the **room join request** (ch 16); the host accepts into OBJECT_GAME_SETTINGS, sends it
   whole (**0x04**) and answers (ch 18). Staging: **slot updates** (ch 17) both ways; OBJECT_GAME_SETTINGS field
   changes go out as **0x03** from DME's tick.
7. START: the host sets slot 0 state 7 and +0x174 = now + 4 s, sends OBJECT_GAME_SETTINGS whole and releases
   it (**0x0D**, 0x00199A20); each console loads the map, creates its **OBJECT_PLAYER_DATA** (0x0019A180)
   and reports state 8 (ch 17); the host sets +0x170.
8. Leaving: **0x12** (dme_leave_session) → client-status kind 1 on the others; dme_disconnect.
   Host migration: a **0x16** with state 5 makes its sender host (0x00171D18 → callback 0x00197460:
   net+0x16C, net+0x168, net+0x169).

Offline / split screen (net+0x170 = 1): nothing is sent, the join is synthesised (index 0, host =
me), OBJECT_GAME_SETTINGS is created locally and OBJECT_PLAYER_DATA uses the copies at 0x002357A0.

### 5.3 Roles

The host (the DME session master) owns OBJECT_GAME_SETTINGS, slot assignment (ch 16/17/18), the match
state (ch 5, scores 0x82) and the AI units it creates; each console owns its OBJECT_PLAYER_DATA, its
player stream (ch 9) and the events it causes (ch 0, 1, 4, 10, 14, 15).

### 5.4 Rates and timeouts (C)

| What | Value |
|---|---|
| dme_update, the net update | every frame (50 Hz PAL); the net time advances 20 ms a frame when the DME clock is not steady |
| Player state stream (ch 9) | every frame, extended block every 5th |
| Unit messages (ch 7) | every 15 frames, ≤ 5 |
| Host silence | net+0x6AC = 1 after 1,200 ms without a message from the host |
| UDP health | a client's stream older than 3,000 ms → ch 5 command 0x81 (≤ every 3 s); every stream ≥ 10,000 ms behind → net+0x7D4 = 0x40 and ch 5 command 0x80 (everyone reliable) |
| Talking flag (ch 19) | every 60 vblanks (1.2 s PAL): `[0x00241704]`, counted by the vblank handler 0x0019BC10 |
| LOBBY_KEEP_ALIVE_REQUEST keep-alive | player stream silent > 1,500 ms, ≥ 2 clients |
| LOBBY_GAME_REPORT / LOBBY_PLAYER_STATS_REPORT | every 15,000 ms (or when flagged) |
| Join wait | 30,000 ms (0x0016F7D8); lobby steps: deadline 0x24000 ticks; ladder requests 5,000 ms |
| RT echo, aux UDP hello | echo at the connection's interval (+0x2C); 0x16 resent every 200 ms until 0x19 |

### 5.5 Below the protocol

The rest of the network start-up is kept in `parts/transport.md` (unchanged): the IOP modules and
RPC servers (1.2, 1.3), ezNetCnf/ezNetCtl (2), the socket layers libnet / libnetb / rt_comm's 25-slot
table 0x001D3970 and the DNAS insock shim (3), DNAS (4), the rt_msg_client API table 0x001D7E30 and
rt_crypt (5.4, 5.5), and the hook points. Server addresses (C, `mp_level_boot` 0x0059DEB8 fills
net+0x180..): +0x180 crypto byte (1), +0x182 the IP string in use, +0x1A2 `randc3-master.online.scee.com`,
+0x202 `nightcrawler.insomniacgames.com` (unused), +0x262 `randc3-prod.rt.au.playstation.com`,
+0x2C4 port 10075 (0x275B); the resolver L39_0067A7E0 fills the IP strings; L39_0068C210 picks the
SCEE or the Australian server by the network profile's byte +0x4E.

---

<a id="s6"></a>
## 6. What is still inferred (I), why, and how to confirm it

No item of any layer is I: every packet, frame id, request, DME message, channel and object above has
its sender and receiver read in the code (C) or was also seen in a run (V). What remains inferred is
the meaning of some fields and enum values, listed here. "Retail capture" means a recorded session
with a retail-compatible Medius/DME server (none is available); "port run" means a logged
`OPENRAC_RAC3_NET_LOG` run on box1, which the protocol work can ask for.

| Layer | Field(s) | Why it is I | Plan to confirm |
|---|---|---|---|
| 2 RT | 0x07 accept +5 (u16 → conn+0x58), 0x0D's flag, bodies of 0x11 / 0x19 / 0x1C, the 0x1D query layout | the client stores or ignores them; only a server gives them meaning | read the readers of conn+0x58 and 0x0016D9A0 in full; a retail capture for the server-only bodies |
| 2 RT | where 0x12 (client public key) falls in the handshake; the two 64-byte key blocks 0x001A3360/0x001A33A0 | the sender 0x0016C6C8 is reached through the encrypted-connect path, not followed to its caller | follow the callers of 0x0016C6C8 and 0x00183BE8; irrelevant to the port (it runs no RT) |
| 3 Medius | enum values the game writes but never interprets: ConnectionClass 1, character encoding 2, the language codes, PolicyType 1, LOBBY_CHAT_TOGGLE_REQUEST 0/1, LobbyFilterType, FilterMaskLevelType 0x20, LadderType 0, ComparisonOperator, JoinType 0, WorldStatus, GameHostType 0/4 names | the code just passes constants; Horizon's enums name them (given at each field in 4.3.2: Ethernet, UTF8, the languages, Privacy, EqualsFilter, Level123, WorldClosed, ...) | none needed for the port (its lobby echoes them); a retail capture would confirm |
| 3 Medius | reply fields the game copies whole but does not read: AccountType and WorldID (login reply), LOBBY_ACCOUNT_INFO_REQUEST, LOBBY_CHANNEL_INFO_REQUEST and buddy-list inner fields, LOBBY_KEEP_ALIVE_REQUEST's IP, LOBBY_CHANNEL_FILTER_SET_ANSWER +0x1C.., LOBBY_GAME_LIST_ENTRY record +0x22..+0x48 and +0x8C, LOBBY_GAME_INFO_ANSWER +0x1C/+0x30/+0x34/+0x184/+0x188, LOBBY_GAME_JOIN_ANSWER +0x1C, LOBBY_CHAT_TEXT_CHECK_ANSWER text | the copies' readers (the screens that draw them) were not followed | trace the context offsets of each copy (e.g. channel ctx+0xF8, ctx48+4) into the screen code of level 39; or a port run with distinctive values and a look at the screen |
| 3 Medius | clan, ignore and ladder messages: field names beyond the offsets | only StatusCode and EndOfList were checked in their handlers | read the clan screens 0x006CE*..0x006D33xx when clans are implemented |
| 3 Medius | LOBBY_GAME_REPORT PlayerCount (+0x18) | 1 at create, no other writer found | search the writes into 0x001A6174 (net+0x4A4) by all levels; a port run where a second player joins |
| 3 Medius | GameName/settings option meanings (29 options, game-mode and RulesSet names) | no strings tie options to names | match the create screen's option rows (L39_006AC860, table 0x0047F0C0) with the screen's text ids in a port run |
| 3 Medius | channel GenericField2-4 / GenericFieldLevel, SecurityLevel, MaxPlayers; NetConnectionInfo AccessKey and callbacks +0xF8/+0xFC | not read on the paths followed | read L39_006DB098's list screens and the RT connect for the AccessKey |
| 4 DME | 0x00, 0x13, 0x14 meanings | sent only by the DME server; the client stores (0x00 → conn+0x410) or skips them | read the readers of conn+0x410; a retail capture |
| 4 DME | 0x05 / 0x0E data streams and 0x18 voice setup: purpose and the 0x18 fields | no game call reaches the stream API; StreamMedia (voice) was not followed | read 0x00175190, 0x00175A18 and 0x0017C1E0 (voice) when voice is done |
| 4 DME | 0x19 / 0x1A LAN discovery: purpose | the query's sender on the broadcast socket was not found | search the rt_p2p table 0x001D7D00 users for the tag 0x8001 builder |
| 4 DME | 0x10 bytes +0/+2, 0x16 bytes +2..+4 and its per-state jump table 0x001B70D0, the use of 0x17's order, 0x04's obj+0x2C/+0x30, which kind bits 0x001706F8 adds to 0x03 | not read past the fields the handlers store | read 0x00150650, the 0x001B70D0 cases, 0x00171D18's choice of the next host, 0x001706F8 |
| 5 in game | ch 0 vectors, ch 3 vectors/seat/flags, ch 9's protected byte (health?), ch 10 flags and +0x1C/+0x1E, ch 5's score blocks and +0x0E/+0x10 | copied whole into player/net-player structs whose readers were not traced | trace the net-player fields (+0x255C, +0x1D0) and the game-mode struct 0x00352940 in the scoreboard code; matched C helps (L39_0066FA90) |
| 5 in game | ch 2 b0/b2 and its receiver; ch 12 payload | handlers are level-specific (L40 0x004EDEF0, 0x005349F0), not read | read those two handlers on level 40 |
| 5 in game | ch 16 per-mode f32 values (net+0x88 / +0x98, OBJECT_GAME_SETTINGS +0x110/+0x130) | only seen being copied | find the writer of net+0x88 (likely the stats/skill code) |
| 5 in game | ch 18 result 3 (reserved slots) | depends on a level hook at gp-0x6AB4 | read the hook installed in level 39 (clan games) |
| 5 in game | ch 19: "talking" = the voice stream handle 0x002416B8 is open | the value is 0x0019B910's test of that handle; the handle's writer in StreamMedia was not read | read the writers of $gp-0x6908 (0x002416B8) in the voice code 0x0019C2C0 / StreamMedia |
| 5 in game | why the port sends ch 19 every frame | the counter 0x00241704 is bumped only by the vblank handler 0x0019BC10 (C); if the port never runs that handler the counter stays 0 and `0 % 60 == 0` passes each frame | a port run logging `[0x00241704]`, or reading how the port's AddIntcHandler2 (common/lib/kernel.c) dispatches cause 2 |
| 5 objects | OBJECT_PLAYER_DATA +0x00 (4 × 16 span), +0x3C meaning, +0x44 name, +0x48, +0x4C, +0x50, +0x80..+0x84, camera-mode names; OBJECT_GAME_SETTINGS +0x17E, game-mode names, most of the 22 option bytes, +0x19F; the type codes' names | no reader found, or no strings | readers of net-player +4 → +0x20..+0x30; the map code's reads of OBJECT_PLAYER_DATA through `nwobj_get`; the option bytes' writer L39_006AC860 against the create screen's labels |

<a id="s6-horizon"></a>
### 6.1 Cross-check with the Horizon Private Server

[Horizon Private Server](https://github.com/Horizon-Private-Server/horizon-server) (MIT license; its
`RT.Common/Types.cs` and `Constants.cs`, the `RT.Models` message classes and `Server.Medius`
MAS/MLS/MPS, read on 2026-11-22) was used as research only: no code was copied, and every field it
names was looked up in the binary. "C, agrees with Horizon" means the binary was read as well;
"Horizon says" alone stays I.

**Agrees (C in the binary, same in Horizon):** all RT ids 0x00-0x1E and their names; the Medius
class/type of all 77 requests and 5 notices; the message classes (0 DME, 1 Lobby, 2 Application,
3 LobbyReport, 4 LobbyExt, 5 LobbyAuthentication: this build uses 0, 1, 2, 4); the sizes and layouts
of LOBBY_CITY_LIST_ENTRY (its own layout), LOBBY_GAME_LIST_ENTRY (incl. SecurityLevel/WorldStatus/HostType at
+0x40/+0x44/+0x48), LOBBY_GAME_INFO_ANSWER, LOBBY_GAME_CREATE_REQUEST, LOBBY_GAME_JOIN_REQUEST, LOBBY_LOGIN_ANSWER,
LOBBY_CHANNEL_FILTER_SET_REQUEST, LOBBY_CHANNEL_LIST_ENTRY (Horizon's extra GameWorldCount applies only to Medius > 1.08),
LOBBY_GAME_REPORT, LOBBY_PLAYER_STATS_REPORT, LOBBY_GAME_LEAVE_REPORT, LOBBY_LADDER_STATS_READ_REQUEST / LOBBY_LADDER_STATS_WRITE_REQUEST, LOBBY_GAME_LIST_FILTER_ADD_REQUEST,
LOBBY_CHAT_LINE and LOBBY_PLAYER_FIND_REQUEST type values, LOBBY_CHAT_TEXT_CHECK_REQUEST, LOBBY_PRESENCE_UPDATE actions,
NetConnectionInfo (0x9C); for Medius ≤ 1.08 the RT accept, aux-UDP hello, connect/disconnect
notices and echo; DME ROOM_FRAGMENT's header and ROOM_CLIENT_STATUS's states (Horizon's NetClientStatus).
**Architecture:** Horizon's MAS (sessions, login), MLS (channels, games), MPS (bridge to the room
server) and DME server match the binary's three lobby connects (authentication, lobby, channel)
and the DME connect. Horizon's universe list (GetUniverseInformation) is registered in this build
but never sent (C): the game takes its server from the hard-coded hostnames and its city from
LOBBY_CITY_LIST_REQUEST. The one UYA-specific rule found: Horizon pads *JoinGameResponse*'s connection info
for application ids 10683/10684 (`PadLegacyConnectionInfo`).

**Disagreements (the binary wins in this document):**

| # | Item | Binary (C) | Horizon |
|---|---|---|---|
| 1 | RT 0x00/0x15 connect body, first 5 bytes | 3 version bytes `01 08 01` (0x0016EA80), then u16 world id, then u32 app id | u32 TargetWorldId, one byte (Medius < 1.09), then app id: same length, different split |
| 2 | RT 0x1B | received (same handler as 0x13), never sent | named CLIENT_CRYPTKEY_PEER (a client message) |
| 3 | DME 0x02 fragment piece | 0x1EA = 490 bytes (0x0014CB98, checked) | at most 484 (DME_FRAGMENT_MAX_PAYLOAD_SIZE) |
| 4 | LOBBY_GAME_JOIN_ANSWER size | the library consumes 0xBC = 0x20 + 0x9C | writes 2 pad bytes more for UYA (4 for other games); the dispatcher would read them as a further record (I: harmless only if a short tail is dropped) |
| 5 | LOBBY_KEEP_ALIVE_ANSWER (*GetMyIPResponse*) | not read by the game; the port writes status at +0x18 | IP char[20] at +0x15, StatusCode at +0x2C |
| 6 | LOBBY_CHAT_TOGGLE_REQUEST (*ChatToggle*) | 0x2C bytes: SessionKey and 0/1 at +0x28 | models only the MessageID |
| 7 | LOBBY_GAME_CREATE_REQUEST +0x78 | one string copied at +0x78 (64-byte block) | GamePassword[32] + SpectatorPassword[32] (consistent for passwords ≤ 31 chars) |
| 8 | DME ids | 23 handlers; 0x03-0x0F, 0x12, 0x15, 0x17, 0x18 (net objects, session) | no names for those; lists LANTextMessage 0x21 and LANRawMessage 0x22, not registered in this build |
| 9 | DME 0x10 ROOM_PEER_HELLO | +2 is compared with this console's own value | names it the player's IPv4 address (fits the check; kept I) |

---

## 7. The port: what it did, and stage 2

### 7.1 Before (OpenRAC develop at d426aa129)

Measured in hidden runs on box1 (menu log), not only read:

| Step | What the port did | What the game showed |
| --- | --- | --- |
| MULTIPLAYER | I5BOOTN.ELF runs natively (`host/exec.c`, prefix `mp_`) | the menus, with "No network adaptor detected" |
| adaptor check | `sceSifSearchModuleByName("dev9")` (0x0012C220) went to the IOP's loadfile RPC, whose reply the port never writes | ONLINE PLAY unavailable |
| (adaptor found) front end's module load 0x00198248 | `sceSifMInitRpc` (0x00192460) waits for an IOP answer forever | the game froze on LOADING |
| network configuration (L39 0x0067D758) | ezNetCnf/ezNetCtl RPCs "succeed" with untouched buffers; the screen's clock (L39 0x0067CEA0, timer 0 through 0x0012DCB0) never advances in the port | "Checking for Network Configurations..." for ever |
| DNAS (L39 0x0067ABC8) | the DNAS library's RPC (0x80000F03) answers nothing | would stay at "authenticating" |
| net_lib_start 0x00198478 | lgAudInit (0x0018E628, the USB headset) halts in a loop with no IOP | frozen |
| SCE-RT, Medius, DME | rt_comm's sockets go through libnetb to an IOP that does not exist; MSIF bind loops | no lobby could start |

### 7.2 Stage 2: what routes where (OpenRAC `feat/rac3-online`, `port/game/rac3-pal/host/mp_net.c`)

See `port/game/rac3-pal/README.md`, "Online play", for the cut points. Server side (OpenRAC/Server,
master): `PACKET_RELAY` (packet 7, any member to any member or all) and `PARTY_ADVERT` (party opcode 13, the
host's opaque game settings, returned by `PARTY_LIST` with the adverts flag), both in the README.

How the Medius lobby maps onto the server:

| Medius (game) | Port's lobby (`mp_net.c`) | OpenRAC server |
| --- | --- | --- |
| MediusConnect (x3: auth, lobby, channel) | connected at once; the game's callback at the next pump | none |
| LOBBY_SESSION_OPEN_REQUEST / LOBBY_LOCALE_SET_REQUEST / LOBBY_LICENCE_TEXT_REQUEST / announcements / LOBBY_CHAT_TOGGLE_REQUEST / LOBBY_KEEP_ALIVE_REQUEST | answered locally (key `OPENRACSESSION01`, no texts) | none |
| LOBBY_LOGIN_REQUEST (name) | connects, says hello with the account name; replies when the session id comes | `PACKET_HELLO` |
| LOBBY_CHANNEL_LIST_REQUEST / LOBBY_CHANNEL_JOIN_REQUEST | one numbered channel, "OpenRAC CH 01" | none |
| LOBBY_GAME_LIST_REQUEST | the parties of this disc's family that carry an advert, one record each | `PARTY_LIST` with adverts (family `rac3-pal-1`) |
| LOBBY_GAME_INFO_REQUEST | from the last list, or this console's own game (its latest LOBBY_GAME_REPORT) | none |
| LOBBY_GAME_CREATE_REQUEST | hosts a party named after the game; its settings become the advert; world id 900 | `PARTY_CREATE`, `PARTY_ADVERT` |
| LOBBY_GAME_REPORT | updates the advert when it changed | `PARTY_ADVERT` |
| LOBBY_GAME_JOIN_REQUEST | own world: at once; another: joins that party, then replies with a connection info naming the world | `PARTY_JOIN` |
| LOBBY_GAME_PLAYER_LIST_REQUEST | the party's members | none |
| LOBBY_PRESENCE_UPDATE(LeftGameWorld) | leaves the party | `PARTY_LEAVE` |
| clans, buddies, ladders, stats | empty lists / success | none |

How DME maps: the DME connection's client indices are the party order (host 0, members as they
joined, the same on every console); dme_join_session calls the game's join, client and host
callbacks; every DME message (`0x0014AD08` app messages, `0x0014CAB8` / `0x0014C950` all others)
goes out as `PACKET_RELAY` with a one-byte kind (1) before the DME packet `[class][id][payload]`, to the
member of the destination index or to every other member; received ones go to `0x0014CE48`.

Further cuts the runs needed (all in `mp_net.c`, rows in `mp/libraries.tsv`):

| Function | Why |
| --- | --- |
| `0x00198248` the front end's network module load | only looks for the adaptor; nothing is loaded on an IOP |
| `0x00192460` / `0x00192790` sceSifMInitRpc / sceSifMBindRpc | no IOP to answer; the client is marked bound |
| L39 `0x0067CEA0` the lobby screens' millisecond clock | timer 0 (through `0x0012DCB0`) does not run in the port; the host's clock |
| `0x0018E628` lgAudInit | the USB headset library loops on its RPC; no headset |
| L39 `0x0067ABC8` the DNAS screen | its resolver thread never runs (the screen never blocks); the addresses are written in its state 10 |
| `0x00198608` / `0x00198660` DME's malloc/free hooks | newlib's heap fails DME's registry allocations; a 0xC4000-byte heap in the port's statics |
| `0x00195410` the lobby's pump | its hook runs MediusUpdate only on some screens; the lobby's replies are delivered here too |
| `0x00195010` the profiles' reset, L39 `0x0068C0E0` the PROFILES screen | `OPENRAC_RAC3_PROFILE=Name[:password]` gives a scripted run (empty card, no on-screen keyboard) a profile and logs in with it once |
| `0x00125F50` AddIntcHandler2 (common/lib/kernel.c) | the network's vblank timer `0x0019BC10` reads its handler argument; the port called it without one and it faulted |
| hostgen roots (113 core, 103 level 39) | SCE-RT's, DME's and the lobby's function tables are filled at run time, so hostgen never saw those functions; every one formed in code at a function boundary is now a root |

### 7.3 Status (2026-11-22, two hidden PAL clients on box1, OpenRAC server on box1:7790)

Reached, verified in the menu logs and the server log:

1. MULTIPLAYER > ONLINE PLAY: network adaptor found, NETWORK STARTUP lists the "OpenRAC server"
   configuration, the interface comes up, DNAS passes, the server's address is "resolved".
2. PROFILES > LOGIN: MediusConnect, LOBBY_SESSION_OPEN_REQUEST, LOBBY_LOCALE_SET_REQUEST, LOBBY_LOGIN_REQUEST: the port
   says `PACKET_HELLO` to the server under the profile's name ("Session 21 (Ratchet, version
   rac3-online-1)"); "Login succeeded"; city select, licence (LOBBY_LICENCE_TEXT_REQUEST), announcements.
3. The ONLINE PLAY lobby (QUICK PLAY, CREATE GAME, FIND GAME, CITIES, BUDDIES, CLAN, STATS).
4. Client A, CREATE GAME (Bakisi Isles, Siege, 8 players): LOBBY_GAME_CREATE_REQUEST hosts party `J5877H`
   (family `rac3-pal-1`) with its settings as the advert, LOBBY_GAME_JOIN_REQUEST of its own world, the DME room
   joined as client 0 (host 0), LOBBY_GAME_REPORT and the LOBBY_GAME_INFO_REQUEST poll agree: the STAGING room shows.
5. Client B, QUICK PLAY: the game list (the server's `PARTY_LIST` with adverts) gives A's game, GAME INFO
   shows "Bakisi Isles / Siege", JOIN: the server logs "Session 22 (Clank) joined party J5877H
   (2/4)"; B's DME room is joined as client 1 (host 0) and told client 0 is there; A is told
   "Clank joined the party" and "room client 1 joined". B then shows "Joining Game..." and sends
   DME app id 19 to the host every frame through `PACKET_RELAY` (that id is the talking flag 0x00240644,
   not the join request, which is id 16: see 7.6).

So two local clients reach the same game room through a locally run server.

Since then (runs s2, s3, f2 on box1, 2026-11-22 night, OpenRAC `feat/rac3-online` 3ac9f2842):

6. The joiner sends the room join request (ch 16, `CHANNEL_ROOM_JOIN_REQUEST`, dest -1 = every member); the
   host answers (ch 18, `CHANNEL_ROOM_JOIN_ANSWER`), fills OBJECT_GAME_SETTINGS and sends it whole: both STAGING rooms
   show PLAYER 2/8 with both names. The port had dropped dest -1 as "the DME server".
7. READY (Circle) on both: `CHANNEL_SLOT_UPDATE` (ch 17) and OBJECT_GAME_SETTINGS changes flow; the host's START
   shows "Starting...1" and releases OBJECT_GAME_SETTINGS (owner -1 on the joiner). Both consoles then go
   to the map load, which stops at `func_00126D00` (no C yet): the maps work's part.
8. The talking flag (ch 19) now goes every 60th vblank (0x00241704 follows the host clock: the
   game's vblank handler 0x0019BC10 is installed with the voice code, which the port never starts).
9. FIND GAME lists the server's games (SEARCH with every option at Any); the city only filters
   when one is picked (GenericField1 == the city id, 40 + region index).

Not yet:

1. The map load after START (maps agent), then the in-game channels (section 4.5) through the
   relay.
2. A crashed or hung client keeps its party until its TCP connection drops (seen: a hung pair
   kept party LW36GH listed); the server could expire members that stop sending.
3. NTSC-U was not run; voice, clans, buddies, ladders and stats are answered empty.
4. A profile without `OPENRAC_RAC3_PROFILE` needs the multiplayer save on the card (net.bin).

How to run it: build `openrac-rac3-pal` from `feat/rac3-online`; start the server
(`./gradlew :server:run`, or the jar with `--port 7790`); run two clients with
`OPENRAC_SERVER=127.0.0.1:7790 OPENRAC_RAC3_PROFILE=<name>` (and `OPENRAC_RAC3_NET_LOG=1` for the
trace), each with its own `--cards` folder; the scripted presses used are in
box1 `~/RACSwarm/rac3-online/pair.sh` / the runs' command lines (MULTIPLAYER, ONLINE PLAY, the
configuration, city, licence (Square), announcements, then CREATE GAME + Circle on A and QUICK
PLAY, Any, JOIN on B).

### 7.4 Player-count limits in the game's net code (for rooms larger than 8)

The server's rooms go up to 24 players (`FAMILY_PARTY_SIZES`, default 8 for the rac3 families) and
the port's relay and slot tables hold 24 (`ROOM_MAX`); the port tells the game only of slots 0-7
(`GAME_CLIENTS`), because the unmodified game's tables are sized for 8 clients. Every fixed limit
found, PAL addresses (V read in the code, I inferred), so the client can be widened later:

| Where | Address | Size | What it bounds |
| --- | --- | --- | --- |
| DME connection table | 0x001CD4C8 | 4 slots | connections (lobby, game world); not players |
| DME per-client records | `0x0014F8E8(conn, i)` in the DmeConn | by +0x43C (client count) | DME's own session layer (the port bypasses it) |
| DME message ids | 0x0014AB38 | id < 0x100, class < 6 | app channels, not players |
| net block: client by slot | net + 0x148 (0x001A5E18) | 8 words (V) | the session's clients in join order |
| net block: slot by client | net + 0x6AD (0x001A637D) | byte per client index (I: 8) | maps a DME client index to a slot |
| net block: last receive time per slot | net + 0x68C (0x001A635C) | 8 words (V) | host silence, UDP health |
| net block: last player-state time per slot | net + 0x7B0 (0x001A6480) | 8 words (V) | UDP health (0x001959F8) |
| net block: per-player names | net + 0x7DD (0x001A64AD) | 8 x 0x10 (V, 0x00198478 clears 8) | names in the session |
| net block: per-player bytes | net + 0x7D5 (0x001A64A5) | 8 (V) | cleared per session |
| net block: per-player words | net + 0x640 (0x001A6310) | 8 words (V, set to -1 by 0x00198478) | slot requests (host side, 0x0019A4D0) |
| net-player table | 0x0022F3D0 | 8 x 0xC70 (V) | each remote player's state, jitter buffer (16 x 0xA4 at +0x1F0) |
| offline OBJECT_PLAYER_DATA copies | 0x002357A0 | 8 x 0x90 (V) | local / split-screen players |
| OBJECT_GAME_SETTINGS | (object, 0x1A4 bytes) | arrays of 8 (V): names 16 x 8 at +0x000, clan tags 8 x 8 at +0x080, skins/teams/client/state 2 x 8 at +0x0C0..+0x0F0, floats 4 x 8 at +0x110/+0x130, account ids 4 x 8 at +0x150 | the staging room's slots, teams and names |
| OBJECT_GAME_SETTINGS talking mask | +0x19E | 1 byte, bit per client (V) | 8 clients |
| per-player lobby values | 0x001A58D0 / 0x001A5930 / 0x001A5990 | stride 0x20, read by player index (I: 8 or fewer) | loadout values copied into OBJECT_PLAYER_DATA |
| player UID | 0x00197498 | client index in bits 28-31 (V) | up to 16 clients |
| match state, channel 5 | payload +0x12 | 8 per-player bytes (V) | the host's per-player state and the end/score block (command 0x82: 0x20 + 0x10 + 3 x 8 bytes) |
| player-state stream, channel 9 | bits f60 | 4 bits: player index (V) | up to 16 |
| knockback, channel 15 / kills, channel 4 | target or victim byte | player index, byte (V) | 256 |
| create-game settings | option 1 (MaxPlayers) | whole word (V); the CREATE GAME page offers up to 8 (I) | the game's own maximum |
| net-object types | 0x001741A8 | 0x40 fields a type, 0x400 in all, 475 bytes of data a type, 16 types (V) | the OBJECT_GAME_SETTINGS layout above cannot grow past 475 bytes without a new type |
| staging and scoreboard pages | level 39 screens (0x006C1730 staging; the end-of-game board in the maps) | 8 rows drawn (I, the pages list PLAYER/SKILL/TEAM/SKIN/READY for 8) | the UI; needs scrolling for more |

Widening past 8 means: the net block arrays (or moving them), the net-player table, OBJECT_GAME_SETTINGS
(a second object type for players 9-24), the talking mask, channel 5's per-player bytes, and the
staging and scoreboard pages. The relay, the slots and the port's object store need no change.

### 7.5 Hub and regions: where the game's services land

The retail service is split the same way the OpenRAC server now is (Server README, "Hub and
regions"): an authentication server, then a choice of lobby servers, then the game rooms of the
chosen one.

| Game step (retail) | Retail server | OpenRAC |
| --- | --- | --- |
| MediusConnect #1, LOBBY_SESSION_OPEN_REQUEST, LOBBY_LOGIN_REQUEST | MAS (authentication) | the **hub** (`OPENRAC_SERVER`): `PACKET_HELLO` with the profile's name |
| LOBBY_CITY_LIST_REQUEST (1/0x8C) -> CITY SELECT | MAS/MLS: the location list | the hub's `PACKET_REGIONS`: one location per region (id 1..n, the region's name); a server without the `FEATURE_REGIONS` feature bit gets NoResult and the game shows its own five cities |
| LOBBY_CHANNEL_FILTER_SET_REQUEST (4/0x12), LOBBY_CHANNEL_LIST_REQUEST (4/0x15), LOBBY_CHANNEL_JOIN_REQUEST (1/0x25) | MLS of the city: its channels, then a connect to the channel's lobby | the chosen **region**: in single-process mode (`MODE=all`) the hub's own region, on the same connection; a separate region is a second connection to its host:port (next step below) |
| LOBBY_GAME_LIST_REQUEST, LOBBY_GAME_INFO_REQUEST, LOBBY_GAME_CREATE_REQUEST, LOBBY_GAME_JOIN_REQUEST, LOBBY_GAME_REPORT | MLS | the region's parties and adverts |
| DME connect / join, in-game traffic | DME server of the world | the region's party: `PACKET_RELAY`, `PARTY_SLOTS`, the port's object store |

Next step for separate regions: the port logs LOBBY_CHANNEL_FILTER_SET_REQUEST's bytes (`OPENRAC_RAC3_NET_LOG`)
to find where the chosen location id travels; LOBBY_CHANNEL_JOIN_REQUEST then opens the second connection to that
region (`PACKET_HELLO` again under the same name) and the lobby's party requests go there, the hub
connection staying for the session.

### 7.6 Protocol findings that affect the port (static verification, 2026-11-22)

Found while verifying layers 3-6 above; nothing here was run. Each is C (read in the code).

| # | Finding | Where | What the port should do |
|---|---|---|---|
| 1 | The room join request is DME app id **16** (0x00240648), sent once by L39_006D34D8; app id **19** is the talking flag (0x00240644), sent to the host when `[0x00241704] % 60 == 0`; that word is the vblank counter of the handler 0x0019BC10 (every 1.2 s on a console). Every frame in the port: most likely the port never runs that VBLANK_S handler, so the counter stays 0 (I) | layer 5 | read 7.3's "join request (DME app id 19)" as the talking flag; look at id 16 for the join; run 0x0019BC10 once per vblank (or per frame) |
| 2 | `0x0016EF40` is **nwobj_send** (the whole object, DME 0x04), not "set target". The game calls it after creating OBJECT_GAME_SETTINGS, after accepting a join, at START, and after creating OBJECT_PLAYER_DATA | layer 4 0x04, layer 5 objects | relay the object (kind 2) in the port's 0x0016EF40 to the target (-1 = all) |
| 3 | DME's object tick (dme_update → 0x00171B78) sends **changed fields** (0x03) every frame; the port replaces dme_update, so in-place edits (slot states, ready, team, start times, the talking mask, OBJECT_PLAYER_DATA +0x86) never leave the console | layer 4 0x03 | diff each owned object against its last relayed copy in the port's dme_update and relay the changed object (whole is fine) |
| 4 | `0x0016ED68` sets the console's player object in its client record (DME 0x0F); it sends no object data | layer 4 0x0F | keep relaying at nwobj_send instead (item 2) |
| 5 | `0x0016EDD0` **releases ownership** (DME 0x0D: owner := -1, the object stays). The port sends kind 3 and calls on_destroy, so at START (0x00199A20 releases OBJECT_GAME_SETTINGS) the others would lose OBJECT_GAME_SETTINGS | layer 4 0x0D | send an ownership change instead (owner -1) and call on_update(-1, id, -2) |
| 6 | The object callbacks take the **creator's client index** first, and on_update's field argument is -1 for a whole object; -2 means "ownership only" and both game callbacks return at once on it. The port calls on_create(0, id) and on_update(0, id, -2), so a joiner never copies OBJECT_GAME_SETTINGS +0x178/+0x17E into net+0x130/net+0x638 | layer 4 0x04 | on_create(creator, id), on_update(creator, id, -1) |
| 7 | The object record's +0x08 is the **creator's client index** (on_create 0x00199A60 copies it to the net-player's +0); the port writes the object id there | layer 5 objects | +0x08 = the owner's client index |
| 8 | LOBBY_CITY_LIST_ENTRY ids must be **40..99** (L39_006D7138 drops others and falls back to its five cities); the port numbers regions 1..n | layer 3 LOBBY_CITY_LIST_REQUEST | ids 40 + region index; the chosen city then comes back as bit (id-40) in LOBBY_CHANNEL_FILTER_SET_REQUEST's FilterMask2/3, which answers 7.5's "where the location id travels" |
| 9 | At START the voice stop 0x0019BF20 (from L39_006C4448) waits in `while ([0x002416E4])` for the vblank timer's alarm chain (handler 0x0019BCD0, re-armed through the kernel library's timer alarm 0x0012E3E8) to count down; the port has no timer interrupt, so the game spun there (V, gdb on box1) | in game | wrap 0x0012E3E8: run the handler at once |
| 10 | DME delivers a message to the sender's own index locally: the host's own `CHANNEL_SLOT_UPDATE` (state 8 in slot 0 after its map load) opens the start barrier (V) | layers 4/5 | deliver dest == own index into the room inbox |
| 11 | After START GameSettings is released (owner -1) and still changes (+0x170 start time, slot states): its changes go out from whoever makes them (V) | layer 4 0x03 | send the changes of owned and of released objects |
| 12 | The map's start barrier compares the room clock (net+0x128, copied each frame from dme_get_time 0x0014B008) with GameSettings +0x170; the port's DME clock read 0, so it never opened (V) | layer 4 | dme_get_time = the OpenRAC server's clock from `PACKET_TIME`, the same on every console |
