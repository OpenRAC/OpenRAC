<script lang="ts">
  // One game's disc (Android's shelf, the desktop's game list): the real print (discArt.ts), or
  // the launcher's own label until it loads or when offline. `idle` makes it float and sway;
  // `small` is the list's (a lighter shadow). The shadow's depth is --shade (0 to 1).
  import { discPrint } from "$lib/discArt";
  import { titleLines } from "$lib/labels";
  import { gameTileArt, keepDecoded } from "$lib/media";
  import { themeStyle } from "$lib/themes";

  let {
    game,
    version,
    title,
    idle = false,
    small = false,
  }: { game: string; version: string; title: string; idle?: boolean; small?: boolean } = $props();

  const print = $derived(discPrint(game, version));
  /** The print could not be loaded: the label shows instead. */
  let failed = $state(false);
  const name = $derived(titleLines(title));
  const art = $derived(gameTileArt(game));
  $effect(() => {
    if (print) keepDecoded(print);
  });
</script>

<div class="disc" class:idle class:small style={themeStyle(game)}>
  <div class="face">
    {#if print && !failed}
      <img
        class="print"
        src={print}
        alt=""
        draggable="false"
        onerror={() => {
          failed = true;
        }}
      />
    {:else}
      <div class="label">
        {#if art}<img class="art" src={art} alt="" draggable="false" />{/if}
        <span class="band"
          ><b>{name.series}</b>{#if name.game}<small>{name.game}</small>{/if}</span
        >
        <span class="hub"></span>
      </div>
    {/if}
    <span class="sheen"></span>
  </div>
</div>

<style>
  .disc {
    position: relative;
    width: 100%;
    height: 100%;
    border-radius: 50%;
    filter: drop-shadow(0 14px 22px rgb(0 0 0 / calc(0.5 * var(--shade, 1))));
  }

  /* The one picked sways left and right and floats, the two never in step. */
  .disc.idle {
    animation: float 3.4s ease-in-out infinite alternate;
  }

  .disc.idle .face {
    animation: sway 5.2s ease-in-out infinite alternate;
  }

  @keyframes float {
    from {
      translate: 0 -3px;
    }
    to {
      translate: 0 3px;
    }
  }

  @keyframes sway {
    from {
      rotate: -4deg;
    }
    to {
      rotate: 4deg;
    }
  }

  /* A list's disc (SideList): a lighter shadow; its label has no room for the name. */
  .disc.small {
    filter: drop-shadow(0 4px 7px rgb(0 0 0 / 0.5));
  }

  .small .band {
    display: none;
  }

  /* The disc itself: a circle, its hole (15 mm of 120) cut out. */
  .face {
    position: absolute;
    inset: 0;
    border-radius: 50%;
    mask: radial-gradient(circle closest-side, transparent 0 12.6%, black 13% 99.2%, transparent 100%);
  }

  .print,
  .label {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    border-radius: 50%;
  }

  .label {
    overflow: hidden;
    background: var(--panel);
    box-shadow: inset 0 0 0 3px color-mix(in srgb, var(--soft) 30%, transparent);
  }

  .label .art {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    object-fit: cover;
    object-position: 50% 30%;
  }

  .band {
    position: absolute;
    left: 0;
    right: 0;
    bottom: 0;
    height: 28%;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding-top: 3%;
    background: var(--ink-deep);
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    font-size: 0.95rem;
    line-height: 1.15;
    color: var(--on-game);
    text-align: center;
  }

  .band small {
    font-size: 0.8em;
  }

  /* The clear ring around the hole. */
  .hub {
    position: absolute;
    inset: 36%;
    border-radius: 50%;
    background: color-mix(in srgb, var(--soft) 35%, transparent);
  }

  /* A soft light that wanders round the disc, on its own beat. */
  .sheen {
    position: absolute;
    inset: 0;
    border-radius: 50%;
    background: conic-gradient(
      from 210deg,
      transparent 0 8%,
      rgb(255 255 255 / 0.1) 12%,
      transparent 18% 58%,
      rgb(255 255 255 / 0.07) 62%,
      transparent 68%
    );
    pointer-events: none;
  }

  .idle .sheen {
    animation: wander 9s linear infinite;
  }

  @keyframes wander {
    to {
      rotate: 1turn;
    }
  }

  @media (prefers-reduced-motion: reduce) {
    .disc.idle,
    .disc.idle .face,
    .idle .sheen {
      animation: none;
    }
  }
</style>
