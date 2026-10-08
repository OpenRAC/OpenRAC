// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "dma.h"

namespace ps2 {

DmaStop run_source_chain(GuestMemory& memory, DmaChannel& channel, const DmaSink& sink) {
  const bool transfer_tag = (channel.chcr & 0x40) != 0;
  const bool tag_interrupt = (channel.chcr & 0x80) != 0;
  u32 depth = (channel.chcr >> 4) & 3;

  auto stop = [&](DmaStop why) {
    channel.chcr = (channel.chcr & ~0x130u) | (depth << 4);  // STR off, ASP updated
    return why;
  };

  // Data left over from a transfer that was started with a count.
  if (channel.qwc) {
    sink(memory.dma(channel.madr), std::size_t{channel.qwc} * 16);
    channel.madr += channel.qwc * 16;
    channel.qwc = 0;
  }

  for (u32 tags = 0; tags < (1u << 22); tags++) {
    const u8* tag = memory.dma(channel.tadr);
    u64 lo = load<u64>(tag);
    u32 qwc = static_cast<u32>(lo & 0xFFFF);
    u32 id = static_cast<u32>(bits(lo, 28, 3));
    bool irq = bits(lo, 31, 1) != 0;
    u32 address = static_cast<u32>(lo >> 32);

    channel.chcr = (channel.chcr & 0xFFFFu) | (static_cast<u32>(lo) & 0xFFFF0000u);
    if (transfer_tag) {
      sink(tag + 8, 8);
    }

    u32 after_tag = channel.tadr + 16;
    u32 from = after_tag;
    bool end = false;
    switch (id) {
      case dmatag::REFE:
        from = address;
        end = true;
        break;
      case dmatag::CNT:
        channel.tadr = after_tag + qwc * 16;
        break;
      case dmatag::NEXT:
        channel.tadr = address;
        break;
      case dmatag::REF:
      case dmatag::REFS:
        from = address;
        channel.tadr = after_tag;
        break;
      case dmatag::CALL:
        if (depth < 2) {
          channel.asr[depth++] = after_tag + qwc * 16;
        }
        channel.tadr = address;
        break;
      case dmatag::RET:
        if (depth > 0) {
          channel.tadr = channel.asr[--depth];
        } else {
          end = true;
        }
        break;
      default:
        end = true;
        break;
    }

    channel.madr = from;
    if (qwc) {
      sink(memory.dma(from), std::size_t{qwc} * 16);
      channel.madr += qwc * 16;
    }

    if (end) {
      return stop(DmaStop::End);
    }
    if (irq && tag_interrupt) {
      return stop(DmaStop::Interrupt);
    }
  }
  return stop(DmaStop::Runaway);
}

}  // namespace ps2
