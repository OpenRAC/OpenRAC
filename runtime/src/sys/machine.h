// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <array>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "disc.h"
#include "drawing.h"
#include "ps2/dma.h"
#include "ps2/ee.h"
#include "ps2/graphics.h"
#include "ps2/memory.h"

namespace sys {

using ps2::u16;
using ps2::u32;
using ps2::u64;
using ps2::u8;

class Machine;

// A function of the game's program that the runtime answers itself. It
// reads its arguments from the EE's registers and sets the result; the
// machine then returns to the caller.
using Service = std::function<void(Machine&)>;

// The whole console as a game program needs it, without the parts a game
// only reaches through Sony's libraries: the EE with its memory, the vector
// units, the drawing path, the DMA controller, the timers, the interrupt
// controller and the kernel's services. The second processor and everything
// behind it (disc drive, memory cards, pads, sound) are not modelled; the
// library functions that talk to them are replaced by name (see `hook`).
class Machine {
 public:
  Machine();
  Machine(const Machine&) = delete;
  Machine& operator=(const Machine&) = delete;

  // --- the parts ---
  ps2::GuestMemory memory;
  ps2::Graphics graphics;  // VIF1, VU1, GIF, GS
  // How the machine reaches them: in order, and on a thread of their own
  // once `drawing.start()` has been called. Reading `graphics` directly is
  // for after `drawing.sync()`.
  Drawing drawing{graphics};
  ps2::Gif unused_gif{graphics.gs};
  ps2::Vif1 vif0{unused_gif};
  ps2::Vu vu0{ps2::Vu::Memory{vif0.micro.data(), 4096, vif0.data.data(), 4096}};
  ps2::Ee ee{memory, vu0};
  Disc disc;

  // --- starting ---
  // Load the program the disc boots (SYSTEM.CNF's BOOT2) and get ready to
  // run it. `hz` is the display's field rate.
  bool boot(std::string* error = nullptr);
  double hz = 50.0;
  std::string program_name;  // the program's file on the disc, which is the disc's code (SCES_509.16)

  // --- running ---
  // Run until the next vertical blank has been announced to the program.
  void run_frame();
  // Called at each vertical blank, after the game's handlers: show a frame.
  std::function<void()> on_vblank;
  bool halted = false;
  u64 frames = 0;

  // --- replacing library functions ---
  // Make calls to `address` go to the named service.
  bool hook(u32 address, const std::string& service);
  // A table of `address service` lines; '#' starts a comment.
  bool load_hooks(const std::string& path, std::string* error = nullptr);
  void add_service(const std::string& name, Service service) { services_[name] = std::move(service); }
  bool has_service(const std::string& name) const { return services_.count(name) != 0; }

  // For services: arguments, result, guest strings, time.
  u32 arg(unsigned n) { return static_cast<u32>(ee.gpr[4 + n].lo); }
  void result(u64 value) { ee.gpr[2].lo = value; }
  std::string string_at(u32 address, std::size_t limit = 1024);
  std::string format(u32 format_address, unsigned first_arg);
  // Let guest time jump to the next vertical blank (the program is waiting).
  void skip_to_vblank();
  bool odd_field() const { return (frames & 1) != 0; }

  // --- what the replaced libraries share ---
  // The pad as libpad reports it: buttons as a bit mask (bit set = pressed,
  // in the order select, L3, R3, start, up, right, down, left, L2, R2, L1,
  // R1, triangle, circle, cross, square) and the two sticks, 0x80 centred.
  struct Pad {
    u16 buttons = 0;
    u8 right_x = 0x80, right_y = 0x80, left_x = 0x80, left_y = 0x80;
  } pad;
  // The function the program asked to be told when a disc read finishes.
  u32 cd_callback = 0;
  // Tell it soon, as the drive would: not before the caller has returned.
  void cd_read_finished() { cd_callback_at_ = ee.cycles + 200000; update_event(); }
  // The silent sound server's next handle for a bank, stream or sound.
  u32 sound_next_handle = 0x100;
  // The memory card in the first slot: a directory of the host (none when
  // the name is empty), the files the program has open on it, and whether
  // the program has been told of it yet.
  struct Card {
    std::string directory;
    std::unordered_map<int, std::FILE*> open;
    bool seen = false;
    ~Card();
  } card;
  // The memory card library's "last function and its result".
  int mc_function = 0, mc_result = 0;

  // --- interrupts ---
  void raise(unsigned cause);  // an INTC cause: 1 DMAC, 2 vertical blank start, 3 its end, ...

  // --- what to print ---
  int verbose = 0;
  void log(int level, const char* format, ...) __attribute__((format(printf, 3, 4)));
  // Counts of things met that are not modelled, by a short description.
  std::map<std::string, u64> notes;
  void note(const std::string& what);

  // The kernel's memory of what the program asked for.
  struct Handler {
    u32 function = 0, argument = 0;
    int id = 0;
  };
  std::array<std::vector<Handler>, 16> intc_handlers;
  std::array<std::vector<Handler>, 16> dmac_handlers;
  std::unordered_map<int, int> semaphores;

  // Hardware registers the EE reads and writes directly.
  std::array<ps2::DmaChannel, 10> dma{};
  std::array<u32, 10> dma_sadr{};
  u32 d_ctrl = 0, d_stat = 0, d_pcr = 0, d_sqwc = 0, d_rbsr = 0, d_rbor = 0, d_enable = 0x1201;
  u32 intc_stat = 0, intc_mask = 0;
  struct Timer {
    u32 mode = 0, compare = 0, hold = 0, base_count = 0;
    u64 base_cycles = 0;
  };
  std::array<Timer, 4> timers{};
  u32 gs_imr = 0;

  static constexpr u64 kEeHz = 294912000;

 private:
  u64 hw_read(u32 address, unsigned bytes);
  void hw_write(u32 address, u64 value, unsigned bytes);
  void hw_write128(u32 address, u64 lo, u64 hi);
  void event();
  void vblank();
  void deliver();
  void syscall(u32 code);
  void kernel(int number);
  void dma_start(unsigned channel);
  void dma_done(unsigned channel);
  u32 timer_count(const Timer& t) const;
  void add_default_services();

  std::unordered_map<std::string, Service> services_;
  std::vector<Service> hooked_;
  std::vector<std::string> hooked_names_;
  std::unordered_map<u32, u32> hooked_original_;

  u64 frame_cycles_ = 0, vblank_at_ = 0, vblank_end_at_ = ~u64{0};
  u32 pending_ = 0;
  bool in_handler_ = false, frame_done_ = false;
  int next_id_ = 1;
  std::string tty_;
  std::unordered_map<u32, u32> sif_regs_;
  u32 stack_top_ = ps2::GuestMemory::kRamBytes;
  u64 last_reported_unknown_ = 0;
  u64 cd_callback_at_ = ~u64{0};
  void update_event();
};

// Services every game shares, registered by `add_default_services` and
// defined in services.cpp.
void add_library_services(Machine& machine);
// The memory card library (memcard.cpp).
void add_memory_card_services(Machine& machine);

}  // namespace sys
