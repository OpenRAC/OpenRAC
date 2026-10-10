<script lang="ts">
  // A game as a player meets it: set it up from your own disc, play the
  // native port, edit its levels in Godot. Each button runs the same actions the
  // developer page lists (launcher/actions.json), found here by their ids.
  import Icon from "$components/Icon.svelte";
  import SaveManagerBlock from "$components/SaveManagerBlock.svelte";
  import { api, pickFile, type ActionView } from "$lib/api";
  import {
    app,
    cancel,
    currentVersion,
    guard,
    installGame,
    jobs,
    openVersion,
    refresh,
    run,
    runChain,
    saveConfig,
    toast,
    uninstallGame,
  } from "$lib/app.svelte";
  import { GODOT_URL } from "$lib/links";
  import { regionLabel } from "$lib/labels";
  import { themeStyle } from "$lib/themes";

  const v = $derived(currentVersion());
  const game = $derived(app.library?.games.find((g) => g.id === v?.game) ?? null);
  const scope = $derived({ kind: "version" as const, key: v?.key ?? "" });
  const action = (id: string): ActionView | null => v?.actions.find((a) => a.id === id && a.thisPlatform) ?? null;

  const play = $derived(action("play"));
  const build = $derived(action("port-build"));
  const playGodot = $derived(action("editor-preview"));
  const extract = $derived(action("editor-extract"));
  const bring = $derived(action("editor-import"));
  const edit = $derived(action("editor-open"));

  const GAME_MEDIA: Record<string, { bg: string; gif?: string; video?: string }> = {
    rac1: { bg: "/img/rac1-bg.webp", video: "/img/rac1-gameplay.mp4", gif: "/img/rac1-gameplay.gif" },
    rac2: { bg: "/img/gc-bg.webp", gif: "/img/rac2-gameplay.gif" },
    rac3: { bg: "/img/uya-bg.webp", gif: "/img/rac3-gameplay.gif" },
    rac4: { bg: "/img/deadlocked-bg.webp", gif: "/img/rac4-gameplay.gif" },
  };

  const media = $derived(v ? GAME_MEDIA[v.game] : null);

  const disc = $derived(v?.status.disc.state ?? "missing");
  /** The job of one of this version's actions that is queued or running, if any. */
  const jobOf = (ids: string[]) =>
    jobs.list.find(
      (j) =>
        j.scope.kind === "version" &&
        j.scope.key === v?.key &&
        ids.includes(j.actionId) &&
        (j.state === "queued" || j.state === "running"),
    );
  const playing = $derived(jobOf(["play"]));
  /** Building the game (the native port), if that is queued or running. */
  const building = $derived(jobOf(["port-build"]));
  /** Setting this game up from the disc, if that is queued or running. */
  const installing = $derived(
    jobs.list.find((j) => j.actionId === `install:${v?.game ?? ""}` && (j.state === "queued" || j.state === "running")),
  );
  /** Which build of this game the player's disc is, once it is set up. */
  const setUp = $derived(game?.install?.extracted ?? null);
  const otherVersion = $derived(
    setUp && setUp.version !== v?.key ? (game?.versions.find((other) => other.key === setUp.version) ?? null) : null,
  );
  const preparing = $derived(jobOf(["editor-extract", "editor-import"]));
  /** The game is built: compiled into the set-up (OpenGOAL's way), or the port's program is there to play. */
  const built = $derived(!!game?.install?.compiled || !!play?.runnable);

  let opening = $state(false);

  /** OpenGOAL's install: pick the disc image, extract and validate it. */
  async function setUpGame() {
    if (!v) return;
    await installGame(v.game, v.title);
    await refresh();
  }

  async function removeSetUp() {
    if (!v) return;
    const sure = confirm(
      `Remove what was set up from your ${v.title} disc? Your disc image and your saves are kept; set it up again any time.`,
    );
    if (sure) await uninstallGame(v.game);
  }

  async function chooseGodot() {
    if (!app.config) return;
    const picked = await pickFile("Godot 4");
    if (picked) await saveConfig({ ...app.config, godot: picked });
  }

  /** Extracts and imports the levels when the project is not there yet, then opens Godot. */
  async function editLevels() {
    if (!edit) return;
    opening = true;
    const steps = edit.runnable ? [edit] : [extract, bring, edit].filter((a) => a !== null);
    const done = await runChain(scope, steps);
    opening = false;
    if (done) toast("Godot is opening your levels.");
  }

  /** What an action still needs, in a player's words. */
  const needs = (a: ActionView | null) => (a?.blockers ?? []).map((b) => b.replace(/^needs /, ""));
  const lacksGodot = $derived(needs(edit).some((b) => b.startsWith("Godot")));
</script>

{#if !v || !game}
  <div class="page">
    <p class="muted">That game is not in this OpenRAC folder.</p>
    <button onclick={() => (app.page = "library")}>Back to the games</button>
  </div>
{:else}
  <div class="page" style={themeStyle(v.game)}>
    {#if media}
      <div class="page-ambient-bg" aria-hidden="true">
        <img src={media.bg} alt="" />
      </div>
    {/if}

    <button class="ghost back" onclick={() => (app.page = "library")}><Icon name="arrow" size={16} />All games</button>

    <section class="hero rise" aria-label={`${v.title} (${v.region})`}>
      {#if media}
        <div class="hero-backdrop" aria-hidden="true">
          <img src={media.gif ?? media.bg} alt="" class="hero-bg" />
          <div class="hero-overlay"></div>
        </div>
      {/if}
      <div class="title box">
        {v.title}
        <small>{game.year ? `${game.year} · ` : ""}{v.region}</small>
      </div>
      {#if game.versions.length > 1}
        <div class="versions" role="tablist" aria-label="Versions">
          {#each game.versions as other (other.key)}
            <button
              role="tab"
              aria-selected={other.key === v.key}
              class="box"
              class:on={other.key === v.key}
              onclick={() => {
                openVersion(other.key);
              }}>{regionLabel(other.name)}</button
            >
          {/each}
        </div>
      {/if}

      <div class="go">
        {#if installing}
          <button class="big" onclick={() => void cancel(installing)}>
            <span class="spinner"></span>{installing.state === "queued" ? "Waiting…" : "Stop"}
          </button>
          <p class="line clip">{installing.lines.at(-1)?.line ?? "Reading your disc…"}</p>
        {:else if !setUp}
          <button class="big primary" onclick={() => void setUpGame()}
            ><Icon name="disc" size={22} />Set up from your disc</button
          >
          <p class="line">
            Choose the image (.iso) of your own, legitimately obtained {game.title} disc. OpenRAC never downloads a game.
          </p>
        {:else if !play || play.state === "planned"}
          <button class="big" disabled><Icon name="play" size={22} />Not playable yet</button>
          <p class="line">
            {#if otherVersion}Your disc is the {otherVersion.region} version ({setUp.serial}).{:else}Your disc is set up
              ({setUp.serial}).{/if}
            The native port is not built yet: its decompilation comes first.
          </p>
        {:else if otherVersion}
          <button class="big" disabled><Icon name="play" size={22} />Play</button>
          <p class="line">Your disc is the {otherVersion.region} version: open that version to play.</p>
        {:else if playing}
          <button class="big" onclick={() => void cancel(playing)}>
            <span class="spinner"></span>{playing.state === "queued" ? "Waiting…" : "Stop the game"}
          </button>
          <p class="line clip">
            {playing.lines.at(-1)?.line ?? "Getting the game ready (the first time takes a minute)…"}
          </p>
        {:else if building}
          <button class="big" onclick={() => void cancel(building)}>
            <span class="spinner"></span>{building.state === "queued" ? "Waiting…" : "Stop the build"}
          </button>
          <p class="line clip">{building.lines.at(-1)?.line ?? "Building the game for this computer…"}</p>
        {:else if play.runnable}
          <button class="big primary" onclick={() => void run(scope, play)}><Icon name="play" size={22} />Play</button>
          <p class="line">The decompiled game, built for this computer.</p>
          {#if build?.runnable}
            <button
              title="After updating OpenRAC or the decompilation: compiles again what changed"
              onclick={() => void run(scope, build)}><Icon name="build" size={16} />Rebuild</button
            >
          {/if}
        {:else if build?.runnable}
          <button class="big primary" onclick={() => void run(scope, build)}
            ><Icon name="build" size={22} />Build the game</button
          >
          <p class="line">Compiles the decompiled game for this computer: a few minutes the first time.</p>
        {:else}
          <button class="big" disabled><Icon name="play" size={22} />Play</button>
          <p class="line">Not ready: it needs {needs(play).join(", ")}.</p>
        {/if}
      </div>

      <ol class="steps" aria-label="Setting the game up">
        <li class:done={!!setUp}>Extract and check your disc</li>
        <li class:done={game.install?.decompiled}>
          Prepare the assets{#if !game.install?.decompiled}<small>not available yet</small>{/if}
        </li>
        <li class:done={built}>
          Build the game{#if !built}<small>{build ? "not built yet" : "not available yet"}</small>{/if}
        </li>
      </ol>
    </section>

    <section class="panel editor">
      <div class="grow">
        <h2>Level editor</h2>
        {#if !extract || extract.state === "planned"}
          <p class="muted">The level editor cannot open this game yet.</p>
        {:else if disc !== "found"}
          <p class="muted">Set up your disc first; the editor reads the levels from it.</p>
        {:else if lacksGodot}
          <p class="muted">
            The levels open in Godot 4, a free editor.
            <button class="link" onclick={() => void guard(api.openUrl(GODOT_URL))}>Get Godot</button>, then choose
            where it is.
          </p>
        {:else if preparing}
          <p class="muted clip">{preparing.title}: {preparing.lines.at(-1)?.line ?? "starting…"}</p>
        {:else if edit?.runnable}
          <p class="muted">Move, add or delete objects in any level and save. Your edits stay on this computer.</p>
        {:else}
          <p class="muted">
            The first time, the levels are read from your disc and prepared for Godot: about three minutes and 630 MB.
          </p>
        {/if}
      </div>
      {#if extract && extract.state !== "planned" && disc === "found"}
        {#if lacksGodot}
          <button onclick={() => void chooseGodot()}><Icon name="folder" size={16} />Choose Godot</button>
        {:else}
          <button class="primary" disabled={opening || !!preparing} onclick={() => void editLevels()}>
            {#if opening || preparing}<span class="spinner"></span>{:else}<Icon name="edit" size={16} />{/if}
            {edit?.runnable ? "Edit levels" : "Prepare and edit levels"}
          </button>
        {/if}
      {/if}
    </section>

    <SaveManagerBlock version={v} />

    <details class="panel more">
      <summary>More</summary>
      <div class="row wrap">
        {#if playGodot?.runnable}
          <button onclick={() => void run(scope, playGodot)}
            ><Icon name="play" size={16} />Preview a level in Godot</button
          >
        {/if}
        <button onclick={() => void guard(api.openPath(v.dir))}
          ><Icon name="folder" size={16} />Open the game's folder</button
        >
        {#if setUp}
          <button onclick={() => void removeSetUp()}><Icon name="x" size={16} />Remove the set-up</button>
        {/if}
        <button onclick={() => (app.page = "tasks")}><Icon name="terminal" size={16} />What the launcher ran</button>
        <button onclick={() => (app.page = "settings")}
          ><Icon name="setup" size={16} />Settings and developer tools</button
        >
      </div>
    </details>
  </div>
{/if}

<style>
  .steps {
    position: relative;
    display: flex;
    gap: 8px;
    margin: 0;
    padding: 0;
    list-style: none;
    counter-reset: step;
  }

  .steps li {
    flex: 1;
    counter-increment: step;
    border: 1px solid rgb(255 255 255 / 0.15);
    border-radius: var(--radius-sm);
    background: rgb(0 0 0 / 0.35);
    padding: 8px 12px;
    font-size: 13px;
    color: var(--soft);
  }

  .steps li::before {
    content: counter(step) ". ";
    font-weight: 700;
  }

  .steps li.done {
    border-color: color-mix(in srgb, var(--ok) 50%, transparent);
    color: var(--ok);
  }

  .steps small {
    display: block;
    color: var(--dim);
    font-size: 11px;
  }

  .page {
    position: relative;
    max-width: 920px;
    margin: 0 auto;
    padding: 20px 28px 60px;
    display: flex;
    flex-direction: column;
    gap: 20px;
  }

  .page-ambient-bg {
    position: absolute;
    top: -20px;
    left: 50%;
    transform: translateX(-50%);
    width: 100vw;
    height: 480px;
    pointer-events: none;
    z-index: 0;
    overflow: hidden;
    opacity: 0.18;
  }

  .page-ambient-bg::after {
    content: "";
    position: absolute;
    inset: 0;
    background:
      radial-gradient(ellipse 70% 60% at 50% 20%, transparent 20%, var(--ink) 80%),
      linear-gradient(to bottom, transparent 60%, var(--ink) 100%);
  }

  .page-ambient-bg img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  .back {
    position: relative;
    z-index: 1;
    align-self: flex-start;
  }

  .hero {
    position: relative;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
    padding: 34px 28px 30px;
    border-radius: 30px;
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
    opacity: 0.38;
  }

  .hero-overlay {
    position: absolute;
    inset: 0;
    background:
      radial-gradient(ellipse 90% 80% at 50% 20%, transparent 20%, rgba(14, 16, 22, 0.7) 100%),
      linear-gradient(180deg, rgba(14, 16, 22, 0.3) 0%, rgba(14, 16, 22, 0.65) 100%);
  }

  .hero > :not(.hero-backdrop) {
    position: relative;
    z-index: 1;
  }

  .box {
    border: 7px solid var(--bd);
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
    font-size: 13px;
    letter-spacing: 0.04em;
    color: var(--tx2);
  }

  .versions {
    display: flex;
    gap: 10px;
  }

  .versions button {
    border-width: 4px;
    border-radius: 0;
    font-family: var(--title-font);
    font-weight: var(--title-weight);
    font-size: 13px;
    opacity: 0.6;
    padding: 6px 18px;
  }

  .versions button.on,
  .versions button:hover {
    opacity: 1;
  }

  .go {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 10px;
    margin-top: 8px;
    max-width: 100%;
  }

  .big {
    font-size: 22px;
    font-weight: 700;
    padding: 16px 46px;
    border-radius: 999px;
    gap: 12px;
  }

  .line {
    font-size: 14px;
    color: var(--tx2);
    text-align: center;
    max-width: 640px;
  }

  .editor {
    display: flex;
    align-items: center;
    gap: 20px;
  }

  .editor h2 {
    margin: 0 0 4px;
  }

  .more summary {
    cursor: pointer;
    color: var(--soft);
    font-weight: 600;
  }

  .more .row {
    margin-top: 14px;
    gap: 10px;
  }

  .wrap {
    flex-wrap: wrap;
  }

  .link {
    border: 0;
    background: none;
    padding: 0;
    color: var(--amber);
    font-weight: 600;
  }
</style>
