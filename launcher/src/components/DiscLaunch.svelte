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
    turnsAt,
  } from "$lib/insertion";
  import { discPrint } from "$lib/discArt";
  import { ACCENTS } from "$lib/themes";
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
    // Taken in hand it tips towards the player a little.
    const tip = lift * -60;
    return `left: ${x}px; top: ${y}px; width: ${size}px; height: ${size}px; transform: translate(-50%, -50%) translateZ(${z}px) rotateX(${tip}deg) scale(${1 + lift}) rotate(${spinAt(t)}deg)`;
  });

  // A disc at 15 turns a second jumps a quarter turn a frame and seems to shake; the eye sees it
  // smeared. Two pictures of the print, smeared once as the disc comes (a short arc, and all the way
  // round into rings), come over it as it speeds up: two layers a frame, not a copy per step.
  const print = $derived(discPrint(game, version));
  let arc = $state<HTMLCanvasElement | null>(null);
  /** The smeared pictures are drawn: until then (or without the print) a soft ring blurs it. */
  let smeared = $state(false);
  let ring = $state<HTMLCanvasElement | null>(null);
  /** How much the short smear shows, then the rings, 0 to 1. */
  const arcIn = $derived(Math.min(1, (turnsAt(t) / 15) * 4));
  const ringIn = $derived(Math.min(1, Math.max(0, (turnsAt(t) / 15 - 0.25) / 0.45)));

  /** Draws the print `steps` times over `degrees`, each fainter, so all weigh the same. */
  function smearInto(canvas: HTMLCanvasElement, img: HTMLImageElement, degrees: number, steps: number) {
    const g = canvas.getContext("2d");
    if (!g) return;
    const s = canvas.width;
    for (let k = 0; k < steps; k++) {
      g.save();
      g.globalAlpha = 1 / (k + 1);
      g.translate(s / 2, s / 2);
      g.rotate(((k / (steps - 1) - 0.5) * degrees * Math.PI) / 180);
      g.drawImage(img, -s / 2, -s / 2, s, s);
      g.restore();
    }
  }

  /** Its shadow on the drive: it shows as the disc comes down, and spreads as it is lifted. */
  const shade = $derived.by(() => {
    const lift = liftAt(t);
    const near = travelAt(t) ** 3;
    return `opacity: ${near * Math.max(0, 0.75 - lift * 3)}; transform: translate(-50%, -50%) scale(${1 + lift * 1.6})`;
  });

  /** The console gives a little as the lid shuts. */
  const knock = $derived(Math.sin(Math.PI * Math.min(1, Math.max(0, (t - shutAt() + 0.04) / 0.2))) * 4);
  const glow = $derived(ACCENTS[game]?.["--glow"]);
  const black = $derived(blackAt(t));
  const hole = $derived({ x: left + SX * fit, y: top + SY * fit, r: black * Math.hypot(screen.w, screen.h) });

  let done = false;
  let el = $state<HTMLElement | null>(null);

  onMount(() => {
    // Over the whole window, rail too: a page that slides holds its fixed children inside it.
    if (el) document.body.appendChild(el);
    screen = { w: innerWidth, h: innerHeight };
    el?.focus();
    if (print) {
      const img = new Image();
      img.onload = () => {
        if (!arc || !ring) return;
        smearInto(arc, img, 40, 12);
        smearInto(ring, img, 360, 48);
        smeared = true;
      };
      img.src = print;
    }
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
  style={`--in: ${consoleAt(t)}${glow ? `; --glow: ${glow}` : ""}`}
  onclick={stop}
  onkeydown={(e) => {
    if (e.key === "Escape") stop();
  }}
>
  <div
    class="console"
    style={`left: ${left}px; top: ${top}px; transform: perspective(1600px) rotateX(14deg) translateY(${(1 - consoleAt(t)) * 60 + knock}px) scale(${fit})`}
  >
    <div class="body"></div>
    <div class="strip"></div>
    <div class="bay">
      <div class="well">
        <div class="laser"></div>
        <div class="spindle"></div>
      </div>
    </div>
    <div class="shade" style={`left: ${SX}px; top: ${SY}px; width: ${SIZE}px; height: ${SIZE}px; ${shade}`}></div>
    <div class="disc" style={disc}>
      <Disc {game} {version} {title} />
      {#if print}
        <canvas class="smear" width="640" height="640" bind:this={arc} style={`opacity: ${smeared ? arcIn : 0}`}
        ></canvas>
        <canvas class="smear" width="640" height="640" bind:this={ring} style={`opacity: ${smeared ? ringIn : 0}`}
        ></canvas>
      {/if}
      {#if !smeared}
        <span class="blur" style={`opacity: ${speedAt(t) * 0.6}`}></span>
      {/if}
    </div>
    <div
      class="lid"
      style={`transform: translateZ(8px) rotateX(${104 * (1 - lidAt(t))}deg); filter: brightness(${1.35 - 0.35 * lidAt(t)})`}
    ></div>
    <div class="led" class:on={t >= shutAt()}></div>
  </div>
  {#if black > 0}
    <div
      class="black"
      style={`left: ${hole.x}px; top: ${hole.y}px; width: ${hole.r * 2.6}px; height: ${hole.r * 2.6}px`}
    ></div>
  {/if}
</div>

<style>
  .launch {
    position: fixed;
    inset: 0;
    z-index: 1000;
    background:
      radial-gradient(
        50% 60% at 50% 54%,
        color-mix(in srgb, var(--glow) calc(var(--in) * 14%), transparent),
        transparent 75%
      ),
      color-mix(in srgb, var(--ink) calc(var(--in) * 100%), transparent);
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
    background: linear-gradient(180deg, rgb(46 46 51), rgb(27 27 31) 60%, rgb(20 20 23));
    box-shadow:
      inset 0 1px 0 rgb(255 255 255 / 0.09),
      inset 0 -2px 0 rgb(0 0 0 / 0.4),
      0 30px 60px rgb(0 0 0 / 0.55);
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

  .smear {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    mask: radial-gradient(circle closest-side, transparent 0 12.6%, black 13% 99.2%, transparent 100%);
  }

  .shade {
    position: absolute;
    border-radius: 50%;
    background: radial-gradient(circle closest-side, rgb(0 0 0 / 0.7) 60%, transparent);
    filter: blur(10px);
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
    background:
      linear-gradient(180deg, rgb(255 255 255 / 0.06), transparent 30%),
      repeating-radial-gradient(circle at 50% 50%, rgb(40 40 45) 0 2px, rgb(33 33 37) 2px 22px);
    box-shadow:
      inset 0 0 0 1px rgb(255 255 255 / 0.06),
      0 10px 24px rgb(0 0 0 / 0.45);
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
    background: radial-gradient(circle closest-side, rgb(0 0 0) 72%, transparent);
    transform: translate(-50%, -50%);
  }
</style>
