// The once-a-day look for newer versions of the installed mods (core/src/remote.rs's
// check_updates). It runs when the launcher starts, so a player is offered the updates without
// opening the Mods tab, and the Mods tab reads the same answers rather than asking again.
//
// When a game was last checked is kept in this browser's storage, one key per game, so the look
// happens once a day however often the launcher is opened. A private window, where storage
// throws, is checked once per start and no more.
import { api, type ModUpdate, type VersionView } from "$lib/api";
import { playerVersion } from "$lib/player.svelte";
import { app } from "$lib/app.svelte";

const DAY = 24 * 60 * 60 * 1000;

const checkedKey = (game: string) => `openrac.mods.checked.${game}`;

/** When this game's mods were last looked at, 0 for never (or storage the page cannot read). */
export function lastChecked(game: string): number {
  try {
    return Number(localStorage.getItem(checkedKey(game)) ?? 0) || 0;
  } catch {
    return 0;
  }
}

export function markChecked(game: string, when = Date.now()) {
  try {
    localStorage.setItem(checkedKey(game), String(when));
  } catch {
    // Private windows: looked at again next time.
  }
}

export const checkDue = (game: string, now = Date.now()): boolean => now - lastChecked(game) > DAY;

/** The updates one game's look found, for the prompt and for the Mods tab. */
export interface ModOffer {
  game: string;
  /** The version whose mods they are (`pal`): what plugins_update needs. */
  version: string;
  /** The game's title, for the prompt's line. */
  title: string;
  updates: ModUpdate[];
}

/** What the last look found, by game: the Mods tab shows these instead of looking again. */
export const modUpdates = $state<{ found: Record<string, ModOffer>; offer: ModOffer | null }>({
  found: {},
  offer: null,
});

/** The versions whose gameplay mods can be updated: one per game, the one a player plays. */
function versionsToCheck(): VersionView[] {
  const found: VersionView[] = [];
  for (const game of app.library?.games ?? []) {
    const v = playerVersion(game);
    const play = v?.actions.find((a) => a.id === "play");
    if (v && play && play.state !== "planned") found.push(v);
  }
  return found;
}

/**
 * Looks for newer versions of the mods of every game that has one from a link and has not been
 * looked at today, and puts what it found in `modUpdates`. `offer` is set to the first game with
 * something to update, which is what the prompt shows. Nothing is installed here.
 */
export async function lookForModUpdates(): Promise<void> {
  for (const v of versionsToCheck()) {
    if (!checkDue(v.game)) continue;
    const sources = await api.pluginsSources(v.game).catch(() => null);
    if (!sources || Object.keys(sources.mods).length === 0) continue;
    const found = await api.pluginsCheckUpdates(v.game, v.name).catch(() => null);
    markChecked(v.game);
    if (!found) continue;
    const updates = found.updates.filter((u) => !sources.mods[u.id]?.pin);
    if (updates.length === 0) continue;
    const offer: ModOffer = { game: v.game, version: v.name, title: v.title, updates };
    modUpdates.found[v.game] = offer;
    modUpdates.offer ??= offer;
  }
}

/** The prompt was answered: it does not come back this start. */
export function dismissModUpdates() {
  modUpdates.offer = null;
}

/** A game's mods were updated: nothing is left to offer for it, here or on the Mods tab. */
export function forgetModUpdates(game: string) {
  modUpdates.found = Object.fromEntries(Object.entries(modUpdates.found).filter(([id]) => id !== game));
  if (modUpdates.offer?.game === game) modUpdates.offer = null;
}
