// What a game offers beside its Play button: the "..." menu of what is done less often, and the
// Mods switch (config.playWithMods) with how many mods it brings. The game's page shows them on the
// desktop; on Android they sit beside the disc on the Games tab (DiscShelf.svelte) instead.
import type { MenuItem } from "$components/Menu.svelte";
import { api, ask, onAndroid, pickMemoryCard, type GameView, type VersionView } from "./api";
import { app, gameWindow, guard, openVersion, run, saveConfig, toast, uninstallGame } from "./app.svelte";
import { supports } from "./capabilities";
import { isBuilt, rebuild, rebuildSteps, type PlayerState } from "./player.svelte";

/** Asked of the game's page as it opens: unfold Help with its logs (the menu's way from Games). */
export const pageAsks = $state({ logs: false });

async function removeSetUp(v: VersionView) {
  const sure = await ask(
    `Remove what was set up from your ${v.title} disc? Your disc image and your saves are kept; set it up again any time.`,
    "Remove",
  );
  if (sure) await uninstallGame(v.game);
}

/** The game's saves from a PCSX2 memory card onto OpenRAC's card, after a backup of it. */
async function importPcsx2(serial: string) {
  const dir = await api.pcsx2MemcardsDir().catch(() => null);
  const source = await pickMemoryCard("Choose a PCSX2 memory card", dir);
  if (!source) return;
  const yes = await ask(
    "Copy this game's saves from the PCSX2 memory card? The saves on OpenRAC's card are backed up first, then replaced.",
    "Import saves",
  );
  if (!yes) return;
  try {
    const done = await api.importPcsx2Saves(serial, source);
    app.savesChanged++; // the saves list reads the card again
    const kept = done.backup ? " The saves that were there are in Backups." : "";
    toast(`Imported the saves from PCSX2.${kept}`, "info");
  } catch (e) {
    toast(`Import failed: ${String(e)}`, "error");
  }
}

/** The game's "..." menu. `showLogs` unfolds its logs; left out, the game's page opens on them. */
export function gameMenu(
  game: GameView,
  v: VersionView,
  next: PlayerState | null,
  showLogs: () => void = () => {
    pageAsks.logs = true;
    openVersion(v.key);
  },
): MenuItem[] {
  const action = (id: string) => v.actions.find((a) => a.id === id && a.thisPlatform) ?? null;
  const scope = { kind: "version" as const, key: v.key };
  const setUp = !!game.install?.extracted;
  const playGodot = action("editor-preview");
  const extract = action("editor-extract");
  const items: MenuItem[] = [];
  if (next?.kind === "return") {
    items.push({ label: "Stop the game", icon: "x", danger: true, run: () => void gameWindow(true) });
  }
  // From scratch: everything built before is deleted first (tools/build_port.py --clean); on
  // Android, the levels are read from the disc again (rebuildSteps).
  if (isBuilt(game, v) && (next?.kind === "play" || next?.kind === "levels") && rebuildSteps(v)) {
    items.push({ label: "Rebuild the game", icon: "build", run: () => void rebuild(v) });
  }
  if (playGodot?.runnable) {
    items.push({ label: "Preview a level in Godot", icon: "play", run: () => void run(scope, playGodot) });
  }
  if (extract && extract.state !== "planned" && !onAndroid && supports(app.info, "mods")) {
    // Android has no Godot: its Mods tab has the mods and packs, not the level editor.
    items.push({ label: "Edit its levels (Mods)", icon: "edit", run: () => (app.page = "mods") });
  }
  if (setUp) items.push({ label: "Logs and crash reports", icon: "book", run: showLogs });
  // Android: the game's files are in the Files app (Settings, Your files), and the activity log
  // is the rail's running job.
  // Not on Android: its file picker hands over no path to read the card from.
  const serial = v.serial;
  if (setUp && serial && !onAndroid) {
    items.push({ label: "Import saves from PCSX2", icon: "save", run: () => void importPcsx2(serial) });
  }
  if (!onAndroid) {
    items.push({ label: "Open the game's folder", icon: "folder", run: () => void guard(api.openPath(v.dir)) });
    items.push({ label: "Activity log", icon: "terminal", run: () => (app.page = "tasks") });
  }
  if (setUp) items.push({ label: "Remove the set-up", icon: "trash", danger: true, run: () => void removeSetUp(v) });
  return items;
}

/** Turns the Mods switch: off, Play starts the game without mods; on, with what the Mods tab switched on. */
export async function setWithMods(on: boolean) {
  if (app.config) await saveConfig({ ...app.config, playWithMods: on });
}

/** How many mods the Mods tab has switched on for a game: its gameplay mods, its packs and
 *  Enhanced graphics (which the switch turns off as well). */
export async function modsOnCount(v: VersionView): Promise<number> {
  const [mods, packs, enhanced] = await Promise.all([
    api.pluginsList(v.game, v.name).catch(() => []),
    api.overridesList(v.game).catch(() => []),
    api.enhancedView(v.game, v.name).catch(() => null),
  ]);
  return mods.filter((m) => m.enabled).length + packs.filter((p) => p.enabled).length + (enhanced?.mod.enabled ? 1 : 0);
}
