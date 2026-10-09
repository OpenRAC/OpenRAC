<script lang="ts">
  import type { IsoInspection, VersionView } from "$lib/api";
  import { api, pickIsoFile } from "$lib/api";
  import { refresh, toast } from "$lib/app.svelte";
  import { size } from "$lib/format";
  import Icon from "./Icon.svelte";

  let {
    version,
    onclose,
  }: {
    version: VersionView;
    onclose: () => void;
  } = $props();

  let inspecting = $state(false);
  let importing = $state(false);
  let inspection = $state<IsoInspection | null>(null);
  let errorMsg = $state<string | null>(null);
  let successMsg = $state<string | null>(null);

  async function chooseFile() {
    errorMsg = null;
    successMsg = null;
    try {
      const picked = await pickIsoFile(`Select ISO for ${version.title} (${version.region})`);
      if (!picked) return;
      inspecting = true;
      inspection = await api.inspectIso(version.key, picked);
    } catch (e) {
      errorMsg = typeof e === "string" ? e : (e as Error).message;
    } finally {
      inspecting = false;
    }
  }

  async function performImport() {
    if (!inspection) return;
    importing = true;
    errorMsg = null;
    try {
      const res = await api.importIso(version.key, inspection.path);
      successMsg = `Assets extracted to ${res.extractedAssetsDir} (${res.extractedFiles.length} files). Inputs placed!`;
      toast("Imported disc & extracted assets", "info");
      await refresh();
      setTimeout(() => {
        onclose();
      }, 1800);
    } catch (e) {
      errorMsg = typeof e === "string" ? e : (e as Error).message;
    } finally {
      importing = false;
    }
  }
</script>

<div class="modal-backdrop" onclick={onclose} role="presentation">
  <!-- svelte-ignore a11y_click_events_have_key_events -->
  <div
    class="modal"
    onclick={(e) => {
      e.stopPropagation();
    }}
    role="dialog"
    aria-modal="true"
    tabindex="-1"
  >
    <div class="head">
      <div>
        <h3>Import ISO · {version.title}</h3>
        <p class="muted">
          Expected: <span class="mono">{version.serial}</span> · {version.region}
          {#if version.disc?.size}· {size(version.disc.size)}{/if}
        </p>
      </div>
      <button class="ghost close" onclick={onclose} aria-label="Close">✕</button>
    </div>

    {#if !inspection}
      <div class="pick-area">
        <p class="desc">
          Select an ISO image from your computer (e.g. from <code>/home/lynder063/Downloads/games-ps2/</code>). The
          launcher will verify the game version before importing.
        </p>
        <button class="primary big" onclick={() => void chooseFile()} disabled={inspecting}>
          <Icon name="disc" size={18} />
          {inspecting ? "Inspecting ISO..." : "Choose ISO File"}
        </button>
      </div>
    {:else}
      <div class="inspection-results">
        <div class="file-info row">
          <Icon name="disc" size={24} />
          <div class="grow">
            <strong>{inspection.filename}</strong>
            <span class="muted mono">({size(inspection.size)})</span>
          </div>
          <button class="ghost small" onclick={() => void chooseFile()} disabled={importing}> Change file </button>
        </div>

        <div
          class="status-card"
          class:ok={inspection.status === "exactMatch"}
          class:warn={inspection.status === "revisionMismatch"}
          class:err={inspection.status === "wrongGame" ||
            inspection.status === "invalidIso" ||
            inspection.status === "notPs2Disc"}
        >
          <div class="row">
            <span class="status-badge">
              {#if inspection.status === "exactMatch"}
                ✅ Exact Match
              {:else if inspection.status === "revisionMismatch"}
                ⚠️ Revision Mismatch
              {:else if inspection.status === "wrongGame"}
                ❌ Wrong Game
              {:else}
                ❌ Invalid Disc
              {/if}
            </span>
            <span class="grow"></span>
            {#if inspection.serial}
              <span class="mono serial-pill">{inspection.serial}</span>
            {/if}
          </div>
          <p class="message">{inspection.message}</p>
        </div>

        {#if inspection.detectedGameTitle}
          <div class="details">
            <div class="row">
              <span class="muted">Detected:</span>
              <span><strong>{inspection.detectedGameTitle}</strong> ({inspection.detectedRegion ?? "PS2"})</span>
            </div>
            <div class="row">
              <span class="muted">Target:</span>
              <span><strong>{version.title}</strong> ({version.region})</span>
            </div>
            <div class="row">
              <span class="muted">Assets extract to:</span>
              <span class="mono">/home/lynder063/OpenRAC/{version.game}</span>
            </div>
          </div>
        {/if}

        {#if successMsg}
          <div class="notice ok">
            <Icon name="check" size={16} />
            <p>{successMsg}</p>
          </div>
        {/if}

        {#if errorMsg}
          <div class="notice err">
            <Icon name="warn" size={16} />
            <p>{errorMsg}</p>
          </div>
        {/if}

        <div class="actions-row">
          <button class="ghost" onclick={onclose} disabled={importing}>Cancel</button>
          {#if inspection.status === "exactMatch" || inspection.status === "revisionMismatch"}
            <button class="primary" onclick={() => void performImport()} disabled={importing || !!successMsg}>
              {#if importing}
                Extracting assets & placing inputs...
              {:else}
                Import & Extract Assets
              {/if}
            </button>
          {/if}
        </div>
      </div>
    {/if}
  </div>
</div>

<style>
  .modal-backdrop {
    position: fixed;
    inset: 0;
    background: rgba(8, 8, 12, 0.75);
    backdrop-filter: blur(4px);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 999;
    padding: 20px;
  }

  .modal {
    background: var(--bg-card, #1c1c26);
    border: 1px solid var(--border, #2d2d3d);
    border-radius: 20px;
    width: 100%;
    max-width: 580px;
    box-shadow: 0 20px 50px rgba(0, 0, 0, 0.6);
    padding: 24px;
    display: flex;
    flex-direction: column;
    gap: 18px;
  }

  .head {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.08);
    padding-bottom: 14px;
  }

  .head h3 {
    margin: 0 0 4px;
    font-size: 18px;
    color: var(--text, #fff);
  }

  .close {
    border: none;
    font-size: 18px;
    padding: 4px 8px;
    cursor: pointer;
  }

  .pick-area {
    display: flex;
    flex-direction: column;
    align-items: center;
    text-align: center;
    padding: 30px 10px;
    gap: 18px;
  }

  .desc {
    color: var(--soft, #a0a0b0);
    max-width: 440px;
    line-height: 1.5;
    font-size: 14px;
  }

  .big {
    padding: 12px 24px;
    font-size: 15px;
    gap: 8px;
  }

  .inspection-results {
    display: flex;
    flex-direction: column;
    gap: 14px;
  }

  .file-info {
    background: rgba(255, 255, 255, 0.04);
    padding: 10px 14px;
    border-radius: 12px;
    gap: 12px;
    align-items: center;
  }

  .status-card {
    border-radius: 12px;
    padding: 12px 14px;
    border: 1px solid transparent;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .status-card.ok {
    background: rgba(30, 180, 100, 0.1);
    border-color: rgba(30, 180, 100, 0.35);
    color: #4ade80;
  }

  .status-card.warn {
    background: rgba(240, 160, 40, 0.1);
    border-color: rgba(240, 160, 40, 0.35);
    color: #fbbf24;
  }

  .status-card.err {
    background: rgba(240, 60, 60, 0.1);
    border-color: rgba(240, 60, 60, 0.35);
    color: #f87171;
  }

  .status-badge {
    font-weight: 700;
    font-size: 13px;
  }

  .serial-pill {
    background: rgba(0, 0, 0, 0.3);
    padding: 2px 8px;
    border-radius: 6px;
    font-size: 12px;
  }

  .message {
    margin: 0;
    font-size: 13px;
    line-height: 1.4;
    color: var(--text, #eee);
  }

  .details {
    background: rgba(0, 0, 0, 0.2);
    border-radius: 10px;
    padding: 10px 14px;
    display: flex;
    flex-direction: column;
    gap: 6px;
    font-size: 13px;
  }

  .notice {
    padding: 10px 14px;
    border-radius: 10px;
    display: flex;
    align-items: center;
    gap: 10px;
    font-size: 13px;
  }

  .notice.ok {
    background: rgba(30, 180, 100, 0.15);
    color: #4ade80;
  }

  .notice.err {
    background: rgba(240, 60, 60, 0.15);
    color: #f87171;
  }

  .actions-row {
    display: flex;
    justify-content: flex-end;
    gap: 10px;
    margin-top: 6px;
  }
</style>
