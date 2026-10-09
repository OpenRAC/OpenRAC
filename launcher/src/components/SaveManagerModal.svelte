<script lang="ts">
  // Save manager prototype modal: displays memory card slots (save0..save4),
  // timestamps, snapshots/backups and allows creating or restoring backups.
  import Icon from "$components/Icon.svelte";
  import { api, type GameSaveStatus, type VersionView } from "$lib/api";
  import { toast } from "$lib/app.svelte";
  import { onMount } from "svelte";

  let { version, onclose }: { version: VersionView; onclose: () => void } = $props();

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

<div class="modal-backdrop" onclick={onclose} role="presentation">
  <!-- svelte-ignore a11y_click_events_have_key_events -->
  <div
    class="modal-card rise"
    onclick={(e) => {
      e.stopPropagation();
    }}
    role="dialog"
    tabindex="-1"
    aria-modal="true"
    aria-labelledby="save-mgr-title"
  >
    <div class="modal-header">
      <div class="header-title">
        <Icon name="save" size={22} />
        <div>
          <h2 id="save-mgr-title">Save Management (Prototype)</h2>
          <small>{version.title} · {version.serial} ({version.region})</small>
        </div>
      </div>
      <button class="ghost close-btn" onclick={onclose} aria-label="Close">
        <Icon name="x" size={18} />
      </button>
    </div>

    <div class="modal-body">
      {#if loading && !status}
        <div class="loading-state">
          <span class="spinner"></span>
          <p>Scanning memory card directory...</p>
        </div>
      {:else if status}
        <section class="section">
          <div class="section-header">
            <h3>Memory Card Slots</h3>
            <button class="ghost small-btn" onclick={openFolder}>
              <Icon name="folder" size={14} />Open folder
            </button>
          </div>
          <p class="section-desc">
            Location: <code>{status.memcardDir}</code>
            {#if status.gameFolderName}
              · PS2 Game ID: <code>{status.gameFolderName}</code>
            {/if}
          </p>

          <div class="slots-grid">
            {#each status.slots as slot (slot.slotIndex)}
              <div class="slot-card" class:slot-empty={slot.isEmpty} class:slot-active={!slot.isEmpty}>
                <div class="slot-header">
                  <span class="slot-badge">Slot {slot.slotIndex + 1}</span>
                  <span class="slot-status">{slot.isEmpty ? "Empty" : "Active Save"}</span>
                </div>
                <div class="slot-meta">
                  <div class="meta-row">
                    <span class="meta-label">File:</span>
                    <span class="meta-value">{slot.filename}</span>
                  </div>
                  {#if !slot.isEmpty}
                    <div class="meta-row">
                      <span class="meta-label">Saved:</span>
                      <span class="meta-value highlight">{slot.timestamp ?? "Unknown"}</span>
                    </div>
                    <div class="meta-row">
                      <span class="meta-label">Size:</span>
                      <span class="meta-value">{formatBytes(slot.size)}</span>
                    </div>
                  {/if}
                </div>
              </div>
            {/each}
          </div>
        </section>

        <section class="section">
          <div class="section-header">
            <h3>Snapshots & Backups</h3>
          </div>
          <p class="section-desc">
            Create isolated backups to prevent progress loss, test different save states, or share saves.
          </p>

          <div class="backup-form">
            <input
              type="text"
              placeholder="Backup note (optional, e.g. planet_rilgar)"
              bind:value={backupNote}
              disabled={creatingBackup}
              onkeydown={(e) => {
                if (e.key === "Enter") void createBackup();
              }}
            />
            <button class="primary" disabled={creatingBackup || !status.exists} onclick={createBackup}>
              {#if creatingBackup}<span class="spinner"></span>{:else}<Icon name="save" size={16} />{/if}
              Take Snapshot
            </button>
          </div>

          {#if status.backups.length > 0}
            <div class="backups-list">
              {#each status.backups as backup (backup.name)}
                <div class="backup-item">
                  <div class="backup-info">
                    <span class="backup-name">{backup.name}</span>
                    <span class="backup-meta">
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
                      <Icon name="refresh" size={14} />
                    {/if}
                    Restore
                  </button>
                </div>
              {/each}
            </div>
          {:else}
            <div class="empty-backups">No snapshots created yet.</div>
          {/if}
        </section>
      {/if}
    </div>
  </div>
</div>

<style>
  .modal-backdrop {
    position: fixed;
    inset: 0;
    z-index: 1000;
    background: rgba(10, 12, 16, 0.75);
    backdrop-filter: blur(4px);
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 24px;
  }

  .modal-card {
    background: var(--bg);
    border: 1px solid var(--bd);
    border-radius: 20px;
    width: 100%;
    max-width: 680px;
    max-height: 85vh;
    display: flex;
    flex-direction: column;
    box-shadow: 0 16px 40px rgba(0, 0, 0, 0.6);
    overflow: hidden;
  }

  .modal-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 20px 24px;
    border-bottom: 1px solid var(--bd);
    background: var(--box);
  }

  .header-title {
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .header-title h2 {
    font-size: 18px;
    font-weight: 600;
    margin: 0;
    color: var(--tx);
  }

  .header-title small {
    display: block;
    font-size: 12px;
    color: var(--tx2);
    margin-top: 2px;
  }

  .close-btn {
    padding: 6px;
    border-radius: 8px;
    color: var(--tx2);
  }

  .modal-body {
    padding: 20px 24px;
    overflow-y: auto;
    display: flex;
    flex-direction: column;
    gap: 24px;
  }

  .loading-state {
    padding: 40px;
    text-align: center;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 12px;
    color: var(--tx2);
  }

  .section {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .section-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
  }

  .section-header h3 {
    font-size: 15px;
    font-weight: 600;
    color: var(--tx);
    margin: 0;
  }

  .section-desc {
    font-size: 12px;
    color: var(--tx2);
    margin: 0;
  }

  .section-desc code {
    background: var(--code-bg, rgba(255, 255, 255, 0.06));
    padding: 2px 6px;
    border-radius: 4px;
    font-size: 11px;
  }

  .slots-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(115px, 1fr));
    gap: 10px;
  }

  .slot-card {
    background: var(--box);
    border: 1px solid var(--bd);
    border-radius: 12px;
    padding: 10px;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .slot-card.slot-active {
    border-color: rgba(var(--tint-rgb, 255, 140, 0), 0.4);
    background: linear-gradient(180deg, var(--box) 0%, rgba(var(--tint-rgb, 255, 140, 0), 0.05) 100%);
  }

  .slot-card.slot-empty {
    opacity: 0.6;
  }

  .slot-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .slot-badge {
    font-weight: 600;
    font-size: 12px;
    color: var(--tx);
  }

  .slot-status {
    font-size: 10px;
    color: var(--tx2);
  }

  .slot-active .slot-status {
    color: var(--ok, #4ade80);
    font-weight: 500;
  }

  .slot-meta {
    display: flex;
    flex-direction: column;
    gap: 4px;
    font-size: 11px;
  }

  .meta-row {
    display: flex;
    justify-content: space-between;
    gap: 4px;
  }

  .meta-label {
    color: var(--tx2);
  }

  .meta-value {
    color: var(--tx);
    font-family: var(--mono);
    font-size: 10px;
  }

  .meta-value.highlight {
    color: var(--brand, #ff9800);
    font-weight: 500;
  }

  .backup-form {
    display: flex;
    gap: 10px;
  }

  .backup-form input {
    flex: 1;
    background: var(--input-bg, rgba(0, 0, 0, 0.2));
    border: 1px solid var(--bd);
    border-radius: 8px;
    padding: 8px 12px;
    color: var(--tx);
    font-size: 13px;
  }

  .backup-form input:focus {
    outline: none;
    border-color: var(--brand, #ff9800);
  }

  .backups-list {
    display: flex;
    flex-direction: column;
    gap: 8px;
    max-height: 180px;
    overflow-y: auto;
  }

  .backup-item {
    background: var(--box);
    border: 1px solid var(--bd);
    border-radius: 10px;
    padding: 8px 12px;
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .backup-info {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .backup-name {
    font-size: 13px;
    font-weight: 500;
    color: var(--tx);
    font-family: var(--mono);
  }

  .backup-meta {
    font-size: 11px;
    color: var(--tx2);
  }

  .empty-backups {
    font-size: 12px;
    color: var(--tx2);
    font-style: italic;
    padding: 8px 0;
  }

  .small-btn {
    padding: 4px 8px;
    font-size: 12px;
    gap: 6px;
    border-radius: 6px;
  }

  .restore-btn:hover {
    color: var(--brand, #ff9800);
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
