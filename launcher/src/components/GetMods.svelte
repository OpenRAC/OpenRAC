<script lang="ts">
  // My mods' first line: where mods come from. The launcher lists no mods itself: the player finds
  // them on the official hub (openrac.dev, whose Install opens the launcher) or anywhere else, and
  // adds a link, a .zip or a folder in Add a mod below. With the hub, a click or tap anywhere on it
  // opens it; a keyboard or a pad reaches the same through its button.
  import { api } from "$lib/api";
  import { guardShown } from "$lib/app.svelte";
  import { GET_MODS as T } from "$lib/modsText";
  import Icon from "./Icon.svelte";

  /** Whether the hub has mods for this game (Ratchet & Clank's for now). */
  let { hub = true }: { hub?: boolean } = $props();

  const openHub = () => void guardShown(api.openUrl(T.hubUrl));

  /** A click on the banner, not on one of its buttons (they do their own). */
  function onBanner(e: MouseEvent) {
    if (hub && !(e.target instanceof Element && e.target.closest("button"))) openHub();
  }
</script>

<!-- The banner's click repeats its button's, which keyboards and pads use. -->
<!-- svelte-ignore a11y_click_events_have_key_events, a11y_no_noninteractive_element_interactions -->
<section class="get" class:linked={hub} aria-label={T.title} onclick={onBanner}>
  <span class="mark"><Icon name="globe" size={22} /></span>
  <div class="grow">
    <h2>{T.title}</h2>
    <p>{hub ? T.lead : T.leadNoHub}</p>
  </div>
  {#if hub}<div class="buttons">
      <button class="hub" onclick={openHub}><Icon name="external" size={16} />{T.hub}</button>
      <button class="publish" onclick={() => void guardShown(api.openUrl(T.publishUrl))}
        ><Icon name="external" size={16} />{T.publish}</button
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
    border-radius: var(--radius);
    background: linear-gradient(120deg, color-mix(in srgb, var(--amber) 14%, var(--panel)), var(--panel) 70%);
    /* A card: its buttons take the fill for a card. */
    --control: var(--control-card);
    --control-hi: var(--control-card-hi);
  }

  /* With the hub: the whole banner opens it. */
  .get.linked {
    cursor: pointer;
    transition: border-color var(--fast) var(--ease);
  }

  @media (hover: hover) {
    :global(html:where(:not([data-input="pad"]))) .get.linked:hover {
      background: var(--on-fill);
      color: var(--amber);
    }
  }

  .get.linked:has(.hub:focus-visible) {
    background: var(--on-fill);
    color: var(--amber);
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
