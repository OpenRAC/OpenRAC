<script lang="ts">
  import { untrack } from "svelte";

  // Behind Android's Games: a ribbon of light in the game's colour, like silk or a wave of water,
  // folding and drifting slowly across the screen, with specks of light about it. It slides along
  // as the wheel of discs turns (`shift`, in discs). One still frame when motion is reduced.
  // Drawn by the GPU at the screen's own pixels (WebGL): each sheet of the cloth is worked out per
  // pixel with its edges smoothed, so it stays sharp and free of jagged steps on any screen,
  // and it costs the processor next to nothing. Where the rail lies over it (`--frost` from the
  // left, CSS), the cloth is drawn soft, as through frosted glass.
  let { shift = 0, still = false }: { shift?: number; still?: boolean } = $props();

  let canvas = $state<HTMLCanvasElement | null>(null);

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

  /** The middle and half-width of sheet `s` at `u` across (0 to 1), in shares of the height: the
   *  shader's own sum, for the specks that ride the first sheet. */
  function sheet(s: (typeof SHEETS)[number], u: number, a: [number, number, number]): [number, number] {
    const mid = s.mid + Math.sin(u * 2.3 + a[0]) * s.rise + Math.sin(u * 4.1 + a[1]) * s.rise * 0.45;
    const half = s.wide * (0.08 + 0.92 * Math.abs(Math.cos(u * 1.9 + a[2])));
    return [mid, half];
  }

  /** Specks of light about the ribbon: where along it, how far off it, size, pace, twinkle. */
  const SPECKS = Array.from({ length: 70 }, (_, i) => {
    const r = (n: number) => (((Math.sin(i * 12.9898 + n * 78.233) * 43758.5453) % 1) + 1) % 1;
    return { u: r(1), off: (r(2) - 0.5) * 2, size: 0.5 + r(3) ** 3 * 2, pace: 0.004 + r(4) * 0.012, tw: r(5) * 6.28 };
  });

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
  float frost = 1.0 - smoothstep(uFrost - 16.0 * uDpr, uFrost + 16.0 * uDpr, x);
  float soft = frost * 16.0 * uDpr;
  float light = sheet(u, y, H, 0.63, 0.09, 0.10, uA0, soft)
              + sheet(u, y, H, 0.66, 0.07, 0.07, uA1, soft) * 0.7
              + sheet(u, y, H, 0.60, 0.06, 0.05, uA2, soft) * 0.55;
  light *= 1.0 - 0.5 * frost;
  vec3 cloth = (uCol + (1.0 - uCol) * 0.55) * light;
  gl_FragColor = vec4(cloth, clamp(max(cloth.r, max(cloth.g, cloth.b)), 0.0, 1.0));
}`;

  const SPECK_VS = `
attribute vec4 s;
uniform vec2 uRes;
varying float vA;
varying float vR;
void main() {
  gl_Position = vec4(s.x / uRes.x * 2.0 - 1.0, 1.0 - s.y / uRes.y * 2.0, 0.0, 1.0);
  vR = s.z;
  vA = s.w;
  gl_PointSize = s.z * 2.0 + 3.0;
}`;

  const SPECK_FS = `
precision mediump float;
uniform vec3 uCol;
varying float vA;
varying float vR;
void main() {
  float d = length(gl_PointCoord - 0.5) * (vR * 2.0 + 3.0);
  float a = vA * (1.0 - smoothstep(vR - 0.6, vR + 0.6, d));
  gl_FragColor = vec4((uCol + (1.0 - uCol) * 0.85) * a, a);
}`;

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

  interface Gl {
    gl: WebGLRenderingContext;
    cloth: WebGLProgram;
    specks: WebGLProgram;
    quad: WebGLBuffer;
    points: WebGLBuffer;
    /** Where each program's inputs are, looked up once. */
    at: Record<string, WebGLUniformLocation | null>;
    p: number;
    s: number;
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
    const gl = c.getContext("webgl", { alpha: true, premultipliedAlpha: true, antialias: false, depth: false });
    if (!gl) return null;
    const cloth = program(gl, QUAD_VS, CLOTH_FS);
    const specks = program(gl, SPECK_VS, SPECK_FS);
    const quad = gl.createBuffer();
    const points = gl.createBuffer();
    if (!cloth || !specks) return null;
    gl.bindBuffer(gl.ARRAY_BUFFER, quad);
    // Only the band the cloth can reach (36% to 92% of the height, glow included): the rest of
    // the screen is never shaded.
    const top = 1 - 2 * 0.36,
      bottom = 1 - 2 * 0.92;
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, bottom, 1, bottom, -1, top, 1, top]), gl.STATIC_DRAW);
    const at: Gl["at"] = {};
    for (const name of ["uRes", "uDpr", "uAlong", "uFrost", "uCol", "uA0", "uA1", "uA2"])
      at[name] = gl.getUniformLocation(cloth, name);
    at.sRes = gl.getUniformLocation(specks, "uRes");
    at.sCol = gl.getUniformLocation(specks, "uCol");
    return {
      gl,
      cloth,
      specks,
      quad,
      points,
      at,
      p: gl.getAttribLocation(cloth, "p"),
      s: gl.getAttribLocation(specks, "s"),
    };
  }

  $effect(() => {
    const c = canvas;
    if (!c) return;
    let g = setUp(c);
    let dpr = 1,
      frame = 0,
      last = 0,
      colour = rgb(c),
      frost = frostOf(c),
      read = 0,
      slide = untrack(() => shift);
    const speckData = new Float32Array(SPECKS.length * 4);
    const size = () => {
      // Sharp at the screen's own pixels, up to twice the CSS ones (a 3x phone looks the same).
      dpr = Math.min(devicePixelRatio || 1, 2);
      c.width = Math.max(1, Math.round(c.clientWidth * dpr));
      c.height = Math.max(1, Math.round(c.clientHeight * dpr));
      frost = frostOf(c);
    };
    const draw = (t: number) => {
      if (!g) return;
      const { gl, cloth, specks, quad, points, at, p, s: sAt } = g;
      const w = c.width,
        h = c.height;
      const col = colour.map((v) => v / 255) as [number, number, number];
      const along = slide * 0.08;
      gl.viewport(0, 0, w, h);
      gl.clearColor(0, 0, 0, 0);
      gl.clear(gl.COLOR_BUFFER_BIT);
      gl.disable(gl.BLEND);
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
      // The specks, near the ribbon, drifting along it and twinkling; softer under the rail.
      const main = SHEETS[0],
        a0 = a[0];
      if (!a0) return;
      SPECKS.forEach((sp, i) => {
        const u = (sp.u + t * sp.pace) % 1;
        const [mid, half] = sheet(main, u + along, a0);
        const x = u * w;
        const fr = x < frost * dpr ? 1 : 0;
        speckData[i * 4] = x;
        speckData[i * 4 + 1] = (mid + sp.off * (half + 0.05)) * h;
        speckData[i * 4 + 2] = sp.size * dpr * (1 + fr);
        speckData[i * 4 + 3] = (0.2 + 0.6 * Math.sin(t * 1.3 + sp.tw) ** 4) * (1 - 0.6 * fr);
      });
      gl.enable(gl.BLEND);
      gl.blendFunc(gl.ONE, gl.ONE);
      gl.useProgram(specks);
      gl.bindBuffer(gl.ARRAY_BUFFER, points);
      gl.bufferData(gl.ARRAY_BUFFER, speckData, gl.DYNAMIC_DRAW);
      gl.enableVertexAttribArray(sAt);
      gl.vertexAttribPointer(sAt, 4, gl.FLOAT, false, 0, 0);
      gl.uniform2f(at.sRes ?? null, w, h);
      gl.uniform3f(at.sCol ?? null, col[0], col[1], col[2]);
      gl.drawArrays(gl.POINTS, 0, SPECKS.length);
      gl.disableVertexAttribArray(sAt);
    };
    const start = performance.now();
    const tick = (now: number) => {
      frame = requestAnimationFrame(tick);
      // Slow drift: 60 frames a second are plenty, also on a 120 Hz screen.
      if (document.hidden || now - last < 15) return;
      last = now;
      // The colour follows the game's as it changes (a transition), read a few times a second.
      if (now - read > 120) {
        colour = rgb(c);
        read = now;
      }
      slide += (shift - slide) * 0.08;
      draw(10 + (now - start) / 1000);
    };
    const run = () => {
      if (still) draw(10);
      else frame = requestAnimationFrame(tick);
    };
    const observer = new ResizeObserver(() => {
      size();
      if (still) draw(10);
    });
    // A context the system took back (memory, a driver reset): drawn again once it is returned.
    const lost = (e: Event) => {
      e.preventDefault();
      cancelAnimationFrame(frame);
      g = null;
    };
    const restored = () => {
      g = setUp(c);
      run();
    };
    c.addEventListener("webglcontextlost", lost);
    c.addEventListener("webglcontextrestored", restored);
    observer.observe(c);
    size();
    run();
    return () => {
      cancelAnimationFrame(frame);
      observer.disconnect();
      c.removeEventListener("webglcontextlost", lost);
      c.removeEventListener("webglcontextrestored", restored);
    };
  });
</script>

<canvas bind:this={canvas} aria-hidden="true"></canvas>

<style>
  /* The deep below the wave holds a little of its light. */
  canvas {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    background: linear-gradient(transparent 45%, color-mix(in srgb, var(--glow) 13%, transparent));
  }
</style>
