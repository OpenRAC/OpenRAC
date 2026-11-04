<script lang="ts">
  // The change set (CKAN's word): what a link from openrac.dev, or Browse's Install, would change,
  // before anything is downloaded (lib/deepLink.svelte.ts, core/src/resolver.rs). The mods picked,
  // what they also install, recommended mods (ticked) and suggested ones (not), one warning when
  // native code comes with it, conflicts that block with their fix, the dev-build note; then each
  // mod's progress, and Play. Index text is shown as text only: no {@html} anywhere.
  import type { ModAction, PlannedMod } from "$lib/api";
  import { app, run, saveConfig } from "$lib/app.svelte";
  import { apply, changeSet, close, fix, toggle } from "$lib/deepLink.svelte";
  import {
    CHANGE_SET as T,
    STEP_LABEL,
    alsoCount,
    appliedText,
    conflictText,
    installCount,
    missingText,
    modErrorText,
    nativeWarning,
    neededBy,
    noteText,
    offeredBy,
    replaceWith,
    updateFrom,
    whyText,
  } from "$lib/modsText";
  import { modal } from "$lib/portal";
  import Icon from "./Icon.svelte";

  const shown = $derived(changeSet.link !== null && !!app.config?.setupComplete);
  const plan = $derived(changeSet.plan);
  const wantedIds = $derived(new Set(changeSet.wanted.map((w) => w.id)));
  const all = $derived(plan ? [...plan.install, ...plan.alreadyOk] : []);
  const picked = $derived(all.filter((m) => wantedIds.has(m.id)));
  const packs = $derived(plan ? plan.packs.filter((m) => wantedIds.has(m.id)) : []);
  const also = $derived(plan ? plan.install.filter((m) => !wantedIds.has(m.id)) : []);
  const natives = $derived(plan ? plan.install.filter((m) => m.kind === "native").map((m) => m.name) : []);
  const canInstall = $derived(
    !!plan && !plan.blocked && (plan.install.length > 0 || plan.alreadyOk.some((m) => m.enable)),
  );
  const failedName = $derived.by(() => {
    const id = changeSet.result?.failed?.modId;
    return id ? (all.find((m) => m.id === id)?.name ?? id) : null;
  });
  const fixLabel = (a: ModAction): string | null => {
    switch (a.type) {
      case "switchToDev":
        return T.switchToDev;
      case "retry":
        return T.retry;
      case "useCached":
        return T.useCached;
      case "report":
        return T.report;
      default:
        return null;
    }
  };

  const line = (m: PlannedMod) =>
    m.installed && m.installed !== m.version
      ? `${m.name} ${updateFrom(m.installed, m.version)}`
      : `${m.name} ${m.version}`;

  async function play() {
    const game = app.library?.games.find((g) => g.id === changeSet.game);
    const v = game?.versions.find((x) => x.name === changeSet.version);
    const action = v?.actions.find((a) => a.id === "play" && a.thisPlatform);
    if (app.config && !app.config.playWithMods) await saveConfig({ ...app.config, playWithMods: true });
    close();
    if (v) {
      app.version = v.key;
      app.page = "game";
      if (action?.runnable) await run({ kind: "version", key: v.key }, action);
    }
  }

  function onkeydown(e: KeyboardEvent) {
    if (shown && e.key === "Escape") close();
  }
</script>

<svelte:window {onkeydown} />

{#if shown}
  <div class="overlay" role="presentation" use:modal>
    <button class="backdrop" onclick={close} tabindex="-1" aria-label={T.closeAria}></button>
    <div class="modal rise" role="dialog" aria-modal="true" aria-labelledby="change-set-title" tabindex="-1">
      <div class="header">
        <div class="title-row">
          <Icon name="library" size={20} />
          <h2 id="change-set-title">{T.title}</h2>
        </div>
        {#if changeSet.title}<span class="game">{changeSet.title}</span>{/if}
      </div>

      <div class="body" aria-live="polite">
        {#if changeSet.phase === "loading"}
          <p class="muted"><span class="spinner"></span>{T.loading}</p>
        {:else if changeSet.phase === "error" && changeSet.error}
          <p class="error" role="alert"><Icon name="warn" size={16} /><span>{modErrorText(changeSet.error)}</span></p>
          <details class="more"><summary>{T.copyDetails}</summary><code>{changeSet.error.detail}</code></details>
        {:else if plan && (changeSet.phase === "ready" || changeSet.phase === "applying" || changeSet.phase === "done")}
          <section>
            <h3>{T.picked}</h3>
            <ul class="mods">
              {#each picked as m (m.id)}
                <li>
                  <span>{line(m)}</span>
                  {#if changeSet.phase !== "ready" && changeSet.progress[m.id]}
                    {@const s = STEP_LABEL[changeSet.progress[m.id] ?? "waiting"]}
                    <span class={`pill ${s.tone}`}>{s.text}</span>
                  {:else if !plan.install.some((x) => x.id === m.id)}
                    <span class="dim">{T.alreadyHave}</span>
                  {/if}
                </li>
              {/each}
              {#each packs as m (m.id)}<li><span>{line(m)}</span></li>{/each}
            </ul>
          </section>

          {#if also.length > 0}
            <section>
              <h3>{T.alsoInstalls} <span class="dim">({alsoCount(also.length)})</span></h3>
              <ul class="mods small">
                {#each also as m (m.id)}
                  <li>
                    <span>{line(m)}</span>
                    {#if changeSet.phase !== "ready" && changeSet.progress[m.id]}
                      {@const s = STEP_LABEL[changeSet.progress[m.id] ?? "waiting"]}
                      <span class={`pill ${s.tone}`}>{s.text}</span>
                    {:else if m.requiredBy.length > 0}
                      <span class="dim"
                        >{neededBy(m.requiredBy.map((id) => all.find((x) => x.id === id)?.name ?? id))}</span
                      >
                    {/if}
                  </li>
                {/each}
              </ul>
            </section>
          {/if}

          {#if changeSet.phase === "ready"}
            {#each [{ title: T.recommended, offers: changeSet.offers.recommends }, { title: T.suggested, offers: changeSet.offers.suggests }] as group (group.title)}
              {#if group.offers.length > 0}
                <section>
                  <h3>{group.title}</h3>
                  {#each group.offers as o (o.id)}
                    <label class="offer">
                      <input
                        type="checkbox"
                        checked={changeSet.ticked.includes(o.id)}
                        onchange={() => void toggle(o.id)}
                      />
                      <span
                        ><strong>{o.name}</strong> <span class="dim">{offeredBy(o.by)}</span><br />{o.description}</span
                      >
                    </label>
                  {/each}
                </section>
              {/if}
            {/each}

            {#if natives.length > 0}
              <p class="warn"><Icon name="warn" size={16} /><span>{nativeWarning(natives)}</span></p>
            {/if}
            {#each plan.notes as n, i (i)}<p class="note">{noteText(n)}</p>{/each}

            {#each plan.conflicts as c (`${c.id}:${c.with}`)}
              <div class="block" role="alert">
                <span>{conflictText(c.name, c.withName, c.reason)}</span>
                {#if c.action}
                  {@const a = c.action}
                  <button class="small" onclick={() => void fix(a)}>{replaceWith(c.withName, c.name)}</button>
                {/if}
              </div>
            {/each}
            {#each plan.missing as m (m.id)}
              <div class="block" role="alert"><span>{missingText(m.id, m.requiredBy)}</span></div>
            {/each}
            {#each plan.incompatible as i (i.id)}
              <div class="block" role="alert">
                <span>{whyText(i.name, i.why)}</span>
                {#if i.action && fixLabel(i.action)}
                  {@const a = i.action}
                  <button class="small" onclick={() => void fix(a)}>{fixLabel(a)}</button>
                {/if}
              </div>
            {/each}
          {/if}

          {#if changeSet.phase === "done" && changeSet.result}
            {@const r = changeSet.result}
            <p class={r.failed ? "error" : "ok"} role="status">
              {appliedText(r.done.length, plan.install.length + plan.packs.length, failedName)}
            </p>
            {#if r.failed}<p class="muted">{modErrorText(r.failed)}</p>{/if}
            {#if r.backup}<p class="note">{T.backedUp}</p>{/if}
          {/if}
        {/if}
      </div>

      <div class="actions">
        {#if changeSet.phase === "done"}
          <button class="ghost" onclick={close}>{T.backToMods}</button>
          {#if changeSet.result?.failed}
            {@const f = changeSet.result.failed}
            {#if f.action?.type === "retry"}<button class="ghost" onclick={() => void apply()}>{T.retry}</button>{/if}
          {/if}
          <button class="primary" onclick={() => void play()}>{T.play}</button>
        {:else if changeSet.phase === "error"}
          <button class="ghost" onclick={close}>{T.cancel}</button>
          {#if changeSet.error?.action && fixLabel(changeSet.error.action)}
            {@const a = changeSet.error.action}
            <button class="primary" onclick={() => void fix(a)}>{fixLabel(a)}</button>
          {/if}
        {:else}
          {#if plan?.blocked}<span class="dim grow">{T.blocked}</span>{/if}
          <button class="ghost" disabled={changeSet.phase === "applying"} onclick={close}>{T.cancel}</button>
          {#if plan?.action?.type === "switchToDev"}
            {@const a = plan.action}
            <button class="primary" onclick={() => void fix(a)}>{T.switchToDev}</button>
          {:else}
            <button class="primary" disabled={!canInstall || changeSet.phase !== "ready"} onclick={() => void apply()}>
              {#if changeSet.phase === "applying"}<span class="spinner"></span>{T.working}{:else}{installCount(
                  plan?.install.length ?? 0,
                )}{/if}
            </button>
          {/if}
        {/if}
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
    max-width: 560px;
    max-height: 100%;
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
    gap: 12px;
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

  .game {
    color: var(--soft);
    font-size: 0.8125rem;
  }

  .body {
    padding: 20px 24px;
    display: flex;
    flex-direction: column;
    gap: 14px;
    overflow-y: auto;
  }

  section {
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  h3 {
    margin: 0;
    font-size: 0.8125rem;
    color: var(--gold);
  }

  .mods {
    margin: 0;
    padding: 0;
    list-style: none;
    display: flex;
    flex-direction: column;
  }

  .mods li {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 12px;
    padding: 6px 0;
    border-bottom: 1px solid var(--line);
    color: var(--body);
  }

  .mods li:last-child {
    border-bottom: 0;
  }

  .mods.small li {
    font-size: 0.8125rem;
  }

  .offer {
    display: flex;
    gap: 10px;
    align-items: flex-start;
    font-size: 0.8125rem;
    color: var(--body);
  }

  .offer strong {
    color: var(--text);
  }

  .warn,
  .error,
  .block {
    display: flex;
    align-items: center;
    gap: 8px;
    margin: 0;
    padding: 10px 14px;
    border-radius: var(--radius-sm);
    font-size: 0.8125rem;
  }

  .warn {
    border: 1px solid var(--amber);
    color: var(--amber);
  }

  .error,
  .block {
    border: 1px solid var(--err);
    background: color-mix(in srgb, var(--err) 12%, transparent);
    color: var(--err);
  }

  .block {
    justify-content: space-between;
  }

  .ok {
    margin: 0;
    color: var(--ok);
  }

  .note {
    margin: 0;
    font-size: 0.8125rem;
    color: var(--soft);
  }

  .muted {
    display: flex;
    gap: 10px;
    align-items: center;
    margin: 0;
  }

  .pill.ok {
    color: var(--ok);
  }

  .pill.warn {
    color: var(--amber);
  }

  .pill.err {
    color: var(--err);
  }

  .pill.off {
    color: var(--dim);
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
    font-size: 0.8125rem;
  }

  code {
    font-family: var(--font-mono);
    font-size: 0.75rem;
    word-break: break-word;
  }
</style>
