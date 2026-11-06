// What a game's page offers to start the game without the launcher's window. Steam's own entry
// works where Steam runs (Linux, Windows; macOS and Android are left out); the `.desktop`
// shortcut is Linux's.
import type { Platform } from "./generated/Platform";

export function shortcutOptions(platform: Platform | undefined): { steam: boolean; desktop: boolean } {
  return { steam: platform === "linux" || platform === "windows", desktop: platform === "linux" };
}
