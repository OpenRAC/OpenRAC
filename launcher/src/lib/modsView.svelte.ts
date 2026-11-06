// How My mods shows its lists when there are many (ModsToolbar.svelte): a search, a kind to show
// and an order. The gameplay mods (PluginsPanel) and the packs (PacksPanel) both read it, and each
// reports what it holds (how many of each kind, how many are on, how many the search finds), so the
// toolbar can count them, say when the search finds nothing and say when Play starts without them.

import type { ModFilter, ModGroup, ModSort } from "./modsList";

/** What one list holds, as it reported it. */
export interface ListReport {
  /** Its rows of each group. */
  counts: Partial<Record<ModGroup, number>>;
  /** Its rows switched on. */
  on: number;
  /** Its rows the search and the kind picked show. */
  found: number;
}

export const modsView = $state<{
  query: string;
  filter: ModFilter;
  sort: ModSort;
  /** Each list's report, by list. */
  lists: Record<string, ListReport>;
}>({ query: "", filter: "all", sort: "order", lists: {} });

/** A list says what it holds. */
export function report(list: string, what: ListReport): void {
  modsView.lists[list] = what;
}

/** A list is gone (another game, another page): its rows are no longer counted. */
export function forget(list: string): void {
  modsView.lists = Object.fromEntries(Object.entries(modsView.lists).filter(([name]) => name !== list));
}

/** All lists' rows of a group, or of every group. */
export function total(group: ModFilter = "all"): number {
  let n = 0;
  for (const { counts } of Object.values(modsView.lists)) {
    for (const [g, c] of Object.entries(counts)) if (group === "all" || g === group) n += c;
  }
  return n;
}

/** All lists' rows switched on. */
export const totalOn = () => Object.values(modsView.lists).reduce((n, l) => n + l.on, 0);

/** All lists' rows the search and the kind show. */
export const totalFound = () => Object.values(modsView.lists).reduce((n, l) => n + l.found, 0);

/** Whether a group's rows show with the kind picked. */
export const shows = (group: ModGroup) => modsView.filter === "all" || modsView.filter === group;

/** Whether something is searched. */
export const searching = () => modsView.query.trim() !== "";

/** Whether rows can be moved: in load order, with nothing searched. */
export const movable = () => modsView.sort === "order" && !searching();

/** Back to everything: another game, or the page opened again. The lists' reports stay: the lists
 *  shown say again what they hold, and those that go forget theirs. */
export function resetView(): void {
  modsView.query = "";
  modsView.filter = "all";
  modsView.sort = "order";
}
