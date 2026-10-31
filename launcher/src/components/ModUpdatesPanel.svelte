<script lang="ts">
  // The updates of a game's mods from the index (lib/modAutoUpdate, core/src/mod_updates.rs): the
  // ones that wait for a click ("Update available": a major, or new hooks), with their changelog;
  // the ones held back, and why ("held back: A needs B 1.2.0"); the ones that just went in by
  // themselves, with what changed. Shown only when it has something to say. Changelogs are text.
  import { api, errorText, type UpdateItem, type VersionView } from "$lib/api";
  import { autoUpdates, gameRunning, updateVersion } from "$lib/modAutoUpdate.svelte";
  import { UPDATES as T, holdText, updateFrom } from "$lib/modsText";
  import { onMount } from "svelte";
  import Icon from "./Icon.svelte";

  let { version, onchange }: { version: VersionView; onchange?: () => void } = $props();

  let busy = $state<string | null>(null);
  let error = $state<string | null>(null);

  const plan = $derived(autoUpdates.plans[version.game] ?? null);
  const recent = $derived(autoUpdates.recent[version.game] ?? []);
  const shown = $derived((!!plan && (plan.waiting.length > 0 || plan.held.length > 0)) || recent.length > 0);

  onMount(() => {
    // The look at start may not have run for this game yet: what waits, without updating.
    if (!autoUpdates.plans[version.game]) {
      void api
        .modsUpdates(version.game, version.name)
        .then((p) => {
          autoUpdates.plans[version.game] = p;
        })
        .catch(() => undefined);
    }
  });

  async function update(item: UpdateItem) {
    if (gameRunning()) {
      error = T.running;
      return;
    }
    busy = item.id;
    error = null;
    try {
      await updateVersion(version, [item.id]);
      onchange?.();
    } catch (e) {
      error = errorText(e);
    }
    busy = null;
  }
</script>

{#if shown && plan}
  <section class="panel updates" aria-label={T.title}>
    {#if plan.waiting.length > 0}
      <h2>{T.waitingTitle}</h2>
      <ul>
        {#each plan.waiting as item (item.id)}
          <li>
            <div class="grow">
              <strong>{item.name}</strong> <span class="dim">{updateFrom(item.from, item.to)}</span>
              <p class="why">{holdText(item.hold)}</p>
              <details class="more">
                <summary>{T.changelog}</summary>
                <pre>{item.changelog || T.noChangelog}</pre>
              </details>
            </div>
            <button class="small" disabled={busy !== null} onclick={() => void update(item)}>
              {#if busy === item.id}<span class="spinner"></span>{T.updating}{:else}<Icon
                  name="refresh"
                  size={14}
                />{T.update}{/if}
            </button>
          </li>
        {/each}
      </ul>
    {/if}
    {#if plan.held.length > 0}
      <h2>{T.heldTitle}</h2>
      <ul>
        {#each plan.held as item (item.id)}
          <li>
            <div class="grow">
              <strong>{item.name}</strong> <span class="dim">{updateFrom(item.from, item.to)}</span>
              <p class="why">{holdText(item.hold)}</p>
            </div>
          </li>
        {/each}
      </ul>
    {/if}
    {#if recent.length > 0}
      <h2>{T.recentTitle}</h2>
      <ul>
        {#each recent as item (item.id)}
          <li>
            <div class="grow">
              <strong>{item.name}</strong> <span class="dim">{updateFrom(item.from, item.to)}</span>
              <details class="more">
                <summary>{T.changelog}</summary>
                <pre>{item.changelog || T.noChangelog}</pre>
              </details>
            </div>
          </li>
        {/each}
      </ul>
    {/if}
    {#if error}<p class="err" role="alert">{error}</p>{/if}
  </section>
{/if}

<style>
  .updates {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  h2 {
    font-size: 0.875rem;
    margin: 0;
  }

  ul {
    margin: 0;
    padding: 0;
    list-style: none;
  }

  li {
    display: flex;
    align-items: flex-start;
    gap: 12px;
    padding: 8px 0;
    border-bottom: 1px solid var(--line);
  }

  li:last-child {
    border-bottom: 0;
  }

  .grow {
    flex: 1;
    min-width: 0;
  }

  .why {
    margin: 2px 0 0;
    font-size: 0.8125rem;
    color: var(--soft);
  }

  pre {
    white-space: pre-wrap;
    font-family: var(--font-sans);
    font-size: 0.8125rem;
    color: var(--body);
    margin: 6px 0 0;
  }

  .err {
    margin: 0;
    color: var(--err);
    font-size: 0.8125rem;
  }
</style>
