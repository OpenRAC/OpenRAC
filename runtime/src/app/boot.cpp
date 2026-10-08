// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// openrac-boot: runs the program on your own disc image and reports how far
// it gets. A development tool: it shows what the machine model still lacks.

#include <cstdio>
#include <cstdlib>
#include <string>

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
  int frames = 600, report = 60;
  bool window_wanted = false;
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
    } else if (arg == "--ntsc") {
      machine.hz = 59.94;
    } else if (arg == "--window") {
      window_wanted = true;
    } else if (arg[0] != '-' && iso.empty()) {
      iso = arg;
    } else {
      std::fprintf(stderr, "usage: openrac-boot DISC.iso [--hooks FILE] [--frames N] [--report N] [--verbose N] "
                           "[--ntsc] [--window] [--ppm FILE]\n");
      return 2;
    }
  }
  if (iso.empty() || !machine.disc.open(iso)) {
    std::fprintf(stderr, "cannot open the disc image %s\n", iso.c_str());
    return 1;
  }
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
  for (int frame = 0; frame < frames && !machine.halted; frame++) {
#ifndef OPENRAC_NO_WINDOW
    if (window_wanted && !window.pump()) {
      break;
    }
#endif
    machine.run_frame();
    bool shown = machine.graphics.gs.display(image);
#ifndef OPENRAC_NO_WINDOW
    if (window_wanted && shown) {
      window.present(image, 4.0f / 3.0f);
    }
#else
    (void)shown;
#endif
    if (report > 0 && (frame + 1) % report == 0) {
      std::fprintf(stderr, "frame %d: pc %08x ra %08x, %llu primitives, %llu pixels, %llu VU1 instructions\n", frame + 1,
                   machine.ee.pc, static_cast<u32>(machine.ee.gpr[31].lo),
                   static_cast<unsigned long long>(machine.graphics.gs.stats.primitives),
                   static_cast<unsigned long long>(machine.graphics.gs.stats.pixels),
                   static_cast<unsigned long long>(machine.graphics.vu1_instructions));
    }
  }

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
  if (!ppm.empty() && machine.graphics.gs.display(image)) {
    write_ppm(ppm, image);
  }
  return 0;
}
