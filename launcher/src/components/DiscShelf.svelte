<script lang="ts">
  // Android's Games tab as a shelf of discs (user decision, after Cartouche): every game a disc on a
  // wheel at the left, the one picked large and swaying, its neighbours above and below; beside it
  // its name, where its last save is, and its buttons. Up and down (or a drag) turn the wheel, A plays:
  // the disc goes into the console (DiscLaunch.svelte) and the game starts. X opens its mods, Y its
  // page. The rail stays on the left (Header.svelte).
  import { fly } from "svelte/transition";
  import { backOut, cubicIn, cubicOut } from "svelte/easing";
  import type { GameView, SaveSlotInfo } from "$lib/api";
  import { api } from "$lib/api";
  import { app, dismissCrashes, jobs, openVersion, pickVersion } from "$lib/app.svelte";
  import { grouped, pct } from "$lib/format";
  import { regionLabel, titleLines } from "$lib/labels";
  import { reduced } from "$lib/motion";
  import { modUpdates } from "$lib/modUpdates.svelte";
  import { news, type NewsItem } from "$lib/news";
  import { hasPort, offersStep, playerAct, playerState, playerVersion, playWarning } from "$lib/player.svelte";
  import { backgroundStatus } from "$lib/progress";
  import { when } from "$lib/reports";
  import { newestSave, savedSeconds } from "$lib/saves";
  import { ACCENTS, themeStyle } from "$lib/themes";
  import Disc from "./Disc.svelte";
  import DiscLaunch from "./DiscLaunch.svelte";
  import Icon from "./Icon.svelte";

  let { games }: { games: GameView[] } = $props();

  /** The game picked: the one picked last, else the first. */
  const at = $derived(
    Math.max(
      0,
      games.findIndex((g) => g.id === app.version?.split("/")[0]),
    ),
  );
  const game = $derived(games.at(at));
  const v = $derived(game ? playerVersion(game) : null);
  const next = $derived(game && v ? playerState(game, v) : null);
  const name = $derived(titleLines(game?.title ?? ""));
  const offers = $derived(offersStep(next));
  const playable = $derived(hasPort(v));
  const warning = $derived(playWarning(v));
  const crashed = $derived(game ? (app.crashes[game.id]?.recent ?? 0) > 0 : false);
  /** How far its code is rebuilt: said for a game not done yet. */
  const rebuilt = $derived(v?.progress && v.progress.percent < 100 ? v.progress.percent : null);
  const matched = $derived(
    app.config?.developer && game
      ? game.versions.map((o) => `${regionLabel(o.name)} ${o.progress ? pct(o.progress.percent) : "–"}`).join(" · ")
      : "",
  );

  /** Which way the wheel last turned: 1 down the list, -1 up; the words beside it follow. */
  let dir = $state(1);

  function pick(i: number) {
    const g = games.at(Math.min(games.length - 1, Math.max(0, i)));
    const gv = g ? playerVersion(g) : null;
    if (!gv || g?.id === game?.id) return;
    dir = i < at ? -1 : 1;
    pickVersion(gv.key);
  }

  /** The game's colour (palettes.json accents) for the motes behind the wheel; the launcher's gold
   *  for a game without one. It changes slowly as the wheel turns (--glow is registered). */
  const glow = $derived(game ? ACCENTS[game.id]?.["--glow"] : undefined);

  /** Motes of light that rise slowly behind the wheel: where each starts, how big, how slow. */
  const MOTES = Array.from({ length: 14 }, (_, i) => ({
    x: (i * 37 + 11) % 100,
    size: 3 + ((i * 7) % 5),
    seconds: 16 + ((i * 11) % 13),
    delay: -((i * 5.3) % 24),
    drift: ((i % 3) - 1) * 24,
  }));

  // ---- the last save: where the player left off ----

  /** The newest save of each version read, by serial; a read that failed is tried again later. */
  const saves = $state<Record<string, SaveSlotInfo | null>>({});
  const set = $derived(!!next && !["set-up", "installing", "other-disc"].includes(next.kind));
  $effect(() => {
    const serial = v?.serial;
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
  const lastWhen = $derived.by(() => {
    const seconds = last ? savedSeconds(last) : null;
    return seconds ? when(seconds) : null;
  });
  const continues = $derived(next?.kind === "play" && !!last);

  // ---- what changed ----

  const TONE: Record<NewsItem["kind"], string> = { update: "var(--amber)", mods: "var(--ok)", crash: "var(--err)" };
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

  // ---- the wheel ----

  /** How far a finger has dragged the wheel, in px, while it is down. */
  let drag = $state(0);
  let dragging = $state(false);
  let wheel = $state<HTMLElement | null>(null);
  /** The disc's size, read from the wheel (a CSS min of the screen's height). */
  let size = $state(400);
  $effect(() => {
    const el = wheel;
    if (!el) return;
    const read = () => (size = el.querySelector<HTMLElement>(".slot.on")?.offsetWidth ?? 400);
    read();
    const watch = new ResizeObserver(read);
    watch.observe(el);
    return () => {
      watch.disconnect();
    };
  });
  const step = $derived(size * 0.98);

  // Where the wheel stands, in discs (0 = the first game in the middle). It follows the game picked
  // on a spring moved frame by frame: a new turn asked mid-way keeps the wheel's speed and bends
  // its path, never a jump back to where a CSS transition would restart. 4 % past the mark.
  const STIFF = 77,
    DAMP = 12.7;
  let pos = $state(0);
  let speed = 0;
  let placed = false;
  let frame = 0;
  let then = 0;
  const goal = () => at - drag / step;

  function settle(now: number) {
    const dt = Math.min(1 / 30, (now - then) / 1000);
    then = now;
    const to = goal();
    speed += (-STIFF * (pos - to) - DAMP * speed) * dt;
    pos += speed * dt;
    if (Math.abs(pos - to) < 0.0008 && Math.abs(speed) < 0.002) {
      pos = to;
      speed = 0;
      frame = 0;
      return;
    }
    frame = requestAnimationFrame(settle);
  }

  $effect(() => {
    const to = goal();
    if (dragging || reduced || !placed) {
      // A finger moves the wheel itself; the first look has it in place.
      cancelAnimationFrame(frame);
      frame = 0;
      pos = to;
      speed = 0;
      placed = true;
      return;
    }
    if (!frame) {
      then = performance.now();
      frame = requestAnimationFrame(settle);
    }
  });
  $effect(() => () => {
    cancelAnimationFrame(frame);
  });

  /** Where disc `i` stands: its offset from the middle of the wheel, and how it shows there. */
  function place(i: number) {
    const d = i - pos;
    const far = Math.abs(d);
    const y = Math.sign(d) * (far <= 1 ? far * step : step + (far - 1) * step * 0.9);
    const scale = far <= 1 ? 1 - 0.23 * far : 0.77 - 0.1 * (far - 1);
    const opacity = far <= 1 ? 1 - 0.65 * far : Math.max(0, 0.35 - 0.35 * (far - 1));
    return `--y: ${y}px; --k: ${scale}; --r: ${d * 30}deg; opacity: ${opacity}`;
  }

  let start: { y: number; moved: boolean } | null = null;

  function down(e: PointerEvent) {
    if (e.pointerType === "mouse" && e.button !== 0) return;
    start = { y: e.clientY, moved: false };
  }

  function move(e: PointerEvent) {
    if (!start) return;
    const dy = e.clientY - start.y;
    if (!start.moved && Math.abs(dy) < 8) return;
    if (!start.moved) (e.currentTarget as HTMLElement).setPointerCapture(e.pointerId);
    start.moved = true;
    dragging = true;
    // Past the first and the last disc the wheel gives, but less and less.
    const ahead = at - dy / step;
    drag = ahead < 0 || ahead > games.length - 1 ? dy * 0.35 : dy;
  }

  function up() {
    if (!start) return;
    const moved = start.moved;
    start = null;
    dragging = false;
    if (!moved) return;
    const target = Math.round(at - drag / step);
    drag = 0;
    pick(target);
  }

  /** A disc pressed: the one picked plays (or does its step); another is picked. */
  function pressDisc(i: number) {
    if (i !== at) pick(i);
    else primary();
  }

  /** The controller's focus picks the disc it lands on; it stays on the disc as the wheel turns. */
  function focusDisc(i: number) {
    if (document.documentElement.dataset.input === "pad" && i !== at) pick(i);
  }

  // ---- what A does ----

  let launching = $state<{ game: string; version: string; title: string; from: DOMRect } | null>(null);

  function primary() {
    if (!v || !next || !game) return;
    if (!offers) {
      openVersion(v.key);
      return;
    }
    const disc = wheel?.querySelector<HTMLElement>(".slot.on .roll");
    if (next.kind === "play" && !reduced && disc) {
      launching = { game: game.id, version: v.name, title: game.title, from: disc.getBoundingClientRect() };
      return;
    }
    void playerAct(v, next);
  }

  /** The screen is black: the game starts; the console steps aside once the game has covered it. */
  function launched() {
    if (v && next) void playerAct(v, next);
    setTimeout(() => (launching = null), 1200);
  }

  function openMods() {
    if (!v || !playable || launching || document.querySelector('[role="dialog"], [role="menu"]')) return;
    pickVersion(v.key);
    app.page = "mods";
  }

  function openPage() {
    if (v && !launching) openVersion(v.key);
  }

  const job = $derived(
    backgroundStatus(
      jobs.list.find((j) => j.state === "running"),
      jobs.list.filter((j) => j.state === "queued").length,
      null,
    ),
  );

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

  /** The words beside the disc come in one after another, from the side the wheel turns from, while
   *  the last game's leave the other way: the two cross, so the place is never empty. */
  const enter = (i: number) => ({
    y: 18 * dir,
    duration: reduced ? 0 : 460,
    delay: reduced ? 0 : 70 + 45 * i,
    easing: cubicOut,
  });
  const leave = () => ({ y: -14 * dir, duration: reduced ? 0 : 150, easing: cubicIn });
</script>

{#if game && v && next}
  <div class="shelf" bind:this={root} style={glow ? `--glow: ${glow}` : undefined}>
    <div class="stage">
      <div class="backdrop" aria-hidden="true">
        {#if !reduced}
          {#each MOTES as m, i (i)}
            <span
              class="mote"
              style={`left: ${m.x}%; width: ${m.size}px; height: ${m.size}px; animation-duration: ${m.seconds}s; animation-delay: ${m.delay}s; --drift: ${m.drift}px`}
            ></span>
          {/each}
        {/if}
      </div>
      <div
        class="wheel"
        class:dragging
        bind:this={wheel}
        onpointerdown={down}
        onpointermove={move}
        onpointerup={up}
        onpointercancel={up}
        aria-label="Your games"
        role="group"
      >
        {#each games as g, i (g.id)}
          {@const gv = playerVersion(g)}
          {@const near = Math.abs(i - at) <= 1}
          <button
            class="slot"
            class:on={i === at}
            class:away={i === at && !!launching}
            style={place(i)}
            inert={!near}
            aria-label={i === at ? `${g.title}: ${next.status}` : g.title}
            onclick={() => {
              pressDisc(i);
            }}
            onfocus={() => {
              focusDisc(i);
            }}
          >
            <span class="roll" style={`animation-delay: ${Math.abs(i - at) * 90}ms`}>
              <Disc game={g.id} version={gv?.name ?? "ntsc"} title={g.title} idle dim={i !== at} />
            </span>
          </button>
        {/each}
      </div>

      {#key game.id}
        <div class="info" style={themeStyle(game.id)} out:fly={leave()}>
          <p class="over" in:fly={enter(0)}>
            {game.year ? `${game.year} · ` : ""}{regionLabel(v.name)}
          </p>
          <h1 in:fly={enter(1)}>
            <span>{name.series}</span>{#if name.game}<span>{name.game}</span>{/if}
          </h1>
          <div class="facts" in:fly={enter(2)}>
            {#if last}
              <p class="save">
                {#if last.planetName}<b>{last.planetName}</b> · slot {last.slotIndex + 1}{/if}
                {#if last.bolts !== null}· <b>{grouped(last.bolts)}</b> bolts{/if}
                {#if lastWhen}· {lastWhen}{/if}
              </p>
            {:else if rebuilt !== null && !playable}
              <p class="save">Not playable yet · <b>{pct(rebuilt)}</b> of its code rebuilt</p>
            {:else}
              <p class={`save ${next.tone}`}>{next.status}</p>
            {/if}
            {#if crashed}<p class="note err">It crashed last time: a report is on its page.</p>{/if}
            {#if warning}<p class="note warn" title={warning}>Early and unstable</p>{/if}
            {#if last?.mods?.length}<p class="note">
                Saved with {last.mods.length} mod{last.mods.length === 1 ? "" : "s"}
              </p>{/if}
            {#if matched}<p class="note mono">{matched} matched</p>{/if}
          </div>
          <div class="acts" in:fly={enter(3)}>
            {#if offers}
              <button class="go" class:primary={next.primary} onclick={primary}>
                <span class="glyph a" aria-hidden="true">A</span>{continues ? "Continue" : next.label}
              </button>
            {/if}
            <button onclick={openPage}><span class="glyph" aria-hidden="true">Y</span>Game page</button>
            {#if playable}
              <button onclick={openMods}><span class="glyph" aria-hidden="true">X</span>Mods</button>
            {/if}
          </div>
        </div>
      {/key}

      {#if changes.length}
        <div class="news" role="group" aria-label="What's new">
          {#each changes as n, i (n.key)}
            <span
              class="chip"
              style={`--c: ${TONE[n.kind]}`}
              in:fly={{ y: -10, duration: 420, delay: 300 + 60 * i, easing: backOut }}
            >
              <button
                class="main"
                onclick={() => {
                  follow(n);
                }}><i></i>{n.title}</button
              >
              {#if n.kind === "crash" && n.game}
                {@const crashedGame = n.game}
                <button class="x" aria-label="Dismiss" onclick={() => void dismissCrashes(crashedGame)}
                  ><Icon name="x" size={12} /></button
                >
              {/if}
            </span>
          {/each}
        </div>
      {/if}
    </div>

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
        <span><span class="glyph">↕</span>Games</span>
        <span><span class="glyph wide">LB</span><span class="glyph wide">RB</span>Tabs</span>
      </div>
    </footer>

    {#if launching}
      <DiscLaunch
        game={launching.game}
        version={launching.version}
        title={launching.title}
        from={launching.from}
        ondone={launched}
        oncancel={() => (launching = null)}
      />
    {/if}
  </div>
{/if}

<style>
  .shelf {
    --spring: cubic-bezier(0.34, 1.45, 0.64, 1);
    --ease-out: cubic-bezier(0.22, 1, 0.36, 1);
    position: relative;
    transition: --glow 900ms var(--ease-out);
    min-height: 100%;
    display: flex;
    flex-direction: column;
  }

  /* clip, not hidden: a disc taking the focus must not scroll the stage under the wheel. */
  .stage {
    position: relative;
    flex: 1 0 360px;
    overflow: clip;
  }

  /* Behind everything: motes of light in the game's colour rising slowly; no picture, no glow. */
  .backdrop {
    position: absolute;
    inset: 0;
    overflow: hidden;
    pointer-events: none;
  }

  .mote {
    position: absolute;
    bottom: -12px;
    border-radius: 50%;
    background: color-mix(in srgb, var(--glow) 70%, white);
    filter: blur(1px);
    opacity: 0;
    animation: rise linear infinite;
  }

  @keyframes rise {
    0% {
      opacity: 0;
      transform: translate(0, 0);
    }
    15% {
      opacity: 0.35;
    }
    80% {
      opacity: 0.2;
    }
    100% {
      opacity: 0;
      transform: translate(var(--drift), -78vh);
    }
  }

  /* ---- the wheel: the discs stand one above the other, the one picked in the middle ---- */

  .wheel {
    position: absolute;
    left: 0;
    top: 0;
    bottom: 0;
    width: 50%;
    touch-action: none;
    /* The discs above and below fade out at the edges instead of being cut. */
    mask-image: linear-gradient(to bottom, transparent 0, black 16%, black 84%, transparent 100%);
  }

  .slot {
    position: absolute;
    left: 50%;
    top: 50%;
    width: min(390px, 55vh);
    aspect-ratio: 1;
    margin: 0;
    padding: 0;
    border: 0;
    border-radius: 50%;
    background: none;
    box-shadow: none;
    min-height: 0;
    transform: translate(-50%, -50%) translateY(var(--y)) scale(var(--k)) rotate(var(--r));
    will-change: transform, opacity;
  }

  /* Taken into the console: the shelf keeps no copy of it. */
  .slot.away {
    opacity: 0 !important;
    transition: none;
  }

  .slot.on {
    z-index: 1;
  }

  .dragging .slot {
    transition: none;
  }

  .slot:focus-visible,
  :global(html[data-input="pad"]) .slot:focus {
    outline: none;
  }

  /* A pressed disc gives a little, and comes back with a bounce. */
  .slot:active .roll {
    scale: 0.97;
    transition-duration: 90ms;
  }

  .roll {
    display: block;
    width: 100%;
    height: 100%;
    transition: scale 300ms var(--spring);
    animation: roll-in 760ms var(--spring) both;
  }

  /* The discs roll in from the left as the tab opens. */
  @keyframes roll-in {
    from {
      opacity: 0;
      translate: -70% 0;
      rotate: -120deg;
    }
  }

  /* ---- the words beside it ---- */

  .info {
    position: absolute;
    left: 47%;
    right: var(--page-x);
    top: 50%;
    translate: 0 -50%;
    max-width: 560px;
  }

  .over {
    margin: 0 0 12px;
    font-size: 0.8125rem;
    text-transform: uppercase;
    letter-spacing: 0.1em;
    color: var(--dim);
  }

  h1 {
    margin: 0;
    display: flex;
    flex-direction: column;
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    font-size: 2.6rem;
    line-height: 1.1;
    color: var(--on-game);
  }

  .facts {
    margin-top: 16px;
  }

  .facts p {
    margin: 0;
  }

  .save {
    font-size: 1.0625rem;
    color: var(--soft);
  }

  .save b {
    color: var(--text);
    font-weight: 600;
  }

  .save.ok {
    color: var(--ok);
  }

  .save.warn {
    color: var(--amber);
  }

  .note {
    margin-top: 6px !important;
    font-size: 0.8125rem;
    color: var(--dim);
  }

  .note.err {
    color: var(--err);
  }

  .note.warn {
    color: var(--amber);
  }

  .note.mono {
    font-variant-numeric: tabular-nums;
  }

  .acts {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    margin-top: 24px;
  }

  .acts button {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    height: 46px;
    padding: 0 18px 0 12px;
    border-radius: var(--radius);
    background: var(--panel);
    color: var(--text);
    font-weight: 600;
    box-shadow: none;
    transition:
      transform 300ms var(--spring),
      background var(--fast);
  }

  /* Pressed: down at once, back with a bounce. */
  .acts button:active {
    transform: scale(0.95);
    transition-duration: 90ms;
  }

  /* Where the controller's focus is, the button breathes. */
  :global(html[data-input="pad"]) .acts button:focus {
    animation: breathe 1.6s ease-in-out infinite alternate;
  }

  @keyframes breathe {
    to {
      transform: scale(1.03);
    }
  }

  .go.primary {
    padding-right: 24px;
    background: var(--amber);
    color: var(--bar-ink-deep);
    font-weight: 700;
  }

  /* ---- what changed ---- */

  .news {
    position: absolute;
    top: 14px;
    right: var(--page-x);
    display: flex;
    gap: 8px;
  }

  .chip {
    display: inline-flex;
    align-items: center;
    border-radius: 99px;
    background: var(--panel);
  }

  .chip button {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    min-height: 0;
    padding: 6px 12px;
    border-radius: 99px;
    background: none;
    box-shadow: none;
    font-size: 0.8125rem;
    color: var(--text);
    transition: transform 300ms var(--spring);
  }

  .chip button:active {
    transform: scale(0.94);
    transition-duration: 90ms;
  }

  .chip i {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--c);
  }

  .chip .x {
    padding: 6px 10px 6px 0;
    color: var(--soft);
  }

  /* ---- the foot ---- */

  .foot {
    position: sticky;
    bottom: 0;
    z-index: 3;
    flex: none;
    height: 44px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    padding: 0 var(--page-x);
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
    transition: width 400ms var(--ease-out);
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

  :global(html:not([data-input="pad"])) .hints {
    opacity: 0.45;
  }

  .hints > span {
    display: inline-flex;
    align-items: center;
    gap: 6px;
  }

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

  /* A phone held sideways: a smaller title, the buttons closer. */
  @media (max-height: 500px) {
    h1 {
      font-size: 1.85rem;
    }

    .acts {
      margin-top: 14px;
    }

    .acts button {
      height: 38px;
    }

    .foot {
      height: 38px;
    }
  }

  @media (prefers-reduced-motion: reduce) {
    .slot,
    .roll {
      transition: none;
      animation: none;
    }
  }
</style>
