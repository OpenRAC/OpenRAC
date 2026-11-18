<script lang="ts">
  // A tab with a list on the left (Games, Mods, Develop): the LIST, always there, and the open
  // page beside it, scrolling on its own. On Android the list is a row above the page instead: the
  // navigation rail is already on the left, and two columns there leave the page too narrow.
  import type { Snippet } from "svelte";
  import { onAndroid } from "$lib/api";

  let { list, children }: { list: Snippet; children: Snippet } = $props();
</script>

<!-- A controller's moves between the two columns (pad.ts, steer): right from the list goes to the
     page's main button, left from the page to the entry open; up and down stay in their column. -->
<div class="frame">
  {#if onAndroid}
    <aside>{@render list()}</aside>
    <div class="content">{@render children()}</div>
  {:else}
    <aside data-pad-right=".content .go .big" data-pad-down="none">{@render list()}</aside>
    <div
      class="content"
      data-pad-left="aside [aria-current]"
      data-pad-up="header.site-header [aria-current]"
      data-pad-down="none"
    >
      {@render children()}
    </div>
  {/if}
</div>

<style>
  /* As tall as <main>, never taller: the list and the page each scroll in their own column, and
     <main> stays still. A sticky list in a scrolling <main> looks the same, but WebKitGTK stops
     drawing it for the rest of a slide (app.css) if the page grows past the window during it, as
     a page does when what it loads arrives. */
  .frame {
    display: grid;
    grid-template-columns: 248px minmax(0, 1fr);
    grid-template-rows: minmax(0, 1fr);
    height: 100%;
  }

  aside {
    overflow-y: auto;
  }

  /* Each column starts at the window's top, under a desktop's header: up over <main>'s room for it
     (App.svelte), its first line below the header. */
  :global(html:not([data-platform="android"])) .frame {
    height: calc(100% + var(--nav-top));
    margin-top: calc(-1 * var(--nav-top));
  }

  :global(html:not([data-platform="android"])) aside,
  :global(html:not([data-platform="android"])) .content {
    padding-top: var(--nav-top);
  }

  .content {
    min-width: 0;
    overflow: hidden auto;
  }

  /* The list stays put while the page beside it slides (app.css). */
  :global(html:not([data-platform="android"])) aside {
    view-transition-name: side;
  }

  @media (max-width: 1000px) {
    .frame {
      grid-template-columns: 76px minmax(0, 1fr);
    }
  }

  /* One column, and <main> scrolls the two together. */
  :global(html[data-platform="android"]) .frame {
    grid-template-columns: minmax(0, 1fr);
    grid-template-rows: none;
    height: auto;
    min-height: 100%;
  }

  :global(html[data-platform="android"]) .content {
    overflow: visible;
  }

  /* It scrolls sideways when the entries are wider than the page, by dragging (no bar on Android:
     app.css), and goes up with the page. */
  :global(html[data-platform="android"]) aside {
    align-self: start;
    overflow: auto hidden;
  }
</style>
