// "Share my setup": the mods that are on, as a link to openrac.dev's set page, which lists them and
// offers one install link with these exact versions (the contract's Share-my-setup link). Only mods
// the index knows are shared: a mod from a private link could not be installed by a friend.
import type { PluginView } from "./api";

export const SET_URL = "https://openrac.dev/mods/set";
const MAX = 32;

/** The set page's address for the mods that are on and in the index; null when there are none. */
export function shareLink(
  mods: PluginView[],
  inIndex: (id: string) => boolean,
  game: string,
  disc: string,
): string | null {
  const shared = mods
    .filter((m) => m.enabled && m.kind !== "built-in" && inIndex(m.id) && /^\d+\.\d+\.\d+(-[\w.]+)?$/.test(m.version))
    .slice(0, MAX)
    .map((m) => `${m.id}@${m.version}`);
  if (shared.length === 0) return null;
  // Ids and versions are [a-z0-9.-]: nothing in them needs encoding, so the link reads plainly.
  return `${SET_URL}?m=${shared.join(",")}&g=${encodeURIComponent(game)}&d=${encodeURIComponent(disc)}`;
}
