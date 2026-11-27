<script lang="ts">
  // How much OpenRAC checked a mod (the index's `trust`): an icon and a word, its meaning in the
  // tooltip. Not checked is the one label in colour (--err), so it stands out in a list.
  import { TRUST_HINT, TRUST_LABEL, trustKey } from "$lib/modsText";

  let { trust, big = false }: { trust: string | null | undefined; big?: boolean } = $props();

  const key = $derived(trustKey(trust));
</script>

{#if key}
  <span class="trust" class:risk={key === "unchecked"} class:big title={TRUST_HINT[key]}>
    <b aria-hidden="true">{TRUST_LABEL[key].icon}</b>{TRUST_LABEL[key].text}
  </span>
{/if}

<style>
  .trust {
    display: inline-flex;
    flex: none;
    gap: 4px;
    align-items: center;
    height: 20px;
    padding: 0 6px 0 4px;
    border-radius: var(--radius-xs);
    color: var(--body);
    font-size: 12px;
    font-weight: 500;
    line-height: 20px;
    white-space: nowrap;
  }

  .trust b {
    width: 14px;
    color: var(--amber);
    font-weight: 400;
    text-align: center;
  }

  .trust.risk {
    background: color-mix(in srgb, var(--err) 14%, transparent);
    color: var(--err);
    font-weight: 600;
  }

  .trust.risk b {
    color: var(--err);
    font-weight: 700;
  }

  .trust.big {
    color: var(--text);
    font-size: 13px;
    font-weight: 600;
  }
</style>
