// What a page read from the launcher last, kept while the launcher runs. A page is built again
// each time its tab opens: it shows what it read the last time at once, and reads it again behind
// that. Without it, Mods came in empty and its lists, buttons and notices popped in during the
// tab's slide, and Games' "Continue" and last save came a moment after the shelf: both showed as
// buttons flickering.

const kept = new Map<string, unknown>();

/** The value last kept under `key`, or `fallback` when none was. */
export function lastRead<T>(key: string, fallback: T): T {
  return kept.has(key) ? (kept.get(key) as T) : fallback;
}

/** Whether something was kept under `key`. */
export function hasRead(key: string): boolean {
  return kept.has(key);
}

/** Keeps `value` under `key` for the next time the page opens. */
export function keepRead(key: string, value: unknown): void {
  kept.set(key, value);
}
