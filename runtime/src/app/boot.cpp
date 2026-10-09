// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// openrac-boot: runs the program on your own disc image and reports how far
// it gets. A development tool: it shows what the machine model still lacks.

#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "ps2/vu_dis.h"
#include "sys/machine.h"

#ifndef OPENRAC_NO_WINDOW
#include "host/window.h"
#endif

using namespace ps2;

namespace {

bool write_ppm(const std::string& path, const Image& image) {
  std::FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) {
    return false;
  }
  std::fprintf(f, "P6\n%d %d\n255\n", image.width, image.height);
  for (u32 p : image.pixels) {
    u8 rgb[3] = {static_cast<u8>(p), static_cast<u8>(p >> 8), static_cast<u8>(p >> 16)};
    std::fwrite(rgb, 1, 3, f);
  }
  std::fclose(f);
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  std::string iso, hooks, ppm;
  int frames = -1, report = 60, states_frame = -1;
  // Threads that draw: by default most of the fast cores, leaving one for the program itself.
  int gs_threads = static_cast<int>(std::min(12u, std::max(2u, std::thread::hardware_concurrency()) - 1));
  bool window_wanted = false;
  // Scripted input: hold these buttons from one frame for some frames.
  struct Press {
    int frame, length;
    unsigned buttons;
  };
  std::vector<Press> presses;
  sys::Machine machine;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--hooks" && i + 1 < argc) {
      hooks = argv[++i];
    } else if (arg == "--frames" && i + 1 < argc) {
      frames = std::atoi(argv[++i]);
    } else if (arg == "--report" && i + 1 < argc) {
      report = std::atoi(argv[++i]);
    } else if (arg == "--verbose" && i + 1 < argc) {
      machine.verbose = std::atoi(argv[++i]);
    } else if (arg == "--ppm" && i + 1 < argc) {
      ppm = argv[++i];
    } else if (arg == "--press" && i + 1 < argc) {
      // FRAME:BUTTONS[:FRAMES], buttons as a hexadecimal mask (cross is 4000, start 8)
      Press p{0, 4, 0};
      std::sscanf(argv[++i], "%d:%x:%d", &p.frame, &p.buttons, &p.length);
      presses.push_back(p);
    } else if (arg == "--gs-states" && i + 1 < argc) {
      states_frame = std::atoi(argv[++i]);
    } else if (arg == "--gs-threads" && i + 1 < argc) {
      gs_threads = std::atoi(argv[++i]);
    } else if (arg == "--ntsc") {
      machine.hz = 59.94;
    } else if (arg == "--window") {
      window_wanted = true;
    } else if (arg[0] != '-' && iso.empty()) {
      iso = arg;
    } else {
      std::fprintf(stderr, "usage: openrac-boot DISC.iso [--hooks FILE] [--frames N] [--report N] [--verbose N] "
                           "[--ntsc] [--window] [--ppm FILE] [--press FRAME:BUTTONS[:FRAMES]]\n");
      return 2;
    }
  }
  if (frames < 0) {
    frames = window_wanted ? INT_MAX : 600;  // a window runs until it is closed
  }
  if (iso.empty() || !machine.disc.open(iso)) {
    std::fprintf(stderr, "cannot open the disc image %s\n", iso.c_str());
    return 1;
  }
  machine.graphics.gs.set_threads(static_cast<unsigned>(std::max(gs_threads, 0)));
  std::string error;
  if (!machine.boot(&error) || (!hooks.empty() && !machine.load_hooks(hooks, &error))) {
    std::fprintf(stderr, "%s\n", error.c_str());
    return 1;
  }

#ifndef OPENRAC_NO_WINDOW
  host::Window window;
  if (window_wanted && !window.open("OpenRAC", 960, 720)) {
    return 1;
  }
#else
  (void)window_wanted;
#endif

  Image image;
  auto reported_at = std::chrono::steady_clock::now();
  auto next_frame_at = reported_at;
  for (int frame = 0; frame < frames && !machine.halted; frame++) {
#ifndef OPENRAC_NO_WINDOW
    if (window_wanted && !window.pump()) {
      break;
    }
#endif
    // For working on the GS: what one frame is drawn with, by state, with
    // how many primitives and the area they span.
    struct Use {
      int count = 0, x0 = 1 << 30, y0 = 1 << 30, x1 = -(1 << 30), y1 = -(1 << 30), first = 0;
      float lod0 = 1e9f, lod1 = -1e9f;
    };
    std::map<std::string, Use> states;
    int order = 0;
    if (frame == states_frame) {
      machine.graphics.gs.on_primitive = [&](const std::string& state, int x0, int y0, int x1, int y1, float lod0, float lod1) {
        Use& u = states[state];
        if (u.count++ == 0) {
          u.first = order;
        }
        order++;
        u.x0 = std::min(u.x0, x0);
        u.y0 = std::min(u.y0, y0);
        u.x1 = std::max(u.x1, x1);
        u.y1 = std::max(u.y1, y1);
        u.lod0 = std::min(u.lod0, lod0);
        u.lod1 = std::max(u.lod1, lod1);
      };
    }
    machine.pad.buttons = 0;
    for (const Press& p : presses) {
      if (frame >= p.frame && frame < p.frame + p.length) {
        machine.pad.buttons |= static_cast<u16>(p.buttons);
      }
    }
#ifndef OPENRAC_NO_WINDOW
    if (window_wanted) {
      host::Controls c = window.controls();
      machine.pad.buttons |= static_cast<u16>(c.buttons);
      machine.pad.left_x = c.left_x;
      machine.pad.left_y = c.left_y;
      machine.pad.right_x = c.right_x;
      machine.pad.right_y = c.right_y;
    }
#endif
    machine.run_frame();
    if (machine.ee.vu0_runaways) {
      break;
    }
    if (frame == states_frame) {
      machine.graphics.gs.on_primitive = nullptr;
      std::vector<std::pair<int, std::string>> lines;
      for (const auto& [state, u] : states) {
        char head[96];
        std::snprintf(head, sizeof(head), "%6d x%-6d [%4d,%4d - %4d,%4d] ", u.first, u.count, u.x0, u.y0, u.x1, u.y1);
        char tail[48] = "";
        if (u.lod1 >= u.lod0 && (u.lod0 != 0 || u.lod1 != 0)) {
          std::snprintf(tail, sizeof(tail), " | lod %.2f..%.2f", static_cast<double>(u.lod0), static_cast<double>(u.lod1));
        }
        lines.push_back({u.first, head + state + tail});
      }
      std::sort(lines.begin(), lines.end());
      for (const auto& line : lines) {
        std::printf("%s\n", line.second.c_str());
      }
    }
    bool shown = machine.graphics.gs.display(image);
#ifndef OPENRAC_NO_WINDOW
    if (window_wanted && shown) {
      window.present(image, 4.0f / 3.0f);
    }
    if (window_wanted) {
      // No faster than the console: one frame per field.
      next_frame_at += std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(1.0 / machine.hz));
      auto now = std::chrono::steady_clock::now();
      if (next_frame_at > now) {
        std::this_thread::sleep_until(next_frame_at);
      } else {
        next_frame_at = now;
      }
    }
#else
    (void)shown;
#endif
    if (report > 0 && (frame + 1) % report == 0) {
      auto now = std::chrono::steady_clock::now();
      double seconds = std::chrono::duration<double>(now - reported_at).count();
      reported_at = now;
      std::fprintf(stderr, "frame %d: pc %08x ra %08x, %llu primitives, %llu pixels, %llu VU1 instructions, %.1f frames a second\n",
                   frame + 1, machine.ee.pc, static_cast<u32>(machine.ee.gpr[31].lo),
                   static_cast<unsigned long long>(machine.graphics.gs.stats.primitives),
                   static_cast<unsigned long long>(machine.graphics.gs.stats.pixels),
                   static_cast<unsigned long long>(machine.graphics.vu1_instructions), report / std::max(seconds, 1e-9));
    }
  }

  std::fprintf(stderr, "gs: %llu texture levels decoded (%llu texels), %llu transfers, %llu batches drawn\n",
               static_cast<unsigned long long>(machine.graphics.gs.stats.texture_decodes),
               static_cast<unsigned long long>(machine.graphics.gs.stats.texels_decoded),
               static_cast<unsigned long long>(machine.graphics.gs.stats.transfers),
               static_cast<unsigned long long>(machine.graphics.gs.stats.flushes));
  std::fprintf(stderr, "stopped after %llu frames at pc %08x (ra %08x)\n", static_cast<unsigned long long>(machine.frames),
               machine.ee.pc, static_cast<u32>(machine.ee.gpr[31].lo));
  for (const auto& [what, count] : machine.notes) {
    std::fprintf(stderr, "  not modelled: %s (%llu times)\n", what.c_str(), static_cast<unsigned long long>(count));
  }
  if (machine.ee.unknown) {
    std::fprintf(stderr, "  %llu unknown EE instructions, the last %08x at %08x\n",
                 static_cast<unsigned long long>(machine.ee.unknown), machine.ee.last_unknown, machine.ee.last_unknown_pc);
  }
  if (machine.vu0.unknown_ops || machine.graphics.vu1.unknown_ops || machine.graphics.vif.unknown_codes) {
    std::fprintf(stderr, "  unknown: %llu VU0, %llu VU1 instructions, %llu VIF1 codes\n",
                 static_cast<unsigned long long>(machine.vu0.unknown_ops),
                 static_cast<unsigned long long>(machine.graphics.vu1.unknown_ops),
                 static_cast<unsigned long long>(machine.graphics.vif.unknown_codes));
  }
  if (machine.ee.vu0_runaways) {
    std::fprintf(stderr, "  %llu VU0 microprograms did not stop (the first started at %u by the EE at %08x)\n",
                 static_cast<unsigned long long>(machine.ee.vu0_runaways), machine.ee.vu0_runaway_start,
                 machine.ee.vu0_runaway_from);
    u32 at = machine.ee.vu0_runaway_start;
    if (std::getenv("OPENRAC_VU0_LISTING")) {
      // For working on the interpreter: the program as it sits in VU0, and where it was.
      for (u32 n = 0; n < 512; n++) {
        u32 up = load<u32>(&machine.vif0.micro[n * 8 + 4]), low = load<u32>(&machine.vif0.micro[n * 8]);
        if (up || low) {
          std::fprintf(stderr, "%s\n", vudis::pair(n, up, low).c_str());
        }
      }
      std::fprintf(stderr, "pc %u; vi:", machine.vu0.pc);
      for (unsigned n = 0; n < 16; n++) {
        std::fprintf(stderr, " %04x", machine.vu0.vi[n]);
      }
      std::fprintf(stderr, "\n");
    }
    (void)at;
  }
  if (machine.graphics.vu1_runaways) {
    std::fprintf(stderr, "  %llu of %llu VU1 program starts did not stop (the last from %u, at %u when cut off)\n",
                 static_cast<unsigned long long>(machine.graphics.vu1_runaways),
                 static_cast<unsigned long long>(machine.graphics.vu1_starts), machine.graphics.vu1_runaway_start,
                 machine.graphics.vu1_runaway_pc);
  }
  if (!ppm.empty() && machine.graphics.gs.display(image)) {
    write_ppm(ppm, image);
  }
  return 0;
}
