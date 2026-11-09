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
function storeArt(game: string): string | null {
  const host = typeof window === "undefined" ? null : (window as Window & { __OPENRAC_STORE_ARTWORK__?: boolean });
  return host?.__OPENRAC_STORE_ARTWORK__ && Object.hasOwn(GAME_ART, game) ? `/img/store/${game}.svg` : null;
}

/** A game's thumbnail, or null. */
export function gameThumb(game: string): string | null {
  return storeArt(game) ?? GAME_THUMB[game] ?? null;
}

/** A game's picture, or null for a game without one (its tile then shows its colours). */
export function gameArt(game: string): string | null {
  return storeArt(game) ?? GAME_ART[game] ?? null;
}

/** A game's picture as Android's Games tab shows it: framed for a tile, 1280 wide, with the game's
 *  logo top left (scripts/game_art.py). */
export function gameTileArt(game: string): string | null {
  return storeArt(game) ?? GAME_ART[game]?.replace(/-bg\.webp$/, "-tile.webp") ?? null;
}

/** Whether the game's tile picture carries its logo, so the tile need not write its name over it.
 *  The store bundles' illustrations carry none. */
export function tileHasLogo(game: string): boolean {
  return !storeArt(game) && Object.hasOwn(GAME_ART, game);
}

// The tiles' pictures, decoded once and kept (Android's Games tab): a tile that mounts again finds
// its picture ready, rather than showing its colours for a frame first.
const kept: HTMLImageElement[] = [];

/** Decodes every game's tile picture ahead of its first showing; once. */
export function preloadArt(): void {
  if (kept.length || typeof Image === "undefined") return;
  for (const src of Object.keys(GAME_ART).map(gameTileArt)) {
    if (!src) continue;
    const img = new Image();
    img.src = src;
    img.decode().catch(() => undefined); // a picture that is not there stays out
    kept.push(img);
  }
}

/** For an <img onerror>: a picture that could not be loaded is hidden, so the colours show. */
export function hideMissing(e: Event): void {
  (e.currentTarget as HTMLElement).style.display = "none";
}
