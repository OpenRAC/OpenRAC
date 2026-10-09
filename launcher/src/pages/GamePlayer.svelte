<script lang="ts">
  // A game as a player meets it: set it up from your own disc, play the
  // native port, edit its levels in Godot. Each button runs the same actions the
  // developer page lists (launcher/actions.json), found here by their ids.
  import Icon from "$components/Icon.svelte";
  import { api, pickDisc, pickFile, type ActionView } from "$lib/api";
  import {
    app,
    cancel,
    currentVersion,
    guard,
    jobs,
    openVersion,
    refresh,
    run,
    runChain,
    saveConfig,
    toast,
  } from "$lib/app.svelte";
  import { GODOT_URL } from "$lib/links";
  import { regionLabel } from "$lib/labels";
  import { themeStyle } from "$lib/themes";

  const v = $derived(currentVersion());
  const game = $derived(app.library?.games.find((g) => g.id === v?.game) ?? null);
  const scope = $derived({ kind: "version" as const, key: v?.key ?? "" });
  const action = (id: string): ActionView | null => v?.actions.find((a) => a.id === id && a.thisPlatform) ?? null;

  const play = $derived(action("play"));
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
  const preparing = $derived(jobOf(["editor-extract", "editor-import"]));

  let adding = $state(false);
  let opening = $state(false);

  async function addDisc() {
    if (!v) return;
    const picked = await pickDisc(`The image of your ${v.title} disc`);
    if (!picked) return;
    adding = true;
    const placed = await guard(api.addDisc(v.key, picked));
    adding = false;
    if (placed) {
      toast("Your disc was added.");
      await refresh();
    }
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
    <button class="ghost back" onclick={() => (app.page = "library")}><Icon name="arrow" size={16} />All games</button>

    <section class="hero rise" aria-label={`${v.title} (${v.region})`}>
      {#if media}
        <div class="hero-backdrop" aria-hidden="true">
          <img src={media.bg} alt="" class="hero-bg" />
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
        {#if !play || play.state === "planned"}
          <button class="big" disabled><Icon name="play" size={22} />Not playable yet</button>
          <p class="line">The native port of this game is not built yet: its decompilation comes first.</p>
        {:else if playing}
          <button class="big" onclick={() => void cancel(playing)}>
            <span class="spinner"></span>{playing.state === "queued" ? "Waiting…" : "Stop the game"}
          </button>
          <p class="line clip">
            {playing.lines.at(-1)?.line ?? "Getting the game ready (the first time takes a minute)…"}
          </p>
        {:else if disc !== "found"}
          <button class="big primary" disabled={adding} onclick={() => void addDisc()}>
            {#if adding}<span class="spinner"></span>{:else}<Icon name="disc" size={22} />{/if}Add your disc
          </button>
          <p class="line">
            {#if disc === "mismatch"}{v.status.disc.message}{:else}Choose the image (.iso) of your own {v.title} disc. OpenRAC
              never downloads a game.{/if}
          </p>
        {:else if play.runnable}
          <button class="big primary" onclick={() => void run(scope, play)}><Icon name="play" size={22} />Play</button>
          <p class="line">The decompiled game, built for this computer.</p>
        {:else}
          <button class="big" disabled><Icon name="play" size={22} />Play</button>
          <p class="line">Not ready: it needs {needs(play).join(", ")}.</p>
        {/if}
      </div>
    </section>

    <section class="panel editor">
      <div class="grow">
        <h2>Level editor</h2>
        {#if !extract || extract.state === "planned"}
          <p class="muted">The level editor cannot open this game yet.</p>
        {:else if disc !== "found"}
          <p class="muted">Add your disc first; the editor reads the levels from it.</p>
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
        <button onclick={() => void guard(api.openPath("baserom"))}
          ><Icon name="disc" size={16} />Open your discs' folder</button
        >
        <button onclick={() => (app.page = "tasks")}><Icon name="terminal" size={16} />What the launcher ran</button>
        <button onclick={() => (app.page = "settings")}
          ><Icon name="setup" size={16} />Settings and developer tools</button
        >
      </div>
    </details>
  </div>
{/if}

<style>
  .page {
    max-width: 920px;
    margin: 0 auto;
    padding: 20px 28px 60px;
    display: flex;
    flex-direction: column;
    gap: 20px;
  }

  .back {
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
