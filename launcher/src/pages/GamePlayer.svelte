<script lang="ts">
  // A game's page. One main button, always in the same place,
  // says the next thing to do (set up from your disc, build, play); what is done
  // less often is in the menu beside it. Below: the saves and, beside them, Help
  // (a recent crash, the logs); the level editor is under Mods. Each button runs
  // the actions the Develop tab lists (launcher/actions.json), found by their ids.
  // With developer tools on, one row of chips under the band leads there.
  import CountUp from "$components/CountUp.svelte";
  import CrashNotice from "$components/CrashNotice.svelte";
  import DifferencesBlock from "$components/DifferencesBlock.svelte";
  import DiscSwitch from "$components/DiscSwitch.svelte";
  import FolderPrompt from "$components/FolderPrompt.svelte";
  import GameShortcuts from "$components/GameShortcuts.svelte";
  import GamesFrame from "$components/GamesFrame.svelte";
  import Icon from "$components/Icon.svelte";
  import { toolFailure } from "$lib/toolErrors";
  import JobProgress from "$components/JobProgress.svelte";
  import ProgressBar from "$components/ProgressBar.svelte";
  import Menu from "$components/Menu.svelte";
  import ModsSwitch from "$components/ModsSwitch.svelte";
  import ReportsBlock from "$components/ReportsBlock.svelte";
  import SaveManagerBlock from "$components/SaveManagerBlock.svelte";
  import { api, onAndroid, type ActionView, type ReportEntry } from "$lib/api";
  import { app, clock, currentVersion, guard, jobs, openDevelop, run } from "$lib/app.svelte";
  import { choiceFor, programOf } from "$lib/discChoice";
  import { gameMenu, pageAsks, startsPlay } from "$lib/gameMenu.svelte";
  import { pct } from "$lib/format";
  import { SITE_URL } from "$lib/links";
  import { titleLines } from "$lib/labels";
  import { gameArt, hideMissing } from "$lib/media";
  import { arrive } from "$lib/motion";
  import { tick } from "svelte";
  import { fly } from "svelte/transition";
  import { place, when } from "$lib/reports";
  import { isBuilt, playerAct, playerState, playerVersion } from "$lib/player.svelte";
  import { themeStyle } from "$lib/themes";

  // One game, whatever the region: the page plays the version your disc is (playerVersion),
  // always. Each disc's decompilation and actions are in the Develop tab.
  const developer = $derived(app.config?.developer ?? false);
  const opened = $derived(currentVersion());
  const game = $derived(app.library?.games.find((g) => g.id === opened?.game) ?? null);
  const v = $derived(game ? playerVersion(game) : opened);
  const scope = $derived({ kind: "version" as const, key: v?.key ?? "" });
  // Developer tools on and two of the game's discs found: the switch beside Play (DiscSwitch).
  const choice = $derived(developer && game ? choiceFor(app.discChoices, game.id) : null);
  const action = (id: string): ActionView | null => v?.actions.find((a) => a.id === id && a.thisPlatform) ?? null;

  const rebuild = $derived(action("port-rebuild"));
  const levels = $derived(action("port-levels"));
  const levelsCheck = $derived(action("port-levels-check"));
  /** The levels were read from the disc and, where the version has the check, the game read them all. */
  const levelsReady = $derived(levelsCheck ? levelsCheck.done : !!levels?.done);

  // Each game's still picture, under its band and behind the page.
  const art = $derived(v ? gameArt(v.game) : null);

  /** The game has a PC port to play (or to build and play); without one, the page says how far it is. */
  const portReady = $derived(!!action("play") && action("play")?.state !== "planned");
  /** What Play warns of before it starts (actions.json `warning`: a game far from finished), said on the page too. */
  const playWarning = $derived(action("play")?.warning ?? null);

  const next = $derived(v && game ? playerState(game, v) : null);
  /** Built by an OpenRAC from before an update that changed it (port_build.rs), and nothing runs. */
  const updateReady = $derived(!!v?.portOutdated && !!rebuild?.runnable && next?.kind === "play");
  const setUp = $derived(game?.install?.extracted ?? null);
  const built = $derived(!!v && !!game && isBuilt(game, v));
  /** The steps are shown while there is one left that the player can take. */
  const showSteps = $derived(!!next && next.kind !== "play" && next.kind !== "playing" && next.kind !== "return");

  /** The last step towards playing that stopped with an error (set-up, build, levels), while nothing
   *  runs: the page says so under the main button, with the way to what it printed. */
  const stopped = $derived.by(() => {
    if (!v || next?.job) return null;
    const ids = [
      `install:${v.game}`,
      "port-build",
      "port-rebuild",
      "port-levels",
      "port-levels-check",
      "overrides-stack",
    ];
    const mine = jobs.list.filter(
      (j) => ids.includes(j.actionId) && (j.scope.kind === "repository" || j.scope.key === v.key),
    );
    const last = mine.at(-1);
    return last?.state === "failed" ? last : null;
  });

  /** The newest log or crash report: "Last played today at 18:15 on Batalia", under the main button. */
  let last = $state<ReportEntry | null>(null);
  /** The newest read wins: switching games while one is read drops the older answer. */
  let reading = 0;
  $effect(() => {
    // Again whenever the game ends (app.reportsChanged) or another game is shown.
    const game = v?.game;
    const mine = ++reading;
    last = null;
    if (!game || !setUp) return;
    void api
      .reportsList(game)
      .then((entries) => {
        if (mine === reading) last = entries[0] ?? null;
      })
      .catch(() => undefined);
  });
  const lastPlayed = $derived.by(() => {
    if (!last) return "";
    const day = when(last.time);
    return `Last played ${day.charAt(0).toLowerCase()}${day.slice(1)} ${place(last)}`;
  });

  /** Android's Help, folded at the page's foot until opened. */
  let helpOpen = $state(false);

  /** The menu's way to the logs: Help, with its logs unfolded. */
  async function showLogs() {
    helpOpen = true;
    await tick();
    document.getElementById("logs-list")?.setAttribute("open", "");
    const logs = document.getElementById("logs");
    logs?.scrollIntoView({ behavior: "smooth", block: "start" });
    logs?.focus({ preventScroll: true });
  }

  // Opened from Games' "..." menu on its logs.
  $effect(() => {
    if (pageAsks.logs && setUp) {
      pageAsks.logs = false;
      void showLogs();
    }
  });

  /** The Mods switch under Play while Play would start the game (ModsSwitch). */
  const startsGame = $derived(startsPlay(next));

  /** What is done less often than playing, in the menu beside the main button. */
  const menu = $derived(game && v ? gameMenu(game, v, next, () => void showLogs()) : []);
</script>

<GamesFrame>
  {#if !v || !game || !next}
    <div class="page">
      <p class="muted">That game is not in this OpenRAC folder.</p>
      <button
        onclick={() => {
          app.version = null;
          app.page = "library";
        }}>Open the first game</button
      >
    </div>
  {:else}
    <!-- Keyed by the game: switching games builds its page afresh (its saves are read again) and
         brings it in, the band first, then what is under it. -->
    {#key v.key}
      <div class="page" style={themeStyle(v.game)}>
        {#if art}
          <div class="page-ambient-bg" aria-hidden="true">
            <img src={art} alt="" decoding="sync" onerror={hideMissing} />
          </div>
        {/if}

        <FolderPrompt primary={false} />
        {#if app.library?.actionsError}
          <div class="panel notice">
            <Icon name="warn" />
            <p class="grow">The games show, but no actions: {app.library.actionsError}</p>
          </div>
        {/if}
        {#if playWarning}
          <div class="panel notice" role="alert">
            <Icon name="warn" />
            <p class="grow">{playWarning}</p>
          </div>
        {/if}
        {#if updateReady && rebuild}
          <!-- Recommended, never forced: Play still starts the build there is. -->
          <div class="panel notice" role="status">
            <Icon name="warn" />
            <p class="grow">
              An update is ready. Rebuild {v.title} to play the latest version: it takes a few minutes, and your saves and
              levels are kept.
            </p>
            <button class="primary" onclick={() => void run(scope, rebuild)}>Rebuild now</button>
          </div>
        {/if}
        {#if app.stopped?.game === v.game}
          <CrashNotice notice={app.stopped} title={v.title} />
        {/if}

        <!-- |global like the rest of the page: a local intro inside {#key} could leave the band at its
             starting opacity when the key changed (switching discs), so the band stayed invisible. -->
        <section class="hero" aria-label={v.title} in:fly|global={arrive()}>
          {#if art}
            <div class="hero-backdrop" aria-hidden="true">
              <img src={art} alt="" class="hero-bg" decoding="sync" fetchpriority="high" onerror={hideMissing} />
              <div class="hero-overlay"></div>
            </div>
          {/if}
          <div class="title box">
            {#if onAndroid}
              <!-- As on Games' banner: the series, then the game's own name on its own line. -->
              {@const name = titleLines(v.title)}
              <span>{name.series}</span>{#if name.game}<span>{name.game}</span>{/if}
            {:else}
              {v.title}
            {/if}
            {#if game.year}<small>{game.year}</small>{/if}
          </div>

          {#if onAndroid}
            <!-- Android: Play, the Mods switch and the menu are beside the disc on Games; the band only
                 leads back there, and says what stopped. -->
            <button class="back" onclick={() => (app.page = "library")}><Icon name="arrow" size={18} />Games</button>
            {#if stopped}
              <p class="stopped">
                <Icon name="warn" size={14} />{stopped.title} stopped: {toolFailure(stopped.lines).title.toLowerCase()}.
                <button class="link" onclick={() => (app.page = "tasks")}>See what it printed</button>
              </p>
            {/if}
          {:else}
            <div class="go">
              <div class="go-row">
                <button
                  class="big"
                  class:primary={next.primary}
                  disabled={next.disabled}
                  onclick={() => void playerAct(v, next)}
                >
                  {#if next.job}<span class="spinner"></span>{:else}<Icon name={next.icon} size={22} />{/if}{next.label}
                </button>
                <!-- An empty menu is not shown, but keeps its place: the button does not move when it fills. -->
                {#if menu.length}<Menu items={menu} />{:else}<span class="menu-slot" aria-hidden="true"></span>{/if}
              </div>
              {#if choice}<DiscSwitch {game} {choice} busy={!!next.job} />{/if}
              {#if startsGame}
                <ModsSwitch version={v} setUp={!!setUp} onopen={() => (app.page = "mods")} />
              {/if}
              {#if next.hint && !next.job}<p class="line">{next.hint}</p>{/if}
              {#if lastPlayed && next.kind === "play"}<p class="line last">{lastPlayed}</p>{/if}
              {#if stopped}
                <p class="stopped">
                  <Icon name="warn" size={14} />{stopped.title} stopped: {toolFailure(
                    stopped.lines,
                  ).title.toLowerCase()}.
                  <button class="link" onclick={() => (app.page = "tasks")}>See what it printed</button>
                </p>
              {/if}
            </div>
          {/if}

          {#if next.job}<JobProgress job={next.job} now={clock.now} />{/if}

          {#if showSteps}
            <ol class="steps" aria-label="Setting the game up">
              <li class:done={!!setUp}>Disc set up</li>
              {#if next.kind !== "unavailable" && (next.kind !== "set-up" || next.primary)}
                <li class:done={built}>Game built</li>
                {#if levels}<li class:done={levelsReady}>Levels ready</li>{/if}
              {/if}
            </ol>
          {/if}
        </section>

        {#if developer}
          <!-- A contributor's facts about this disc, as chips; the rest is in Develop. -->
          <div class="chips" aria-label="Developer details" in:fly|global={arrive(60)}>
            <span class="chip mono">{v.key}</span>
            {#if v.serial}<span class="chip mono">{v.serial}</span>{/if}
            {#if v.progress}<span class="chip mono">{pct(v.progress.percent)} matched</span>{/if}
            <button
              class="chip link"
              onclick={() => {
                openDevelop(v.key);
              }}>Open in Develop ›</button
            >
          </div>
        {/if}
        {#if !portReady}
          <!-- Every game has this page; one without a port yet says so, and how far it is. -->
          <section class="panel coming" aria-labelledby="coming-title" in:fly|global={arrive(90)}>
            <h2 id="coming-title">Not playable yet</h2>
            <p class="muted">
              OpenRAC is rebuilding {v.title} so it runs natively on your computer. Play appears here when it is ready{setUp
                ? "; your disc is already set up for it."
                : ". You can set your disc up now, so it is ready too."}
            </p>
            {#if v.progress}
              <div class="progress">
                <span class="grow">Code rebuilt so far</span>
                <strong><CountUp value={v.progress.percent} format={pct} /></strong>
              </div>
              <ProgressBar percent={v.progress.percent} label={`${v.title}: code rebuilt so far`} />
            {/if}
            <p>
              <button class="link" onclick={() => void guard(api.openUrl(SITE_URL))}
                >Follow the progress on openrac.dev</button
              >
            </p>
          </section>
        {/if}
        {#if !!setUp || portReady}
          <!-- Android: the saves alone here; Help folds at the page's foot, as a player seldom needs it. -->
          <div class="below" class:pair={!!setUp && !onAndroid} in:fly|global={arrive(90)}>
            <SaveManagerBlock version={v} />
            {#if setUp && !onAndroid}<ReportsBlock game={v.game} program={choice ? programOf(v.key) : null} />{/if}
          </div>
        {/if}
        {#if portReady && !onAndroid}
          <!-- Start this game without the launcher's window: Steam's own entry, a desktop shortcut. -->
          <div in:fly|global={arrive(100)}><GameShortcuts version={v.key} /></div>
        {/if}
        {#if portReady}
          <!-- What the port does otherwise than the PS2 game; the game fixes switch back. Folded on
               Android, under a line of what it holds. -->
          <DifferencesBlock version={v} foldable={onAndroid} />
        {/if}
        {#if setUp && onAndroid}
          <div in:fly|global={arrive(120)}>
            <ReportsBlock game={v.game} program={choice ? programOf(v.key) : null} foldable bind:unfolded={helpOpen} />
          </div>
        {/if}
      </div>
    {/key}
  {/if}
</GamesFrame>

<style>
  /* What is left to do, as one line: done steps are ticked, the rest are plain. */
  .steps {
    display: flex;
    flex-wrap: wrap;
    gap: 6px 22px;
    margin: 0;
    padding: 0;
    list-style: none;
    font-size: 0.8125rem;
    color: var(--tx2);
  }

  .steps li {
    display: flex;
    align-items: center;
    gap: 7px;
  }

  .steps li::before {
    content: "";
    width: 14px;
    height: 14px;
    border-radius: 50%;
    background: color-mix(in srgb, currentColor 14%, transparent);
    opacity: 0.7;
  }

  .steps li.done {
    color: var(--ok);
  }

  .steps li.done::before {
    background: var(--ok)
      url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 14 14'%3E%3Cpath d='M3.5 7.3l2.2 2.2 4.8-5' fill='none' stroke='%2312121a' stroke-width='1.8' stroke-linecap='round' stroke-linejoin='round'/%3E%3C/svg%3E")
      center / 14px no-repeat;
    opacity: 1;
  }

  .page {
    position: relative;
    max-width: var(--page-max, 920px);
    margin: 0 auto;
    padding: var(--page-top, 20px) var(--page-x, 28px) var(--page-bottom, 60px);
    display: flex;
    flex-direction: column;
    gap: var(--page-gap, 20px);
  }

  .page-ambient-bg {
    position: absolute;
    top: -20px;
    left: 0;
    width: 100%;
    height: 480px;
    pointer-events: none;
    z-index: 0;
    overflow: hidden;
    opacity: 0.2;
    /* Screen: the picture only adds light, so the light under the bar (app.css body::after) still
       shows through it. It fades out at its edges rather than under a coat of the page's colour,
       which covered that light. */
    mix-blend-mode: screen;
    mask-image: radial-gradient(ellipse 70% 75% at 50% 20%, black 25%, transparent 85%);
  }

  .page-ambient-bg img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  /* Over the game's picture the menu's button is a dark shade, so it shows on any picture. */
  .hero :global(.trigger) {
    background: rgb(0 0 0 / 0.45);
  }

  .hero {
    position: relative;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
    padding: 34px 28px 30px;
    border-radius: var(--radius);
    background:
      radial-gradient(90% 70% at 70% 0, var(--from), transparent 70%), linear-gradient(160deg, var(--from), var(--to));
    box-shadow: var(--shadow);
    overflow: hidden;
  }

  .hero-backdrop {
    position: absolute;
    inset: 0;
    pointer-events: none;
    overflow: hidden;
    z-index: 0;
  }

  .hero-bg {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    object-fit: cover;
    /* Above the middle: the band is wider than the picture, and the faces are in its upper part. */
    object-position: 50% 25%;
    opacity: 0.6;
    /* The game's picture settles into the band as the page opens. */
    animation: settle 1.4s cubic-bezier(0.2, 0.8, 0.2, 1) both;
    will-change: transform, opacity;
  }

  @keyframes settle {
    from {
      opacity: 0;
      transform: scale(1.08);
    }
  }

  /* Where the page slides in whole (app.css's view transition), its pictures come with it. */
  :global(html[data-motion="slides"]) .hero-bg,
  :global(html[data-motion="slides"]) .page-ambient-bg img {
    animation: none;
  }

  /* The picture behind the page fades in. */
  .page-ambient-bg img {
    animation: ambient 0.8s ease-out both;
  }

  @keyframes ambient {
    from {
      opacity: 0;
    }
  }

  .hero-overlay {
    position: absolute;
    inset: 0;
    background:
      radial-gradient(
        ellipse 90% 80% at 50% 20%,
        transparent 20%,
        color-mix(in srgb, var(--ink) 70%, transparent) 100%
      ),
      linear-gradient(
        180deg,
        color-mix(in srgb, var(--ink) 30%, transparent) 0%,
        color-mix(in srgb, var(--ink) 65%, transparent) 100%
      );
  }

  /* :global: a component's root (JobProgress) carries no class of this page's, and would stay
     under the picture and its shade. */
  .hero > :global(:not(.hero-backdrop)) {
    position: relative;
    z-index: 1;
  }

  .box {
    background: var(--box);
    color: var(--tx);
    text-align: center;
    box-shadow: var(--shadow-sm);
  }

  .title {
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    font-size: clamp(26px, 3.4vw, 40px);
    line-height: 1.15;
    padding: 18px 30px;
  }

  .title small {
    display: block;
    margin-top: 6px;
    font-size: 0.8125rem;
    letter-spacing: 0.04em;
    color: var(--tx2);
  }

  .go {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 10px;
    margin-top: 8px;
    max-width: 100%;
  }

  .go-row {
    display: flex;
    align-items: center;
    gap: 10px;
  }

  /* The menu's place while it has nothing in it (Menu.svelte's trigger is 44 px). */
  .menu-slot {
    flex: none;
    width: 44px;
    height: 44px;
  }

  .big {
    /* Play, Rebuild and Stop are one width: the button keeps its place from one to the next. */
    min-width: 13.5rem;
    justify-content: center;
    white-space: nowrap;
    font-size: 1.375rem;
    font-weight: 700;
    padding: 16px 46px;
    border-radius: var(--radius-sm);
    gap: 12px;
    transition:
      transform 0.15s ease,
      box-shadow 0.2s ease,
      filter 0.15s ease;
  }

  /* The main button lifts towards the pointer, and gives under it. */
  @media (hover: hover) {
    :global(html:where(:not([data-input="pad"]))) .big.primary:hover:not(:disabled) {
      transform: translateY(-2px);
      box-shadow: 0 12px 28px color-mix(in srgb, var(--bar-top) 38%, transparent);
    }
  }

  .big:active:not(:disabled) {
    transform: translateY(0) scale(0.98);
    box-shadow: none;
  }

  .last {
    color: var(--lav);
  }

  /* ---- Android: the band in the look of the Games tab ----
     The picture whole and bright, sinking into the page on the left and at its foot; the name
     written on it in two lines, no box; the buttons rounded as Games' are. */
  :global(html[data-platform="android"]) .hero {
    min-height: 236px;
    align-items: end;
    padding: 26px 30px;
    border-radius: var(--radius);
    background: var(--ink);
  }

  :global(html[data-platform="android"]) .hero-bg {
    opacity: 1;
  }

  :global(html[data-platform="android"]) .hero-overlay {
    background:
      linear-gradient(90deg, var(--ink) 0%, color-mix(in srgb, var(--ink) 80%, transparent) 26%, transparent 60%),
      linear-gradient(0deg, color-mix(in srgb, var(--ink) 92%, transparent) 0%, transparent 55%),
      linear-gradient(
        270deg,
        color-mix(in srgb, var(--ink) 88%, transparent) 0%,
        color-mix(in srgb, var(--ink) 60%, transparent) 30%,
        transparent 50%
      );
  }

  :global(html[data-platform="android"]) .hero > .title.box {
    display: flex;
    flex-direction: column;
    padding: 0;
    border: 0;
    background: none;
    box-shadow: none;
    text-align: left;
    font-size: 2rem;
    line-height: 1.1;
    color: var(--on-game);
    text-shadow: 0 2px 18px rgb(0 0 0 / 0.45);
  }

  :global(html[data-platform="android"]) .hero > .title small {
    order: -1;
    margin: 0 0 6px;
    font-family: var(--font-sans);
    font-size: 0.75rem;
    font-weight: 600;
    letter-spacing: 0.14em;
    color: var(--glow);
    text-shadow: none;
  }

  :global(html[data-platform="android"]) .hero .line {
    text-shadow: 0 1px 8px var(--ink);
  }

  /* Back to the shelf of discs, in the band's top left corner. */
  .hero > .back {
    position: absolute;
    top: 16px;
    left: 18px;
    display: inline-flex;
    align-items: center;
    gap: 4px;
    min-height: 40px;
    padding: 0 16px 0 10px;
    border-radius: 99px;
    background: rgb(0 0 0 / 0.45);
    color: var(--text);
    font-weight: 600;
    box-shadow: none;
    transition: transform 300ms cubic-bezier(0.34, 1.45, 0.64, 1);
  }

  .hero > .back:active {
    transform: scale(0.95);
    transition-duration: 90ms;
  }

  :global(html[data-platform="android"]) .hero .big {
    border-radius: var(--radius);
  }

  .notice {
    position: relative;
    z-index: 1;
    display: flex;
    align-items: center;
    gap: 12px;
    color: var(--amber);
  }

  .notice p {
    margin: 0;
    color: var(--text);
  }

  /* The last step that failed, under the main button: what stopped, and the way to its output. */
  .stopped {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 4px 8px;
    margin: 0;
    font-size: 0.8125rem;
    color: var(--err);
  }

  .stopped .link {
    font-size: 0.8125rem;
  }

  /* A game without a port yet: what is happening, how far it is, and where to follow it. */
  .coming {
    position: relative;
    z-index: 1;
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .coming p {
    margin: 0;
    max-width: 68ch;
  }

  .coming .progress {
    display: flex;
    align-items: baseline;
    gap: 12px;
    font-size: 0.875rem;
    color: var(--soft);
  }

  .coming .progress strong {
    font-family: var(--font-head);
    font-size: 1.125rem;
    color: var(--gold);
    font-variant-numeric: tabular-nums;
  }

  .link {
    padding: 0;
    min-height: 0;
    border: 0;
    background: none;
    box-shadow: none;
    color: var(--amber);
    font-weight: 600;
  }

  /* With developer tools on: one row of small chips, the last one a way to Develop. */
  .chips {
    position: relative;
    z-index: 1;
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
    margin-top: -6px;
  }

  .chip {
    font-size: 0.75rem;
    color: var(--soft);
    background: var(--control);
    border-radius: 7px;
    padding: 3px 9px;
    white-space: nowrap;
  }

  button.chip {
    font-family: var(--font-sans);
    font-weight: 600;
    color: var(--amber);
  }

  /* The saves and Help: side by side where the window is wide enough, else one under the other. */
  .below {
    position: relative;
    z-index: 1;
    display: grid;
    grid-template-columns: minmax(0, 1fr);
    gap: 16px;
    align-items: start;
  }

  @media (min-width: 1000px) {
    .below.pair {
      grid-template-columns: minmax(0, 1.35fr) minmax(0, 1fr);
    }
  }

  .line {
    font-size: 0.875rem;
    color: var(--tx2);
    text-align: center;
    max-width: 640px;
  }
</style>
