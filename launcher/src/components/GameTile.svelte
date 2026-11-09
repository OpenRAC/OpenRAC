<script lang="ts">
  // One game on the shelf of Android's Games tab (GamesHome.svelte): its picture, its name and
  // whether it can be played, or how far its code is rebuilt. A tap picks the game for the banner
  // above; a tap on the game picked opens its page. The controller picks a game as its focus
  // lands there, so A then opens the page.
  import type { GameView } from "$lib/api";
  import { app, openVersion } from "$lib/app.svelte";
  import { pct } from "$lib/format";
  import { titleLines } from "$lib/labels";
  import { gameTileArt, hideMissing } from "$lib/media";
  import { playerState, playerVersion } from "$lib/player.svelte";
  import { ACCENTS, themeStyle } from "$lib/themes";

  /** `selected`: the game the banner shows. `onpick`: makes this game the one shown. */
  let {
    game,
    delay = 0,
    selected = false,
    onpick,
  }: { game: GameView; delay?: number; selected?: boolean; onpick: () => void } = $props();

  const art = $derived(gameTileArt(game.id));
  const name = $derived(titleLines(game.title));
  const v = $derived(playerVersion(game));
  const next = $derived(v ? playerState(game, v) : null);
  const crashed = $derived((app.crashes[game.id]?.recent ?? 0) > 0);
  /** A game that cannot be played yet says how far its code is rebuilt instead. */
  const playable = $derived(v?.actions.some((a) => a.id === "play" && a.state !== "planned") ?? false);
  const rebuilt = $derived(!playable && v?.progress ? v.progress.percent : null);
  const glow = $derived(ACCENTS[game.id]?.["--glow"] ?? "var(--amber)");

  function press() {
    if (!v) return;
    if (selected) openVersion(v.key);
    else onpick();
  }

  /** The controller's focus picks the game; a finger's tap does it in `press`. */
  function focused() {
    if (document.documentElement.dataset.input === "pad" && !selected) onpick();
  }
</script>

{#if v && next}
  <article
    class="tile rise"
    class:selected
    style={`${themeStyle(game.id)}; --tile-glow: ${glow}; animation-delay: ${delay}ms`}
  >
    {#if art}
      <img class="art" src={art} alt="" decoding="sync" onerror={hideMissing} />
    {/if}
    <div class="shade"></div>
    <button
      class="open"
      onclick={press}
      onfocus={focused}
      aria-label={`${game.title}: ${rebuilt !== null ? `${pct(rebuilt)} rebuilt` : next.status}${crashed ? ". It crashed recently: a report is on its page" : ""}`}
    ></button>
    {#if crashed}<span class="dot-crash" aria-hidden="true"></span>{/if}
    <div class="foot" aria-hidden="true">
      <h2>
        <span>{name.series}</span>{#if name.game}<span>{name.game}</span>{/if}
      </h2>
      {#if rebuilt !== null}
        <p class="status">{pct(rebuilt)} rebuilt</p>
        <span class="bar"><i style={`width: ${Math.max(2, Math.min(100, rebuilt))}%`}></i></span>
      {:else}
        <p class={`status ${next.tone}`}>{next.status}</p>
      {/if}
    </div>
  </article>
{/if}

<style>
  .tile {
    position: relative;
    flex: none;
    width: 210px;
    aspect-ratio: 16 / 9;
    border-radius: var(--radius);
    overflow: hidden;
    background: linear-gradient(160deg, var(--from), var(--to));
    box-shadow: var(--shadow-sm);
    isolation: isolate;
    /* Turned a little to the right, each tile on its own (user decision: 10 degrees). */
    --tilt: perspective(1100px) rotateY(10deg);
    transform: var(--tilt);
    transition:
      transform var(--medium) var(--ease),
      box-shadow var(--medium),
      width var(--medium) var(--ease);
  }

  /* The game the banner shows: larger, ringed in its own colour. */
  .tile.selected {
    width: 250px;
    box-shadow:
      0 0 0 3px var(--tile-glow),
      0 0 24px color-mix(in srgb, var(--tile-glow) 40%, transparent),
      var(--shadow);
  }

  .tile:has(.open:active) {
    transform: var(--tilt) scale(0.97);
    transition-duration: var(--fast);
  }

  /* app.css's rise moves `transform`, which would take the tilt away: the tile rises with
     `translate` instead. */
  .tile.rise {
    animation-name: tile-rise;
  }

  @keyframes tile-rise {
    from {
      opacity: 0;
      translate: 0 18px;
    }
    to {
      opacity: 1;
      translate: none;
    }
  }

  /* The light catches the tile's near edge. */
  .tile::before {
    content: "";
    position: absolute;
    z-index: 1;
    inset: 0;
    border-radius: inherit;
    background: linear-gradient(100deg, color-mix(in srgb, var(--amber) 18%, transparent), transparent 32%);
    pointer-events: none;
  }

  /* A pixel past the tile's edges, so that no line of its own colours shows at its round corners. */
  .art {
    position: absolute;
    inset: -1px;
    width: calc(100% + 2px);
    height: calc(100% + 2px);
    object-fit: cover;
  }

  .shade {
    position: absolute;
    inset: -1px;
    background: linear-gradient(to top, color-mix(in srgb, var(--ink-deep) 92%, transparent) 0%, transparent 64%);
  }

  .open {
    position: absolute;
    inset: 0;
    z-index: 2;
    border: 0;
    border-radius: inherit;
    background: none;
    box-shadow: none;
    padding: 0;
    min-height: 0;
  }

  .open:focus-visible,
  :global(html[data-input="pad"]) .open:focus {
    outline: none;
  }

  :global(html[data-input="pad"]) .tile:has(.open:focus) {
    transform: var(--tilt) translateY(-3px);
  }

  .foot {
    position: absolute;
    left: 12px;
    right: 10px;
    bottom: 9px;
    pointer-events: none;
  }

  h2 {
    margin: 0;
    display: flex;
    flex-direction: column;
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    font-size: 0.8125rem;
    line-height: 1.2;
    color: var(--on-game);
  }

  .status {
    margin: 3px 0 0;
    font-size: 0.71875rem;
    font-weight: 500;
    color: var(--soft);
  }

  .status.ok {
    color: var(--ok);
  }

  .status.warn {
    color: var(--amber);
  }

  .bar {
    display: block;
    margin-top: 6px;
    height: 3px;
    border-radius: 2px;
    background: rgb(255 255 255 / 0.15);
  }

  .bar i {
    display: block;
    height: 100%;
    border-radius: inherit;
    background: var(--tile-glow);
  }

  .dot-crash {
    position: absolute;
    top: 9px;
    right: 10px;
    z-index: 1;
    width: 9px;
    height: 9px;
    border-radius: 50%;
    background: var(--err);
    box-shadow: 0 0 0 2px rgb(0 0 0 / 0.4);
  }
</style>
