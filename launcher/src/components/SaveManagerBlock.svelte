<script lang="ts">
  // In-page save manager section block for a game version (memory card slots & snapshots).
  // Styled strictly with OpenRAC launcher design tokens (panels, pills, cards, retro badges).
  import Icon from "$components/Icon.svelte";
  import { api, type GameSaveStatus, type VersionView } from "$lib/api";
  import { toast } from "$lib/app.svelte";
  import { onMount } from "svelte";

  let { version }: { version: VersionView } = $props();

  let loading = $state(true);
  let status = $state<GameSaveStatus | null>(null);
  let backupNote = $state("");
  let creatingBackup = $state(false);
  let restoringBackupName = $state<string | null>(null);

  async function loadStatus() {
    loading = true;
    try {
      status = await api.inspectSaves(version.serial);
    } catch (e) {
      toast(`Failed to inspect saves: ${String(e)}`, "error");
    } finally {
      loading = false;
    }
  }

  async function createBackup() {
    if (!status) return;
    creatingBackup = true;
    try {
      const res = await api.backupSaves(version.serial, backupNote.trim() || undefined);
      toast(`Backup created: ${res.name}`, "info");
      backupNote = "";
      await loadStatus();
    } catch (e) {
      toast(`Backup failed: ${String(e)}`, "error");
    } finally {
      creatingBackup = false;
    }
  }

  async function restoreBackup(name: string) {
    if (
      !confirm(
        `Restore backup "${name}"? Current saves will be replaced (an automatic safety snapshot will be taken first).`,
      )
    ) {
      return;
    }
    restoringBackupName = name;
    try {
      await api.restoreBackup(version.serial, name);
      toast(`Restored backup: ${name}`, "info");
      await loadStatus();
    } catch (e) {
      toast(`Restore failed: ${String(e)}`, "error");
    } finally {
      restoringBackupName = null;
    }
  }

  async function openFolder() {
    try {
      await api.openSavesFolder(version.serial);
    } catch (e) {
      toast(`Could not open folder: ${String(e)}`, "error");
    }
  }

  function formatBytes(bytes: number): string {
    if (bytes === 0) return "0 B";
    const k = 1024;
    const sizes = ["B", "KB", "MB", "GB"];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return `${parseFloat((bytes / Math.pow(k, i)).toFixed(1))} ${sizes[i]}`;
  }

  onMount(() => {
    void loadStatus();
  });
</script>

<section class="panel saves-section" aria-label="Memory Card & Saves">
  <div class="row header-row">
    <div class="grow">
      <h2>Memory Card & Save Files</h2>
      <p class="muted blurb">
        {#if status?.exists}
          Memory card ready ({status.gameFolderName ?? version.serial}). Five hardware save slots.
        {:else}
          Save directory will be created in <code class="mono">memcard/{version.serial}</code> when the game runs.
        {/if}
      </p>
    </div>

    <div class="header-tools">
      <button class="ghost small" onclick={openFolder} title="Open host memory card directory">
        <Icon name="folder" size={14} />Folder
      </button>
      <button class="ghost small icon-only" onclick={loadStatus} disabled={loading} title="Refresh save slots">
        {#if loading}<span class="spinner"></span>{:else}<Icon name="refresh" size={14} />{/if}
      </button>
    </div>
  </div>

  {#if loading && !status}
    <div class="loading-state">
      <span class="spinner"></span>
      <span class="muted">Scanning memory card...</span>
    </div>
  {:else if status}
    <div class="slots-grid">
      {#each status.slots as slot (slot.slotIndex)}
        <div class="slot-box" class:slot-active={!slot.isEmpty} class:slot-empty={slot.isEmpty}>
          <div class="slot-head">
            <span class="slot-title">SLOT {slot.slotIndex + 1}</span>
            <span class={`pill ${slot.isEmpty ? "info" : "ok"}`}>
              <span class="dot"></span>{slot.isEmpty ? "Empty" : "Saved"}
            </span>
          </div>

          {#if !slot.isEmpty}
            <div class="slot-body">
              <div class="planet-display" title={slot.planetName ?? "Unknown Planet"}>
                <Icon name="globe" size={14} />
                <span class="planet-text">{slot.planetName ?? "Unknown"}</span>
              </div>

              <div class="bolt-row">
                <span class="bolt-badge">
                  <Icon name="bolt" size={14} />
                  <strong>{slot.bolts != null ? slot.bolts.toLocaleString() : "0"}</strong>
                </span>
                <span class="save-time">{slot.timestamp ?? "Ready"}</span>
              </div>

              <div class="slot-footer">
                <span class="mono dim">{slot.filename}</span>
                <span class="dim">{formatBytes(slot.size)}</span>
              </div>
            </div>
          {:else}
            <div class="slot-empty-body">
              <p class="muted">No save data</p>
              <span class="mono dim">{slot.filename}</span>
            </div>
          {/if}
        </div>
      {/each}
    </div>

    <!-- Snapshots & Backups subsection -->
    <div class="snapshots-box">
      <div class="snapshot-bar">
        <div class="snapshot-input-wrap">
          <input
            type="text"
            placeholder="Backup note (e.g. before_drek, 100_percent)"
            bind:value={backupNote}
            disabled={creatingBackup}
            onkeydown={(e) => {
              if (e.key === "Enter") void createBackup();
            }}
          />
        </div>
        <button class="primary small" disabled={creatingBackup || !status.exists} onclick={createBackup}>
          {#if creatingBackup}<span class="spinner"></span>{:else}<Icon name="save" size={14} />{/if}
          Take snapshot
        </button>
      </div>

      {#if status.backups.length > 0}
        <details class="backups-details">
          <summary>Saved snapshots ({status.backups.length})</summary>
          <div class="backups-table">
            {#each status.backups as backup (backup.name)}
              <div class="backup-row">
                <div class="backup-label">
                  <span class="mono backup-name">{backup.name}</span>
                  <span class="dim backup-date">
                    {new Date(backup.createdMillis).toLocaleString()} · {formatBytes(backup.totalSize)}
                  </span>
                </div>
                <button
                  class="ghost small restore-action"
                  disabled={restoringBackupName === backup.name}
                  onclick={() => void restoreBackup(backup.name)}
                >
                  {#if restoringBackupName === backup.name}
                    <span class="spinner"></span>
                  {:else}
                    <Icon name="refresh" size={13} />
                  {/if}
                  Restore
                </button>
              </div>
            {/each}
          </div>
        </details>
      {/if}
    </div>
  {/if}
</section>

<style>
  .saves-section {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .header-row {
    align-items: flex-start;
  }

  .header-tools {
    display: flex;
    align-items: center;
    gap: 8px;
    flex-shrink: 0;
  }

  .icon-only {
    padding: 6px;
    border-radius: 999px;
  }

  .blurb {
    margin-top: 4px;
    font-size: 13.5px;
  }

  .blurb code {
    background: var(--ink-deep);
    border: 1px solid var(--line);
    padding: 2px 6px;
    border-radius: 4px;
  }

  .loading-state {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 24px 0;
  }

  /* Grid of save cards matching OpenRAC layout */
  .slots-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
    gap: 12px;
  }

  .slot-box {
    background: var(--panel-hi);
    border: 1px solid var(--line);
    border-radius: var(--radius);
    padding: 14px;
    display: flex;
    flex-direction: column;
    gap: 10px;
    transition:
      border-color 0.15s,
      background 0.15s;
  }

  .slot-box.slot-active {
    border-color: rgba(234, 179, 8, 0.4);
    background: linear-gradient(180deg, var(--panel-hi) 0%, rgba(234, 179, 8, 0.05) 100%);
  }

  .slot-box.slot-empty {
    opacity: 0.55;
    border-style: dashed;
  }

  .slot-box.slot-empty:hover {
    opacity: 0.8;
  }

  .slot-head {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 6px;
  }

  .slot-title {
    font-family: var(--font-head);
    font-size: 11px;
    font-weight: 700;
    color: var(--gold);
    letter-spacing: 0.12em;
  }

  .slot-body {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .planet-display {
    display: flex;
    align-items: center;
    gap: 6px;
    color: var(--text);
    font-size: 13px;
    font-weight: 600;
  }

  .planet-text {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .bolt-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 6px;
    background: var(--ink-deep);
    border: 1px solid var(--line);
    border-radius: 8px;
    padding: 5px 8px;
  }

  .bolt-badge {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    color: #eab308;
    font-size: 12.5px;
  }

  .bolt-badge strong {
    font-variant-numeric: tabular-nums;
  }

  .save-time {
    font-size: 10.5px;
    color: var(--soft);
    font-variant-numeric: tabular-nums;
  }

  .slot-footer {
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 11px;
    padding-top: 2px;
  }

  .slot-empty-body {
    display: flex;
    flex-direction: column;
    justify-content: center;
    gap: 6px;
    padding: 12px 0 6px;
  }

  .slot-empty-body p {
    font-size: 12.5px;
    font-style: italic;
  }

  /* Snapshots Box */
  .snapshots-box {
    background: var(--ink-deep);
    border: 1px solid var(--line);
    border-radius: var(--radius);
    padding: 14px 16px;
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .snapshot-bar {
    display: flex;
    gap: 10px;
    align-items: center;
  }

  .snapshot-input-wrap {
    flex: 1;
  }

  .snapshot-input-wrap input {
    background: var(--panel);
    border-color: var(--line);
    border-radius: 999px;
    padding: 6px 14px;
    font-size: 13px;
  }

  .backups-details summary {
    font-size: 13px;
    font-weight: 600;
    color: var(--soft);
    cursor: pointer;
  }

  .backups-details summary:hover {
    color: var(--text);
  }

  .backups-table {
    display: flex;
    flex-direction: column;
    gap: 6px;
    margin-top: 10px;
    max-height: 150px;
    overflow-y: auto;
  }

  .backup-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    background: var(--panel);
    border: 1px solid var(--line);
    border-radius: 8px;
    padding: 6px 12px;
  }

  .backup-label {
    display: flex;
    flex-direction: column;
    gap: 2px;
    overflow: hidden;
  }

  .backup-name {
    font-size: 12px;
    color: var(--text);
  }

  .backup-date {
    font-size: 10.5px;
  }

  .restore-action {
    flex-shrink: 0;
  }

  .restore-action:hover {
    color: var(--amber);
  }

  .spinner {
    display: inline-block;
    width: 14px;
    height: 14px;
    border: 2px solid rgba(255, 255, 255, 0.2);
    border-top-color: currentColor;
    border-radius: 50%;
    animation: spin 0.8s linear infinite;
  }

  @keyframes spin {
    to {
      transform: rotate(360deg);
    }
  }
</style>
