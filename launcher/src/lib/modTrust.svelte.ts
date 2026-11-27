// What the mod list says about each installed mod's checks (core/src/index.rs `trust`, `owner`,
// `held`, a version's `scan` and `built`), read once a session from the signed index for My mods'
// labels and a mod page's Safety part. A mod not in the list (from a link to a repository, or
// built by hand) has none.
import { api, type IndexMod } from "./api";

export const modTrust = $state<{ byId: Record<string, IndexMod>; read: boolean }>({
  byId: {},
  read: false,
});

let reading: Promise<void> | null = null;

/** Reads the list once (its cached copy when offline); errors leave the labels out. */
export function loadTrust(): Promise<void> {
  reading ??= api
    .indexFetch()
    .then((view) => {
      modTrust.byId = Object.fromEntries(view.index.mods.map((m) => [m.id, m]));
    })
    .catch(() => undefined)
    .finally(() => {
      modTrust.read = true;
    });

  return reading;
}

/** A mod's label: "openrac", "verified", "trusted", "unchecked", or "" when the list has none. */
export const trustOf = (id: string): string => modTrust.byId[id]?.trust ?? "";

/** Whether the "Checked by OpenRAC only" filter hides a row: only Not checked ones go. */
export const hiddenByCheck = (id: string, checkedOnly: boolean): boolean =>
  checkedOnly && trustOf(id) === "unchecked";
