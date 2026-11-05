<script lang="ts">
  // My mods' toolbar, once there are many (lib/modsView.svelte.ts): a search, the kind to show
  // (all, gameplay, graphics and sound, tools; each with its count) and the order (load order,
  // which can be changed by dragging or a row's menu, or A to Z, for looking only).
  import { TOOLBAR_FROM, type ModFilter } from "$lib/modsList";
  import { VIEW } from "$lib/modsText";
  import { modsView, total } from "$lib/modsView.svelte";
  import Icon from "./Icon.svelte";

  const FILTERS: { id: ModFilter; label: string }[] = [
    { id: "all", label: VIEW.all },
    { id: "gameplay", label: VIEW.gameplay },
    { id: "looks", label: VIEW.looks },
    { id: "tools", label: VIEW.tools },
  ];
  /** The kinds the player has: one with none is left out (All always shows). */
  const shown = $derived(FILTERS.filter((f) => f.id === "all" || total(f.id) > 0));
  const visible = $derived(total() >= TOOLBAR_FROM);

  // A kind that is no longer there (its last mod removed): back to All.
  $effect(() => {
    if (!shown.some((f) => f.id === modsView.filter)) modsView.filter = "all";
  });
</script>

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
{/if}

<style>
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
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--panel);
    color: var(--soft);
  }

  .find:focus-within {
    border-color: var(--amber);
  }

  .find input {
    flex: 1;
    min-width: 0;
    border: 0;
    background: none;
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
    padding: 4px;
    color: var(--soft);
  }

  .kinds {
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
    padding: 3px;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--panel);
  }

  .kinds button {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    border: 0;
    background: none;
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
    background: var(--panel-hi);
    color: var(--amber);
    box-shadow: inset 0 0 0 1px var(--line-strong);
  }

  .kinds button.on b {
    color: var(--amber);
  }

  .kinds button:focus-visible {
    outline: 2px solid var(--amber);
    outline-offset: 2px;
  }
</style>
