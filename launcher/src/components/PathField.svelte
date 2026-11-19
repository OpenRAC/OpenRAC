<script lang="ts">
  // A path setting: the value, what detection found, a picker, and a check.
  import { type Candidate, type Check, pickFile, pickFolder } from "$lib/api";
  import { splitPath } from "$lib/format";

  import Icon from "./Icon.svelte";

  let {
    label,
    hint,
    value = $bindable(null),
    candidates = [],
    folder = false,
    detail = false,
    check,
  }: {
    label: string;
    hint: string;
    value?: string | null;
    candidates?: Candidate[];
    folder?: boolean;
    /** Also say what a good value was found to be (a version, a count): for contributors. */
    detail?: boolean;
    check: (path: string) => Promise<Check>;
  } = $props();

  let result = $state<Check | null>(null);
  let checking = $state(false);
  /** The field's id: the label's words, without the spaces an id may not hold. */
  const id = $derived(`path-${label.toLowerCase().replace(/[^a-z0-9]+/g, "-")}`);

  $effect(() => {
    const path = value;

    result = null;
    if (!path) return;
    checking = true;
    let stale = false;

    void check(path)
      .then((r) => {
        if (!stale) result = r;
      })
      .finally(() => {
        if (!stale) checking = false;
      });

    return () => {
      stale = true;
    };
  });

  /** A dialog is open: a second press opens no other. */
  let browsing = false;

  async function browse() {
    if (browsing) return;
    browsing = true;

    try {
      const picked = folder ? await pickFolder(label) : await pickFile(label);

      if (picked) value = picked;
    } finally {
      browsing = false;
    }
  }
</script>

<div class="field">
  <div class="row">
    <label class="grow" for={id}>{label}</label>
    {#if checking}
      <span class="spinner dim"></span>
    {:else if result && !result.ok}
      <span class="pill err"><span class="dot"></span>Problem</span>
    {:else if result}
      <span class="good" title={result.message}><Icon name="check" size={14} />Found</span>
    {/if}
  </div>
  <p class="dim hint">{hint}</p>
  <div class="row">
    <input
      {id}
      type="text"
      spellcheck="false"
      value={value ?? ""}
      title={value ?? undefined}
      placeholder="Not set"
      onchange={(e) => (value = e.currentTarget.value.trim() || null)}
    />
    <button class="small" onclick={() => void browse()}
      ><Icon name="folder" size={14} />Browse</button
    >
    {#if value}<button class="small" onclick={() => (value = null)} aria-label={`Clear ${label}`}
        ><Icon name="x" size={14} /></button
      >{/if}
  </div>
  {#if result && !result.ok}<p class="message err">{result.message}</p>{:else if result && detail}<p
      class="message dim"
    >
      {result.message}
    </p>{/if}
  {#if candidates.length && candidates.some((c) => c.path !== value)}
    <div class="found">
      Found:
      {#each candidates as c (c.path)}
        {@const part = splitPath(c.path)}
        <!-- One line: a long path loses the middle of its folders, never its file; the whole path is
             in the tooltip. -->
        <button
          class="ghost small mono path"
          class:on={c.path === value}
          onclick={() => (value = c.path)}
          title={`${c.path} (${c.source})`}
          ><span class="head">{part.head}</span>{#if part.tail}<span class="tail">{part.tail}</span
            >{/if}</button
        >
      {/each}
    </div>
  {/if}
</div>

<style>
  .good {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    font-size: 0.75rem;
    font-weight: 600;
    color: var(--ok);
  }

  .field {
    display: flex;
    flex-direction: column;
    gap: 6px;
    padding: 14px 0;
    border-bottom: 1px solid var(--line);
  }

  .field:last-child {
    border-bottom: 0;
  }

  label {
    font-weight: 600;
  }

  .hint,
  .message {
    font-size: 0.8125rem;
  }

  /* A path in a message can be one long word: it breaks anywhere rather than leave the window. */
  .message {
    overflow-wrap: anywhere;
  }

  /* The path field never pushes the row past the panel. */
  input {
    flex: 1;
    min-width: 0;
  }

  .err {
    color: var(--err);
  }

  .found {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 4px;
    font-size: 0.8125rem;
    color: var(--dim);
  }

  .found .path {
    display: inline-flex;
    gap: 0;
    max-width: 100%;
    min-width: 0;
  }

  .path .head {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .path .tail {
    flex: none;
    white-space: nowrap;
  }

  .found .on {
    color: var(--amber);
  }
</style>
