<script lang="ts">
  import { supports } from "$lib/capabilities";
  // A desktop's header: no bar of its own, over the page, the logo with its first letter in the
  // game's colour and the theme's buttons: the tabs, the site, Discord.
  // On Android the same places are a navigation rail on the left, or a bar at the bottom when
  // the window is narrow, as Android's own apps have them (Material's navigation rail and bar).
  import { api, inTauri, onAndroid } from "$lib/api";
  import { app, jobs, openDevelop, pendingJobs, runningJob, type Page } from "$lib/app.svelte";
  import { DISCORD_URL, SITE_URL } from "$lib/links";
  import { channelName, updateLabel } from "$lib/updates";
  import { untrack } from "svelte";
  import Icon, { type IconName } from "./Icon.svelte";
  import Logo from "./Logo.svelte";

  /** The first run: the bar with the name only, nothing to go to yet. */
  /** `shown`: the page on screen, which follows app.page inside a page's slide (App.svelte), so that
   *  the tabs change with the page rather than a moment before it. */
  let { setup = false, shown }: { setup?: boolean; shown?: Page } = $props();

  // A player has three places to be: the games, modding and the settings. What the launcher
  // ran (Activity) opens from the running job here and from a game's menu. With
  // developer tools on, Develop is a tab of its own: the checkout, each version and the jobs.
  const tabs = $derived<{ page: Page; label: string; icon: IconName }[]>([
    { page: "library", label: "Games", icon: "library" },
    // Every launcher, installed or on a checkout (an installed one mods in its data folder,
    // tools/bundle_standalone.py). Android's has the mods, Enhanced graphics and the packs, without
    // the level editor (no Godot there: Mods.svelte).
    { page: "mods", label: "Mods", icon: "edit" },
    ...(app.config?.developer ? [{ page: "develop" as Page, label: "Develop", icon: "terminal" as IconName }] : []),
    { page: "settings", label: "Settings", icon: "gear" },
  ]);

  const running = $derived(runningJob());
  const pending = $derived(pendingJobs());
  /** The tab that is open: a game's page belongs to the games. */
  const current = $derived.by<Page>(() => {
    const page = shown ?? app.page;
    return page === "game" ? "library" : page;
  });

  // The open tab's highlight: one layer over the whole row of tabs, clipped to the open tab. A
  // move animates only the clip, so nothing is laid out again each frame. The first place is set without a transition.
  let nav = $state<HTMLElement | null>(null);
  let slider = $state({ left: 0, right: 0, shown: false, moving: false });
  /** Whether the highlight has a place yet: not state, as place() is run by an effect. */
  let placed = false;
  function place(page: Page) {
    const on = nav?.querySelector<HTMLElement>(`.tab[data-page="${page}"]`);
    slider =
      on && nav
        ? {
            left: on.offsetLeft,
            right: nav.clientWidth - on.offsetLeft - on.offsetWidth,
            shown: true,
            moving: placed,
          }
        : { left: 0, right: 0, shown: false, moving: false };
    placed = !!on;
  }
  $effect(() => {
    place(current);
  });
  $effect(() => {
    if (!nav) return;
    // The fonts arrive after the first layout and change the tabs' widths.
    const resized = new ResizeObserver(() => {
      place(untrack(() => current));
    });
    resized.observe(nav);
    return () => {
      resized.disconnect();
    };
  });

  /** Whether the page has scrolled under a desktop's header (its haze, below): the page, or a
   *  column of a tab with a list (SideFrame.svelte), is not at its top. */
  let under = $state(false);
  const scrolled = () => [...document.querySelectorAll("main, main > .frame > *")].some((e) => e.scrollTop > 1);
  $effect(() => {
    if (onAndroid) return;
    const look = () => {
      under = scrolled();
    };
    document.addEventListener("scroll", look, { capture: true, passive: true });
    return () => {
      document.removeEventListener("scroll", look, { capture: true });
    };
  });
  // Another page starts at its top.
  let seen: Page | null = null;
  $effect(() => {
    if (current === seen) return;
    seen = current;
    under = false;
  });
</script>

<header class="site-header" class:under>
  <!-- The first run: the name, not yet a way back to the games. -->
  {#if setup}
    <div class="brand" title={app.info?.version ? `OpenRAC Launcher ${app.info.version}` : undefined}>
      <Logo height={34} />
      <span class="brand-mark"><Logo mark height={37} label="" /></span>
      {#if app.info?.version}<small class="ver">{app.info.version}</small>{/if}
    </div>
  {:else}
    <button
      class="brand"
      onclick={() => (app.page = "library")}
      aria-label="OpenRAC: your games"
      title={app.info?.version ? `OpenRAC Launcher ${app.info.version}` : undefined}
    >
      <Logo height={34} />
      <span class="brand-mark"><Logo mark height={37} label="" /></span>
      <!-- The launcher's version beside the logo, on every page (also in the tooltip, for a phone's
           width, which shows the mark alone). -->
      {#if app.info?.version}<small class="ver">{app.info.version}</small>{/if}
    </button>
  {/if}

  {#if !setup}
    <nav bind:this={nav}>
      <span
        class="slider"
        class:shown={slider.shown}
        class:moving={slider.moving}
        style={`--l: ${slider.left}px; --r: ${slider.right}px`}
        aria-hidden="true"
      ></span>
      {#each tabs as tab (tab.page)}
        {@const on = current === tab.page}
        <button
          class="tab"
          class:on
          disabled={tab.page === "mods" && !supports(app.info, "mods")}
          title={tab.page === "mods" && !supports(app.info, "mods")
            ? "Mods are not available in this build"
            : undefined}
          data-page={tab.page}
          aria-current={on ? "page" : undefined}
          onclick={() => (app.page = tab.page)}
        >
          <span class="icon">
            <Icon name={tab.icon} size={onAndroid ? 22 : 17} />
            {#if onAndroid && tab.page === "settings" && app.update}<span class="badge" aria-hidden="true"></span>{/if}
          </span>
          <span class="label">{tab.label}</span>
          {#if onAndroid && tab.page === "settings" && app.update}<span class="sr-only">, an update is out</span>{/if}
          {#if tab.page === "develop" && pending > 0}<span class="count">{pending}</span>{/if}
        </button>
      {/each}
    </nav>
  {/if}

  {#if running && !setup}
    <button
      class="job"
      title={`${running.title}: show its output`}
      onclick={() => {
        jobs.selected = running.key;
        if (app.config?.developer) openDevelop("console");
        else app.page = "tasks";
      }}
    >
      <span class="spinner"></span>
      <span class="clip">{running.title}</span>
      {#if pending > 1}<span class="count">+{pending - 1}</span>{/if}
    </button>
  {/if}

  <div class="end">
    <!-- Android says it with a dot on Settings, where the update is (a pill at the rail's foot
         looked like a stray bar). -->
    {#if app.update && !onAndroid}
      {@const update = app.update}
      <!-- A newer launcher: in the bar's dark brown, with a dot that says something is new. -->
      <button
        class="update"
        onclick={() => (app.showUpdateModal = true)}
        title={update.switchTo
          ? `${updateLabel(update)}: the channel chosen in Settings`
          : `Launcher ${update.version} is out: see what's new and update`}
      >
        <span class="pulse" aria-hidden="true"></span>{update.switchTo
          ? `Switch to ${channelName(update.switchTo)}`
          : "Update"}<span class="label">{update.version}</span>
      </button>
    {/if}
    {#if !inTauri}<span class="preview">Browser preview</span>{/if}
    {#if !onAndroid}
      <!-- Where OpenRAC lives: the site and the community, two marks in one capsule of the selected
           tab's dark brown. -->
      <div class="links">
        <button class="site" onclick={() => void api.openUrl(SITE_URL)} aria-label="openrac.dev" title="openrac.dev"
          ><Icon name="globe" size={18} /></button
        >
        <button
          class="discord"
          onclick={() => void api.openUrl(DISCORD_URL)}
          aria-label="Join the OpenRAC Discord"
          title="Discord"
        >
          <svg class="mark" viewBox="0 0 24 24" width="18" height="18" aria-hidden="true">
            <path
              d="M20.317 4.37a19.791 19.791 0 0 0-4.885-1.515.074.074 0 0 0-.079.037c-.21.375-.444.864-.608 1.25a18.27 18.27 0 0 0-5.487 0 12.64 12.64 0 0 0-.617-1.25.077.077 0 0 0-.079-.037A19.736 19.736 0 0 0 3.677 4.37a.07.07 0 0 0-.032.027C.533 9.046-.32 13.58.099 18.057a.082.082 0 0 0 .031.057 19.9 19.9 0 0 0 5.993 3.03.078.078 0 0 0 .084-.028 14.09 14.09 0 0 0 1.226-1.994.076.076 0 0 0-.041-.106 13.107 13.107 0 0 1-1.872-.892.077.077 0 0 1-.008-.128 10.2 10.2 0 0 0 .372-.292.074.074 0 0 1 .077-.01c3.928 1.793 8.18 1.793 12.062 0a.074.074 0 0 1 .078.01c.12.098.246.198.373.292a.077.077 0 0 1-.006.127 12.299 12.299 0 0 1-1.873.892.077.077 0 0 0-.041.107c.36.698.772 1.362 1.225 1.993a.076.076 0 0 0 .084.028 19.839 19.839 0 0 0 6.002-3.03.077.077 0 0 0 .032-.054c.5-5.177-.838-9.674-3.549-13.66a.061.061 0 0 0-.031-.03zM8.02 15.33c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.956-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.956 2.418-2.157 2.418zm7.975 0c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.955-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.946 2.418-2.157 2.418z"
              fill="currentColor"
            />
          </svg>
        </button>
      </div>
    {/if}
  </div>
</header>

<style>
  .site-header {
    position: relative;
    z-index: 20;
    display: flex;
    align-items: center;
    gap: 22px;
    flex: none;
    height: var(--nav-top);
    width: 100%;
    padding: 0 18px;
    /* No bar of its own anywhere (see below): it lies over the page. */
    background: none;
    color: var(--bar-name);
  }

  /* A desktop has no bar of its own: the header lies over the page (App.svelte), on the page's own
     ground, the logo's first letter in the game's colour as Android's rail has it, its buttons the
     theme's. Once the page scrolls under it, a haze fades in behind it: blurred, barely darkened,
     fading out below, so nothing runs into the buttons and neither an edge nor a shadow shows. */
  :global(html:not([data-platform="android"])) .site-header {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    /* Android keeps the picked game's own ink. */
    --bar-ink-deep: var(--ink-deep);
    --bar-name: var(--text);
  }

  :global(html:not([data-platform="android"])) .site-header::before {
    content: "";
    position: absolute;
    inset: 0 0 -20px;
    z-index: -1;
    pointer-events: none;
    background: color-mix(in srgb, var(--ink) 30%, transparent);
    backdrop-filter: blur(18px);
    mask-image: linear-gradient(black 65%, transparent);
    -webkit-mask-image: linear-gradient(black 65%, transparent);
    /* Hidden, not only clear, at a page's top: WebKit can still draw a clear backdrop's filter. */
    visibility: hidden;
    opacity: 0;
    transition:
      opacity var(--medium) var(--ease),
      visibility var(--medium);
  }

  :global(html:not([data-platform="android"])) .site-header.under::before {
    visibility: visible;
    opacity: 1;
  }

  :global(html:not([data-platform="android"])) .brand :global(.logo .first) {
    fill: var(--amber);
  }

  :global(html:not([data-platform="android"])) .brand .ver {
    background: var(--control);
    color: var(--amber);
  }

  /* The open tab in the game's colour, its words in the page's dark ink. */
  :global(html:not([data-platform="android"])) .slider {
    background: var(--amber);
  }

  :global(html:not([data-platform="android"])) .tab.on {
    color: var(--bar-ink-deep);
  }

  :global(html:not([data-platform="android"])) .tab.on .count {
    background: var(--bar-ink-deep);
    color: var(--amber);
  }

  :global(html:not([data-platform="android"])) .preview {
    background: var(--control);
    color: var(--soft);
  }

  button {
    border: 0;
    background: none;
    box-shadow: none;
    color: inherit;
    padding: 0;
    min-height: 0;
  }

  /* The logo in the header's ink (a desktop's first letter: above). */
  .brand {
    display: inline-flex;
    align-items: center;
    flex-shrink: 0;
    gap: 8px;
    color: var(--bar-name);
  }

  /* The logo is not a button to look at: no fill under the pointer and no dimming on a press. */
  @media (hover: hover) {
    :global(html:where(:not([data-input="pad"]))) button.brand:hover {
      background-color: transparent;
    }
  }

  button.brand:active {
    filter: none;
  }

  /* Its mark alone, for a window too narrow for the name (below). */
  .brand .brand-mark {
    display: none;
  }

  /* The logo comes in when the launcher opens (not on Android, below), and then holds still: its
     mark is the "O" of the name, so it cannot move on its own. */
  .brand :global(.logo) {
    animation: logo-in 0.7s var(--ease) both;
  }

  @keyframes logo-in {
    from {
      opacity: 0;
      transform: translateX(-8px);
    }
  }

  /* The version beside the logo, as tall as the capitals of "RAC" and level with them (the logo's
     box is centred on its letters: Logo.svelte), in the selected tab's dark brown and gold. */
  .brand .ver {
    display: inline-flex;
    align-items: center;
    height: 15px;
    padding: 0 6px;
    margin-left: 2px;
    border-radius: var(--radius-xs);
    background: var(--bar-ink-deep);
    color: var(--gold);
    font-size: 0.625rem;
    font-weight: 700;
    line-height: 1;
    font-variant-numeric: tabular-nums;
  }

  /* The tabs stand on the bar on their own, an icon and a word each, with room between them: no
     capsule round them. */
  nav {
    position: relative;
    /* The tabs' fills, the slider over them and the tabs' words stack inside the bar. */
    isolation: isolate;
    display: flex;
    align-items: center;
    flex-shrink: 0;
    gap: 6px;
  }

  /* The open tab's pill: one layer over the row, clipped to the open tab, the clip gliding from
     tab to tab. */
  .slider {
    position: absolute;
    inset: 0;
    background: var(--control-hi);
    clip-path: inset(0 var(--r) 0 var(--l) round var(--radius-sm));
    opacity: 0;
    /* Over every tab's fill (theirs is at -2), under their words. */
    z-index: -1;
  }

  .slider.shown {
    opacity: 1;
  }

  .slider.moving {
    transition: clip-path 0.34s cubic-bezier(0.25, 1, 0.5, 1);
  }

  /* The open tab is a dark chip with gold text (the slider above, moving from tab to tab); the
     others write in the bar's ink and darken a little under the pointer. */
  .tab {
    position: relative;
    display: inline-flex;
    align-items: center;
    gap: 7px;
    height: 32px;
    min-height: 32px;
    border-radius: var(--radius-sm);
    padding: 0 14px 0 12px;
    font-size: 0.9375rem;
    /* Medium, not bold: close to the logo's thin strokes beside it (user decision). */
    font-weight: 500;
    /* A button of its own: the theme's control fill (under it, ::before), gold text, no edge. */
    background: none;
    box-shadow: none;
    color: var(--amber);
    transition: color var(--medium) var(--ease);
  }

  /* Each tab's fill sits under the slider, so the slider passes over the tabs it crosses and the
     tab opened keeps its fill until the slider gets there. */
  .tab::before {
    content: "";
    position: absolute;
    inset: 0;
    z-index: -2;
    border-radius: inherit;
    background: var(--control);
    transition: background var(--fast) var(--ease);
  }

  @media (hover: hover) {
    /* The fill is the ::before: the button's own hover fill would cover the slider. */
    :global(html:where(:not([data-input="pad"]))) .tab:hover {
      background-color: transparent;
    }

    :global(html:where(:not([data-input="pad"]))) .tab:hover:not(.on)::before {
      background: var(--control-hi);
    }
  }

  /* The open one: the slider under it, its words lighter. */
  .tab.on {
    color: var(--gold);
  }

  .count {
    min-width: 20px;
    height: 20px;
    border-radius: 10px;
    background: var(--bar-ink-deep);
    color: var(--amber);
    font-size: 0.75rem;
    display: inline-grid;
    place-items: center;
    padding: 0 6px;
  }

  .tab.on .count {
    background: var(--amber);
    box-shadow: none;
    color: var(--bar-ink-deep);
  }

  .job {
    gap: 8px;
    max-width: 280px;
    font-size: 0.8125rem;
    font-weight: 500;
    color: var(--amber);
    background: var(--control);
    border-radius: var(--radius-sm);
    height: 32px;
    padding: 0 12px;
    animation: rise var(--medium) var(--ease) both;
  }

  .end {
    margin-left: auto;
    display: flex;
    align-items: center;
    flex-shrink: 0;
    gap: 10px;
  }

  .preview {
    display: inline-flex;
    align-items: center;
    height: 24px;
    padding: 0 10px;
    border-radius: var(--radius-sm);
    background: rgb(0 0 0 / 0.12);
    font-size: 0.75rem;
    font-weight: 700;
    color: var(--bar-ink-deep);
  }

  /* The site and Discord: two buttons of their own, as the tabs, not one joined capsule. */
  .links {
    display: inline-flex;
    align-items: center;
    gap: 6px;
  }

  .site,
  .discord {
    display: grid;
    place-items: center;
    width: 32px;
    height: 32px;
    border-radius: var(--radius-sm);
    background: var(--control);
    color: var(--amber);
    transition:
      background var(--fast) var(--ease),
      color var(--fast) var(--ease);
  }

  .discord .mark {
    transition: transform var(--medium) var(--ease);
  }

  @media (hover: hover) {
    :global(html:where(:not([data-input="pad"]))) .site:hover,
    :global(html:where(:not([data-input="pad"]))) .discord:hover {
      background: var(--control-hi);
      color: var(--gold);
    }

    :global(html:where(:not([data-input="pad"]))) .discord:hover .mark {
      transform: rotate(-12deg) scale(1.08);
    }
  }

  .site:active,
  .discord:active {
    background: var(--control-hi);
    color: var(--gold);
  }

  /* An update: a bar button (the control fill, gold text), with a dot that pulses. */
  .update {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    height: 32px;
    font-size: 0.8125rem;
    font-weight: 500;
    background: var(--control);
    color: var(--gold);
    border-radius: var(--radius-sm);
    padding: 4px 14px 4px 11px;
    transition: color var(--fast);
  }

  /* Under the pointer only its gold brightens: no blue edge (app.css's buttons) and no lift, which
     moved it by a pixel and left its text redrawn a little differently. */
  @media (hover: hover) {
    :global(html:where(:not([data-input="pad"]))) .update:hover {
      color: var(--amber);
    }
  }

  /* The dot rings a few times when the update shows, then rests: a ring that grows and fades
     (transform and opacity, which the compositor runs without restyling the page each frame). */
  .pulse {
    position: relative;
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--amber);
    box-shadow: none;
  }

  .pulse::after {
    content: "";
    position: absolute;
    inset: 0;
    border-radius: 50%;
    background: var(--amber);
    box-shadow: none;
    opacity: 0;
    animation: pulse 1.8s ease-out 5;
  }

  @keyframes pulse {
    from {
      opacity: 0.6;
      transform: scale(1);
    }
    to {
      opacity: 0;
      transform: scale(3);
    }
  }

  .update .label {
    margin-left: -4px;
  }

  @media (max-width: 1240px) {
    header,
    .site-header {
      gap: 18px;
    }

    .preview {
      display: none;
    }
  }

  /* Down to the window's smallest (960 px), with developer tools, a job and an update all showing:
     the update keeps its word, the job less of its name. */
  @media (max-width: 1180px) {
    header,
    .site-header {
      gap: 14px;
    }

    .update .label {
      display: none;
    }

    .job {
      max-width: 150px;
    }
  }

  /* Where room runs short (developer tools, a job and an update all showing), the tabs keep their
     words and drop their icons first. */
  @media (max-width: 1100px) {
    :global(html:not([data-platform="android"])) .tab .icon {
      display: none;
    }

    :global(html:not([data-platform="android"])) .tab {
      padding: 0 12px;
    }
  }

  /* A narrow window keeps the tabs. */
  @media (max-width: 900px) {
    .site-header {
      gap: 12px;
    }

    .job {
      max-width: 160px;
    }
  }

  /* A phone held upright: the logo's mark stands for the name, the version (in the tooltip), the
     links and the running task give way, and the tabs scroll if they still do not fit. */
  @media (max-width: 560px) {
    .brand > :global(.logo),
    .brand .ver,
    .links,
    .job {
      display: none;
    }

    .brand .brand-mark {
      display: contents;
    }

    nav {
      flex-shrink: 1;
      min-width: 0;
      overflow-x: auto;
      scrollbar-width: none;
    }

    .tab {
      padding: 5px 11px;
    }
  }

  /* ---- Android: a navigation rail on the left ---- */

  :global(html[data-platform="android"]) .site-header {
    flex-direction: column;
    gap: 16px;
    flex: none;
    /* The window's whole height beside the page, which scrolls on its own (App.svelte): the
       shell is the window's height, and the rail stretches to it, never past it. */
    align-self: stretch;
    width: var(--nav-left);
    height: auto;
    min-height: 0;
    padding: 16px 0 12px;
    /* No ground of its own: over the page's left edge (App.svelte), the page's ground under it,
       Games' wave seen through it as through frosted glass (WaveBackdrop.svelte). */
    position: absolute;
    top: 0;
    bottom: 0;
    left: 0;
    color: var(--text);
    /* It scrolls only up and down (no bar on Android: app.css): a tab one pixel wider than the
       rail inside its border made a sideways scroll bar along its foot. */
    overflow-x: hidden;
    overflow-y: auto;
  }

  /* The logo's mark in the gold, as wide as the rail's destinations: the rail has
     no room for the name. */
  :global(html[data-platform="android"]) .brand {
    flex: none;
    justify-content: center;
    width: 56px;
    padding-top: 6px;
  }

  :global(html[data-platform="android"]) .brand > :global(.logo),
  :global(html[data-platform="android"]) .brand .ver {
    display: none;
  }

  /* It stays still there: the logo of an app, not a control that plays. */
  :global(html[data-platform="android"]) .brand .brand-mark {
    display: contents;
    color: var(--amber);
  }

  :global(html[data-platform="android"]) .brand .brand-mark :global(.logo) {
    width: 53px;
    height: auto;
    animation: none;
  }

  :global(html[data-platform="android"]) nav {
    flex-direction: column;
    gap: 12px;
    height: auto;
    padding: 0;
    border-radius: 0;
    background: none;
    box-shadow: none;
  }

  :global(html[data-platform="android"]) .slider,
  :global(html[data-platform="android"]) .tab::before {
    display: none;
  }

  /* A destination: its icon in a circle that grows out from the middle when it is the open one,
     and its name under it. */
  :global(html[data-platform="android"]) .tab {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 4px;
    width: var(--nav-left);
    height: auto;
    padding: 0;
    border-radius: 0;
    background: none;
    box-shadow: none;
    font-size: 0.75rem;
    font-weight: 700;
    color: var(--soft);
  }

  :global(html[data-platform="android"]) .tab:active,
  :global(html[data-platform="android"]) .tab:global(.pad-press) {
    transform: none;
    filter: none;
  }

  /* The focus rings the icon's circle: the tab is as wide as the rail, which cut a ring round it
     down to two lines across. */
  :global(html[data-platform="android"]) .tab:focus-visible,
  :global(html[data-platform="android"][data-input="pad"]) .tab:focus {
    outline: none;
  }

  :global(html[data-platform="android"]) .tab:focus-visible .icon,
  :global(html[data-platform="android"][data-input="pad"]) .tab:focus .icon {
    outline: 3px solid var(--amber);
    outline-offset: 3px;
  }

  /* On the desktop's bar: a small icon before the word, in its colour. */
  .icon {
    position: relative;
    display: grid;
    place-items: center;
  }

  .icon::before {
    display: none;
  }

  /* On Android's rail: a circle, as round as the logo's "O" above them. */
  :global(html[data-platform="android"]) .icon {
    width: 40px;
    height: 40px;
    border-radius: 50%;
  }

  :global(html[data-platform="android"]) .icon::before {
    display: block;
    content: "";
    position: absolute;
    inset: 0;
    border-radius: inherit;
    background: color-mix(in srgb, var(--amber) 22%, transparent);
    transform: scale(0.5);
    opacity: 0;
    transition:
      transform var(--medium) var(--ease),
      opacity var(--fast);
  }

  /* Their strokes heavier than the page's icons, to stand under the logo's. */
  .icon :global(svg) {
    position: relative;
    stroke-width: 2.3;
    transition: transform var(--medium) var(--ease);
  }

  :global(html[data-platform="android"]) .tab.on {
    color: var(--amber);
  }

  .tab.on .icon::before {
    transform: none;
    opacity: 1;
  }

  .tab.on .icon :global(svg) {
    transform: scale(1.08);
  }

  /* A new launcher is out: a dot on Settings, where it is installed from. */
  .badge {
    position: absolute;
    top: 4px;
    right: 4px;
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--ok);
    box-shadow: 0 0 0 2px var(--ink-deep);
  }

  :global(html[data-platform="android"]) .tab .count {
    position: absolute;
    top: -2px;
    left: 26px;
    min-width: 18px;
    height: 18px;
    font-size: 0.75rem;
    background: var(--amber);
    box-shadow: none;
    color: var(--bar-ink-deep);
  }

  /* The running task: its spinner, its name in the tooltip. */
  :global(html[data-platform="android"]) .job {
    display: inline-flex;
    flex-direction: column;
    gap: 4px;
    max-width: none;
    padding: 8px;
    color: var(--amber);
    background: none;
    box-shadow: none;
    border: 0;
  }

  :global(html[data-platform="android"]) .job .clip {
    display: none;
  }

  :global(html[data-platform="android"]) .job .count {
    background: var(--ink-deep);
  }

  :global(html[data-platform="android"]) .end {
    margin: auto 0 0;
    flex-direction: column;
  }

  /* ---- Android on a narrow window: a navigation bar at the bottom ---- */

  @media (max-width: 599px) {
    :global(html[data-platform="android"]) .site-header {
      flex-direction: row;
      justify-content: space-around;
      width: 100%;
      height: var(--nav-bottom);
      padding: 12px 0 16px;
      /* In the window's foot, below the page, on its own ground. */
      position: static;
      background: var(--rail);
      border-right: 0;
      border-top: 1px solid var(--line);
      overflow: visible;
    }

    :global(html[data-platform="android"]) .brand,
    :global(html[data-platform="android"]) .job,
    :global(html[data-platform="android"]) .end {
      display: none;
    }

    :global(html[data-platform="android"]) nav {
      flex: 1;
      flex-direction: row;
      justify-content: space-around;
      overflow: visible;
    }

    :global(html[data-platform="android"]) .tab {
      flex: 1;
      width: auto;
    }
  }
</style>
