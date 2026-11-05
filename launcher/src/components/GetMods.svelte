<script lang="ts">
  // My mods' first line: where mods come from. The launcher lists no mods itself: the player finds
  // them on the official hub (openrac.dev, whose Install opens the launcher) or anywhere else, and
  // adds a link, a .zip or a folder in Add a mod below.
  import { api } from "$lib/api";
  import { guardShown } from "$lib/app.svelte";
  import { GET_MODS as T } from "$lib/modsText";
  import Icon from "./Icon.svelte";

  /** Whether the hub has mods for this game (Ratchet & Clank's for now). */
  let { hub = true }: { hub?: boolean } = $props();
</script>

<section class="get" aria-label={T.title}>
  <span class="mark"><Icon name="globe" size={22} /></span>
  <div class="grow">
    <h2>{T.title}</h2>
    <p>{hub ? T.lead : T.leadNoHub}</p>
  </div>
  {#if hub}<div class="buttons">
      <button onclick={() => void guardShown(api.openUrl(T.hubUrl))}><Icon name="external" size={16} />{T.hub}</button>
      <button class="ghost small" onclick={() => void guardShown(api.openUrl(T.publishUrl))}
        ><Icon name="external" size={14} />{T.publish}</button
      >
    </div>{/if}
</section>

<style>
  .get {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 12px 16px;
    padding: 16px 18px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius);
    background: linear-gradient(120deg, color-mix(in srgb, var(--amber) 14%, var(--panel)), var(--panel) 70%);
  }

  .mark {
    flex: none;
    display: inline-flex;
    padding: 10px;
    border-radius: 999px;
    background: color-mix(in srgb, var(--amber) 18%, transparent);
    color: var(--amber);
  }

  .grow {
    flex: 1 1 260px;
    min-width: 0;
  }

  h2 {
    margin: 0 0 2px;
  }

  p {
    margin: 0;
    font-size: 0.875rem;
    color: var(--body);
  }

  .buttons {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 8px;
  }
</style>
