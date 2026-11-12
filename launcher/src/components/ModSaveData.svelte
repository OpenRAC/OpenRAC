<script lang="ts">
  // What the mods that are on keep beside a save, a part of the Going Commando save editor: each
  // mod whose manifest describes its save data (`save_data`, core/src/mod_save_data.rs) gets its
  // fields of the record of the save opened. They are the mod's own file, in its data folder, not
  // the card: each mod saves on its own (write_mod_save_data_rac2, refused while the game runs).
  import Icon from "$components/Icon.svelte";
  import { api, type ModSaveData } from "$lib/api";
  import { onMount } from "svelte";

  let {
    serial,
    slot,
    playing = false,
  }: {
    serial: string;
    slot: number;
    /** The launcher is running the game: the mod keeps its record in memory, so saving waits. */
    playing?: boolean;
  } = $props();

  let mods = $state<ModSaveData[] | null>(null);
  /** The values being edited, per mod id and field key. */
  let values = $state<Record<string, Record<string, number>>>({});
  let loadError = $state<string | null>(null);
  let saveError = $state<Record<string, string>>({});
  let savedNote = $state<Record<string, string>>({});
  let busy = $state<string | null>(null);

  function get(id: string, key: string): number {
    return values[id]?.[key] ?? 0;
  }

  function set(id: string, key: string, value: number) {
    values[id] = { ...(values[id] ?? {}), [key]: value };
  }

  function take(read: ModSaveData) {
    values[read.id] = Object.fromEntries(read.fields.map((f) => [f.key, f.value]));
  }

  async function load() {
    loadError = null;
    try {
      const read = await api.readModSaveDataRac2(serial, slot);
      mods = read;
      for (const m of read) take(m);
    } catch (e) {
      loadError = String(e);
    }
  }

  function changedKeys(m: ModSaveData): string[] {
    return m.fields.filter((f) => get(m.id, f.key) !== f.value).map((f) => f.key);
  }

  async function save(m: ModSaveData) {
    const keys = changedKeys(m);
    if (keys.length === 0 || busy || playing) return;
    busy = m.id;
    saveError[m.id] = "";
    try {
      const read = await api.writeModSaveDataRac2(
        serial,
        slot,
        m.id,
        Object.fromEntries(keys.map((k) => [k, get(m.id, k)])),
      );
      mods = (mods ?? []).map((x) => (x.id === read.id ? read : x));
      take(read);
      savedNote[m.id] = `Saved into ${m.name}'s own file.`;
    } catch (e) {
      saveError[m.id] = String(e);
    } finally {
      busy = null;
    }
  }

  function reset(m: ModSaveData) {
    take(m);
  }

  onMount(() => {
    void load();
  });
</script>

{#if loadError}
  <p class="problem"><Icon name="warn" size={16} />{loadError}</p>
{:else if mods && mods.length > 0}
  {#each mods as m (m.id)}
    {@const dirty = changedKeys(m).length > 0}
    <section class="mod" aria-labelledby={`msd-${m.id}-h`}>
      <h3 id={`msd-${m.id}-h`}>{m.title || m.name} <span class="from">{m.name}</span></h3>
      {#if !m.found}
        <p class="note">{m.name} has no record of this save yet: saving here adds one.</p>
      {/if}
      <ul class="rows">
        {#each m.fields as f (f.key)}
          <li class="line">
            <label for={`msd-${m.id}-${f.key}`}
              >{f.name}{#if get(m.id, f.key) !== f.value}<span class="mark" role="img" aria-label="changed"
                ></span>{/if}</label
            >
            {#if f.choices && f.choices.length > f.max - f.min}
              <select
                id={`msd-${m.id}-${f.key}`}
                bind:value={
                  () => get(m.id, f.key),
                  (x: number) => {
                    set(m.id, f.key, x);
                  }
                }
              >
                {#each f.choices as c, i (i)}
                  {#if i >= f.min && i <= f.max}<option value={i}>{c}</option>{/if}
                {/each}
              </select>
            {:else}
              <input
                id={`msd-${m.id}-${f.key}`}
                type="number"
                min={f.min}
                max={f.max}
                step="1"
                bind:value={
                  () => get(m.id, f.key),
                  (x: number) => {
                    set(m.id, f.key, x);
                  }
                }
              />
            {/if}
          </li>
        {/each}
      </ul>
      <div class="actions">
        {#if saveError[m.id]}
          <p class="problem"><Icon name="warn" size={16} />{saveError[m.id]}</p>
        {/if}
        {#if playing && dirty}
          <p class="running"><Icon name="info" size={16} />The game is running: close it, then save.</p>
        {/if}
        {#if savedNote[m.id] && !dirty}
          <p class="ok"><Icon name="save" size={16} />{savedNote[m.id]}</p>
        {/if}
        <div class="row">
          <span class="grow dim">{dirty ? `These save into ${m.name}'s file, not the card.` : "No changes here."}</span>
          <button
            class="ghost"
            onclick={() => {
              reset(m);
            }}
            disabled={!dirty || busy !== null}>Reset</button
          >
          <button class="primary" onclick={() => save(m)} disabled={!dirty || busy !== null || playing}>
            {#if busy === m.id}<span class="spinner"></span>{:else}<Icon name="save" size={16} />{/if}Save these
          </button>
        </div>
      </div>
    </section>
  {/each}
{/if}

<style>
  .mod {
    container-type: inline-size;
    min-width: 0;
  }

  .from {
    margin-left: 8px;
    font-size: 0.8125rem;
    font-weight: 500;
    color: var(--soft);
  }

  ul {
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .line {
    display: grid;
    grid-template-columns: minmax(0, 200px) minmax(0, 1fr);
    align-items: center;
    gap: 12px;
    padding: 7px 2px;
    border-top: 1px solid var(--line);
  }

  .line > label {
    min-width: 0;
    overflow-wrap: anywhere;
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
    justify-self: start;
  }

  input[type="number"] {
    width: 110px;
    font-variant-numeric: tabular-nums;
  }

  .mark {
    display: inline-block;
    width: 7px;
    height: 7px;
    margin-left: 7px;
    border-radius: 50%;
    background: var(--amber);
    vertical-align: middle;
  }

  .note {
    margin: 8px 0 0;
    font-size: 0.75rem;
    color: var(--dim);
  }

  .dim {
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
  }
</style>
