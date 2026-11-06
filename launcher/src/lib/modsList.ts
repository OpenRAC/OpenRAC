// My mods' lists, as rows: their groups, the search, the order and a move. No Svelte or Tauri
// here, so it can be tested; the toolbar's state is lib/modsView.svelte.ts.

/** The kinds of thing My mods holds. */
export type ModGroup = "gameplay" | "looks" | "tools";
export type ModFilter = "all" | ModGroup;
/** Load order (the order the game reads them, which can be changed) or by name. */
export type ModSort = "order" | "name";

/** The group a gameplay mod's kind goes in: a mod that is only a page in the launcher is a tool,
 *  Enhanced graphics changes the look, the rest runs in the game. */
export function groupOf(kind: string): ModGroup {
  if (kind === "tool") return "tools";
  if (kind === "built-in") return "looks";
  return "gameplay";
}

/** Whether a row's words hold every word of the search, in any case and order. */
export function matches(query: string, ...words: (string | null | undefined)[]): boolean {
  const text = words.filter(Boolean).join(" ").toLowerCase();
  return query
    .toLowerCase()
    .split(/\s+/)
    .filter(Boolean)
    .every((w) => text.includes(w));
}

/** The rows as shown: those the search finds, in load order or by name. */
export function arrange<T>(rows: T[], query: string, sort: ModSort, words: (row: T) => string[]): T[] {
  const found = rows.filter((r) => matches(query, ...words(r)));
  if (sort === "name") found.sort((a, b) => (words(a)[0] ?? "").localeCompare(words(b)[0] ?? ""));
  return found;
}

/** The steps a row takes to go from FROM to TO, one place at a time: true is up. */
export function steps(from: number, to: number): boolean[] {
  return Array.from({ length: Math.abs(to - from) }, () => to < from);
}

/** The toolbar shows from this many rows on: with fewer, the lists are short enough as they are. */
export const TOOLBAR_FROM = 6;
