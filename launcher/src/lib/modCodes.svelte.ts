// The mod code being reviewed, wherever it came from: the Mods page's field, a paste, or a link
// from outside (openrac://modset/CODE). ModSetReview.svelte, at the launcher's top, shows it.
import { api, onModsetLink, type VersionView } from "./api";
import { app, toast } from "./app.svelte";
import { playerVersion } from "./player.svelte";
import { MOD_CODES as T } from "./modsText";

export const modCode = $state<{
  /** The code to review, and the disc it is reviewed for. */
  open: { text: string; version: VersionView } | null;
  /** Bumped when the mods changed: the lists are read again. */
  revision: number;
  /** The review is applying a code or shows one applied: a new code waits for it to close. */
  busy: boolean;
  /** The code that came while busy (the last one), opened when the review closes. */
  queued: { text: string; version: VersionView } | null;
}>({ open: null, revision: 0, busy: false, queued: null });

/** The disc played of `game`, or of the game last opened, or of any game with mods. */
export function versionFor(game?: string): VersionView | null {
  const games = app.library?.games ?? [];
  const last = app.version?.split("/")[0];
  const wanted = game ? [games.find((g) => g.id === game)] : [games.find((g) => g.id === last), ...games];
  for (const g of wanted) {
    const v = g ? playerVersion(g) : null;
    if (v) return v;
  }
  return null;
}

/** Opens a code's review. Without a version, the disc played. */
export function openCode(text: string, version?: VersionView | null) {
  const v = version ?? versionFor();
  if (!v) {
    toast(T.noGame, "error");
    return;
  }
  if (modCode.open && modCode.busy) {
    modCode.queued = { text, version: v };
    toast(T.queued, "info");
    return;
  }
  modCode.open = { text, version: v };
}

export function closeCode() {
  modCode.open = modCode.queued;
  modCode.queued = null;
  modCode.busy = false;
}

/** The mods changed. */
export function modsChanged() {
  modCode.revision += 1;
}

/** Opens the link waiting, if one is. */
export async function takeCodeLink() {
  const link = await api.modsetTakeLink();
  if (link) openCode(link);
}

/** Links from outside: the one that started the launcher, then any that come. */
export async function followCodeLinks(): Promise<() => void> {
  const stop = await onModsetLink(() => void takeCodeLink());
  void takeCodeLink();
  return stop;
}
