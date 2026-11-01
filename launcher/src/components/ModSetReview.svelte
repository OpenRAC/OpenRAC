<script lang="ts">
  // A mod code's review: everything Apply would do, grouped by what happens, and nothing changed
  // until its one button. It opens from any page (lib/modCodes): the Mods page's field, a paste, or a
  // link from outside, and a link never does more than open it. A source that is not OpenRAC's, and
  // that the player has never used, has to be ticked first; Apply waits for a running game to close.
  // After Apply the window says what came of it and offers the way back.
  import { api, errorText, type Applied, type Review, type VersionView } from "$lib/api";
  import { canApply, dateText, groupMods, matching, optionalMods, untrusted, valueText, versions } from "$lib/modCodes";
  import { closeCode, modCode, modsChanged, versionFor } from "$lib/modCodes.svelte";
  import { gameRunning, toast } from "$lib/app.svelte";
  import { KIND_HINT, KIND_LABEL, MOD_CODES as T } from "$lib/modsText";
  import Icon from "./Icon.svelte";
  import ModSetDialog from "./ModSetDialog.svelte";
  import Reason from "./Reason.svelte";

  let review = $state<Review | null>(null);
  let version = $state<VersionView | null>(null);
  let error = $state<string | null>(null);
  let applied = $state<Applied | null>(null);
  let working = $state(false);
  let extras = $state<string[]>([]);
  let trusted = $state<string[]>([]);

  const entry = $derived(modCode.open);
  const running = $derived(gameRunning());

  /** Reads a code for a disc; one for another game is read again for that game's. */
  async function read(text: string, v: VersionView) {
    review = null;
    error = null;
    applied = null;
    extras = [];
    trusted = [];
    version = v;
    try {
      const got = await api.modsetReview(v.game, v.name, text);
      const other = got.wrongGame ? versionFor(got.game) : null;
      if (other) {
        version = other;
        review = await api.modsetReview(other.game, other.name, text);
      } else {
        review = got;
      }
    } catch (e) {
      error = errorText(e);
    }
  }

  $effect(() => {
    if (entry) void read(entry.text, entry.version);
  });

  function toggle(list: string[], id: string, on: boolean): string[] {
    return on ? [...list, id] : list.filter((x) => x !== id);
  }

  async function apply() {
    if (!review || !version) return;
    working = true;
    error = null;
    try {
      applied = await api.modsetApply(version.game, version.name, review.set, review.code, { extras, trusted });
      modsChanged();
    } catch (e) {
      error = errorText(e);
    }
    working = false;
  }

  async function undo() {
    if (!applied || !version) return;
    working = true;
    try {
      await api.modsetRestore(version.game, applied.profile);
      modsChanged();
      toast(T.undone);
      closeCode();
    } catch (e) {
      error = errorText(e);
    }
    working = false;
  }

  const groups = $derived(review ? groupMods(review) : []);
  const optional = $derived(review ? optionalMods(review) : []);
  const same = $derived(review ? matching(review) : 0);
  const asking = $derived.by(() => {
    if (!review) return [];
    const needed = untrusted(review, extras, []);
    return review.ask.filter((s) => needed.includes(s.url));
  });
  const ready = $derived(!!review && canApply(review, extras, trusted, running));
  const subtitle = $derived.by(() => {
    if (!review) return null;
    const date = dateText(review.created);
    return review.code ? T.codeAndDate(review.code, date) : T.madeOn(date);
  });
</script>

{#if entry}
  <ModSetDialog
    id="mod-review"
    icon={applied ? "check" : "library"}
    title={applied ? T.doneTitle : (review?.name ?? T.reading)}
    subtitle={applied ? null : subtitle}
    busy={working}
    onclose={closeCode}
  >
    {#if applied}
      <p class="lead">{T.done}</p>
      {#if applied.skipped.length}<Reason text={T.doneSkipped(applied.skipped.length)} tone="warn" role="status" />{/if}
    {:else if review}
      {#if review.wrongGame}<Reason text={T.otherGame} tone="err" role="status" />{/if}
      {#if review.otherOpenrac}<Reason text={T.otherOpenrac(review.otherOpenrac)} tone="warn" />{/if}
      {#if review.nothing && !optional.length && !review.wrongGame}<p class="lead">{T.already}</p>{/if}

      {#each groups as group (group.key)}
        <section>
          <h3>{group.title}</h3>
          <ul>
            {#each group.mods as m (m.id)}
              <li class:gone={group.key === "unavailable"}>
                <div class="row">
                  <span class="name">{m.name}</span>
                  {#if m.native && group.key !== "unavailable" && group.key !== "enable"}
                    <span class="runs" title={KIND_HINT.native}><span class="dot"></span>{KIND_LABEL.native.text}</span>
                  {/if}
                  <span class="version">{versions(m)}</span>
                </div>
                {#if m.source && group.key !== "enable"}<div class="meta">{m.source}</div>{/if}
                {#if m.fresh}<div class="fresh"><Icon name="warn" size={14} />{T.fresh}</div>{/if}
                {#if m.why}<div class="why">
                    {m.why}{#if m.nearest}&nbsp;{T.nearest(m.nearest)}{/if}
                  </div>{/if}
              </li>
            {/each}
          </ul>
        </section>
      {/each}

      {#if review.disable.length}
        <section>
          <h3>{T.group.disable}</h3>
          <ul>
            {#each review.disable as m (m.id)}
              <li>
                <div class="row">
                  <span class="name">{m.name}</span><span class="version">{m.version}</span>
                </div>
              </li>
            {/each}
          </ul>
        </section>
      {/if}

      {#if review.settings.length}
        <section>
          <h3>{T.group.settings}</h3>
          <ul>
            {#each review.settings as s, i (i)}
              <li>
                <div class="row">
                  <span class="name">{s.name}</span>
                  <span class="version">{valueText(s.from)} → {valueText(s.to)}</span>
                </div>
                <div class="meta">{s.modName}</div>
              </li>
            {/each}
          </ul>
        </section>
      {/if}

      {#if optional.length}
        <section>
          <h3>{T.group.optional}</h3>
          <ul>
            {#each optional as m (m.id)}
              <li>
                <label class="row tick">
                  <input
                    type="checkbox"
                    checked={extras.includes(m.id)}
                    onchange={(e) => (extras = toggle(extras, m.id, e.currentTarget.checked))}
                  />
                  <span class="name">{m.name}</span>
                  <span class="version">{versions(m)}</span>
                </label>
              </li>
            {/each}
          </ul>
        </section>
      {/if}

      {#if review.manual.length}
        <section>
          <h3>{T.group.manual}</h3>
          <p class="note">{T.manualHint}</p>
          <ul>
            {#each review.manual as m (m.id)}
              <li>
                <div class="row">
                  <span class="name">{m.name}</span><span class="version">{m.version}</span>
                </div>
              </li>
            {/each}
          </ul>
        </section>
      {/if}

      {#if review.packs.length}
        <section>
          <h3>{T.group.packs}</h3>
          <p class="note">{T.packsHint}</p>
          <ul>
            {#each review.packs as pack (pack)}<li><span class="name">{pack}</span></li>{/each}
          </ul>
        </section>
      {/if}

      {#if same > 0 && !review.nothing}<p class="note">{T.matches(same)}</p>{/if}

      {#each asking as source (source.url)}
        <div class="trust">
          <p><strong>{T.trustTitle}</strong> {T.trustLine(source.label)}</p>
          <label>
            <input
              type="checkbox"
              checked={trusted.includes(source.url)}
              onchange={(e) => (trusted = toggle(trusted, source.url, e.currentTarget.checked))}
            />{T.trust(source.label)}
          </label>
        </div>
      {/each}

      {#if review.partial && !review.wrongGame}<Reason text={T.partial} tone="warn" />{/if}
      {#if running}<Reason text={T.running} tone="warn" role="status" />{/if}
    {:else if !error}
      <p class="note"><span class="spinner"></span></p>
    {/if}
    {#if error}<Reason text={error} tone="err" role="alert" />{/if}

    {#snippet actions()}
      {#if applied}
        <button class="ghost small grow" disabled={working} onclick={() => void undo()}>{T.undo}</button>
        <button class="primary" disabled={working} onclick={closeCode}>{T.close}</button>
      {:else if review && !(review.nothing && !optional.length) && !review.wrongGame}
        <span class="saved grow">{T.saved}</span>
        <button class="ghost" disabled={working} onclick={closeCode}>{T.cancel}</button>
        <button class="primary" disabled={working || !ready} onclick={() => void apply()}>
          {#if working}<span class="spinner"></span>{T.applying}{:else}{T.apply}{/if}
        </button>
      {:else}
        <button class="primary" onclick={closeCode}>{T.close}</button>
      {/if}
    {/snippet}
  </ModSetDialog>
{/if}

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
  }

  /* A ledger, not cards: rows on one rule, the versions in a column to the right. */
  li {
    padding: 7px 0;
    border-top: 1px solid var(--line);
    font-size: 0.875rem;
    color: var(--body);
  }

  .row {
    display: flex;
    align-items: baseline;
    flex-wrap: wrap;
    gap: 2px 10px;
  }

  .name {
    color: var(--text);
    overflow-wrap: anywhere;
  }

  .version {
    margin-left: auto;
    font-family: var(--font-mono);
    font-size: 0.8125rem;
    font-variant-numeric: tabular-nums;
    color: var(--lav);
  }

  .runs {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    font-size: 0.75rem;
    color: var(--warn);
  }

  .dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: currentColor;
  }

  .meta {
    font-size: 0.75rem;
    color: var(--dim);
  }

  .fresh {
    display: flex;
    align-items: center;
    gap: 6px;
    margin-top: 2px;
    font-size: 0.75rem;
    color: var(--warn);
  }

  .why {
    margin-top: 2px;
    font-size: 0.8125rem;
    color: var(--soft);
  }

  .gone .name {
    color: var(--soft);
  }

  .tick {
    cursor: pointer;
  }

  .tick input {
    align-self: center;
  }

  /* The one decision here that protects the computer: it stands apart from the ledger. */
  .trust {
    display: flex;
    flex-direction: column;
    gap: 8px;
    padding: 12px 14px;
    background: color-mix(in srgb, var(--warn) 10%, transparent);
    border: 1px solid color-mix(in srgb, var(--warn) 45%, transparent);
    border-radius: var(--radius-sm);
    font-size: 0.875rem;
    color: var(--body);
  }

  .trust p {
    margin: 0;
  }

  .trust strong {
    color: var(--text);
  }

  .trust label {
    display: flex;
    align-items: center;
    gap: 8px;
    min-height: 28px;
    color: var(--text);
    cursor: pointer;
  }

  .saved {
    font-size: 0.75rem;
    color: var(--soft);
  }

  .grow {
    flex: 1 1 180px;
    min-width: 0;
    text-align: left;
  }
</style>
