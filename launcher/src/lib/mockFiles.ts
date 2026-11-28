// The browser preview's Settings > Files places (core/src/files.rs) and keyboard shortcuts
// (core/src/key_shortcuts.rs), kept in memory. Values are taken as given; the real checks are the
// core's.

import type { FilePlace } from "./generated/FilePlace";
import type { ShortcutRow } from "./generated/ShortcutRow";

const HOME = "/home/you/.local/share/openrac";

export function mockFilePlaces(
  game: string | null,
  version: string | null,
  developer: boolean,
): FilePlace[] {
  const place = (
    id: string,
    title: string,
    help: string,
    group: FilePlace["group"],
    path: string,
    count = 0,
  ): FilePlace => ({ id, title, help, group, path, count });
  const out = [
    place(
      "screenshots",
      "Screenshots",
      "Pictures you took in the game.",
      "yours",
      `${HOME}/screenshots`,
      14,
    ),
    place("saves", "Saves", "Each game's memory card.", "yours", `${HOME}/memcard`, 2),
    place("settings", "Settings", "Each game's settings file.", "yours", `${HOME}/settings`, 3),
    place("controls", "Controls", "Your controls and shortcuts.", "yours", `${HOME}/controls`, 1),
    place("mod-files", "Mod files", "Each mod's own files.", "yours", `${HOME}/mod-data`),
    place(
      "recordings",
      "Diagnostics recordings",
      "Gameplay recordings and their reports.",
      "yours",
      `${HOME}/perf`,
    ),
  ];

  if (game && version) {
    const data = `/home/you/OpenRAC/active/${game}/data`;

    out.push(
      place(
        "texture-packs",
        "Texture packs",
        "This game's texture packs.",
        "game",
        `${data}/texture_packs/${game}-${version}`,
        1,
      ),
      place(
        "saved-textures",
        "Saved textures",
        "Textures saved to make a pack.",
        "game",
        `${HOME}/textures/${game}-${version}`,
      ),
      place("mods", "Mods", "This game's mods, a folder each.", "game", `${data}/plugins`, 4),
      place(
        "exports",
        "Exports",
        "Models, textures and sounds you exported.",
        "game",
        `${data}/exports`,
      ),
      place(
        "overrides",
        "Overrides",
        "Replaced files the game reads over the disc's.",
        "game",
        `${data}/overrides`,
      ),
      place(
        "logs",
        "Logs and crash reports",
        "A log each time you play, crash reports in crashes.",
        "game",
        data,
        9,
      ),
    );
    if (developer)
      out.push(
        place(
          "editor-projects",
          "Editor projects",
          "The level editor's projects.",
          "developer",
          `${data}/levels/${version}`,
        ),
        place(
          "packed-levels",
          "Packed levels",
          "Levels packed from the editor.",
          "developer",
          `${data}/packed/${version}`,
        ),
      );
  }

  return out;
}

const SHORTCUTS: [string, string, string, string][] = [
  ["screenshot", "Screenshot", "Game", "F12"],
  ["full_screen", "Full screen", "Game", "F11,Alt+Enter"],
  ["volume_down", "Volume down", "Game", "F9"],
  ["volume_up", "Volume up", "Game", "F10"],
  ["frame_counter", "Frame counter", "Game", "F2"],
  ["enhanced_graphics", "Enhanced graphics", "Game", "F4"],
  ["texture_pack", "Show or hide the texture pack", "Game", "Ctrl+F2"],
  ["stats_panel", "Stats panel", "Developer", "F3"],
  ["record_diagnostics", "Record diagnostics", "Developer", "Ctrl+F3"],
  ["pause", "Pause and resume", "Developer", "F5"],
  ["step_tick", "Step one tick", "Developer", "F6"],
  ["slower", "Slower", "Developer", "F7"],
  ["faster", "Faster", "Developer", "F8"],
  ["level_select", "Level select", "Developer", "F1"],
];

const bound = new Map<string, string>();

export function mockShortcuts(): ShortcutRow[] {
  return SHORTCUTS.map(([id, name, section, defaults]) => ({
    id,
    name,
    section,
    keys: bound.get(id) ?? defaults,
    defaults,
  }));
}

export function mockSetShortcut(id: string, keys: string | null): ShortcutRow[] {
  if (!SHORTCUTS.some(([known]) => known === id)) throw new Error(`${id} is not a shortcut`);
  if (keys === null) bound.delete(id);
  else bound.set(id, keys);

  return mockShortcuts();
}

export function mockResetShortcuts(): ShortcutRow[] {
  bound.clear();

  return mockShortcuts();
}
