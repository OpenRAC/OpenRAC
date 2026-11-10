// The print of each game's real disc, for Android's Games tab (DiscShelf.svelte) and the desktop's
// game list (SideList.svelte): a scan from the LaunchBox Games Database, as Cartouche finds it
// (github.com/Hakimba/cartouche, app/data/art/ps2.tsv), shipped in public/img/discs/ (user
// decision; scripts/disc_prints.py makes them again). A print that fails to load shows the label.

import { storeArtwork } from "./media";

/** The games with prints: the North American disc for NTSC-U, the European for PAL. */
const PRINTS = new Set(["rac1", "rac2", "rac3", "rac4", "rac5", "rac6"]);

/** The print of `game`'s disc in region `version` (`pal`, `ntsc`, `ntsc-j`), or null. */
export function discPrint(game: string, version: string): string | null {
  // A store build shows its own drawings, never the games' art: the label then.
  if (!PRINTS.has(game) || storeArtwork()) return null;
  return `/img/discs/${game}-${version === "pal" ? "pal" : "ntsc"}.webp`;
}
