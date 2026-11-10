<script lang="ts">
  // My mods' head (lib/modsView.svelte.ts). With the Mods switch under Play off and something on,
  // a line says Play starts without them (gameplay mods, Enhanced graphics and every pack alike),
  // with the way to turn it on. Once there are many: a search, the kind to show (all, gameplay,
  // graphics and sound, tools; each with its count) and the order (load order, which can be changed
  // by dragging or a row's menu, or A to Z, for looking only). A search that finds nothing says so
  // here, once, as the lists hide.
  import { app, saveConfig } from "$lib/app.svelte";
  import { TOOLBAR_FROM, type ModFilter } from "$lib/modsList";
  import { VIEW, WITH_MODS } from "$lib/modsText";
  import { modsView, searching, total, totalFound, totalOn } from "$lib/modsView.svelte";
  import Icon from "./Icon.svelte";

  const FILTERS: { id: ModFilter; label: string }[] = [
    { id: "all", label: VIEW.all },
    { id: "gameplay", label: VIEW.gameplay },
    { id: "looks", label: VIEW.looks },
    { id: "tools", label: VIEW.tools },
  ];
  /** The kinds the player has: one with none is left out (All always shows). */
  const shown = $derived(FILTERS.filter((f) => f.id === "all" || total(f.id) > 0));
  /** Shown from that many rows, and while it hides any: removing mods during a search never
   *  leaves rows hidden with no way to show them. */
  const visible = $derived(
    total() >= TOOLBAR_FROM || searching() || modsView.filter !== "all" || modsView.sort !== "order",
  );
  /** The Mods switch under Play is off (the default): Play starts the game without these. */
  const purist = $derived(!(app.config?.playWithMods ?? false));

  async function playWithMods() {
    if (app.config) await saveConfig({ ...app.config, playWithMods: true });
  }

  // A kind that is no longer there (its last mod removed): back to All. Not while the lists are
  // still being read (none has said what it holds yet).
  $effect(() => {
    if (total() > 0 && !shown.some((f) => f.id === modsView.filter)) modsView.filter = "all";
  });
</script>

{#if purist && totalOn() > 0}
  <div class="notice" role="status">
    <Icon name="info" size={16} /><span class="grow">{WITH_MODS.offNotice}</span>
    <button class="small" onclick={() => void playWithMods()}>{WITH_MODS.turnOn}</button>
  </div>
{/if}
{#if visible}
  <div class="toolbar" role="search" aria-label={VIEW.label}>
    <label class="find">
      <Icon name="search" size={16} />
      <input
        type="search"
        aria-label={VIEW.search}
        placeholder={VIEW.searchPlaceholder}
        bind:value={modsView.query}
        onkeydown={(e) => {
          if (e.key === "Escape" && modsView.query) {
            e.stopPropagation();
            modsView.query = "";
          }
        }}
      />
      {#if modsView.query}
        <button class="clear" aria-label={VIEW.clear} onclick={() => (modsView.query = "")}
          ><Icon name="x" size={14} /></button
        >
      {/if}
    </label>
    <div class="kinds" role="radiogroup" aria-label={VIEW.label}>
      {#each shown as f (f.id)}
        <button
          role="radio"
          aria-checked={modsView.filter === f.id}
          class:on={modsView.filter === f.id}
          onclick={() => (modsView.filter = f.id)}>{f.label}<b>{total(f.id)}</b></button
        >
      {/each}
    </div>
    <div class="kinds" role="radiogroup" aria-label={VIEW.sortLabel}>
      {#each [{ id: "order", label: VIEW.sortOrder }, { id: "name", label: VIEW.sortName }] as const as o (o.id)}
        <button
          role="radio"
          aria-checked={modsView.sort === o.id}
          class:on={modsView.sort === o.id}
          onclick={() => (modsView.sort = o.id)}>{o.label}</button
        >
      {/each}
    </div>
  </div>
  {#if searching() && totalFound() === 0}<p class="none" role="status">{VIEW.none(modsView.query.trim())}</p>{/if}
{/if}

<style>
  /* The Mods switch off: said above the lists, with the way to turn it on. */
  .notice {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 8px 10px 8px 12px;
    border-radius: var(--radius-sm);
    background: var(--card-hi);
    --control: var(--control-card);
    --control-hi: var(--control-card-hi);
    font-size: 0.8125rem;
    color: var(--body);
  }

  .notice > :global(svg) {
    flex: none;
    color: var(--amber);
  }

  .none {
    margin: 0;
    font-size: 0.875rem;
    color: var(--soft);
  }

  .toolbar {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 10px 14px;
  }

  .find {
    flex: 1 1 240px;
    display: flex;
    align-items: center;
    gap: 8px;
    min-width: 0;
    padding: 0 10px;
    border-radius: var(--radius-sm);
    background: var(--control);
    color: var(--soft);
  }

  /* The whole bar is the field: the ring and the hover take all of it, icon too, not only the
     input inside. */
  .find:focus-within {
    background: var(--on-fill);
    color: var(--amber);
    box-shadow: inset 0 0 0 2px var(--amber);
  }

  @media (hover: hover) {
    :global(html:where(:not([data-input="pad"]))) .find:hover:not(:focus-within) {
      background: var(--control-hi);
    }

    :global(html:where(:not([data-input="pad"]))) .find input:hover:not(:disabled) {
      background: none;
    }
  }

  .find input:focus,
  .find input:focus-visible {
    box-shadow: none;
  }

  .find input {
    flex: 1;
    min-width: 0;
    border: 0;
    background: none;
    box-shadow: none;
    padding: 9px 0;
    color: var(--text);
    outline: none;
  }

  .find input::-webkit-search-cancel-button {
    display: none;
  }

  .clear {
    border: 0;
    background: none;
    box-shadow: none;
    padding: 4px;
    color: var(--soft);
  }

  .kinds {
    display: flex;
    flex-wrap: wrap;
    /* Filter chips, each on its own: no strip joining them. */
    gap: 6px;
  }

  .kinds button {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    border: 0;
    background: var(--control);
    padding: 6px 12px;
    font-size: 0.8125rem;
    font-weight: 600;
    color: var(--soft);
  }

  .kinds button b {
    font-weight: 600;
    color: var(--dim);
    font-variant-numeric: tabular-nums;
  }

  .kinds button.on {
    background: var(--on-fill);
    color: var(--amber);
  }

  .kinds button.on b {
    color: var(--amber);
  }

  .kinds button:focus-visible {
    outline: 2px solid var(--amber);
    outline-offset: 2px;
  }
</style>
