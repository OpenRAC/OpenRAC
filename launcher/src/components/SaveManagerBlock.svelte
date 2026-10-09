<script lang="ts">
  // In-page save manager section block for a game version (memory card slots & snapshots).
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
  let showBackups = $state(false);

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
      showBackups = true;
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

<section class="panel save-block" aria-label="Save files & Memory Card">
  <div class="block-top">
    <div class="grow">
      <div class="title-row">
        <Icon name="save" size={20} />
        <h2>Save files & Memory Card</h2>
      </div>
      <p class="muted blurb">
        {#if status?.exists}
          Memory card ready ({status.gameFolderName ?? version.serial}). 5 hardware save slots (save0–save4).
        {:else}
          Save directory will be created automatically in <code>openrac/memcard/{version.serial}</code> when the game runs.
        {/if}
      </p>
    </div>

    <div class="header-actions">
      <button class="ghost small-btn" onclick={openFolder} title="Open host memory card directory">
        <Icon name="folder" size={15} />Open folder
      </button>
      <button class="ghost small-btn" onclick={loadStatus} disabled={loading} title="Refresh save slots">
        {#if loading}<span class="spinner"></span>{:else}<Icon name="refresh" size={15} />{/if}
      </button>
    </div>
  </div>

  {#if loading && !status}
    <div class="loading-state">
      <span class="spinner"></span>
      <span class="muted">Scanning save files...</span>
    </div>
  {:else if status}
    <div class="slots-row">
      {#each status.slots as slot (slot.slotIndex)}
        <div class="slot-card" class:active={!slot.isEmpty} class:empty={slot.isEmpty}>
          <div class="slot-top">
            <span class="slot-tag">Slot {slot.slotIndex + 1}</span>
            <span class="slot-badge">{slot.isEmpty ? "Empty" : "Saved"}</span>
          </div>
          <div class="slot-content">
            {#if !slot.isEmpty}
              <div class="slot-planet" title={slot.planetName ?? "Unknown Planet"}>
                <Icon name="globe" size={13} />
                <span class="planet-name">{slot.planetName ?? "Unknown Planet"}</span>
              </div>
              <div class="slot-stats">
                <div class="slot-bolts" title="Bolts">
                  <Icon name="bolt" size={12} />
                  <span>{slot.bolts != null ? slot.bolts.toLocaleString() : "0"}</span>
                </div>
                <div class="slot-date">{slot.timestamp ?? "Ready"}</div>
              </div>
              <div class="slot-sub">{slot.filename} · {formatBytes(slot.size)}</div>
            {:else}
              <div class="slot-empty-text">No save data</div>
              <div class="slot-sub">{slot.filename}</div>
            {/if}
          </div>
        </div>
      {/each}
    </div>

    <!-- Snapshots & Backup section inside block -->
    <div class="snapshot-section">
      <div class="snapshot-controls">
        <input
          type="text"
          placeholder="Snapshot note (e.g. before_boss, 100_percent)"
          bind:value={backupNote}
          disabled={creatingBackup}
          onkeydown={(e) => {
            if (e.key === "Enter") void createBackup();
          }}
        />
        <button class="primary" disabled={creatingBackup || !status.exists} onclick={createBackup}>
          {#if creatingBackup}<span class="spinner"></span>{:else}<Icon name="save" size={15} />{/if}
          Take snapshot
        </button>
        {#if status.backups.length > 0}
          <button class="ghost toggle-btn" onclick={() => (showBackups = !showBackups)}>
            {showBackups ? "Hide backups" : `Backups (${status.backups.length})`}
          </button>
        {/if}
      </div>

      {#if showBackups && status.backups.length > 0}
        <div class="backups-list">
          {#each status.backups as backup (backup.name)}
            <div class="backup-item">
              <div class="backup-meta">
                <span class="backup-name">{backup.name}</span>
                <span class="backup-date">
                  {new Date(backup.createdMillis).toLocaleString()} · {formatBytes(backup.totalSize)}
                </span>
              </div>
              <button
                class="ghost small-btn restore-btn"
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
      {/if}
    </div>
  {/if}
</section>

<style>
  .save-block {
    display: flex;
    flex-direction: column;
    gap: 16px;
    padding: 20px 24px;
    background: var(--box);
    border: 1px solid var(--bd);
    border-radius: 20px;
  }

  .block-top {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
  }

  .title-row {
    display: flex;
    align-items: center;
    gap: 10px;
    color: var(--tx);
  }

  .title-row h2 {
    font-size: 18px;
    font-weight: 600;
    margin: 0;
  }

  .blurb {
    margin: 6px 0 0;
    font-size: 13px;
    color: var(--tx2);
  }

  .blurb code {
    background: var(--code-bg, rgba(255, 255, 255, 0.06));
    padding: 2px 6px;
    border-radius: 4px;
    font-size: 12px;
  }

  .header-actions {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .small-btn {
    padding: 6px 12px;
    font-size: 12px;
    gap: 6px;
    border-radius: 8px;
  }

  .loading-state {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 12px 0;
    font-size: 13px;
  }

  .slots-row {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
    gap: 10px;
  }

  .slot-card {
    background: var(--bg);
    border: 1px solid var(--bd);
    border-radius: 12px;
    padding: 12px;
    display: flex;
    flex-direction: column;
    gap: 8px;
    transition:
      border-color 0.2s,
      background 0.2s;
  }

  .slot-card.active {
    border-color: rgba(var(--tint-rgb, 255, 140, 0), 0.45);
    background: linear-gradient(180deg, var(--bg) 0%, rgba(var(--tint-rgb, 255, 140, 0), 0.06) 100%);
  }

  .slot-card.empty {
    opacity: 0.6;
  }

  .slot-top {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .slot-tag {
    font-size: 11px;
    font-weight: 600;
    color: var(--tx);
  }

  .slot-badge {
    font-size: 10px;
    color: var(--tx2);
  }

  .slot-card.active .slot-badge {
    color: var(--ok, #4ade80);
    font-weight: 600;
  }

  .slot-content {
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .slot-planet {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 13px;
    font-weight: 600;
    color: var(--tx);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .planet-name {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .slot-stats {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 6px;
  }

  .slot-bolts {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    font-size: 12px;
    font-weight: 600;
    color: #eab308;
  }

  .slot-date {
    font-size: 11px;
    color: var(--tx2);
  }

  .slot-empty-text {
    font-size: 12px;
    color: var(--tx2);
    font-style: italic;
    padding: 6px 0;
  }

  .slot-sub {
    font-size: 10px;
    color: var(--tx2);
    font-family: var(--mono);
  }

  .snapshot-section {
    display: flex;
    flex-direction: column;
    gap: 10px;
    padding-top: 10px;
    border-top: 1px solid var(--bd);
  }

  .snapshot-controls {
    display: flex;
    gap: 10px;
    align-items: center;
  }

  .snapshot-controls input {
    flex: 1;
    background: var(--input-bg, rgba(0, 0, 0, 0.25));
    border: 1px solid var(--bd);
    border-radius: 8px;
    padding: 7px 12px;
    color: var(--tx);
    font-size: 13px;
  }

  .snapshot-controls input:focus {
    outline: none;
    border-color: var(--brand, #ff9800);
  }

  .toggle-btn {
    font-size: 12px;
    padding: 7px 12px;
    border-radius: 8px;
  }

  .backups-list {
    display: flex;
    flex-direction: column;
    gap: 6px;
    max-height: 160px;
    overflow-y: auto;
    background: var(--bg);
    border: 1px solid var(--bd);
    border-radius: 10px;
    padding: 8px;
  }

  .backup-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 6px 10px;
    border-radius: 6px;
    background: var(--box);
  }

  .backup-meta {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .backup-name {
    font-size: 12px;
    font-weight: 500;
    color: var(--tx);
    font-family: var(--mono);
  }

  .backup-date {
    font-size: 11px;
    color: var(--tx2);
  }

  .restore-btn:hover {
    color: var(--brand, #ff9800);
  }

  .spinner {
    display: inline-block;
    width: 13px;
    height: 13px;
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
