<script lang="ts">
  // The Level editor tab's "Start over": two rows under the editor's steps, each a button that asks
  // first, in a dialog that lists what goes, with its size, and what stays.
  // - Read the levels again: the game's copy of the levels (and the lighting baked from them) goes,
  //   then is read from the disc at once, as a job like any other (Activity shows it).
  // - Reset the level editor: the Godot projects and the exported models and textures go; with the
  //   tick box, unticked at first, the player's own packed levels, edits and packs too. Opening a
  //   level afterwards reads fresh copies from the disc.
  // The folders are chosen and checked by the core (core/src/reset.rs); this page sends the version
  // and the choice, never a path. Neither runs while the game or a job does. The words are
  // lib/editorReset.ts's; what the core says when it fails goes to the log, not to the player.
  import { api, type ResetKind, type ResetPreview, type VersionView } from "$lib/api";
  import { app, jobs, refresh, runChain, toast } from "$lib/app.svelte";
  import {
    PART_TEXT,
    RESET,
    anything,
    blockedBy,
    chosenParts,
    editorDone,
    failureText,
    keptLine,
    partSize,
    theirParts,
    total,
  } from "$lib/editorReset";
  import { size } from "$lib/format";
  import { hidePaths } from "$lib/paths";
  import { modal } from "$lib/portal";
  import Icon from "./Icon.svelte";

  let { version, onchanged }: { version: VersionView; onchanged?: () => void } = $props();

  const developer = $derived(app.config?.developer ?? false);
  const action = (v: VersionView, id: string) => v.actions.find((a) => a.id === id && a.thisPlatform) ?? null;
  const levels = $derived(action(version, "port-levels"));
  const blocked = $derived(blockedBy(jobs.list, !!app.game?.running));
  const noDisc = $derived(version.status.disc.state !== "found");
  /** Reading the levels again, after the reset: its job's last line on the row. */
  const reading = $derived(
    jobs.list.find(
      (j) =>
        j.scope.kind === "version" &&
        j.scope.key === version.key &&
        ["port-levels", "port-levels-visibility"].includes(j.actionId) &&
        (j.state === "queued" || j.state === "running"),
    ) ?? null,
  );

  /** The dialog, while it is open. */
  let asking = $state<{
    kind: ResetKind;
    preview: ResetPreview | null;
    error: string | null;
    withEdits: boolean;
    working: boolean;
  } | null>(null);

  const chosen = $derived(asking?.preview ? chosenParts(asking.preview, asking.withEdits) : []);
  const theirs = $derived(asking?.preview ? theirParts(asking.preview) : []);

  async function ask(kind: ResetKind) {
    if (asking || blocked) return;
    asking = { kind, preview: null, error: null, withEdits: false, working: false };
    // The dialog this answer is for: closed (or another opened) meanwhile, it is not shown.
    const open = asking;
    try {
      const preview = await api.resetPreview(version.key, kind);
      open.preview = preview;
    } catch (e) {
      console.warn("reset preview", e);
      open.error = failureText(e, true);
    }
  }

  function close() {
    if (!asking?.working) asking = null;
  }

  async function confirm() {
    if (!asking?.preview || asking.working) return;
    const { kind, withEdits } = asking;
    asking.working = true;
    let done;
    try {
      done = await api.resetApply(version.key, kind, withEdits);
    } catch (e) {
      console.warn("reset", e);
      asking.working = false;
      asking.error = failureText(e, false);
      // Some folders may have gone before the one that could not: the list and the page read them again.
      const open = asking;
      open.preview = await api.resetPreview(version.key, kind).catch(() => open.preview);
      await refresh();
      onchanged?.();
      return;
    }
    asking = null;
    await refresh();
    onchanged?.();
    if (kind === "editor") {
      toast(editorDone(done));
      return;
    }
    // Read them from the disc at once, with the visibility the game adds to them.
    const fresh = app.library?.games.flatMap((g) => g.versions).find((x) => x.key === version.key) ?? version;
    const steps = [action(fresh, "port-levels"), action(fresh, "port-levels-visibility")].filter((a) => a !== null);
    const ok = steps.length > 0 && (await runChain({ kind: "version", key: version.key }, steps));
    await refresh();
    toast(ok ? RESET.levelsDone : RESET.levelsStopped, ok ? "info" : "error");
  }

  function onkeydown(e: KeyboardEvent) {
    if (e.key === "Escape" && asking) close();
  }
</script>

<svelte:window {onkeydown} />

<section class="panel start-over" aria-labelledby="start-over-title">
  <h2 id="start-over-title">{RESET.title}</h2>
  <p class="muted lead">{RESET.lead}</p>
  <ul>
    {#if levels}
      <li>
        <div class="text">
          <h3>{RESET.levelsTitle}</h3>
          <p class="muted">
            {#if reading}
              {reading.title}: {hidePaths(reading.lines.at(-1)?.line ?? "…").text}
            {:else if noDisc}
              {RESET.noDisc}
            {:else if blocked}
              {blocked}
            {:else}
              {RESET.levelsLine}
            {/if}
          </p>
        </div>
        <button class="small" disabled={!!blocked || noDisc || !!asking} onclick={() => void ask("levels")}>
          {#if reading}<span class="spinner"></span>{:else}<Icon name="refresh" size={14} />{/if}{RESET.levelsButton}
        </button>
      </li>
    {/if}
    <li>
      <div class="text">
        <h3>{RESET.editorTitle}</h3>
        <p class="muted">{blocked ?? RESET.editorLine}</p>
      </div>
      <button class="small danger" disabled={!!blocked || !!asking} onclick={() => void ask("editor")}>
        <Icon name="trash" size={14} />{RESET.editorButton}
      </button>
    </li>
  </ul>
</section>

{#if asking}
  {@const kind = asking.kind}
  {@const sum = total(chosen)}
  <div class="overlay" role="presentation" use:modal>
    <button class="backdrop" onclick={close} tabindex="-1" aria-label={RESET.cancel}></button>
    <div class="dialog rise" role="dialog" aria-modal="true" aria-labelledby="reset-title" tabindex="-1">
      <h2 id="reset-title">{kind === "levels" ? RESET.levelsTitle : RESET.editorTitle}</h2>

      {#if asking.error}
        <p class="error" role="alert"><Icon name="warn" size={16} /><span>{asking.error}</span></p>
      {/if}

      {#if !asking.preview && !asking.error}
        <p class="muted"><span class="spinner"></span> {RESET.checking}</p>
      {:else if asking.preview}
        <h3>{RESET.removes}</h3>
        <ul class="parts">
          {#each chosen as p (p.part)}
            <li>
              <div>
                <strong>{PART_TEXT[p.part].title}</strong>
                <span class="muted">{PART_TEXT[p.part].line}</span>
                {#if developer}{#each p.folders as f (f)}<code>{f}</code>{/each}{/if}
              </div>
              <span class="size">{partSize(p)}</span>
            </li>
          {/each}
        </ul>

        {#if theirs.length}
          <label class="theirs">
            <input type="checkbox" bind:checked={asking.withEdits} disabled={asking.working} />
            <span>
              <strong>{RESET.withEdits}</strong>
              <span class="muted">{theirs.map((p) => `${PART_TEXT[p.part].title} (${partSize(p)})`).join(", ")}.</span>
            </span>
          </label>
        {/if}

        <h3>{RESET.keeps}</h3>
        <p class="muted kept">{keptLine(kind, asking.withEdits)}</p>
        {#if kind === "editor"}<p class="muted"><Icon name="info" size={14} /> {RESET.closeGodot}</p>{/if}
      {/if}

      <div class="actions">
        {#if asking.preview}
          <span class="muted grow">{anything(chosen) ? RESET.inAll(size(sum.bytes)) : RESET.nothing}</span>
        {/if}
        <button class="ghost" onclick={close} disabled={asking.working}>{RESET.cancel}</button>
        <button
          class={kind === "editor" ? "danger-solid" : "primary"}
          disabled={!asking.preview || asking.working || (kind === "editor" && !anything(chosen))}
          onclick={() => void confirm()}
        >
          {#if asking.working}<span class="spinner"></span>{RESET.working}{:else}{kind === "levels"
              ? RESET.confirmLevels
              : RESET.confirmEditor}{/if}
        </button>
      </div>
    </div>
  </div>
{/if}

<style>
  .start-over {
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .start-over h2 {
    font-size: 1rem;
  }

  .lead {
    margin: 0 0 4px;
    font-size: 0.875rem;
  }

  .start-over ul {
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .start-over li {
    display: flex;
    align-items: center;
    gap: 8px 16px;
    padding: 10px 0;
  }

  .start-over li + li {
    border-top: 1px solid var(--line);
  }

  .text {
    flex: 1 1 0;
    min-width: 0;
  }

  .text h3 {
    font-family: var(--font-sans);
    font-size: 0.875rem;
    font-weight: 600;
    color: var(--text);
  }

  .text p {
    margin: 2px 0 0;
    font-size: 0.8125rem;
  }

  .start-over li > button {
    flex: none;
    min-width: 120px;
    justify-content: center;
  }

  .danger {
    color: var(--err);
  }

  .overlay {
    position: fixed;
    inset: 0;
    z-index: 100;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 24px 16px;
  }

  .backdrop {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    border: none;
    border-radius: 0;
    padding: 0;
    background: color-mix(in srgb, var(--ink-deep) 75%, transparent);
    backdrop-filter: blur(4px);
    cursor: default;
  }

  .dialog {
    position: relative;
    z-index: 1;
    width: 100%;
    max-width: 540px;
    max-height: 100%;
    overflow-y: auto;
    display: flex;
    flex-direction: column;
    gap: 10px;
    padding: 20px 24px;
    background: var(--panel);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius);
    box-shadow: var(--shadow);
  }

  .dialog h2 {
    margin: 0;
    font-family: var(--font-head);
    font-size: 1.125rem;
    font-weight: 600;
    color: var(--text);
  }

  .dialog h3 {
    margin: 6px 0 0;
    font-family: var(--font-sans);
    font-size: 0.75rem;
    font-weight: 700;
    letter-spacing: 0.06em;
    text-transform: uppercase;
    color: var(--gold);
  }

  .dialog p {
    margin: 0;
    font-size: 0.875rem;
    display: flex;
    align-items: center;
    gap: 6px;
  }

  .dialog p.kept {
    display: block;
  }

  .parts {
    margin: 0;
    padding: 0;
    list-style: none;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .parts li {
    display: flex;
    justify-content: space-between;
    gap: 12px;
    font-size: 0.875rem;
  }

  .parts li div {
    display: flex;
    flex-direction: column;
    gap: 2px;
    min-width: 0;
  }

  .parts code {
    font-size: 0.75rem;
    overflow-wrap: anywhere;
  }

  .size {
    flex: none;
    font-variant-numeric: tabular-nums;
    color: var(--soft);
  }

  .theirs {
    display: flex;
    align-items: flex-start;
    gap: 10px;
    padding: 10px 12px;
    border: 1px dashed var(--line-strong);
    border-radius: var(--radius-sm);
    font-size: 0.875rem;
    cursor: pointer;
  }

  .theirs input {
    margin-top: 3px;
  }

  .theirs > span {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .error {
    color: var(--err);
  }

  .actions {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    justify-content: flex-end;
    gap: 8px;
    margin-top: 8px;
  }

  .grow {
    flex: 1 1 auto;
    font-size: 0.875rem;
  }

  .danger-solid {
    background: color-mix(in srgb, var(--err) 22%, var(--panel-hi));
    border-color: color-mix(in srgb, var(--err) 55%, transparent);
    color: var(--text);
  }
</style>
