// The print of each game's real disc, for Android's Games tab (DiscShelf.svelte): a scan from the
// LaunchBox Games Database, as Cartouche finds it (github.com/Hakimba/cartouche, app/data/art/ps2.tsv).
// The scans are their owners': the launcher never carries them, the device downloads one when it is
// first shown and its webview keeps it. Without a connection the disc wears the launcher's own label.

const LAUNCHBOX = "https://images.launchbox-app.com/";

/** The scan of each game's disc, by version: the North American disc for NTSC-U, the European for PAL. */
const PRINTS: Record<string, { ntsc: string; pal: string }> = {
  rac1: { ntsc: "03c0c663-f1f1-4fbb-adb1-91a44d755eff", pal: "c1b455c3-82e0-4b55-8e3e-56cc508c6f78" },
  rac2: { ntsc: "2b79e872-135c-47a4-a5fa-191483b7a1cf", pal: "8cf4d363-4040-44b8-8412-dd2058a80c30" },
  rac3: { ntsc: "0a3fd622-89ee-44cb-97ff-4167dc56ef76", pal: "46d42ac4-b987-43b1-b185-b3ecfc802989" },
  rac4: { ntsc: "d07c9d1f-0966-46ae-9b51-2c6404e32d82", pal: "0e7e6169-ad74-41df-b37a-2a9730acc340" },
  rac5: { ntsc: "b21c6132-f24a-4f4b-afbe-8ca153aff245", pal: "0095ef61-7cca-432a-963b-4ada2e7bc04e" },
  rac6: { ntsc: "e596a445-b75c-4b48-acd6-3097eafff420", pal: "0be486f6-8286-44a6-8d05-3694a86a7ba7" },
};

/** Whether prints are fetched at all: in the app, yes; in the browser preview only with ?discs, so
 *  that the tests never wait on the network. */
function fetching(): boolean {
  if (typeof window === "undefined") return false;
  if ("__TAURI_INTERNALS__" in window) return true;
  return new URLSearchParams(location.search).has("discs");
}

/** The URL of the print of `game`'s disc in region `version` (`pal`, `ntsc`, `ntsc-j`), or null. */
export function discPrint(game: string, version: string): string | null {
  const prints = PRINTS[game];
  if (!prints || !fetching()) return null;
  return LAUNCHBOX + (version === "pal" ? prints.pal : prints.ntsc) + ".png";
}
