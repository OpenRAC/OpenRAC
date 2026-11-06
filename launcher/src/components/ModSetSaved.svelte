<script lang="ts">
  // The mods as they were before each code: a folded line at the foot of the gameplay mods, there
  // only once a code was applied. Restoring asks first.
  import { api, ask, errorText, type ModProfile, type VersionView } from "$lib/api";
  import { toast } from "$lib/app.svelte";
  import { dateText } from "$lib/modCodes";
  import { modCode, modsChanged } from "$lib/modCodes.svelte";
  import { MOD_CODES as T } from "$lib/modsText";

  let { version: v }: { version: VersionView } = $props();

  let saved = $state<ModProfile[]>([]);
  let busy = $state(false);

  $effect(() => {
    // Read again whenever the mods changed; an answer overtaken by that is dropped.
    const asked = modCode.revision;
    const game = v.game;
    void api
      .modsetProfiles(game)
      .then((list) => {
        if (v.game === game && asked === modCode.revision) saved = list;
      })
      .catch(() => (saved = []));
  });

  async function restore(p: ModProfile) {
    if (!(await ask(T.restoreConfirm, T.restore))) return;
    busy = true;
    try {
      await api.modsetRestore(v.game, p.id);
      modsChanged();
      toast(T.undone);
    } catch (e) {
      toast(errorText(e), "error");
    }
    busy = false;
  }
</script>

{#if saved.length}
  <details class="saved">
    <summary>{T.savedTitle} ({saved.length})</summary>
    <p>{T.savedHint}</p>
    <ul>
      {#each saved as p (p.id)}
        <li>
          <span>{T.savedRow(p.code, dateText(p.created))}</span>
          <button class="small" disabled={busy} onclick={() => void restore(p)}>{T.restore}</button>
        </li>
      {/each}
    </ul>
  </details>
{/if}

<style>
  .saved {
    margin-top: 10px;
    font-size: 0.8125rem;
    color: var(--soft);
  }

  summary {
    cursor: pointer;
    width: fit-content;
    min-height: 28px;
    line-height: 28px;
  }

  p {
    margin: 2px 0 8px;
    color: var(--dim);
  }

  ul {
    margin: 0;
    padding: 0;
    list-style: none;
  }

  li {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    padding: 6px 0;
    border-top: 1px solid var(--line);
    color: var(--body);
  }
</style>
