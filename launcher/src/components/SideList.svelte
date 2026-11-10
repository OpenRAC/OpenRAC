<script module lang="ts">
  /** The entries come in one after the other only the first time a list shows: switching tabs
   *  keeps the list where it is (app.css's slide), so it would only seem to blink. */
  const lists = { introduced: false };
</script>

<script lang="ts">
  // The list on the left of the Games, Mods and Develop tabs: one entry per place, all the same
  // size, the open one marked (the header's selected-tab colours on the list's dark ground).
  // Each entry is a picture (a game's, framed in its colours) or an icon, a name, and a line on
  // where it stands. In a narrow window only the pictures show, the names in their tooltips. On
  // Android it is a row of pictures above the page (SideFrame), each named across its foot.
  import { onAndroid } from "$lib/api";
  import { shortTitle, type SideEntry } from "$lib/labels";
  import { hideMissing } from "$lib/media";
  import { untrack } from "svelte";
  import Disc from "./Disc.svelte";
  import Icon from "./Icon.svelte";

  let {
    entries,
    selected,
    onpick,
    label,
  }: { entries: SideEntry[]; selected: string | null; onpick: (key: string) => void; label: string } = $props();

  const intro = !lists.introduced;
  lists.introduced = true;

  // The open entry's highlight slides to the entry picked, as the bar's does from tab to tab. Placed
  // without a transition the first time, so that it does not fly in from the top.
  let list = $state<HTMLElement | null>(null);
  let marker = $state({ top: 0, height: 0, shown: false, moving: false });
  let placed = false;
  function place(key: string | null) {
    const on = key === null ? null : list?.querySelector<HTMLElement>(`.entry[data-key="${CSS.escape(key)}"]`);
    marker = on
      ? { top: on.offsetTop, height: on.offsetHeight, shown: true, moving: placed }
      : { top: 0, height: 0, shown: false, moving: false };
    placed = !!on;
  }
  $effect(() => {
    place(selected);
  });
  $effect(() => {
    if (!list) return;
    // A narrow window, or the fonts arriving, changes the entries' sizes.
    const resized = new ResizeObserver(() => {
      place(untrack(() => selected));
    });
    resized.observe(list);
    return () => {
      resized.disconnect();
    };
  });
</script>

<!-- A disc that grows as its game is picked changes its entry's height: the highlight follows. -->
<nav
  class="list"
  aria-label={label}
  bind:this={list}
  ontransitionend={(e) => {
    if ((e.target as HTMLElement).classList.contains("disc")) place(selected);
  }}
>
  <span
    class="marker"
    class:shown={marker.shown}
    class:moving={marker.moving}
    style={`transform: translateY(${marker.top}px); height: ${marker.height}px`}
    aria-hidden="true"
  ></span>
  {#each entries as e, i (e.key)}
    {#if e.group}<span class="group">{e.group}</span>{/if}
    <button
      class="entry"
      class:intro
      class:on={selected === e.key}
      data-key={e.key}
      aria-current={selected === e.key ? "page" : undefined}
      title={e.state ? `${e.name}: ${e.state.text}` : e.name}
      style={`${e.style ?? ""}; --i: ${i}`}
      onclick={() => {
        onpick(e.key);
      }}
    >
      {#if e.disc}
        <span class="disc">
          <Disc game={e.disc.game} version={e.disc.version} title={e.disc.title} idle={selected === e.key} small />
          {#if e.mark}<span class="mark" aria-hidden="true"></span>{/if}
        </span>
      {:else}
        <span class="thumb" class:icon={!e.art}>
          {#if e.art}<img src={e.art} alt="" decoding="async" onerror={hideMissing} />{:else if e.icon}<Icon
              name={e.icon}
              size={20}
            />{/if}
          {#if e.mark}<span class="mark" aria-hidden="true"></span>{/if}
        </span>
      {/if}
      <span class="text">
        <span class="name">{onAndroid ? shortTitle(e.name) : e.name}</span>
        {#if e.mark}<span class="sr-only">, crashed recently</span>{/if}
        {#if e.state}
          <span class={`state ${e.state.tone}`}>
            {#if e.state.busy}<span class="spinner"></span>{:else}<span class="dot"></span>{/if}{e.state.text}
          </span>
        {/if}
      </span>
    </button>
  {/each}
</nav>

<style>
  .list {
    position: relative;
    display: flex;
    flex-direction: column;
    gap: 4px;
    padding: 20px 10px;
    background: var(--rail);
    border-right: 1px solid var(--line);
    /* A row under the pointer: one step lighter than the rail. */
    --control: var(--control-card);
    --control-hi: var(--control-card-hi);
    min-height: 100%;
  }

  .group {
    padding: 14px 8px 6px;
    font: 600 0.75rem/1 var(--font-head);
    letter-spacing: 0.14em;
    text-transform: uppercase;
    color: var(--dim);
  }

  .group:first-child {
    padding-top: 0;
  }

  /* The open entry's ground, under the entries. */
  .marker {
    position: absolute;
    top: 0;
    left: 10px;
    right: 10px;
    border-radius: var(--radius-sm);
    /* The open game: a picked row, the accent at 12 % (no ring round its picture). */
    background: color-mix(in srgb, var(--amber) 12%, transparent);
    opacity: 0;
    pointer-events: none;
  }

  .marker.shown {
    opacity: 1;
  }

  .marker.moving {
    transition:
      transform var(--medium) var(--ease),
      height var(--medium) var(--ease);
  }

  .entry {
    position: relative;
    display: flex;
    align-items: center;
    gap: 12px;
    width: 100%;
    padding: 8px;
    border: 0;
    border-radius: var(--radius-sm);
    background: none;
    box-shadow: none;
    color: var(--text);
    text-align: left;
    font-weight: 400;
    transition:
      background-color 0.18s ease,
      border-color 0.18s ease,
      transform 0.18s ease;
  }

  .entry.intro {
    animation: list-in 0.36s cubic-bezier(0.2, 0.8, 0.2, 1) both;
    animation-delay: calc(var(--i, 0) * 35ms);
  }

  @keyframes list-in {
    from {
      opacity: 0;
      transform: translateX(-8px);
    }
  }

  @media (hover: hover) {
    :global(html:where(:not([data-input="pad"]))) .entry:hover:not(.on) {
      background: var(--control);
      transform: translateX(2px);
    }
  }

  .entry:active {
    transform: scale(0.98);
  }

  .entry.on .name {
    color: var(--amber);
  }

  /* Every picture the same size, in the game's own frame colour. */
  .thumb {
    position: relative;
    flex: none;
    display: grid;
    place-items: center;
    width: 64px;
    height: 44px;
    border-radius: var(--radius-xs);
    overflow: hidden;
    background: linear-gradient(160deg, var(--from, var(--panel)), var(--to, var(--ink-deep)));
    color: var(--soft);
    transition: box-shadow 0.2s ease;
  }

  .thumb.icon {
    background: var(--card);
    --control: var(--control-card);
    --control-hi: var(--control-card-hi);
  }

  .thumb img {
    width: 100%;
    height: 100%;
    object-fit: cover;
    filter: saturate(0.85);
    transition:
      filter 0.2s ease,
      transform 0.3s ease;
  }

  @media (hover: hover) {
    :global(html:where(:not([data-input="pad"]))) .entry:hover .thumb img {
      transform: scale(1.06);
      filter: none;
    }
  }

  .entry.on .thumb img {
    filter: none;
  }

  /* A game's disc in place of its picture (the desktop's game list): the open one larger and
     turned, a disc under the pointer rolling a little to the right. */
  .disc {
    position: relative;
    flex: none;
    width: 58px;
    height: 58px;
    margin: -2px 0;
    transition:
      width var(--medium) var(--ease),
      height var(--medium) var(--ease),
      margin var(--medium) var(--ease),
      translate 0.35s cubic-bezier(0.34, 1.45, 0.64, 1),
      rotate 0.5s cubic-bezier(0.34, 1.45, 0.64, 1);
  }

  .entry.on .disc {
    width: 76px;
    height: 76px;
    margin: -6px -2px -6px -6px;
    rotate: -12deg;
  }

  .entry:not(.on) .disc :global(.disc) {
    opacity: 0.85;
    transition: opacity 0.2s ease;
  }

  @media (hover: hover) {
    :global(html:where(:not([data-input="pad"]))) .entry:hover:not(.on) .disc {
      translate: 5px 0;
      rotate: 24deg;
    }

    :global(html:where(:not([data-input="pad"]))) .entry:hover .disc :global(.disc) {
      opacity: 1;
    }
  }

  /* The controller's focus on a disc rolls it as the pointer does. */
  :global(html[data-input="pad"]) .entry:focus:not(.on) .disc {
    translate: 5px 0;
    rotate: 24deg;
  }

  .disc .mark {
    top: 2px;
    right: 2px;
  }

  .mark {
    position: absolute;
    top: 4px;
    right: 4px;
    width: 9px;
    height: 9px;
    border-radius: 50%;
    background: var(--err);
    box-shadow: 0 0 0 2px var(--ink-deep);
  }

  .text {
    display: flex;
    flex-direction: column;
    gap: 2px;
    min-width: 0;
  }

  .name {
    font-size: 0.8438rem;
    font-weight: 600;
    line-height: 1.3;
    color: var(--text);
    transition: color var(--medium) var(--ease);
  }

  .state {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 0.75rem;
    color: var(--dim);
  }

  .state.ok {
    color: var(--ok);
  }

  .state.warn {
    color: var(--amber);
  }

  .dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: currentColor;
    flex: none;
  }

  /* A narrow window: the pictures only. */
  @media (max-width: 1000px) {
    .list {
      padding: 16px 8px;
    }

    .marker {
      left: 8px;
      right: 8px;
    }

    .group {
      display: none;
    }

    .entry {
      justify-content: center;
      padding: 6px;
    }

    .text {
      position: absolute;
      width: 1px;
      height: 1px;
      overflow: hidden;
      clip-path: inset(50%);
    }

    .thumb {
      width: 52px;
      height: 40px;
    }

    .disc,
    .entry.on .disc {
      width: 48px;
      height: 48px;
      margin: -2px 0;
    }
  }

  :global(html[data-platform="android"]) .list {
    flex-direction: row;
    gap: 10px;
    width: max-content;
    min-width: 100%;
    min-height: 0;
    padding: var(--page-top) var(--page-x) 0;
    background: none;
    border-right: none;
  }

  :global(html[data-platform="android"]) :is(.group, .marker) {
    display: none;
  }

  :global(html[data-platform="android"]) .entry {
    position: relative;
    flex: none;
    width: auto;
    padding: 0;
    border: none;
    border-radius: var(--radius-sm);
  }

  :global(html[data-platform="android"]) .entry.on {
    background: none;
    box-shadow: none;
    transform: none;
  }

  @media (hover: hover) {
    :global(html:where(:not([data-input="pad"]))[data-platform="android"]) .entry:hover:not(.on) {
      background: none;
      box-shadow: none;
      transform: none;
    }
  }

  :global(html[data-platform="android"]) .thumb {
    width: 112px;
    height: 63px;
    border-radius: var(--radius-sm);
  }

  :global(html[data-platform="android"]) .entry:not(.on) .thumb {
    opacity: 0.7;
  }

  /* The name across the picture's foot, the state left to the page. */
  :global(html[data-platform="android"]) .text {
    position: absolute;
    inset: auto 0 0;
    width: auto;
    height: auto;
    padding: 16px 8px 5px;
    clip-path: none;
    border-radius: 0 0 10px 10px;
    background: linear-gradient(transparent, rgb(0 0 0 / 0.8));
    pointer-events: none;
  }

  :global(html[data-platform="android"]) .name {
    overflow: hidden;
    font-size: 0.75rem;
    line-height: 1.2;
    white-space: nowrap;
    text-overflow: ellipsis;
    color: var(--on-game);
  }

  :global(html[data-platform="android"]) .state {
    display: none;
  }
</style>
