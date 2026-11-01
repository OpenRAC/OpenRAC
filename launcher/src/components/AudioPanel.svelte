<script lang="ts">
  // Mods → Audio: export the game's sounds as WAV files to edit in an audio editor, and import the
  // edits back as an overrides pack (it joins My mods), as AssetsPanel does for models and textures.
  // The Python side does the work (editor/audio.py, through actions.json). The packs are the game's,
  // shared by its discs; the version (the one the game page plays) only picks the disc exported from.
  // editor/AUDIO_MODDING.md is the modder's guide.
  import Icon from "$components/Icon.svelte";
  import { api, inTauri, pickFolder, type ActionView, type VersionView } from "$lib/api";
  import { app, guardShown, jobs, runChain, toast } from "$lib/app.svelte";
  import { importOutcome } from "$lib/audioImport";
  import { levelsOf } from "$lib/labels";
  import { AUDIO } from "$lib/modsText";
  import { hidePaths } from "$lib/paths";
  import type { Job } from "$lib/queue";
  import CommandBox from "./CommandBox.svelte";
  import JobProgress from "./JobProgress.svelte";

  let { version: v, onimported }: { version: VersionView; onimported?: () => void } = $props();

  const developer = $derived(app.config?.developer ?? false);
  const scope = $derived({ kind: "version" as const, key: v.key });
  const action = (id: string): ActionView | null => v.actions.find((a) => a.id === id && a.thisPlatform) ?? null;
  const importer = $derived(action("audio-import"));

  type What = keyof typeof AUDIO.what;
  const WHATS = Object.keys(AUDIO.what) as What[];
  /** The level list's choice for the whole game. */
  const WHOLE_GAME = -1;

  // svelte-ignore state_referenced_locally
  let level = $state(levelsOf(v.game)[0]?.number ?? 0);
  let what = $state<What>("effects");
  let busy = $state<"export" | "import" | null>(null);
  /** What the last import said, in a sentence. */
  let said = $state<ReturnType<typeof importOutcome> | null>(null);

  const exporter = $derived(action(level === WHOLE_GAME ? "audio-export-all" : "audio-export"));
  const large = $derived(level === WHOLE_GAME && what !== "effects" ? AUDIO.large[what] : null);

  /** The newest job of these actions of this disc. */
  const newest = (ids: string[]): Job | null =>
    jobs.list.filter((j) => j.scope.kind === "version" && j.scope.key === v.key && ids.includes(j.actionId)).at(-1) ??
    null;
  const exported = $derived(newest(["audio-export", "audio-export-all"]));
  const imported = $derived(newest(["audio-import"]));
  const folder = $derived(exported?.state === "succeeded" ? (exported.outputs[0] ?? null) : null);

  async function exportSounds() {
    if (!exporter) return;
    busy = "export";
    // The whole game's export names no level.
    const ok = await runChain(scope, [exporter], level === WHOLE_GAME ? { option: what } : { level, option: what });
    busy = null;
    const written = newest(["audio-export", "audio-export-all"])?.outputs[0];
    if (ok && written) {
      toast(AUDIO.exported);
      await guardShown(api.openOutput(written));
    } else if (!ok) {
      toast(AUDIO.exportStopped, "error");
    }
  }

  async function importEdits() {
    if (!importer) return;
    const edited = inTauri
      ? await pickFolder(AUDIO.pickImport)
      : "/home/you/exports/audio-level_00-effects-20261101T0900";
    if (!edited) return;
    busy = "import";
    said = null;
    const ok = await runChain(scope, [importer], { path: edited });
    busy = null;
    said = importOutcome(ok, newest(["audio-import"])?.lines ?? []);
    onimported?.();
  }
</script>

{#if exporter}
  <div class="audio">
    <p class="muted intro">
      {AUDIO.lead}
      <button class="link" onclick={() => void guardShown(api.openPath("editor/AUDIO_MODDING.md"))}
        >{AUDIO.howItWorks}</button
      >
    </p>
    <ol class="steps">
      <li>
        <div class="text">
          <h3>{AUDIO.exportTitle}</h3>
          <p class="muted">
            {#if busy === "export"}
              {AUDIO.exporting}
            {:else if !exporter.runnable}
              {AUDIO.readyFirst}
            {:else}
              {AUDIO.exportLine}{#if large}&nbsp;{large}{/if}
            {/if}
          </p>
        </div>
        <div class="controls">
          <select aria-label={AUDIO.levelLabel} bind:value={level} disabled={!exporter.runnable}>
            <option value={WHOLE_GAME}>{AUDIO.allLevels}</option>
            {#each levelsOf(v.game) as l (l.number)}<option value={l.number}>{l.name}</option>{/each}
          </select>
          <select aria-label={AUDIO.whatLabel} bind:value={what} disabled={!exporter.runnable}>
            {#each WHATS as w (w)}<option value={w}>{AUDIO.what[w]}</option>{/each}
          </select>
          {#if folder && busy !== "export"}
            <button class="ghost" onclick={() => void guardShown(api.openOutput(folder))}
              ><Icon name="folder" size={16} />{AUDIO.openFolder}</button
            >
          {/if}
          <button disabled={!exporter.runnable || busy !== null} onclick={() => void exportSounds()}>
            {#if busy === "export"}<span class="spinner"></span>{:else}<Icon
                name="external"
                size={16}
              />{/if}{AUDIO.export}
          </button>
        </div>
        {#if busy === "export" && exported}
          <div class="span"><JobProgress job={exported} plain /></div>
        {/if}
        {#if developer}
          <div class="span">
            <CommandBox
              lines={[
                `python3 editor/audio.py export <disc.iso> <out> --what ${what}${level === WHOLE_GAME ? "" : ` --level ${level}`}`,
              ]}
            />
          </div>
        {/if}
      </li>
      <li>
        <div class="text">
          <h3>{AUDIO.importTitle}</h3>
          <p
            class="muted"
            class:failed={said?.tone === "err"}
            class:warning={said?.tone === "warn"}
            class:done={said?.tone === "ok"}
            role="status"
          >
            {#if busy === "import"}
              {AUDIO.importing}
            {:else if said}
              {hidePaths(said.text).text}
              {#if imported}
                <button
                  class="link activity"
                  onclick={() => {
                    jobs.selected = imported.key;
                    app.page = "tasks";
                  }}>{AUDIO.openActivityLog}</button
                >
              {/if}
            {:else}
              {AUDIO.importLine}
            {/if}
          </p>
        </div>
        <div class="controls">
          <button disabled={!importer?.runnable || busy !== null} onclick={() => void importEdits()}>
            {#if busy === "import"}<span class="spinner"></span>{:else}<Icon name="save" size={16} />{/if}{AUDIO.import}
          </button>
        </div>
        {#if developer}
          <div class="span">
            <CommandBox lines={[`python3 editor/audio.py import <out> --into <data>/overrides`]} />
          </div>
        {/if}
      </li>
    </ol>
  </div>
{/if}

<style>
  .audio {
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .intro {
    margin: 0 0 4px;
    font-size: 0.875rem;
  }

  /* Export's two choices and its buttons wrap under its text when they do not fit beside it. */
  li:first-child {
    grid-template-columns: 26px minmax(0, 1fr);
  }

  li:first-child .controls {
    grid-column: 2;
    justify-content: flex-start;
  }

  .failed {
    color: var(--err);
  }

  .done {
    color: var(--ok);
  }

  .warning {
    color: var(--warn);
  }

  .activity {
    margin-left: 0.5em;
  }

  .link {
    border: 0;
    background: none;
    padding: 0;
    font-size: inherit;
    color: var(--amber);
    font-weight: 600;
  }
</style>
