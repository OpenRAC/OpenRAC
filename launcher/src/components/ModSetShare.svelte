<script lang="ts">
  // Share these mods: what a code would hold, the suggestions the sharer may add, then the code. The
  // short code is the one thing to look at; the full code sits under it, for when the server cannot
  // be reached or a friend has none. A mod a catalogue does not hold, or one a party refuses, is
  // said here, before a friend finds out. Nothing leaves the computer before Make code.
  import { api, errorText, type ShareMod, type Shared, type SharePreview, type VersionView } from "$lib/api";
  import { MOD_CODES as T } from "$lib/modsText";
  import { copyText } from "$lib/clipboard";
  import Icon from "./Icon.svelte";
  import ModSetDialog from "./ModSetDialog.svelte";
  import Reason from "./Reason.svelte";

  let { version: v, onclose }: { version: VersionView; onclose: () => void } = $props();

  let preview = $state<SharePreview | null>(null);
  let shared = $state<Shared | null>(null);
  let extras = $state<string[]>([]);
  let working = $state(false);
  let error = $state<string | null>(null);
  /** Which copy button just worked: it says so for a moment. */
  let copied = $state<"code" | "full" | null>(null);

  $effect(() => {
    void api
      .modsetSharePreview(v.game)
      .then((p) => (preview = p))
      .catch((e: unknown) => (error = errorText(e)));
  });

  const label = (m: ShareMod) => `${m.name} ${m.version}`;

  function toggle(id: string, on: boolean) {
    extras = on ? [...extras, id] : extras.filter((x) => x !== id);
  }

  async function make() {
    working = true;
    error = null;
    try {
      shared = await api.modsetShare(v.game, v.name, extras);
    } catch (e) {
      error = errorText(e);
    }
    working = false;
  }

  async function copy(what: "code" | "full", text: string) {
    if (!(await copyText(text, T.copyRefused))) return;
    copied = what;
    setTimeout(() => {
      if (copied === what) copied = null;
    }, 1600);
  }

  const empty = $derived(!!preview && !preview.mods.length && !preview.manual.length && !preview.optional.length);
</script>

<ModSetDialog id="mod-share" icon="share" title={T.shareTitle} busy={working} {onclose}>
  {#if shared}
    {#if shared.code}
      <div class="code-well">
        <span class="sr-only">{T.codeLabel}</span>
        <output class="code">{shared.code}</output>
      </div>
      <p class="note">{T.codeLife}</p>
    {:else}
      <Reason text={T.offline} tone="warn" role="status" />
      <p class="note">{T.offlineFull}</p>
      <textarea class="full" readonly rows="4" aria-label={T.copyFull} value={shared.long}></textarea>
    {/if}
  {:else if !preview}
    {#if !error}<p class="note"><span class="spinner"></span></p>{/if}
  {:else if empty}
    <p class="lead">{T.nothing}</p>
  {:else}
    <p class="lead">{T.shareLead}</p>
    {#if preview.mods.length}
      <section>
        <h3>{T.inCode}</h3>
        <ul>
          {#each preview.mods as m (m.id)}<li>{label(m)}</li>{/each}
        </ul>
      </section>
    {/if}
    {#if preview.optional.length}
      <section>
        <h3>{T.suggest}</h3>
        <p class="note">{T.suggestHint}</p>
        <ul class="ticks">
          {#each preview.optional as m (m.id)}
            <li>
              <label>
                <input
                  type="checkbox"
                  checked={extras.includes(m.id)}
                  onchange={(e) => {
                    toggle(m.id, e.currentTarget.checked);
                  }}
                />{label(m)}
              </label>
            </li>
          {/each}
        </ul>
      </section>
    {/if}
    {#if preview.manual.length}
      <section>
        <h3>{T.byHand}</h3>
        <p class="note">{T.byHandHint}</p>
        <ul>
          {#each preview.manual as m (m.id)}<li>{label(m)}</li>{/each}
        </ul>
      </section>
    {/if}
    {#if preview.solo.length}
      <section>
        <h3>{T.solo}</h3>
        <p class="note">{T.soloHint}</p>
        <ul>
          {#each preview.solo as m (m.id)}<li>{label(m)}</li>{/each}
        </ul>
      </section>
    {/if}
  {/if}
  {#if error}<Reason text={error} tone="err" role="alert" />{/if}

  {#snippet actions()}
    {#if shared}
      <button class="ghost small grow" onclick={() => void copy("full", shared?.long ?? "")}>
        <Icon name={copied === "full" ? "check" : "copy"} size={14} />{copied === "full" ? T.copied : T.copyFull}
      </button>
      <button class="ghost" onclick={onclose}>{T.close}</button>
      {#if shared.code}
        <button class="primary" onclick={() => void copy("code", shared?.code ?? "")}>
          <Icon name={copied === "code" ? "check" : "copy"} size={16} />{copied === "code" ? T.copied : T.copyCode}
        </button>
      {/if}
    {:else}
      <button class="ghost" disabled={working} onclick={onclose}>{T.cancel}</button>
      <button class="primary" disabled={working || !preview || empty} onclick={() => void make()}>
        {#if working}<span class="spinner"></span>{T.making}{:else}{T.make}{/if}
      </button>
    {/if}
  {/snippet}
</ModSetDialog>

<style>
  .lead,
  .note {
    margin: 0;
  }

  .lead {
    color: var(--body);
  }

  .note {
    font-size: 0.8125rem;
    color: var(--soft);
  }

  section {
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  h3 {
    margin: 0;
    font-size: 0.875rem;
    font-weight: 600;
    color: var(--gold);
  }

  ul {
    margin: 0;
    padding: 0;
    list-style: none;
    display: flex;
    flex-direction: column;
    gap: 4px;
    color: var(--body);
    font-size: 0.875rem;
  }

  .ticks label {
    display: flex;
    align-items: center;
    gap: 8px;
    min-height: 28px;
    cursor: pointer;
  }

  /* The code is what this window is for: read aloud, typed on another machine, so it is large and
     set in the launcher's display face, the dash a real gap in its middle. */
  .code-well {
    display: grid;
    place-items: center;
    padding: 28px 16px;
    background: var(--ink-deep);
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
  }

  .code {
    font-family: var(--font-head);
    font-size: clamp(1.75rem, 7vw, 2.5rem);
    font-weight: 600;
    font-variant-numeric: tabular-nums;
    letter-spacing: 0.08em;
    color: var(--amber);
    user-select: all;
    white-space: nowrap;
  }

  .full {
    width: 100%;
    resize: none;
    padding: 10px 12px;
    font-family: var(--font-mono);
    font-size: 0.75rem;
    line-height: 1.5;
    word-break: break-all;
    color: var(--body);
    background: var(--ink-deep);
    border: 1px solid var(--line-field);
    border-radius: var(--radius-sm);
  }

  .grow {
    margin-right: auto;
  }
</style>
