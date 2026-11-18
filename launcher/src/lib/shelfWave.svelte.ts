// Android's Games wave lives behind the whole app (App.svelte), not inside the Games page. In a tab
// slide the page gets its own layer, and Android's WebView placed the wave's layer up to a third of
// the screen too high. Behind the app it stays in the window's own picture, which never moves.

/** What the Games tab (DiscShelf) tells the wave: whether it shows, where the wheel is, its colour. */
export const shelfWave = $state<{ on: boolean; shift: number; glow: string | undefined }>({
  on: false,
  shift: 0,
  glow: undefined,
});
