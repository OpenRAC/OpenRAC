// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// openrac-<id>-cli (libopenrac-<id>-cli.so in the APK): the game's windowless program, which the
// Android launcher runs for what needs no window (--check-levels, an interface export). The game
// itself is libopenrac-<id>.so, the library SDL's activity loads; this program calls the same main
// in it (SDL_main), so the APK carries each game's code once, not twice.

int SDL_main(int argc, char** argv);

int main(int argc, char** argv) {
    return SDL_main(argc, argv);
}
