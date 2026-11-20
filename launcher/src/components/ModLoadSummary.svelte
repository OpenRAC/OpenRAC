<script lang="ts">
  import { api, inTauri, type VersionView } from "$lib/api";
  import { app } from "$lib/app.svelte";
  import type { ModLoadSummary } from "$lib/generated/ModLoadSummary";
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
  const names = (levels: string[]) =>
    levels.map((id) => levelName("rac1", Number(id.slice(6)))).join(", ");
</script>

<section class="panel load-summary" aria-label="Level and asset pack summary">
  <div class="summary-head">
    <div class="eyebrow">
      LEVELS &amp; ASSET PACKS {#if !inTauri}<span> · PREVIEW DATA</span>{/if}
    </div>
    <div class="title">
      <h2>Before you play <span class="badge">{edition}</span></h2>
      <span class="badge status" class:ready={enabled && current && !hasNotes}>{statusLabel}</span>
    </div>
  </div>
  {#if error}
    <p class="notice">The pack summary could not be read. {error}</p>
    <div class="actions">
      <button class="small" disabled={loading} onclick={() => void refresh()}>Retry</button>
    </div>
  {:else if summary}
    <p class="notice" class:quiet={enabled && current && !hasNotes}>
      {#if !enabled}The next launch uses the original levels. Turn on <strong>Mods</strong> under Play
        to use your edits.
      {:else if !prepared}Your packs are selected, but no prepared content is recorded for {edition} yet.
      {:else if summary.stale}Your selection or packed files changed. Prepare again to confirm the
        next launch.
      {:else if hasNotes}The files are prepared for {edition}, with notes to review below.
      {:else}These are the files prepared for {edition}. Launch with <strong>Mods</strong> enabled to
        use them.{/if}
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
          <p>
            {prepared
              ? item.levels.length
                ? names(item.levels)
                : "Original files"
              : "Prepare to check"}
          </p>
        </div>
      {/each}
    </div>
    {#if prepared && (!enabled || !current)}<p class="caption">
        Counts above describe the previous preparation, not a confirmed next launch.
      </p>{/if}
    <div class="details-grid">
      <div>
        <h3>Selected packs <span class="badge count">{summary.packs.length}</span></h3>
        {#if summary.packs.length}
          <ul class="selected-packs">
            {#each summary.packs as pack (pack.id)}
              <li>
                <span class="pack-dot"></span><span class="pack-name">{pack.name}</span><small
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
      <details class="more recorded-edits" open>
        <summary>Edits recorded by the packer</summary>
        <p class="caption">
          The latest packing report. It can include object, gameplay and collision edits.
        </p>
        {#if summary.packedVersion && summary.packedVersion !== version.name}<p class="notice">
            This report belongs to another disc edition.
          </p>{/if}
        <pre>{summary.packedChanges.length
            ? summary.packedChanges.join("\n")
            : "No packing report available."}</pre>
        {#if summary.changesTruncated}<p class="caption">Showing the first 200 lines.</p>{/if}
      </details>
    </div>
    {#if summary.notes.length}<details class="more notices">
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
      <div class="actions">
        <button class="ghost" disabled={loading} onclick={() => void refresh()}>Refresh</button
        ><button disabled={!canPrepare || !inTauri} onclick={prepare}>Prepare current mods</button>
      </div>
    </div>
  {:else}<p class="muted">Reading your pack selection…</p>{/if}
</section>

<style>
  /* Laid out as the Level editor panel above it (EditorFlow): one column, 16 px between parts. */
  .load-summary {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .summary-head {
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .eyebrow {
    font-size: 0.75rem;
    font-weight: 600;
    letter-spacing: 0.1em;
    color: var(--brand);
  }

  .title {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    justify-content: space-between;
    gap: 8px 16px;
  }

  h2 {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 10px;
  }

  /* Badges sit on --ink-deep, with no border (STYLE.md). */
  .badge {
    display: inline-flex;
    align-items: center;
    padding: 3px 8px;
    border-radius: var(--radius-xs);
    background: var(--ink-deep);
    color: var(--soft);
    font-family: var(--font-sans);
    font-size: 0.75rem;
    font-weight: 600;
    letter-spacing: 0;
    white-space: nowrap;
  }

  .status {
    color: var(--warn);
  }

  .status.ready {
    color: var(--ok);
  }

  /* A hint inside a page: --panel mixed 9 % with the accent (STYLE.md). */
  .notice {
    padding: 12px 14px;
    border-radius: var(--radius-sm);
    background: color-mix(in srgb, var(--amber) 9%, var(--panel));
    color: var(--body);
    font-size: 0.875rem;
    line-height: 1.5;
  }

  .notice.quiet {
    padding: 0;
    background: transparent;
    color: var(--soft);
  }

  .metrics {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    gap: 8px;
  }

  .metric {
    display: flex;
    flex-direction: column;
    gap: 4px;
    min-width: 0;
    padding: 12px 14px;
    border-radius: var(--radius-sm);
    background: var(--card-hi);
  }

  .metric > span {
    font-size: 0.8125rem;
    color: var(--soft);
  }

  .metric strong {
    font-size: 1.375rem;
    font-weight: 700;
    line-height: 1.3;
    color: var(--gold);
  }

  .metric strong small {
    margin-left: 4px;
    font-size: 0.8125rem;
    font-weight: 400;
    color: var(--soft);
  }

  .metric p {
    font-size: 0.8125rem;
    color: var(--soft);
    overflow-wrap: anywhere;
  }

  .previous .metric strong {
    color: var(--dim);
  }

  .details-grid {
    display: grid;
    grid-template-columns: minmax(0, 1fr) minmax(0, 1fr);
    gap: 16px 24px;
    align-items: start;
  }

  .details-grid > div {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  h3 {
    display: flex;
    align-items: center;
    gap: 8px;
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
    font-size: 0.875rem;
  }

  .selected-packs li + li {
    border-top: 1px solid var(--line);
  }

  .pack-name {
    min-width: 0;
    overflow-wrap: anywhere;
  }

  .selected-packs small {
    margin-left: auto;
    font-size: 0.75rem;
    color: var(--soft);
    white-space: nowrap;
  }

  .pack-dot {
    flex: 0 0 auto;
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--amber);
  }

  /* The fold's title reads as the h3 beside it (details.more in app.css draws the arrow). */
  .recorded-edits > summary {
    min-height: 24px;
    font-family: var(--font-head);
    font-size: 0.9375rem;
    font-weight: 700;
    color: var(--text);
  }

  .recorded-edits > :not(summary),
  .notices > :not(summary) {
    margin-top: 8px;
  }

  pre {
    margin: 0;
    padding: 10px 12px;
    max-height: 220px;
    overflow: auto;
    white-space: pre-wrap;
    word-break: break-word;
    font-family: var(--font-mono);
    font-size: 0.75rem;
    line-height: 1.6;
    color: var(--soft);
    background: var(--ink-deep);
    border-radius: var(--radius-sm);
  }

  .caption {
    font-size: 0.8125rem;
    line-height: 1.5;
    color: var(--soft);
  }

  .muted {
    font-size: 0.875rem;
  }

  .notices > summary {
    font-size: 0.875rem;
    color: var(--body);
  }

  .notices ul {
    margin-bottom: 0;
    padding-left: 18px;
    font-size: 0.8125rem;
    color: var(--soft);
  }

  .summary-foot {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    justify-content: space-between;
    gap: 12px 16px;
    padding-top: 16px;
    border-top: 1px solid var(--line);
  }

  .actions {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
  }

  @media (max-width: 800px) {
    .metrics {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .details-grid {
      grid-template-columns: minmax(0, 1fr);
    }
  }
</style>
