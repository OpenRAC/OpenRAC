<script lang="ts">
  // Developers only: which of a game's discs the page plays, when the launcher knows more than one
  // of them, in the games folder or set up before (core/src/disc_choice.rs). Play, the levels, saves and logs follow the choice; each
  // disc keeps its own set-up, levels and build. A player never sees this, nor any region.
  import type { DiscChoice, GameView } from "$lib/api";
  import { chooseDisc } from "$lib/app.svelte";
  import { regionLabel } from "$lib/labels";

  let { game, choice, busy }: { game: GameView; choice: DiscChoice; busy: boolean } = $props();

  let switching = $state(false);
  const versions = $derived(game.versions.filter((v) => choice.versions.includes(v.key)));

  async function pick(key: string) {
    if (key === choice.chosen || switching) return;
    switching = true;
    await chooseDisc(key);
    switching = false;
  }
</script>

<div class="switch" role="radiogroup" aria-label="Disc to play">
  <span class="what">Disc</span>
  {#each versions as v (v.key)}
    <button
      role="radio"
      aria-checked={v.key === choice.chosen}
      class:on={v.key === choice.chosen}
      disabled={busy || switching}
      title={busy ? "Wait for the game or the job to finish" : `${v.title} · ${v.serial}`}
      onclick={() => void pick(v.key)}>{regionLabel(v.name)}</button
    >
  {/each}
</div>

<style>
  .switch {
    display: inline-flex;
    align-items: center;
    gap: 2px;
    padding: 3px;
    border-radius: var(--radius-sm);
    background: rgb(0 0 0 / 0.35);
  }

  .what {
    padding: 0 8px 0 10px;
    font-size: 0.75rem;
    color: var(--tx2);
  }

  button {
    border: 0;
    background: none;
    box-shadow: none;
    font-size: 0.8125rem;
    color: var(--tx2);
    padding: 5px 14px;
    border-radius: var(--radius-sm);
  }

  @media (hover: hover) {
    :global(html:where(:not([data-input="pad"]))) button:hover:not(:disabled) {
      color: var(--on-game);
    }
  }

  button.on {
    background: var(--bd);
    color: var(--on-game);
  }
</style>
