<script lang="ts">
  // Play on Android's Games tab: the console comes in, its lid open; the disc leaves the shelf, turns,
  // is taken in hand and pushed onto the spindle with a click, spins up; the lid shuts, the light
  // comes on, and the screen goes black from the disc's centre (insertion.ts says where everything is
  // at each moment). Then `ondone` starts the game. B, Back or a tap before the click puts the disc
  // back (`oncancel`). The console is the launcher's own drawing, with no maker's marks.
  import { onMount } from "svelte";
  import {
    blackAt,
    consoleAt,
    duration,
    lidAt,
    liftAt,
    seatedAt,
    shutAt,
    speedAt,
    spinAt,
    travelAt,
  } from "$lib/insertion";
  import Disc from "./Disc.svelte";

  let {
    game,
    version,
    title,
    from,
    ondone,
    oncancel,
  }: {
    game: string;
    version: string;
    title: string;
    from: DOMRect;
    ondone: () => void;
    oncancel: () => void;
  } = $props();

  /** The console's own size; it is drawn at this size and scaled to the screen. */
  const W = 860,
    H = 560;
  /** The spindle's centre in the console, and the disc's size on it. */
  const SX = 537,
    SY = 280,
    SIZE = 420;

  let t = $state(0);
  let screen = $state({ w: 1280, h: 720 });
  const fit = $derived(Math.min((screen.w * 0.86) / W, (screen.h * 0.86) / H));
  const left = $derived((screen.w - W * fit) / 2);
  const top = $derived(screen.h * 0.52 - (H * fit) / 2);

  // The disc's path, in the console's own units: from where it stood on the shelf to the spindle.
  const fromX = $derived((from.left + from.width / 2 - left) / fit);
  const fromY = $derived((from.top + from.height / 2 - top) / fit);
  const fromSize = $derived(from.width / fit);
  const disc = $derived.by(() => {
    const go = travelAt(t);
    const lift = liftAt(t);
    const x = fromX + (SX - fromX) * go;
    const y = fromY + (SY - fromY) * go;
    const size = fromSize + (SIZE - fromSize) * go;
    const z = 2 + lift * 600 + (1 - go) * 160;
    return `left: ${x}px; top: ${y}px; width: ${size}px; height: ${size}px; transform: translate(-50%, -50%) translateZ(${z}px) scale(${1 + lift}) rotate(${spinAt(t)}deg)`;
  });
  const black = $derived(blackAt(t));
  const hole = $derived({ x: left + SX * fit, y: top + SY * fit, r: black * Math.hypot(screen.w, screen.h) });

  let done = false;
  let el = $state<HTMLElement | null>(null);

  onMount(() => {
    // Over the whole window, rail too: a page that slides holds its fixed children inside it.
    if (el) document.body.appendChild(el);
    screen = { w: innerWidth, h: innerHeight };
    el?.focus();
    const begun = performance.now();
    let frame = 0;
    const tick = (now: number) => {
      t = (now - begun) / 1000;
      if (t >= duration()) {
        if (!done) {
          done = true;
          ondone();
        }
        return;
      }
      frame = requestAnimationFrame(tick);
    };
    frame = requestAnimationFrame(tick);
    return () => {
      cancelAnimationFrame(frame);
      // Svelte no longer finds it once moved to <body>, so it goes from there by hand.
      // eslint-disable-next-line svelte/no-dom-manipulating
      el?.remove();
    };
  });

  /** Before the click the disc can still go back to the shelf. */
  function stop() {
    if (t < seatedAt() && !done) {
      done = true;
      oncancel();
    }
  }
</script>

<div
  class="launch"
  role="dialog"
  aria-modal="true"
  aria-label={`Starting ${title}`}
  tabindex="-1"
  bind:this={el}
  style={`--in: ${consoleAt(t)}`}
  onclick={stop}
  onkeydown={(e) => {
    if (e.key === "Escape") stop();
  }}
>
  <div
    class="console"
    style={`left: ${left}px; top: ${top}px; transform: perspective(1600px) rotateX(14deg) translateY(${(1 - consoleAt(t)) * 60}px) scale(${fit})`}
  >
    <div class="body"></div>
    <div class="strip"></div>
    <div class="bay">
      <div class="well">
        <div class="laser"></div>
        <div class="spindle"></div>
      </div>
    </div>
    <div class="disc" style={disc}>
      <Disc {game} {version} {title} />
      <span class="blur" style={`opacity: ${speedAt(t) * 0.85}`}></span>
    </div>
    <div
      class="lid"
      style={`transform: translateZ(8px) rotateX(${118 * (1 - lidAt(t))}deg); filter: brightness(${0.75 + 0.25 * lidAt(t)})`}
    ></div>
    <div class="led" class:on={t >= shutAt()}></div>
  </div>
  {#if black > 0}
    <div
      class="black"
      style={`left: ${hole.x}px; top: ${hole.y}px; width: ${hole.r * 2}px; height: ${hole.r * 2}px`}
    ></div>
  {/if}
</div>

<style>
  .launch {
    position: fixed;
    inset: 0;
    z-index: 1000;
    background: color-mix(in srgb, var(--ink) calc(var(--in) * 100%), transparent);
    outline: none;
    overflow: hidden;
  }

  .console {
    position: absolute;
    width: 860px;
    height: 560px;
    transform-origin: 0 0;
    transform-style: preserve-3d;
    opacity: var(--in);
  }

  .body {
    position: absolute;
    inset: 0;
    border-radius: 14px;
    background: linear-gradient(180deg, rgb(43 43 48), rgb(26 26 30) 60%, rgb(20 20 23));
    box-shadow: 0 30px 60px rgb(0 0 0 / 0.55);
  }

  /* The ribbed strip along its left side, where the slim keeps its buttons. */
  .strip {
    position: absolute;
    left: 0;
    top: 0;
    bottom: 0;
    width: 210px;
    border-radius: 14px 0 0 14px;
    background: repeating-linear-gradient(90deg, rgb(32 32 36) 0 6px, rgb(24 24 27) 6px 9px);
  }

  .bay {
    position: absolute;
    left: 236px;
    top: 22px;
    right: 22px;
    bottom: 22px;
    border-radius: 8px;
    background: rgb(18 18 20);
    box-shadow: inset 0 6px 18px rgb(0 0 0 / 0.8);
  }

  .well {
    position: absolute;
    left: 50%;
    top: 50%;
    width: 440px;
    height: 440px;
    transform: translate(-50%, -50%);
    border-radius: 50%;
    background: radial-gradient(circle, rgb(30 30 34) 0 60%, rgb(13 13 15) 61% 100%);
  }

  .laser {
    position: absolute;
    left: 50%;
    top: 18%;
    width: 34px;
    height: 120px;
    transform: translateX(-50%);
    border-radius: 4px;
    background: rgb(10 10 11);
  }

  .laser::after {
    content: "";
    position: absolute;
    left: 10px;
    right: 10px;
    top: 14px;
    height: 52px;
    border-radius: 2px;
    background: linear-gradient(rgb(224 122 30), rgb(184 87 12));
  }

  .spindle {
    position: absolute;
    left: 50%;
    top: 50%;
    width: 64px;
    height: 64px;
    transform: translate(-50%, -50%);
    border-radius: 50%;
    background: radial-gradient(circle, rgb(0 0 0) 0 18%, rgb(58 58 64) 20% 55%, rgb(27 27 31) 57%);
  }

  .disc {
    position: absolute;
  }

  /* The print smears as the drive turns it: a ring of soft light over it, not a blur of each frame. */
  .blur {
    position: absolute;
    inset: 0;
    border-radius: 50%;
    background: radial-gradient(
      circle closest-side,
      transparent 0 13%,
      rgb(200 200 210 / 0.35) 14% 30%,
      rgb(120 120 130 / 0.3) 31% 70%,
      rgb(200 200 210 / 0.25) 71% 99%,
      transparent 100%
    );
    pointer-events: none;
  }

  .lid {
    position: absolute;
    left: 236px;
    right: 22px;
    top: 22px;
    height: 516px;
    transform-origin: 50% 0;
    border-radius: 8px;
    background: repeating-radial-gradient(circle at 50% 50%, rgb(38 38 43) 0 2px, rgb(32 32 36) 2px 22px);
  }

  .led {
    position: absolute;
    left: 40px;
    bottom: 26px;
    width: 12px;
    height: 12px;
    border-radius: 50%;
    background: rgb(58 18 16);
    transition: background 120ms;
  }

  .led.on {
    background: rgb(93 255 122);
  }

  .black {
    position: fixed;
    border-radius: 50%;
    background: rgb(0 0 0);
    transform: translate(-50%, -50%);
  }
</style>
