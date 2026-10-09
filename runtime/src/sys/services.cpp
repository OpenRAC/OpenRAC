// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the runtime does in place of library functions that, on the console,
// ask the second processor for something: the disc, the clock, the console's
// settings. Each is the documented behaviour of a public function as a
// program sees it, written from that behaviour, not from any SDK source.

#include <ctime>

#include "machine.h"

namespace sys {

using namespace ps2;

namespace {

u8 bcd(int v) {
  return static_cast<u8>(((v / 10) << 4) | (v % 10));
}

}  // namespace

void add_library_services(Machine& machine) {
  // --- the disc drive: every request completes at once ---
  machine.add_service("sceCdInit", [](Machine& m) { m.result(1); });
  machine.add_service("sceCdDiskReady", [](Machine& m) { m.result(2); });  // "complete"
  machine.add_service("sceCdMmode", [](Machine& m) { m.result(1); });
  machine.add_service("sceCdSync", [](Machine& m) { m.result(0); });       // nothing in progress
  machine.add_service("sceCdGetError", [](Machine& m) { m.result(0); });
  machine.add_service("sceCdBreak", [](Machine& m) { m.result(1); });
  // sceCdRead(first sector, count, buffer, mode)
  machine.add_service("sceCdRead", [](Machine& m) {
    u32 sector = m.arg(0), count = m.arg(1), buffer = m.arg(2);
    u8* to = m.ee.pointer(buffer);
    bool fits = to && m.ee.pointer(buffer + count * 2048 - 1) == to + count * 2048 - 1;
    if (!fits || !m.disc.read(sector, count, to)) {
      m.log(0, "sceCdRead: cannot read %u sectors at %u to %08x", count, sector, buffer);
      m.result(0);
      return;
    }
    m.log(2, "sceCdRead: %u sectors at %u to %08x", count, sector, buffer);
    m.result(1);
  });
  // sceCdReadClock(clock): eight bytes, binary coded decimal, Japan's time on
  // the console; the host's local time here.
  machine.add_service("sceCdReadClock", [](Machine& m) {
    std::time_t now = std::time(nullptr);
    std::tm t{};
    localtime_r(&now, &t);
    const u8 clock[8] = {0, bcd(t.tm_sec), bcd(t.tm_min), bcd(t.tm_hour), 0, bcd(t.tm_mday), bcd(t.tm_mon + 1),
                         bcd(t.tm_year % 100)};
    for (u32 n = 0; n < 8; n++) {
      m.ee.write8(m.arg(0) + n, clock[n]);
    }
    m.result(1);
  });

  // sceCdCallback(function): returns the one it replaces.
  machine.add_service("sceCdCallback", [](Machine& m) {
    m.result(m.cd_callback);
    m.cd_callback = m.arg(0);
  });
  // A read that announces its end through that callback (sceCdRead used
  // this way, and the sound library's own wrapper around it).
  machine.add_service("sceCdRead.callback", [](Machine& m) {
    u32 sector = m.arg(0), count = m.arg(1), buffer = m.arg(2);
    u8* to = m.ee.pointer(buffer);
    bool fits = to && m.ee.pointer(buffer + count * 2048 - 1) == to + count * 2048 - 1;
    if (!fits || !m.disc.read(sector, count, to)) {
      m.log(0, "disc read: cannot read %u sectors at %u to %08x", count, sector, buffer);
      m.result(0);
      return;
    }
    m.log(2, "disc read: %u sectors at %u to %08x", count, sector, buffer);
    m.cd_read_finished();
    m.result(1);
  });

  // --- the second processor's remote calls ---
  // sceSifBindRpc(client, server, mode): the server is found at once. A
  // program waits for the client record's server pointer (its tenth word).
  machine.add_service("sceSifBindRpc", [](Machine& m) {
    m.rpc_servers[m.arg(0)] = m.arg(1);
    m.ee.write32(m.arg(0) + 0x24, 1);
    m.result(0);
  });
  machine.add_service("sceSifCheckStatRpc", [](Machine& m) { m.result(0); });  // never busy
  // sceSifCallRpc as the 989snd sound library uses it, answered by a sound
  // server that makes no sound yet. The library has two clients. The loader's
  // calls return one word, the handle of what was loaded. The command
  // client's calls return a word of all ones, one result word a command, and
  // all ones again; function 0x4D carries a batch (a count, then for each
  // command its number and size as half-words and its data, padded to a
  // word), any other function is one command with its data.
  // (client, function, mode, send, send size, receive, receive size, end function, end argument)
  machine.add_service("sceSifCallRpc.989snd", [](Machine& m) {
    u32 function = m.arg(1), send = m.arg(3);
    u32 receive = static_cast<u32>(m.ee.gpr[9].lo), size = static_cast<u32>(m.ee.gpr[10].lo);
    u32 end_function = static_cast<u32>(m.ee.gpr[11].lo);
    u32 end_argument = m.ee.read32(static_cast<u32>(m.ee.gpr[29].lo));

    // Server 0x11 is the games' own "stash": a stretch of the second
    // processor's memory where a program parks data (sent there with
    // SifSetDma) to fetch it back later. Function 2 tells where the stretch
    // is and how long; function 1 copies from an address in it (the word
    // sent) into the receive buffer.
    auto server = m.rpc_servers.find(m.arg(0));
    if (server != m.rpc_servers.end() && server->second == 0x11) {
      constexpr u32 kStashBase = 0x00080000, kStashBytes = 0x00170000;
      if (function == 2 && size >= 8) {
        m.ee.write32(receive, kStashBase);
        m.ee.write32(receive + 4, kStashBytes);
      } else if (function == 1) {
        u32 from = m.ee.read32(send);
        for (u32 n = 0; n < size; n++) {
          m.ee.write8(receive + n, m.iop_memory[(from + n) & (Machine::kIopBytes - 1)]);
        }
      }
      if (end_function) {
        m.ee.call(end_function, end_argument);
      }
      m.result(0);
      return;
    }

    // What a silent server answers. Streams and sounds get handles and are
    // "buffered" at once; nothing is ever still playing.
    // What the sound server answers. The streams (music, speech) are
    // played; sounds from the banks get a handle and are over at once.
    auto command = [&m](u32 number, u32 data) -> u32 {
      switch (number) {
        case 0x11: case 0x21:  // play a sound
          return m.sound_next_handle++;
        case 0x09:  // the volume of a group of sounds (group, volume)
          m.sound.set_group_volume(m.ee.read32(data), static_cast<int>(m.ee.read32(data + 4)));
          return 0;
        case 0x2C: {
          // Play a stream from the disc: (sector, a second sector, volume
          // above an offset, pan above an offset, group, the stream to
          // follow, flags). Flag 4: repeat. The answer is the handle, or
          // that of the stream it follows.
          u32 sector = m.ee.read32(data), after = m.ee.read32(data + 20), flags = m.ee.read32(data + 24);
          int volume = static_cast<int>(m.ee.read32(data + 8) >> 16);
          u32 group = m.ee.read32(data + 16);
          if (after && m.sound.playing(after)) {
            return m.sound.play(after, sector, volume, group, (flags & 4) != 0, after) ? after : 0;
          }
          u32 handle = m.sound_next_handle++;
          return m.sound.play(handle, sector, volume, group, (flags & 4) != 0, 0) ? handle : 0;
        }
        case 0x2D:  // pause a stream
          m.sound.pause(m.ee.read32(data), true);
          return 0;
        case 0x2E:  // and go on
          m.sound.pause(m.ee.read32(data), false);
          return 0;
        case 0x15:  // stop a sound, or a stream
          m.sound.stop(m.ee.read32(data));
          return 0;
        case 0x34:  // stop every stream
          m.sound.stop_all();
          return 0;
        case 0x4F:  // is the stream buffered?
          return 1;
        case 0x19: {  // is it still playing? Its handle if so.
          u32 handle = m.ee.read32(data);
          return m.sound.playing(handle) ? handle : 0;
        }
        case 0x32:  // time left in the stream
          return m.sound.remaining(m.ee.read32(data));
        default:
          return 0;
      }
    };

    if (size == 4) {
      if (m.verbose >= 2) {
        std::string words;
        for (u32 w = 0; w < m.arg(4) && w < 40; w += 4) {
          char text[12];
          std::snprintf(text, sizeof(text), " %08x", m.ee.read32(send + w));
          words += text;
        }
        m.log(2, "sound loader call %x (%u bytes):%s", function, m.arg(4), words.c_str());
      }
      m.ee.write32(receive, m.sound_next_handle++ << 16);
    } else if (size >= 8) {
      m.ee.write32(receive, 0xFFFFFFFFu);
      u32 results = size / 4 - 2;
      if (function == 0x4D) {
        u32 count = m.ee.read32(send), at = send + 4;
        for (u32 n = 0; n < count && n < results; n++) {
          u32 number = m.ee.read16(at), bytes = m.ee.read16(at + 2);
          u32 answer = command(number, at + 4);
          if (m.verbose >= 2) {
            std::string words;
            for (u32 w = 0; w < bytes && w < 40; w += 4) {
              char text[12];
              std::snprintf(text, sizeof(text), " %08x", m.ee.read32(at + 4 + w));
              words += text;
            }
            m.log(2, "sound command %02x (%u bytes):%s -> %x", number, bytes, words.c_str(), answer);
          }
          m.ee.write32(receive + 4 + n * 4, answer);
          at += (4 + bytes + 3) & ~3u;
        }
      } else {
        u32 answer = command(function, send);
        m.log(2, "sound command %02x, waited for -> %x", function, answer);
        for (u32 n = 0; n < results; n++) {
          m.ee.write32(receive + 4 + n * 4, answer);
        }
      }
      m.ee.write32(receive + 4 + results * 4, 0xFFFFFFFFu);
    }
    if (end_function) {
      m.ee.call(end_function, end_argument);
    }
    m.result(0);
  });

  // --- the memory card ---
  add_memory_card_services(machine);

  // --- the pad ---
  machine.add_service("scePad2GetState", [](Machine& m) { m.result(1); });  // connected and stable
  // scePad2GetButtonProfile(socket, profile): which buttons exist, four bytes
  // of ones for a pad with every button and both sticks.
  machine.add_service("scePad2GetButtonProfile", [](Machine& m) {
    m.ee.write32(m.arg(1), 0xFFFFFFFFu);
    m.result(4);
  });
  // sceVibGetProfile(socket, profile): which motors exist.
  machine.add_service("sceVibGetProfile", [](Machine& m) {
    m.ee.write32(m.arg(1), 3);
    m.result(1);
  });
  // scePad2Read(socket, data): two bytes of buttons (a clear bit is a
  // pressed button), the right stick, the left stick, twelve pressures.
  machine.add_service("scePad2Read", [](Machine& m) {
    const Machine::Pad& pad = m.pad;
    u32 to = m.arg(1);
    m.ee.write8(to + 0, static_cast<u8>(~pad.buttons));
    m.ee.write8(to + 1, static_cast<u8>(~(pad.buttons >> 8)));
    m.ee.write8(to + 2, pad.right_x);
    m.ee.write8(to + 3, pad.right_y);
    m.ee.write8(to + 4, pad.left_x);
    m.ee.write8(to + 5, pad.left_y);
    // Pressures in the order right, left, up, down, triangle, circle, cross,
    // square, L1, R1, L2, R2: full when pressed.
    static constexpr unsigned bit[12] = {5, 7, 4, 6, 12, 13, 14, 15, 10, 11, 8, 9};
    for (u32 n = 0; n < 12; n++) {
      m.ee.write8(to + 6 + n, (pad.buttons >> bit[n]) & 1 ? 0xFF : 0);
    }
    m.result(18);
  });

  // --- the display ---
  // sceGsSyncV(mode): wait for the next vertical blank; which field follows.
  machine.add_service("sceGsSyncV", [](Machine& m) {
    m.skip_to_vblank();
    m.result(m.odd_field() ? 0 : 1);
  });

  // --- the console ---
  machine.add_service("sceScfGetLanguage", [](Machine& m) { m.result(1); });  // English
  // A printf-like function of the game: show what it says.
  machine.add_service("printf", [](Machine& m) {
    std::string text = m.format(m.arg(0), 1);
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
      text.pop_back();
    }
    m.log(0, "[game] %s", text.c_str());
    m.result(0);
  });
}

}  // namespace sys
