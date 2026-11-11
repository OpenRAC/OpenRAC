<script lang="ts">
  // Play on Android's Games tab: the console comes in, its lid open; the disc leaves the shelf, turns,
  // is taken in hand and pushed onto the spindle with a click, spins up; the lid shuts, the light
  // comes on, and the screen goes black from the disc's centre (insertion.ts says where everything is
  // at each moment), with the click and the drive's sound (driveSounds.ts). Then `ondone` starts the game. B, Back or a tap before the click puts the disc
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
  import { driveSounds, type DriveSounds } from "$lib/driveSounds";
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

  /** Seconds since the press. Read by the handlers only: each frame writes its styles straight to
   *  the elements (`paint`), so nothing re-renders, and only transforms and opacities change: the
   *  WebView composites them without laying out or repainting a thing. */
  let t = 0;
  let screen = { w: 1280, h: 720 };
  let fit = 1,
    left = 0,
    top = 0;
  let fromX = 0,
    fromY = 0,
    fromScale = 1;

  const print = $derived(discPrint(game, version));
  const glow = $derived(ACCENTS[game]?.["--glow"]);

  let el = $state<HTMLElement | null>(null);
  let ink = $state<HTMLElement | null>(null);
  let light = $state<HTMLElement | null>(null);
  let stage = $state<HTMLElement | null>(null);
  let box = $state<HTMLElement | null>(null);
  let discEl = $state<HTMLElement | null>(null);
  let spinEl = $state<HTMLElement | null>(null);
  let shadeEl = $state<HTMLElement | null>(null);
  let lidEl = $state<HTMLElement | null>(null);
  let litEl = $state<HTMLElement | null>(null);
  let ledEl = $state<HTMLElement | null>(null);
  let holeEl = $state<HTMLElement | null>(null);
  let allEl = $state<HTMLElement | null>(null);
  let blurEl = $state<HTMLElement | null>(null);

  // A disc at 15 turns a second jumps a quarter turn a frame and seems to shake; the eye sees it
  // smeared. Two pictures of the print, smeared once as the disc comes (a short arc, and all the way
  // round into rings), come over it as it speeds up: two layers a frame, not a copy per step.
  let arc = $state<HTMLCanvasElement | null>(null);
  let ring = $state<HTMLCanvasElement | null>(null);
  /** The smeared pictures are drawn: until then (or without the print) a soft ring blurs it. */
  let smeared = false;

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

  /** The black circle is drawn once at this size and scaled: a layer the size of the screen's
   *  diagonal, several times over, is more than the GPU keeps in one piece, and it fell apart into
   *  blocks in the last frames. */
  const HOLE = 256;
  let diagonal = 1;

  function measure() {
    screen = { w: innerWidth, h: innerHeight };
    fit = Math.min((screen.w * 0.86) / W, (screen.h * 0.86) / H);
    left = (screen.w - W * fit) / 2;
    top = screen.h * 0.52 - (H * fit) / 2;
    // The disc's path, in the console's own units: from where it stood on the shelf to the spindle.
    fromX = (from.left + from.width / 2 - left) / fit;
    fromY = (from.top + from.height / 2 - top) / fit;
    fromScale = from.width / fit / SIZE;
    diagonal = Math.hypot(screen.w, screen.h);
    if (box) {
      box.style.left = `${left}px`;
      box.style.top = `${top}px`;
    }
    if (holeEl) {
      holeEl.style.left = `${left + SX * fit}px`;
      holeEl.style.top = `${top + SY * fit}px`;
    }
  }

  const clamp01 = (x: number) => Math.min(1, Math.max(0, x));

  /** Puts everything where it is at `t`. */
  function paint() {
    const into = consoleAt(t);
    const go = travelAt(t);
    const lift = liftAt(t);
    const lid = lidAt(t);
    const black = blackAt(t);
    const over = t >= duration();

    if (ink) ink.style.opacity = `${into}`;
    if (light) light.style.opacity = `${into * 0.14}`;
    if (stage) {
      stage.style.opacity = `${into}`;
      stage.style.visibility = over ? "hidden" : "";
    }
    // The console gives a little as the lid shuts.
    const knock = Math.sin(Math.PI * clamp01((t - shutAt() + 0.04) / 0.2)) * 1.5;
    if (box)
      box.style.transform = `perspective(1600px) rotateX(14deg) translate3d(0, ${(1 - into) * 60 + knock}px, 0) scale(${fit})`;

    if (discEl) {
      const x = (fromX - SX) * (1 - go);
      const y = (fromY - SY) * (1 - go);
      const scale = (fromScale + (1 - fromScale) * go) * (1 + lift);
      const z = 2 + lift * 600 + (1 - go) * 160;
      // Taken in hand it tips towards the player a little.
      discEl.style.transform = `translate3d(${x}px, ${y}px, ${z}px) rotateX(${lift * -60}deg) scale(${scale})`;
    }
    if (spinEl) spinEl.style.transform = `rotate(${spinAt(t) % 360}deg)`;
    const turns = turnsAt(t) / 15;
    if (arc) arc.style.opacity = `${smeared ? Math.min(1, turns * 4) : 0}`;
    if (ring) ring.style.opacity = `${smeared ? clamp01((turns - 0.25) / 0.45) : 0}`;
    if (blurEl) blurEl.style.opacity = `${smeared ? 0 : speedAt(t) * 0.6}`;

    // Its shadow on the drive: it shows as the disc comes down, and spreads as it is lifted.
    if (shadeEl) {
      shadeEl.style.opacity = `${go ** 3 * Math.max(0, 0.75 - lift * 3)}`;
      shadeEl.style.transform = `scale(${1 + lift * 1.6})`;
    }
    if (lidEl) {
      lidEl.style.transform = `translateZ(8px) rotateX(${104 * (1 - lid)}deg)`;
    }
    // Open, it catches the light.
    if (litEl) litEl.style.opacity = `${1 - lid}`;
    ledEl?.classList.toggle("on", t >= shutAt());

    if (holeEl) {
      holeEl.style.opacity = black > 0 && !over ? "1" : "0";
      holeEl.style.transform = `scale(${Math.max(0.001, (black * diagonal * 2.6) / HOLE)})`;
    }
    // The last of the circle's growth hands over to plain black, which stays until the game covers
    // the launcher: one flat layer, nothing to fall apart while Android takes the window away.
    if (allEl) allEl.style.opacity = `${over ? 1 : clamp01((black - 0.7) / 0.3)}`;
  }

  let done = false;
  let sounds: DriveSounds | null = null;

  onMount(() => {
    // Over the whole window, rail too: a page that slides holds its fixed children inside it.
    if (el) document.body.appendChild(el);
    measure();
    paint();
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
    // The press has just happened: the page may play sound now.
    sounds = driveSounds(seatedAt(), duration() - seatedAt());
    const begun = performance.now();
    let frame = 0;
    const tick = (now: number) => {
      t = Math.max(0, (now - begun) / 1000);
      paint();
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
    const resized = () => {
      measure();
      paint();
    };
    addEventListener("resize", resized);
    return () => {
      cancelAnimationFrame(frame);
      removeEventListener("resize", resized);
      sounds?.stop(0.3);
      // Svelte no longer finds it once moved to <body>, so it goes from there by hand.
      // eslint-disable-next-line svelte/no-dom-manipulating
      el?.remove();
    };
  });

  /** Before the click the disc can still go back to the shelf. */
  function stop() {
    if (t < seatedAt() && !done) {
      done = true;
      sounds?.stop(0.05);
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
  style={glow ? `--glow: ${glow}` : ""}
  onclick={stop}
  onkeydown={(e) => {
    if (e.key === "Escape") stop();
  }}
>
  <div class="ink" bind:this={ink}></div>
  <div class="light" bind:this={light}></div>
  <!-- The fade sits outside the console's 3D: an opacity on it would flatten the lid and the disc
       until it reached 1, and they would jump into depth as the console finished coming in. -->
  <div class="stage" bind:this={stage}>
    <div class="console" bind:this={box}>
      <div class="body"></div>
      <div class="strip"></div>
      <div class="bay">
        <div class="well">
          <div class="laser"></div>
          <div class="spindle"></div>
        </div>
      </div>
      <div class="shade" bind:this={shadeEl} style={`left: ${SX - SIZE / 2}px; top: ${SY - SIZE / 2}px`}></div>
      <div class="disc" bind:this={discEl} style={`left: ${SX - SIZE / 2}px; top: ${SY - SIZE / 2}px`}>
        <div class="spin" bind:this={spinEl}>
          <Disc {game} {version} {title} />
          {#if print}
            <canvas class="smear" width="640" height="640" bind:this={arc}></canvas>
            <canvas class="smear" width="640" height="640" bind:this={ring}></canvas>
          {/if}
          <span class="blur" bind:this={blurEl}></span>
        </div>
      </div>
      <div class="lid" bind:this={lidEl}><span class="lit" bind:this={litEl}></span></div>
      <div class="led" bind:this={ledEl}></div>
    </div>
  </div>
  <div class="hole" bind:this={holeEl} style={`width: ${HOLE}px; height: ${HOLE}px`}></div>
  <div class="all" bind:this={allEl}></div>
</div>

<style>
  .launch {
    position: fixed;
    inset: 0;
    z-index: 1000;
    outline: none;
    overflow: hidden;
    contain: strict;
  }

  .ink,
  .light,
  .stage,
  .all {
    position: absolute;
    inset: 0;
    opacity: 0;
    will-change: opacity;
  }

  .ink {
    background: var(--ink);
  }

  .light {
    background: radial-gradient(50% 60% at 50% 54%, var(--glow, transparent), transparent 75%);
  }

  .all {
    background: rgb(0 0 0);
  }

  .console {
    position: absolute;
    width: 860px;
    height: 560px;
    transform-origin: 0 0;
    transform-style: preserve-3d;
    will-change: transform;
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

  .disc,
  .shade {
    position: absolute;
    width: 420px;
    height: 420px;
    will-change: transform;
  }

  .spin {
    position: absolute;
    inset: 0;
    will-change: transform;
  }

  .smear {
    position: absolute;
    opacity: 0;
    inset: 0;
    width: 100%;
    height: 100%;
    mask: radial-gradient(circle closest-side, transparent 0 12.6%, black 13% 99.2%, transparent 100%);
  }

  .shade {
    opacity: 0;
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
    opacity: 0;
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
    will-change: transform;
  }

  .lit {
    position: absolute;
    inset: 0;
    border-radius: inherit;
    background: rgb(255 255 255 / 0.1);
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

  /* Set from the frame loop (paint), not the markup. */
  .led:global(.on) {
    background: rgb(93 255 122);
  }

  /* Drawn once at its small size and scaled up: its edge is soft anyway. */
  .hole {
    position: absolute;
    margin: -128px 0 0 -128px;
    border-radius: 50%;
    background: radial-gradient(circle closest-side, rgb(0 0 0) 72%, transparent);
    opacity: 0;
    will-change: transform;
  }
</style>
