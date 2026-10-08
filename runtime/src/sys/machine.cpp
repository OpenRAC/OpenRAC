// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "machine.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace sys {

using namespace ps2;

namespace {

// The field of SYSCALL that marks one put at a replaced function's entry.
constexpr u32 kHookCode = 0x80000;

constexpr u32 kDmaBase[10] = {0x10008000, 0x10009000, 0x1000A000, 0x1000B000, 0x1000B400,
                              0x1000C000, 0x1000C400, 0x1000C800, 0x1000D000, 0x1000D400};
enum : unsigned { kVif0 = 0, kVif1 = 1, kGifChannel = 2, kFromSpr = 8, kToSpr = 9 };

}  // namespace

Machine::Machine() {
  ee.on_read = [this](u32 address, unsigned bytes) { return hw_read(address, bytes); };
  ee.on_write = [this](u32 address, u64 value, unsigned bytes) { hw_write(address, value, bytes); };
  ee.on_write128 = [this](u32 address, u64 lo, u64 hi) { hw_write128(address, lo, hi); };
  ee.on_syscall = [this](u32 code) { syscall(code); };
  ee.on_event = [this] { event(); };
  add_default_services();
}

void Machine::log(int level, const char* format, ...) {
  if (level > verbose) {
    return;
  }
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
  std::fputc('\n', stderr);
}

void Machine::note(const std::string& what) {
  if (notes[what]++ == 0) {
    log(1, "not modelled: %s (pc %08x)", what.c_str(), ee.pc - 4);
  }
}

std::string Machine::string_at(u32 address, std::size_t limit) {
  std::string out;
  while (out.size() < limit) {
    char c = static_cast<char>(ee.read8(address++));
    if (!c) {
      break;
    }
    out.push_back(c);
  }
  return out;
}

// A printf format with its arguments taken from the argument registers
// (integers a0-a3, t0-t3). Enough for the messages games print.
std::string Machine::format(u32 format_address, unsigned first_arg) {
  std::string f = string_at(format_address), out;
  unsigned next = first_arg;
  auto take = [&]() -> u64 { return next < 8 ? ee.gpr[4 + next++].lo : 0; };
  for (std::size_t n = 0; n < f.size(); n++) {
    if (f[n] != '%') {
      out.push_back(f[n]);
      continue;
    }
    std::string spec = "%";
    n++;
    while (n < f.size() && std::strchr("-+ #0123456789.l", f[n])) {
      if (f[n] != 'l') {
        spec.push_back(f[n]);
      }
      n++;
    }
    if (n >= f.size()) {
      break;
    }
    char buffer[128];
    switch (f[n]) {
      case '%':
        out.push_back('%');
        break;
      case 's':
        out += string_at(static_cast<u32>(take()));
        break;
      case 'c':
        out.push_back(static_cast<char>(take()));
        break;
      case 'd': case 'i': case 'u': case 'x': case 'X': case 'p':
        spec.push_back(f[n] == 'p' ? 'x' : f[n]);
        std::snprintf(buffer, sizeof(buffer), spec.c_str(), static_cast<unsigned>(take()));
        out += buffer;
        break;
      case 'f': case 'g': case 'e':
        take();
        out += "<float>";
        break;
      default:
        out += spec;
        out.push_back(f[n]);
        break;
    }
  }
  return out;
}

// --- replaced functions ----------------------------------------------------------

bool Machine::hook(u32 address, const std::string& service) {
  auto found = services_.find(service);
  if (found == services_.end()) {
    return false;
  }
  u32 index = static_cast<u32>(hooked_.size());
  hooked_.push_back(found->second);
  hooked_names_.push_back(service);
  if (!hooked_original_.count(address)) {
    hooked_original_[address] = ee.read32(address);
  }
  // SYSCALL with our mark and the service's number in its code field.
  ee.write32(address, ((kHookCode | index) << 6) | 0x0C);
  return true;
}

bool Machine::load_hooks(const std::string& path, std::string* error) {
  std::ifstream in(path);
  if (!in) {
    if (error) *error = "cannot read " + path;
    return false;
  }
  std::string line;
  int number = 0;
  while (std::getline(in, line)) {
    number++;
    std::size_t hash = line.find('#');
    if (hash != std::string::npos) {
      line.erase(hash);
    }
    std::istringstream words(line);
    std::string address, service;
    if (!(words >> address >> service)) {
      continue;
    }
    if (!hook(static_cast<u32>(std::stoul(address, nullptr, 16)), service)) {
      if (error) *error = path + ":" + std::to_string(number) + ": no service named " + service;
      return false;
    }
  }
  return true;
}

void Machine::syscall(u32 code) {
  if (code & kHookCode) {
    u32 index = code & (kHookCode - 1);
    if (index < hooked_.size()) {
      log(3, "%s(%x, %x, %x, %x) from %08x", hooked_names_[index].c_str(), arg(0), arg(1), arg(2), arg(3),
          static_cast<u32>(ee.gpr[31].lo));
      u32 entry = ee.pc - 4;
      hooked_[index](*this);
      // Unless the service sent the program somewhere itself, return to the caller.
      if (ee.pc == entry + 4) {
        ee.leave();
      }
    }
    return;
  }
  // The kernel call's number is in v1, negative for the forms meant for
  // interrupt handlers.
  int number = static_cast<s32>(ee.gpr[3].lo);
  kernel(number < 0 ? -number : number);
}

// --- the kernel ------------------------------------------------------------------

void Machine::kernel(int number) {
  u32 a0 = arg(0), a1 = arg(1), a2 = arg(2), a3 = arg(3);
  log(4, "syscall %02x(%x, %x, %x, %x) at %08x", number, a0, a1, a2, a3, ee.pc - 4);
  switch (number) {
    case 0x02:  // SetGsCrt(interlace, mode, field)
      log(1, "SetGsCrt: interlace %u, mode %u, field %u", a0, a1, a2);
      break;
    case 0x04:  // Exit
      log(0, "the program called Exit(%d)", static_cast<s32>(a0));
      halted = true;
      ee.stop();
      break;
    case 0x10:  // AddIntcHandler(cause, handler, next, arg)
    case 0x12: {  // AddDmacHandler(channel, handler, next, arg)
      auto& list = (number == 0x10 ? intc_handlers : dmac_handlers)[a0 & 15];
      Handler h{a1, a3, next_id_++};
      if (a2 == 0) {
        list.insert(list.begin(), h);  // "next 0" puts it at the head
      } else {
        list.push_back(h);
      }
      result(static_cast<u64>(h.id));
      break;
    }
    case 0x11:    // RemoveIntcHandler(cause, id)
    case 0x13: {  // RemoveDmacHandler(channel, id)
      auto& list = (number == 0x11 ? intc_handlers : dmac_handlers)[a0 & 15];
      std::erase_if(list, [&](const Handler& h) { return h.id == static_cast<int>(a1); });
      result(list.size());
      break;
    }
    case 0x14: case 0x1A:  // _EnableIntc(cause)
      result((intc_mask >> a0) & 1 ? 0 : 1);
      intc_mask |= 1u << (a0 & 15);
      break;
    case 0x15: case 0x1B:  // _DisableIntc(cause)
      result((intc_mask >> a0) & 1);
      intc_mask &= ~(1u << (a0 & 15));
      break;
    case 0x16: case 0x1C:  // _EnableDmac(channel)
      result((d_stat >> (16 + a0)) & 1 ? 0 : 1);
      d_stat |= 1u << (16 + (a0 & 15));
      intc_mask |= 2;  // and the controller's own line to the CPU
      break;
    case 0x17: case 0x1D:  // _DisableDmac(channel)
      result((d_stat >> (16 + a0)) & 1);
      d_stat &= ~(1u << (16 + (a0 & 15)));
      break;
    case 0x20:  // CreateThread
      note("CreateThread (threads are not run)");
      result(static_cast<u64>(next_id_++));
      break;
    case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26:  // Delete, Start, Exit, ExitDelete, Terminate
    case 0x29: case 0x2A:                                              // ChangeThreadPriority
    case 0x2B: case 0x2C:                                              // RotateThreadReadyQueue
    case 0x32: case 0x33: case 0x34:                                   // Sleep, Wakeup
      result(0);
      break;
    case 0x2F:  // GetThreadId
      result(1);
      break;
    case 0x3C: {  // SetupThread(gp, stack, stack_size, args, root): the initial stack pointer
      u32 stack = a1 == 0xFFFFFFFFu ? GuestMemory::kRamBytes - a2 : a1;
      stack_top_ = stack + a2;
      result(stack_top_);
      break;
    }
    case 0x3D:  // SetupHeap(start, size): the end of the heap
      result(a1 == 0xFFFFFFFFu ? stack_top_ - 0x4000 : a0 + a1);
      break;
    case 0x3E:  // EndOfHeap
      result(stack_top_ - 0x4000);
      break;
    case 0x40: {  // CreateSema(param): the initial count is the third word
      int id = next_id_++;
      semaphores[id] = static_cast<int>(ee.read32(a0 + 8));
      result(static_cast<u64>(id));
      break;
    }
    case 0x41:  // DeleteSema
      semaphores.erase(static_cast<int>(a0));
      result(a0);
      break;
    case 0x42: case 0x43:  // SignalSema
      semaphores[static_cast<int>(a0)]++;
      result(a0);
      break;
    case 0x44:  // WaitSema: with one thread, a wait that would block cannot be satisfied
      if (semaphores[static_cast<int>(a0)] > 0) {
        semaphores[static_cast<int>(a0)]--;
      } else {
        note("WaitSema on an empty semaphore");
      }
      result(a0);
      break;
    case 0x45: case 0x46:  // PollSema
      if (semaphores[static_cast<int>(a0)] > 0) {
        semaphores[static_cast<int>(a0)]--;
        result(a0);
      } else {
        result(static_cast<u64>(-1));
      }
      break;
    case 0x61:  // EnableCache
    case 0x62:  // DisableCache
    case 0x64:  // FlushCache
    case 0x68:  // iFlushCache
      result(0);
      break;
    case 0x70:  // GsGetIMR
      result(gs_imr);
      break;
    case 0x71:  // GsPutIMR
      result(gs_imr);
      gs_imr = a0;
      break;
    case 0x73:  // SetVSyncFlag
      result(0);
      break;
    case 0x74:  // SetSyscall: a program's own kernel calls; the ones that patch the kernel are replaced
      result(0);
      break;
    case 0x76:  // SifDmaStat: every transfer is over
      result(static_cast<u64>(-1));
      break;
    case 0x77:  // SifSetDma: nothing is on the other side
      note("SifSetDma (no second processor)");
      result(static_cast<u64>(next_id_++));
      break;
    case 0x78:  // SifSetDChain
      result(0);
      break;
    case 0x79:  // SifSetReg
      sif_regs_[a0] = a1;
      result(0);
      break;
    case 0x7A:  // SifGetReg
      result(sif_regs_.count(a0) ? sif_regs_[a0] : 0);
      break;
    case 0x7F:  // GetMemorySize
      result(GuestMemory::kRamBytes);
      break;
    default:
      note("kernel call 0x" + [&] { char b[8]; std::snprintf(b, sizeof(b), "%02x", number); return std::string(b); }());
      result(0);
      break;
  }
}

// --- starting ------------------------------------------------------------------

bool Machine::boot(std::string* error) {
  auto fail = [&](const std::string& why) {
    if (error) *error = why;
    return false;
  };
  auto cnf = disc.find("SYSTEM.CNF");
  if (!cnf) {
    return fail("no SYSTEM.CNF on the disc");
  }
  std::vector<u8> text = disc.read_file(*cnf);
  std::string config(text.begin(), text.end());
  std::size_t at = config.find("cdrom0:");
  if (at == std::string::npos) {
    return fail("SYSTEM.CNF names no program");
  }
  std::size_t end = config.find_first_of(";\r\n", at);
  std::string name = config.substr(at + 7, end - at - 7);
  auto program = disc.find(name);
  if (!program) {
    return fail("the disc has no " + name);
  }
  std::vector<u8> elf = disc.read_file(*program);
  if (elf.size() < 52 || std::memcmp(elf.data(), "\x7F" "ELF", 4) != 0) {
    return fail(name + " is not an ELF program");
  }
  u32 entry = load<u32>(&elf[24]), phoff = load<u32>(&elf[28]);
  unsigned phentsize = load<u16>(&elf[42]), phnum = load<u16>(&elf[44]);
  for (unsigned n = 0; n < phnum; n++) {
    const u8* ph = &elf[phoff + n * phentsize];
    if (load<u32>(ph) != 1) {
      continue;  // not a loadable segment
    }
    u32 offset = load<u32>(ph + 4), address = load<u32>(ph + 8), filesz = load<u32>(ph + 16), memsz = load<u32>(ph + 20);
    if (offset + filesz > elf.size() || (address & 0x1FFFFFFF) + memsz > GuestMemory::kRamBytes) {
      return fail("a segment of " + name + " does not fit");
    }
    std::memcpy(memory.ram(address), &elf[offset], filesz);
    std::memset(memory.ram(address + filesz), 0, memsz - filesz);
    log(1, "loaded %s: %08x-%08x", name.c_str(), address, address + memsz);
  }
  ee.reset();
  ee.pc = entry;
  ee.next_pc = entry + 4;
  ee.gpr[29].lo = 0x01FFF000;

  frame_cycles_ = static_cast<u64>(static_cast<double>(kEeHz) / hz);
  vblank_at_ = frame_cycles_;
  ee.event_at = vblank_at_;
  halted = false;
  frames = 0;
  return true;
}

// --- time and interrupts -----------------------------------------------------------

void Machine::run_frame() {
  frame_done_ = false;
  while (!frame_done_ && !halted && !ee.vu0_runaways) {
    ee.run(~u64{0});
    if (ee.unknown && verbose >= 0 && last_reported_unknown_ != ee.unknown) {
      last_reported_unknown_ = ee.unknown;
      log(0, "the EE met an instruction it does not know: %08x at %08x (%llu so far)", ee.last_unknown,
          ee.last_unknown_pc, static_cast<unsigned long long>(ee.unknown));
    }
  }
}

void Machine::skip_to_vblank() {
  if (ee.cycles < vblank_at_) {
    ee.cycles = vblank_at_;
  }
}

void Machine::raise(unsigned cause) {
  intc_stat |= 1u << cause;
  pending_ |= 1u << cause;
  ee.event_at = ee.cycles;  // look at it before the next instruction
}

void Machine::event() {
  if (ee.cycles >= vblank_end_at_) {
    vblank_end_at_ = ~u64{0};
    raise(3);
  }
  if (ee.cycles >= vblank_at_) {
    vblank();
  }
  deliver();
  if (ee.cycles >= cd_callback_at_ && !in_handler_ && (ee.cop0[12] & 0x10001) == 0x10001) {
    cd_callback_at_ = ~u64{0};
    if (cd_callback) {
      in_handler_ = true;
      ee.call(cd_callback, 1);  // reason 1: a read finished
      in_handler_ = false;
    }
  }
  update_event();
}

void Machine::update_event() {
  u64 next = std::min({vblank_at_, vblank_end_at_, cd_callback_at_ > ee.cycles ? cd_callback_at_ : ee.cycles + 4000});
  // Interrupts the program has switched off stay pending; look again soon.
  if (pending_ & intc_mask) {
    next = std::min(next, ee.cycles + 4000);
  }
  ee.event_at = next;
}

void Machine::vblank() {
  vblank_end_at_ = vblank_at_ + frame_cycles_ / 12;
  vblank_at_ += frame_cycles_;
  frames++;
  graphics.gs.vblank(odd_field());
  raise(2);
  deliver();
  if (on_vblank) {
    on_vblank();
  }
  frame_done_ = true;
  ee.stop();
}

void Machine::deliver() {
  // Handlers run with interrupts off, one cause at a time, and only when the
  // program has interrupts on.
  if (in_handler_ || (ee.cop0[12] & 0x10001) != 0x10001) {
    return;
  }
  in_handler_ = true;
  for (unsigned cause = 0; cause < 16; cause++) {
    u32 bit = 1u << cause;
    if (!(pending_ & bit)) {
      continue;
    }
    if (!(intc_mask & bit)) {
      // Not asked for: forget it, the status bit stays for anyone polling.
      if (cause != 1) {
        pending_ &= ~bit;
      }
      continue;
    }
    pending_ &= ~bit;
    intc_stat &= ~bit;
    if (cause == 1) {
      // The DMA controller: one handler list per channel that finished and
      // whose interrupt the program enabled.
      for (unsigned channel = 0; channel < 10; channel++) {
        u32 done = 1u << channel, enabled = 1u << (16 + channel);
        if ((d_stat & done) && (d_stat & enabled)) {
          d_stat &= ~done;
          for (const Handler& h : std::vector<Handler>(dmac_handlers[channel])) {
            ee.call(h.function, channel, h.argument);
          }
        }
      }
    } else {
      for (const Handler& h : std::vector<Handler>(intc_handlers[cause])) {
        ee.call(h.function, cause, h.argument);
      }
    }
  }
  in_handler_ = false;
}

// --- DMA -----------------------------------------------------------------------

void Machine::dma_done(unsigned channel) {
  dma[channel].chcr &= ~0x100u;
  d_stat |= 1u << channel;
  if (d_stat & (1u << (16 + channel))) {
    raise(1);
  }
}

void Machine::dma_start(unsigned channel) {
  DmaChannel& ch = dma[channel];
  bool from_memory = (ch.chcr & 1) != 0;
  unsigned mode = (ch.chcr >> 2) & 3;
  log(3, "DMA channel %u: chcr %08x madr %08x qwc %x tadr %08x", channel, ch.chcr, ch.madr, ch.qwc, ch.tadr);

  switch (channel) {
    case kVif0:
    case kVif1: {
      Vif1& vif = channel == kVif0 ? vif0 : graphics.vif;
      DmaSink sink = [&vif](const u8* data, std::size_t bytes) { vif.write(data, bytes); };
      if (!from_memory) {
        // To memory: what the GS sends back from a local-to-host transfer.
        graphics.gs.transfer_out(memory.dma(ch.madr), std::size_t{ch.qwc} * 16);
        ch.madr += ch.qwc * 16;
        ch.qwc = 0;
      } else if (mode == 1) {
        if (run_source_chain(memory, ch, sink) == DmaStop::Runaway) {
          note("a VIF display list that does not end");
        }
      } else {
        sink(memory.dma(ch.madr), std::size_t{ch.qwc} * 16);
        ch.madr += ch.qwc * 16;
        ch.qwc = 0;
      }
      break;
    }
    case kGifChannel: {
      DmaSink sink = [this](const u8* data, std::size_t bytes) { graphics.gif.write(3, data, bytes / 16); };
      if (mode == 1) {
        ch.chcr &= ~0x40u;  // GIF chains carry no data in their tags
        run_source_chain(memory, ch, sink);
      } else {
        sink(memory.dma(ch.madr), std::size_t{ch.qwc} * 16);
        ch.madr += ch.qwc * 16;
        ch.qwc = 0;
      }
      break;
    }
    case kFromSpr: {
      // Scratchpad to memory.
      u32& sadr = dma_sadr[channel];
      if (mode == 0) {
        std::memcpy(memory.dma(ch.madr), memory.scratchpad(sadr), std::size_t{ch.qwc} * 16);
        ch.madr += ch.qwc * 16;
        sadr = (sadr + ch.qwc * 16) & 0x3FFF;
        ch.qwc = 0;
      } else {
        // Destination chain: each tag in the scratchpad says where its data goes.
        for (int guard = 0; guard < 4096; guard++) {
          u64 tag = load<u64>(memory.scratchpad(sadr));
          u32 qwc = static_cast<u32>(tag & 0xFFFF), id = static_cast<u32>((tag >> 28) & 7);
          u32 to = static_cast<u32>(tag >> 32);
          sadr = (sadr + 16) & 0x3FFF;
          std::memcpy(memory.dma(to), memory.scratchpad(sadr), std::size_t{qwc} * 16);
          sadr = (sadr + qwc * 16) & 0x3FFF;
          ch.chcr = (ch.chcr & 0xFFFF) | (static_cast<u32>(tag) & 0xFFFF0000u);
          if (id == 7 || ((tag >> 31) & 1 && (ch.chcr & 0x80))) {
            break;
          }
        }
      }
      break;
    }
    case kToSpr: {
      // Memory to scratchpad.
      u32& sadr = dma_sadr[channel];
      DmaSink sink = [this, &sadr](const u8* data, std::size_t bytes) {
        for (std::size_t n = 0; n < bytes; n++) {
          *memory.scratchpad(sadr) = data[n];
          sadr = (sadr + 1) & 0x3FFF;
        }
      };
      if (mode == 1) {
        run_source_chain(memory, ch, sink);
      } else {
        sink(memory.dma(ch.madr), std::size_t{ch.qwc} * 16);
        ch.madr += ch.qwc * 16;
        ch.qwc = 0;
      }
      break;
    }
    default:
      note("DMA on channel " + std::to_string(channel));
      break;
  }
  dma_done(channel);
}

// --- hardware registers ----------------------------------------------------------

u32 Machine::timer_count(const Timer& t) const {
  if (!(t.mode & 0x80)) {
    return t.base_count & 0xFFFF;
  }
  // The bus clock is half the CPU's; then /1, /16, /256, or one count a scan line.
  static constexpr u64 divider[3] = {2, 32, 512};
  unsigned clock = t.mode & 3;
  u64 per_count = clock < 3 ? divider[clock] : frame_cycles_ / (hz < 55 ? 312 : 262);
  return static_cast<u32>(t.base_count + (ee.cycles - t.base_cycles) / per_count) & 0xFFFF;
}

u64 Machine::hw_read(u32 address, unsigned bytes) {
  (void)bytes;
  if (address >= 0x12000000 && address < 0x12002000) {
    return graphics.gs.read_privileged(address & ~0xFu);
  }
  if (address >= 0x10000000 && address < 0x10002000) {
    const Timer& t = timers[(address >> 11) & 3];
    switch (address & 0x7F0) {
      case 0x00: return timer_count(t);
      case 0x10: return t.mode;
      case 0x20: return t.compare;
      default: return t.hold;
    }
  }
  if (address >= 0x10008000 && address < 0x1000E000) {
    for (unsigned n = 0; n < 10; n++) {
      if ((address & ~0xFFu) == kDmaBase[n]) {
        switch (address & 0xF0) {
          case 0x00: return dma[n].chcr;
          case 0x10: return dma[n].madr;
          case 0x20: return dma[n].qwc;
          case 0x30: return dma[n].tadr;
          case 0x40: return dma[n].asr[0];
          case 0x50: return dma[n].asr[1];
          case 0x80: return dma_sadr[n];
          default: return 0;
        }
      }
    }
  }
  switch (address) {
    case 0x1000E000: return d_ctrl;
    case 0x1000E010: return d_stat;
    case 0x1000E020: return d_pcr;
    case 0x1000E030: return d_sqwc;
    case 0x1000E040: return d_rbsr;
    case 0x1000E050: return d_rbor;
    case 0x1000F000: return intc_stat;
    case 0x1000F010: return intc_mask;
    case 0x1000F520: return d_enable;
    case 0x10003020:  // GIF_STAT
    case 0x10003800:  // VIF0_STAT
    case 0x10003C00:  // VIF1_STAT: idle, FIFOs empty
    case 0x1000F130:  // serial status
      return 0;
    default: {
      char text[48];
      std::snprintf(text, sizeof(text), "a read of hardware register %08x", address);
      note(text);
      return 0;
    }
  }
}

void Machine::hw_write(u32 address, u64 value, unsigned bytes) {
  (void)bytes;
  u32 v = static_cast<u32>(value);
  if (address >= 0x12000000 && address < 0x12002000) {
    graphics.gs.write_privileged(address & ~0xFu, value);
    return;
  }
  if (address >= 0x10000000 && address < 0x10002000) {
    Timer& t = timers[(address >> 11) & 3];
    switch (address & 0x7F0) {
      case 0x00:
        t.base_count = v & 0xFFFF;
        t.base_cycles = ee.cycles;
        break;
      case 0x10:
        t.base_count = timer_count(t);
        t.base_cycles = ee.cycles;
        t.mode = v & 0x3FF;  // bits 10 and 11 are flags, cleared by writing them
        break;
      case 0x20:
        t.compare = v & 0xFFFF;
        break;
      default:
        t.hold = v & 0xFFFF;
        break;
    }
    return;
  }
  if (address >= 0x10008000 && address < 0x1000E000) {
    for (unsigned n = 0; n < 10; n++) {
      if ((address & ~0xFFu) != kDmaBase[n]) {
        continue;
      }
      switch (address & 0xF0) {
        case 0x00:
          dma[n].chcr = v;
          if (v & 0x100) {
            dma_start(n);
          }
          break;
        case 0x10: dma[n].madr = v; break;
        case 0x20: dma[n].qwc = v & 0xFFFF; break;
        case 0x30: dma[n].tadr = v; break;
        case 0x40: dma[n].asr[0] = v; break;
        case 0x50: dma[n].asr[1] = v; break;
        case 0x80: dma_sadr[n] = v & 0x3FFF; break;
        default: break;
      }
      return;
    }
  }
  switch (address) {
    case 0x1000E000: d_ctrl = v; break;
    case 0x1000E010:
      // Writing 1 clears a channel's status bit and flips its enable bit.
      d_stat &= ~(v & 0x3FF);
      d_stat ^= v & 0x03FF0000;
      break;
    case 0x1000E020: d_pcr = v; break;
    case 0x1000E030: d_sqwc = v; break;
    case 0x1000E040: d_rbsr = v; break;
    case 0x1000E050: d_rbor = v; break;
    case 0x1000F590: d_enable = v; break;
    case 0x1000F000: intc_stat &= ~v; break;
    case 0x1000F010: intc_mask ^= v & 0x7FFF; break;
    case 0x1000F180:  // the serial port: the kernel's console
      if (v == '\n') {
        log(0, "[tty] %s", tty_.c_str());
        tty_.clear();
      } else if (v != '\r') {
        tty_.push_back(static_cast<char>(v));
      }
      break;
    case 0x10003000:  // GIF_CTRL
    case 0x10003810:  // VIF0_FBRST
    case 0x10003820:  // VIF0_ERR
    case 0x10003C10:  // VIF1_FBRST
    case 0x10003C20:  // VIF1_ERR
    case 0x1000F100:  // serial control
      break;
    default: {
      char text[64];
      std::snprintf(text, sizeof(text), "a write to hardware register %08x", address);
      note(text);
      break;
    }
  }
}

void Machine::hw_write128(u32 address, u64 lo, u64 hi) {
  u8 quad[16];
  store<u64>(quad, lo);
  store<u64>(quad + 8, hi);
  switch (address) {
    case 0x10004000: vif0.write(quad, 16); break;          // VIF0 FIFO
    case 0x10005000: graphics.vif.write(quad, 16); break;  // VIF1 FIFO
    case 0x10006000: graphics.gif.write(3, quad, 1); break;  // GIF FIFO
    default:
      hw_write(address, lo, 8);
      hw_write(address + 8, hi, 8);
      break;
  }
}

void Machine::add_default_services() {
  add_service("return0", [](Machine& m) { m.result(0); });
  add_service("return1", [](Machine& m) { m.result(1); });
  add_service("return2", [](Machine& m) { m.result(2); });
  add_service("return_minus1", [](Machine& m) { m.result(static_cast<u64>(-1)); });
  add_service("nothing", [](Machine&) {});
  add_library_services(*this);
}

}  // namespace sys
