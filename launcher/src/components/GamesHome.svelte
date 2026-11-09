<script lang="ts">
  // Android's Games tab, as a console's home screen (user decision, after SteamOS): the game picked
  // in a large banner with its next step (Continue, Set up) and where its last save is; under it a
  // shelf of every game, then what changed (an update, mods to update, a crash); at the foot the
  // job running and what the controller's buttons do. The rail stays on the left (Header.svelte).
  // A game is picked by a tap on its tile, or by the controller's focus landing on it; A on the
  // game picked, or a tap on it, opens its page. X opens its mods, Y its page.
  import type { GameView, SaveSlotInfo } from "$lib/api";
  import { api } from "$lib/api";
  import { app, dismissCrashes, jobs, openVersion, pickVersion } from "$lib/app.svelte";
  import { grouped, pct } from "$lib/format";
  import { titleLines } from "$lib/labels";
  import { gameArt, hideMissing } from "$lib/media";
  import { modUpdates } from "$lib/modUpdates.svelte";
  import { news, type NewsItem } from "$lib/news";
  import { hasPort, offersStep, playerAct, playerState, playerVersion, playWarning } from "$lib/player.svelte";
  import { backgroundStatus } from "$lib/progress";
  import { when } from "$lib/reports";
  import { newestSave, savedSeconds } from "$lib/saves";
  import { ACCENTS, themeStyle } from "$lib/themes";
  import GameTile from "./GameTile.svelte";
  import Icon from "./Icon.svelte";

  let { games }: { games: GameView[] } = $props();

  /** The game shown: the one picked last, else the first. */
  const game = $derived(games.find((g) => g.id === app.version?.split("/")[0]) ?? games[0]);
  const v = $derived(game ? playerVersion(game) : null);
  const next = $derived(game && v ? playerState(game, v) : null);
  const name = $derived(titleLines(game?.title ?? ""));
  const art = $derived(game ? gameArt(game.id) : null);
  const glow = $derived((game && ACCENTS[game.id]?.["--glow"]) ?? "var(--amber)");
  /** The banner offers the next step itself only when there is one to take (as the game's page). */
  const offers = $derived(offersStep(next));
  const playable = $derived(hasPort(v));
  const warning = $derived(playWarning(v));
  const crashed = $derived(game ? (app.crashes[game.id]?.recent ?? 0) > 0 : false);
  /** How far the game's code is rebuilt: for a game not done yet, or for a developer. */
  const rebuilt = $derived(v?.progress && (v.progress.percent < 100 || app.config?.developer) ? v.progress : null);

  // ---- the last save: where the player left off ----

  /** The newest save of each version read, by serial: read once, again when the saves change. A
   *  read that failed is not kept, so the next look tries again. */
  const saves = $state<Record<string, SaveSlotInfo | null>>({});
  const set = $derived(!!next && !["set-up", "installing", "other-disc"].includes(next.kind));
  $effect(() => {
    const serial = v?.serial;
    // Read again whenever the card changed elsewhere (an import, the game ending).
    const changed = app.savesChanged;
    if (!serial || !set || !playable || changed < 0) return;
    api
      .inspectSaves(serial)
      .then((status) => {
        saves[serial] = newestSave(status.slots);
      })
      .catch(() => undefined);
  });
  const last = $derived(v ? (saves[v.serial] ?? null) : null);
  /** When it was saved, by the game's own clock, as the game's page says it. */
  const lastWhen = $derived.by(() => {
    const seconds = last ? savedSeconds(last) : null;
    return seconds ? when(seconds) : null;
  });
  /** Continue: the game is ready to play and has a save to go on from. */
  const continues = $derived(next?.kind === "play" && !!last);

  // ---- what changed ----

  const NEWS_LOOK: Record<NewsItem["kind"], { icon: "download" | "refresh" | "warn"; tone: string }> = {
    update: { icon: "download", tone: "var(--amber)" },
    mods: { icon: "refresh", tone: "var(--ok)" },
    crash: { icon: "warn", tone: "var(--err)" },
  };
  const changes = $derived(
    news({
      update: app.update,
      mods: Object.values(modUpdates.found),
      games: games.map((g) => ({
        id: g.id,
        title: g.title,
        version: playerVersion(g)?.key ?? null,
        crashes: app.crashes[g.id]?.recent ?? 0,
      })),
    }),
  );

  function follow(n: NewsItem) {
    if (n.kind === "update") app.showUpdateModal = true;
    else if (n.kind === "mods" && n.version) {
      pickVersion(n.version);
      app.page = "mods";
    } else if (n.version) openVersion(n.version);
  }

  // ---- the foot: the job running, and the controller's buttons ----

  const job = $derived(
    backgroundStatus(
      jobs.list.find((j) => j.state === "running"),
      jobs.list.filter((j) => j.state === "queued").length,
      null,
    ),
  );
  /** Where the focus is: the banner's step, a tile, or anything else. */
  let focusOn = $state<"go" | "tile" | "other">("other");
  /** What A does there, kept up to date as the step changes (a setup ending, a save found). */
  const aSays = $derived(
    focusOn === "go" ? (continues ? "Continue" : (next?.label ?? "Select")) : focusOn === "tile" ? "Open" : "Select",
  );

  function focusMoved(e: FocusEvent) {
    const el = e.target as HTMLElement;
    focusOn = el.closest(".go") ? "go" : el.closest(".tile") ? "tile" : "other";
  }

  /** X: the game's mods, for a game with a port; not while a dialog or a menu is open over the tab. */
  function openMods() {
    if (!v || !playable || document.querySelector('[role="dialog"], [role="menu"]')) return;
    pickVersion(v.key);
    app.page = "mods";
  }

  function openPage() {
    if (v) openVersion(v.key);
  }

  let root = $state<HTMLElement | null>(null);
  $effect(() => {
    const el = root;
    if (!el) return;
    el.addEventListener("padmore", openPage);
    document.addEventListener("padextra", openMods);
    return () => {
      el.removeEventListener("padmore", openPage);
      document.removeEventListener("padextra", openMods);
    };
  });
</script>

{#if game && v && next}
  <div class="home" bind:this={root} onfocusin={focusMoved} style={`--hero-glow: ${glow}`}>
    <section class="hero" aria-label={game.title} style={themeStyle(game.id)}>
      {#key game.id}
        {#if art}<img class="pic" src={art} alt="" decoding="async" onerror={hideMissing} />{/if}
      {/key}
      <div class="veil"></div>
      <div class="body">
        {#if continues}<p class="kicker">Continue where you left off</p>{/if}
        <h1>
          <span>{name.series}</span>{#if name.game}<span>{name.game}</span>{/if}
        </h1>
        {#if last}
          <p class="meta">
            {#if last.planetName}<span><b>{last.planetName}</b> · slot {last.slotIndex + 1}</span>{/if}
            {#if last.bolts !== null}<span><b>{grouped(last.bolts)}</b> bolts</span>{/if}
            {#if lastWhen}<span>{lastWhen}</span>{/if}
          </p>
        {:else}
          <p class="meta"><span class={`state ${next.tone}`}>{next.status}</span></p>
        {/if}
        {#if warning !== null || crashed || (last?.mods?.length ?? 0) > 0}
          <p class="pills">
            {#if warning}<span class="pill warn" title={warning}><Icon name="warn" size={13} />Early and unstable</span
              >{/if}
            {#if crashed}<span class="pill warn"><Icon name="warn" size={13} />Crashed recently</span>{/if}
            {#if last?.mods?.length}<span class="pill ok"
                >Saved with {last.mods.length} mod{last.mods.length === 1 ? "" : "s"}</span
              >{/if}
          </p>
        {/if}
        <div class="acts">
          {#if offers}
            <button class="go" class:primary={next.primary} onclick={() => void playerAct(v, next)}>
              <span class="glyph a" aria-hidden="true">A</span>{continues ? "Continue" : next.label}
            </button>
          {/if}
          <button class="sec" onclick={openPage}><span class="glyph" aria-hidden="true">Y</span>Game page</button>
          {#if playable}
            <button class="sec" onclick={openMods}><span class="glyph" aria-hidden="true">X</span>Mods</button>
          {/if}
        </div>
      </div>
      {#if rebuilt}
        <div class="card">
          <p class="label">This game</p>
          <p class="row"><span>Code rebuilt</span><b>{pct(rebuilt.percent)}</b></p>
        </div>
      {/if}
    </section>

    <section class="shelf" aria-label="Your games">
      <h2>Your games</h2>
      <div class="tiles">
        {#each games as g, i (g.id)}
          <GameTile
            game={g}
            delay={i * 50}
            selected={g.id === game.id}
            onpick={() => {
              const gv = playerVersion(g);
              if (gv) pickVersion(gv.key);
            }}
          />
        {/each}
      </div>
    </section>

    {#if changes.length}
      <section class="shelf" aria-label="What's new">
        <h2>What's new</h2>
        <div class="news">
          {#each changes as n (n.key)}
            {@const look = NEWS_LOOK[n.kind]}
            <div class="nw" style={`--c: ${look.tone}`}>
              <button
                class="main"
                onclick={() => {
                  follow(n);
                }}
              >
                <span class="ic"><Icon name={look.icon} size={18} /></span>
                <span class="txt"><b>{n.title}</b><span>{n.text}</span></span>
              </button>
              {#if n.kind === "crash" && n.game}
                {@const crashedGame = n.game}
                <button class="x" aria-label="Dismiss" onclick={() => void dismissCrashes(crashedGame)}>
                  <Icon name="x" size={14} />
                </button>
              {/if}
            </div>
          {/each}
        </div>
      </section>
    {/if}

    <footer class="foot">
      <div class="job">
        {#if job}
          <span class="spinner"></span>
          <span class="what">{job.title}</span>
          {#if job.percent !== null}
            <span class="pb"><i style={`width: ${job.percent}%`}></i></span>
            <span class="num">{job.percent} %</span>
          {/if}
        {/if}
      </div>
      <div class="hints" aria-hidden="true">
        <span><span class="glyph a">A</span>{aSays}</span>
        {#if playable}<span><span class="glyph">X</span>Mods</span>{/if}
        <span><span class="glyph">Y</span>Game page</span>
        <span><span class="glyph wide">LB</span><span class="glyph wide">RB</span>Tabs</span>
      </div>
    </footer>
  </div>
{/if}

<style>
  .home {
    position: relative;
    min-height: 100%;
    display: flex;
    flex-direction: column;
  }

  /* ---- the banner ---- */

  /* Its height follows the screen's, so that the shelf shows under it on a phone held sideways. */
  .hero {
    --hero-h: clamp(220px, 54vh, 460px);
    position: relative;
    flex: 1 0 var(--hero-h);
    min-height: var(--hero-h);
    overflow: hidden;
  }

  .pic {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    object-fit: cover;
    object-position: 50% 25%;
    animation: fade var(--medium) var(--ease) both;
  }

  @keyframes fade {
    from {
      opacity: 0;
    }
  }

  /* The picture sinks into the page on the left, under the words, and at its foot. */
  .veil {
    position: absolute;
    inset: 0;
    background:
      linear-gradient(90deg, var(--ink) 0%, color-mix(in srgb, var(--ink) 86%, transparent) 30%, transparent 64%),
      linear-gradient(0deg, var(--ink) 0%, transparent 45%),
      radial-gradient(70% 90% at 50% 0, color-mix(in srgb, var(--hero-glow) 14%, transparent), transparent 70%);
  }

  .body {
    position: absolute;
    left: var(--page-x);
    right: var(--page-x);
    bottom: 18px;
    max-width: 600px;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .kicker {
    margin: 0;
    font-size: 0.75rem;
    font-weight: 600;
    letter-spacing: 0.14em;
    text-transform: uppercase;
    color: var(--hero-glow);
  }

  h1 {
    margin: 0;
    display: flex;
    flex-direction: column;
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    font-size: 2rem;
    line-height: 1.1;
    color: var(--on-game);
    text-shadow: 0 2px 18px rgb(0 0 0 / 0.45);
  }

  .meta {
    margin: 0;
    display: flex;
    flex-wrap: wrap;
    gap: 4px 16px;
    font-size: 0.875rem;
    color: var(--soft);
  }

  .meta b {
    color: var(--text);
    font-weight: 600;
  }

  .state.ok {
    color: var(--ok);
  }

  .state.warn {
    color: var(--amber);
  }

  .pills {
    margin: 0;
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
  }

  .pill {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 3px 10px;
    border-radius: 99px;
    background: color-mix(in srgb, var(--ink-deep) 45%, transparent);
    font-size: 0.78rem;
  }

  .pill.warn {
    color: var(--err);
  }

  .pill.ok {
    color: var(--ok);
  }

  .acts {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    margin-top: 4px;
  }

  .acts button {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    height: 46px;
    padding: 0 18px 0 12px;
    border-radius: var(--radius);
    font-weight: 600;
  }

  .go,
  .sec {
    background: color-mix(in srgb, var(--panel) 85%, transparent);
    color: var(--text);
  }

  .go.primary {
    padding-right: 24px;
    background: var(--amber);
    color: var(--bar-ink-deep);
    font-size: 1rem;
    font-weight: 700;
  }

  .card {
    position: absolute;
    top: 18px;
    right: var(--page-x);
    min-width: 190px;
    padding: 10px 14px;
    border-radius: var(--radius);
    background: color-mix(in srgb, var(--panel) 88%, transparent);
    font-size: 0.8125rem;
    color: var(--soft);
  }

  .card p {
    margin: 0;
  }

  .card .label {
    margin-bottom: 4px;
    font-size: 0.6875rem;
    font-weight: 600;
    letter-spacing: 0.12em;
    text-transform: uppercase;
    color: var(--dim);
  }

  .card .row {
    display: flex;
    justify-content: space-between;
    gap: 12px;
  }

  .card b {
    color: var(--hero-glow);
    font-variant-numeric: tabular-nums;
  }

  /* ---- the shelves ---- */

  .shelf {
    flex: none;
    padding: 0 0 4px var(--page-x);
  }

  .shelf h2 {
    margin: 4px 0 8px;
    font-family: var(--font-sans);
    font-size: 0.78rem;
    font-weight: 600;
    letter-spacing: 0.12em;
    text-transform: uppercase;
    color: var(--soft);
  }

  /* Room between the tiles for the one picked, which grows over its neighbours' gap. */
  .tiles {
    display: flex;
    align-items: center;
    gap: 26px;
    min-height: 152px;
    padding: 4px var(--page-x) 10px 16px;
    overflow: auto hidden;
    scrollbar-width: none;
  }

  .news {
    display: flex;
    gap: 12px;
    padding: 0 var(--page-x) 12px 0;
    overflow: auto hidden;
    scrollbar-width: none;
  }

  .nw {
    flex: none;
    width: 270px;
    display: flex;
    align-items: center;
    border-radius: var(--radius);
    background: var(--panel);
  }

  .nw .main {
    flex: 1;
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 10px 14px;
    border-radius: inherit;
    background: none;
    text-align: left;
    box-shadow: none;
  }

  .nw .x {
    flex: none;
    display: grid;
    place-items: center;
    width: 36px;
    height: 36px;
    min-height: 0;
    margin-right: 6px;
    padding: 0;
    border-radius: 50%;
    background: none;
    box-shadow: none;
    color: var(--soft);
  }

  .nw .ic {
    flex: none;
    display: grid;
    place-items: center;
    width: 36px;
    height: 36px;
    border-radius: 10px;
    color: var(--c);
    background: color-mix(in srgb, var(--c) 18%, transparent);
  }

  .nw .txt {
    display: flex;
    flex-direction: column;
    min-width: 0;
  }

  .nw b {
    font-size: 0.875rem;
    color: var(--text);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .nw .txt > span {
    font-size: 0.78rem;
    color: var(--soft);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  /* ---- the foot ---- */

  .foot {
    position: sticky;
    bottom: 0;
    z-index: 3;
    flex: none;
    height: 44px;
    margin-top: auto;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    padding: 0 var(--page-x);
    background: var(--rail);
    border-top: 1px solid var(--line);
    font-size: 0.8125rem;
    color: var(--soft);
  }

  .job {
    display: flex;
    align-items: center;
    gap: 10px;
    min-width: 0;
  }

  .job .what {
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .pb {
    flex: none;
    width: 110px;
    height: 5px;
    border-radius: 3px;
    background: color-mix(in srgb, var(--text) 12%, transparent);
  }

  .pb i {
    display: block;
    height: 100%;
    border-radius: inherit;
    background: var(--amber);
  }

  .num {
    font-variant-numeric: tabular-nums;
  }

  .hints {
    flex: none;
    display: flex;
    align-items: center;
    gap: 16px;
    transition: opacity var(--medium);
  }

  /* The controller's buttons, faint while a finger is in use. */
  :global(html:not([data-input="pad"])) .hints {
    opacity: 0.45;
  }

  .hints > span {
    display: inline-flex;
    align-items: center;
    gap: 6px;
  }

  /* A button of the controller: its letter in the middle of a round (or, for LB and RB, a pill)
     face, by the grid, not by the font's line height. */
  .glyph {
    flex: none;
    display: inline-grid;
    place-items: center;
    width: 22px;
    height: 22px;
    border-radius: 50%;
    background: color-mix(in srgb, var(--text) 14%, transparent);
    color: var(--text);
    font-family: var(--font-sans);
    font-size: 0.6875rem;
    font-weight: 800;
    line-height: 1;
    font-variant-numeric: tabular-nums;
  }

  .glyph.wide {
    width: auto;
    padding: 0 6px;
    border-radius: 6px;
  }

  .glyph.a {
    background: var(--amber);
    color: var(--bar-ink-deep);
  }

  .go.primary .glyph.a {
    background: var(--bar-ink-deep);
    color: var(--amber);
  }

  /* A low screen (a phone held sideways): a lower banner and smaller tiles, the shelf in sight. */
  @media (max-height: 500px) {
    .hero {
      --hero-h: 196px;
    }

    .body {
      bottom: 12px;
      gap: 5px;
    }

    h1 {
      font-size: 1.4rem;
    }

    .acts button {
      height: 38px;
    }

    .card {
      top: 12px;
    }

    .shelf h2 {
      margin: 2px 0 4px;
    }

    .tiles {
      --tile-w: 164px;
      min-height: 112px;
      padding-bottom: 6px;
    }

    .foot {
      height: 38px;
    }
  }
</style>
