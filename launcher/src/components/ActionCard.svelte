<script lang="ts">
  // One action from launcher/actions.json: what it does, whether it can run
  // here and now (and if not, why), and the button.
  import { api, type ActionView, type Scope } from "$lib/api";
  import { guard, jobs, run } from "$lib/app.svelte";
  import { STATE } from "$lib/labels";
  import Icon, { type IconName } from "./Icon.svelte";

  let { action, scope, icon }: { action: ActionView; scope: Scope; icon: IconName } = $props();

  const state = $derived(STATE[action.state]);
  const sameScope = (s: Scope) =>
    s.kind === scope.kind && (s.kind === "repository" || (scope.kind === "version" && s.key === scope.key));
  const busy = $derived(
    jobs.list.some(
      (j) => j.actionId === action.id && sameScope(j.scope) && (j.state === "queued" || j.state === "running"),
    ),
  );
</script>

<article class="action" class:planned={action.state === "planned"}>
  <div class="row top">
    <h3 class="grow">{action.label}</h3>
    <span class={`pill ${state.tone}`} title={state.hint}><span class="dot"></span>{state.text}</span>
  </div>
  {#if action.description}<p class="muted">{action.description}</p>{/if}

  {#if action.state !== "planned" && action.blockers.length}
    <ul class="blockers">
      {#each action.blockers as b (b)}<li><Icon name="warn" size={14} />{b}</li>{/each}
    </ul>
  {/if}

  {#if action.todo && action.state !== "connected"}
    <details>
      <summary>For contributors</summary>
      <p>{action.todo}</p>
    </details>
  {/if}

  <div class="row bottom">
    <button
      class:primary={action.kind === "play" || action.kind === "build"}
      disabled={!action.runnable || busy}
      onclick={() => void run(scope, action)}
    >
      {#if busy}<span class="spinner"></span>{:else}<Icon name={icon} size={16} />{/if}
      {busy ? "Running…" : action.detached ? "Launch" : "Run"}
    </button>
    <span class="grow"></span>
    {#if action.docs}
      {@const docs = action.docs}
      <button class="ghost small" onclick={() => void guard(api.openPath(docs))} title={docs}>
        <Icon name="book" size={14} />Docs
      </button>
    {/if}
  </div>
</article>

<style>
  .action {
    display: flex;
    flex-direction: column;
    gap: 10px;
    background: var(--panel-hi);
    border: 1px solid var(--line);
    border-radius: var(--radius);
    padding: 16px 18px;
  }

  .action.planned {
    background: transparent;
    border-style: dashed;
  }

  .action.planned h3 {
    color: var(--soft);
  }

  .top {
    align-items: flex-start;
  }

  p {
    font-size: 14px;
  }

  .blockers {
    list-style: none;
    margin: 0;
    padding: 0;
    display: flex;
    flex-direction: column;
    gap: 4px;
    font-size: 13px;
    color: var(--amber);
  }

  .blockers li {
    display: flex;
    align-items: center;
    gap: 7px;
  }

  details {
    font-size: 13px;
    color: var(--dim);
  }

  summary {
    cursor: pointer;
    font-weight: 600;
  }

  details p {
    margin-top: 6px;
    font-size: 13px;
  }

  .bottom {
    margin-top: auto;
  }
</style>
