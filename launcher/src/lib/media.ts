// Each game's picture (public/img), under OpenRAC's game ids, with the site's file names: under
// its page's band and faintly behind the page. Where these come from is in docs/STYLE.md.
// A picture that is not there is left out where it is shown, and the game's colours carry it.
const GAME_ART: Record<string, string> = {
  rac1: "/img/rac1-bg.webp",
  rac2: "/img/gc-bg.webp",
  rac3: "/img/uya-bg.webp",
  rac4: "/img/deadlocked-bg.webp",
  rac5: "/img/size-matters-bg.webp",
  rac6: "/img/sac-bg.webp",
};

// Each game's thumbnail: its picture, small, for the game list (scripts/media.py thumbs makes
// them). The list shows six at 64 by 44; six full pictures would decode tens of MB for that.
const GAME_THUMB: Record<string, string> = {
  rac1: "/img/rac1-thumb.webp",
  rac2: "/img/gc-thumb.webp",
  rac3: "/img/uya-thumb.webp",
  rac4: "/img/deadlocked-thumb.webp",
  rac5: "/img/size-matters-thumb.webp",
  rac6: "/img/sac-thumb.webp",
};

/** Store bundles use original OpenRAC illustrations; desktop/dev keeps its existing artwork. */
/** Whether the build shows the store's own drawings in place of the games' art. */
export function storeArtwork(): boolean {
  const host = typeof window === "undefined" ? null : (window as Window & { __OPENRAC_STORE_ARTWORK__?: boolean });
  return !!host?.__OPENRAC_STORE_ARTWORK__;
}

function storeArt(game: string): string | null {
  return storeArtwork() && Object.hasOwn(GAME_ART, game) ? `/img/store/${game}.svg` : null;
}

/** A game's thumbnail, or null. */
export function gameThumb(game: string): string | null {
  return storeArt(game) ?? GAME_THUMB[game] ?? null;
}

/** A game's picture, or null for a game without one (its colours show instead). */
export function gameArt(game: string): string | null {
  return storeArt(game) ?? GAME_ART[game] ?? null;
}

/** A game's picture 1280 wide (scripts/game_art.py): a disc's label until its print is there. */
export function gameTileArt(game: string): string | null {
  return storeArt(game) ?? GAME_ART[game]?.replace(/-bg\.webp$/, "-tile.webp") ?? null;
}

// Disc pictures (labels and prints), loaded and decoded once and kept: a disc that mounts again (a
// tab changed, the wheel turned) draws at once rather than blank for a frame. Android's WebView
// fetched them again from the app on every visit to Games (the app's files are not cached), and the
// tab's slide waited for each decode (motion.ts): about 170 ms of a still screen per visit. Each is
// kept as an in-memory copy (a blob: URL) that shows from then on.
const kept = new Map<string, { img: HTMLImageElement; url: string | null }>();

/** Loads and decodes `src` once and keeps it. */
export function keepDecoded(src: string): void {
  if (kept.has(src) || typeof Image === "undefined") return;
  const entry: { img: HTMLImageElement; url: string | null } = { img: new Image(), url: null };
  kept.set(src, entry);
  const decode = (url: string) => {
    entry.img.src = url;
    return entry.img.decode();
  };
  const copy =
    typeof fetch === "function" && typeof URL.createObjectURL === "function"
      ? fetch(src)
          .then((r) => (r.ok ? r.blob() : Promise.reject(new Error(String(r.status)))))
          .then((blob) => {
            const url = URL.createObjectURL(blob);
            return decode(url).then(() => {
              entry.url = url;
            });
          })
      : Promise.reject(new Error("no fetch"));
  // Without a copy, the file itself; a picture that is not there stays out.
  copy.catch(() => decode(src)).catch(() => undefined);
}

/** Where to show `src` from: its kept copy once it is loaded and decoded, else the file. */
export function keptSrc(src: string): string {
  return kept.get(src)?.url ?? src;
}

/** Whether `url` is a kept copy, decoded already: nothing to wait for. */
export function isKeptCopy(url: string): boolean {
  return url.startsWith("blob:") && [...kept.values()].some((k) => k.url === url);
}

/** Decodes every game's label picture ahead of its first showing. */
export function preloadArt(): void {
  for (const src of Object.keys(GAME_ART).map(gameTileArt)) if (src) keepDecoded(src);
}

/** For an <img onerror>: a picture that could not be loaded is hidden, so the colours show. */
export function hideMissing(e: Event): void {
  (e.currentTarget as HTMLElement).style.display = "none";
}
