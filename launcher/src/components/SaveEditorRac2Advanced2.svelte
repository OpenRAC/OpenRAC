<script lang="ts">
  // The Going Commando save editor's advanced fields, a part of its Advanced section: where the
  // save resumes, platinum bolts spent, crystals and moonstones, weapon mods and the ship's parts.
  // It reads and saves on its own (read_save_rac2_extra, write_save_rac2_extra: refused while the
  // game runs with the card, the card backed up first), so it can sit in any part of the editor.
  // Each field was proven by a save the game wrote and loaded back (docs/RAC2_SAVE_FORMAT.md, "The
  // advanced fields").
  import Icon from "$components/Icon.svelte";
  import { api, type Rac2ExtraFields, type Rac2ExtraView } from "$lib/api";
  import { extraChanged, extraErrors, modCost, modGroups } from "$lib/saveEditRac2Extra";
  import { onMount } from "svelte";

  let {
    serial,
    slot,
    playing = false,
    onsaved,
  }: {
    serial: string;
    slot: number;
    /** The launcher is running the game, which writes the card too: saving waits until it is closed. */
    playing?: boolean;
    /** Called after a save, so the rest of the editor reads the slot again. */
    onsaved?: () => void;
  } = $props();

  let view = $state<Rac2ExtraView | null>(null);
  let fields = $state<Rac2ExtraFields | null>(null);
  let loadError = $state<string | null>(null);
  let saveError = $state<string | null>(null);
  let busy = $state(false);
  let savedNote = $state<string | null>(null);

  const errors = $derived(view && fields ? extraErrors(view, fields) : {});
  const dirty = $derived(!!view && !!fields && extraChanged(view, fields));
  const canSave = $derived(dirty && !busy && !playing && Object.keys(errors).length === 0);
  const groups = $derived(view ? modGroups(view) : []);
  const cost = $derived(view && fields ? modCost(view, fields) : 0);
  const levelName = (id: number) => view?.levels.find((l) => l.id === id)?.name ?? `Level ${String(id)}`;

  async function load() {
    loadError = null;
    try {
      const read = await api.readSaveRac2Extra(serial, slot);
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
      const result = await api.writeSaveRac2Extra(serial, slot, view.fingerprint, $state.snapshot(fields));
      view = result.view;
      fields = structuredClone(result.view.fields);
      savedNote = `Saved. Your card was backed up first (${result.backup.note}).`;
      onsaved?.();
    } catch (e) {
      saveError = String(e);
    } finally {
      busy = false;
    }
  }

  function reset() {
    if (view) fields = $state.snapshot(view.fields);
  }

  onMount(() => {
    void load();
  });
</script>

<div class="adv2">
  {#if loadError}
    <p class="problem"><Icon name="warn" size={16} />{loadError}</p>
  {:else if !view || !fields}
    <p class="dim">Reading the save…</p>
  {:else}
    <section aria-labelledby="se2x-where-h">
      <h4 id="se2x-where-h">Where the save starts</h4>
      <div class="line">
        <label for="se2x-level"
          >Planet{#if fields.level !== view.fields.level}<span class="mark" role="img" aria-label="changed"
            ></span>{/if}</label
        >
        <div class="field">
          <select id="se2x-level" bind:value={fields.level}>
            {#each view.levels as l (l.id)}
              <option value={l.id}>{l.name}</option>
            {/each}
          </select>
          {#if fields.level !== view.fields.level}
            <button class="ghost small" onclick={() => fields && view && (fields.level = view.fields.level)}
              >Back to {levelName(view.fields.level)}</button
            >
          {/if}
          <span class="hint"
            >Loading the save puts Ratchet at this planet's landing spot. The game keeps no position in the save, so he
            always starts there, wherever he saved.</span
          >
        </div>
      </div>
    </section>

    <section aria-labelledby="se2x-gems-h">
      <h4 id="se2x-gems-h">Platinum bolts, crystals and moonstones</h4>
      <div class="line">
        <label for="se2x-spent"
          >Platinum bolts spent{#if fields.platinumSpent !== view.fields.platinumSpent}<span
              class="mark"
              role="img"
              aria-label="changed"
            ></span>{/if}</label
        >
        <div class="field">
          <input
            id="se2x-spent"
            type="number"
            min="0"
            max={view.platinumFound}
            step="1"
            bind:value={fields.platinumSpent}
            aria-invalid={!!errors.platinumSpent}
          />
          <span class="hint"
            >Of {view.platinumFound} found. The game lets you spend the rest on weapon mods: {Math.max(
              0,
              view.platinumFound - fields.platinumSpent,
            )} left.</span
          >
          {#if errors.platinumSpent}<span class="err-text">{errors.platinumSpent}</span>{/if}
        </div>
      </div>
      {#each [["crystals", "Crystals", "Found on Tabora, where they sell for bolts."], ["moonstones", "Moonstones", "Found on Grelbin, where they sell for bolts."]] as const as [key, label, hint] (key)}
        <div class="line">
          <label for={`se2x-${key}`}
            >{label}{#if fields[key] !== view.fields[key]}<span class="mark" role="img" aria-label="changed"
              ></span>{/if}</label
          >
          <div class="field">
            <input
              id={`se2x-${key}`}
              type="number"
              min="0"
              max={view.maxGems}
              step="1"
              bind:value={fields[key]}
              aria-invalid={!!errors[key]}
            />
            <span class="hint">{hint}</span>
            {#if errors[key]}<span class="err-text">{errors[key]}</span>{/if}
          </div>
        </div>
      {/each}
    </section>

    <section aria-labelledby="se2x-mods-h">
      <h4 id="se2x-mods-h">
        Weapon mods <span class="count">{fields.mods.filter((m) => m.bought).length} of {fields.mods.length}</span>
      </h4>
      <ul class="mods">
        {#each groups as g (g.item)}
          <li>
            <span class="weapon" class:off={!g.owned}
              >{g.name}{#if !g.owned}<span class="dim not-owned">not in the inventory</span>{/if}</span
            >
            <span class="chips">
              {#each g.mods as k (k)}
                {@const m = fields.mods[k]}
                {@const info = view.mods[k]}
                {#if m && info}
                  <label class="chip" class:on={m.bought}>
                    <input type="checkbox" bind:checked={m.bought} />
                    {info.name}
                    <span class="price" title="Its price at the mod vendor">{info.price} PB</span>
                    {#if m.bought !== view.fields.mods[k]?.bought}<span class="mark" role="img" aria-label="changed"
                      ></span>{/if}
                  </label>
                {/if}
              {/each}
            </span>
          </li>
        {/each}
      </ul>
      <p class="note">
        A mod ticked here is bought and switched on. Ticking mods does not spend platinum bolts{#if cost !== 0}: these
          changes would cost {Math.abs(cost)} at the vendor{cost < 0 ? " back" : ""}; change "Platinum bolts spent"
          above to match if you like{/if}.
      </p>
    </section>

    <section aria-labelledby="se2x-ship-h">
      <h4 id="se2x-ship-h">Ship parts</h4>
      {#each view.ship as part, k (part.name)}
        <div class="line">
          <label for={`se2x-ship-${String(k)}`}
            >{part.name}{#if fields.ship[k] !== view.fields.ship[k]}<span class="mark" role="img" aria-label="changed"
              ></span>{/if}</label
          >
          <div class="field">
            <select id={`se2x-ship-${String(k)}`} bind:value={fields.ship[k]}>
              {#if view.fields.ship[k] === null}
                <option value={null}>Something the shop doesn't sell (kept)</option>
              {/if}
              {#each part.choices as c, i (c)}
                <option value={i}>{c}</option>
              {/each}
            </select>
          </div>
        </div>
      {/each}
      <p class="note">The parts the Ship Shack sells. The paint job is with Ratchet's fields above.</p>
    </section>

    <div class="actions">
      {#if saveError}
        <p class="problem"><Icon name="warn" size={16} />{saveError}</p>
      {/if}
      {#if playing && dirty}
        <p class="running"><Icon name="info" size={16} />The game is running with this card: close it, then save.</p>
      {/if}
      {#if savedNote && !dirty}
        <p class="ok"><Icon name="save" size={16} />{savedNote}</p>
      {/if}
      <div class="row">
        <span class="grow dim">{dirty ? "These fields save on their own." : "No changes here."}</span>
        <button class="ghost" onclick={reset} disabled={!dirty || busy}>Reset</button>
        <button class="primary" onclick={save} disabled={!canSave}>
          {#if busy}<span class="spinner"></span>{:else}<Icon name="save" size={16} />{/if}Save these
        </button>
      </div>
    </div>
  {/if}
</div>

<style>
  /* Reflows by its own width, as the editor's sections do. */
  .adv2 {
    container-type: inline-size;
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 14px;
  }

  h4 {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 10px;
    margin: 10px 0 6px;
    font-size: 0.9375rem;
  }

  .count {
    font-size: 0.8125rem;
    font-weight: 500;
    color: var(--soft);
  }

  .line {
    display: grid;
    grid-template-columns: minmax(0, 160px) minmax(0, 1fr);
    align-items: center;
    gap: 12px;
    padding: 9px 2px;
    border-top: 1px solid var(--line);
  }

  .line > label {
    min-width: 0;
    overflow-wrap: anywhere;
  }

  .field {
    display: flex;
    align-items: center;
    gap: 10px;
    flex-wrap: wrap;
    min-width: 0;
  }

  select,
  input[type="number"] {
    font: inherit;
    font-size: 0.875rem;
    background: var(--control);
    color: var(--text);
    border-radius: var(--radius-sm);
    padding: 6px 10px;
    max-width: 100%;
    min-width: 0;
  }

  input[type="number"] {
    width: 110px;
    font-variant-numeric: tabular-nums;
  }

  input[type="number"][aria-invalid="true"] {
    box-shadow: inset 0 0 0 2px var(--err);
  }

  input[type="checkbox"] {
    accent-color: var(--amber);
    margin: 0;
  }

  .hint {
    flex: 1 1 12rem;
    min-width: 0;
    font-size: 0.75rem;
    color: var(--dim);
  }

  .dim {
    color: var(--dim);
  }

  .err-text {
    font-size: 0.8125rem;
    color: var(--err);
  }

  ul {
    margin: 0;
    padding: 0;
    list-style: none;
  }

  /* One row a weapon: the name, then its mods; the columns line up through subgrid. */
  .mods {
    display: grid;
    grid-template-columns: minmax(0, 180px) minmax(0, 1fr);
    column-gap: 12px;
  }

  .mods li {
    display: grid;
    grid-column: 1 / -1;
    grid-template-columns: subgrid;
    align-items: center;
    padding: 7px 2px;
    border-top: 1px solid var(--line);
  }

  .weapon {
    min-width: 0;
    overflow-wrap: anywhere;
  }

  .weapon.off {
    color: var(--soft);
  }

  .weapon .not-owned {
    margin-left: 6px;
    font-size: 0.75rem;
  }

  .chips {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    min-width: 0;
  }

  .chip {
    position: relative;
    display: inline-flex;
    align-items: center;
    gap: 7px;
    font-size: 0.8125rem;
    border-radius: var(--radius-sm);
    padding: 5px 12px;
    cursor: pointer;
    color: var(--soft);
    background: var(--control);
  }

  .chip.on {
    color: var(--text);
    background: var(--on-fill);
  }

  .price {
    font-size: 0.6875rem;
    color: var(--dim);
    font-variant-numeric: tabular-nums;
  }

  .mark {
    display: inline-block;
    width: 7px;
    height: 7px;
    margin-left: 7px;
    border-radius: 50%;
    background: var(--amber);
    box-shadow: none;
    vertical-align: middle;
  }

  .note {
    margin: 8px 0 0;
    font-size: 0.75rem;
    color: var(--dim);
  }

  .actions {
    display: flex;
    flex-direction: column;
    gap: 8px;
    padding-top: 10px;
    border-top: 1px solid var(--line);
  }

  .row {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 10px;
  }

  .grow {
    flex: 1 1 10rem;
    min-width: 0;
    font-size: 0.8125rem;
  }

  .problem,
  .running,
  .ok {
    display: flex;
    align-items: flex-start;
    gap: 8px;
    margin: 0;
    font-size: 0.8125rem;
  }

  .problem {
    color: var(--err);
  }

  .running {
    color: var(--info);
  }

  .ok {
    color: var(--ok);
  }

  @container (max-width: 560px) {
    .line {
      grid-template-columns: minmax(0, 1fr);
      gap: 6px;
    }

    .mods {
      grid-template-columns: minmax(0, 1fr);
      row-gap: 6px;
    }
  }
</style>
