<script lang="ts">
  import { api, inTauri, type VersionView } from "$lib/api";
  import type { ModLoadSummary } from "$lib/generated/ModLoadSummary";
  import { app } from "$lib/app.svelte";
  import { levelName } from "$lib/labels";

  let {
    version,
    refreshKey = "",
    prepare,
    canPrepare = false,
  }: {
    version: VersionView;
    refreshKey?: string;
    prepare?: () => void;
    canPrepare?: boolean;
  } = $props();
  let summary = $state<ModLoadSummary | null>(null);
  let error = $state<string | null>(null);
  let loading = $state(false);
  let request = 0;
  let loadedVersion = "";
  const enabled = $derived(app.config?.playWithMods ?? false);
  const edition = $derived(version.name === "pal" ? "PAL" : "NTSC-U");
  const prepared = $derived(summary?.preparedAt != null);
  const current = $derived(prepared && !summary?.stale);
  const hasNotes = $derived(!!summary?.notes.length);
  const statusLabel = $derived(
    error
      ? "Summary unavailable"
      : !enabled
        ? "Mods are off"
        : current
          ? hasNotes
            ? "Prepared with notes"
            : "Prepared"
          : prepared
            ? "Needs preparation"
            : "Not prepared yet",
  );

  async function refresh(input = { version: version.key, revision: refreshKey }) {
    const id = ++request;
    if (loadedVersion !== input.version) {
      summary = null;
      loadedVersion = input.version;
    }
    loading = true;
    try {
      const result = await api.overridesSummary(input.version);
      if (id === request) {
        summary = result;
        error = null;
      }
    } catch (e) {
      if (id === request) {
        summary = null;
        error = String(e);
      }
    } finally {
      if (id === request) loading = false;
    }
  }
  $effect(() => {
    void refresh({ version: version.key, revision: refreshKey });
  });
  const names = (levels: string[]) => levels.map((id) => levelName("rac1", Number(id.slice(6)))).join(", ");
</script>

<section class="panel load-summary" aria-label="Level and asset pack summary">
  <div class="summary-head">
    <div>
      <div class="eyebrow">
        LEVELS &amp; ASSET PACKS {#if !inTauri}<span> · PREVIEW DATA</span>{/if}
      </div>
      <h2>Before you play <span class="edition">{edition}</span></h2>
    </div>
    <span class="status" class:ready={enabled && current && !hasNotes}>{statusLabel}</span>
  </div>
  {#if error}
    <p class="notice">The pack summary could not be read. {error}</p>
    <button class="ghost" disabled={loading} onclick={() => void refresh()}>Retry</button>
  {:else if summary}
    <p class="notice" class:quiet={enabled && current && !hasNotes}>
      {#if !enabled}The next launch uses the original levels. Turn on <strong>Mods</strong> under Play to use your edits.
      {:else if !prepared}Your packs are selected, but no prepared content is recorded for {edition} yet.
      {:else if summary.stale}Your selection or packed files changed. Prepare again to confirm the next launch.
      {:else if hasNotes}The files are prepared for {edition}, with notes to review below.
      {:else}These are the files prepared for {edition}. Launch with <strong>Mods</strong> enabled to use them.{/if}
    </p>
    <div class="metrics" class:previous={!enabled || !current}>
      {#each [{ label: "Objects & gameplay", levels: summary.gameplayLevels }, { label: "Scenery", levels: summary.sceneryLevels }, { label: "Terrain", levels: summary.terrainLevels }, { label: "Collision", levels: summary.collisionLevels }] as item (item.label)}
        <div class="metric">
          <span>{item.label}</span>
          <strong
            >{prepared ? item.levels.length : "—"}<small
              >{prepared ? (item.levels.length === 1 ? " level" : " levels") : ""}</small
            ></strong
          >
          <p>{prepared ? (item.levels.length ? names(item.levels) : "Original files") : "Prepare to check"}</p>
        </div>
      {/each}
    </div>
    {#if prepared && (!enabled || !current)}<p class="caption">
        Counts above describe the previous preparation, not a confirmed next launch.
      </p>{/if}
    <div class="details-grid">
      <div>
        <h3>Selected packs <span class="count">{summary.packs.length}</span></h3>
        {#if summary.packs.length}
          <ul class="selected-packs">
            {#each summary.packs as pack (pack.id)}
              <li>
                <span class="pack-dot"></span><span>{pack.name}</span><small
                  >{prepared
                    ? `${pack.appliedFiles} ${pack.appliedFiles === 1 ? "file" : "files"} prepared`
                    : "Selected"}</small
                >
              </li>
            {/each}
          </ul>
        {:else}<p class="muted">No installed asset packs selected.</p>{/if}
        {#if summary.packedChanges.length}<p class="caption">
            Your packed level edits: {prepared && summary.packedFilesApplied
              ? `${summary.packedFilesApplied} files in the last preparation`
              : "not confirmed in the prepared content"}.
          </p>{/if}
      </div>
      <details class="recorded-edits" open>
        <summary>Edits recorded by the packer</summary>
        <p class="caption">The latest packing report. It can include object, gameplay and collision edits.</p>
        {#if summary.packedVersion && summary.packedVersion !== version.name}<p class="notice">
            This report belongs to another disc edition.
          </p>{/if}
        <pre>{summary.packedChanges.length ? summary.packedChanges.join("\n") : "No packing report available."}</pre>
        {#if summary.changesTruncated}<p class="caption">Showing the first 200 lines.</p>{/if}
      </details>
    </div>
    {#if summary.notes.length}<details class="notices">
        <summary>Preparation notes and files left out ({summary.notes.length})</summary>
        <ul>
          {#each summary.notes as note, i (i)}<li>{note}</li>{/each}
        </ul>
      </details>{/if}
    <div class="summary-foot">
      <span class="caption"
        >{summary.preparedAt
          ? `Last prepared for ${summary.preparedVersion === "pal" ? "PAL" : summary.preparedVersion === "ntsc" ? "NTSC-U" : "an unknown edition"} · ${new Date(summary.preparedAt * 1000).toLocaleString()}`
          : "Preparing a pack does not start the game."}</span
      >
      <div>
        <button class="ghost" disabled={loading} onclick={() => void refresh()}>Refresh</button><button
          disabled={!canPrepare || !inTauri}
          onclick={prepare}>Prepare current mods</button
        >
      </div>
    </div>
  {:else}<p class="muted">Reading your pack selection…</p>{/if}
</section>

<style>
  .load-summary {
    border-top: 3px solid var(--amber);
    margin-bottom: 20px;
  }
  .summary-head,
  .summary-foot {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
  }
  .eyebrow {
    font-size: 11px;
    letter-spacing: 0.1em;
    color: var(--dim);
    margin-bottom: 7px;
  }
  h2 {
    display: flex;
    align-items: center;
    gap: 12px;
    margin: 0;
  }
  .edition,
  .status {
    border: 1px solid var(--line);
    border-radius: 999px;
    padding: 5px 10px;
    font-size: 12px;
    white-space: nowrap;
  }
  .status {
    color: var(--warn);
    background: color-mix(in srgb, var(--warn) 8%, transparent);
  }
  .status.ready {
    color: var(--ok);
    background: color-mix(in srgb, var(--ok) 8%, transparent);
  }
  .notice {
    padding: 12px 14px;
    border-radius: 8px;
    background: rgb(243 197 106 / 0.07);
    line-height: 1.5;
    font-size: 13px;
  }
  .notice.quiet {
    background: transparent;
    padding: 0;
    color: var(--dim);
  }
  .metrics {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    gap: 12px;
    margin: 18px 0;
  }
  .metric {
    background: rgb(255 255 255 / 0.025);
    border: 1px solid var(--line);
    padding: 15px;
    border-radius: 10px;
  }
  .metric > span {
    display: block;
    font-size: 12px;
    color: var(--dim);
  }
  .metric strong {
    display: block;
    font-size: 28px;
    margin: 9px 0 6px;
  }
  .metric strong small {
    margin-left: 4px;
    font-size: 13px;
    font-weight: 400;
    color: var(--dim);
  }
  .metric p {
    margin: 0;
    font-size: 12px;
    color: var(--dim);
  }
  .previous .metric strong {
    color: var(--dim);
  }
  .details-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 24px;
  }
  h3 {
    font-size: 13px;
    margin: 4px 0 12px;
  }
  .count {
    color: var(--dim);
    font-weight: 400;
    margin-left: 5px;
  }
  .selected-packs {
    list-style: none;
    padding: 0;
    margin: 0;
  }
  .selected-packs li {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 0;
    font-size: 13px;
  }
  .selected-packs small {
    margin-left: auto;
    color: var(--dim);
    font-size: 11px;
  }
  .pack-dot {
    width: 6px;
    height: 6px;
    background: var(--amber);
    border-radius: 50%;
  }
  summary {
    cursor: pointer;
    font-size: 13px;
  }
  pre {
    white-space: pre-wrap;
    font-size: 11px;
    line-height: 1.7;
    max-height: 200px;
    overflow: auto;
  }
  .caption {
    color: var(--dim);
    font-size: 11px;
    line-height: 1.5;
  }
  .summary-foot {
    border-top: 1px solid var(--line);
    margin-top: 18px;
    padding-top: 14px;
  }
  .summary-foot > div {
    display: flex;
    gap: 8px;
  }
  .summary-foot button {
    font-size: 12px;
  }
  .notices {
    margin-top: 14px;
    font-size: 12px;
  }
  @media (max-width: 800px) {
    .metrics {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }
    .details-grid {
      grid-template-columns: 1fr;
      gap: 16px;
    }
    .summary-foot {
      align-items: flex-start;
      flex-direction: column;
    }
  }
  @media (max-width: 540px) {
    .metrics {
      grid-template-columns: 1fr;
    }
    .summary-head {
      align-items: flex-start;
      flex-direction: column;
    }
  }
</style>
