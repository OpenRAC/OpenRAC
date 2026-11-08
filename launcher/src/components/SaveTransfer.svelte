<script lang="ts">
  // The Saves panel's Export and Import dialogs (core/src/save_transfer.rs): saves carried to
  // another computer or phone in one OpenRAC saves file (.zip).
  // - Export: all saves, or one slot; the system's save dialog chooses where the file goes.
  // - Import: the file is read and checked first (another game's file or a damaged one is refused,
  //   in the dialog), then its slots are listed with what each replaces on this card. Merge brings
  //   in the ticked slots only; Replace makes the card's saves the file's. The card is backed up
  //   first either way. The words are lib/saveTransfer.ts's.
  import Icon from "$components/Icon.svelte";
  import { api, type SaveImportPreview, type SaveSlotInfo } from "$lib/api";
  import { toast } from "$lib/app.svelte";
  import { modal } from "$lib/portal";
  import { when } from "$lib/reports";
  import {
    TRANSFER,
    exportedText,
    firstTicks,
    importChoice,
    importedText,
    replacesLine,
    savesCount,
    slotKey,
    slotLine,
  } from "$lib/saveTransfer";
  import { onMount } from "svelte";
  import { onIOS } from "$lib/ios";
  import { SvelteSet } from "svelte/reactivity";

  let {
    serial,
    mode,
    used = [],
    source = null,
    onclose,
  }: {
    serial: string;
    mode: "export" | "import";
    /** Export: the card's used slots, to choose one of. */
    used?: Pick<SaveSlotInfo, "slotIndex" | "planetName" | "bolts">[];
    /** Import: the file the player picked. */
    source?: string | null;
    /** `true` when the card changed, so the list reads it again. */
    onclose: (changedCard: boolean) => void;
  } = $props();

  /** Export: null for all saves, else a slot's number. */
  let pick = $state<number | null>(null);
  let preview = $state<SaveImportPreview | null>(null);
  let error = $state<string | null>(null);
  let working = $state(false);
  let replaceAll = $state(false);
  const ticked = new SvelteSet<string>();

  const chosen = $derived(preview ? importChoice(replaceAll, preview.slots, ticked) : null);
  const ready = $derived(
    mode === "export" || (!!preview && preview.slots.length > 0 && (replaceAll || !!chosen?.slots.length)),
  );

  onMount(() => {
    if (mode !== "import" || !source) return;
    api
      .previewSaveImport(serial, source)
      .then((p) => {
        preview = p;
        for (const key of firstTicks(p.slots)) ticked.add(key);
        // A card without saves of its own loses nothing: the file's are brought in whole.
        if (!p.cardHasSaves) replaceAll = true;
      })
      .catch((e: unknown) => (error = String(e)));
  });

  function close() {
    if (!working) onclose(false);
  }

  function toggle(key: string) {
    if (ticked.has(key)) ticked.delete(key);
    else ticked.add(key);
  }

  async function confirm() {
    if (!ready || working) return;
    working = true;
    error = null;
    try {
      if (mode === "export") {
        const done = await api.exportSaves(serial, pick);
        if (!done.saved) {
          working = false; // backed out of the save dialog: the choice stays open
          return;
        }
        toast(exportedText(done.saved, done.slots.length), "info");
        onclose(false);
        return;
      }
      if (!source || !chosen) return;
      const done = await api.importSaves(serial, source, chosen);
      toast(importedText(chosen.replaceAll, chosen.slots.length, !!done.backup), "info");
      onclose(true);
    } catch (e) {
      error = String(e);
      working = false;
    }
  }

  function onkeydown(e: KeyboardEvent) {
    if (e.key === "Escape") close();
  }
</script>

<svelte:window {onkeydown} />

<div class="overlay" role="presentation" use:modal>
  <button class="backdrop" onclick={close} tabindex="-1" aria-label={TRANSFER.cancel}></button>
  <div class="dialog rise" role="dialog" aria-modal="true" aria-labelledby="transfer-title" tabindex="-1">
    <h2 id="transfer-title">{mode === "export" ? TRANSFER.exportTitle : TRANSFER.importTitle}</h2>

    {#if error}
      <p class="error" role="alert"><Icon name="warn" size={16} /><span>{error}</span></p>
    {/if}

    {#if mode === "export"}
      <p class="muted lead">
        {onIOS ? "Creates one OpenRAC saves file to share with another device or save in Files." : TRANSFER.exportLead}
      </p>
      <div class="choices" role="radiogroup" aria-label="What to export">
        <label class="choice">
          <input type="radio" name="export-pick" checked={pick === null} onchange={() => (pick = null)} />
          <span><strong>{TRANSFER.exportAll}</strong> <span class="muted">({savesCount(used.length)})</span></span>
        </label>
        {#each used as s (s.slotIndex)}
          <label class="choice">
            <input
              type="radio"
              name="export-pick"
              checked={pick === s.slotIndex}
              onchange={() => (pick = s.slotIndex)}
            />
            <span>{slotLine(s)}</span>
          </label>
        {/each}
      </div>
    {:else if !preview && !error}
      <p class="muted"><span class="spinner"></span> {TRANSFER.reading}</p>
    {:else if preview}
      <p class="muted lead">
        {preview.title ?? preview.serial} · exported {when(Date.parse(preview.created) / 1000)} · launcher {preview.launcherVersion}
      </p>
      {#if !preview.slots.length}
        <p class="muted">{TRANSFER.nothing}</p>
      {:else}
        <div class="choices" role="radiogroup" aria-label="How to import">
          <label class="choice">
            <input type="radio" name="import-mode" checked={!replaceAll} onchange={() => (replaceAll = false)} />
            <span><strong>{TRANSFER.merge}</strong> <span class="muted">{TRANSFER.mergeLine}</span></span>
          </label>
          <label class="choice">
            <input type="radio" name="import-mode" checked={replaceAll} onchange={() => (replaceAll = true)} />
            <span><strong>{TRANSFER.replace}</strong> <span class="muted">{TRANSFER.replaceLine}</span></span>
          </label>
        </div>
        <ul class="slots">
          {#each preview.slots as s (slotKey(s.slot))}
            {@const key = slotKey(s.slot)}
            <li>
              <label class:off={replaceAll}>
                <input
                  type="checkbox"
                  checked={replaceAll || ticked.has(key)}
                  disabled={replaceAll || working}
                  onchange={() => {
                    toggle(key);
                  }}
                />
                <span class="named">
                  <strong>{slotLine(s.slot)}</strong>
                  <span class:replaces={!!s.replaces} class="muted">{replacesLine(s)}</span>
                </span>
              </label>
            </li>
          {/each}
        </ul>
        {#if preview.cardHasSaves}<p class="muted"><Icon name="info" size={14} /> {TRANSFER.backedUp}</p>{/if}
      {/if}
    {/if}

    <div class="actions">
      <button class="ghost" onclick={close} disabled={working}>{TRANSFER.cancel}</button>
      <!-- A file that was refused has nothing to import: Cancel alone. -->
      {#if mode === "export" || preview}
        <button class="primary" disabled={!ready || working} onclick={() => void confirm()}>
          {#if working}<span class="spinner"></span>{:else}<Icon
              name={mode === "export" ? "upload" : "download"}
              size={16}
            />{/if}{mode === "export" ? (onIOS ? "Share…" : TRANSFER.exportButton) : TRANSFER.importButton}
        </button>
      {/if}
    </div>
  </div>
</div>

<style>
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
    max-width: 520px;
    max-height: 100%;
    overflow-y: auto;
    display: flex;
    flex-direction: column;
    gap: 12px;
    padding: 20px 24px;
    background: var(--card);
    --control: var(--control-card);
    --control-hi: var(--control-card-hi);
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

  .dialog p {
    margin: 0;
    font-size: 0.875rem;
    display: flex;
    align-items: center;
    gap: 6px;
  }

  .dialog p.lead {
    display: block;
  }

  ul {
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .choices {
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .choice,
  .slots label {
    display: flex;
    align-items: flex-start;
    gap: 10px;
    padding: 8px 10px;
    background: var(--control);
    border-radius: var(--radius-sm);
    font-size: 0.875rem;
    cursor: pointer;
  }

  .choice input,
  .slots input {
    margin-top: 3px;
  }

  .slots {
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .slots label.off {
    cursor: default;
  }

  .named {
    display: flex;
    flex-direction: column;
    gap: 2px;
    min-width: 0;
    font-variant-numeric: tabular-nums;
  }

  .named strong {
    font-weight: 600;
  }

  .named .muted {
    font-size: 0.8125rem;
  }

  /* What the card loses. */
  .named .replaces {
    color: var(--amber);
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
    margin-top: 4px;
  }

  @media (max-width: 600px) {
    .actions button {
      flex: 1 1 auto;
      min-height: 44px;
    }
  }
</style>
