<script lang="ts">
  // The Level editor tab's one panel: where the player is with this disc's levels, in a sentence,
  // and the step that comes next as the main button; the other steps below it, and Troubleshoot
  // (Start over) folded at the bottom. The state is the core's look at the editor's folders
  // (core/src/editor_state.rs), asked again when the tab opens, when the window comes back into
  // focus (the player saved in Godot) and when one of the editor's jobs ends; the words and the
  // order of the states are lib/editorFlow.ts's. A failed step is said in a player's words
  // (lib/toolErrors.ts), never in the tool's: what it printed is behind "Show details".
  import { api, pickFile, type EditorState, type GameView, type VersionView } from "$lib/api";
  import { app, guardShown, jobs, run, runChain, saveConfig, toast } from "$lib/app.svelte";
  import { STEP_LABEL, editorStage, updateNote, updateNoteLine, type Step } from "$lib/editorFlow";
  import { levelName, levelsOf } from "$lib/labels";
  import { GODOT_URL } from "$lib/links";
  import { EDITOR } from "$lib/modsText";
  import { hidePaths } from "$lib/paths";
  import { foundDisc, setUp } from "$lib/player.svelte";
  import type { Job } from "$lib/queue";
  import { toolFailure, type NextStep } from "$lib/toolErrors";
  import { onMount } from "svelte";
  import EditorReset from "./EditorReset.svelte";
  import Icon from "./Icon.svelte";

  let { game, version, onchanged }: { game: GameView; version: VersionView; onchanged?: () => void } = $props();

  const scope = $derived({ kind: "version" as const, key: version.key });
  const action = (id: string) => version.actions.find((a) => a.id === id && a.thisPlatform) ?? null;
  const extract = $derived(action("editor-extract"));
  const edit = $derived(action("editor-open"));
  const pack = $derived(action("editor-pack"));
  const early = $derived(!pack);
  const lacksGodot = $derived((edit?.blockers ?? []).some((b) => b.replace(/^needs /, "").startsWith("Godot")));
  const discFound = $derived(version.status.disc.state === "found");

  const live = (j: Job) => j.state === "queued" || j.state === "running";
  const jobsOf = (ids: string[]) =>
    jobs.list.filter((j) => j.scope.kind === "version" && j.scope.key === version.key && ids.includes(j.actionId));
  const extracting = $derived(jobsOf(["editor-extract", "editor-import"]).find(live) ?? null);
  const packing = $derived(jobsOf(["editor-pack"]).find(live) ?? null);

  // ---- What the core found, asked again when it may have changed ----

  let found = $state<EditorState | null>(null);
  let unreadable = $state(false);
  let asking = 0;
  async function look() {
    const mine = ++asking;
    try {
      const seen = await api.editorState(version.key);
      if (mine === asking) {
        found = seen;
        unreadable = false;
      }
    } catch (e) {
      // The folders could not be read (no install folder yet, say): the panel goes on from the
      // actions alone, and the log has why.
      console.warn("editor state", e);
      if (mine === asking) unreadable = true;
    }
  }

  onMount(() => {
    // Back from Godot: what was saved there is seen without a click.
    const focused = () => void look();
    const shown = () => {
      if (document.visibilityState === "visible") void look();
    };
    window.addEventListener("focus", focused);
    document.addEventListener("visibilitychange", shown);
    return () => {
      window.removeEventListener("focus", focused);
      document.removeEventListener("visibilitychange", shown);
    };
  });

  // An editor job ended (here, from Activity or from the game page): look again.
  const ended = $derived(
    jobsOf(["editor-extract", "editor-import", "editor-pack", "overrides-stack"]).filter((j) => !live(j)).length,
  );
  // (It runs once at first too: the tab's first look.)
  $effect(() => {
    if (ended >= 0) void look();
  });

  // ---- The step a failed job stopped on, in a player's words ----

  let dismissed = $state<number | null>(null);
  const failed = $derived.by(() => {
    const last = jobsOf(["editor-extract", "editor-pack"])
      .filter((j) => !live(j))
      .at(-1);
    return last?.state === "failed" && last.key !== dismissed ? last : null;
  });
  const failure = $derived(failed ? toolFailure(failed.lines) : null);
  let details = $state(false);

  const stage = $derived(
    editorStage({
      game: version.game,
      state: unreadable ? emptyState() : found,
      discFound,
      lacksGodot,
      early,
      extracting,
      packing,
    }),
  );
  const note = $derived(found ? updateNote(version.game, found.carried) : null);
  // The failure's own sentence is the stage's when the core already found the same thing.
  const failureShown = $derived(failure && !(failure.next === "update" && stage.kind === "needs-update"));

  function emptyState(): EditorState {
    return {
      levels: edit?.runnable ? [0] : [],
      outdated: [],
      unpacked: [],
      broken: [],
      packedAt: null,
      packedLevels: 0,
      packedOtherVersion: false,
      carried: [],
      backups: [],
      gameLevelsFound: false,
      formatKnown: false,
    };
  }

  // ---- The steps ----

  /** The level the editor opens. */
  let picked = $state<number | null>(null);
  const level = $derived(picked ?? levelsOf(version.game)[0]?.number ?? 0);
  let busy = $state<Step | null>(null);
  let pickingGodot = false;

  async function chooseGodot() {
    if (!app.config || pickingGodot) return;
    pickingGodot = true;
    const picked = await pickFile(EDITOR.pickGodot).finally(() => (pickingGodot = false));
    if (!picked) return;
    const check = await guardShown(api.checkTool("godot", picked));
    if (!check) return;
    if (!check.ok) {
      toast(hidePaths(EDITOR.notGodot(check.message)).text, "error");
      return;
    }
    await saveConfig({ ...app.config, godot: picked });
  }

  async function take(step: Step | NextStep) {
    if (busy) return;
    switch (step) {
      case "set-up":
        await setUp(game.id, game.title, foundDisc(game.id)?.path);
        return;
      case "get-godot":
        await guardShown(api.openUrl(GODOT_URL));
        return;
      case "prepare":
      case "open":
        if (!edit) return;
        busy = step;
        try {
          // The first time (or a level not read yet), the levels are read before Godot opens.
          const done =
            edit.runnable && found?.levels.includes(level) !== false
              ? (await guardShown(api.openLevelInGodot(version.key, level))) !== undefined
              : await runChain(scope, extract ? [extract, edit] : [edit], level);
          if (done) toast(EDITOR.opening(levelName(version.game, level)));
        } finally {
          busy = null;
        }
        return;
      case "update":
      case "repair":
        if (!extract) return;
        busy = step;
        try {
          await runChain(scope, [extract]);
        } finally {
          busy = null;
        }
        await look();
        onchanged?.();
        return;
      case "retry":
        if (failed?.actionId === "editor-pack") return take("pack");
        if (failed?.actionId === "editor-extract") return take("update");
        return;
      case "pack":
        if (pack) await run(scope, pack);
        return;
      case "play":
        if (!app.config) return;
        if (!app.config.playWithMods) await saveConfig({ ...app.config, playWithMods: true });
        app.version = version.key;
        app.page = "game";
        return;
      case "start-over":
        troubleshoot = true;
        return;
      case "settings":
        app.page = "settings";
        return;
      case "report":
        app.page = "tasks";
        return;
      default:
        return;
    }
  }

  /** The error's button: the step that helps, in words. */
  const NEXT_LABEL: Partial<Record<NextStep, string>> = {
    update: STEP_LABEL.update,
    retry: "Try again",
    "start-over": "Troubleshoot",
    "set-up": STEP_LABEL["set-up"],
    open: STEP_LABEL.open,
    settings: "Open Settings",
    report: "See the log",
  };

  const disabled = (step: Step) => {
    if (busy || extracting || packing) return true;
    if (step === "pack") return !pack?.runnable;
    if (step === "update" || step === "repair") return !extract?.runnable;
    if (step === "open" || step === "prepare") return !edit || lacksGodot;
    return false;
  };

  const ICON: Record<Step, "disc" | "external" | "edit" | "refresh" | "save" | "play"> = {
    "set-up": "disc",
    "get-godot": "external",
    prepare: "edit",
    open: "edit",
    update: "refresh",
    repair: "refresh",
    pack: "save",
    play: "play",
  };

  const picksLevel = (step: Step) => step === "open" || step === "prepare";
  const steps = $derived([...(stage.primary ? [stage.primary] : []), ...stage.secondary]);

  let troubleshoot = $state(false);
  const backup = $derived(found?.backups[0] ?? null);
</script>

<section class="panel flow" aria-labelledby="flow-title" aria-busy={stage.tone === "busy"}>
  <div class={`status ${stage.tone}`} role="status" aria-live="polite">
    <span class="mark" aria-hidden="true">
      {#if stage.tone === "busy"}<span class="spinner"></span>{:else if stage.tone === "ok"}<Icon
          name="check"
          size={18}
        />{:else if stage.tone === "warn"}<Icon name="warn" size={18} />{:else}<Icon name="info" size={18} />{/if}
    </span>
    <div class="words">
      <h2 id="flow-title">{stage.title}</h2>
      {#if stage.line}<p>{stage.line}</p>{/if}
      {#if stage.detail}<p class="muted detail">{stage.detail}</p>{/if}
    </div>
  </div>

  {#if failureShown && failure && failed}
    <div class="failure" role="alert">
      <Icon name="warn" size={16} />
      <div class="words">
        <strong>{failure.title}</strong>
        <p>{failure.line}</p>
        <div class="row">
          {#if failure.next !== "none" && NEXT_LABEL[failure.next]}
            <button class="small" onclick={() => void take(failure.next)}>{NEXT_LABEL[failure.next]}</button>
          {/if}
          <button class="small ghost" aria-expanded={details} onclick={() => (details = !details)}
            >{details ? "Hide details" : "Show details"}</button
          >
          <button class="small ghost" onclick={() => (dismissed = failed.key)}>Dismiss</button>
        </div>
        {#if details}<pre class="details">{failure.details || "It printed nothing."}</pre>{/if}
      </div>
    </div>
  {/if}

  {#if note && stage.tone !== "busy"}
    <div class="note">
      <p>{updateNoteLine(note)}</p>
      {#if note.redo.length}
        <details>
          <summary>What to redo by hand</summary>
          <ul>
            {#each note.redo as line, i (i)}<li>{line}</li>{/each}
          </ul>
        </details>
      {/if}
      {#if backup}
        <button class="small ghost" onclick={() => void guardShown(api.openOutput(backup.path))}
          ><Icon name="folder" size={14} />Open the backup folder</button
        >
      {/if}
    </div>
  {/if}

  {#if steps.length}
    <div class="actions">
      {#if steps.some(picksLevel) && !lacksGodot}
        <select
          aria-label={EDITOR.levelLabel}
          value={level}
          onchange={(e) => (picked = Number(e.currentTarget.value))}
          disabled={!!busy || !!extracting}
        >
          {#each levelsOf(version.game) as l (l.number)}<option value={l.number}>{l.name}</option>{/each}
        </select>
      {/if}
      {#each steps as step, i (step)}
        <button
          class={i === 0 && stage.primary ? "primary" : ""}
          disabled={disabled(step)}
          onclick={() => void take(step)}
        >
          {#if busy === step}<span class="spinner"></span>{:else}<Icon name={ICON[step]} size={16} />{/if}
          {step === "open" && stage.kind === "needs-update" ? "Open anyway" : STEP_LABEL[step]}
        </button>
        {#if step === "get-godot"}
          <button onclick={() => void chooseGodot()}><Icon name="folder" size={16} />{EDITOR.chooseGodot}</button>
        {/if}
      {/each}
      {#if !early && stage.kind === "packed"}
        <button class="ghost" disabled={disabled("pack")} onclick={() => void take("pack")}
          ><Icon name="save" size={16} />Pack again</button
        >
      {/if}
    </div>
    {#if stage.kind === "needs-update" && stage.secondary.includes("open")}
      <p class="muted hint">Opening an outdated level works for looking around; pack it after the update.</p>
    {/if}
  {/if}

  <details class="trouble" bind:open={troubleshoot}>
    <summary><Icon name="reset" size={14} />Troubleshoot and start over</summary>
    <p class="muted">
      When levels look wrong in Godot or in the game, or for a clean editor. Your disc, saves and gameplay mods are
      never touched.
    </p>
    {#if backup}
      <p class="muted">
        Older copies of your levels, kept by updates, are in the backup folder ({found?.backups.length ?? 0}
        {(found?.backups.length ?? 0) === 1 ? "copy" : "copies"}).
        <button class="link" onclick={() => void guardShown(api.openOutput(backup.path))}>Open it</button>
      </p>
    {/if}
    <EditorReset {version} bare onchanged={() => (void look(), onchanged?.())} />
  </details>
</section>

<style>
  .flow {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .status {
    display: flex;
    gap: 14px;
    align-items: flex-start;
  }

  .mark {
    flex: 0 0 auto;
    display: grid;
    place-items: center;
    width: 34px;
    height: 34px;
    border-radius: 999px;
    background: var(--panel-hi);
    border: 1px solid var(--line-strong);
    color: var(--lav);
  }

  .ok .mark {
    color: var(--ok);
  }

  .warn .mark {
    color: var(--warn);
  }

  .busy .mark {
    color: var(--amber);
  }

  .words {
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .words h2 {
    font-size: 1.0625rem;
    margin: 0;
  }

  .words p {
    margin: 0;
    max-width: 68ch;
    color: var(--body);
  }

  .detail {
    font-size: 0.8125rem;
  }

  .failure {
    display: flex;
    gap: 10px;
    padding: 12px 14px;
    border: 1px solid color-mix(in srgb, var(--err) 45%, transparent);
    border-radius: var(--radius-sm);
    background: color-mix(in srgb, var(--err) 8%, var(--panel));
    color: var(--err);
  }

  .failure strong {
    color: var(--text);
  }

  .failure p {
    font-size: 0.875rem;
  }

  .row,
  .actions {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 8px;
  }

  .row {
    margin-top: 6px;
  }

  .details {
    margin: 6px 0 0;
    padding: 10px;
    max-height: 220px;
    overflow: auto;
    white-space: pre-wrap;
    word-break: break-word;
    font-family: var(--font-mono);
    font-size: 0.75rem;
    color: var(--soft);
    background: var(--ink-deep);
    border-radius: var(--radius-sm);
  }

  .note {
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    gap: 6px;
    padding: 12px 14px;
    border: 1px dashed var(--line-strong);
    border-radius: var(--radius-sm);
    font-size: 0.875rem;
  }

  .note p {
    margin: 0;
  }

  .note ul {
    margin: 6px 0 0;
    padding-left: 18px;
    color: var(--soft);
  }

  .actions select {
    font: inherit;
    font-size: 0.875rem;
    background: var(--ink-deep);
    color: var(--text);
    border: 1px solid var(--line-field);
    border-radius: 999px;
    padding: 7px 12px;
  }

  .actions select:focus-visible {
    outline: 2px solid var(--amber);
    outline-offset: 2px;
  }

  .hint {
    margin: -8px 0 0;
    font-size: 0.8125rem;
  }

  .trouble {
    border-top: 1px solid var(--line);
    padding-top: 12px;
  }

  .trouble > summary {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    cursor: pointer;
    font-size: 0.875rem;
    font-weight: 600;
    color: var(--soft);
  }

  .trouble > summary:focus-visible {
    outline: 2px solid var(--amber);
    outline-offset: 2px;
  }

  .trouble > p {
    margin: 10px 0 0;
    font-size: 0.8125rem;
  }

  .link {
    border: 0;
    background: none;
    padding: 0;
    color: var(--amber);
    font-weight: 600;
    font-size: inherit;
  }

  @media (max-width: 600px) {
    .actions > * {
      flex: 1 1 100%;
      justify-content: center;
    }
  }
</style>
