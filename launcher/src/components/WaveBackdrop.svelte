<script lang="ts" module>
  // The wave's canvas and its WebGL context, made once and kept while the launcher runs. Games is
  // built again each time its tab opens; a new context each time (made by the GPU process, its
  // shader compiled and linked) held the tap up for tens of ms, and the old ones piled up until
  // the browser dropped them.

  const QUAD_VS = `
attribute vec2 p;
void main() { gl_Position = vec4(p, 0.0, 1.0); }`;

  // The cloth: each sheet a smooth band of light, no threads, its edges soft lines.
  const CLOTH_FS = `
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
uniform vec2 uRes;
uniform float uDpr;
uniform float uAlong;
uniform float uFrost;
uniform vec3 uCol;
uniform vec3 uA0;
uniform vec3 uA1;
uniform vec3 uA2;

float sheet(float u, float y, float H, float m, float rise, float wide, vec3 a, float soft) {
  float mid = m + sin(u * 2.3 + a.x) * rise + sin(u * 4.1 + a.y) * rise * 0.45;
  float halfW = wide * (0.08 + 0.92 * abs(cos(u * 1.9 + a.z)));
  float span = max(2.0 * halfW * H, 0.001);
  float v = (y - mid * H) / span;
  // Brighter where it folds (narrow), as a cloth turned edge-on.
  float crowd = clamp(0.03 / halfW, 0.3, 6.0);
  // The sheet: one smooth fill, its edge smoothed over a pixel (wider through frosted glass),
  // brighter towards its edges than in its middle.
  float aa = (uDpr + soft) / span;
  float inside = 1.0 - smoothstep(0.5 - aa, 0.5 + aa, abs(v));
  float body = inside * (0.55 + 0.45 * abs(2.0 * v)) * crowd * 0.13;
  // Its upper edge catches the light, its lower one a little: soft lines, never a hard step.
  float lw = 1.2 * uDpr + soft;
  float keep = 1.2 * uDpr / lw;
  float upper = exp(-pow((v + 0.5) * span / lw, 2.0)) * 0.16 * keep;
  float lower = exp(-pow((v - 0.5) * span / lw, 2.0)) * 0.08 * keep;
  // And a glow about it.
  float out_ = max(abs(v) - 0.5, 0.0) * span;
  float glow = exp(-out_ / (22.0 * uDpr + soft)) * 0.025 * crowd;
  return body + (upper + lower) * min(crowd, 2.5) + glow;
}

void main() {
  float x = gl_FragCoord.x;
  float y = uRes.y - gl_FragCoord.y;
  float H = uRes.y;
  float u = x / uRes.x + uAlong;
  // Under the rail: through frosted glass, its edges wide and faint, the whole dimmer.
  // Full under the rail, then easing out over about a hundred pixels past its edge (smootherstep).
  float e = clamp((x - uFrost + 24.0 * uDpr) / (110.0 * uDpr), 0.0, 1.0);
  float frost = 1.0 - e * e * e * (e * (e * 6.0 - 15.0) + 10.0);
  float soft = frost * 16.0 * uDpr;
  float light = sheet(u, y, H, 0.63, 0.09, 0.10, uA0, soft)
              + sheet(u, y, H, 0.66, 0.07, 0.07, uA1, soft) * 0.7
              + sheet(u, y, H, 0.60, 0.06, 0.05, uA2, soft) * 0.55;
  light *= 1.0 - 0.5 * frost;
  vec3 cloth = (uCol + (1.0 - uCol) * 0.55) * light;
  gl_FragColor = vec4(cloth, clamp(max(cloth.r, max(cloth.g, cloth.b)), 0.0, 1.0));
}`;

  interface Gl {
    gl: WebGLRenderingContext;
    cloth: WebGLProgram;
    quad: WebGLBuffer;
    /** Where the program's inputs are, looked up once. */
    at: Record<string, WebGLUniformLocation | null>;
    p: number;
  }

  function program(gl: WebGLRenderingContext, vs: string, fs: string): WebGLProgram | null {
    const p = gl.createProgram();
    for (const [type, src] of [
      [gl.VERTEX_SHADER, vs],
      [gl.FRAGMENT_SHADER, fs],
    ] as const) {
      const sh = gl.createShader(type);
      if (!sh) return null;
      gl.shaderSource(sh, src);
      gl.compileShader(sh);
      gl.attachShader(p, sh);
    }
    gl.linkProgram(p);
    return gl.getProgramParameter(p, gl.LINK_STATUS) ? p : null;
  }

  function setUp(c: HTMLCanvasElement): Gl | null {
    const gl = c.getContext("webgl", {
      alpha: true,
      premultipliedAlpha: true,
      antialias: false,
      depth: false,
      // A backdrop: the battery's GPU where there are two.
      powerPreference: "low-power",
    });
    if (!gl) return null;
    const cloth = program(gl, QUAD_VS, CLOTH_FS);
    const quad = gl.createBuffer();
    if (!cloth) return null;
    gl.bindBuffer(gl.ARRAY_BUFFER, quad);
    // Only the band the cloth can reach (36% to 92% of the height, glow included): the rest of
    // the screen is never shaded.
    const top = 1 - 2 * 0.36,
      bottom = 1 - 2 * 0.92;
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, bottom, 1, bottom, -1, top, 1, top]), gl.STATIC_DRAW);
    const at: Gl["at"] = {};
    for (const name of ["uRes", "uDpr", "uAlong", "uFrost", "uCol", "uA0", "uA1", "uA2"])
      at[name] = gl.getUniformLocation(cloth, name);
    return {
      gl,
      cloth,
      quad,
      at,
      p: gl.getAttribLocation(cloth, "p"),
    };
  }

  /** `frozen`: a still copy of the wave shown while a page slides (motion.ts): Android's WebView
   *  leaves a WebGL canvas out of the slide's pictures, so the wave vanished as the slide started
   *  and popped back in one frame after it. A plain canvas is in them. */
  let kept: { canvas: HTMLCanvasElement; frozen: HTMLCanvasElement; g: Gl | null } | null = null;
</script>

<script lang="ts">
  import { untrack } from "svelte";

  // Behind Android's Games: a ribbon of light in the game's colour, like silk or a wave of water,
  // folding and drifting slowly across the screen. It slides along
  // as the wheel of discs turns (`shift`, in discs). One still frame when motion is reduced.
  // Drawn by the GPU at the screen's own pixels (WebGL): each sheet of the cloth is worked out per
  // pixel with its edges smoothed, so it stays sharp and free of jagged steps on any screen,
  // and it costs the processor next to nothing. Where the rail lies over it (`--frost` from the
  // left, CSS), the cloth is drawn soft, as through frosted glass.
  let { shift = 0, still = false }: { shift?: number; still?: boolean } = $props();

  let host = $state<HTMLElement | null>(null);

  /** The ribbon's sheets, each a fold of the same cloth: where it lies (share of the height), how
   *  far it rises and falls, how wide it is, its own pace and phase, and how bright it is. */
  const SHEETS = [
    { mid: 0.63, rise: 0.09, wide: 0.1, pace: 1, phase: 0, light: 1 },
    { mid: 0.66, rise: 0.07, wide: 0.07, pace: 0.8, phase: 2.1, light: 0.7 },
    { mid: 0.6, rise: 0.06, wide: 0.05, pace: 1.25, phase: 4.2, light: 0.55 },
  ] as const;
  const TAU = Math.PI * 2;

  /** The three moving angles of sheet `s` at time `t`, wrapped, so the GPU's sines stay exact
   *  however long the page is open. */
  function angles(s: (typeof SHEETS)[number], t: number): [number, number, number] {
    const q = t * s.pace;
    const wrap = (a: number, p: number) => ((a % p) + p) % p;
    return [
      wrap(q * 0.21 + s.phase, TAU),
      wrap(-q * 0.13 + s.phase * 1.7, TAU),
      wrap(-q * 0.11 + s.phase * 0.6, Math.PI),
    ];
  }

  /** The colour as numbers: --glow is a registered colour, so it reads back as rgb(). */
  function rgb(el: Element): [number, number, number] {
    const [r, g, b] = (
      getComputedStyle(el)
        .getPropertyValue("--glow")
        .match(/[\d.]+/g) ?? []
    ).map(Number);
    return r === undefined || g === undefined || b === undefined ? [244, 190, 64] : [r, g, b];
  }

  /** How far from the left the rail lies over the cloth, in CSS pixels (--frost, 0 if none). */
  function frostOf(el: Element): number {
    return parseFloat(getComputedStyle(el).getPropertyValue("--frost")) || 0;
  }

  $effect(() => {
    const holder = host;
    if (!holder) return;
    kept ??= { canvas: document.createElement("canvas"), frozen: document.createElement("canvas"), g: null };
    const k = kept;
    const c = k.canvas;
    c.className = "live";
    k.frozen.className = "frozen";
    holder.append(c, k.frozen);
    if (!k.g || k.g.gl.isContextLost()) k.g = setUp(c);
    let g = k.g;
    let dpr = 1,
      frame = 0,
      last = 0,
      colour = rgb(c),
      frost = frostOf(c),
      read = 0,
      // Coming in, it moves on as after one turn of the wheel down (a short push), while the page
      // fades it in (app.css, `wave`).
      slide = untrack(() => (still ? shift : shift - 1)),
      // A slow GPU: drawn at the CSS pixels instead (see `pace`), for good.
      cap = 2,
      drawn = 0,
      since = 0,
      /** The time of the last frame drawn, for the still copy. */
      at = 10;
    const size = () => {
      // Sharp at the screen's own pixels, up to twice the CSS ones (a 3x phone looks the same).
      dpr = Math.min(devicePixelRatio || 1, cap);
      // Only when it changed: setting either clears the picture, which a canvas kept from the last
      // visit still shows until its first frame here.
      const w = Math.max(1, Math.round(c.clientWidth * dpr)),
        h = Math.max(1, Math.round(c.clientHeight * dpr));
      if (c.width !== w) c.width = w;
      if (c.height !== h) c.height = h;
      frost = frostOf(c);
    };
    const draw = (t: number) => {
      if (!g) return;
      const { gl, cloth, quad, at, p } = g;
      const w = c.width,
        h = c.height;
      const col = colour.map((v) => v / 255) as [number, number, number];
      const along = slide * 0.08;
      gl.viewport(0, 0, w, h);
      gl.clearColor(0, 0, 0, 0);
      gl.clear(gl.COLOR_BUFFER_BIT);
      gl.useProgram(cloth);
      gl.bindBuffer(gl.ARRAY_BUFFER, quad);
      gl.enableVertexAttribArray(p);
      gl.vertexAttribPointer(p, 2, gl.FLOAT, false, 0, 0);
      gl.uniform2f(at.uRes ?? null, w, h);
      gl.uniform1f(at.uDpr ?? null, dpr);
      gl.uniform1f(at.uAlong ?? null, along);
      gl.uniform1f(at.uFrost ?? null, frost * dpr);
      gl.uniform3f(at.uCol ?? null, col[0], col[1], col[2]);
      const a = SHEETS.map((sh) => angles(sh, t));
      a.forEach((v, i) => {
        gl.uniform3f(at[`uA${i}`] ?? null, v[0], v[1], v[2]);
      });
      gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
      gl.disableVertexAttribArray(p);
    };
    const start = performance.now();
    const tick = (now: number) => {
      frame = requestAnimationFrame(tick);
      // 60 frames a second while it follows the wheel or comes in with its push; the slow drift
      // alone, 30 (user decision: half the GPU work on a shelf left alone, same picture).
      const moving = Math.abs(shift - slide) > 0.002 || now - start < 2000;
      if (document.hidden || now - last < (moving ? 15 : 31)) return;
      const step = Math.min(now - last, 100) / (1000 / 60);
      last = now;
      // The colour follows the game's as it changes (a transition), read a few times a second.
      if (now - read > 120) {
        colour = rgb(c);
        read = now;
      }
      // After the wheel at the same pace whatever the frame rate: 8 % of the way each 60th of a second.
      slide += (shift - slide) * (1 - Math.pow(0.92, step));
      // The weak-GPU check counts 60-a-second frames only.
      if (!moving) drawn = 0;
      // The push: about three times its pace at first, back to its own within a second or so.
      const rush = still ? 0 : 1.2 * (1 - Math.exp(-(now - start) / 600));
      at = 10 + (now - start) / 1000 + rush;
      // While a page slides the still copy shows: nothing to draw.
      if (sliding()) return;
      draw(at);
      pace(now);
    };
    const sliding = () => "sliding" in document.documentElement.dataset;
    /** The wave as it is now, copied into the plain canvas the slide's pictures show. Drawn and
     *  copied in one go: a WebGL canvas can be read only before its frame is shown. */
    const freeze = () => {
      if (!g) return;
      const f = k.frozen;
      if (f.width !== c.width) f.width = c.width;
      if (f.height !== c.height) f.height = c.height;
      draw(at);
      const ctx = f.getContext("2d");
      if (!ctx) return;
      ctx.clearRect(0, 0, f.width, f.height);
      ctx.drawImage(c, 0, 0);
    };
    // Every 90 frames drawn: if they came under 36 a second (a weak GPU, the shader at twice the
    // pixels too much for it), the cloth is drawn at the CSS pixels from then on, a quarter of the
    // work. Once only; a hidden page does not count.
    const pace = (now: number) => {
      if (cap === 1) return;
      if (drawn === 0) since = now;
      if (++drawn < 90) return;
      const slow = (now - since) / (drawn - 1) > 28;
      drawn = 0;
      if (slow && dpr > 1) {
        cap = 1;
        size();
      }
    };
    const shown = () => {
      drawn = 0;
    };
    const run = () => {
      if (still) draw(10);
      else frame = requestAnimationFrame(tick);
    };
    document.addEventListener("slidestart", freeze);
    const observer = new ResizeObserver(() => {
      size();
      if (still) draw(10);
    });
    // A context the system took back (memory, a driver reset): drawn again once it is returned.
    const lost = (e: Event) => {
      e.preventDefault();
      cancelAnimationFrame(frame);
      g = k.g = null;
    };
    const restored = () => {
      g = k.g = setUp(c);
      run();
    };
    c.addEventListener("webglcontextlost", lost);
    c.addEventListener("webglcontextrestored", restored);
    document.addEventListener("visibilitychange", shown);
    observer.observe(c);
    size();
    // Coming in with a slide: the still copy for the slide's picture of this page.
    if (sliding()) freeze();
    run();
    return () => {
      cancelAnimationFrame(frame);
      observer.disconnect();
      c.removeEventListener("webglcontextlost", lost);
      c.removeEventListener("webglcontextrestored", restored);
      document.removeEventListener("visibilitychange", shown);
      document.removeEventListener("slidestart", freeze);
      // The canvases, the context and its program stay for the next visit.
      c.remove();
      k.frozen.remove();
    };
  });
</script>

<div class="wave" bind:this={host} aria-hidden="true"></div>

<style>
  /* The canvases are made in code (kept between visits), so they are styled through their holder. */
  .wave,
  .wave :global(canvas) {
    position: absolute;
    inset: 0;
  }

  /* The deep below the wave holds a little of its light. */
  .wave :global(canvas) {
    width: 100%;
    height: 100%;
    background: linear-gradient(transparent 45%, color-mix(in srgb, var(--glow) 13%, transparent));
  }

  /* While a page slides (motion.ts): the still copy in place of the live wave. */
  .wave :global(canvas.frozen),
  :global(html[data-sliding]) .wave :global(canvas.live) {
    visibility: hidden;
  }

  :global(html[data-sliding]) .wave :global(canvas.frozen) {
    visibility: visible;
  }
</style>
