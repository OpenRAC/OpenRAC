// What a game offers beside its Play button: the "..." menu of what is done less often, and the
// Mods switch (config.playWithMods) with how many mods it brings. The game's page shows them on the
// desktop; on Android they sit beside the disc on the Games tab (DiscShelf.svelte) instead.
import type { MenuItem } from "$components/Menu.svelte";

import { api, ask, type GameView, onAndroid, type VersionView } from "./api";
import { app, gameWindow, guard, openVersion, run, saveConfig, uninstallGame } from "./app.svelte";
import { isBuilt, type PlayerState, rebuild, rebuildSteps } from "./player.svelte";

/** Asked of the game's page as it opens: unfold Help with its logs (the menu's way from Games). */
export const pageAsks = $state({ logs: false });

async function removeSetUp(v: VersionView) {
  const sure = await ask(
    `Remove what was set up from your ${v.title} disc? Your disc image and your saves are kept; set it up again any time.`,
    "Remove",
  );

  if (sure) await uninstallGame(v.game);
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
  const preview = action("editor-preview");
  const items: MenuItem[] = [];

  if (next?.kind === "return") {
    items.push({
      label: "Stop the game",
      icon: "x",
      danger: true,
      run: () => void gameWindow(true),
    });
  }

  // From scratch: everything built before is deleted first (tools/build_port.py --clean); on
  // Android, the levels are read from the disc again (rebuildSteps).
  if (isBuilt(game, v) && (next?.kind === "play" || next?.kind === "levels") && rebuildSteps(v)) {
    items.push({ label: "Rebuild the game", icon: "build", run: () => void rebuild(v) });
  }

  if (preview?.runnable) {
    items.push({
      label: "Preview a level in OpenRAC Editor",
      icon: "play",
      run: () => void run(scope, preview),
    });
  }

  if (setUp) items.push({ label: "Logs and crash reports", icon: "book", run: showLogs });

  // Android: the game's files are in the Files app (Settings, Your files), and the activity log
  // is the rail's running job.
  if (!onAndroid) {
    items.push({
      label: "Open the game's folder",
      icon: "folder",
      run: () => void guard(api.openPath(v.dir)),
    });
    items.push({ label: "Activity log", icon: "terminal", run: () => (app.page = "tasks") });
  }

  if (setUp)
    items.push({
      label: "Remove the set-up",
      icon: "trash",
      danger: true,
      run: () => void removeSetUp(v),
    });

  return items;
}

/** Whether Play would start the game now (play, or build or get levels and play): the Mods
 *  switch shows then. */
export function startsPlay(next: PlayerState | null): boolean {
  return (
    !!next && !next.job && (next.kind === "play" || next.kind === "build" || next.kind === "levels")
  );
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

  return (
    mods.filter((m) => m.enabled).length +
    packs.filter((p) => p.enabled).length +
    (enhanced?.mod.enabled ? 1 : 0)
  );
}
