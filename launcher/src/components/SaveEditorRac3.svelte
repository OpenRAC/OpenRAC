<script lang="ts">
  // The Up Your Arsenal save editor, a first view: one slot of the memory card, opened from the
  // Save files list. It shows what core/src/save_edit_rac3.rs reads (format:
  // docs/RAC3_SAVE_FORMAT.md): where the save resumes, the bolts and the weapons, and changes the
  // bolts and the ammo of the weapons the save owns. It saves through write_save_rac3, which
  // refuses while the game runs with the card and backs the card up first, as the Going Commando
  // editor (SaveEditorRac2.svelte) does.
  import { onMount } from "svelte";

  import Icon from "$components/Icon.svelte";
  import { api, type Rac3SaveFields, type Rac3SaveView, type SaveBackupInfo } from "$lib/api";
  import { toast } from "$lib/app.svelte";
  import { inertPage, portal } from "$lib/portal";
  import { changedRac3, fieldErrorsRac3, playTime } from "$lib/saveEditRac3";

  let {
    serial,
    slot,
    playing = false,
    onclose,
  }: {
    serial: string;
    slot: number;
    /** The launcher is running the game, which writes the card too: saving waits until it is closed. */
    playing?: boolean;
    /** `true` when the card changed (saved or undone), so the list reads it again. */
    onclose: (changedCard: boolean) => void;
  } = $props();

  let view = $state<Rac3SaveView | null>(null);
  let fields = $state<Rac3SaveFields | null>(null);
  let loadError = $state<string | null>(null);
  let saveError = $state<string | null>(null);
  let busy = $state(false);
  let saved = $state<SaveBackupInfo | null>(null);
  let touchedCard = false;
  let modalEl = $state<HTMLElement | null>(null);

  const errors = $derived(view && fields ? fieldErrorsRac3(view, fields) : {});
  const dirty = $derived(!!view && !!fields && changedRac3(view, fields));
  const canSave = $derived(dirty && !busy && !playing && Object.keys(errors).length === 0);

  async function load() {
    loadError = null;

    try {
      const read = await api.readSaveRac3(serial, slot);

      view = read;
      fields = structuredClone(read.fields);
    } catch (e) {
      loadError = String(e);
    }
  }

  async function save() {
    if (!view || !fields || !canSave) return;
    busy = true;
    saveError = null;

    try {
      const result = await api.writeSaveRac3(
        serial,
        slot,
        view.fingerprint,
        $state.snapshot(fields),
      );

      view = result.view;
      fields = structuredClone(result.view.fields);
      saved = result.backup;
      touchedCard = true;
    } catch (e) {
      saveError = String(e);
    } finally {
      busy = false;
    }
  }

  async function undo() {
    if (!saved) return;
    busy = true;
    saveError = null;

    try {
      await api.restoreBackup(serial, saved.name);
      toast(`Slot ${String(slot + 1)} is back as it was before the edit.`, "info");
      onclose(true);
    } catch (e) {
      saveError = String(e);
    } finally {
      busy = false;
    }
  }

  function close() {
    if (busy) return;
    onclose(touchedCard);
  }

  function reset() {
    if (view) fields = $state.snapshot(view.fields);
  }

  onMount(() => {
    const back = document.activeElement instanceof HTMLElement ? document.activeElement : null;
    const release = inertPage();

    modalEl?.focus();
    void load();

    return () => {
      release();
      back?.focus();
    };
  });
</script>

<svelte:window
  onkeydown={(e) => {
    if (e.key === "Escape") close();
    else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "s") {
      e.preventDefault();
      if (canSave) void save();
    }
  }}
/>

<div class="overlay" role="presentation" use:portal>
  <button class="backdrop" onclick={close} tabindex="-1" aria-label="Close the save editor"
  ></button>
  <div
    class="modal rise"
    role="dialog"
    aria-modal="true"
    aria-labelledby="save-editor-rac3-title"
    tabindex="-1"
    bind:this={modalEl}
  >
    <div class="head">
      <div class="grow">
        <h2 id="save-editor-rac3-title">
          {view
            ? `Slot ${String(slot + 1)} · ${view.planetName ?? "Unknown planet"}`
            : `Slot ${String(slot + 1)}`}
        </h2>
        {#if view}
          <p class="sub">
            {#if view.savedAt}Saved {view.savedAt} ·
            {/if}Played {playTime(view.playSeconds)}
          </p>
        {/if}
      </div>
      <button class="ghost small" onclick={close} disabled={busy} aria-label="Close">
        <Icon name="x" size={16} />
      </button>
    </div>

    <div class="body">
      {#if loadError}
        <p class="problem"><Icon name="warn" size={16} />{loadError}</p>
      {:else if !view || !fields}
        <p class="muted row"><span class="spinner"></span>Reading the save…</p>
      {:else}
        <section aria-labelledby="se3-ratchet-h">
          <h3 id="se3-ratchet-h">Ratchet</h3>
          <div class="line">
            <label for="se3-bolts">Bolts</label>
            <div class="field">
              <span class="bolt"><Icon name="bolt" size={16} /></span>
              <input
                id="se3-bolts"
                type="number"
                min="0"
                max={view.maxBolts}
                step="1"
                bind:value={fields.bolts}
                aria-invalid={!!errors.bolts}
              />
              <button
                class="ghost small"
                onclick={() => {
                  if (view && fields) fields.bolts = view.maxBolts;
                }}
                disabled={fields.bolts === view.maxBolts}>Max</button
              >
              {#if errors.bolts}<span class="err-text">{errors.bolts}</span>{/if}
            </div>
          </div>
        </section>

        <section aria-labelledby="se3-weapons-h">
          <h3 id="se3-weapons-h">Weapons</h3>
          <p class="note">
            Owning a weapon and its version are shown as the save has them. The ammo of an owned
            weapon can be changed, up to its version's full ammo.
          </p>
          <table>
            <thead>
              <tr><th>Weapon</th><th>Version</th><th>Ammo</th></tr>
            </thead>
            <tbody>
              {#each fields.weapons as state, i (state.id)}
                {@const w = view.weapons[i] ?? {
                  id: state.id,
                  name: "",
                  versionName: "",
                  version: 1,
                  maxAmmo: 0,
                  xp: 0,
                }}
                <tr class:dim={!state.owned}>
                  <td>{w.name}</td>
                  <td>{state.owned ? w.versionName : "Not owned"}</td>
                  <td>
                    {#if state.owned && w.maxAmmo > 0}
                      <input
                        type="number"
                        min="0"
                        max={w.maxAmmo}
                        step="1"
                        aria-label={`${w.name} ammo`}
                        bind:value={state.ammo}
                        aria-invalid={!!errors[`ammo-${String(w.id)}`]}
                      />
                      <span class="dim"> / {w.maxAmmo}</span>
                      <button
                        class="ghost small"
                        onclick={() => {
                          state.ammo = w.maxAmmo;
                        }}
                        disabled={state.ammo === w.maxAmmo}>Fill</button
                      >
                    {:else}
                      <span class="dim">–</span>
                    {/if}
                  </td>
                </tr>
              {/each}
            </tbody>
          </table>
        </section>
      {/if}
    </div>

    <div class="foot">
      {#if saveError}
        <p class="problem"><Icon name="warn" size={16} />{saveError}</p>
      {/if}
      {#if saved && !dirty}
        <div class="row">
          <span class="grow ok"
            ><Icon name="save" size={16} />Saved. Your card was backed up first: Undo puts it back.</span
          >
          <button class="ghost" onclick={undo} disabled={busy}>Undo</button>
          <button
            onclick={() => {
              onclose(true);
            }}
            disabled={busy}>Close</button
          >
        </div>
      {:else}
        <div class="row">
          <span class="grow dim">Your memory card is backed up before anything is written.</span>
          {#if dirty}
            <button class="ghost" onclick={reset} disabled={busy}>Reset</button>
          {/if}
          <button class="ghost" onclick={close} disabled={busy}>{dirty ? "Cancel" : "Close"}</button
          >
          <button
            class="primary"
            onclick={save}
            disabled={!canSave}
            title={playing ? "Close the game first" : "Save (Ctrl+S)"}
          >
            {#if busy}<span class="spinner"></span>{:else}<Icon name="save" size={16} />{/if}Save
          </button>
        </div>
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
    padding: 24px;
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

  .modal {
    position: relative;
    z-index: 1;
    width: 100%;
    max-width: 760px;
    max-height: calc(100vh - 48px);
    background: var(--card);
    --control: var(--control-card);
    --control-hi: var(--control-card-hi);
    border-radius: var(--radius);
    box-shadow: var(--shadow);
    display: flex;
    flex-direction: column;
    overflow: hidden;
  }

  .modal:focus {
    outline: none;
  }

  .head,
  .foot {
    padding: 16px 22px;
    background: var(--card-hi);
  }

  .head {
    display: flex;
    align-items: flex-start;
    gap: 12px;
    border-bottom: 1px solid var(--line);
  }

  .head h2 {
    margin: 0;
    overflow-wrap: anywhere;
  }

  .sub,
  .note {
    margin: 4px 0 0;
    font-size: 0.8125rem;
    color: var(--soft);
  }

  .body {
    padding: 8px 22px 18px;
    overflow: auto;
  }

  .foot {
    border-top: 1px solid var(--line);
  }

  .foot .row {
    flex-wrap: wrap;
    gap: 8px;
    font-size: 0.8125rem;
  }

  .line {
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .line label {
    min-width: 90px;
  }

  .field {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .bolt {
    color: var(--amber);
    display: inline-flex;
  }

  table {
    width: 100%;
    border-collapse: collapse;
    margin-top: 8px;
    font-size: 0.875rem;
  }

  th,
  td {
    text-align: left;
    padding: 6px 8px;
    border-bottom: 1px solid var(--line);
  }

  td input {
    width: 80px;
  }

  .dim {
    color: var(--soft);
  }

  .err-text {
    color: var(--err);
    font-size: 0.8125rem;
  }

  .problem {
    display: flex;
    gap: 8px;
    align-items: center;
    color: var(--err);
  }
</style>
