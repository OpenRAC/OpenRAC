// The print of each game's real disc, for Android's Games tab (DiscShelf.svelte) and the desktop's
// game list (SideList.svelte): a scan from the LaunchBox Games Database, as Cartouche finds it
// (github.com/Hakimba/cartouche, app/data/art/ps2.tsv), shipped in public/img/discs/ (user
// decision; scripts/disc_prints.py makes them again). A print that fails to load shows the label.

import { storeArtwork } from "./media";

/** The regions each game has a print of: the North American disc (ntsc), the European (pal) and the
 *  Japanese (jp). Secret Agent Clank had no PS2 release in Asia. */
const PRINTS: Record<string, string[]> = {
  rac1: ["ntsc", "pal", "jp"],
  rac2: ["ntsc", "pal", "jp"],
  rac3: ["ntsc", "pal", "jp"],
  rac4: ["ntsc", "pal", "jp"],
  rac5: ["ntsc", "pal", "jp"],
  rac6: ["ntsc", "pal"],
};

/** The print region of a version of game.json, by its name (`pal1`: pal, `jp`: jp, `ntsc2`: ntsc).
 *  Korean discs show the Japanese print (user decision: no scan of one exists). */
function region(version: string): string {
  if (version.startsWith("pal")) return "pal";
  if (["jp", "ntsc-j", "kr", "ko", "ntsc-k"].includes(version)) return "jp";
  return "ntsc";
}

/** The print of `game`'s disc in `version` (a version of its game.json), or null. A region without a
 *  print shows the North American disc. */
export function discPrint(game: string, version: string): string | null {
  const regions = PRINTS[game];
  // A store build shows its own drawings, never the games' art: the label then.
  if (!regions || storeArtwork()) return null;
  const want = region(version);
  return `/img/discs/${game}-${regions.includes(want) ? want : "ntsc"}.webp`;
}
