<script lang="ts">
  // The list of games beside the Games and Mods tabs (SideList), the open one marked; on the
  // desktop each game is its disc. By default it is the Games tab's: a click opens the game's page.
  // Another page passes its own SELECTED, ONSELECT and STATE (Mods: what can be modded).
  import { type GameView, onAndroid } from "$lib/api";
  import { app, openVersion } from "$lib/app.svelte";
  import type { EntryState, SideEntry } from "$lib/labels";
  import { gameThumb } from "$lib/media";
  import { slide } from "$lib/motion";
  import { playerState, playerVersion } from "$lib/player.svelte";
  import { themeStyle } from "$lib/themes";

  import SideList from "./SideList.svelte";

  let {
    selected,
    onselect,
    state,
    label = "Games",
  }: {
    selected?: string | null;
    onselect?: (game: GameView) => void;
    state?: (game: GameView) => EntryState | null;
    label?: string;
  } = $props();

  const games = $derived(app.library?.games ?? []);
  // Another tab pressed: this list stays on screen while that tab's page loads and slides in, so
  // it keeps its game marked. Unmarked then, its disc shrank and turned back during the slide.
  const opened = $derived(
    selected !== undefined
      ? selected
      : app.page !== "library"
        ? (app.version?.split("/")[0] ?? null)
        : null,
  );

  /** What the entry says under the name: the page's own words, else the game's state for playing. */
  function stateOf(game: GameView): EntryState | null {
    if (state) return state(game);
    const v = playerVersion(game);
    const next = v ? playerState(game, v) : null;

    return next ? { text: next.status, tone: next.tone, busy: !!next.job } : null;
  }

  const entries = $derived<SideEntry[]>(
    games.map((game) => ({
      key: game.id,
      name: game.title,
      state: stateOf(game),
      art: gameThumb(game.id),
      // On the desktop each game is its disc (user decision), as on Android's Games tab.
      disc: onAndroid
        ? undefined
        : { game: game.id, version: playerVersion(game)?.name ?? "ntsc", title: game.title },
      style: themeStyle(game.id),
      mark: !state && (app.crashes[game.id]?.recent ?? 0) > 0,
    })),
  );

  function open(game: GameView) {
    if (onselect) {
      onselect(game);

      return;
    }

    const v = playerVersion(game);

    if (v) openVersion(v.key);
  }

  /** The page slides to the game picked on Android: up from below for one further down the list.
   *  On desktop it changes in place, only the accent easing to the game's (app.css): a slide there
   *  faded the page out and scaled its buttons on every pick. */
  function pick(key: string) {
    const to = games.findIndex((g) => g.id === key);
    const game = games[to];

    if (!game) return;

    if (key === opened) {
      open(game);

      return;
    }

    if (!onAndroid) {
      open(game);

      return;
    }

    const from = games.findIndex((g) => g.id === opened);

    slide(
      () => {
        open(game);
      },
      to >= from ? "down" : "up",
    );
  }
</script>

<SideList {entries} selected={opened} onpick={pick} {label} />
