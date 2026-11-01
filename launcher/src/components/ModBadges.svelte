<script lang="ts">
  // The tags a mod's card wears on openrac.dev/mods: what it changes, its discs, and AI help.
  import type { IndexMod } from "$lib/api";
  import { regions } from "$lib/modBrowse";
  import { BROWSE as T, IMPACT_LABEL } from "$lib/modsText";

  let { mod, game }: { mod: IndexMod; game: string } = $props();
</script>

<span class="badges">
  {#each mod.impact as impact (impact)}<span class="badge strong">{IMPACT_LABEL[impact] ?? impact}</span>{/each}
  {#each regions(mod, game) as region (region)}<span class="badge">{region}</span>{/each}
  {#if mod.aiAssisted}<span class="badge">{T.aiAssisted}</span>{/if}
</span>

<style>
  .badges {
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
    margin-top: 4px;
  }

  .badge {
    font-size: 0.6875rem;
    font-weight: 600;
    padding: 1px 7px;
    border-radius: 999px;
    white-space: nowrap;
    color: var(--dim);
    border: 1px solid var(--line);
  }

  .badge.strong {
    color: var(--text);
  }
</style>
