// What changed, as Android's Games tab lists it under the shelf: an update of the launcher, mods
// with updates, a game that crashed. No Svelte or Tauri here, so it can be tested.

export interface NewsItem {
  key: string;
  kind: "update" | "mods" | "crash";
  /** The game it is about (for mods and a crash). */
  game: string | null;
  /** The version's key (`rac1/pal`) it opens, for mods and a crash. */
  version: string | null;
  title: string;
  text: string;
}

export interface NewsInput {
  /** The launcher's update waiting, if any. */
  update: { version: string } | null;
  /** Mods with updates, per game (modUpdates.found). */
  mods: { game: string; version: string; title: string; updates: unknown[] }[];
  /** Each game, with the version the player plays and how many crashes are new. */
  games: { id: string; title: string; version: string | null; crashes: number }[];
}

/** The news, in the order shown: the launcher's update, mods, then crashes. */
export function news({ update, mods, games }: NewsInput): NewsItem[] {
  const list: NewsItem[] = [];
  if (update) {
    list.push({
      key: "update",
      kind: "update",
      game: null,
      version: null,
      title: `OpenRAC ${update.version} is ready`,
      text: "Update the launcher",
    });
  }
  for (const offer of mods) {
    const n = offer.updates.length;
    if (n === 0) continue;
    list.push({
      key: `mods-${offer.game}`,
      kind: "mods",
      game: offer.game,
      version: `${offer.game}/${offer.version}`,
      title: `${n} mod update${n === 1 ? "" : "s"}`,
      text: offer.title,
    });
  }
  for (const g of games) {
    if (!g.version || g.crashes <= 0) continue;
    list.push({
      key: `crash-${g.id}`,
      kind: "crash",
      game: g.id,
      version: g.version,
      title: `${g.title} crashed`,
      text: "A report is on its page",
    });
  }
  return list;
}
