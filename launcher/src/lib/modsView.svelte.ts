// How My mods shows its lists when there are many (ModsToolbar.svelte): a search, a kind to show
// and an order. The gameplay mods (PluginsPanel) and the packs (PacksPanel) both read it, and each
// says how many it has of each kind, so the toolbar can count them. The words are lib/modsText.ts's.

import type { ModFilter, ModGroup, ModSort } from "./modsList";

export const modsView = $state<{
  query: string;
  filter: ModFilter;
  sort: ModSort;
  /** How many rows each list has of each group, by list. */
  counts: Record<string, Partial<Record<ModGroup, number>>>;
}>({ query: "", filter: "all", sort: "order", counts: {} });

/** A list says how many rows it has of each group. */
export function report(list: string, counts: Partial<Record<ModGroup, number>>): void {
  modsView.counts[list] = counts;
}

/** All lists' rows of a group, or of every group. */
export function total(group: ModFilter = "all"): number {
  let n = 0;
  for (const counts of Object.values(modsView.counts)) {
    for (const [g, c] of Object.entries(counts)) if (group === "all" || g === group) n += c;
  }
  return n;
}

/** Whether a group's rows show with the kind picked. */
export const shows = (group: ModGroup) => modsView.filter === "all" || modsView.filter === group;

/** Whether rows can be moved: in load order, with nothing searched. */
export const movable = () => modsView.sort === "order" && modsView.query.trim() === "";

/** Back to everything (another game). */
export function resetView(): void {
  modsView.query = "";
  modsView.filter = "all";
  modsView.counts = {};
}
