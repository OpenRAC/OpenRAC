<script lang="ts">
  // One game on Android's Games tab (Library.svelte): its art, its name, whether it can be
  // played, and the one thing to do next. The whole tile opens the game's page; its button (Play,
  // Set up) takes a finger only, as a controller's A opens the page too.
  import type { GameView } from "$lib/api";
  import { app, dismissCrashes, openVersion } from "$lib/app.svelte";
  import { pct } from "$lib/format";
  import { regionLabel } from "$lib/labels";
  import { gameTileArt, hideMissing } from "$lib/media";
  import { playerAct, playerState, playerVersion } from "$lib/player.svelte";
  import { themeStyle } from "$lib/themes";
  import Icon from "./Icon.svelte";

  /** `selected`: the game picked last, whose colour the launcher wears (themes.ts accent). It is
   *  where the controller's focus starts (pad.ts, aria-current), and is ringed only while no tile
   *  has the focus's ring, so that two rings never show. */
  let { game, delay = 0, selected = false }: { game: GameView; delay?: number; selected?: boolean } = $props();

  // The game's still picture (media.ts); without one, the tile shows the game's colours.
  const art = $derived(gameTileArt(game.id));
  /** The name in two lines, the series and the game ("Ratchet & Clank:", "Going Commando").
   *  "Ratchet: Deadlocked" is one name and keeps one line. */
  const name = $derived.by(() => {
    const at = game.title.startsWith("Ratchet & Clank: ") ? game.title.indexOf(": ") : -1;
    return at < 0
      ? { series: game.title, game: "" }
      : { series: game.title.slice(0, at + 1), game: game.title.slice(at + 2) };
  });
  const v = $derived(playerVersion(game));
  const next = $derived(v ? playerState(game, v) : null);
  /** The tile offers the next step itself only when there is one to take. */
  const offers = $derived(!!next && !next.disabled && next.kind !== "installing" && (next.primary || !!next.job));
  /** What Play warns of before it starts (actions.json `warning`), shown in short on the tile. */
  const playWarning = $derived(v?.actions.find((a) => a.id === "play" && a.thisPlatform)?.warning ?? null);
  /** With developer tools on: how far each version's decompilation is. */
  const matched = $derived(
    app.config?.developer
      ? game.versions.map((o) => `${regionLabel(o.name)} ${o.progress ? pct(o.progress.percent) : "–"}`).join(" · ")
      : "",
  );
</script>

{#if v && next}
  <article class="tile rise" class:selected style={`${themeStyle(game.id)}; animation-delay: ${delay}ms`}>
    {#if art}
      <img class="art" src={art} alt="" decoding="sync" onerror={hideMissing} />
    {/if}
    <div class="shade"></div>

    <button
      class="open"
      aria-current={selected || undefined}
      onclick={() => {
        openVersion(v.key);
      }}
      aria-label={`${game.title}: ${next.status}${(app.crashes[game.id]?.recent ?? 0) > 0 ? ". It crashed recently: a report is on its page" : ""}`}
    ></button>

    <div class="foot">
      <div class="name">
        <h2 aria-label={game.title}>
          <span>{name.series}</span>{#if name.game}<span>{name.game}</span>{/if}
        </h2>
        <p class={`status ${next.tone}`}>
          {#if next.job}<span class="spinner"></span>{:else}<span class="dot"></span>{/if}{next.status}
        </p>
        {#if matched}<p class="matched">{matched} matched</p>{/if}
        {#if playWarning}
          <!-- A game far from finished (actions.json `warning`): its page and Play say the whole of it. -->
          <p class="crashed" title={playWarning}><Icon name="warn" size={13} />Early and unstable</p>
        {/if}
        {#if (app.crashes[game.id]?.recent ?? 0) > 0}
          <p class="crashed">
            <Icon name="warn" size={13} />Crashed recently<span class="where">: a report is on its page</span>
            <button
              class="x"
              data-pad-skip
              onclick={() => void dismissCrashes(game.id)}
              aria-label="Dismiss: hide this until the game crashes again"
              title="Dismiss: hide this until the game crashes again"
            >
              <Icon name="x" size={12} />
            </button>
          </p>
        {/if}
      </div>
      {#if offers}
        <button class="act" class:primary={next.primary} data-pad-skip onclick={() => void playerAct(v, next)}>
          <Icon name={next.icon} size={18} />{next.kind === "set-up" ? "Set up" : next.label}
        </button>
      {/if}
    </div>
  </article>
{/if}

<style>
  .tile {
    position: relative;
    display: flex;
    align-items: flex-end;
    /* Two rows of tiles fill the window at any height it can have. */
    min-height: clamp(240px, calc((100vh - var(--nav-top) - var(--nav-bottom) - 108px) / 2), 400px);
    border-radius: var(--radius);
    overflow: hidden;
    background: linear-gradient(160deg, var(--from), var(--to));
    box-shadow: var(--shadow-sm);
    /* Its foot follows the tile's own width (below). */
    container-type: inline-size;
    /* Its own layer all along, its round corners clipping the picture there: promoted only while
       the picture zoomed, the tile showed a bar along its foot until the zoom ended (Android's
       WebView). */
    isolation: isolate;
    will-change: transform;
    transition:
      transform var(--medium) var(--ease),
      box-shadow var(--medium);
  }

  /* Raised under the pointer, or with the focus of a keyboard or a controller; a touch only
     presses it in while the finger is down, and leaves nothing behind. */
  @media (hover: hover) {
    :global(html:where(:not([data-input="pad"]))) .tile:hover {
      transform: translateY(-3px);
      box-shadow: var(--shadow);
    }

    /* app.css fills every button under the pointer; this one lies over the whole tile, and that
       fill hid the picture while the pointer rested on it. */
    :global(html:where(:not([data-input="pad"]))) .open:hover {
      background: none;
    }

    :global(html:where(:not([data-input="pad"]))) .crashed .x:hover {
      opacity: 1;
      background: var(--control-hi);
    }
  }

  .tile:has(:focus-visible),
  :global(html[data-input="pad"]) .tile:focus-within {
    transform: translateY(-3px);
    box-shadow: var(--shadow);
  }

  .tile:has(.open:active),
  .tile:has(:global(.open.pad-press)) {
    transform: scale(0.98);
    transition-duration: var(--fast);
  }

  /* Android: compact cards, three rows of them in the screen held sideways, so that every game
     shows at once. */
  :global(html[data-platform="android"]) .tile {
    min-height: clamp(128px, calc((100vh - 2 * var(--page-top) - var(--nav-bottom) - 28px) / 3), 200px);
    border-radius: var(--radius);
  }

  :global(html[data-platform="android"]) .foot {
    padding: 14px 16px;
    gap: 12px;
  }

  :global(html[data-platform="android"]) h2 {
    font-size: 1.125rem;
  }

  /* The compact card says only that it crashed; the button's label and the page say the rest. */
  :global(html[data-platform="android"]) .crashed .where {
    display: none;
  }

  :global(html[data-platform="android"]) .act {
    padding: 8px 16px;
    font-size: 0.875rem;
  }

  /* The game picked last: ringed in the launcher's accent, which is its colour, unless a tile has
     the focus's ring (below), which is then the only one. */
  .tile::after {
    content: "";
    position: absolute;
    inset: 0;
    border-radius: inherit;
    box-shadow: inset 0 0 0 2px var(--amber);
    opacity: 0;
    transition: opacity var(--medium) var(--ease);
    pointer-events: none;
  }

  .tile.selected::after {
    opacity: 1;
  }

  /* With the controller in use the focus is the one ring. Not with :has(): Android's WebView left
     a :has() rule unapplied after the launcher opened, until the tiles were drawn again. */
  :global(html[data-input="pad"]) .tile.selected::after {
    opacity: 0;
  }

  :global(.tiles:has(.open:focus-visible)) .tile.selected::after {
    opacity: 0;
  }

  /* A pixel past the tile's edges, the picture and its shade, so that no line of the tile's own
     colours shows at its round corners. The picture stays still under the pointer and the focus. */
  .art {
    position: absolute;
    inset: -1px;
    width: calc(100% + 2px);
    height: calc(100% + 2px);
    object-fit: cover;
  }

  .crashed {
    display: flex;
    align-items: center;
    gap: 6px;
    margin: 6px 0 0;
    font-size: 0.7812rem;
    color: var(--err);
  }

  .shade {
    position: absolute;
    inset: -1px;
    background: linear-gradient(
      to top,
      color-mix(in srgb, var(--ink-deep) 94%, transparent) 0%,
      color-mix(in srgb, var(--ink-deep) 70%, transparent) 40%,
      color-mix(in srgb, var(--ink-deep) 5%, transparent) 72%
    );
  }

  /* The tile's own click target sits under the foot's button. */
  .open {
    position: absolute;
    inset: 0;
    border: 0;
    border-radius: inherit;
    background: none;
    box-shadow: none;
    padding: 0;
    min-height: 0;
  }

  /* The focus rings the whole tile from outside (the tile cuts what is inside it), and the picked
     game's ring inside it gives way. */
  .open:focus-visible,
  :global(html[data-input="pad"]) .open:focus {
    outline: none;
  }

  /* The controller's focus by :focus-within (the pad reaches only .open in a tile), as :has() did
     not show after the launcher opened on Android (above). */
  :global(html[data-input="pad"]) .tile:focus-within {
    outline: 3px solid var(--amber);
    outline-offset: 3px;
  }

  .tile:has(.open:focus-visible) {
    outline: 3px solid var(--amber);
    outline-offset: 3px;
  }

  .foot {
    position: relative;
    width: 100%;
    display: flex;
    align-items: flex-end;
    justify-content: space-between;
    gap: 16px;
    padding: 20px 22px;
    pointer-events: none;
  }

  .foot .act,
  .foot .x {
    pointer-events: auto;
  }

  .crashed .x {
    display: inline-flex;
    padding: 3px;
    color: inherit;
    background: none;
    box-shadow: none;
    border: 0;
    border-radius: 50%;
    opacity: 0.75;
  }

  h2 {
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    font-size: 1.375rem;
    line-height: 1.2;
    color: var(--on-game);
    /* The game's own name always on the line under the series. */
    display: flex;
    flex-direction: column;
  }

  .status {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-top: 6px;
    font-size: 0.875rem;
    font-weight: 500;
    color: var(--soft);
  }

  .matched {
    margin-top: 2px;
    font-size: 0.8125rem;
    color: var(--soft);
    font-variant-numeric: tabular-nums;
  }

  .status.ok {
    color: var(--ok);
  }

  .status.warn {
    color: var(--amber);
  }

  .dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: currentColor;
  }

  .act {
    flex: none;
    font-size: 0.9375rem;
    padding: 10px 22px;
    background: rgb(0 0 0 / 0.5);
  }

  /* A narrow tile: its button under its name, the tile's width. */
  @container (max-width: 340px) {
    .foot {
      flex-direction: column;
      align-items: stretch;
      gap: 14px;
    }

    .act {
      justify-content: center;
    }
  }

  .act.primary {
    background: var(--amber);
    box-shadow: none;
  }
</style>
