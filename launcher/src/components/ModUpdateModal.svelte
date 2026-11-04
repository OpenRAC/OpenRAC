<script lang="ts">
  // The launcher's day-old look for newer mods found some (lib/modUpdates): what they are, and one
  // button that updates them all. "Later" leaves them for the Mods tab, which shows the same
  // answers; "Don't ask again" turns the look off (checkModUpdates) and says the Mods tab still
  // looks when asked. Nothing is installed without the player's click.
  import { api, type ModUpdate } from "$lib/api";
  import { app, saveConfig } from "$lib/app.svelte";
  import { dismissModUpdates, forgetModUpdates, modUpdates } from "$lib/modUpdates.svelte";
  import { START_UPDATES as T } from "$lib/modsText";
  import { modal } from "$lib/portal";
  import Icon from "./Icon.svelte";

  let working = $state(false);
  let error = $state<string | null>(null);

  const offer = $derived(modUpdates.offer);
  /** The launcher's own update window comes first: one modal at a time. */
  const shown = $derived(!!offer && !app.showUpdateModal && !!app.config?.setupComplete);

  const label = (u: ModUpdate) => `${u.id} ${u.installed} → ${u.available}`;

  function close() {
    if (working) return;
    dismissModUpdates();
  }

  async function never() {
    if (working || !app.config) return;
    dismissModUpdates();
    await saveConfig({ ...app.config, checkModUpdates: false });
  }

  async function updateAll() {
    if (working || !offer) return;
    working = true;
    error = null;
    try {
      for (const u of offer.updates) {
        await api.pluginsUpdate(offer.game, offer.version, u.id);
      }
      forgetModUpdates(offer.game);
    } catch (e) {
      error = e instanceof Error ? e.message : String(e);
    }
    working = false;
  }

  function onkeydown(e: KeyboardEvent) {
    if (e.key === "Escape") close();
  }
</script>

<svelte:window {onkeydown} />

{#if shown && offer}
  <div class="overlay" role="presentation" use:modal>
    <button class="backdrop" onclick={close} tabindex="-1" aria-label={T.later}></button>
    <div class="modal rise" role="dialog" aria-modal="true" aria-labelledby="mod-updates-title" tabindex="-1">
      <div class="header">
        <div class="title-row">
          <Icon name="refresh" size={20} />
          <h2 id="mod-updates-title">{T.title}</h2>
        </div>
      </div>

      <div class="body">
        <p class="lead">{T.lead(offer.title, offer.updates.length)}</p>
        <ul class="mods">
          {#each offer.updates as update (update.id)}
            <li>{label(update)}</li>
          {/each}
        </ul>
        <p class="kept">{T.kept}</p>
        {#if error}
          <p class="error" role="alert"><Icon name="warn" size={16} /><span>{T.failed(error)}</span></p>
        {/if}
      </div>

      <div class="actions">
        <button class="ghost small grow" disabled={working} onclick={() => void never()} title={T.neverHint}
          >{T.never}</button
        >
        <button class="ghost" disabled={working} onclick={close}>{T.later}</button>
        <button class="primary" disabled={working} onclick={() => void updateAll()}>
          {#if working}<span class="spinner"></span>{T.working}{:else}{T.all(offer.updates.length)}{/if}
        </button>
      </div>
    </div>
  </div>
{/if}

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
    max-width: 480px;
    background: var(--panel);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius);
    box-shadow: var(--shadow);
    display: flex;
    flex-direction: column;
    overflow: hidden;
  }

  .modal:focus {
    outline: none;
  }

  .header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 18px 24px;
    border-bottom: 1px solid var(--line);
    background: var(--panel-hi);
  }

  .title-row {
    display: flex;
    align-items: center;
    gap: 10px;
    color: var(--amber);
  }

  .title-row h2 {
    margin: 0;
    font-size: 1.125rem;
    font-family: var(--font-head);
    font-weight: 600;
    color: var(--text);
  }

  .body {
    padding: 20px 24px;
    display: flex;
    flex-direction: column;
    gap: 12px;
    max-height: 360px;
    overflow-y: auto;
  }

  .lead {
    margin: 0;
    color: var(--body);
  }

  .mods {
    margin: 0;
    padding: 12px 14px;
    list-style: none;
    background: var(--ink-deep);
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
    display: flex;
    flex-direction: column;
    gap: 4px;
    font-family: var(--font-mono);
    font-size: 0.8125rem;
    color: var(--body);
  }

  .kept {
    margin: 0;
    font-size: 0.8125rem;
    color: var(--soft);
  }

  .error {
    display: flex;
    align-items: center;
    gap: 8px;
    margin: 0;
    padding: 10px 14px;
    background: color-mix(in srgb, var(--err) 12%, transparent);
    border: 1px solid var(--err);
    border-radius: var(--radius-sm);
    color: var(--err);
    font-size: 0.8125rem;
  }

  .actions {
    display: flex;
    align-items: center;
    justify-content: flex-end;
    gap: 12px;
    padding: 16px 24px;
    border-top: 1px solid var(--line);
    background: var(--panel-hi);
  }

  .grow {
    flex: 1;
    text-align: left;
  }
</style>
