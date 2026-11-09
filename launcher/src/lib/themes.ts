// Each game's card gradient and typeface, from the OpenRAC website's game cards
// (openrac-site src/lib/projects.ts, `theme` and `fontClass`). The site
// names the games by their own ids; OpenRAC by release order:
//   rac1 = "rac1", rac2 = "gc", rac3 = "uya", rac4 = "deadlocked", rac5 = "sm", rac6 = "sac".
// Keep the values in step with the site (docs/STYLE.md).
import palettes from "./palettes.json";

export interface Theme {
  /** The card's gradient, top to bottom. */
  from: string;
  to: string;
  /** The title's typeface: the site's Audiowide for RAC1, Orbitron for the others. */
  font: "brand" | "head";
  weight: number;
}

export const THEMES: Record<string, Theme> = {
  rac1: {
    from: "#3c7a3a",
    to: "#12331f",
    font: "brand",
    weight: 400,
  },
  rac2: {
    from: "#7a4d1a",
    to: "#2a1a0c",
    font: "head",
    weight: 600,
  },
  rac3: {
    from: "#25514f",
    to: "#0d1f24",
    font: "head",
    weight: 600,
  },
  rac4: {
    from: "#7f1d1d",
    to: "#0f1115",
    font: "head",
    weight: 700,
  },
  rac5: {
    from: "#223342",
    to: "#131d26",
    font: "head",
    weight: 600,
  },
  rac6: {
    from: "#0369a1",
    to: "#082f49",
    font: "head",
    weight: 600,
  },
};

/** For a game the site has no card for yet: the launcher's own colours. */
export const FALLBACK: Theme = {
  from: "#2c2a26",
  to: "#1c1a16",
  font: "head",
  weight: 600,
};

export function theme(game: string): Theme {
  return THEMES[game] ?? FALLBACK;
}

/** The theme as CSS custom properties, for a style attribute (the site's names: --box, --bd, --tx,
 *  --tx2). The title's box and text wear the launcher's own colours for every game (user decision:
 *  one look; a game keeps only its picture, card gradient, typeface and the light under the bar). */
export function themeStyle(game: string): string {
  const t = theme(game);
  const font = { brand: "var(--font-brand)", head: "var(--font-head)" }[t.font];
  return [
    "--box: color-mix(in srgb, var(--panel) 92%, transparent)",
    "--bd: var(--amber)",
    "--tx: var(--gold)",
    "--tx2: var(--amber)",
    `--from: ${t.from}`,
    `--to: ${t.to}`,
    `--title-font: ${font}`,
    `--title-weight: ${t.weight}`,
  ].join("; ");
}

/** The picked game's accent, from palettes.json: only the light under the bar (--glow) in that game's
 *  colour, Going Commando's teal, Up Your Arsenal's yellow, Deadlocked's red, Size Matters' blue.
 *  With Game colours off every game wears the launcher's own gold and coal; Ratchet & Clank's light
 *  is the launcher's own (app.css). */
export const ACCENTS: Record<string, Record<string, string>> = palettes.accents;

/** A game's own colours, worn instead of the launcher's while Settings → General → Game colours is on
 *  (the default): its bar, its gold and, on the launcher's own theme, its coal (the --tint-* tokens,
 *  app.css). */
export const GAME_COLORS: Record<string, Record<string, string>> = palettes.game_colors;

/** The custom properties a game's accent sets on the page, with `colors` its own colours too; none
 *  for the launcher's own (rac1, or no game). */
export function accentProperties(game: string | null, colors = false): [string, string][] {
  const a = game ? ACCENTS[game] : undefined;
  const c = game && colors ? GAME_COLORS[game] : undefined;
  return [...Object.entries(a ?? {}), ...Object.entries(c ?? {})];
}

/** The custom properties accentProperties may set, to take them all off again. */
export const ACCENT_PROPERTIES = [
  ...new Set([...Object.values(ACCENTS), ...Object.values(GAME_COLORS)].flatMap((a) => Object.keys(a))),
];

/** Where the accent's game, and whether it wears its own colours, are kept for the first paint,
 *  before the settings have loaded. */
const STORED_ACCENT = "openrac-accent";
const STORED_COLORS = "openrac-game-colors";

/** Puts game `game`'s accent on the page (with `colors` its own colours too) and keeps it for the
 *  next start, so the launcher opens in it rather than easing into it from the launcher's own
 *  (app.css). */
export function applyAccent(game: string | null, colors = false, root: HTMLElement = document.documentElement) {
  for (const name of ACCENT_PROPERTIES) root.style.removeProperty(name);
  for (const [name, value] of accentProperties(game, colors)) root.style.setProperty(name, value);
  if (colors) root.dataset.gameColors = "";
  else delete root.dataset.gameColors;
  try {
    if (game) localStorage.setItem(STORED_ACCENT, game);
    else localStorage.removeItem(STORED_ACCENT);
    localStorage.setItem(STORED_COLORS, colors ? "1" : "0");
  } catch {
    // Storage refused (a private window): the accent then comes with the settings.
  }
}

/** Whether the last start wore the game's own colours: yes unless it had them off (the default). */
export function storedGameColors(): boolean {
  try {
    return localStorage.getItem(STORED_COLORS) !== "0";
  } catch {
    return true;
  }
}

/** The game whose accent the last start showed, if any. */
export function storedAccent(): string | null {
  try {
    return localStorage.getItem(STORED_ACCENT);
  } catch {
    return null;
  }
}
