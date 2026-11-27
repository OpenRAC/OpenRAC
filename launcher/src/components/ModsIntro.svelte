<script lang="ts">
  // "Before you install mods": shown once, the first time the Mods tab opens. What the four labels
  // mean, the risk of a mod that runs code, and that OpenRAC takes no responsibility. "Show only
  // checked mods" turns on the "Checked by OpenRAC only" filter (ModsToolbar.svelte) as it closes.
  import { api } from "$lib/api";
  import { app, guardShown, saveConfig } from "$lib/app.svelte";
  import { MODS_INTRO as T, READ_MORE_URL, TRUST_HINT, TRUST_LABEL } from "$lib/modsText";
  import { modal } from "$lib/portal";

  import Icon from "./Icon.svelte";
  import TrustLabel from "./TrustLabel.svelte";

  const shown = $derived(!!app.config?.setupComplete && !app.config.modsIntroSeen);
  const keys = Object.keys(TRUST_LABEL) as (keyof typeof TRUST_LABEL)[];

  async function close(checkedOnly: boolean) {
    if (!app.config) return;
    await saveConfig({
      ...app.config,
      modsIntroSeen: true,
      checkedOnly: checkedOnly || app.config.checkedOnly,
    });
  }
</script>

{#if shown}
  <div class="overlay" role="presentation" use:modal>
    <div class="backdrop"></div>
    <div class="modal rise" role="dialog" aria-modal="true" aria-labelledby="mods-intro-title">
      <div class="header">
        <Icon name="library" size={20} />
        <h2 id="mods-intro-title">{T.title}</h2>
      </div>
      <div class="body">
        <p class="lead">{T.lead}</p>
        <ul class="tiers">
          {#each keys as k (k)}
            <li><TrustLabel trust={k} big /><span>{TRUST_HINT[k]}</span></li>
          {/each}
        </ul>
        <p class="fine"><b>{T.riskLead}</b> {T.risk}</p>
        <p class="fine"><b>{T.noResponsibility}</b> {T.responsibility}</p>
      </div>
      <div class="actions">
        <button class="ghost grow" onclick={() => void guardShown(api.openUrl(READ_MORE_URL))}
          >{T.readMore}</button
        >
        <button onclick={() => void close(true)}>{T.checkedOnly}</button>
        <button class="primary" onclick={() => void close(false)}>{T.understand}</button>
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
    background: color-mix(in srgb, var(--ink-deep) 75%, transparent);
    backdrop-filter: blur(4px);
  }

  .modal {
    position: relative;
    z-index: 1;
    display: flex;
    flex-direction: column;
    width: 100%;
    max-width: 560px;
    max-height: 100%;
    overflow: hidden;
    border-radius: var(--radius);
    background: var(--card);
    box-shadow: var(--shadow);
    --control: var(--control-card);
    --control-hi: var(--control-card-hi);
  }

  .header {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 18px 24px;
    border-bottom: 1px solid var(--line);
    background: var(--card-hi);
    color: var(--amber);
  }

  h2 {
    margin: 0;
    color: var(--text);
    font-family: var(--font-head);
    font-size: 1.125rem;
    font-weight: 600;
  }

  .body {
    display: flex;
    flex-direction: column;
    gap: 14px;
    padding: 20px 24px;
    overflow-y: auto;
  }

  .lead {
    margin: 0;
    color: var(--body);
    font-size: 0.875rem;
  }

  .tiers {
    display: flex;
    flex-direction: column;
    gap: 8px;
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .tiers li {
    display: grid;
    grid-template-columns: 150px 1fr;
    gap: 14px;
    align-items: center;
    padding: 10px 14px;
    border-radius: var(--radius-sm);
    background: var(--card-hi);
    color: var(--body);
    font-size: 0.8125rem;
    line-height: 1.45;
  }

  .tiers li > :global(.trust) {
    justify-self: start;
    margin-left: -4px;
  }

  .fine {
    margin: 0;
    color: var(--soft);
    font-size: 0.8125rem;
    line-height: 1.45;
  }

  .fine b {
    color: var(--text);
  }

  .actions {
    display: flex;
    align-items: center;
    justify-content: flex-end;
    gap: 12px;
    padding: 16px 24px;
    border-top: 1px solid var(--line);
    background: var(--card-hi);
  }

  .grow {
    margin-right: auto;
  }

  @media (max-width: 520px) {
    .tiers li {
      grid-template-columns: 1fr;
      gap: 6px;
    }

    .actions {
      flex-wrap: wrap;
    }
  }
</style>
